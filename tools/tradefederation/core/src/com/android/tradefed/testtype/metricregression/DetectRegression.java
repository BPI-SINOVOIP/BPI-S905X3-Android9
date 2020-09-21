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
package com.android.tradefed.testtype.metricregression;

import com.android.ddmlib.Log;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.testtype.suite.ModuleDefinition;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.MetricsXmlParser;
import com.android.tradefed.util.MetricsXmlParser.ParseException;
import com.android.tradefed.util.MultiMap;
import com.android.tradefed.util.Pair;
import com.android.tradefed.util.TableBuilder;

import com.google.common.annotations.VisibleForTesting;
import com.google.common.collect.ImmutableSet;
import com.google.common.collect.Sets;
import com.google.common.primitives.Doubles;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Random;
import java.util.Set;
import java.util.stream.Collectors;

/** An algorithm to detect local metrics regression. */
@OptionClass(alias = "regression")
public class DetectRegression implements IRemoteTest {

    @Option(
        name = "pre-patch-metrics",
        description = "Path to pre-patch metrics folder.",
        mandatory = true
    )
    private File mPrePatchFolder;

    @Option(
        name = "post-patch-metrics",
        description = "Path to post-patch metrics folder.",
        mandatory = true
    )
    private File mPostPatchFolder;

    @Option(
        name = "strict-mode",
        description = "When before/after metrics mismatch, true=throw exception, false=log error"
    )
    private boolean mStrict = false;

    @Option(name = "blacklist-metrics", description = "Ignore metrics that match these names")
    private Set<String> mBlacklistMetrics = new HashSet<>();

    private static final String TITLE = "Metric Regressions";
    private static final String PROLOG =
            "\n====================Metrics Comparison Results====================\nTest Summary\n";
    private static final String EPILOG =
            "==================End Metrics Comparison Results==================\n";
    private static final String[] TABLE_HEADER = {
        "Metric Name", "Pre Avg", "Post Avg", "False Positive Probability"
    };
    /** Matches metrics xml filenames. */
    private static final String METRICS_PATTERN = "metrics-.*\\.xml";

    private static final int SAMPLES = 100000;
    private static final double STD_DEV_THRESHOLD = 2.0;

    private static final Set<String> DEFAULT_IGNORE =
            ImmutableSet.of(
                    ModuleDefinition.PREPARATION_TIME,
                    ModuleDefinition.TEST_TIME,
                    ModuleDefinition.TEAR_DOWN_TIME);

    @VisibleForTesting
    public static class TableRow {
        String name;
        double preAvg;
        double postAvg;
        double probability;

        public String[] toStringArray() {
            return new String[] {
                name,
                String.format("%.2f", preAvg),
                String.format("%.2f", postAvg),
                String.format("%.3f", probability)
            };
        }
    }

    public DetectRegression() {
        mBlacklistMetrics.addAll(DEFAULT_IGNORE);
    }

    @Override
    public void run(ITestInvocationListener listener) {
        try {
            // Load metrics from files, and validate them.
            Metrics before =
                    MetricsXmlParser.parse(
                            mBlacklistMetrics, mStrict, getMetricsFiles(mPrePatchFolder));
            Metrics after =
                    MetricsXmlParser.parse(
                            mBlacklistMetrics, mStrict, getMetricsFiles(mPostPatchFolder));
            before.crossValidate(after);
            runRegressionDetection(before, after);
        } catch (IOException | ParseException e) {
            throw new RuntimeException(e);
        }
    }

    /**
     * Computes metrics regression between pre-patch and post-patch.
     *
     * @param before pre-patch metrics
     * @param after post-patch metrics
     */
    @VisibleForTesting
    void runRegressionDetection(Metrics before, Metrics after) {
        Set<String> runMetricsToCompare =
                Sets.intersection(before.getRunMetrics().keySet(), after.getRunMetrics().keySet());
        List<TableRow> runMetricsResult = new ArrayList<>();
        for (String name : runMetricsToCompare) {
            List<Double> beforeMetrics = before.getRunMetrics().get(name);
            List<Double> afterMetrics = after.getRunMetrics().get(name);
            if (computeRegression(beforeMetrics, afterMetrics)) {
                runMetricsResult.add(getTableRow(name, beforeMetrics, afterMetrics));
            }
        }

        Set<Pair<TestDescription, String>> testMetricsToCompare =
                Sets.intersection(
                        before.getTestMetrics().keySet(), after.getTestMetrics().keySet());
        MultiMap<String, TableRow> testMetricsResult = new MultiMap<>();
        for (Pair<TestDescription, String> id : testMetricsToCompare) {
            List<Double> beforeMetrics = before.getTestMetrics().get(id);
            List<Double> afterMetrics = after.getTestMetrics().get(id);
            if (computeRegression(beforeMetrics, afterMetrics)) {
                testMetricsResult.put(
                        id.first.toString(), getTableRow(id.second, beforeMetrics, afterMetrics));
            }
        }
        logResult(before, after, runMetricsResult, testMetricsResult);
    }

    /** Prints results to the console. */
    @VisibleForTesting
    void logResult(
            Metrics before,
            Metrics after,
            List<TableRow> runMetricsResult,
            MultiMap<String, TableRow> testMetricsResult) {
        TableBuilder table = new TableBuilder(TABLE_HEADER.length);
        table.addTitle(TITLE).addLine(TABLE_HEADER).addDoubleLineSeparator();

        int totalRunMetrics =
                Sets.intersection(before.getRunMetrics().keySet(), after.getRunMetrics().keySet())
                        .size();
        String runResult =
                String.format(
                        "Run Metrics (%d compared, %d changed)",
                        totalRunMetrics, runMetricsResult.size());
        table.addLine(runResult).addSingleLineSeparator();
        runMetricsResult.stream().map(TableRow::toStringArray).forEach(table::addLine);
        if (!runMetricsResult.isEmpty()) {
            table.addSingleLineSeparator();
        }

        int totalTestMetrics =
                Sets.intersection(before.getTestMetrics().keySet(), after.getTestMetrics().keySet())
                        .size();
        int changedTestMetrics =
                testMetricsResult
                        .keySet()
                        .stream()
                        .mapToInt(k -> testMetricsResult.get(k).size())
                        .sum();
        String testResult =
                String.format(
                        "Test Metrics (%d compared, %d changed)",
                        totalTestMetrics, changedTestMetrics);
        table.addLine(testResult).addSingleLineSeparator();
        for (String test : testMetricsResult.keySet()) {
            table.addLine("> " + test);
            testMetricsResult
                    .get(test)
                    .stream()
                    .map(TableRow::toStringArray)
                    .forEach(table::addLine);
            table.addBlankLineSeparator();
        }
        table.addDoubleLineSeparator();

        StringBuilder sb = new StringBuilder(PROLOG);
        sb.append(
                String.format(
                        "%d tests. %d sets of pre-patch metrics. %d sets of post-patch metrics.\n\n",
                        before.getNumTests(), before.getNumRuns(), after.getNumRuns()));
        sb.append(table.build()).append('\n').append(EPILOG);

        CLog.logAndDisplay(Log.LogLevel.INFO, sb.toString());
    }

    private List<File> getMetricsFiles(File folder) throws IOException {
        CLog.i("Loading metrics from: %s", mPrePatchFolder.getAbsolutePath());
        return FileUtil.findFiles(folder, METRICS_PATTERN)
                .stream()
                .map(File::new)
                .collect(Collectors.toList());
    }

    private static TableRow getTableRow(String name, List<Double> before, List<Double> after) {
        TableRow row = new TableRow();
        row.name = name;
        row.preAvg = calcMean(before);
        row.postAvg = calcMean(after);
        row.probability = probFalsePositive(before.size(), after.size());
        return row;
    }

    /** @return true if there is regression from before to after, false otherwise */
    @VisibleForTesting
    static boolean computeRegression(List<Double> before, List<Double> after) {
        final double mean = calcMean(before);
        final double stdDev = calcStdDev(before);
        int regCount = 0;
        for (double value : after) {
            if (Math.abs(value - mean) > stdDev * STD_DEV_THRESHOLD) {
                regCount++;
            }
        }
        return regCount > after.size() / 2;
    }

    @VisibleForTesting
    static double calcMean(List<Double> list) {
        return list.stream().collect(Collectors.averagingDouble(x -> x));
    }

    @VisibleForTesting
    static double calcStdDev(List<Double> list) {
        final double mean = calcMean(list);
        return Math.sqrt(
                list.stream().collect(Collectors.averagingDouble(x -> Math.pow(x - mean, 2))));
    }

    private static double probFalsePositive(int priorRuns, int postRuns) {
        int failures = 0;
        Random rand = new Random();
        for (int run = 0; run < SAMPLES; run++) {
            double[] prior = new double[priorRuns];
            for (int x = 0; x < priorRuns; x++) {
                prior[x] = rand.nextGaussian();
            }
            double estMu = calcMean(Doubles.asList(prior));
            double estStd = calcStdDev(Doubles.asList(prior));
            int count = 0;
            for (int y = 0; y < postRuns; y++) {
                if (Math.abs(rand.nextGaussian() - estMu) > estStd * STD_DEV_THRESHOLD) {
                    count++;
                }
            }
            failures += count > postRuns / 2 ? 1 : 0;
        }
        return (double) failures / SAMPLES;
    }
}
