/*
 * Copyright (C) 2018 The Android Open Source Project
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
package com.android.tradefed.testtype.host;

import com.android.ddmlib.Log.LogLevel;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.device.StubDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.TestDescription;

import java.util.Date;
import java.util.HashMap;
import java.util.List;

/**
 * Logger matching the events and logging them in order to make it easier to debug. The log on
 * host-side and device side will strictly match in order to make it easy to search for it.
 */
public class PrettyTestEventLogger implements ITestInvocationListener {

    private static final String TAG = "TradefedEventsTag";
    private final List<ITestDevice> mDevices;

    public PrettyTestEventLogger(List<ITestDevice> devices) {
        mDevices = devices;
    }

    @Override
    public void testStarted(TestDescription test) {
        Date date = new Date();
        String message =
                String.format(
                        "==================== %s STARTED: %s ====================",
                        test.toString(), date.toString());
        CLog.d(message);
        logOnAllDevices(message);
    }

    @Override
    public void testEnded(TestDescription test, HashMap<String, Metric> testMetrics) {
        Date date = new Date();
        String message =
                String.format(
                        "==================== %s ENDED: %s ====================",
                        test.toString(), date.toString());
        CLog.d(message);
        logOnAllDevices(message);
    }

    private void logOnAllDevices(String format, Object... args) {
        for (ITestDevice device : mDevices) {
            // Only attempt to log on real devices.
            if (!(device.getIDevice() instanceof StubDevice)) {
                device.logOnDevice(TAG, LogLevel.DEBUG, format, args);
            }
        }
    }
}
