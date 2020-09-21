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
package com.android.tradefed.testtype.suite;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.testtype.IInvocationContextReceiver;
import com.android.tradefed.testtype.IMultiDeviceTest;
import com.android.tradefed.testtype.IRemoteTest;

import java.util.HashMap;
import java.util.Map;

/** Stub Test class that uses multiple devices. */
public class MultiDeviceStubTest
        implements IRemoteTest, IMultiDeviceTest, IInvocationContextReceiver {

    private IInvocationContext mContext;
    private Map<ITestDevice, IBuildInfo> mDeviceBuildInfos;
    private int mExpectedDevice = -1;

    @Override
    public void setInvocationContext(IInvocationContext invocationContext) {
        mContext = invocationContext;
    }

    @Override
    public void setDeviceInfos(Map<ITestDevice, IBuildInfo> deviceInfos) {
        mDeviceBuildInfos = deviceInfos;
    }

    @Override
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
        listener.testRunStarted(getClass().getSimpleName(), 2);
        // If the data are not injected properly, the run will fail.
        if (mExpectedDevice == -1) {
            listener.testRunFailed("You forgot to call setExpectedDevice");
        } else if (mContext.getDeviceConfigNames().size() != mExpectedDevice) {
            listener.testRunFailed(
                    String.format(
                            "mContext has %s devices instead of %s",
                            mContext.getDeviceConfigNames().size(), mExpectedDevice));
        } else if (mDeviceBuildInfos.size() != mExpectedDevice) {
            listener.testRunFailed(
                    String.format(
                            "mDeviceInfos has %s devices instead of %s",
                            mDeviceBuildInfos.size(), mExpectedDevice));
        }
        for (int i = 0; i < 2; i++) {
            TestDescription tid = new TestDescription(getClass().getSimpleName(), "test" + i);
            listener.testStarted(tid, 0);
            listener.testEnded(tid, 5, new HashMap<String, Metric>());
        }
        listener.testRunEnded(500, new HashMap<String, Metric>());
    }

    public void setExceptedDevice(int num) {
        mExpectedDevice = num;
    }
}
