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

package com.android.build.tests;

import com.android.ddmlib.Log;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.testtype.IBuildReceiver;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.util.proto.TfMetricProtoUtil;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * A device-less test that parses standard Android build image stats file and performs aggregation
 */
@OptionClass(alias = "image-stats")
public class ImageStats implements IRemoteTest, IBuildReceiver {

    // built-in aggregation labels
    private static final String LABEL_TOTAL = "total";
    private static final String LABEL_CATEGORIZED = "categorized";
    private static final String LABEL_UNCATEGORIZED = "uncategorized";

    private static final String FILE_SIZES = "fileSizes";

    @Option(name = "size-stats-file", description = "Specify the name of the file containing image "
            + "stats; when \"file-from-build-info\" is set to true, the name refers to a file that "
            + "can be found in build info (note that build provider must be properly configured to "
            + "download it), otherwise it refers to a local file, typically used for debugging "
            + "purposes", mandatory = true)
    private String mStatsFileName = null;

    @Option(name = "file-from-build-info", description = "If the \"size-stats-file\" references a "
            + "file from build info, or local; use local file for debugging purposes.")
    private boolean mFileFromBuildInfo = false;

    @Option(name = "aggregation-pattern", description = "A key value pair consists of a regex as "
            + "key and a string label as value. The regex is used to scan and group file size "
            + "entries together for aggregation; that is, all files with names matching the "
            + "pattern are grouped together for summing; this also means that a file could be "
            + "counted multiple times; note that the regex must be a full match, not substring. "
            + "The string label is used for identifying the summed group of file sizes when "
            + "reporting; the regex may contain unnamed capturing groups, and values may contain "
            + "numerical \"back references\" as place holders to be replaced with content of "
            + "corresponding capturing group, example: ^.+\\.(.+)$ -> group-by-extension-\\1; back "
            + "references are 1-indexed and there maybe up to 9 capturing groups; no strict checks "
            + "are performed to ensure that capturing groups and place holders are 1:1 mapping. "
            + "There are 3 built-in aggregations: total, categorized and uncategorized.")
    private Map<String, String> mAggregationPattern = new HashMap<>();

    @Option(name = "min-report-size", description = "Minimum size in bytes that an aggregated "
            + "category needs to reach before being included in reported metrics. 0 for no limit. "
            + "Note that built-in categories are always reported.")
    private long mMinReportSize = 0;

    private IBuildInfo mBuildInfo;

    @Override
    public void setBuild(IBuildInfo buildInfo) {
        mBuildInfo = buildInfo;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
        File statsFile;
        if (mFileFromBuildInfo) {
            statsFile = mBuildInfo.getFile(mStatsFileName);
        } else {
            statsFile = new File(mStatsFileName);
        }
        long start = System.currentTimeMillis();
        Map<String, String> fileSizes = null;
        // fixed run name, 1 test to run
        listener.testRunStarted("image-stats", 1);
        if (statsFile == null || !statsFile.exists()) {
            throw new RuntimeException(
                    "Invalid image stats file (<null>) specified or it does not exist.");
        } else {
            TestDescription td = new TestDescription(ImageStats.class.getName(), FILE_SIZES);
            listener.testStarted(td);
            try (InputStream in = new FileInputStream(statsFile)) {
                fileSizes = performAggregation(parseFileSizes(in),
                        processAggregationPatterns(mAggregationPattern));
            } catch (IOException ioe) {
                String message = String.format("Failed to parse image stats file: %s",
                        statsFile.getAbsolutePath());
                CLog.e(message);
                CLog.e(ioe);
                listener.testFailed(td, ioe.toString());
                listener.testEnded(td, new HashMap<String, Metric>());
                listener.testRunFailed(message);
                listener.testRunEnded(
                        System.currentTimeMillis() - start, new HashMap<String, Metric>());
                throw new RuntimeException(message, ioe);
            }
            String logOutput = String.format("File sizes: %s", fileSizes.toString());
            if (mFileFromBuildInfo) {
                CLog.v(logOutput);
            } else {
                // assume local debug, print outloud
                CLog.logAndDisplay(Log.LogLevel.VERBOSE, logOutput);
            }
            listener.testEnded(td, TfMetricProtoUtil.upgradeConvert(fileSizes));
        }
        listener.testRunEnded(System.currentTimeMillis() - start, new HashMap<String, Metric>());
    }

    /**
     * Processes text files like 'installed-files.txt' (as built by standard Android build rules for
     * device targets) into a map of file path to file sizes
     * @param in an unread {@link InputStream} for the content of the file sizes; the stream will be
     *           fully read after executing the method
     * @return
     */
    protected Map<String, Long> parseFileSizes(InputStream in) throws IOException {
        Map<String, Long> ret = new HashMap<>();
        try (BufferedReader br = new BufferedReader(new InputStreamReader(in))) {
            String line;
            while ((line = br.readLine()) != null) {
                // trim both ends of the raw line and make a split by whitespaces
                // e.g. trying to match a line like this:
                //     96992106  /system/app/Chrome/Chrome.apk
                String[] fields = line.trim().split("\\s+");
                if (fields.length != 2) {
                    CLog.w("Unable to split line to file size and name: %s", line);
                    continue;
                }
                long size = 0;
                try {
                    size = Long.parseLong(fields[0]);
                } catch (NumberFormatException nfe) {
                    CLog.w("Failed to parse file size from field '%s', ignored", fields[0]);
                    continue;
                }
                ret.put(fields[1], size);
            }
        }
        return ret;
    }

    /** Compiles the supplied aggregation regex's */
    protected Map<Pattern, String> processAggregationPatterns(Map<String, String> rawPatterns) {
        Map<Pattern, String> ret = new HashMap<>();
        for (Map.Entry<String, String> e : rawPatterns.entrySet()) {
            Pattern p = Pattern.compile(e.getKey());
            ret.put(p, e.getValue());
        }
        return ret;
    }

    /**
     * Converts a matched file entry to the final aggregation label name.
     * <p>
     * The main thing being converted here is that capturing groups in the regex (used to match the
     * filenames) are extracted, and used to replace the corresponding placeholders in raw label.
     * For each 1-indexed capturing group, the captured content is used to replace the "\x"
     * placeholders in raw label, with x being a number between 1-9, corresponding to the index of
     * the capturing group.
     *
     * @param matcher  the {@link Matcher} representing the matched result from the regex and input
     * @param rawLabel the corresponding aggregation label
     * @return
     */
    protected String getAggregationLabel(Matcher matcher, String rawLabel) {
        if (matcher.groupCount() == 0) {
            // no capturing groups, return label as is
            return rawLabel;
        }
        if (matcher.groupCount() > 9) {
            // since we are doing replacement of back references to capturing groups manually,
            // artificially limiting this to avoid overly complex code to handle \1 vs \10
            // in other words, "9 capturing groups ought to be enough for anybody"
            throw new RuntimeException("too many capturing groups");
        }
        String label = rawLabel;
        for (int i = 1; i <= matcher.groupCount(); i++) {
            String marker = String.format("\\%d", i); // e.g. "\1"
            if (label.indexOf(marker) == -1) {
                CLog.w("Capturing groups were defined in regex '%s', but corresponding "
                        + "back-reference placeholder '%s' not found in label '%s'",
                        matcher.pattern(), marker, rawLabel);
                continue;
            }
            label = label.replace(marker, matcher.group(i));
        }
        // ensure that the resulting label is not the same as the fixed "uncategorized" label
        if (LABEL_UNCATEGORIZED.equals(label)) {
            throw new IllegalArgumentException(String.format("Use of aggregation label '%s' "
                    + "conflicts with built-in default.", LABEL_UNCATEGORIZED));
        }
        return label;
    }

    /**
     * Performs aggregation by adding raw file size entries together based on the regex's the full
     * path names are matched. Note that this means a file entry could get aggregated multiple
     * times. The returned map will also include a fixed entry called "uncategorized" that adds the
     * sizes of all file entries that were never matched together.
     * @param stats the map of raw stats: full path name -> file size
     * @param patterns the map of aggregation patterns: a regex that could match file names -> the
     *                 name of the aggregated result category (e.g. all apks)
     * @return
     */
    protected Map<String, String> performAggregation(Map<String, Long> stats,
            Map<Pattern, String> patterns) {
        Set<String> uncategorizedFiles = new HashSet<>(stats.keySet());
        Map<String, Long> result = new HashMap<>();
        long total = 0;

        for (Map.Entry<String, Long> stat : stats.entrySet()) {
            // aggregate for total first
            total += stat.getValue();
            for (Map.Entry<Pattern, String> pattern : patterns.entrySet()) {
                Matcher m = pattern.getKey().matcher(stat.getKey());
                if (m.matches()) {
                    // the file entry being looked at matches one of the preconfigured rules
                    String label = getAggregationLabel(m, pattern.getValue());
                    Long size = result.get(label);
                    if (size == null) {
                        size = 0L;
                    }
                    size += stat.getValue();
                    result.put(label, size);
                    // keep track of files that we've already aggregated at least once
                    if (uncategorizedFiles.contains(stat.getKey())) {
                        uncategorizedFiles.remove(stat.getKey());
                    }
                }
            }
        }
        // final pass for uncategorized files
        long uncategorized = 0;
        for (String file : uncategorizedFiles) {
            uncategorized += stats.get(file);
        }
        Map<String, String> ret = new HashMap<>();
        for (Map.Entry<String, Long> e : result.entrySet()) {
            if (mMinReportSize > 0 && e.getValue() < mMinReportSize) {
                // has a min report size requirement and current category does not meet it
                CLog.v("Skipped reporting for %s (value %d): it's below threshold %d",
                        e.getKey(), e.getValue(), mMinReportSize);
                continue;
            }
            ret.put(e.getKey(), Long.toString(e.getValue()));
        }
        ret.put(LABEL_UNCATEGORIZED, Long.toString(uncategorized));
        ret.put(LABEL_TOTAL, Long.toString(total));
        ret.put(LABEL_CATEGORIZED, Long.toString(total - uncategorized));
        return ret;
    }
}
