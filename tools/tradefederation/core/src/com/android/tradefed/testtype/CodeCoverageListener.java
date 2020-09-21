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

package com.android.tradefed.testtype;

import static com.google.common.base.Verify.verifyNotNull;

import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.FileInputStreamSource;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.result.ResultForwarder;
import com.android.tradefed.util.FileUtil;

import java.io.File;
import java.util.HashMap;

/**
 * A {@link ResultForwarder} that will pull coverage measurements off of the device and log them as
 * test artifacts.
 */
final class CodeCoverageListener extends ResultForwarder {

    public static final String COVERAGE_MEASUREMENT_KEY = "coverageFilePath";

    private final ITestDevice mDevice;

    private String mCurrentRunName;

    public CodeCoverageListener(ITestDevice device, ITestInvocationListener... listeners) {
        super(listeners);
        mDevice = device;
    }

    @Override
    public void testRunStarted(String runName, int testCount) {
        super.testRunStarted(runName, testCount);
        mCurrentRunName = runName;
    }

    @Override
    public void testRunEnded(long elapsedTime, HashMap<String, Metric> runMetrics) {
        // Get the path of the coverage measurement on the device.
        Metric devicePathMetric = runMetrics.get(COVERAGE_MEASUREMENT_KEY);
        if (devicePathMetric == null) {
            super.testRunFailed("No coverage measurement.");
            super.testRunEnded(elapsedTime, runMetrics);
            return;
        }
        String devicePath = devicePathMetric.getMeasurements().getSingleString();
        if (devicePath == null) {
            super.testRunFailed("No coverage measurement.");
            super.testRunEnded(elapsedTime, runMetrics);
            return;
        }

        try {
            File coverageFile = mDevice.pullFile(devicePath);
            verifyNotNull(coverageFile, "Failed to pull the coverage file from %s", devicePath);

            try (FileInputStreamSource source = new FileInputStreamSource(coverageFile)) {
                testLog(mCurrentRunName + "_runtime_coverage", LogDataType.COVERAGE, source);
            } finally {
                FileUtil.deleteFile(coverageFile);
            }
        } catch (DeviceNotAvailableException e) {
            throw new RuntimeException(e);
        } finally {
            super.testRunEnded(elapsedTime, runMetrics);
        }
    }
}
