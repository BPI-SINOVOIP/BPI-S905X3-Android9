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

import com.android.loganalysis.item.JavaCrashItem;

import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * An {@link IParser} to handle Java crashes.
 */
public class JavaCrashParser implements IParser {

    /**
     * Matches: java.lang.Exception
     * Matches: java.lang.Exception: reason
     */
    private static final Pattern EXCEPTION = Pattern.compile("^([^\\s:]+)(: (.*))?$");
    /**
     * Matches: Caused by: java.lang.Exception
     */
    private static final Pattern CAUSEDBY = Pattern.compile("^Caused by: .+$");
    /**
     * Matches: \tat class.method(Class.java:1)
     */
    private static final Pattern AT = Pattern.compile("^\tat .+$");

    // Sometimes logcat explicitly marks where exception begins and ends
    private static final String BEGIN_MARKER = "----- begin exception -----";
    private static final String END_MARKER = "----- end exception -----";

    /**
     * {@inheritDoc}
     *
     * @return The {@link JavaCrashItem}.
     */
    @Override
    public JavaCrashItem parse(List<String> lines) {
        JavaCrashItem jc = null;
        StringBuilder stack = new StringBuilder();
        StringBuilder message = new StringBuilder();
        boolean inMessage = false;
        boolean inCausedBy = false;
        boolean inStack = false;

        for (String line : lines) {
            if (line.contains(BEGIN_MARKER)) {
                inMessage = false;
                inCausedBy = false;
                inStack = false;
                stack = new StringBuilder();
                message = new StringBuilder();
                jc = null;
                continue;
            }
            if (line.contains(END_MARKER)) {
                break;
            }
            if (!inStack) {
                Matcher exceptionMatch = EXCEPTION.matcher(line);
                if (exceptionMatch.matches()) {
                    inMessage = true;
                    inStack = true;

                    jc = new JavaCrashItem();
                    jc.setException(exceptionMatch.group(1));
                    if (exceptionMatch.group(3) != null) {
                        message.append(exceptionMatch.group(3));
                    }
                }
            } else {
                // Match: Caused by: java.lang.Exception
                Matcher causedByMatch = CAUSEDBY.matcher(line);
                if (causedByMatch.matches()) {
                    inMessage = false;
                    inCausedBy = true;
                }

                // Match: \tat class.method(Class.java:1)
                Matcher atMatch = AT.matcher(line);
                if (atMatch.matches()) {
                    inMessage = false;
                    inCausedBy = false;
                }

                if (!causedByMatch.matches() && !atMatch.matches()) {
                    if (inMessage) {
                        message.append("\n");
                        message.append(line);
                    }
                    if (!inMessage && !inCausedBy) {
                        addMessageStack(jc, message.toString(), stack.toString());
                        return jc;
                    }
                }
            }

            if (inStack) {
                stack.append(line);
                stack.append("\n");
            }
        }

        addMessageStack(jc, message.toString(), stack.toString());
        return jc;
    }

    /**
     * Adds the message and stack to the {@link JavaCrashItem}.
     */
    private void addMessageStack(JavaCrashItem jc, String message, String stack) {
        if (jc != null) {
            if (message.length() > 0) {
                jc.setMessage(message);
            }
            jc.setStack(stack.trim());
        }
    }
}

