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

package com.android.tv.settings.name;

import android.content.Context;
import android.content.SharedPreferences;

/**
 * The class to store the status of device name suggestion indicating whether it is finished.
 */
public class DeviceNameSuggestionStatus {
    private static final String SUGGESTION_STATUS_STORAGE_FILE_NAME = "suggestionStatusStorage";
    private static final String IS_SUGGESTION_FINISHED = "IsSuggestionFinished";
    private SharedPreferences mSharedPreferences;
    private static DeviceNameSuggestionStatus sInstance;

    /**
     * Create the instance if it does not exist.
     */
    public static DeviceNameSuggestionStatus getInstance(Context context) {
        if (sInstance == null) {
            sInstance = new DeviceNameSuggestionStatus(context);
        }
        return sInstance;
    }

    private DeviceNameSuggestionStatus(Context context) {
        mSharedPreferences = context.getSharedPreferences(
                SUGGESTION_STATUS_STORAGE_FILE_NAME, Context.MODE_PRIVATE);
    }

    /**
     * Set the suggestion to be finished.
     */
    public void setFinished() {
        if (!isFinished()) {
            mSharedPreferences.edit().putBoolean(IS_SUGGESTION_FINISHED, true).apply();
        }
    }

    /**
     * Return the status of the suggestion.
     * @return True if finished.
     */
    public boolean isFinished() {
        return mSharedPreferences.getBoolean(IS_SUGGESTION_FINISHED, false);
    }
}
