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

import com.android.loganalysis.item.DumpsysProcessMeminfoItem;
import com.android.loganalysis.parser.DumpsysProcessMeminfoParser;
import com.android.tradefed.config.Option;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.metrics.proto.MetricMeasurement.DataType;
import com.android.tradefed.metrics.proto.MetricMeasurement.Measurements;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.metrics.proto.MetricMeasurement.NumericValues;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;

/**
 * A {@link ScheduledDeviceMetricCollector} to measure peak memory usage of specified processes.
 * Collects PSS and USS (private dirty) memory usage values from dumpsys meminfo. The result will be
 * reported as a test run metric with key in the form of PSS#ProcName[#DeviceNum], in KB.
 */
public class ProcessMaxMemoryCollector extends ScheduledDeviceMetricCollector {

    @Option(
        name = "memory-usage-process-name",
        description = "Process names (from `dumpsys meminfo`) to measure memory usage for"
    )
    private List<String> mProcessNames = new ArrayList<>();

    private class DeviceMemoryData {
        /** Peak PSS per process */
        private Map<String, Long> mProcPss = new HashMap<>();
        /** Peak USS per process */
        private Map<String, Long> mProcUss = new HashMap<>();
    }

    // Memory usage data per device
    private Map<ITestDevice, DeviceMemoryData> mMemoryData;
    private Map<ITestDevice, Map<String, NumericValues.Builder>> mPssMemoryPerProcess;
    private Map<ITestDevice, Map<String, NumericValues.Builder>> mUssMemoryPerProcess;

    @Override
    void onStart(DeviceMetricData runData) {
        mMemoryData = new HashMap<>();
        mPssMemoryPerProcess = new HashMap<>();
        mUssMemoryPerProcess = new HashMap<>();

        for (ITestDevice device : getDevices()) {
            mMemoryData.put(device, new DeviceMemoryData());
            mPssMemoryPerProcess.put(device, new HashMap<>());
            mUssMemoryPerProcess.put(device, new HashMap<>());
        }
    }

    @Override
    void collect(ITestDevice device, DeviceMetricData runData) throws InterruptedException {
        try {
            Map<String, Long> procPss = mMemoryData.get(device).mProcPss;
            Map<String, Long> procUss = mMemoryData.get(device).mProcUss;
            for (String proc : mProcessNames) {
                String dumpResult = device.executeShellCommand("dumpsys meminfo --checkin " + proc);
                if (dumpResult.startsWith("No process found")) {
                    // process not found, skip
                    continue;
                }
                DumpsysProcessMeminfoItem item =
                        new DumpsysProcessMeminfoParser()
                                .parse(Arrays.asList(dumpResult.split("\n")));
                Long pss =
                        item.get(DumpsysProcessMeminfoItem.TOTAL)
                                .get(DumpsysProcessMeminfoItem.PSS);
                Long uss =
                        item.get(DumpsysProcessMeminfoItem.TOTAL)
                                .get(DumpsysProcessMeminfoItem.PRIVATE_DIRTY);
                if (pss == null || uss == null) {
                    CLog.e("Error parsing meminfo output: " + dumpResult);
                    continue;
                }

                // Track PSS values
                if (mPssMemoryPerProcess.get(device) == null) {
                    mPssMemoryPerProcess.put(device, new HashMap<>());
                }
                if (mPssMemoryPerProcess.get(device).get(proc) == null) {
                    mPssMemoryPerProcess.get(device).put(proc, NumericValues.newBuilder());
                }
                mPssMemoryPerProcess.get(device).get(proc).addNumericValue(pss);

                // Track USS values
                if (mUssMemoryPerProcess.get(device) == null) {
                    mUssMemoryPerProcess.put(device, new HashMap<>());
                }
                if (mUssMemoryPerProcess.get(device).get(proc) == null) {
                    mUssMemoryPerProcess.get(device).put(proc, NumericValues.newBuilder());
                }
                mUssMemoryPerProcess.get(device).get(proc).addNumericValue(uss);

                if (procPss.getOrDefault(proc, 0L) < pss) {
                    procPss.put(proc, pss);
                }
                if (procUss.getOrDefault(proc, 0L) < uss) {
                    procUss.put(proc, uss);
                }
            }
        } catch (DeviceNotAvailableException e) {
            CLog.e(e);
        }
    }

    @Override
    void onEnd(DeviceMetricData runData) {
        for (ITestDevice device : getDevices()) {
            // Report all the PSS data for each process
            for (Entry<String, NumericValues.Builder> values :
                    mPssMemoryPerProcess.get(device).entrySet()) {
                Metric.Builder metric = Metric.newBuilder();
                metric.setMeasurements(
                                Measurements.newBuilder()
                                        .setNumericValues(values.getValue().build()))
                        .build();
                metric.setUnit("kB").setType(DataType.RAW);
                runData.addMetricForDevice(device, "PSS#" + values.getKey(), metric);
            }

            // Report all the USS data for each process
            for (Entry<String, NumericValues.Builder> values :
                    mUssMemoryPerProcess.get(device).entrySet()) {
                Metric.Builder metric = Metric.newBuilder();
                metric.setMeasurements(
                                Measurements.newBuilder()
                                        .setNumericValues(values.getValue().build()))
                        .build();
                metric.setUnit("kB").setType(DataType.RAW);
                runData.addMetricForDevice(device, "USS#" + values.getKey(), metric);
            }

            // Continue reporting the max PSS / USS for compatibility
            Map<String, Long> procPss = mMemoryData.get(device).mProcPss;
            Map<String, Long> procUss = mMemoryData.get(device).mProcUss;
            for (Entry<String, Long> pss : procPss.entrySet()) {
                Metric.Builder metric = Metric.newBuilder();
                metric.setMeasurements(
                        Measurements.newBuilder().setSingleInt(pss.getValue()).build());
                metric.setUnit("kB").setType(DataType.PROCESSED);
                runData.addMetricForDevice(device, "MAX_PSS#" + pss.getKey(), metric);
            }
            for (Entry<String, Long> uss : procUss.entrySet()) {
                Metric.Builder metric = Metric.newBuilder();
                metric.setMeasurements(
                        Measurements.newBuilder().setSingleInt(uss.getValue()).build());
                metric.setUnit("kB").setType(DataType.PROCESSED);
                runData.addMetricForDevice(device, "MAX_USS#" + uss.getKey(), metric);
            }
        }
    }
}
