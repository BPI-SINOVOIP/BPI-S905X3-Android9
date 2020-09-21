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

import static android.os.Vibrator.VibrationIntensity;

import android.support.annotation.VisibleForTesting;
import android.content.Context;
import android.database.ContentObserver;
import android.graphics.drawable.Drawable;
import android.media.AudioAttributes;
import android.net.Uri;
import android.os.Handler;
import android.os.VibrationEffect;
import android.os.Vibrator;
import android.provider.Settings;
import android.util.ArrayMap;
import android.util.Log;

import com.android.internal.accessibility.AccessibilityShortcutController;
import com.android.internal.logging.nano.MetricsProto.MetricsEvent;
import com.android.settings.R;
import com.android.settings.widget.RadioButtonPickerFragment;
import com.android.settingslib.widget.CandidateInfo;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.Map;

/**
 * Fragment for changing vibration settings.
 */
public abstract class VibrationPreferenceFragment extends RadioButtonPickerFragment {
    private static final String TAG = "VibrationPreferenceFragment";

    @VisibleForTesting
    final static String KEY_INTENSITY_OFF = "intensity_off";
    @VisibleForTesting
    final static String KEY_INTENSITY_LOW = "intensity_low";
    @VisibleForTesting
    final static String KEY_INTENSITY_MEDIUM = "intensity_medium";
    @VisibleForTesting
    final static String KEY_INTENSITY_HIGH = "intensity_high";
    // KEY_INTENSITY_ON is only used when the device doesn't support multiple intensity levels.
    @VisibleForTesting
    final static String KEY_INTENSITY_ON = "intensity_on";

    private final Map<String, VibrationIntensityCandidateInfo> mCandidates;
    private final SettingsObserver mSettingsObserver;

    public VibrationPreferenceFragment() {
        mCandidates = new ArrayMap<>();
        mSettingsObserver = new SettingsObserver();
    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        mSettingsObserver.register();
        if (mCandidates.isEmpty()) {
            loadCandidates(context);
        }
    }

    private void loadCandidates(Context context) {
        final boolean supportsMultipleIntensities = context.getResources().getBoolean(
                R.bool.config_vibration_supports_multiple_intensities);
        if (supportsMultipleIntensities) {
            mCandidates.put(KEY_INTENSITY_OFF,
                    new VibrationIntensityCandidateInfo(KEY_INTENSITY_OFF,
                        R.string.accessibility_vibration_intensity_off,
                        Vibrator.VIBRATION_INTENSITY_OFF));
            mCandidates.put(KEY_INTENSITY_LOW,
                    new VibrationIntensityCandidateInfo(KEY_INTENSITY_LOW,
                        R.string.accessibility_vibration_intensity_low,
                        Vibrator.VIBRATION_INTENSITY_LOW));
            mCandidates.put(KEY_INTENSITY_MEDIUM,
                    new VibrationIntensityCandidateInfo(KEY_INTENSITY_MEDIUM,
                        R.string.accessibility_vibration_intensity_medium,
                        Vibrator.VIBRATION_INTENSITY_MEDIUM));
            mCandidates.put(KEY_INTENSITY_HIGH,
                    new VibrationIntensityCandidateInfo(KEY_INTENSITY_HIGH,
                        R.string.accessibility_vibration_intensity_high,
                        Vibrator.VIBRATION_INTENSITY_HIGH));
        } else {
            mCandidates.put(KEY_INTENSITY_OFF,
                    new VibrationIntensityCandidateInfo(KEY_INTENSITY_OFF,
                        R.string.switch_off_text, Vibrator.VIBRATION_INTENSITY_OFF));
            mCandidates.put(KEY_INTENSITY_ON,
                    new VibrationIntensityCandidateInfo(KEY_INTENSITY_ON,
                        R.string.switch_on_text, getDefaultVibrationIntensity()));
        }
    }

    @Override
    public void onDetach() {
        super.onDetach();
        mSettingsObserver.unregister();
    }

    /**
     * Get the setting string of the vibration intensity setting this preference is dealing with.
     */
    protected abstract String getVibrationIntensitySetting();

    /**
     * Get the default intensity for the desired setting.
     */
    protected abstract int getDefaultVibrationIntensity();

    /**
     * When a new vibration intensity is selected by the user.
     */
    protected void onVibrationIntensitySelected(int intensity) { }

    /**
     * Play a vibration effect with intensity just selected by user
     */
    protected void playVibrationPreview() {
        Vibrator vibrator = getContext().getSystemService(Vibrator.class);
        VibrationEffect effect = VibrationEffect.get(VibrationEffect.EFFECT_CLICK);
        AudioAttributes.Builder builder = new AudioAttributes.Builder();
        builder.setUsage(getPreviewVibrationAudioAttributesUsage());
        vibrator.vibrate(effect, builder.build());
    }

    /**
     * Get the AudioAttributes usage for vibration preview.
     */
    protected int getPreviewVibrationAudioAttributesUsage() {
        return AudioAttributes.USAGE_UNKNOWN;
    }

    @Override
    protected List<? extends CandidateInfo> getCandidates() {
        List<VibrationIntensityCandidateInfo> candidates = new ArrayList<>(mCandidates.values());
        candidates.sort(
                Comparator.comparing(VibrationIntensityCandidateInfo::getIntensity).reversed());
        return candidates;
    }

    @Override
    protected String getDefaultKey() {
        final int vibrationIntensity = Settings.System.getInt(getContext().getContentResolver(),
                getVibrationIntensitySetting(), getDefaultVibrationIntensity());
        for (VibrationIntensityCandidateInfo candidate : mCandidates.values()) {
            final boolean matchesIntensity = candidate.getIntensity() == vibrationIntensity;
            final boolean matchesOn = candidate.getKey().equals(KEY_INTENSITY_ON)
                    && vibrationIntensity != Vibrator.VIBRATION_INTENSITY_OFF;
            if (matchesIntensity || matchesOn) {
                return candidate.getKey();
            }
        }
        return null;
    }

    @Override
    protected boolean setDefaultKey(String key) {
        VibrationIntensityCandidateInfo candidate = mCandidates.get(key);
        if (candidate == null) {
            Log.e(TAG, "Tried to set unknown intensity (key=" + key + ")!");
            return false;
        }
        Settings.System.putInt(getContext().getContentResolver(),
                getVibrationIntensitySetting(), candidate.getIntensity());
        onVibrationIntensitySelected(candidate.getIntensity());
        return true;
    }

    @VisibleForTesting
    class VibrationIntensityCandidateInfo extends CandidateInfo {
        private String mKey;
        private int mLabelId;
        @VibrationIntensity
        private int mIntensity;

        public VibrationIntensityCandidateInfo(String key, int labelId, int intensity) {
            super(true /* enabled */);
            mKey = key;
            mLabelId = labelId;
            mIntensity = intensity;
        }

        @Override
        public CharSequence loadLabel() {
            return getContext().getString(mLabelId);
        }

        @Override
        public Drawable loadIcon() {
            return null;
        }

        @Override
        public String getKey() {
            return mKey;
        }

        public int getIntensity() {
            return mIntensity;
        }
    }

    private class SettingsObserver extends ContentObserver {
        public SettingsObserver() {
            super(new Handler());
        }

        public void register() {
            getContext().getContentResolver().registerContentObserver(
                    Settings.System.getUriFor(getVibrationIntensitySetting()), false, this);
        }

        public void unregister() {
            getContext().getContentResolver().unregisterContentObserver(this);
        }

        @Override
        public void onChange(boolean selfChange, Uri uri) {
            updateCandidates();
            playVibrationPreview();
        }
    }
}
