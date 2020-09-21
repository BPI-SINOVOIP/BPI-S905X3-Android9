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
package com.android.loganalysis.parser;

import com.android.loganalysis.item.DumpsysProcessMeminfoItem;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * An {@link IParser} used to parse output from `dumpsys meminfo --checkin PROCESS` where PROCESS is
 * from the output of `dumpsys meminfo`. Data is stored as a map of categories to a map of
 * measurement types to values. Format is from {@link android.app.ActivityThread#dumpMemInfoTable}.
 */
public class DumpsysProcessMeminfoParser implements IParser {

    // Order is VERSION,PID,NAME,[native,dalvik,other,total]{11},[name,val{8}]*
    private static final Pattern MEMINFO_OUTPUT =
            Pattern.compile("(\\d+),(\\d+),([^,]+),((?:(?:N/A|\\d+),){44})(.*)");
    // Matches the ending [name,val{8}]
    private static final Pattern MEMINFO_ADDITIONAL_OUTPUT =
            Pattern.compile("([^,]+),((?:(?:N/A|\\d+),){8})");
    // Matches a value with comma
    private static final Pattern MEMINFO_VALUE = Pattern.compile("(N/A|\\d+),");

    @Override
    public DumpsysProcessMeminfoItem parse(List<String> lines) {
        DumpsysProcessMeminfoItem item = new DumpsysProcessMeminfoItem();
        for (String line : lines) {
            Matcher m = MEMINFO_OUTPUT.matcher(line);
            if (!m.matches()) continue;
            try {
                item.setPid(Integer.parseInt(m.group(2)));
            } catch (NumberFormatException e) {
                // skip
            }
            item.setProcessName(m.group(3));
            // parse memory info main areas
            String mainValues = m.group(4);
            Matcher mainMatcher = MEMINFO_VALUE.matcher(mainValues);
            Map<String, Long> nativeData = item.get(DumpsysProcessMeminfoItem.NATIVE);
            Map<String, Long> dalvikData = item.get(DumpsysProcessMeminfoItem.DALVIK);
            Map<String, Long> otherData = item.get(DumpsysProcessMeminfoItem.OTHER);
            Map<String, Long> totalData = item.get(DumpsysProcessMeminfoItem.TOTAL);
            for (int i = 0; i < DumpsysProcessMeminfoItem.MAIN_OUTPUT_ORDER.length; i++) {
                String curMeasurement = DumpsysProcessMeminfoItem.MAIN_OUTPUT_ORDER[i];
                parseNextValue(mainMatcher, nativeData, curMeasurement);
                parseNextValue(mainMatcher, dalvikData, curMeasurement);
                parseNextValue(mainMatcher, otherData, curMeasurement);
                parseNextValue(mainMatcher, totalData, curMeasurement);
            }
            String additionalData = m.group(5);
            Matcher additionalMatcher = MEMINFO_ADDITIONAL_OUTPUT.matcher(additionalData);
            // parse memory info other areas
            while (additionalMatcher.find()) {
                try {
                    String curLabel = additionalMatcher.group(1);
                    Matcher additionalValueMatcher =
                            MEMINFO_VALUE.matcher(additionalMatcher.group(2));
                    Map<String, Long> curData = new HashMap<>();
                    for (int i = 0; i < DumpsysProcessMeminfoItem.OTHER_OUTPUT_ORDER.length; i++) {
                        String curMeasurement = DumpsysProcessMeminfoItem.OTHER_OUTPUT_ORDER[i];
                        parseNextValue(additionalValueMatcher, curData, curMeasurement);
                    }
                    item.put(curLabel, curData);
                } catch (ArrayIndexOutOfBoundsException e) {
                    break;
                }
            }
        }
        return item;
    }

    private void parseNextValue(Matcher m, Map<String, Long> output, String key) {
        if (!m.find()) return;
        String value = m.group(1);
        if ("N/A".equals(value)) return;
        try {
            output.put(key, Long.parseLong(value));
        } catch (NumberFormatException e) {
            // skip
        }
    }
}
