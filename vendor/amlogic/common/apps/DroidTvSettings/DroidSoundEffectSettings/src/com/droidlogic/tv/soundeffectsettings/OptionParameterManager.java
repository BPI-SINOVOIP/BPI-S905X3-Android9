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
 * limitations under the License
 */

package com.droidlogic.tv.soundeffectsettings;

import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.text.TextUtils;
import android.util.Log;
import android.provider.Settings;
import android.widget.Toast;
import android.content.SharedPreferences;

import java.lang.reflect.Method;
import java.lang.Boolean;

import com.droidlogic.tv.soundeffectsettings.R;

public class OptionParameterManager {

    public static final String TAG = "OptionParameterManager";
	public static final boolean DEBUG = true;
    public static final String KEY_MENU_TIME = "menu_time";//DroidLogicTvUtils.KEY_MENU_TIME;
    public static final int DEFUALT_MENU_TIME = 1;//DroidLogicTvUtils.DEFUALT_MENU_TIME;

    private Resources mResources;
    private Context mContext;

    public OptionParameterManager (Context context) {
        mContext = context;
        mResources = mContext.getResources();
    }

    public static boolean CanDebug() {
        return DEBUG;
    }

    public static boolean getBoolean(String key, boolean defaultValue) {
        try {
            final Class<?> systemProperties = Class.forName("android.os.SystemProperties");
            final Method get = systemProperties.getMethod("getBoolean", String.class, Boolean.TYPE);
            return (boolean) get.invoke(null, key, defaultValue);
        } catch (Exception e) {
            Log.e(TAG, "Error getting boolean for  " + key, e);
            // This should never happen
            return defaultValue;
        }
    }

    public static String getString(String key, String defaultValue) {
        try {
            final Class<?> systemProperties = Class.forName("android.os.SystemProperties");
            final Method get = systemProperties.getMethod("getBoolean", String.class, String.class);
            return (String) get.invoke(null, key, defaultValue);
        } catch (Exception e) {
            Log.e(TAG, "Error getting string for  " + key, e);
            // This should never happen
            return defaultValue;
        }
    }
}
