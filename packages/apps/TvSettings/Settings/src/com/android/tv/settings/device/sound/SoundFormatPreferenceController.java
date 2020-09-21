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

package com.android.tv.settings.device.sound;

import android.content.Context;
import android.media.AudioManager;
import android.provider.Settings;
import android.support.v14.preference.SwitchPreference;
import android.support.v7.preference.Preference;
import android.text.TextUtils;
import android.util.Log;

import com.android.settingslib.core.AbstractPreferenceController;

import java.util.Arrays;
import java.util.HashSet;
import java.util.Map;

/**
 * Controller for the surround sound switch preferences.
 */
public class SoundFormatPreferenceController extends AbstractPreferenceController {

    private static final String TAG = "SoundFormatController";

    private int mFormatId;
    private AudioManager mAudioManager;
    private Map<Integer, Boolean> mReportedFormats;

    public SoundFormatPreferenceController(Context context, int formatId) {
        super(context);
        mFormatId = formatId;
        mAudioManager = (AudioManager) context.getSystemService(Context.AUDIO_SERVICE);
        mReportedFormats = mAudioManager.getReportedSurroundFormats();
    }

    @Override
    public boolean isAvailable() {
        return true;
    }

    @Override
    public String getPreferenceKey() {
        return SoundFragment.KEY_SURROUND_SOUND_FORMAT_PREFIX + mFormatId;
    }

    @Override
    public void updateState(Preference preference) {
        super.updateState(preference);
        if (preference.getKey().equals(getPreferenceKey())) {
            preference.setEnabled(getFormatPreferencesEnabledState());
            ((SwitchPreference) preference).setChecked(getFormatPreferenceCheckedState());
        }
    }

    @Override
    public boolean handlePreferenceTreeClick(Preference preference) {
        if (preference.getKey().equals(getPreferenceKey())) {
            setSurroundManualFormatsSetting(((SwitchPreference) preference).isChecked());
        }
        return super.handlePreferenceTreeClick(preference);
    }

    /**
     * @return checked state of a surround sound format switch based on passthrough setting and
     * audio manager state for the format.
     */
    private boolean getFormatPreferenceCheckedState() {
        switch (SoundFragment.getSurroundPassthroughSetting(mContext)) {
            case SoundFragment.VAL_SURROUND_SOUND_NEVER:
                return false;
            case SoundFragment.VAL_SURROUND_SOUND_ALWAYS:
                return true;
            case SoundFragment.VAL_SURROUND_SOUND_AUTO:
                return isReportedFormat();
            case SoundFragment.VAL_SURROUND_SOUND_MANUAL:
                return getFormatsEnabledInManualMode().contains(mFormatId);
            default: return false;
        }
    }

    /** @return true if the format checkboxes should be enabled, i.e. in manual mode. */
    private boolean getFormatPreferencesEnabledState() {
        return SoundFragment.getSurroundPassthroughSetting(mContext)
                == SoundFragment.VAL_SURROUND_SOUND_MANUAL;
    }

    /** @return the formats that are enabled in manual mode, from global settings */
    private HashSet<Integer> getFormatsEnabledInManualMode() {
        HashSet<Integer> formats = new HashSet<>();
        String enabledFormats = Settings.Global.getString(mContext.getContentResolver(),
                Settings.Global.ENCODED_SURROUND_OUTPUT_ENABLED_FORMATS);
        if (enabledFormats == null) {
            // Starting with Android P passthrough setting ALWAYS has been replaced with MANUAL.
            // In that case all formats will be enabled when in MANUAL mode.
            formats.addAll(mAudioManager.getSurroundFormats().keySet());
        } else {
            try {
                Arrays.stream(enabledFormats.split(",")).mapToInt(Integer::parseInt)
                        .forEach(formats::add);
            } catch (NumberFormatException e) {
                Log.w(TAG, "ENCODED_SURROUND_OUTPUT_ENABLED_FORMATS misformatted.", e);
            }
        }
        return formats;
    }

    /**
     * Writes enabled/disabled state for a given format to the global settings.
     */
    private void setSurroundManualFormatsSetting(boolean enabled) {
        HashSet<Integer> formats = getFormatsEnabledInManualMode();
        if (enabled) {
            formats.add(mFormatId);
        } else {
            formats.remove(mFormatId);
        }
        Settings.Global.putString(mContext.getContentResolver(),
                Settings.Global.ENCODED_SURROUND_OUTPUT_ENABLED_FORMATS,
                TextUtils.join(",", formats));
    }

    /** @return true if the given format is reported by the device. */
    private boolean isReportedFormat() {
        return mReportedFormats != null && mReportedFormats.get(mFormatId) != null;
    }
}
