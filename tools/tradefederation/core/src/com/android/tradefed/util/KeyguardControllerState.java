/*
 * Copyright (C) 2017 The Android Open Source Project
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

import com.android.tradefed.log.LogUtil.CLog;

import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/** A container for the keyguard states. Inspired from ActivityManagerState.java. */
public class KeyguardControllerState {
    private static final Pattern NAME_PATTERN = Pattern.compile(".*KeyguardController:");
    private static final Pattern SHOWING_PATTERN = Pattern.compile("mKeyguardShowing=(\\S+)");
    private static final Pattern OCCLUDED_PATTERN = Pattern.compile("mOccluded=(\\S+)");

    private boolean mKeyguardShowing;
    private boolean mKeyguardOccluded;

    private KeyguardControllerState() {}

    /**
     * Creates and populate a {@link KeyguardControllerState} based on a dumpsys output from the
     * KeyguardController.
     *
     * @param dump output from the dumpsys activity activities
     * @return The {@link KeyguardControllerState} representing the output or null if invalid output
     *     is provided.
     */
    public static KeyguardControllerState create(List<String> dump) {
        final String line = dump.get(0);

        final Matcher matcher = NAME_PATTERN.matcher(line);
        if (!matcher.matches()) {
            // Not KeyguardController
            return null;
        }

        final KeyguardControllerState controller = new KeyguardControllerState();
        controller.extract(dump);
        return controller;
    }

    private void extract(List<String> dump) {
        for (String log : dump) {
            String line = log.trim();
            Matcher matcher = SHOWING_PATTERN.matcher(line);
            if (matcher.matches()) {
                CLog.v(line);
                final String showingString = matcher.group(1);
                mKeyguardShowing = Boolean.valueOf(showingString);
                CLog.v(showingString);
                continue;
            }

            matcher = OCCLUDED_PATTERN.matcher(line);
            if (matcher.matches()) {
                CLog.v(line);
                final String occludedString = matcher.group(1);
                mKeyguardOccluded = Boolean.valueOf(occludedString);
                CLog.v(occludedString);
                continue;
            }
        }
    }

    /** Returns True if the keyguard is showing, false otherwise. */
    public boolean isKeyguardShowing() {
        return mKeyguardShowing;
    }

    /** Returns True if the keyguard is occluded, false otherwise. */
    public boolean isKeyguardOccluded() {
        return mKeyguardOccluded;
    }
}
