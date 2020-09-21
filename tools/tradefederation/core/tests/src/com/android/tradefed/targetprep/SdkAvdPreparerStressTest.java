/*
 * Copyright (C) 2011 The Android Open Source Project
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
package com.android.tradefed.targetprep;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.Option;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.testtype.IBuildReceiver;
import com.android.tradefed.testtype.IDeviceTest;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.util.proto.TfMetricProtoUtil;

import java.util.HashMap;
import java.util.Map;

/**
* Long running functional tests for {@link SdkAvdPreparer} that verifies an emulator launch
* is successful many times in sequence
* <p/>
* Requires a SDK Build
*/
public class SdkAvdPreparerStressTest implements IBuildReceiver, IDeviceTest, IRemoteTest {

    @Option(name="iterations", description="Number of emulator launch iterations to perform")
    private int mIterations = 10;

    @Option(name="launch-attempts", description=
        "Number of attempts to launch the emulator for each iteration")
    private int mLaunchAttempts = 3;

    private ITestDevice mDevice;

    private IBuildInfo mBuildInfo;

    @Override
    public void run(ITestInvocationListener listener) {
        SdkAvdPreparer preparer = new SdkAvdPreparer();
        int i = 0;
        preparer.setLaunchAttempts(mLaunchAttempts);
        listener.testRunStarted("stress", 0);
        long startTime = System.currentTimeMillis();
        try {
            for (i = 0; i < mIterations; i++) {
                preparer.setUp(mDevice, mBuildInfo);
            }
        } catch (Exception e) {
            CLog.e(e);
        }
         finally {
            Map<String, String> metrics = new HashMap<String, String>(1);
            metrics.put("iterations", Integer.toString(i));
            listener.testRunEnded(
                    System.currentTimeMillis() - startTime,
                    TfMetricProtoUtil.upgradeConvert(metrics));
        }
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
    public ITestDevice getDevice() {
        return mDevice;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setBuild(IBuildInfo buildInfo) {
       mBuildInfo = buildInfo;
    }
}
