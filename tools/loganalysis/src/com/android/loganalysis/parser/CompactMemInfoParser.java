/*
 * Copyright (C) 2014 The Android Open Source Project
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

import com.android.loganalysis.item.CompactMemInfoItem;

import java.lang.NumberFormatException;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Parser for the compact meminfo output, from the 'dumpsys meminfo -c' command.
 * The output is a csv file which contains the information about how processes use memory.
 * For now we are only interested in the pss of the processes. So we only parse the lines
 * that start with proc and skip everything else.
 *
 * The format of the line is as follows:
 * "proc,[type],[name],[pid],[pss],[activities].
 *
 * Type is the type of the process for example native, cached, foreground, etc.
 * Name is the name of the process.
 * Activities indicates if a process has any activities associated with it.
 *
 */
public class CompactMemInfoParser implements IParser {
    private static final Pattern PROC_PATTERN =
            Pattern.compile("proc,(\\w+),([a-zA-Z_0-9\\.]+),(\\d+),(\\d+),((\\S+),)?(.*)");
    private static final Pattern LOST_RAM_PATTERN = Pattern.compile("lostram,(\\d+)");
    private static final Pattern RAM_PATTERN = Pattern.compile("ram,(\\d+),(\\d+),(\\d+)");
    private static final Pattern ZRAM_PATTERN = Pattern.compile("zram,(\\d+),(\\d+),(\\d+)");
    private static final Pattern TUNING_PATTERN = Pattern.compile("tuning,(\\d+),(\\d+),(\\d+).*");

    /**
     * Parse compact meminfo log. Output a CompactMemInfoItem which contains
     * the list of processes, their pids and their pss.
     */
    @Override
    public CompactMemInfoItem parse(List<String> lines) {
        CompactMemInfoItem item = new CompactMemInfoItem();
        for (String line : lines) {
            Matcher m = PROC_PATTERN.matcher(line);
            if (m.matches()) {
                String type = m.group(1);
                String name = m.group(2);
                try {
                    int pid = Integer.parseInt(m.group(3));
                    long pss = Long.parseLong(m.group(4));
                    long swap = 0;
                    if (m.group(6) != null && !"N/A".equals(m.group(6))) {
                        swap = Long.parseLong(m.group(6));
                    }
                    boolean activities = "a".equals(m.group(7));
                    item.addPid(pid, name, type, pss, swap, activities);
                    continue;
                } catch (NumberFormatException nfe) {
                    // ignore exception
                }
            }

            m = LOST_RAM_PATTERN.matcher(line);
            if (m.matches()) {
                try {
                    long lostRam = Long.parseLong(m.group(1));
                    item.setLostRam(lostRam);
                    continue;
                } catch (NumberFormatException nfe) {
                    // ignore exception
                }
            }

            m = RAM_PATTERN.matcher(line);
            if (m.matches()) {
                try {
                    item.setFreeRam(Long.parseLong(m.group(2)));
                    continue;
                } catch (NumberFormatException nfe) {
                    // ignore exception
                }
            }

            m = ZRAM_PATTERN.matcher(line);
            if (m.matches()) {
                try {
                    item.setTotalZram(Long.parseLong(m.group(1)));
                    item.setFreeSwapZram(Long.parseLong(m.group(3)));
                    continue;
                } catch (NumberFormatException nfe) {
                    // ignore exception
                }
            }

            m = TUNING_PATTERN.matcher(line);
            if (m.matches()) {
                try {
                    item.setTuningLevel(Long.parseLong(m.group(3)));
                    continue;
                } catch (NumberFormatException nfe) {
                    // ignore exception
                }
            }
        }
        return item;
    }
}
