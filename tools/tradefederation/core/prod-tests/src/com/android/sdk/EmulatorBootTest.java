/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.sdk;

import com.android.sdk.tests.EmulatorGpsPreparer;
import com.android.sdk.tests.EmulatorSmsPreparer;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.IConfiguration;
import com.android.tradefed.config.IConfigurationReceiver;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.DeviceUnresponsiveException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.targetprep.BuildError;
import com.android.tradefed.targetprep.SdkAvdPreparer;
import com.android.tradefed.targetprep.TargetSetupError;
import com.android.tradefed.testtype.IBuildReceiver;
import com.android.tradefed.testtype.IDeviceTest;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.util.StreamUtil;

import java.util.HashMap;

/**
 * A class for posting emulator boot result as a test
 */
public class EmulatorBootTest implements IDeviceTest, IRemoteTest, IBuildReceiver,
        IConfigurationReceiver {
    private IConfiguration mConfiguration;
    private String mTestLabel = "emulator_boot_test";
    private SdkAvdPreparer mAvdPreparer;
    private EmulatorSmsPreparer mSmsPreparer;
    private EmulatorGpsPreparer mGpsPreparer;
    private ITestDevice mDevice;

    void createPreparers() {

        mAvdPreparer = (SdkAvdPreparer) mConfiguration.getConfigurationObject("sdk-avd-preparer");
        mSmsPreparer = (EmulatorSmsPreparer) mConfiguration.getConfigurationObject("sms-preparer");
        mGpsPreparer = (EmulatorGpsPreparer) mConfiguration.getConfigurationObject("gps-preparer");
    }

   IBuildInfo mBuildInfo;

    /**
     * {@inheritDoc}
     */
    @Override
    public void setConfiguration(IConfiguration configuration) {
        mConfiguration = configuration;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setBuild(IBuildInfo buildInfo) {
        mBuildInfo = buildInfo;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public ITestDevice getDevice() {
        return mDevice;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setDevice(ITestDevice device) {
        mDevice = device;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
        TestDescription bootTest =
                new TestDescription(EmulatorBootTest.class.getSimpleName(), mTestLabel);
        listener.testRunStarted(EmulatorBootTest.class.getSimpleName(), 1);
        listener.testStarted(bootTest);
        try {
            createPreparers();
            mAvdPreparer.setUp(mDevice, mBuildInfo);
            mSmsPreparer.setUp(mDevice, mBuildInfo);
            mGpsPreparer.setUp(mDevice, mBuildInfo);
            checkLauncherRunningOnEmulator(mDevice);
        }
        catch(BuildError b) {
            listener.testFailed(bootTest, StreamUtil.getStackTrace(b));
            // throw exception to prevent other tests from executing needlessly
            throw new DeviceUnresponsiveException("The emulator failed to boot", b,
                    mDevice.getSerialNumber());
        }
        catch(RuntimeException e) {
            listener.testFailed(bootTest, StreamUtil.getStackTrace(e));
            throw e;
        } catch (TargetSetupError e) {
            listener.testFailed(bootTest, StreamUtil.getStackTrace(e));
            throw new RuntimeException(e);
        }
        finally {
            listener.testEnded(bootTest, new HashMap<String, Metric>());
            listener.testRunEnded(0, new HashMap<String, Metric>());
        }
    }

    private void checkLauncherRunningOnEmulator(ITestDevice device) throws BuildError,
            DeviceNotAvailableException {
        Integer apiLevel = device.getApiLevel();
        String cmd = "ps";
        if (apiLevel >= 21) {
            cmd = "am stack list";
        } else if (apiLevel == 19) {
            cmd = "am stack boxes";
        }
        String cmdResult = device.executeShellCommand(cmd);
        CLog.i("%s on device %s is %s", cmd, mDevice.getSerialNumber(), cmdResult);

        String[] cmdResultLines = cmdResult.split("\n");
        int i;
        for(i = 0; i < cmdResultLines.length; ++i) {
            if (cmdResultLines[i].contains("com.android.launcher")) {
                break;
            }
        }
        if(i == cmdResultLines.length) {
            throw new BuildError("The emulator do not have launcher run",
                    device.getDeviceDescriptor());
        }
    }
}
