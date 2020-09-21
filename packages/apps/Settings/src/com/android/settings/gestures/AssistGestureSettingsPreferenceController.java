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
 * limitations under the License.
 */

package com.android.settings.gestures;

import static android.provider.Settings.Secure.ASSIST_GESTURE_ENABLED;
import static android.provider.Settings.Secure.ASSIST_GESTURE_SILENCE_ALERTS_ENABLED;

import android.content.Context;
import android.content.Intent;
import android.provider.Settings;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceScreen;

import com.android.internal.annotations.VisibleForTesting;
import com.android.settings.R;
import com.android.settings.overlay.FeatureFactory;
import com.android.settings.search.DatabaseIndexingUtils;
import com.android.settings.search.InlineSwitchPayload;
import com.android.settings.search.ResultPayload;

public class AssistGestureSettingsPreferenceController extends GesturePreferenceController {

    private static final String PREF_KEY_VIDEO = "gesture_assist_video";

    private static final String SECURE_KEY_ASSIST = ASSIST_GESTURE_ENABLED;
    private static final String SECURE_KEY_SILENCE = ASSIST_GESTURE_SILENCE_ALERTS_ENABLED;
    private static final int ON = 1;
    private static final int OFF = 0;

    private final String mAssistGesturePrefKey;
    private final AssistGestureFeatureProvider mFeatureProvider;
    private boolean mWasAvailable;

    private PreferenceScreen mScreen;
    private Preference mPreference;

    @VisibleForTesting
    boolean mAssistOnly;

    public AssistGestureSettingsPreferenceController(Context context,
            String key) {
        super(context, key);
        mFeatureProvider = FeatureFactory.getFactory(context).getAssistGestureFeatureProvider();
        mWasAvailable = isAvailable();
        mAssistGesturePrefKey = key;
    }

    @Override
    public int getAvailabilityStatus() {
        final boolean isAvailable = mAssistOnly ? mFeatureProvider.isSupported(mContext)
                : mFeatureProvider.isSensorAvailable(mContext);
        return isAvailable ? AVAILABLE : UNSUPPORTED_ON_DEVICE;
    }

    @Override
    public void displayPreference(PreferenceScreen screen) {
        mScreen = screen;
        mPreference = screen.findPreference(getPreferenceKey());
        super.displayPreference(screen);
    }

    @Override
    public void onResume() {
        if (mWasAvailable != isAvailable()) {
            // Only update the preference visibility if the availability has changed -- otherwise
            // the preference may be incorrectly added to screens with collapsed sections.
            updatePreference();
            mWasAvailable = isAvailable();
        }
    }

    public AssistGestureSettingsPreferenceController setAssistOnly(boolean assistOnly) {
        mAssistOnly = assistOnly;
        return this;
    }

    private void updatePreference() {
        if (mPreference == null) {
            return;
        }

        if (isAvailable()) {
            if (mScreen.findPreference(getPreferenceKey()) == null) {
                mScreen.addPreference(mPreference);
            }
        } else {
            mScreen.removePreference(mPreference);
        }
    }

    private boolean isAssistGestureEnabled() {
        return Settings.Secure.getInt(mContext.getContentResolver(),
                SECURE_KEY_ASSIST, ON) != 0;
    }

    private boolean isSilenceGestureEnabled() {
        return Settings.Secure.getInt(mContext.getContentResolver(),
                SECURE_KEY_SILENCE, ON) != 0;
    }

    @Override
    public boolean setChecked(boolean isChecked) {
        return Settings.Secure.putInt(mContext.getContentResolver(), SECURE_KEY_ASSIST,
                isChecked ? ON : OFF);
    }

    @Override
    protected String getVideoPrefKey() {
        return PREF_KEY_VIDEO;
    }

    @Override
    public CharSequence getSummary() {
        boolean isEnabled = isAssistGestureEnabled() && mFeatureProvider.isSupported(mContext);
        if (!mAssistOnly) {
            isEnabled = isEnabled || isSilenceGestureEnabled();
        }
        return mContext.getText(
                isEnabled ? R.string.gesture_setting_on : R.string.gesture_setting_off);
    }

    @Override
    public boolean isChecked() {
        return Settings.Secure.getInt(mContext.getContentResolver(), SECURE_KEY_ASSIST, OFF) == ON;
    }

    @Override
    //TODO (b/69808376): Remove result payload
    public ResultPayload getResultPayload() {
        final Intent intent = DatabaseIndexingUtils.buildSearchResultPageIntent(mContext,
                AssistGestureSettings.class.getName(), mAssistGesturePrefKey,
                mContext.getString(R.string.display_settings));

        return new InlineSwitchPayload(SECURE_KEY_ASSIST, ResultPayload.SettingsSource.SECURE,
                ON /* onValue */, intent, isAvailable(), ON /* defaultValue */);
    }
}
