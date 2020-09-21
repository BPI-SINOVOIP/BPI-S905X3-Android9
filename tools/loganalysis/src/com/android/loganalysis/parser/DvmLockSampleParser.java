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

import com.google.common.annotations.VisibleForTesting;

import com.android.loganalysis.item.DvmLockSampleItem;

import java.io.BufferedReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * A {@link IParser} to parse DVM lock sample allocation logs
 */
public class DvmLockSampleParser implements IParser {

    private static final String NAME_REGEX = "([^,]+)";
    private static final String FILE_REGEX = "(-|[A-Za-z]+\\.[A-Za-z]+)";
    private static final String INT_REGEX = "(\\d+)";

    /**
     * Matches the DVM lock sample log format:
     *
     * <p>09-04 05:40:07.809 1026 10592 I dvm_lock_sample:
     * [system_server,1,Binder:1026_F,46,NetworkPolicyManagerService.java,2284,-,802,9]
     */
    private static final Pattern LOG_CONTENTION_EVENT_PATTERN =
            Pattern.compile(
                    "\\["
                            + String.join(
                                    ",\\s*",
                                    Arrays.asList(
                                            NAME_REGEX, // Process name
                                            INT_REGEX, // Process sensitivity flag
                                            NAME_REGEX, // Waiting thread name
                                            INT_REGEX, // Wait time
                                            FILE_REGEX, // Waiting Source File
                                            INT_REGEX, // Waiting Source Line
                                            FILE_REGEX, // Owner File Name ("-" if the same)
                                            INT_REGEX, // Owner Acquire Source Line
                                            INT_REGEX // Sample Percentage
                                            ))
                            + "\\]");

    private DvmLockSampleItem mItem = new DvmLockSampleItem();

    /**
     * Parse a dvm log from a {@link BufferedReader} into a {@link DvmLockSampleItem}.
     *
     * @return The {@link DvmLockSampleItem}.
     * @see #parse(List)
     */
    public DvmLockSampleItem parse(BufferedReader input) throws IOException {
        List<String> lines = new ArrayList<>();

        for(String line = input.readLine(); line != null; line = input.readLine()) {
            lines.add(line);
        }

        return parse(lines);
    }

    /**
     * {@inheritDoc}
     *
     * @return The {@link DvmLockSampleItem}.
     */
    @Override
    public DvmLockSampleItem parse(List<String> lines) {
        DvmLockSampleItem mItem = new DvmLockSampleItem();

        for (String line : lines) {
            Matcher m = LOG_CONTENTION_EVENT_PATTERN.matcher(line);

            if(m.matches()) {
                mItem.setAttribute(DvmLockSampleItem.PROCESS_NAME,
                        m.group(1));

                mItem.setAttribute(DvmLockSampleItem.SENSITIVITY_FLAG,
                        1 == Integer.parseInt(m.group(2)));

                mItem.setAttribute(DvmLockSampleItem.WAITING_THREAD_NAME,
                        m.group(3));

                mItem.setAttribute(DvmLockSampleItem.WAIT_TIME,
                        Integer.parseInt(m.group(4)));

                mItem.setAttribute(DvmLockSampleItem.WAITING_SOURCE_FILE,
                        m.group(5));

                mItem.setAttribute(DvmLockSampleItem.WAITING_SOURCE_LINE,
                        Integer.parseInt(m.group(6)));

                // If the owner file name is -, the dvm log format specification
                // says that we should use the waiting source file.
                mItem.setAttribute(DvmLockSampleItem.OWNER_FILE_NAME,
                        m.group(7).equals("-") ? m.group(5) : m.group(7));

                mItem.setAttribute(DvmLockSampleItem.OWNER_ACQUIRE_SOURCE_LINE,
                        Integer.parseInt(m.group(8)));

                mItem.setAttribute(DvmLockSampleItem.SAMPLE_PERCENTAGE,
                        Integer.parseInt(m.group(9)));
            }
        }

        System.out.println(mItem.toJson().toString());
        return mItem;
    }

    @VisibleForTesting
    DvmLockSampleItem getItem() {
        return mItem;
    }
}
