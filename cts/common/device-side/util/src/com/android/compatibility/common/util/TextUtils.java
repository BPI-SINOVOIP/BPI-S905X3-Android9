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

package com.android.compatibility.common.util;

import java.util.regex.Pattern;

public class TextUtils {
    private TextUtils() {
    }

    /**
     * Return the first section in {@code source} between the line matches
     * {@code extractionStartRegex} and the line matches {@code extractionEndRegex}.
     */
    public static String extractSection(String source,
            String extractionStartRegex, boolean startInclusive,
            String extractionEndRegex, boolean endInclusive) {

        final Pattern start = Pattern.compile(extractionStartRegex);
        final Pattern end = Pattern.compile(extractionEndRegex);

        final StringBuilder sb = new StringBuilder();
        final String[] lines = source.split("\\n", -1);

        int i = 0;
        for (; i < lines.length; i++) {
            final String line = lines[i];
            if (start.matcher(line).matches()) {
                if (startInclusive) {
                    sb.append(line);
                    sb.append('\n');
                }
                i++;
                break;
            }
        }

        for (; i < lines.length; i++) {
            final String line = lines[i];
            if (end.matcher(line).matches()) {
                if (endInclusive) {
                    sb.append(line);
                    sb.append('\n');
                }
                break;
            }
            sb.append(line);
            sb.append('\n');
        }
        return sb.toString();
    }
}
