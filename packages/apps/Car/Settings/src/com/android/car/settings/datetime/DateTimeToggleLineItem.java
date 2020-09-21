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
 * limitations under the License
 */

package com.android.car.settings.datetime;

import android.content.Context;
import android.content.Intent;
import android.provider.Settings;

import androidx.car.widget.TextListItem;

/**
 * A LineItem that displays and sets system auto date/time.
 */
class DateTimeToggleLineItem extends TextListItem
        implements DatetimeSettingsFragment.ListRefreshObserver {
    private Context mContext;
    private final String mSettingString;

    public DateTimeToggleLineItem(Context context, String title, String desc,
            String settingString) {
        super(context);
        mContext = context;
        mSettingString = settingString;
        setTitle(title);
        setBody(desc);
        setSwitch(
                isChecked(),
                /* showDivider= */ false,
                (button, isChecked) -> {
                    Settings.Global.putInt(
                            mContext.getContentResolver(),
                            mSettingString,
                            isChecked ? 1 : 0);
                    mContext.sendBroadcast(new Intent(Intent.ACTION_TIME_CHANGED));
                });
    }

    @Override
    public void onPreRefresh() {
        setSwitchState(isChecked());
    }

    private boolean isChecked() {
        return Settings.Global.getInt(mContext.getContentResolver(), mSettingString, 0) > 0;
    }
}
