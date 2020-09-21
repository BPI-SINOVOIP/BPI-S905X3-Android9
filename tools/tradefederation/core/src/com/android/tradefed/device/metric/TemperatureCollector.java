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

import static com.android.tradefed.targetprep.TemperatureThrottlingWaiter.DEVICE_TEMPERATURE_FILE_PATH_NAME;

import com.android.tradefed.config.Option;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.metrics.proto.MetricMeasurement.DataType;
import com.android.tradefed.metrics.proto.MetricMeasurement.DoubleValues;
import com.android.tradefed.metrics.proto.MetricMeasurement.Measurements;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;

import java.util.HashMap;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * A {@link ScheduledDeviceMetricCollector} to measure min and max device temperature. Useful for
 * long duration performance tests to monitor if the device overheats.
 */
public class TemperatureCollector extends ScheduledDeviceMetricCollector {

    private static final String CELCIUS_UNIT = "celcius";

    // Option name intentionally shared with TemperatureThrottlingWaiter
    @Option(
        name = DEVICE_TEMPERATURE_FILE_PATH_NAME,
        description =
                "Name of file that contains device temperature. "
                        + "Example: /sys/class/hwmon/hwmon1/device/msm_therm"
    )
    private String mDeviceTemperatureFilePath = null;

    @Option(
        name = "device-temperature-file-regex",
        description =
                "Regex to parse temperature file. First group must be the temperature parsable"
                        + "to Double. Default: Result:(\\d+) Raw:.*"
    )
    private String mDeviceTemperatureFileRegex = "Result:(\\d+) Raw:.*";

    /**
     * Stores the highest recorded temperature per device. Device will not be present in the map if
     * no valid temperature was recorded.
     */
    private Map<ITestDevice, Double> mMaxDeviceTemps;

    /**
     * Stores the lowest recorded temperature per device. Device will not be present in the map if
     * no valid temperature was recorded.
     */
    private Map<ITestDevice, Double> mMinDeviceTemps;

    // Example: Result:32 Raw:7e51
    private static Pattern mTemperatureRegex;

    private Map<ITestDevice, DoubleValues.Builder> mValues;

    @Override
    void onStart(DeviceMetricData runData) {
        mTemperatureRegex = Pattern.compile(mDeviceTemperatureFileRegex);
        mMaxDeviceTemps = new HashMap<>();
        mMinDeviceTemps = new HashMap<>();
        mValues = new HashMap<>();
    }

    @Override
    void collect(ITestDevice device, DeviceMetricData runData) throws InterruptedException {
        if (mDeviceTemperatureFilePath == null) {
            return;
        }
        try {
            if (!device.isAdbRoot()) {
                return;
            }
            Double temp = getTemperature(device);
            if (temp == null) {
                return;
            }
            if (mValues.get(device) == null) {
                mValues.put(device, DoubleValues.newBuilder());
            }
            mValues.get(device).addDoubleValue(temp);
            mMaxDeviceTemps.putIfAbsent(device, temp);
            mMinDeviceTemps.putIfAbsent(device, temp);
            if (mMaxDeviceTemps.get(device) < temp) {
                mMaxDeviceTemps.put(device, temp);
            }
            if (mMinDeviceTemps.get(device) > temp) {
                mMinDeviceTemps.put(device, temp);
            }
        } catch (DeviceNotAvailableException e) {
            CLog.e(e);
        }
    }

    private Double getTemperature(ITestDevice device) throws DeviceNotAvailableException {
        String cmd = "cat " + mDeviceTemperatureFilePath;
        String result = device.executeShellCommand(cmd).trim();
        Matcher m = mTemperatureRegex.matcher(result);
        if (m.matches()) {
            return Double.parseDouble(m.group(1));
        }
        CLog.e("Error parsing temperature file output: " + result);
        return null;
    }

    @Override
    void onEnd(DeviceMetricData runData) {
        for (ITestDevice device : getDevices()) {
            DoubleValues.Builder values = mValues.get(device);
            if (values != null) {
                Metric.Builder metric = Metric.newBuilder();
                metric.setMeasurements(
                        Measurements.newBuilder().setDoubleValues(values.build()).build());
                metric.setUnit(CELCIUS_UNIT).setType(DataType.RAW);
                runData.addMetricForDevice(device, "temperature", metric);
            }
            // Report the max and min for compatibility
            Double maxTemp = mMaxDeviceTemps.get(device);
            if (maxTemp != null) {
                Metric.Builder metric = Metric.newBuilder();
                metric.setMeasurements(Measurements.newBuilder().setSingleDouble(maxTemp).build());
                // Since we report some processed value report it as PROCESSED.
                metric.setUnit(CELCIUS_UNIT).setType(DataType.PROCESSED);
                runData.addMetricForDevice(device, "max_temperature", metric);
            }
            Double minTemp = mMinDeviceTemps.get(device);
            if (minTemp != null) {
                Metric.Builder metric = Metric.newBuilder();
                metric.setMeasurements(Measurements.newBuilder().setSingleDouble(minTemp).build());
                // Since we report some processed value report it as PROCESSED.
                metric.setUnit(CELCIUS_UNIT).setType(DataType.PROCESSED);
                runData.addMetricForDevice(device, "min_temperature", metric);
            }
        }
    }
}
