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

import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.FileInputStreamSource;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.RunUtil;
import com.android.tradefed.util.SimpleStats;
import com.android.tradefed.util.ZipUtil;
import com.android.tradefed.util.proto.TfMetricProtoUtil;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

/**
 * Collects the traces from all the test directory under the given directory
 * from the test device, log the test directory and post process the trace files
 * under the test directory and aggregate the metrics.
 */
@OptionClass(alias = "atrace-metric")
public class AtraceRunMetricCollector extends FilePullerDeviceMetricCollector {

    private static final String TRACE_SUMMARY = "Trace Summary:";
    private static final String LINE_SEPARATOR = "\\n";
    private static final String METRIC_SEPARATOR = ":";

    @Option(
            name = "script-path",
            description = "Path to the script files used to analyze the trace files."
        )
    private List<String> mScriptPaths = new ArrayList<>();

    private ITestInvocationListener mListener = null;

    @Override
    public void onTestEnd(DeviceMetricData testData, Map<String, Metric> currentTestCaseMetrics) {
        // Collector targets only the run metrics.
    }

    @Override
    public void processMetricDirectory(String key, File metricDirectory, DeviceMetricData runData) {
        mListener = getInvocationListener();
        if (metricDirectory == null) {
            CLog.w("Metric directory is null.");
            return;
        }
        File[] testDirectories = metricDirectory.listFiles();
        for (File testDirectory : testDirectories) {
            uploadTraceFiles(testDirectory);
            if (!mScriptPaths.isEmpty()) {
                for (String scriptPath : mScriptPaths) {
                    if (!processTraceFiles(testDirectory, scriptPath)) {
                        CLog.e("Unable to process the trace files in %s" + testDirectory);
                    }
                }
            }
        }
    }

    /**
     * Compress the testDirectory and upload it in sponge link.
     * @param testDirectory which contains trace files collected during the test.
     */
    private void uploadTraceFiles(File testDirectory) {
        File atraceZip = null;
        try {
            atraceZip = ZipUtil.createZip(testDirectory);
        } catch (IOException e) {
            CLog.e("Unable to create trace zip file");
        }
        if (atraceZip != null) {
            try (FileInputStreamSource streamSource =
                    new FileInputStreamSource(atraceZip)) {
                mListener.testLog(String.format("atrace_%s", testDirectory.getName()),
                        LogDataType.ZIP, streamSource);
            } finally {
                FileUtil.deleteFile(atraceZip);
            }
        }
    }

    /**
     * Run the script for all the trace files in the testDirectory and aggregate the
     * metrics.
     * @param testDirectory which contains trace files.
     * @param scriptPath path to the script.
     * @return true if successfully processed the trace files otherwise return false.
     */
    private boolean processTraceFiles(File testDirectory, String scriptPath) {
        File[] traceFiles = testDirectory.listFiles();
        List<String> output = new ArrayList<>();
        for (File traceFile : traceFiles) {
            CommandResult cr = RunUtil.getDefault().runTimedCmd(30 * 1000, scriptPath,
                    traceFile.getAbsolutePath());
            if (CommandStatus.SUCCESS.equals(cr.getStatus())) {
                CLog.i(cr.getStdout());
                output.add(cr.getStdout());
            } else {
                CLog.e("Unable to parse the trace file %s due to %s - Status - %s ",
                        traceFile.getName(), cr.getStderr(), cr.getStatus());
                return false;
            }
        }
        Map<String, String> finalResult = aggregateMetrics(output);

        // FIXME When we have a better defined metric type pipeline use it instead
        // of the below approach.
        String scriptArgs[] = scriptPath.split(" ");
        String scriptPathArgs[] = scriptArgs[0].split("/");
        String scriptName[] = scriptPathArgs[scriptPathArgs.length - 1].split("\\.");
        TestDescription testId = new TestDescription(scriptName[0], testDirectory.getName());
        mListener.testStarted(testId);
        mListener.testEnded(testId, TfMetricProtoUtil.upgradeConvert(finalResult));
        return true;
    }

    /**
     * Aggregate the metrics from the script output from each test directory and
     * provide detailed stats (sum, mean, min and max of the metrics)
     * @param cmdOutput list of output from the trace files.
     * @return the aggregated and detailed test metrics.
     */
    private Map<String, String> aggregateMetrics(List<String> cmdOutput) {
        Map<String, String> finalResultMap = new LinkedHashMap<>();
        Map<String, SimpleStats> resultAggregatorMap = new HashMap<>();

        // Aggregate the metrics from all the trace files output
        for (String output : cmdOutput) {
            String[] outputLines = output.split(LINE_SEPARATOR);
            boolean isTraceSummary = false;
            for (String line : outputLines) {
                if (line.contains(TRACE_SUMMARY)) {
                    isTraceSummary = true;
                    continue;
                }
                if (isTraceSummary) {
                    String[] metric = line.split(METRIC_SEPARATOR);
                    if (!resultAggregatorMap.containsKey(metric[0].trim())) {
                        SimpleStats resultStats = new SimpleStats();
                        resultStats.add(Double.parseDouble(metric[1].trim()));
                        resultAggregatorMap.put(metric[0].trim(), resultStats);
                    } else {
                        resultAggregatorMap.get(metric[0].trim()).add(
                                Double.parseDouble(metric[1].trim()));
                    }
                }
            }
        }

        for (Map.Entry<String, SimpleStats> resultAggregate : resultAggregatorMap.entrySet()) {
            finalResultMap.put(resultAggregate.getKey() + "_sum",
                    Double.toString(resultAggregate.getValue().mean()
                            * resultAggregate.getValue().size()));
            finalResultMap.put(resultAggregate.getKey() + "_avg",
                    Double.toString(resultAggregate.getValue().mean()));
            finalResultMap.put(resultAggregate.getKey() + "_max",
                    Double.toString(resultAggregate.getValue().max()));
            finalResultMap.put(resultAggregate.getKey() + "_min",
                    Double.toString(resultAggregate.getValue().min()));
        }

        return finalResultMap;
    }

    @Override
    public void processMetricFile(String key, File metricFile, DeviceMetricData runData) {
        // This collector expects only the directories and not the files.

    }

}
