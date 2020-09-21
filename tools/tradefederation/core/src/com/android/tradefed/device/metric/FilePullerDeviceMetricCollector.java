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
package com.android.tradefed.device.metric;

import com.android.tradefed.config.Option;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.proto.TfMetricProtoUtil;

import java.io.File;
import java.io.IOException;
import java.util.AbstractMap.SimpleEntry;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.regex.Pattern;

/**
 * A {@link BaseDeviceMetricCollector} that listen for metrics key coming from the device and pull
 * them as a file from the device. Can be extended for extra-processing of the file.
 */
public abstract class FilePullerDeviceMetricCollector extends BaseDeviceMetricCollector {

    @Option(
        name = "pull-pattern-keys",
        description = "The pattern key name to be pull from the device as a file. Can be repeated."
    )
    private List<String> mKeys = new ArrayList<>();

    @Option(
        name = "directory-keys",
        description = "Path to the directory on the device that contains the metrics."
        )
    protected List<String> mDirectoryKeys = new ArrayList<>();

    @Option(
        name = "clean-up",
        description = "Whether to delete the file from the device after pulling it or not."
    )
    private boolean mCleanUp = true;

    @Override
    public void onTestRunEnd(
            DeviceMetricData runData, final Map<String, Metric> currentRunMetrics) {
        processMetricRequest(runData, TfMetricProtoUtil.compatibleConvert(currentRunMetrics));
    }

    @Override
    public void onTestEnd(DeviceMetricData testData, Map<String, Metric> currentTestCaseMetrics) {
        processMetricRequest(testData, TfMetricProtoUtil.compatibleConvert(currentTestCaseMetrics));
    }

    /**
     * Implementation of the method should allow to log the file, parse it for metrics to be put in
     * {@link DeviceMetricData}.
     *
     * @param key the option key associated to the file that was pulled.
     * @param metricFile the {@link File} pulled from the device matching the option key.
     * @param runData the run {@link DeviceMetricData} where metrics can be stored.
     */
    public abstract void processMetricFile(String key, File metricFile, DeviceMetricData runData);

    /**
     * Implementation of the method should allow to log the directory, parse it for metrics to be
     * put in {@link DeviceMetricData}.
     *
     * @param key the option key associated to the directory that was pulled.
     * @param metricDirectory the {@link File} pulled from the device matching the option key.
     * @param runData the run {@link DeviceMetricData} where metrics can be stored.
     */
    public abstract void processMetricDirectory(
            String key, File metricDirectory, DeviceMetricData runData);

    private void processMetricRequest(DeviceMetricData data, Map<String, String> currentMetrics) {
        if (mKeys.isEmpty() && mDirectoryKeys.isEmpty()) {
            return;
        }
        for (String key : mKeys) {
            Entry<String, File> pulledMetrics = pullMetricFile(key, currentMetrics);
            if (pulledMetrics != null) {
                processMetricFile(pulledMetrics.getKey(), pulledMetrics.getValue(), data);
            }
        }

        for (String key : mDirectoryKeys) {
            Entry<String, File> pulledMetrics = pullMetricDirectory(key);
            if (pulledMetrics != null) {
                processMetricDirectory(pulledMetrics.getKey(), pulledMetrics.getValue(), data);
            }
        }

    }

    private Entry<String, File> pullMetricFile(
            String pattern, final Map<String, String> currentRunMetrics) {
        Pattern p = Pattern.compile(pattern);
        for (Entry<String, String> entry : currentRunMetrics.entrySet()) {
            if (p.matcher(entry.getKey()).find()) {
                for (ITestDevice device : getDevices()) {
                    try {
                        File attemptPull = device.pullFile(entry.getValue());
                        if (attemptPull != null) {
                            if (mCleanUp) {
                                device.executeShellCommand(
                                        String.format("rm -f %s", entry.getValue()));
                            }
                            // Return the actual key and the file associated
                            return new SimpleEntry<String, File>(entry.getKey(), attemptPull);
                        }
                    } catch (DeviceNotAvailableException e) {
                        CLog.e(
                                "Exception when pulling metric file '%s' from %s",
                                entry.getValue(), device.getSerialNumber());
                        CLog.e(e);
                    }
                }
            }
        }
        CLog.e("Could not find a device file associated to pattern '%s'.", pattern);
        return null;
    }

    /**
     * Pulls the directory and all its content from the device and save it in the
     * host under the host_tmp folder.
     *
     * @param keyDirectory path to the source directory in the device.
     * @return Key,value pair of the directory name and path to the directory in the
     * local host.
     */
    private Entry<String, File> pullMetricDirectory(String keyDirectory) {
        try {
            File tmpDestDir = FileUtil.createTempDir("host_tmp");
            for (ITestDevice device : getDevices()) {
                try {
                    if (device.pullDir(keyDirectory, tmpDestDir)) {
                        if (mCleanUp) {
                            device.executeShellCommand(
                                    String.format("rm -rf %s", keyDirectory));
                        }
                        return new SimpleEntry<String, File>(keyDirectory, tmpDestDir);
                    }
                } catch (DeviceNotAvailableException e) {
                    CLog.e(
                            "Exception when pulling directory '%s' from %s",
                            keyDirectory, device.getSerialNumber());
                    CLog.e(e);
                }
            }
        } catch (IOException ioe) {
            CLog.e("Exception while creating the local directory");
            CLog.e(ioe);
        }
        CLog.e("Could not find a device directory associated to path '%s'.", keyDirectory);
        return null;
    }

}
