/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.android.tradefed.invoker.shard;

import com.android.ddmlib.Log.LogLevel;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.IConfiguration;
import com.android.tradefed.config.IConfigurationReceiver;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.DeviceUnresponsiveException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.device.metric.IMetricCollector;
import com.android.tradefed.device.metric.IMetricCollectorReceiver;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.log.ILogRegistry.EventType;
import com.android.tradefed.log.LogRegistry;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.suite.checker.ISystemStatusChecker;
import com.android.tradefed.suite.checker.ISystemStatusCheckerReceiver;
import com.android.tradefed.testtype.IBuildReceiver;
import com.android.tradefed.testtype.IDeviceTest;
import com.android.tradefed.testtype.IInvocationContextReceiver;
import com.android.tradefed.testtype.IMultiDeviceTest;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.testtype.ITestCollector;
import com.android.tradefed.util.StreamUtil;

import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.CountDownLatch;

/**
 * Tests wrapper that allow to execute all the tests of a pool of tests. Tests can be shared by
 * another {@link TestsPoolPoller} so synchronization is required.
 *
 * <p>TODO: Add handling for token module/tests.
 */
public class TestsPoolPoller
        implements IRemoteTest,
                IConfigurationReceiver,
                IDeviceTest,
                IBuildReceiver,
                IMultiDeviceTest,
                IInvocationContextReceiver,
                ISystemStatusCheckerReceiver,
                ITestCollector,
                IMetricCollectorReceiver {

    private static final long WAIT_RECOVERY_TIME = 15 * 60 * 1000;

    private Collection<IRemoteTest> mGenericPool;
    private CountDownLatch mTracker;

    private ITestDevice mDevice;
    private IBuildInfo mBuildInfo;
    private IInvocationContext mContext;
    private Map<ITestDevice, IBuildInfo> mDeviceInfos;
    private IConfiguration mConfig;
    private List<ISystemStatusChecker> mSystemStatusCheckers;
    private List<IMetricCollector> mCollectors;
    private boolean mShouldCollectTest = false;

    /**
     * Ctor where the pool of {@link IRemoteTest} is provided.
     *
     * @param tests {@link IRemoteTest}s pool of all tests.
     * @param tracker a {@link CountDownLatch} shared to get the number of running poller.
     */
    public TestsPoolPoller(Collection<IRemoteTest> tests, CountDownLatch tracker) {
        mGenericPool = tests;
        mTracker = tracker;
    }

    /** Returns the first {@link IRemoteTest} from the pool or null if none remaining. */
    IRemoteTest poll() {
        synchronized (mGenericPool) {
            if (mGenericPool.isEmpty()) {
                return null;
            }
            IRemoteTest test = mGenericPool.iterator().next();
            mGenericPool.remove(test);
            return test;
        }
    }

    /** {@inheritDoc} */
    @Override
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
        try {
            ITestInvocationListener listenerWithCollectors = listener;
            for (IMetricCollector collector : mCollectors) {
                listenerWithCollectors = collector.init(mContext, listenerWithCollectors);
            }
            while (true) {
                IRemoteTest test = poll();
                if (test == null) {
                    return;
                }
                if (test instanceof IBuildReceiver) {
                    ((IBuildReceiver) test).setBuild(mBuildInfo);
                }
                if (test instanceof IConfigurationReceiver) {
                    ((IConfigurationReceiver) test).setConfiguration(mConfig);
                }
                if (test instanceof IDeviceTest) {
                    ((IDeviceTest) test).setDevice(mDevice);
                }
                if (test instanceof IInvocationContextReceiver) {
                    ((IInvocationContextReceiver) test).setInvocationContext(mContext);
                }
                if (test instanceof IMultiDeviceTest) {
                    ((IMultiDeviceTest) test).setDeviceInfos(mDeviceInfos);
                }
                if (test instanceof ISystemStatusCheckerReceiver) {
                    ((ISystemStatusCheckerReceiver) test)
                            .setSystemStatusChecker(mSystemStatusCheckers);
                }
                if (test instanceof ITestCollector) {
                    ((ITestCollector) test).setCollectTestsOnly(mShouldCollectTest);
                }
                // Run the test itself and prevent random exception from stopping the poller.
                try {
                    if (test instanceof IMetricCollectorReceiver) {
                        ((IMetricCollectorReceiver) test).setMetricCollectors(mCollectors);
                        // If test can receive collectors then let it handle the how to set them up
                        test.run(listener);
                    } else {
                        test.run(listenerWithCollectors);
                    }
                } catch (RuntimeException e) {
                    CLog.e(
                            "Caught an Exception in a test: %s. Proceeding to next test.",
                            test.getClass());
                    CLog.e(e);
                } catch (DeviceUnresponsiveException due) {
                    // being able to catch a DeviceUnresponsiveException here implies that recovery
                    // was successful, and test execution should proceed to next test.
                    CLog.w(
                            "Ignored DeviceUnresponsiveException because recovery was "
                                    + "successful, proceeding with next test. Stack trace:");
                    CLog.w(due);
                    CLog.w("Proceeding to the next test.");
                } catch (DeviceNotAvailableException dnae) {
                    HandleDeviceNotAvailable(dnae, test);
                }
            }
        } finally {
            mTracker.countDown();
        }
    }

    /**
     * Helper to wait for the device to maybe come back online, in that case we reboot it to refresh
     * the state and proceed with execution.
     */
    void HandleDeviceNotAvailable(DeviceNotAvailableException originalException, IRemoteTest test)
            throws DeviceNotAvailableException {
        try {
            if (mTracker.getCount() > 1) {
                CLog.d("Wait 5 min for device to maybe coming back online.");
                mDevice.waitForDeviceAvailable(WAIT_RECOVERY_TIME);
                mDevice.reboot();
                CLog.d("TestPoller was recovered after %s went offline", mDevice.getSerialNumber());
                return;
            }
            // We catch and rethrow in order to log that the poller associated with the device
            // that went offline is terminating.
            CLog.e(
                    "Test %s threw DeviceNotAvailableException. Test poller associated with "
                            + "device %s is terminating.",
                    test.getClass(), mDevice.getSerialNumber());
            // Log an event to track more easily the failure
            logDeviceEvent(
                    EventType.SHARD_POLLER_EARLY_TERMINATION,
                    mDevice.getSerialNumber(),
                    originalException);
        } catch (DeviceNotAvailableException e) {
            // ignore this exception
        }
        throw originalException;
    }

    /** Helper to log the device events. */
    private void logDeviceEvent(EventType event, String serial, Throwable t) {
        Map<String, String> args = new HashMap<>();
        args.put("serial", serial);
        args.put("trace", StreamUtil.getStackTrace(t));
        LogRegistry.getLogRegistry().logEvent(LogLevel.DEBUG, event, args);
    }

    @Override
    public void setBuild(IBuildInfo buildInfo) {
        mBuildInfo = buildInfo;
    }

    @Override
    public void setDevice(ITestDevice device) {
        mDevice = device;
    }

    @Override
    public ITestDevice getDevice() {
        return mDevice;
    }

    @Override
    public void setInvocationContext(IInvocationContext invocationContext) {
        mContext = invocationContext;
    }

    @Override
    public void setDeviceInfos(Map<ITestDevice, IBuildInfo> deviceInfos) {
        mDeviceInfos = deviceInfos;
    }

    @Override
    public void setConfiguration(IConfiguration configuration) {
        mConfig = configuration;
    }

    @Override
    public void setSystemStatusChecker(List<ISystemStatusChecker> systemCheckers) {
        mSystemStatusCheckers = systemCheckers;
    }

    @Override
    public void setCollectTestsOnly(boolean shouldCollectTest) {
        mShouldCollectTest = shouldCollectTest;
    }

    @Override
    public void setMetricCollectors(List<IMetricCollector> collectors) {
        mCollectors = collectors;
    }
}
