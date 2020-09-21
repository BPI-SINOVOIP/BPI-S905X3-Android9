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

import com.android.loganalysis.item.SystemPropsItem;
import com.android.loganalysis.util.ArrayUtil;

import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * A {@link IParser} to handle the output from {@code getprop}.
 */
public class SystemPropsParser implements IParser {
    /** Match a single property line, such as "[gsm.sim.operator.numeric]: []" */
    private static final Pattern PROP_LINE = Pattern.compile("^\\[(.*)\\]: \\[(.*)\\]$");

    /**
     * {@inheritDoc}
     *
     * @return The {@link SystemPropsItem}.
     */
    @Override
    public SystemPropsItem parse(List<String> lines) {
        final String text = ArrayUtil.join("\n", lines).trim();
        if ("".equals(text)) {
            return null;
        }

        SystemPropsItem item = new SystemPropsItem();
        item.setText(text);

        for (String line : lines) {
            Matcher m = PROP_LINE.matcher(line);
            if (m.matches()) {
                item.put(m.group(1), m.group(2));
            }
        }
        return item;
    }
}

