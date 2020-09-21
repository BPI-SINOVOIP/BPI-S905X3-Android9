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

import com.android.loganalysis.item.TracesItem;

import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * A {@link IParser} to parse Android traces files.
 * <p>
 * For now, this only extracts the main stack trace from the first process. It is used to get a
 * stack from {@code /data/anr/traces.txt} which can be used to give some context about the ANR. If
 * there is a need, this parser can be expanded to parse all stacks from all processes.
 */
public class TracesParser implements IParser {

    /**
     * Matches: ----- pid PID at YYYY-MM-DD hh:mm:ss -----
     */
    private static final Pattern PID = Pattern.compile(
            "^----- pid (\\d+) at \\d{4}-\\d{2}-\\d{2} \\d{2}:\\d{2}:\\d{2} -----$");

    /**
     * Matches: Cmd line: APP
     */
    private static final Pattern APP = Pattern.compile("^Cmd ?line: (\\S+).*$");

    /**
     * Matches: "main" prio=5 tid=1 STATE
     */
    private static final Pattern STACK = Pattern.compile("^\"main\" .*$");

    /**
     * {@inheritDoc}
     *
     * @return The {@link TracesItem}.
     */
    @Override
    public TracesItem parse(List<String> lines) {
        TracesItem traces = new TracesItem();
        StringBuffer stack = null;

        for (String line : lines) {
            if (stack == null) {
                Matcher m = PID.matcher(line);
                if (m.matches()) {
                    traces.setPid(Integer.parseInt(m.group(1)));
                }
                m = APP.matcher(line);
                if (m.matches()) {
                    traces.setApp(m.group(1));
                }
                m = STACK.matcher(line);
                if (m.matches()) {
                    stack = new StringBuffer();
                    stack.append(line);
                    stack.append("\n");
                }
            } else if (!"".equals(line)) {
                stack.append(line);
                stack.append("\n");
            } else {
                traces.setStack(stack.toString().trim());
                return traces;
            }
        }
        if (stack == null) {
            return null;
        }
        traces.setStack(stack.toString().trim());
        return traces;
    }

}
