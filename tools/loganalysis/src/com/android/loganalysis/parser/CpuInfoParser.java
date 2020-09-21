/*
 * Copyright (C) 2015 The Android Open Source Project
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

import com.android.loganalysis.item.CpuInfoItem;

import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * A {@link IParser} to handle the output of {@code cpuinfo}.
 */
public class CpuInfoParser implements IParser {

    // Example:
    //  0.1% 170/surfaceflinger: 0% user + 0% kernel
    private static final Pattern USAGE_PREFIX = Pattern.compile(
            "^ *\\+?(\\d+\\.?\\d*)\\% (\\d+)/([^ ]+): ");

    /**
     * {@inheritDoc}
     */
    @Override
    public CpuInfoItem parse(List<String> lines) {
        CpuInfoItem item = new CpuInfoItem();
        for (String line : lines) {
            Matcher m = USAGE_PREFIX.matcher(line);
            if (!m.lookingAt()) continue;

            if (m.groupCount() != 3) continue;

            int pid = Integer.parseInt(m.group(2));
            double percent = Double.parseDouble(m.group(1));
            String name = m.group(3);
            item.addRow(pid, percent, name);
        }

        return item;
    }
}
