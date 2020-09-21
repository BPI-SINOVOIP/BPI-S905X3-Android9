/*
 * Copyright (C) 2011 The Android Open Source Project
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

import com.android.loganalysis.item.MemInfoItem;
import com.android.loganalysis.util.ArrayUtil;

import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * A {@link IParser} to handle the output from {@code /proc/meminfo}.
 */
public class MemInfoParser implements IParser {

    /** Match a single MemoryInfo line, such as "MemFree:           65420 kB" */
    private static final Pattern INFO_LINE = Pattern.compile("^([^:]+):\\s+(\\d+) kB");

    /**
     * {@inheritDoc}
     *
     * @return The {@link MemInfoItem}.
     */
    @Override
    public MemInfoItem parse(List<String> lines) {
        final String text = ArrayUtil.join("\n", lines).trim();
        if ("".equals(text)) {
            return null;
        }

        MemInfoItem item = new MemInfoItem();
        item.setText(text);

        for (String line : lines) {
            Matcher m = INFO_LINE.matcher(line);
            if (m.matches()) {
                String key = m.group(1);
                try {
                    Long value = Long.parseLong(m.group(2));
                    item.put(key, value);
                } catch (NumberFormatException e) {
                    // Ignore
                }
            }
        }

        return item;
    }
}
