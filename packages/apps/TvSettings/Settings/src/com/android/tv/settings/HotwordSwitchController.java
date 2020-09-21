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

package com.android.tv.settings;

import android.annotation.Nullable;
import android.content.Context;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.database.ContentObserver;
import android.database.Cursor;
import android.net.Uri;
import android.os.AsyncTask;
import android.support.v14.preference.SwitchPreference;
import android.support.v7.preference.Preference;
import android.text.TextUtils;
import android.util.Log;

import com.android.settingslib.core.AbstractPreferenceController;

/**
 * Controller for the hotword switch preference.
 */
public class HotwordSwitchController extends AbstractPreferenceController {

    private static final String TAG = "HotwordController";
    private static final Uri URI = Uri.parse("content://com.google.android.katniss.search."
            + "searchapi.VoiceInteractionProvider/sharedvalue");
    static final String ASSISTANT_PGK_NAME = "com.google.android.katniss";
    static final String ACTION_HOTWORD_ENABLE =
            "com.google.android.assistant.HOTWORD_ENABLE";
    static final String ACTION_HOTWORD_DISABLE =
            "com.google.android.assistant.HOTWORD_DISABLE";

    static final String KEY_HOTWORD_SWITCH = "hotword_switch";

    /** Listen to hotword state events. */
    public interface HotwordStateListener {
        /** hotword state has changed */
        void onHotwordStateChanged();
        /** request to enable hotwording */
        void onHotwordEnable();
        /** request to disable hotwording */
        void onHotwordDisable();
    }

    private ContentObserver mHotwordSwitchObserver = new ContentObserver(null) {
        @Override
        public void onChange(boolean selfChange) {
            onChange(selfChange, null);
        }

        @Override
        public void onChange(boolean selfChange, Uri uri) {
            new HotwordLoader().execute();
        }
    };

    private static class HotwordState {
        private boolean mHotwordEnabled;
        private boolean mHotwordSwitchVisible;
        private boolean mHotwordSwitchDisabled;
        private String mHotwordSwitchTitle;
        private String mHotwordSwitchDescription;
    }

    /**
     * Task to retrieve state of the hotword switch from a content provider.
     */
    private class HotwordLoader extends AsyncTask<Void, Void, HotwordState> {

        @Override
        protected HotwordState doInBackground(Void... voids) {
            HotwordState hotwordState = new HotwordState();
            Context context = mContext.getApplicationContext();
            try (Cursor cursor = context.getContentResolver().query(URI, null, null, null,
                    null, null)) {
                if (cursor != null) {
                    int idxKey = cursor.getColumnIndex("key");
                    int idxValue = cursor.getColumnIndex("value");
                    if (idxKey < 0 || idxValue < 0) {
                        return null;
                    }
                    while (cursor.moveToNext()) {
                        String key = cursor.getString(idxKey);
                        String value = cursor.getString(idxValue);
                        if (key == null || value == null) {
                            continue;
                        }
                        try {
                            switch (key) {
                                case "is_listening_for_hotword":
                                    hotwordState.mHotwordEnabled = Integer.valueOf(value) == 1;
                                    break;
                                case "is_hotword_switch_visible":
                                    hotwordState.mHotwordSwitchVisible =
                                            Integer.valueOf(value) == 1;
                                    break;
                                case "is_hotword_switch_disabled":
                                    hotwordState.mHotwordSwitchDisabled =
                                            Integer.valueOf(value) == 1;
                                    break;
                                case "hotword_switch_title":
                                    hotwordState.mHotwordSwitchTitle = getLocalizedStringResource(
                                            value, mContext.getString(R.string.hotwording_title));
                                    break;
                                case "hotword_switch_description":
                                    hotwordState.mHotwordSwitchDescription =
                                            getLocalizedStringResource(value, null);
                                    break;
                                default:
                            }
                        } catch (NumberFormatException e) {
                            Log.w(TAG, "Invalid value.", e);
                        }
                    }
                    return hotwordState;
                }
            } catch (Exception e) {
                Log.e(TAG, "Exception loading hotword state.", e);
            }
            return null;
        }

        @Override
        protected void onPostExecute(HotwordState hotwordState) {
            if (hotwordState != null) {
                mHotwordState = hotwordState;
            }
            mHotwordStateListener.onHotwordStateChanged();
        }
    }

    private HotwordStateListener mHotwordStateListener = null;
    private HotwordState mHotwordState = new HotwordState();

    public HotwordSwitchController(Context context) {
        super(context);
    }

    /** Must be invoked to init controller and observe state changes. */
    public void init(HotwordStateListener listener) {
        mHotwordState.mHotwordSwitchTitle = mContext.getString(R.string.hotwording_title);
        mHotwordStateListener = listener;
        try {
            mContext.getContentResolver().registerContentObserver(URI, true,
                    mHotwordSwitchObserver);
            new HotwordLoader().execute();
        } catch (SecurityException e) {
            Log.w(TAG, "Hotword content provider not found.", e);
        }
    }

    /** Must be invoked by caller to unregister receivers. */
    public void unregister() {
        mContext.getContentResolver().unregisterContentObserver(mHotwordSwitchObserver);
    }

    @Override
    public boolean isAvailable() {
        return mHotwordState.mHotwordSwitchVisible;
    }

    @Override
    public String getPreferenceKey() {
        return KEY_HOTWORD_SWITCH;
    }

    @Override
    public void updateState(Preference preference) {
        super.updateState(preference);
        if (KEY_HOTWORD_SWITCH.equals(preference.getKey())) {
            ((SwitchPreference) preference).setChecked(mHotwordState.mHotwordEnabled);
            preference.setIcon(mHotwordState.mHotwordEnabled
                    ? R.drawable.ic_mic_on : R.drawable.ic_mic_off);
            preference.setEnabled(!mHotwordState.mHotwordSwitchDisabled);
            preference.setTitle(mHotwordState.mHotwordSwitchTitle);
            preference.setSummary(mHotwordState.mHotwordSwitchDescription);
        }
    }

    @Override
    public boolean handlePreferenceTreeClick(Preference preference) {
        if (KEY_HOTWORD_SWITCH.equals(preference.getKey())) {
            SwitchPreference hotwordSwitchPref = (SwitchPreference) preference;
            if (hotwordSwitchPref.isChecked()) {
                hotwordSwitchPref.setChecked(false);
                mHotwordStateListener.onHotwordEnable();
            } else {
                hotwordSwitchPref.setChecked(true);
                mHotwordStateListener.onHotwordDisable();
            }
        }
        return super.handlePreferenceTreeClick(preference);
    }

    /**
     * Extracts a string resource from a given package.
     *
     * @param resource fully qualified resource identifier,
     *        e.g. com.google.android.katniss:string/enable_ok_google
     * @param defaultValue returned if resource cannot be extracted
     */
    private String getLocalizedStringResource(String resource, @Nullable String defaultValue) {
        if (TextUtils.isEmpty(resource)) {
            return defaultValue;
        }
        try {
            String[] parts = TextUtils.split(resource, ":");
            if (parts.length == 0) {
                return defaultValue;
            }
            final String pkgName = parts[0];
            Context targetContext = mContext.createPackageContext(pkgName, 0);
            int resId = targetContext.getResources().getIdentifier(resource, null, null);
            if (resId != 0) {
                return targetContext.getResources().getString(resId);
            }
        } catch (Resources.NotFoundException | PackageManager.NameNotFoundException
                | SecurityException e) {
            Log.w(TAG, "Unable to get string resource.", e);
        }
        return defaultValue;
    }
}
