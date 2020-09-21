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

package com.android.example;

import com.android.tradefed.config.Option;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.IManagedTestDevice;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.testtype.IDeviceTest;
import com.android.tradefed.testtype.IRemoteTest;

import java.util.HashMap;

/**
 * Reboots the device and verifies it comes back online.
 * This simple reboot tests acts as an example integration test.
 */
public class RebootTest implements IRemoteTest, IDeviceTest {
    private ITestDevice mDevice = null;

    @Option(name = "num-of-reboots", description = "Number of times to reboot the device.")
    private int mNumDeviceReboots = 1;

    /**
     * {@inheritDoc}
     */
    @Override
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
        long start;
        HashMap<String, Metric> emptyMap = new HashMap<>();
        TestDescription testId;
        start = System.currentTimeMillis();
        listener.testRunStarted(String.format("#%d device reboots", mNumDeviceReboots),
                                mNumDeviceReboots);
        try {
            for (int testCount = 0; testCount < mNumDeviceReboots; testCount++) {
                testId = new TestDescription("RebootTest",
                                            String.format("RebootLoop #%d", testCount));
                listener.testStarted(testId);
                try {
                    getDevice().nonBlockingReboot();
                    if (((IManagedTestDevice) getDevice()).getMonitor().waitForDeviceOnline()
                            == null) {
                        listener.testFailed(testId, "Reboot failed");
                        ((IManagedTestDevice) getDevice()).recoverDevice();
                    }
                }
                finally {
                    listener.testEnded(testId, emptyMap);
                }
            }
        }
        finally {
            listener.testRunEnded(System.currentTimeMillis() - start, emptyMap);
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
}
