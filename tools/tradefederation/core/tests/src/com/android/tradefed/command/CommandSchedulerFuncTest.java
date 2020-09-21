/*
 * Copyright (C) 2010 The Android Open Source Project
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

package com.android.tradefed.command;

import static org.junit.Assert.*;

import com.android.ddmlib.IDevice;
import com.android.ddmlib.Log;
import com.android.tradefed.config.ConfigurationDescriptor;
import com.android.tradefed.config.DeviceConfigurationHolder;
import com.android.tradefed.config.IConfiguration;
import com.android.tradefed.config.IConfigurationFactory;
import com.android.tradefed.config.IDeviceConfiguration;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.DeviceSelectionOptions;
import com.android.tradefed.device.IDeviceManager;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.device.MockDeviceManager;
import com.android.tradefed.device.StubDevice;
import com.android.tradefed.device.TestDeviceOptions;
import com.android.tradefed.device.TestDeviceState;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.invoker.IRescheduler;
import com.android.tradefed.invoker.ITestInvocation;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.util.RunInterruptedException;
import com.android.tradefed.util.RunUtil;
import com.android.tradefed.util.keystore.IKeyStoreClient;

import com.google.common.util.concurrent.SettableFuture;

import org.easymock.EasyMock;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.Future;

/** Longer running test for {@link CommandScheduler} */
@RunWith(JUnit4.class)
public class CommandSchedulerFuncTest {

    private static final String LOG_TAG = "CommandSchedulerFuncTest";
    private static final long WAIT_TIMEOUT_MS = 30 * 1000;
    /** the {@link CommandScheduler} under test, with all dependencies mocked out */
    private CommandScheduler mCommandScheduler;
    private MeasuredInvocation mMockTestInvoker;
    private MockDeviceManager mMockDeviceManager;
    private List<IDeviceConfiguration> mMockDeviceConfig;
    private IConfiguration mSlowConfig;
    private IConfiguration mFastConfig;
    private IConfigurationFactory mMockConfigFactory;
    private CommandOptions mCommandOptions;
    private DeviceSelectionOptions mDeviceOptions;
    private boolean mInterruptible = false;
    private IDeviceConfiguration mMockConfig;

    @Before
    public void setUp() throws Exception {
        mDeviceOptions = new DeviceSelectionOptions();
        mMockDeviceConfig = new ArrayList<IDeviceConfiguration>();
        mMockConfig = new DeviceConfigurationHolder("device");
        mMockConfig.addSpecificConfig(mDeviceOptions);
        mMockConfig.addSpecificConfig(new TestDeviceOptions());
        mMockDeviceConfig.add(mMockConfig);

        mInterruptible = false;
        mSlowConfig = EasyMock.createNiceMock(IConfiguration.class);
        mFastConfig = EasyMock.createNiceMock(IConfiguration.class);
        mMockDeviceManager = new MockDeviceManager(1);
        mMockTestInvoker = new MeasuredInvocation();
        mMockConfigFactory = EasyMock.createMock(IConfigurationFactory.class);
        mCommandOptions = new CommandOptions();
        mCommandOptions.setLoopMode(true);
        mCommandOptions.setMinLoopTime(0);
        EasyMock.expect(mSlowConfig.getCommandOptions()).andStubReturn(mCommandOptions);
        EasyMock.expect(mSlowConfig.getTestInvocationListeners())
                .andStubReturn(new ArrayList<ITestInvocationListener>());
        EasyMock.expect(mFastConfig.getCommandOptions()).andStubReturn(mCommandOptions);
        EasyMock.expect(mFastConfig.getTestInvocationListeners())
                .andStubReturn(new ArrayList<ITestInvocationListener>());
        EasyMock.expect(mSlowConfig.getDeviceRequirements()).andStubReturn(
                new DeviceSelectionOptions());
        EasyMock.expect(mFastConfig.getDeviceRequirements()).andStubReturn(
                new DeviceSelectionOptions());
        EasyMock.expect(mSlowConfig.getDeviceConfig()).andStubReturn(mMockDeviceConfig);
        EasyMock.expect(mSlowConfig.getDeviceConfigByName(EasyMock.eq("device")))
                .andStubReturn(mMockConfig);
        EasyMock.expect(mSlowConfig.getCommandLine()).andStubReturn("");
        EasyMock.expect(mFastConfig.getDeviceConfigByName(EasyMock.eq("device")))
                .andStubReturn(mMockConfig);
        EasyMock.expect(mFastConfig.getDeviceConfig()).andStubReturn(mMockDeviceConfig);
        EasyMock.expect(mFastConfig.getCommandLine()).andStubReturn("");
        EasyMock.expect(mSlowConfig.getConfigurationDescription())
                .andStubReturn(new ConfigurationDescriptor());
        EasyMock.expect(mFastConfig.getConfigurationDescription())
                .andStubReturn(new ConfigurationDescriptor());

        mCommandScheduler =
                new CommandScheduler() {
                    @Override
                    ITestInvocation createRunInstance() {
                        return mMockTestInvoker;
                    }

                    @Override
                    protected IDeviceManager getDeviceManager() {
                        return mMockDeviceManager;
                    }

                    @Override
                    protected IConfigurationFactory getConfigFactory() {
                        if (mInterruptible) {
                            // simulate the invocation becoming interruptible
                            RunUtil.getDefault().allowInterrupt(true);
                        }
                        return mMockConfigFactory;
                    }

                    @Override
                    protected void initLogging() {
                        // ignore
                    }

                    @Override
                    protected void cleanUp() {
                        // ignore
                    }
                };
    }

    @After
    public void tearDown() throws Exception {
        if (mCommandScheduler != null) {
            mCommandScheduler.shutdownOnEmpty();
        }
    }

    /**
     * Test config priority scheduling. Verifies that configs are prioritized according to their
     * total run time.
     *
     * <p>This test continually executes two configs in loop mode. One config executes quickly (ie
     * "fast config"). The other config (ie "slow config") takes ~ 2 * fast config time to execute.
     *
     * <p>The run is stopped after the slow config is executed 20 times. At the end of the test, it
     * is expected that "fast config" has executed roughly twice as much as the "slow config".
     */
    @Test
    public void testRun_scheduling() throws Exception {
        String[] fastConfigArgs = new String[] {"fastConfig"};
        String[] slowConfigArgs = new String[] {"slowConfig"};
        List<String> nullArg = null;
        EasyMock.expect(
                mMockConfigFactory.createConfigurationFromArgs(EasyMock.aryEq(fastConfigArgs),
                EasyMock.eq(nullArg), (IKeyStoreClient)EasyMock.anyObject()))
                .andReturn(mFastConfig).anyTimes();
        EasyMock.expect(
                mMockConfigFactory.createConfigurationFromArgs(EasyMock.aryEq(slowConfigArgs),
                EasyMock.eq(nullArg), (IKeyStoreClient)EasyMock.anyObject()))
                .andReturn(mSlowConfig).anyTimes();

        EasyMock.replay(mFastConfig, mSlowConfig, mMockConfigFactory);
        mCommandScheduler.start();
        mCommandScheduler.addCommand(fastConfigArgs);
        mCommandScheduler.addCommand(slowConfigArgs);

        synchronized (mMockTestInvoker) {
            mMockTestInvoker.wait(WAIT_TIMEOUT_MS);
        }
        mCommandScheduler.shutdown();
        mCommandScheduler.join(WAIT_TIMEOUT_MS);

        Log.i(LOG_TAG, String.format("fast times %d slow times %d",
                mMockTestInvoker.mFastCount, mMockTestInvoker.mSlowCount));
        // assert that fast config has executed roughly twice as much as slow config. Allow for
        // some variance since the execution time of each config (governed via Thread.sleep) will
        // not be 100% accurate
        assertEquals(mMockTestInvoker.mSlowCount * 2, mMockTestInvoker.mFastCount, 5);
        assertFalse(mMockTestInvoker.runInterrupted);
    }

    private class MeasuredInvocation implements ITestInvocation {
        Integer mSlowCount = 0;
        Integer mFastCount = 0;
        Integer mSlowCountLimit = 40;
        public boolean runInterrupted = false;
        public boolean printedStop = false;

        @Override
        public void invoke(
                IInvocationContext metadata,
                IConfiguration config,
                IRescheduler rescheduler,
                ITestInvocationListener... listeners)
                throws DeviceNotAvailableException {
            try {
                if (mInterruptible) {
                    // simulate the invocation becoming interruptible
                    RunUtil.getDefault().allowInterrupt(true);
                }
                if (config.equals(mSlowConfig)) {
                    // sleep for 2 * fast config time
                    RunUtil.getDefault().sleep(200);
                    synchronized (mSlowCount) {
                        mSlowCount++;
                    }
                    if (mSlowCount >= mSlowCountLimit) {
                        synchronized (this) {
                            notify();
                        }
                    }
                } else if (config.equals(mFastConfig)) {
                    RunUtil.getDefault().sleep(100);
                    synchronized (mFastCount) {
                        mFastCount++;
                    }
                } else {
                    throw new IllegalArgumentException("unknown config");
                }
            } catch (RunInterruptedException e) {
                CLog.e(e);
                // Yield right away if an exception occur due to an interrupt.
                runInterrupted = true;
                synchronized (this) {
                    notify();
                }
            }
        }

        @Override
        public void notifyInvocationStopped() {
            printedStop = true;
        }
    }

    /** Test that the Invocation is not interruptible even when Battery is low. */
    @Test
    public void testBatteryLowLevel() throws Throwable {
        ITestDevice mockDevice = EasyMock.createNiceMock(ITestDevice.class);
        EasyMock.expect(mockDevice.getSerialNumber()).andReturn("serial").anyTimes();
        IDevice mockIDevice = new StubDevice("serial");
        EasyMock.expect(mockDevice.getIDevice()).andReturn(mockIDevice).anyTimes();
        EasyMock.expect(mockDevice.getDeviceState()).andReturn(
                TestDeviceState.ONLINE).anyTimes();

        TestDeviceOptions testDeviceOptions= new TestDeviceOptions();
        testDeviceOptions.setCutoffBattery(20);
        mMockConfig.addSpecificConfig(testDeviceOptions);
        assertTrue(testDeviceOptions.getCutoffBattery() == 20);
        EasyMock.expect(mSlowConfig.getDeviceOptions()).andReturn(testDeviceOptions).anyTimes();

        EasyMock.replay(mockDevice);
        mMockDeviceManager.clearAllDevices();
        mMockDeviceManager.addDevice(mockDevice);

        String[] slowConfigArgs = new String[] {"slowConfig"};
        List<String> nullArg = null;
        EasyMock.expect(
                mMockConfigFactory.createConfigurationFromArgs(EasyMock.aryEq(slowConfigArgs),
                EasyMock.eq(nullArg), (IKeyStoreClient)EasyMock.anyObject()))
                .andReturn(mSlowConfig).anyTimes();

        EasyMock.replay(mFastConfig, mSlowConfig, mMockConfigFactory);
        mCommandScheduler.start();
        mCommandScheduler.addCommand(slowConfigArgs);

        synchronized (mMockTestInvoker) {
            mMockTestInvoker.wait(WAIT_TIMEOUT_MS);
        }

        mCommandScheduler.shutdown();
        mCommandScheduler.join(WAIT_TIMEOUT_MS);
        assertFalse(mMockTestInvoker.runInterrupted);
        // Notify was not sent to the invocation because it was not forced shutdown.
        assertFalse(mMockTestInvoker.printedStop);
    }

    /** Test that the Invocation is interruptible when Battery is low. */
    @Test
    public void testBatteryLowLevel_interruptible() throws Throwable {
        ITestDevice mockDevice = EasyMock.createNiceMock(ITestDevice.class);
        EasyMock.expect(mockDevice.getSerialNumber()).andReturn("serial").anyTimes();
        IDevice mockIDevice = new StubDevice("serial") {
            @Override
            public Future<Integer> getBattery() {
                SettableFuture<Integer> f = SettableFuture.create();
                f.set(10);
                return f;
            }
        };

        EasyMock.expect(mockDevice.getIDevice()).andReturn(mockIDevice).anyTimes();
        EasyMock.expect(mockDevice.getDeviceState()).andReturn(
                TestDeviceState.ONLINE).anyTimes();

        TestDeviceOptions testDeviceOptions= new TestDeviceOptions();
        testDeviceOptions.setCutoffBattery(20);
        mMockConfig.addSpecificConfig(testDeviceOptions);
        EasyMock.expect(mSlowConfig.getDeviceOptions()).andReturn(testDeviceOptions).anyTimes();

        EasyMock.replay(mockDevice);
        mMockDeviceManager.clearAllDevices();
        mMockDeviceManager.addDevice(mockDevice);

        String[] slowConfigArgs = new String[] {"slowConfig"};
        List<String> nullArg = null;
        EasyMock.expect(
                mMockConfigFactory.createConfigurationFromArgs(EasyMock.aryEq(slowConfigArgs),
                EasyMock.eq(nullArg), (IKeyStoreClient)EasyMock.anyObject()))
                .andReturn(mSlowConfig).anyTimes();

        EasyMock.replay(mFastConfig, mSlowConfig, mMockConfigFactory);
        mCommandScheduler.start();
        mInterruptible = true;
        mCommandScheduler.addCommand(slowConfigArgs);

        synchronized (mMockTestInvoker) {
            mMockTestInvoker.wait(WAIT_TIMEOUT_MS);
        }

        mCommandScheduler.shutdown();
        mCommandScheduler.join(WAIT_TIMEOUT_MS);
        assertTrue(mMockTestInvoker.runInterrupted);
    }

    /**
     * Test that the Invocation is interrupted by the shutdownHard and finishes with an
     * interruption. {@link CommandScheduler#shutdownHard()}
     */
    @Test
    public void testShutdown_interruptible() throws Throwable {
        String[] slowConfigArgs = new String[] {"slowConfig"};
        List<String> nullArg = null;
        EasyMock.expect(
                mMockConfigFactory.createConfigurationFromArgs(EasyMock.aryEq(slowConfigArgs),
                EasyMock.eq(nullArg), (IKeyStoreClient)EasyMock.anyObject()))
                .andReturn(mSlowConfig).anyTimes();

        EasyMock.replay(mFastConfig, mSlowConfig, mMockConfigFactory);
        mCommandScheduler.start();
        mInterruptible = true;
        mCommandScheduler.addCommand(slowConfigArgs);

        Thread test = new Thread(new Runnable() {
            @Override
            public void run() {
                RunUtil.getDefault().sleep(500);
                mCommandScheduler.shutdownHard();
            }
        });
        test.setName("CommandSchedulerFuncTest#testShutdown_interruptible");
        test.start();
        synchronized (mMockTestInvoker) {
            mMockTestInvoker.wait(WAIT_TIMEOUT_MS);
        }
        test.join();
        mCommandScheduler.join(WAIT_TIMEOUT_MS);
        // Was interrupted during execution.
        assertTrue(mMockTestInvoker.runInterrupted);
        // Notify was sent to the invocation
        assertTrue(mMockTestInvoker.printedStop);
    }

    /**
     * Test that the Invocation is not interrupted by shutdownHard. Invocation terminate then
     * scheduler finishes. {@link CommandScheduler#shutdownHard()}
     */
    @Test
    public void testShutdown_notInterruptible() throws Throwable {
        final LongInvocation li = new LongInvocation(5);
        mCommandOptions.setLoopMode(false);
        mCommandScheduler =
                new CommandScheduler() {
                    @Override
                    ITestInvocation createRunInstance() {
                        return li;
                    }

                    @Override
                    protected IDeviceManager getDeviceManager() {
                        return mMockDeviceManager;
                    }

                    @Override
                    protected IConfigurationFactory getConfigFactory() {
                        if (mInterruptible) {
                            // simulate the invocation becoming interruptible
                            RunUtil.getDefault().allowInterrupt(true);
                        }
                        return mMockConfigFactory;
                    }

                    @Override
                    protected void initLogging() {
                        // ignore
                    }

                    @Override
                    protected void cleanUp() {
                        // ignore
                    }

                    @Override
                    public long getShutdownTimeout() {
                        return 30000;
                    }
                };
        String[] slowConfigArgs = new String[] {"slowConfig"};
        List<String> nullArg = null;
        EasyMock.expect(
                mMockConfigFactory.createConfigurationFromArgs(EasyMock.aryEq(slowConfigArgs),
                EasyMock.eq(nullArg), (IKeyStoreClient)EasyMock.anyObject()))
                .andReturn(mSlowConfig).anyTimes();

        EasyMock.replay(mFastConfig, mSlowConfig, mMockConfigFactory);
        mCommandScheduler.start();
        mInterruptible = false;
        mCommandScheduler.addCommand(slowConfigArgs);

        Thread shutdownThread = new Thread(new Runnable() {
            @Override
            public void run() {
                RunUtil.getDefault().sleep(1000);
                mCommandScheduler.shutdownHard();
            }
        });
        shutdownThread.setName("CommandSchedulerFuncTest#testShutdown_notInterruptible");
        shutdownThread.start();
        synchronized (li) {
            // Invocation will finish first because shorter than shutdownHard final timeout
            li.wait(WAIT_TIMEOUT_MS);
        }
        shutdownThread.join();
        mCommandScheduler.join(WAIT_TIMEOUT_MS);
        // Stop but was not interrupted
        assertFalse(mMockTestInvoker.runInterrupted);
        // Notify was not sent to the invocation because it was not interrupted.
        assertFalse(mMockTestInvoker.printedStop);
    }

    private class LongInvocation implements ITestInvocation {
        public boolean runInterrupted = false;
        private int mIteration = 15;

        public LongInvocation(int iteration) {
            mIteration = iteration;
        }

        @Override
        public void invoke(
                IInvocationContext metadata,
                IConfiguration config,
                IRescheduler rescheduler,
                ITestInvocationListener... listeners)
                throws DeviceNotAvailableException {
            try {
                if (mInterruptible) {
                    // simulate the invocation becoming interruptible
                    RunUtil.getDefault().allowInterrupt(true);
                }
                for (int i = 0; i < mIteration; i++) {
                    RunUtil.getDefault().sleep(2000);
                }
                synchronized (this) {
                    notify();
                }
            } catch (RunInterruptedException e) {
                CLog.e(e);
                // Yield right away if an exception occur due to an interrupt.
                runInterrupted = true;
                synchronized (this) {
                    notify();
                }
            }
        }
    }

    /**
     * Test that the Invocation is interrupted by {@link CommandScheduler#shutdownHard()} but only
     * after the shutdown timeout is expired because the invocation was uninterruptible so we only
     * allow for so much time before shutting down.
     */
    @Test
    public void testShutdown_notInterruptible_timeout() throws Throwable {
        final LongInvocation li = new LongInvocation(15);
        mCommandOptions.setLoopMode(false);
        mCommandScheduler =
                new CommandScheduler() {
                    @Override
                    ITestInvocation createRunInstance() {
                        return li;
                    }

                    @Override
                    protected IDeviceManager getDeviceManager() {
                        return mMockDeviceManager;
                    }

                    @Override
                    protected IConfigurationFactory getConfigFactory() {
                        if (mInterruptible) {
                            // simulate the invocation becoming interruptible
                            RunUtil.getDefault().allowInterrupt(true);
                        }
                        return mMockConfigFactory;
                    }

                    @Override
                    protected void initLogging() {
                        // ignore
                    }

                    @Override
                    protected void cleanUp() {
                        // ignore
                    }

                    @Override
                    public long getShutdownTimeout() {
                        return 5000;
                    }
                };
        String[] slowConfigArgs = new String[] {"slowConfig"};
        List<String> nullArg = null;
        EasyMock.expect(
                mMockConfigFactory.createConfigurationFromArgs(EasyMock.aryEq(slowConfigArgs),
                EasyMock.eq(nullArg), (IKeyStoreClient)EasyMock.anyObject()))
                .andReturn(mSlowConfig).anyTimes();

        EasyMock.replay(mFastConfig, mSlowConfig, mMockConfigFactory);
        mCommandScheduler.start();
        mInterruptible = false;
        mCommandScheduler.addCommand(slowConfigArgs);

        Thread shutdownThread = new Thread(new Runnable() {
            @Override
            public void run() {
                RunUtil.getDefault().sleep(1000);
                mCommandScheduler.shutdownHard();
            }
        });
        shutdownThread.setName("CommandSchedulerFuncTest#testShutdown_notInterruptible_timeout");
        shutdownThread.start();
        synchronized (li) {
            // Setting a timeout longer than the shutdown timeout.
            li.wait(WAIT_TIMEOUT_MS);
        }
        shutdownThread.join();
        mCommandScheduler.join(WAIT_TIMEOUT_MS);
        // Stop and was interrupted by timeout of shutdownHard()
        assertTrue(li.runInterrupted);
    }

    /** Test that if the invocation run time goes over the timeout, it will be forced stopped. */
    @Test
    public void testShutdown_invocation_timeout() throws Throwable {
        final LongInvocation li = new LongInvocation(2);
        mCommandOptions.setLoopMode(false);
        mCommandOptions.setInvocationTimeout(500l);
        mCommandScheduler =
                new CommandScheduler() {
                    @Override
                    ITestInvocation createRunInstance() {
                        return li;
                    }

                    @Override
                    protected IDeviceManager getDeviceManager() {
                        return mMockDeviceManager;
                    }

                    @Override
                    protected IConfigurationFactory getConfigFactory() {
                        return mMockConfigFactory;
                    }

                    @Override
                    protected void initLogging() {
                        // ignore
                    }

                    @Override
                    protected void cleanUp() {
                        // ignore
                    }
                };
        String[] slowConfigArgs = new String[] {"slowConfig"};
        List<String> nullArg = null;
        EasyMock.expect(
                mMockConfigFactory.createConfigurationFromArgs(EasyMock.aryEq(slowConfigArgs),
                EasyMock.eq(nullArg), (IKeyStoreClient)EasyMock.anyObject()))
                .andReturn(mSlowConfig).anyTimes();

        EasyMock.replay(mFastConfig, mSlowConfig, mMockConfigFactory);
        mCommandScheduler.start();
        mInterruptible = true;
        mCommandScheduler.addCommand(slowConfigArgs);
        mCommandScheduler.join(mCommandOptions.getInvocationTimeout() * 2);
        // Stop and was interrupted by timeout
        assertTrue(li.runInterrupted);
    }
}
