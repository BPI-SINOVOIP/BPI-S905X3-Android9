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
 *
 */
package com.android.settings.testutils;

import android.content.Context;
import android.provider.Settings;

import com.android.settings.core.SliderPreferenceController;

public class FakeSliderController extends SliderPreferenceController {

    private final String settingKey = "fake_slider_key";

    public static final String AVAILABILITY_KEY = "fake_slider_availability_key";

    public static final int MAX_STEPS = 9;

    public FakeSliderController(Context context, String key) {
        super(context, key);
    }

    @Override
    public int getSliderPosition() {
        return Settings.System.getInt(mContext.getContentResolver(), settingKey, 0);
    }

    @Override
    public boolean setSliderPosition(int position) {
        return Settings.System.putInt(mContext.getContentResolver(), settingKey, position);
    }

    @Override
    public int getMaxSteps() {
        return MAX_STEPS;
    }

    @Override
    public int getAvailabilityStatus() {
        return Settings.Global.getInt(mContext.getContentResolver(),
                AVAILABILITY_KEY, AVAILABLE);
    }

    @Override
    public boolean isSliceable() {
        return true;
    }
}
