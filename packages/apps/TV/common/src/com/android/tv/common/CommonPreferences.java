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
package com.android.tv.common;

import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.content.SharedPreferences;
import android.database.ContentObserver;
import android.database.Cursor;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.support.annotation.GuardedBy;
import android.support.annotation.IntDef;
import android.support.annotation.MainThread;
import android.util.Log;
import com.android.tv.common.CommonPreferenceProvider.Preferences;
import com.android.tv.common.util.CommonUtils;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.HashMap;
import java.util.Map;

/** A helper class for setting/getting common preferences across applications. */
public class CommonPreferences {
    private static final String TAG = "CommonPreferences";

    private static final String PREFS_KEY_LAUNCH_SETUP = "launch_setup";
    private static final String PREFS_KEY_STORE_TS_STREAM = "store_ts_stream";
    private static final String PREFS_KEY_TRICKPLAY_SETTING = "trickplay_setting";
    private static final String PREFS_KEY_LAST_POSTAL_CODE = "last_postal_code";

    private static final Map<String, Class> sPref2TypeMapping = new HashMap<>();

    static {
        sPref2TypeMapping.put(PREFS_KEY_TRICKPLAY_SETTING, int.class);
        sPref2TypeMapping.put(PREFS_KEY_STORE_TS_STREAM, boolean.class);
        sPref2TypeMapping.put(PREFS_KEY_LAUNCH_SETUP, boolean.class);
        sPref2TypeMapping.put(PREFS_KEY_LAST_POSTAL_CODE, String.class);
    }

    private static final String SHARED_PREFS_NAME =
            CommonConstants.BASE_PACKAGE + ".common.preferences";

    @IntDef({TRICKPLAY_SETTING_NOT_SET, TRICKPLAY_SETTING_DISABLED, TRICKPLAY_SETTING_ENABLED})
    @Retention(RetentionPolicy.SOURCE)
    public @interface TrickplaySetting {}

    /** Trickplay setting is not changed by a user. Trickplay will be enabled in this case. */
    public static final int TRICKPLAY_SETTING_NOT_SET = -1;

    /** Trickplay setting is disabled. */
    public static final int TRICKPLAY_SETTING_DISABLED = 0;

    /** Trickplay setting is enabled. */
    public static final int TRICKPLAY_SETTING_ENABLED = 1;

    @GuardedBy("CommonPreferences.class")
    private static final Bundle sPreferenceValues = new Bundle();

    private static LoadPreferencesTask sLoadPreferencesTask;
    private static ContentObserver sContentObserver;
    private static CommonPreferencesChangedListener sPreferencesChangedListener = null;

    protected static boolean sInitialized;

    /** Listeners for CommonPreferences change. */
    public interface CommonPreferencesChangedListener {
        void onCommonPreferencesChanged();
    }

    /** Initializes the common preferences. */
    @MainThread
    public static void initialize(final Context context) {
        if (sInitialized) {
            return;
        }
        sInitialized = true;
        if (useContentProvider(context)) {
            loadPreferences(context);
            sContentObserver =
                    new ContentObserver(new Handler()) {
                        @Override
                        public void onChange(boolean selfChange) {
                            loadPreferences(context);
                        }
                    };
            context.getContentResolver()
                    .registerContentObserver(Preferences.CONTENT_URI, true, sContentObserver);
        } else {
            new AsyncTask<Void, Void, Void>() {
                @Override
                protected Void doInBackground(Void... params) {
                    getSharedPreferences(context);
                    return null;
                }
            }.execute();
        }
    }

    /** Releases the resources. */
    public static synchronized void release(Context context) {
        if (useContentProvider(context) && sContentObserver != null) {
            context.getContentResolver().unregisterContentObserver(sContentObserver);
        }
        setCommonPreferencesChangedListener(null);
    }

    /** Sets the listener for CommonPreferences change. */
    public static void setCommonPreferencesChangedListener(
            CommonPreferencesChangedListener listener) {
        sPreferencesChangedListener = listener;
    }

    /**
     * Loads the preferences from database.
     *
     * <p>This preferences is used across processes, so the preferences should be loaded again when
     * the databases changes.
     */
    @MainThread
    public static void loadPreferences(Context context) {
        if (sLoadPreferencesTask != null
                && sLoadPreferencesTask.getStatus() != AsyncTask.Status.FINISHED) {
            sLoadPreferencesTask.cancel(true);
        }
        sLoadPreferencesTask = new LoadPreferencesTask(context);
        sLoadPreferencesTask.execute();
    }

    private static boolean useContentProvider(Context context) {
        // If TIS is a part of LC, it should use ContentProvider to resolve multiple process access.
        return CommonUtils.isPackagedWithLiveChannels(context);
    }

    public static synchronized boolean shouldShowSetupActivity(Context context) {
        SoftPreconditions.checkState(sInitialized);
        if (useContentProvider(context)) {
            return sPreferenceValues.getBoolean(PREFS_KEY_LAUNCH_SETUP);
        } else {
            return getSharedPreferences(context).getBoolean(PREFS_KEY_LAUNCH_SETUP, false);
        }
    }

    public static synchronized void setShouldShowSetupActivity(Context context, boolean need) {
        if (useContentProvider(context)) {
            setPreference(context, PREFS_KEY_LAUNCH_SETUP, need);
        } else {
            getSharedPreferences(context).edit().putBoolean(PREFS_KEY_LAUNCH_SETUP, need).apply();
        }
    }

    public static synchronized @TrickplaySetting int getTrickplaySetting(Context context) {
        SoftPreconditions.checkState(sInitialized);
        if (useContentProvider(context)) {
            return sPreferenceValues.getInt(PREFS_KEY_TRICKPLAY_SETTING, TRICKPLAY_SETTING_NOT_SET);
        } else {
            return getSharedPreferences(context)
                    .getInt(PREFS_KEY_TRICKPLAY_SETTING, TRICKPLAY_SETTING_NOT_SET);
        }
    }

    public static synchronized void setTrickplaySetting(
            Context context, @TrickplaySetting int trickplaySetting) {
        SoftPreconditions.checkState(sInitialized);
        SoftPreconditions.checkArgument(trickplaySetting != TRICKPLAY_SETTING_NOT_SET);
        if (useContentProvider(context)) {
            setPreference(context, PREFS_KEY_TRICKPLAY_SETTING, trickplaySetting);
        } else {
            getSharedPreferences(context)
                    .edit()
                    .putInt(PREFS_KEY_TRICKPLAY_SETTING, trickplaySetting)
                    .apply();
        }
    }

    public static synchronized boolean getStoreTsStream(Context context) {
        SoftPreconditions.checkState(sInitialized);
        if (useContentProvider(context)) {
            return sPreferenceValues.getBoolean(PREFS_KEY_STORE_TS_STREAM, false);
        } else {
            return getSharedPreferences(context).getBoolean(PREFS_KEY_STORE_TS_STREAM, false);
        }
    }

    public static synchronized void setStoreTsStream(Context context, boolean shouldStore) {
        if (useContentProvider(context)) {
            setPreference(context, PREFS_KEY_STORE_TS_STREAM, shouldStore);
        } else {
            getSharedPreferences(context)
                    .edit()
                    .putBoolean(PREFS_KEY_STORE_TS_STREAM, shouldStore)
                    .apply();
        }
    }

    public static synchronized String getLastPostalCode(Context context) {
        SoftPreconditions.checkState(sInitialized);
        if (useContentProvider(context)) {
            return sPreferenceValues.getString(PREFS_KEY_LAST_POSTAL_CODE);
        } else {
            return getSharedPreferences(context).getString(PREFS_KEY_LAST_POSTAL_CODE, null);
        }
    }

    public static synchronized void setLastPostalCode(Context context, String postalCode) {
        if (useContentProvider(context)) {
            setPreference(context, PREFS_KEY_LAST_POSTAL_CODE, postalCode);
        } else {
            getSharedPreferences(context)
                    .edit()
                    .putString(PREFS_KEY_LAST_POSTAL_CODE, postalCode)
                    .apply();
        }
    }

    protected static SharedPreferences getSharedPreferences(Context context) {
        return context.getSharedPreferences(SHARED_PREFS_NAME, Context.MODE_PRIVATE);
    }

    private static synchronized void setPreference(Context context, String key, String value) {
        sPreferenceValues.putString(key, value);
        savePreference(context, key, value);
    }

    private static synchronized void setPreference(Context context, String key, int value) {
        sPreferenceValues.putInt(key, value);
        savePreference(context, key, Integer.toString(value));
    }

    private static synchronized void setPreference(Context context, String key, long value) {
        sPreferenceValues.putLong(key, value);
        savePreference(context, key, Long.toString(value));
    }

    private static synchronized void setPreference(Context context, String key, boolean value) {
        sPreferenceValues.putBoolean(key, value);
        savePreference(context, key, Boolean.toString(value));
    }

    private static void savePreference(
            final Context context, final String key, final String value) {
        new AsyncTask<Void, Void, Void>() {
            @Override
            protected Void doInBackground(Void... params) {
                ContentResolver resolver = context.getContentResolver();
                ContentValues values = new ContentValues();
                values.put(Preferences.COLUMN_KEY, key);
                values.put(Preferences.COLUMN_VALUE, value);
                try {
                    resolver.insert(Preferences.CONTENT_URI, values);
                } catch (Exception e) {
                    SoftPreconditions.warn(
                            TAG, "setPreference", e, "Error writing preference values");
                }
                return null;
            }
        }.execute();
    }

    private static class LoadPreferencesTask extends AsyncTask<Void, Void, Bundle> {
        private final Context mContext;

        private LoadPreferencesTask(Context context) {
            mContext = context;
        }

        @Override
        protected Bundle doInBackground(Void... params) {
            Bundle bundle = new Bundle();
            ContentResolver resolver = mContext.getContentResolver();
            String[] projection = new String[] {Preferences.COLUMN_KEY, Preferences.COLUMN_VALUE};
            try (Cursor cursor =
                    resolver.query(Preferences.CONTENT_URI, projection, null, null, null)) {
                if (cursor != null) {
                    while (!isCancelled() && cursor.moveToNext()) {
                        String key = cursor.getString(0);
                        String value = cursor.getString(1);
                        Class prefClass = sPref2TypeMapping.get(key);
                        if (prefClass == int.class) {
                            try {
                                bundle.putInt(key, Integer.parseInt(value));
                            } catch (NumberFormatException e) {
                                Log.w(TAG, "Invalid format, key=" + key + ", value=" + value);
                            }
                        } else if (prefClass == long.class) {
                            try {
                                bundle.putLong(key, Long.parseLong(value));
                            } catch (NumberFormatException e) {
                                Log.w(TAG, "Invalid format, key=" + key + ", value=" + value);
                            }
                        } else if (prefClass == boolean.class) {
                            bundle.putBoolean(key, Boolean.parseBoolean(value));
                        } else {
                            bundle.putString(key, value);
                        }
                    }
                }
            } catch (Exception e) {
                SoftPreconditions.warn(TAG, "getPreference", e, "Error querying preference values");
                return null;
            }
            return bundle;
        }

        @Override
        protected void onPostExecute(Bundle bundle) {
            synchronized (CommonPreferences.class) {
                if (bundle != null) {
                    sPreferenceValues.putAll(bundle);
                }
            }
            if (sPreferencesChangedListener != null) {
                sPreferencesChangedListener.onCommonPreferencesChanged();
            }
        }
    }
}
