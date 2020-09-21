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
package com.android.car.settings.quicksettings;

import static android.provider.Settings.System.SCREEN_BRIGHTNESS;

import android.content.Context;
import android.provider.Settings;
import android.widget.SeekBar;

import com.android.car.settings.common.Logger;

/**
 * A slider to adjust the brightness of the screen
 */
public class BrightnessTile implements QuickSettingGridAdapter.SeekbarTile {
    private static final Logger LOG = new Logger(BrightnessTile.class);
    private static final int MAX_BRIGHTNESS = 255;
    private final Context mContext;

    public BrightnessTile(Context context) {
        mContext = context;
    }

    @Override
    public void onStartTrackingTouch(SeekBar seekBar) {
        // don't care
    }

    @Override
    public void onStopTrackingTouch(SeekBar seekBar) {
        // don't care
    }

    @Override
    public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
        Settings.System.putInt(mContext.getContentResolver(), SCREEN_BRIGHTNESS, progress);
    }

    @Override
    public int getMax() {
        return MAX_BRIGHTNESS;
    }

    @Override
    public void stop() {
        // nothing to do.
    }

    @Override
    public int getCurrent() {
        int currentBrightness = 0;
        try {
            currentBrightness = Settings.System.getInt(mContext.getContentResolver(),
                    SCREEN_BRIGHTNESS);
        } catch (Settings.SettingNotFoundException e) {
            LOG.w("Can't find setting for SCREEN_BRIGHTNESS.");
        }
        return currentBrightness;
    }
}
