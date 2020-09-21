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

import com.android.loganalysis.item.NativeCrashItem;

import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * An {@link IParser} to handle native crashes.
 */
public class NativeCrashParser implements IParser {

    /** Matches: Build fingerprint: 'fingerprint' */
    public static final Pattern FINGERPRINT = Pattern.compile("^Build fingerprint: '(.*)'$");
    /** Matches: pid: 957, tid: 963  >>> com.android.camera <<< */
    private static final Pattern APP = Pattern.compile(
            "^pid: (\\d+), tid: (\\d+)(, name: .+)?  >>> (\\S+) <<<$");

    /**
     * {@inheritDoc}
     *
     * @return The {@link NativeCrashItem}.
     */
    @Override
    public NativeCrashItem parse(List<String> lines) {
        NativeCrashItem nc = null;
        StringBuilder stack = new StringBuilder();

        for (String line : lines) {
            Matcher m = FINGERPRINT.matcher(line);
            if (m.matches()) {
                nc = new NativeCrashItem();
                nc.setFingerprint(m.group(1));
           }

            if (nc != null) {
                m = APP.matcher(line);
                if (m.matches()) {
                    nc.setPid(Integer.valueOf(m.group(1)));
                    nc.setTid(Integer.valueOf(m.group(2)));
                    nc.setApp(m.group(4));
                }

                stack.append(line);
                stack.append("\n");
            }
        }
        if (nc != null) {
            nc.setStack(stack.toString().trim());
        }
        return nc;
    }
}

