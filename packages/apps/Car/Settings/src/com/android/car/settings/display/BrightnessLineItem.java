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

package com.android.car.settings.display;

import static android.provider.Settings.System.SCREEN_BRIGHTNESS;

import android.content.Context;
import android.provider.Settings;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;

import androidx.car.widget.SeekbarListItem;

import com.android.car.settings.R;
import com.android.car.settings.common.Logger;

/**
 * A LineItem that displays and sets display brightness.
 */
public class BrightnessLineItem extends SeekbarListItem {
    private static final Logger LOG = new Logger(BrightnessLineItem.class);
    private static final int MAX_BRIGHTNESS = 255;
    private final Context mContext;

    /**
     * Handles brightness change from user
     */
    private SeekBar.OnSeekBarChangeListener mOnSeekBarChangeListener =
            new OnSeekBarChangeListener() {
                @Override
                public void onStartTrackingTouch(SeekBar seekBar) {
                    // no-op
                }

                @Override
                public void onStopTrackingTouch(SeekBar seekBar) {
                    // no-op
                }

                @Override
                public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                    Settings.System.putInt(
                            mContext.getContentResolver(), SCREEN_BRIGHTNESS, progress);
                }
            };

    public BrightnessLineItem(Context context) {
        super(context);
        mContext = context;
        setMax(MAX_BRIGHTNESS);
        setProgress(getSeekbarValue(context));
        setOnSeekBarChangeListener(mOnSeekBarChangeListener);
        setText(context.getString(R.string.brightness));
    }

    private static int getSeekbarValue(Context context) {
        int currentBrightness = 0;
        try {
            currentBrightness = Settings.System.getInt(context.getContentResolver(),
                    SCREEN_BRIGHTNESS);
        } catch (Settings.SettingNotFoundException e) {
            LOG.w("Can't find setting for SCREEN_BRIGHTNESS.");
        }
        return currentBrightness;
    }
}
