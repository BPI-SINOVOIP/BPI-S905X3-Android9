/*
 * Copyright (C) 2016 The Android Open Source Project
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
package com.android.tradefed.util;

import com.android.tradefed.log.LogUtil.CLog;

import java.util.ArrayList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * A utility class to parse simpleperf result.
 * <p/>
 * Should be useful when implementing test result receiver
 *
 * @see <a href="https://android.googlesource.com/platform/system/extras/+/master/simpleperf/">
 * Introduction of simpleperf</a>
 */
public final class SimplePerfStatResultParser {

    // static immutable field
    private static final String SIMPLEPERF_METRIC_HEAD = "Performance counter statistics:";
    private static final Pattern TOTAL_TIME_SENTENCE_PATTERN = Pattern
            .compile("Total test time: (\\d+\\.\\d+) seconds.");
    private static final Pattern TOTAL_METRIC_SENTENCE_PATTERN = Pattern
            .compile("\\s*([0-9,\\.]*?)(\\(ms\\))?\\s+([0-9a-z_:-]*?)\\s+#\\s+([^#]*)");

    // Private constructor to prevent other program to have an instance
    private SimplePerfStatResultParser() {
        // Do nothing
    }

    /**
     * Utility method to parse single line of simpleperf results
     *
     * @param line single line of simpleperf results
     * @return List of String contains information. If length is 0, no output parsed. If length is
     * 1, it is total time. If length is 3, it contains metric(pos 0), benchmark(pos 1) and
     * comment(pos 2)
     */
    public static List<String> parseSingleLine(String line) {
        List<String> result = new ArrayList<>();

        if (line == null) {
            return result;
        }

        // Skip header line and empty line
        if (line.contains(SIMPLEPERF_METRIC_HEAD) || line.equals("")) {
            return result;
        }

        // Last line contains total test time
        Matcher matcher;
        matcher = TOTAL_TIME_SENTENCE_PATTERN.matcher(line);
        if (matcher.matches()) {
            result.add(matcher.group(1));
            return result;
        }

        // Metric -- benchmark -- comment line
        matcher = TOTAL_METRIC_SENTENCE_PATTERN.matcher(line);
        if (matcher.matches()) {
            result.add(matcher.group(1));
            result.add(matcher.group(3));
            result.add(matcher.group(4));
        }
        return result;
    }

    /**
     * Utility method to parse multiple lines of simpleperf output
     *
     * @param output multiple lines of string
     * @return {@link SimplePerfResult} object to hold simpleperf result information
     */
    public static SimplePerfResult parseRawOutput(String output) {
        SimplePerfResult result = new SimplePerfResult();
        if (output == null) {
            return null;
        }
        int idx = output.indexOf(SIMPLEPERF_METRIC_HEAD);
        if (idx == -1) {
            CLog.e("Cannot find simpleperf metric head message");
            return null;
        } else {
            result.setCommandRawOutput(output.substring(0, idx));
            String simpleperfOutput = output.substring(idx + SIMPLEPERF_METRIC_HEAD.length() + 1);
            result.setSimplePerfRawOutput(simpleperfOutput);
            String[] lines = simpleperfOutput.split("\n");
            for (String line : lines) {
                List<String> singleLineResults = parseSingleLine(line);
                if (singleLineResults.size() == 1) {
                    result.setTotalTestTime(singleLineResults.get(0));
                } else if (singleLineResults.size() == 3) {
                    result.addBenchmarkComment(singleLineResults.get(1), singleLineResults.get(2));
                    result.addBenchmarkMetrics(singleLineResults.get(1), singleLineResults.get(0));
                } else {
                    CLog.i("Line skipped. " + line);
                }
            }
        }
        return result;
    }
}
