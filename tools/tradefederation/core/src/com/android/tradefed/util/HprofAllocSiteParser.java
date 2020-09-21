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
package com.android.tradefed.util;

import com.android.tradefed.log.LogUtil.CLog;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.HashMap;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/** Helper class to parse info from an Allocation Sites section of hprof reports. */
public class HprofAllocSiteParser {

    private static final String ALLOC_SITES_START_PATTERN = "SITES BEGIN";
    private static final String ALLOC_SITES_END_PATTERN = "SITES END";
    private boolean mHasAllocSiteStarted = false;
    // format:
    //            percent          live          alloc'ed  stack class
    //   rank   self  accum     bytes objs     bytes  objs trace name
    //      1 12.24% 12.24%  12441616    1  12441616     1 586322 byte[]
    private static final Pattern RANK_PATTERN =
            Pattern.compile(
                    "(\\s+)([0-9]*)(\\s+)([0-9]*\\.?[0-9]+%)(\\s+)([0-9]*\\.?[0-9]+%)(\\s+)"
                            + "([0-9]+)(\\s+)([0-9]+)(\\s+)([0-9]+)(\\s+)([0-9]+)(\\s+)([0-9]+)"
                            + "(\\s+)(.*)");

    /**
     * Parse a text hprof report.
     *
     * @param hprofReport file containing the hprof report.
     * @return a Map containing the results
     */
    public Map<String, String> parse(File hprofReport) throws IOException {
        Map<String, String> results = new HashMap<>();
        if (hprofReport == null || !hprofReport.exists()) {
            return results;
        }
        internalParse(hprofReport, results);
        return results;
    }

    /**
     * Actual parsing line by line of the report to extract information.
     *
     * @param report the {@link File} containing the hprof report.
     * @param currentRes the {@link Map} where the allocation sites will be stored.
     */
    private void internalParse(File report, Map<String, String> currentRes) throws IOException {
        try (BufferedReader br = new BufferedReader(new FileReader(report))) {
            for (String line; (line = br.readLine()) != null; ) {
                handleAllocSites(line, currentRes);
            }
        }
    }

    /** Handles the allocation sites in the hprof report. */
    private void handleAllocSites(String line, Map<String, String> currentRes) {
        if (line.startsWith(ALLOC_SITES_START_PATTERN)) {
            mHasAllocSiteStarted = true;
        } else if (line.startsWith(ALLOC_SITES_END_PATTERN)) {
            mHasAllocSiteStarted = false;
        } else if (mHasAllocSiteStarted) {
            Matcher m = RANK_PATTERN.matcher(line);
            if (m.find()) {
                CLog.d(
                        "Rank %s-%s-%s-%s-%s-%s-%s-%s-%s",
                        m.group(2),
                        m.group(4),
                        m.group(6),
                        m.group(8),
                        m.group(10),
                        m.group(12),
                        m.group(14),
                        m.group(16),
                        m.group(18));
                currentRes.put(String.format("Rank%s", m.group(2)), m.group(12));
            }
        }
    }
}
