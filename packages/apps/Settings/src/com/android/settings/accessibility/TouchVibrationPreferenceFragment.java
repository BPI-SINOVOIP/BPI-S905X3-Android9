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
package com.android.settings.accessibility;

import android.graphics.drawable.Drawable;
import android.media.AudioAttributes;
import android.os.Vibrator;
import android.os.VibrationEffect;
import android.provider.Settings;

import com.android.internal.logging.nano.MetricsProto.MetricsEvent;
import com.android.settings.R;

/**
 * Fragment for picking accessibility shortcut service
 */
public class TouchVibrationPreferenceFragment extends VibrationPreferenceFragment {
    @Override
    public int getMetricsCategory() {
        return MetricsEvent.ACCESSIBILITY_VIBRATION_TOUCH;
    }

    @Override
    protected int getPreferenceScreenResId() {
        return R.xml.accessibility_touch_vibration_settings;
    }

    /**
     * Get the setting string of the vibration intensity setting this preference is dealing with.
     */
    @Override
    protected String getVibrationIntensitySetting() {
        return Settings.System.HAPTIC_FEEDBACK_INTENSITY;
    }

    @Override
    protected int getDefaultVibrationIntensity() {
        Vibrator vibrator = getContext().getSystemService(Vibrator.class);
        return vibrator.getDefaultHapticFeedbackIntensity();
    }

    @Override
    protected int getPreviewVibrationAudioAttributesUsage() {
        return AudioAttributes.USAGE_ASSISTANCE_SONIFICATION;
    }

    @Override
    public void onVibrationIntensitySelected(int intensity) {
        // We want to keep HAPTIC_FEEDBACK_ENABLED consistent with this setting since some
        // applications check it directly before triggering their own haptic feedback.
        final boolean hapticFeedbackEnabled = !(intensity == Vibrator.VIBRATION_INTENSITY_OFF);
        Settings.System.putInt(getContext().getContentResolver(),
                Settings.System.HAPTIC_FEEDBACK_ENABLED, hapticFeedbackEnabled ? 1 : 0);
    }
}
