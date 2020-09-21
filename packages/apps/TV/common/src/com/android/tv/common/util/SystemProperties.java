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

package com.android.tv.common.util;

import com.android.tv.common.BooleanSystemProperty;

/** A convenience class for getting TV related system properties. */
public final class SystemProperties {

    /** Allow Google Analytics for eng builds. */
    public static final BooleanSystemProperty ALLOW_ANALYTICS_IN_ENG =
            new BooleanSystemProperty("tv_allow_analytics_in_eng", false);

    /** Allow Strict mode for debug builds. */
    public static final BooleanSystemProperty ALLOW_STRICT_MODE =
            new BooleanSystemProperty("tv_allow_strict_mode", true);

    /** When true {@link android.view.KeyEvent}s are logged. Defaults to false. */
    public static final BooleanSystemProperty LOG_KEYEVENT =
            new BooleanSystemProperty("tv_log_keyevent", false);
    /** When true debug keys are used. Defaults to false. */
    public static final BooleanSystemProperty USE_DEBUG_KEYS =
            new BooleanSystemProperty("tv_use_debug_keys", false);

    /** Send {@link com.android.tv.analytics.Tracker} information. Defaults to {@code true}. */
    public static final BooleanSystemProperty USE_TRACKER =
            new BooleanSystemProperty("tv_use_tracker", true);

    public static final BooleanSystemProperty USE_KEY =
            new BooleanSystemProperty("sys.platform.customize_key", false);

    public static final BooleanSystemProperty USE_DEBUG_TRACE =
            new BooleanSystemProperty("tv_use_debug_trace", false);

    public static final BooleanSystemProperty USE_DEBUG_RATINGS =
            new BooleanSystemProperty("tv_use_debug_ratings", false);

    public static final BooleanSystemProperty USE_DEBUG_CHANNEL_UPDATE =
            new BooleanSystemProperty("tv_use_debug_channel_update", false);

    public static final BooleanSystemProperty USE_DEBUG_TUNE_CONTROL =
            new BooleanSystemProperty("tv_use_tune_control", true);//default tune by two steps

    public static final BooleanSystemProperty USE_DEBUG_LOGIC =
            new BooleanSystemProperty("tv_use_debug_logic", false);

    public static final BooleanSystemProperty USE_DEBUG_PVR =
            new BooleanSystemProperty("tv_use_debug_pvr", false);

    public static final BooleanSystemProperty USE_DEBUG_TIMESHIFT =
            new BooleanSystemProperty("tv_use_debug_timeshift", false);

    public static final BooleanSystemProperty USE_DEBUG_ENABLE_SUSPEND_RECORD =
            new BooleanSystemProperty("tv_use_debug_enable_suspend_record", true);

    public static final BooleanSystemProperty USE_DEBUG_DISPLAY =
            new BooleanSystemProperty("tv_use_debug_display", false);

    public static final BooleanSystemProperty USE_DEBUG_LOCK =
            new BooleanSystemProperty("tv_use_debug_lock", false);

    static {
        updateSystemProperties();
    }

    private SystemProperties() {}

    /** Update the TV related system properties. */
    public static void updateSystemProperties() {
        BooleanSystemProperty.resetAll();
    }
}
