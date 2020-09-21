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
package com.android.loganalysis.parser;

import com.android.loganalysis.item.AnrItem;

import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * An {@link IParser} to handle ANRs.
 */
public class AnrParser implements IParser {
    /**
     * Matches: ANR (application not responding) in process: app
     * Matches: ANR in app
     * Matches: ANR in app (class/package)
     */
    public static final Pattern START = Pattern.compile(
            "^ANR (?:\\(application not responding\\) )?in (?:process: )?(\\S+).*$");
    /**
     * Matches: PID: 1234
     */
    private static final Pattern PID = Pattern.compile("^PID: (\\d+)$");
    /**
     * Matches: Reason: reason
     */
    private static final Pattern REASON = Pattern.compile("^Reason: (.*)$");
    /**
     * Matches: Load: 0.71 / 0.83 / 0.51
     */
    private static final Pattern LOAD = Pattern.compile(
            "^Load: (\\d+\\.\\d+) / (\\d+\\.\\d+) / (\\d+\\.\\d+)$");

    /**
     * Matches: 33% TOTAL: 21% user + 11% kernel + 0.3% iowait
     */
    private static final Pattern TOTAL = Pattern.compile("^(\\d+(\\.\\d+)?)% TOTAL: .*$");
    private static final Pattern USER = Pattern.compile("^.* (\\d+(\\.\\d+)?)% user.*$");
    private static final Pattern KERNEL = Pattern.compile("^.* (\\d+(\\.\\d+)?)% kernel.*$");
    private static final Pattern IOWAIT = Pattern.compile("^.* (\\d+(\\.\\d+)?)% iowait.*$");

    /**
     * {@inheritDoc}
     *
     * @return The {@link AnrItem}.
     */
    @Override
    public AnrItem parse(List<String> lines) {
        AnrItem anr = null;
        StringBuilder stack = new StringBuilder();
        boolean matchedTotal = false;

        for (String line : lines) {
            Matcher m = START.matcher(line);
            // Ignore all input until the start pattern is matched.
            if (m.matches()) {
                anr = new AnrItem();
                anr.setApp(m.group(1));
            }

            if (anr != null) {
                m = PID.matcher(line);
                if (m.matches()) {
                    anr.setPid(Integer.valueOf(m.group(1)));
                }
                m = REASON.matcher(line);
                if (m.matches()) {
                    anr.setReason(m.group(1));
                }

                m = LOAD.matcher(line);
                if (m.matches()) {
                    anr.setLoad(AnrItem.LoadCategory.LOAD_1, Double.parseDouble(m.group(1)));
                    anr.setLoad(AnrItem.LoadCategory.LOAD_5, Double.parseDouble(m.group(2)));
                    anr.setLoad(AnrItem.LoadCategory.LOAD_15, Double.parseDouble(m.group(3)));
                }

                m = TOTAL.matcher(line);
                if (!matchedTotal && m.matches()) {
                    matchedTotal = true;
                    anr.setCpuUsage(AnrItem.CpuUsageCategory.TOTAL, Double.parseDouble(m.group(1)));

                    m = USER.matcher(line);
                    Double usage = m.matches() ? Double.parseDouble(m.group(1)) : 0.0;
                    anr.setCpuUsage(AnrItem.CpuUsageCategory.USER, usage);

                    m = KERNEL.matcher(line);
                    usage = m.matches() ? Double.parseDouble(m.group(1)) : 0.0;
                    anr.setCpuUsage(AnrItem.CpuUsageCategory.KERNEL, usage);

                    m = IOWAIT.matcher(line);
                    usage = m.matches() ? Double.parseDouble(m.group(1)) : 0.0;
                    anr.setCpuUsage(AnrItem.CpuUsageCategory.IOWAIT, usage);
                }

                stack.append(line);
                stack.append("\n");
            }
        }

        if (anr != null) {
            anr.setStack(stack.toString().trim());
        }
        return anr;
    }
}

