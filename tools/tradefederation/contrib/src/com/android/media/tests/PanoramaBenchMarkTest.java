/*
 * Copyright (C) 2012 The Android Open Source Project
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

package com.android.media.tests;

import com.android.ddmlib.IDevice;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.testtype.IDeviceTest;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.util.proto.TfMetricProtoUtil;

import org.junit.Assert;

import java.util.HashMap;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Standalone panoramic photo processing benchmark test.
 */
public class PanoramaBenchMarkTest implements IDeviceTest, IRemoteTest {

    private ITestDevice mTestDevice = null;

    private static final Pattern ELAPSED_TIME_PATTERN =
            Pattern.compile("(Total elapsed time:)\\s+(\\d+\\.\\d*)\\s+(seconds)");

    private static final String PANORAMA_TEST_KEY = "PanoramaElapsedTime";
    private static final String TEST_TAG = "CameraLatency";

    /**
     * {@inheritDoc}
     */
    @Override
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
        Assert.assertNotNull(mTestDevice);

        String dataStore = mTestDevice.getMountPoint(IDevice.MNT_DATA);
        String externalStore = mTestDevice.getMountPoint(IDevice.MNT_EXTERNAL_STORAGE);

        mTestDevice.executeShellCommand(String.format("chmod 777 %s/local/tmp/panorama_bench",
                dataStore));

        String shellOutput = mTestDevice.executeShellCommand(
                String.format("%s/local/tmp/panorama_bench %s/panorama_input/test %s/panorama.ppm",
                dataStore, externalStore, externalStore));

        String[] lines = shellOutput.split("\n");

        Map<String, String> metrics = new HashMap<String, String>();
        for (String line : lines) {
            Matcher m = ELAPSED_TIME_PATTERN.matcher(line.trim());
            if (m.matches()) {
                CLog.d(String.format("Found elapsed time \"%s seconds\" from line %s",
                        m.group(2), line));
                metrics.put(PANORAMA_TEST_KEY, m.group(2));
                break;
            } else {
                CLog.d(String.format("Unabled to find elapsed time from line: %s", line));
            }
        }

        reportMetrics(listener, TEST_TAG, metrics);
        cleanupDevice();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setDevice(ITestDevice device) {
        mTestDevice = device;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public ITestDevice getDevice() {
        return mTestDevice;
    }

    /**
     * Removes image files used to mock panorama stitching.
     *
     * @throws DeviceNotAvailableException If the device is unavailable or
     *         something happened while deleting files
     */
    private void cleanupDevice() throws DeviceNotAvailableException {
        String externalStore = mTestDevice.getMountPoint(IDevice.MNT_EXTERNAL_STORAGE);
        mTestDevice.executeShellCommand(String.format("rm -r %s/panorama_input", externalStore));
    }

    /**
     * Report run metrics by creating an empty test run to stick them in.
     *
     * @param listener The {@link ITestInvocationListener} of test results
     * @param runName The test name
     * @param metrics The {@link Map} that contains metrics for the given test
     */
    private void reportMetrics(ITestInvocationListener listener, String runName,
            Map<String, String> metrics) {
        InputStreamSource bugreport = mTestDevice.getBugreport();
        listener.testLog("bugreport", LogDataType.BUGREPORT, bugreport);
        bugreport.close();

        CLog.d(String.format("About to report metrics: %s", metrics));
        listener.testRunStarted(runName, 0);
        listener.testRunEnded(0, TfMetricProtoUtil.upgradeConvert(metrics));
    }
}
