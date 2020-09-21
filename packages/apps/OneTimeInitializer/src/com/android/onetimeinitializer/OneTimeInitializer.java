/*
 * Copyright (C) 2012 The Android Open Source Project
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
 * limitations under the License
 */

package com.android.onetimeinitializer;

import android.content.ComponentName;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.database.Cursor;
import android.net.Uri;
import android.util.Log;

import java.net.URISyntaxException;
import java.util.Set;

/**
 * A class that performs one-time initialization after installation.
 *
 * <p>Android doesn't offer any mechanism to trigger an app right after installation, so we use the
 * BOOT_COMPLETED broadcast intent instead.  This means, when the app is upgraded, the
 * initialization code here won't run until the device reboots.
 */
public class OneTimeInitializer {
    private static final String TAG = OneTimeInitializer.class.getSimpleName();

    // Name of the shared preferences file.
    private static final String SHARED_PREFS_FILE = "oti";

    // Name of the preference containing the mapping version.
    private static final String MAPPING_VERSION_PREF = "mapping_version";

    // This is the content uri for Launcher content provider. See
    // LauncherSettings and LauncherProvider in the Launcher app for details.
    private static final Uri LAUNCHER_CONTENT_URI =
            Uri.parse("content://com.android.launcher2.settings/favorites?notify=true");

    private static final String LAUNCHER_ID_COLUMN = "_id";
    private static final String LAUNCHER_INTENT_COLUMN = "intent";

    private SharedPreferences mPreferences;
    private Context mContext;

    OneTimeInitializer(Context context) {
        mContext = context;
        mPreferences = mContext.getSharedPreferences(SHARED_PREFS_FILE, Context.MODE_PRIVATE);
    }

    void initialize() {
        if (Log.isLoggable(TAG, Log.VERBOSE)) {
            Log.v(TAG, "OneTimeInitializer.initialize");
        }

        final int currentVersion = getMappingVersion();
        int newVersion = currentVersion;
        if (currentVersion < 1) {
            if (Log.isLoggable(TAG, Log.INFO)) {
                Log.i(TAG, "Updating to version 1.");
            }
            updateDialtactsLauncher();

            newVersion = 1;
        }

        updateMappingVersion(newVersion);
    }

    private int getMappingVersion() {
        return mPreferences.getInt(MAPPING_VERSION_PREF, 0);
    }

    private void updateMappingVersion(int version) {
        SharedPreferences.Editor ed = mPreferences.edit();
        ed.putInt(MAPPING_VERSION_PREF, version);
        ed.commit();
    }

    private void updateDialtactsLauncher() {
        ContentResolver cr = mContext.getContentResolver();
        Cursor c = cr.query(LAUNCHER_CONTENT_URI,
                new String[]{LAUNCHER_ID_COLUMN, LAUNCHER_INTENT_COLUMN}, null, null, null);
        if (c == null) {
            return;
        }

        try {
            if (Log.isLoggable(TAG, Log.DEBUG)) {
                Log.d(TAG, "Total launcher icons: " + c.getCount());
            }

            while (c.moveToNext()) {
                long favoriteId = c.getLong(0);
                final String intentUri = c.getString(1);
                if (intentUri != null) {
                    try {
                        final Intent intent = Intent.parseUri(intentUri, 0);
                        final ComponentName componentName = intent.getComponent();
                        final Set<String> categories = intent.getCategories();
                        if (Intent.ACTION_MAIN.equals(intent.getAction()) &&
                                componentName != null &&
                                "com.android.contacts".equals(componentName.getPackageName()) &&
                                "com.android.contacts.activities.DialtactsActivity".equals(
                                        componentName.getClassName()) &&
                                categories != null &&
                                categories.contains(Intent.CATEGORY_LAUNCHER)) {

                            final ComponentName newName = new ComponentName("com.android.dialer",
                                    "com.android.dialer.DialtactsActivity");
                            intent.setComponent(newName);
                            final ContentValues values = new ContentValues();
                            values.put(LAUNCHER_INTENT_COLUMN, intent.toUri(0));

                            String updateWhere = LAUNCHER_ID_COLUMN + "=" + favoriteId;
                            cr.update(LAUNCHER_CONTENT_URI, values, updateWhere, null);
                            if (Log.isLoggable(TAG, Log.INFO)) {
                                Log.i(TAG, "Updated " + componentName + " to " + newName);
                            }
                        }
                    } catch (RuntimeException ex) {
                        Log.e(TAG, "Problem moving Dialtacts activity", ex);
                    } catch (URISyntaxException e) {
                        Log.e(TAG, "Problem moving Dialtacts activity", e);
                    }
                }
            }

        } finally {
            if (c != null) {
                c.close();
            }
        }
    }
}
