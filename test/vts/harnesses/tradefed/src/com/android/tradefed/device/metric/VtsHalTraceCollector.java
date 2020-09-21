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

package com.android.tradefed.device.metric;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.util.FileUtil;

import java.io.File;
import java.io.IOException;
import java.util.Map;

/**
 * A {@link IMetricCollector} that enables vts hal profiling during a test and
 * collects the trace file after the test run.
 */
@OptionClass(alias = "vts-hal-trace-collector")
public class VtsHalTraceCollector extends BaseDeviceMetricCollector {
    // Path of 32 bit test libs on target device.
    static final String VTS_TMP_LIB_DIR_32 = "/data/local/tmp/32/";
    // Path of 64 bit test libs on target device.
    static final String VTS_TMP_LIB_DIR_64 = "/data/local/tmp/64/";
    // Path of vts test directory on target device.
    static final String VTS_TMP_DIR = "/data/local/tmp/";
    // VTS profiling configure binary name.
    static final String PROFILING_CONFIGURE_BINARY = "vts_profiling_configure";
    // Prefix of temp directory that stores the trace files.
    static final String TRACE_DIR_PREFIX = System.getProperty("java.io.tmpdir") + "/vts-profiling/";
    static final String TRACE_PATH = "trace_path";

    @Override
    public void onTestRunStart(DeviceMetricData testData) {
        for (ITestDevice device : getDevices()) {
            try {
                // adb root.
                device.enableAdbRoot();
                // Set selinux permissive mode.
                device.executeShellCommand("setenforce 0");
                // Cleanup existing traces.
                device.executeShellCommand(String.format("rm -rf %s/*.vts.trace", VTS_TMP_DIR));
                device.executeShellCommand("chmod 777 " + VTS_TMP_DIR);
                device.executeShellCommand("chmod 777 " + VTS_TMP_LIB_DIR_32);
                device.executeShellCommand("chmod 777 " + VTS_TMP_LIB_DIR_64);
                device.executeShellCommand(
                        String.format("%s enable %s %s", VTS_TMP_DIR + PROFILING_CONFIGURE_BINARY,
                                VTS_TMP_LIB_DIR_32, VTS_TMP_LIB_DIR_64));
            } catch (DeviceNotAvailableException e) {
                CLog.e("Could not enable vts hal profiling: " + e.toString());
            }
        }
    }

    @Override
    public void onTestRunEnd(
            DeviceMetricData testData, final Map<String, Metric> currentTestCaseMetrics) {
        String moduleName = getRunName().replace(' ', '_');
        CLog.i("Test module name: " + moduleName);
        for (ITestDevice device : getDevices()) {
            try {
                // Pull trace files.
                pullTraceFiles(device, moduleName);
                // Disable profiling.
                device.executeShellCommand(
                        String.format("%s disable", VTS_TMP_DIR + PROFILING_CONFIGURE_BINARY));
                // Cleanup the trace files.
                device.executeShellCommand(String.format("rm -rf %s/*.vts.trace", VTS_TMP_DIR));
            } catch (DeviceNotAvailableException | IOException e) {
                CLog.e("Failed to get vts hal profiling trace: " + e.toString());
            }
        }
    }

    private void pullTraceFiles(ITestDevice device, String moduleName)
            throws DeviceNotAvailableException, IOException {
        File localTracedDir = null;
        IBuildInfo buildInfo = getBuildInfos().get(0);
        String tracePath = buildInfo.getBuildAttributes().get(TRACE_PATH);
        // Create the local directory to store the trace files.
        if (tracePath != null) {
            localTracedDir = new File(String.format("%s/%s", tracePath, moduleName));
        } else {
            localTracedDir = new File(
                    String.format("%s/%s", FileUtil.createTempDir(TRACE_DIR_PREFIX), moduleName));
        }
        if (!localTracedDir.exists()) {
            if (!localTracedDir.mkdirs()) {
                CLog.e("Failed to create trace dir: " + localTracedDir.getAbsolutePath());
                return;
            }
        }
        // Pull the trace files.
        CLog.i("Storing trace files to: " + localTracedDir.getAbsolutePath());
        String out = device.executeShellCommand(String.format("ls %s/*.vts.trace", VTS_TMP_DIR));
        for (String line : out.split("\n")) {
            line = line.trim();
            File trace_file = new File(
                    localTracedDir.getAbsolutePath(), line.substring(VTS_TMP_DIR.length()));
            device.pullFile(line, trace_file);
        }
    }
}
