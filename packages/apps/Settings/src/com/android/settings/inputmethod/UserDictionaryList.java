/*
 * Copyright (C) 2011 The Android Open Source Project
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

package com.android.settings.inputmethod;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.os.Bundle;
import android.provider.UserDictionary;
import android.support.annotation.NonNull;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceGroup;
import android.text.TextUtils;
import android.view.inputmethod.InputMethodInfo;
import android.view.inputmethod.InputMethodManager;
import android.view.inputmethod.InputMethodSubtype;

import com.android.internal.logging.nano.MetricsProto.MetricsEvent;
import com.android.settings.R;
import com.android.settings.SettingsPreferenceFragment;
import com.android.settings.Utils;

import java.util.List;
import java.util.Locale;
import java.util.TreeSet;

public class UserDictionaryList extends SettingsPreferenceFragment {
    public static final String USER_DICTIONARY_SETTINGS_INTENT_ACTION =
            "android.settings.USER_DICTIONARY_SETTINGS";
    private String mLocale;

    @Override
    public int getMetricsCategory() {
        return MetricsEvent.INPUTMETHOD_USER_DICTIONARY;
    }

    @Override
    public void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        setPreferenceScreen(getPreferenceManager().createPreferenceScreen(getActivity()));
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
        getActivity().getActionBar().setTitle(R.string.user_dict_settings_title);

        final Intent intent = getActivity().getIntent();
        final String localeFromIntent =
                null == intent ? null : intent.getStringExtra("locale");

        final Bundle arguments = getArguments();
        final String localeFromArguments =
                null == arguments ? null : arguments.getString("locale");

        final String locale;
        if (null != localeFromArguments) {
            locale = localeFromArguments;
        } else if (null != localeFromIntent) {
            locale = localeFromIntent;
        } else {
            locale = null;
        }
        mLocale = locale;
    }

    @NonNull
    public static TreeSet<String> getUserDictionaryLocalesSet(Context context) {
        final Cursor cursor = context.getContentResolver().query(
                UserDictionary.Words.CONTENT_URI, new String[]{UserDictionary.Words.LOCALE},
                null, null, null);
        final TreeSet<String> localeSet = new TreeSet<>();
        if (cursor == null) {
            // The user dictionary service is not present or disabled. Return empty set.
            return localeSet;
        }
        try {
            if (cursor.moveToFirst()) {
                final int columnIndex = cursor.getColumnIndex(UserDictionary.Words.LOCALE);
                do {
                    final String locale = cursor.getString(columnIndex);
                    localeSet.add(null != locale ? locale : "");
                } while (cursor.moveToNext());
            }
        } finally {
            cursor.close();
        }

        // CAVEAT: Keep this for consistency of the implementation between Keyboard and Settings
        // if (!UserDictionarySettings.IS_SHORTCUT_API_SUPPORTED) {
        //     // For ICS, we need to show "For all languages" in case that the keyboard locale
        //     // is different from the system locale
        //     localeSet.add("");
        // }

        final InputMethodManager imm =
                (InputMethodManager) context.getSystemService(Context.INPUT_METHOD_SERVICE);
        final List<InputMethodInfo> imis = imm.getEnabledInputMethodList();
        for (final InputMethodInfo imi : imis) {
            final List<InputMethodSubtype> subtypes =
                    imm.getEnabledInputMethodSubtypeList(
                            imi, true /* allowsImplicitlySelectedSubtypes */);
            for (InputMethodSubtype subtype : subtypes) {
                final String locale = subtype.getLocale();
                if (!TextUtils.isEmpty(locale)) {
                    localeSet.add(locale);
                }
            }
        }

        // We come here after we have collected locales from existing user dictionary entries and
        // enabled subtypes. If we already have the locale-without-country version of the system
        // locale, we don't add the system locale to avoid confusion even though it's technically
        // correct to add it.
        if (!localeSet.contains(Locale.getDefault().getLanguage().toString())) {
            localeSet.add(Locale.getDefault().toString());
        }

        return localeSet;
    }

    /**
     * Creates the entries that allow the user to go into the user dictionary for each locale.
     *
     * @param userDictGroup The group to put the settings in.
     */
    protected void createUserDictSettings(PreferenceGroup userDictGroup) {
        final Activity activity = getActivity();
        userDictGroup.removeAll();
        final TreeSet<String> localeSet =
                UserDictionaryList.getUserDictionaryLocalesSet(activity);
        if (mLocale != null) {
            // If the caller explicitly specify empty string as a locale, we'll show "all languages"
            // in the list.
            localeSet.add(mLocale);
        }
        if (localeSet.size() > 1) {
            // Have an "All languages" entry in the languages list if there are two or more active
            // languages
            localeSet.add("");
        }

        if (localeSet.isEmpty()) {
            userDictGroup.addPreference(createUserDictionaryPreference(null, activity));
        } else {
            for (String locale : localeSet) {
                userDictGroup.addPreference(createUserDictionaryPreference(locale, activity));
            }
        }
    }

    /**
     * Create a single User Dictionary Preference object, with its parameters set.
     *
     * @param locale The locale for which this user dictionary is for.
     * @return The corresponding preference.
     */
    protected Preference createUserDictionaryPreference(String locale, Activity activity) {
        final Preference newPref = new Preference(getPrefContext());
        final Intent intent = new Intent(USER_DICTIONARY_SETTINGS_INTENT_ACTION);
        if (null == locale) {
            newPref.setTitle(Locale.getDefault().getDisplayName());
        } else {
            if ("".equals(locale)) {
                newPref.setTitle(getString(R.string.user_dict_settings_all_languages));
            } else {
                newPref.setTitle(Utils.createLocaleFromString(locale).getDisplayName());
            }
            intent.putExtra("locale", locale);
            newPref.getExtras().putString("locale", locale);
        }
        newPref.setIntent(intent);
        newPref.setFragment(UserDictionarySettings.class.getName());
        return newPref;
    }

    @Override
    public void onResume() {
        super.onResume();
        createUserDictSettings(getPreferenceScreen());
    }
}
