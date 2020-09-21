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

public class SettingsUtils {
    private SettingsUtils() {
    }

    /**
     * Put a global setting.
     */
    public static void putGlobalSetting(String key, String value) {
        // Hmm, technically we should escape a value, but if I do like '1', it won't work. ??
        SystemUtil.runShellCommandForNoOutput("settings put global " + key + " " + value);
    }

    /**
     * Put a global setting for the current (foreground) user.
     */
    public static void putSecureSetting(String key, String value) {
        SystemUtil.runShellCommandForNoOutput(
                "settings --user current put secure " + key + " " + value);
    }
}
