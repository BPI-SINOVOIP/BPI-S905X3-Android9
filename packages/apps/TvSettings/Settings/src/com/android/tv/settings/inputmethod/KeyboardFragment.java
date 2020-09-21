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

package com.android.tv.settings.inputmethod;

import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.UserHandle;
import android.support.annotation.Keep;
import android.support.annotation.VisibleForTesting;
import android.support.v7.preference.ListPreference;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceCategory;
import android.support.v7.preference.PreferenceScreen;
import android.text.TextUtils;
import android.util.ArraySet;
import android.view.inputmethod.InputMethodInfo;

import com.android.internal.logging.nano.MetricsProto;
import com.android.settingslib.applications.DefaultAppInfo;
import com.android.settingslib.wrapper.PackageManagerWrapper;
import com.android.tv.settings.R;
import com.android.tv.settings.SettingsPreferenceFragment;
import com.android.tv.settings.autofill.AutofillHelper;

import java.util.ArrayList;
import java.util.List;
import java.util.Set;

/**
 * Fragment for managing IMEs and Autofills
 */
@Keep
public class KeyboardFragment extends SettingsPreferenceFragment {
    private static final String TAG = "KeyboardFragment";

    // Order of input methods, make sure they are inserted between 1 (currentKeyboard) and
    // 3 (manageKeyboards).
    private static final int INPUT_METHOD_PREFERENCE_ORDER = 2;

    @VisibleForTesting
    static final String KEY_KEYBOARD_CATEGORY = "keyboardCategory";

    @VisibleForTesting
    static final String KEY_CURRENT_KEYBOARD = "currentKeyboard";

    private static final String KEY_KEYBOARD_SETTINGS_PREFIX = "keyboardSettings:";

    @VisibleForTesting
    static final String KEY_AUTOFILL_CATEGORY = "autofillCategory";

    @VisibleForTesting
    static final String KEY_CURRENT_AUTOFILL = "currentAutofill";

    private static final String KEY_AUTOFILL_SETTINGS_PREFIX = "autofillSettings:";

    private PackageManagerWrapper mPm;

    /**
     * @return New fragment instance
     */
    public static KeyboardFragment newInstance() {
        return new KeyboardFragment();
    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        mPm = new PackageManagerWrapper(context.getPackageManager());
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        setPreferencesFromResource(R.xml.keyboard, null);

        findPreference(KEY_CURRENT_KEYBOARD).setOnPreferenceChangeListener(
                (preference, newValue) -> {
                    InputMethodHelper.setDefaultInputMethodId(getContext(), (String) newValue);
                    return true;
                });

        updateUi();
    }

    @Override
    public void onResume() {
        super.onResume();
        updateUi();
    }

    @VisibleForTesting
    void updateUi() {
        updateAutofill();
        updateKeyboards();
    }

    private void updateKeyboards() {
        updateCurrentKeyboardPreference((ListPreference) findPreference(KEY_CURRENT_KEYBOARD));
        updateKeyboardsSettings();
    }

    private void updateCurrentKeyboardPreference(ListPreference currentKeyboardPref) {
        final PackageManager packageManager = getContext().getPackageManager();
        List<InputMethodInfo> enabledInputMethodInfos = InputMethodHelper
                .getEnabledSystemInputMethodList(getContext());
        final List<CharSequence> entries = new ArrayList<>(enabledInputMethodInfos.size());
        final List<CharSequence> values = new ArrayList<>(enabledInputMethodInfos.size());

        int defaultIndex = 0;
        final String defaultId = InputMethodHelper.getDefaultInputMethodId(getContext());

        for (final InputMethodInfo info : enabledInputMethodInfos) {
            entries.add(info.loadLabel(packageManager));
            final String id = info.getId();
            values.add(id);
            if (TextUtils.equals(id, defaultId)) {
                defaultIndex = values.size() - 1;
            }
        }

        currentKeyboardPref.setEntries(entries.toArray(new CharSequence[entries.size()]));
        currentKeyboardPref.setEntryValues(values.toArray(new CharSequence[values.size()]));
        if (entries.size() > 0) {
            currentKeyboardPref.setValueIndex(defaultIndex);
        }
    }

    Context getPreferenceContext() {
        return getPreferenceManager().getContext();
    }

    private void updateKeyboardsSettings() {
        final Context preferenceContext = getPreferenceContext();
        final PackageManager packageManager = getContext().getPackageManager();
        List<InputMethodInfo> enabledInputMethodInfos = InputMethodHelper
                .getEnabledSystemInputMethodList(getContext());

        PreferenceScreen preferenceScreen = getPreferenceScreen();
        final Set<String> enabledInputMethodKeys = new ArraySet<>(enabledInputMethodInfos.size());
        // Add per-IME settings
        for (final InputMethodInfo info : enabledInputMethodInfos) {
            final Intent settingsIntent = InputMethodHelper.getInputMethodSettingsIntent(info);
            if (settingsIntent == null) {
                continue;
            }
            final String key = KEY_KEYBOARD_SETTINGS_PREFIX + info.getId();

            Preference preference = preferenceScreen.findPreference(key);
            if (preference == null) {
                preference = new Preference(preferenceContext);
                preference.setOrder(INPUT_METHOD_PREFERENCE_ORDER);
                preferenceScreen.addPreference(preference);
            }
            preference.setTitle(getContext().getString(R.string.title_settings,
                    info.loadLabel(packageManager)));
            preference.setKey(key);
            preference.setIntent(settingsIntent);
            enabledInputMethodKeys.add(key);
        }

        for (int i = 0; i < preferenceScreen.getPreferenceCount();) {
            final Preference preference = preferenceScreen.getPreference(i);
            final String key = preference.getKey();
            if (!TextUtils.isEmpty(key)
                    && key.startsWith(KEY_KEYBOARD_SETTINGS_PREFIX)
                    && !enabledInputMethodKeys.contains(key)) {
                preferenceScreen.removePreference(preference);
            } else {
                i++;
            }
        }
    }

    /**
     * Update autofill related preferences.
     */
    private void updateAutofill() {
        final PreferenceCategory autofillCategory = (PreferenceCategory)
                findPreference(KEY_AUTOFILL_CATEGORY);
        List<DefaultAppInfo> candidates = getAutofillCandidates();
        if (candidates.isEmpty()) {
            // No need to show keyboard category and autofill category.
            // Keyboard only preference screen:
            findPreference(KEY_KEYBOARD_CATEGORY).setVisible(false);
            autofillCategory.setVisible(false);
            getPreferenceScreen().setTitle(R.string.system_keyboard);
        } else {
            // Show both keyboard category and autofill category in keyboard & autofill screen.
            findPreference(KEY_KEYBOARD_CATEGORY).setVisible(true);
            autofillCategory.setVisible(true);
            final Preference currentAutofillPref = findPreference(KEY_CURRENT_AUTOFILL);
            updateCurrentAutofillPreference(currentAutofillPref, candidates);
            updateAutofillSettings(candidates);
            getPreferenceScreen().setTitle(R.string.system_keyboard_autofill);
        }

    }

    private List<DefaultAppInfo> getAutofillCandidates() {
        return AutofillHelper.getAutofillCandidates(getContext(),
                mPm, UserHandle.myUserId());
    }

    private void updateCurrentAutofillPreference(Preference currentAutofillPref,
                                         List<DefaultAppInfo> candidates) {

        DefaultAppInfo app = AutofillHelper.getCurrentAutofill(getContext(), candidates);

        CharSequence summary = app == null ? getContext().getString(R.string.autofill_none)
                : app.loadLabel();
        currentAutofillPref.setSummary(summary);
    }

    private void updateAutofillSettings(List<DefaultAppInfo> candidates) {
        final Context preferenceContext = getPreferenceContext();

        final PreferenceCategory autofillCategory = (PreferenceCategory)
                findPreference(KEY_AUTOFILL_CATEGORY);

        final Set<String> autofillServicesKeys = new ArraySet<>(candidates.size());
        for (final DefaultAppInfo info : candidates) {
            final Intent settingsIntent = AutofillHelper.getAutofillSettingsIntent(getContext(),
                    mPm, info);
            if (settingsIntent == null) {
                continue;
            }
            final String key = KEY_AUTOFILL_SETTINGS_PREFIX + info.getKey();

            Preference preference = findPreference(key);
            if (preference == null) {
                preference = new Preference(preferenceContext);
                autofillCategory.addPreference(preference);
            }
            preference.setTitle(getContext().getString(R.string.title_settings, info.loadLabel()));
            preference.setKey(key);
            preference.setIntent(settingsIntent);
            autofillServicesKeys.add(key);
        }

        for (int i = 0; i < autofillCategory.getPreferenceCount();) {
            final Preference preference = autofillCategory.getPreference(i);
            final String key = preference.getKey();
            if (!TextUtils.isEmpty(key)
                    && key.startsWith(KEY_AUTOFILL_SETTINGS_PREFIX)
                    && !autofillServicesKeys.contains(key)) {
                autofillCategory.removePreference(preference);
            } else {
                i++;
            }
        }
    }

    @Override
    public int getMetricsCategory() {
        return MetricsProto.MetricsEvent.INPUTMETHOD_KEYBOARD;
    }
}
