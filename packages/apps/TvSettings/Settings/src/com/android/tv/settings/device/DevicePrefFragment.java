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

package com.android.tv.settings.device;

import android.content.Context;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.content.res.Resources;
import android.media.tv.TvInputInfo;
import android.media.tv.TvInputManager;
import android.os.Bundle;
import android.os.UserHandle;
import android.support.annotation.Keep;
import android.support.annotation.VisibleForTesting;
import android.support.v7.preference.Preference;
import android.text.TextUtils;
import android.util.Log;
import android.view.inputmethod.InputMethodInfo;

import com.android.internal.logging.nano.MetricsProto.MetricsEvent;
import com.android.settingslib.applications.DefaultAppInfo;
import com.android.settingslib.development.DevelopmentSettingsEnabler;
import com.android.settingslib.wrapper.PackageManagerWrapper;
import com.android.tv.settings.MainFragment;
import com.android.tv.settings.R;
import com.android.tv.settings.SettingsPreferenceFragment;
import com.android.tv.settings.autofill.AutofillHelper;
import com.android.tv.settings.device.sound.SoundFragment;
import com.android.tv.settings.inputmethod.InputMethodHelper;
import com.android.tv.settings.system.SecurityFragment;

import java.util.List;

/**
 * The "Device Preferences" screen in TV settings.
 */
@Keep
public class DevicePrefFragment extends SettingsPreferenceFragment {
    private static final String TAG = "DeviceFragment";

    @VisibleForTesting
    static final String KEY_DEVELOPER = "developer";
    private static final String KEY_USAGE = "usageAndDiag";
    private static final String KEY_INPUTS = "inputs";
    private static final String KEY_SOUNDS = "sound_effects";
    @VisibleForTesting
    static final String KEY_CAST_SETTINGS = "cast";
    private static final String KEY_GOOGLE_SETTINGS = "google_settings";
    private static final String KEY_HOME_SETTINGS = "home";
    @VisibleForTesting
    static final String KEY_KEYBOARD = "keyboard";

    private Preference mSoundsPref;
    private boolean mInputSettingNeeded;
    private PackageManagerWrapper mPm;

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        if (isRestricted()) {
            setPreferencesFromResource(R.xml.device_restricted, null);
        } else {
            setPreferencesFromResource(R.xml.device, null);
        }
        mSoundsPref = findPreference(KEY_SOUNDS);
        final Preference inputPref = findPreference(KEY_INPUTS);
        if (inputPref != null) {
            inputPref.setVisible(mInputSettingNeeded);
        }
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        final TvInputManager manager = (TvInputManager) getContext().getSystemService(
                Context.TV_INPUT_SERVICE);
        if (manager != null) {
            for (final TvInputInfo input : manager.getTvInputList()) {
                if (input.isPassthroughInput()) {
                    mInputSettingNeeded = true;
                }
            }
        }
        super.onCreate(savedInstanceState);
    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        mPm = new PackageManagerWrapper(context.getPackageManager());
    }

    @Override
    public void onResume() {
        super.onResume();

        updateDeveloperOptions();
        updateSounds();
        updateGoogleSettings();
        updateCastSettings();
        updateKeyboardAutofillSettings();
        hideIfIntentUnhandled(findPreference(KEY_HOME_SETTINGS));
        hideIfIntentUnhandled(findPreference(KEY_CAST_SETTINGS));
        hideIfIntentUnhandled(findPreference(KEY_USAGE));
    }

    @Override
    public int getMetricsCategory() {
        return MetricsEvent.SETTINGS_TV_DEVICE_CATEGORY;
    }

    private void hideIfIntentUnhandled(Preference preference) {
        if (preference == null || !preference.isVisible()) {
            return;
        }
        preference.setVisible(
                MainFragment.systemIntentIsHandled(getContext(), preference.getIntent()) != null);
    }

    private boolean isRestricted() {
        return SecurityFragment.isRestrictedProfileInEffect(getContext());
    }

    @VisibleForTesting
    void updateDeveloperOptions() {
        final Preference developerPref = findPreference(KEY_DEVELOPER);
        if (developerPref == null) {
            return;
        }

        developerPref.setVisible(DevelopmentSettingsEnabler.isDevelopmentSettingsEnabled(
                getContext()));
    }

    private void updateSounds() {
        if (mSoundsPref == null) {
            return;
        }

        mSoundsPref.setIcon(SoundFragment.getSoundEffectsEnabled(getContext().getContentResolver())
                ? R.drawable.ic_volume_up : R.drawable.ic_volume_off);
    }

    private void updateGoogleSettings() {
        final Preference googleSettingsPref = findPreference(KEY_GOOGLE_SETTINGS);
        if (googleSettingsPref != null) {
            final ResolveInfo info = MainFragment.systemIntentIsHandled(getContext(),
                    googleSettingsPref.getIntent());
            googleSettingsPref.setVisible(info != null);
            if (info != null && info.activityInfo != null) {
                googleSettingsPref.setIcon(
                        info.activityInfo.loadIcon(getContext().getPackageManager()));
                googleSettingsPref.setTitle(
                        info.activityInfo.loadLabel(getContext().getPackageManager()));
            }
        }
    }

    @VisibleForTesting
    void updateCastSettings() {
        final Preference castPref = findPreference(KEY_CAST_SETTINGS);
        if (castPref != null) {
            final ResolveInfo info = MainFragment.systemIntentIsHandled(
                        getContext(), castPref.getIntent());
            if (info != null) {
                try {
                    final Context targetContext = getContext()
                            .createPackageContext(info.resolvePackageName != null
                                    ? info.resolvePackageName : info.activityInfo.packageName, 0);
                    castPref.setIcon(targetContext.getDrawable(info.getIconResource()));
                } catch (Resources.NotFoundException | PackageManager.NameNotFoundException
                        | SecurityException e) {
                    Log.e(TAG, "Cast settings icon not found", e);
                }
                castPref.setTitle(info.activityInfo.loadLabel(getContext().getPackageManager()));
            }
        }
    }

    @VisibleForTesting
    void updateKeyboardAutofillSettings() {
        final Preference keyboardPref = findPreference(KEY_KEYBOARD);

        List<DefaultAppInfo> candidates = AutofillHelper.getAutofillCandidates(getContext(),
                mPm, UserHandle.myUserId());

        // Switch title depends on whether there is autofill
        if (candidates.isEmpty()) {
            keyboardPref.setTitle(R.string.system_keyboard);
        } else {
            keyboardPref.setTitle(R.string.system_keyboard_autofill);
        }

        CharSequence summary = "";
        // append current keyboard to summary
        String defaultImId = InputMethodHelper.getDefaultInputMethodId(getContext());
        if (!TextUtils.isEmpty(defaultImId)) {
            InputMethodInfo info = InputMethodHelper.findInputMethod(defaultImId,
                    InputMethodHelper.getEnabledSystemInputMethodList(getContext()));
            if (info != null) {
                summary = info.loadLabel(getContext().getPackageManager());
            }

        }
        // append current autofill to summary
        DefaultAppInfo appInfo = AutofillHelper.getCurrentAutofill(getContext(), candidates);
        if (appInfo != null) {
            CharSequence autofillInfo = appInfo.loadLabel();
            if (summary.length() > 0) {
                getContext().getString(R.string.string_concat, summary, autofillInfo);
            } else {
                summary = autofillInfo;
            }
        }
        keyboardPref.setSummary(summary);
    }
}
