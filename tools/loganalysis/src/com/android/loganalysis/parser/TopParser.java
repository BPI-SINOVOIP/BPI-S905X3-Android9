/*
 * Copyright (C) 2013 The Android Open Source Project
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

import com.android.loganalysis.item.TopItem;
import com.android.loganalysis.util.ArrayUtil;

import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * A {@link IParser} to handle the output of the top command output.
 * <p>
 * The parser only record the last top entry if multiple entries are printed by the top command. It
 * only parses the total cpu usage.
 * </p>
 */
public class TopParser implements IParser {

    /**
     * Match a valid cpu ticks line, such as:
     * "User 5 + Nice 0 + Sys 14 + Idle 207 + IOW 0 + IRQ 0 + SIRQ 0 = 226"
     */
    private static final Pattern TICKS_PAT = Pattern.compile(
            "User (\\d+) \\+ Nice (\\d+) \\+ Sys (\\d+) \\+ Idle (\\d+) \\+ IOW (\\d+) \\+ " +
            "IRQ (\\d+) \\+ SIRQ (\\d+) = (\\d+)");

    /**
     * {@inheritDoc}
     */
    @Override
    public TopItem parse(List<String> lines) {
        final String text = ArrayUtil.join("\n", lines).trim();
        if ("".equals(text)) {
            return null;
        }

        TopItem item = new TopItem();
        item.setText(text);

        for (String line : lines) {
            Matcher m = TICKS_PAT.matcher(line);
            if (m.matches()) {
                item.setUser(Integer.parseInt(m.group(1)));
                item.setNice(Integer.parseInt(m.group(2)));
                item.setSystem(Integer.parseInt(m.group(3)));
                item.setIdle(Integer.parseInt(m.group(4)));
                item.setIow(Integer.parseInt(m.group(5)));
                item.setIrq(Integer.parseInt(m.group(6)));
                item.setSirq(Integer.parseInt(m.group(7)));
                item.setTotal(Integer.parseInt(m.group(8)));
            }
        }
        return item;
    }
}
