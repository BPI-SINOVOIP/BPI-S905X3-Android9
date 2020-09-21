/*
 * Copyright (C) 2016 The Android Open Source Project
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
package com.android.tradefed.util;

import jline.Completor;

import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Implementation of the {@link Completor} for our TF configurations.
 */
public class ConfigCompletor implements Completor {
    private List<String> mListConfig;
    private static final String RUN_PATTERN = "(run +)(.*)?";
    private Pattern mRunPattern;

    public ConfigCompletor(List<String> listConfig) {
        mListConfig = listConfig;
        mRunPattern = Pattern.compile(RUN_PATTERN);
    }

    /**
     * {@inheritDoc}
     */
    @SuppressWarnings({ "rawtypes", "unchecked" })
    @Override
    public int complete(String buffer, int cursor, List candidates) {
        // if the only thing on the line is "run" we expose all possibilities.
        if (buffer.trim().equals("run")) {
            candidates.addAll(mListConfig);
        } else if (buffer.startsWith("run ")) {
            Matcher matcher = mRunPattern.matcher(buffer);
            if (matcher.find()) {
                for (Object configName : mListConfig) {
                    String match = matcher.group(1) + configName;
                    if (match.startsWith(buffer)) {
                        candidates.add(configName);
                    }
                }
                return buffer.length() - matcher.group(2).length();
            }
        }
        return cursor;
    }
}
