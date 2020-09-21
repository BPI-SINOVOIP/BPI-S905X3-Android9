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

import android.util.Log;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class AppStandbyUtils {
    private static final String TAG = "CtsAppStandbyUtils";

    /**
     * Returns if app standby is enabled.
     *
     * @return true if enabled; or false if disabled.
     */
    public static boolean isAppStandbyEnabled() {
        final String result = SystemUtil.runShellCommand(
                "dumpsys usagestats is-app-standby-enabled").trim();
        return Boolean.parseBoolean(result);
    }

    /**
     * Sets enabled state for app standby feature for runtime switch.
     *
     * App standby feature has 2 switches. This one affects the switch at runtime. If the build
     * switch is off, enabling the runtime switch will not enable App standby.
     *
     * @param enabled if App standby is enabled.
     */
    public static void setAppStandbyEnabledAtRuntime(boolean enabled) {
        final String value = enabled ? "1" : "0";
        Log.d(TAG, "Setting AppStandby " + (enabled ? "enabled" : "disabled") + " at runtime.");
        SettingsUtils.putGlobalSetting("app_standby_enabled", value);
    }

    /**
     * Returns if app standby is enabled at runtime. Note {@link #isAppStandbyEnabled()} may still
     * return {@code false} if this method returns {@code true}, because app standby can be disabled
     * at build time as well.
     *
     * @return true if enabled at runtime; or false if disabled at runtime.
     */
    public static boolean isAppStandbyEnabledAtRuntime() {
        final String result =
                SystemUtil.runShellCommand("settings get global app_standby_enabled").trim();
        final boolean boolResult = result.equals("1");
        Log.d(TAG, "AppStandby is " + (boolResult ? "enabled" : "disabled") + " at runtime.");
        return boolResult;
    }
}
