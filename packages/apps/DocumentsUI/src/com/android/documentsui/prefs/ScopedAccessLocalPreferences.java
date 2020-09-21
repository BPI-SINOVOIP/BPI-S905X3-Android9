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
package com.android.documentsui.prefs;

import static com.android.documentsui.base.SharedMinimal.DEBUG;
import static com.android.documentsui.base.SharedMinimal.DIRECTORY_ROOT;
import static com.android.internal.util.Preconditions.checkArgument;

import android.annotation.IntDef;
import android.annotation.Nullable;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.os.UserHandle;
import android.preference.PreferenceManager;
import android.text.TextUtils;
import android.util.ArraySet;
import android.util.Log;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.ArrayList;
import java.util.List;
import java.util.Map.Entry;
import java.util.Set;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Methods for accessing the local preferences with regards to scoped directory access.
 */
//TODO(b/72055774): add unit tests
public class ScopedAccessLocalPreferences {

    private static final String TAG = "ScopedAccessLocalPreferences";

    private static SharedPreferences getPrefs(Context context) {
        return PreferenceManager.getDefaultSharedPreferences(context);
    }

    public static final int PERMISSION_ASK = 0;
    public static final int PERMISSION_ASK_AGAIN = 1;
    public static final int PERMISSION_NEVER_ASK = -1;
    // NOTE: this status is not used on preferences, but on permissions granted by AM
    public static final int PERMISSION_GRANTED = 2;

    @IntDef(flag = true, value = {
            PERMISSION_ASK,
            PERMISSION_ASK_AGAIN,
            PERMISSION_NEVER_ASK,
            PERMISSION_GRANTED
    })
    @Retention(RetentionPolicy.SOURCE)
    public @interface PermissionStatus {}

    private static final String KEY_REGEX = "^.+\\|(.+)\\|(.*)\\|(.+)$";
    private static final Pattern KEY_PATTERN = Pattern.compile(KEY_REGEX);

    /**
     * Methods below are used to keep track of denied user requests on scoped directory access so
     * the dialog is not offered when user checked the 'Do not ask again' box
     *
     * <p>It uses a shared preferences, whose key is:
     * <ol>
     * <li>{@code USER_ID|PACKAGE_NAME|VOLUME_UUID|DIRECTORY} for storage volumes that have a UUID
     * (typically physical volumes like SD cards).
     * <li>{@code USER_ID|PACKAGE_NAME||DIRECTORY} for storage volumes that do not have a UUID
     * (typically the emulated volume used for primary storage
     * </ol>
     */
    public static @PermissionStatus int getScopedAccessPermissionStatus(Context context,
            String packageName, @Nullable String uuid, String directory) {
        final String key = getScopedAccessDenialsKey(packageName, uuid, directory);
        return getPrefs(context).getInt(key, PERMISSION_ASK);
    }

    public static void setScopedAccessPermissionStatus(Context context, String packageName,
            @Nullable String uuid, String directory, @PermissionStatus int status) {
        checkArgument(!TextUtils.isEmpty(directory),
                "Cannot pass empty directory - did you mean %s?", DIRECTORY_ROOT);
        final String key = getScopedAccessDenialsKey(packageName, uuid, directory);
        if (DEBUG) {
            Log.d(TAG, "Setting permission of " + packageName + ":" + uuid + ":" + directory
                    + " to " + statusAsString(status));
        }

        getPrefs(context).edit().putInt(key, status).apply();
    }

    public static int clearScopedAccessPreferences(Context context, String packageName) {
        final String keySubstring = "|" + packageName + "|";
        final SharedPreferences prefs = getPrefs(context);
        Editor editor = null;
        int removed = 0;
        for (final String key : prefs.getAll().keySet()) {
            if (key.contains(keySubstring)) {
                if (editor == null) {
                    editor = prefs.edit();
                }
                editor.remove(key);
                removed ++;
            }
        }
        if (editor != null) {
            editor.apply();
        }
        return removed;
    }

    private static String getScopedAccessDenialsKey(String packageName, @Nullable String uuid,
            String directory) {
        final int userId = UserHandle.myUserId();
        return uuid == null
                ? userId + "|" + packageName + "||" + directory
                : userId + "|" + packageName + "|" + uuid + "|" + directory;
    }

    /**
     * Clears all preferences associated with a given package.
     *
     * <p>Typically called when a package is removed or when user asked to clear its data.
     */
    public static void clearPackagePreferences(Context context, String packageName) {
        ScopedAccessLocalPreferences.clearScopedAccessPreferences(context, packageName);
    }

    /**
     * Gets all packages that have entries in the preferences
     */
    public static Set<String> getAllPackages(Context context) {
        final SharedPreferences prefs = getPrefs(context);

        final ArraySet<String> pkgs = new ArraySet<>();
        for (Entry<String, ?> pref : prefs.getAll().entrySet()) {
            final String key = pref.getKey();
            final String pkg = getPackage(key);
            if (pkg == null) {
                Log.w(TAG, "getAllPackages(): error parsing pref '" + key + "'");
                continue;
            }
            pkgs.add(pkg);
        }
        return pkgs;
    }

    /**
     * Gets all permissions.
     */
    public static List<Permission> getAllPermissions(Context context) {
        final SharedPreferences prefs = getPrefs(context);
        final ArrayList<Permission> permissions = new ArrayList<>();

        for (Entry<String, ?> pref : prefs.getAll().entrySet()) {
            final String key = pref.getKey();
            final Object value = pref.getValue();
            final Integer status;
            try {
                status = (Integer) value;
            } catch (Exception e) {
                Log.w(TAG, "error gettting value for key '" + key + "': " + value);
                continue;
            }
            final Permission permission = getPermission(key, status);
            if (permission != null) {
                permissions.add(permission);
            }
        }

        return permissions;
    }

    public static String statusAsString(@PermissionStatus int status) {
        switch (status) {
            case PERMISSION_ASK:
                return "PERMISSION_ASK";
            case PERMISSION_ASK_AGAIN:
                return "PERMISSION_ASK_AGAIN";
            case PERMISSION_NEVER_ASK:
                return "PERMISSION_NEVER_ASK";
            case PERMISSION_GRANTED:
                return "PERMISSION_GRANTED";
            default:
                return "UNKNOWN";
        }
    }

    @Nullable
    private static String getPackage(String key) {
        final Matcher matcher = KEY_PATTERN.matcher(key);
        return matcher.matches() ? matcher.group(1) : null;
    }

    private static Permission getPermission(String key, Integer status) {
        final Matcher matcher = KEY_PATTERN.matcher(key);
        if (!matcher.matches()) return null;

        final String pkg = matcher.group(1);
        final String uuid = matcher.group(2);
        final String directory = matcher.group(3);

        return new Permission(pkg, uuid, directory, status);
    }

    public static final class Permission {
        public final String pkg;

        @Nullable
        public final String uuid;
        public final String directory;
        public final int status;

        public Permission(String pkg, String uuid, String directory, Integer status) {
            this.pkg = pkg;
            this.uuid = TextUtils.isEmpty(uuid) ? null : uuid;
            this.directory = directory;
            this.status = status.intValue();
        }

        @Override
        public String toString() {
            return "Permission: [pkg=" + pkg + ", uuid=" + uuid + ", dir=" + directory + ", status="
                    + statusAsString(status) + " (" + status + ")]";
        }
    }
}
