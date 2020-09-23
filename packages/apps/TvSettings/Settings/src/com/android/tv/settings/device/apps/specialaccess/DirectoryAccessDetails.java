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

package com.android.tv.settings.device.apps.specialaccess;

import static android.os.storage.StorageVolume.ScopedAccessProviderContract.AUTHORITY;
import static android.os.storage.StorageVolume.ScopedAccessProviderContract.COL_GRANTED;
import static android.os.storage.StorageVolume.ScopedAccessProviderContract.TABLE_PERMISSIONS;
import static android.os.storage.StorageVolume.ScopedAccessProviderContract
        .TABLE_PERMISSIONS_COLUMNS;
import static android.os.storage.StorageVolume.ScopedAccessProviderContract
        .TABLE_PERMISSIONS_COL_DIRECTORY;
import static android.os.storage.StorageVolume.ScopedAccessProviderContract
        .TABLE_PERMISSIONS_COL_GRANTED;
import static android.os.storage.StorageVolume.ScopedAccessProviderContract
        .TABLE_PERMISSIONS_COL_PACKAGE;
import static android.os.storage.StorageVolume.ScopedAccessProviderContract
        .TABLE_PERMISSIONS_COL_VOLUME_UUID;

import android.annotation.Nullable;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.os.UserHandle;
import android.os.storage.StorageManager;
import android.os.storage.VolumeInfo;
import android.support.v14.preference.SwitchPreference;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceCategory;
import android.support.v7.preference.PreferenceScreen;
import android.util.Log;
import android.util.Pair;

import com.android.internal.logging.nano.MetricsProto.MetricsEvent;
import com.android.settingslib.applications.ApplicationsState;
import com.android.tv.settings.R;
import com.android.tv.settings.SettingsPreferenceFragment;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * Detailed settings for an app's directory access permissions (A.K.A Scoped Directory Access).
 *
 * <p>Currently, it shows the entry for which the user denied access with the "Do not ask again"
 * flag checked on: the user than can use the settings toggle to reset that deniel.
 *
 * <p>This fragments dynamically lists all such permissions, starting with one preference per
 * directory in the primary storage, then adding additional entries for the external volumes (one
 * entry for the whole volume).
 */
// TODO(b/72055774): add unit tests
public class DirectoryAccessDetails extends SettingsPreferenceFragment {

    @SuppressWarnings("hiding")
    private static final String TAG = "DirectoryAccessDetails";

    private static final boolean DEBUG = false;
    private static final boolean VERBOSE = false;
    public static final String ARG_PACKAGE_NAME = "package";

    private boolean mCreated;
    private PackageInfo mPackageInfo;
    private PackageManager mPm;
    private String mPackageName;
    private int mUserId;
    private ApplicationsState.AppEntry mAppEntry;
    private ApplicationsState mState;

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        if (mCreated) {
            Log.w(TAG, "onActivityCreated(): ignoring duplicate call");
            return;
        }
        mCreated = true;
        mPm = getActivity().getPackageManager();
        mState = ApplicationsState.getInstance(getActivity().getApplication());
        retrieveAppEntry();
        if (mPackageInfo == null) {
            Log.w(TAG, "onActivityCreated(): no package info");
            return;
        }

        final Preference pref = new PreferenceCategory(getPrefContext(), null);
        getPreferenceScreen().addPreference(pref);
        refreshUi();
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        addPreferencesFromResource(R.xml.directory_access_details);
    }

    private Context getPrefContext() {
        return getPreferenceManager().getContext();
    }

    protected Intent getIntent() {
        if (getActivity() == null) {
            return null;
        }
        return getActivity().getIntent();
    }

    protected String retrieveAppEntry() {
        final Bundle args = getArguments();
        mPackageName = (args != null) ? args.getString(ARG_PACKAGE_NAME) : null;
        Intent intent = (args == null)
                ? getIntent() : (Intent) args.getParcelable("intent");
        if (mPackageName == null) {
            if (intent != null && intent.getData() != null) {
                mPackageName = intent.getData().getSchemeSpecificPart();
            }
        }
        if (intent != null && intent.hasExtra(Intent.EXTRA_USER_HANDLE)) {
            mUserId = ((UserHandle) intent.getParcelableExtra(
                    Intent.EXTRA_USER_HANDLE)).getIdentifier();
        } else {
            mUserId = UserHandle.myUserId();
        }
        mAppEntry = mState.getEntry(mPackageName, mUserId);
        if (mAppEntry != null) {
            // Get application info again to refresh changed properties of application
            try {
                mPackageInfo = mPm.getPackageInfoAsUser(mAppEntry.info.packageName,
                        PackageManager.MATCH_DISABLED_COMPONENTS
                                | PackageManager.GET_SIGNING_CERTIFICATES
                                | PackageManager.GET_PERMISSIONS, mUserId);
            } catch (PackageManager.NameNotFoundException e) {
                Log.e(TAG, "Exception when retrieving package:" + mAppEntry.info.packageName, e);
            }
        } else {
            Log.w(TAG, "Missing AppEntry; maybe reinstalling?");
            mPackageInfo = null;
        }

        return mPackageName;
    }

    protected boolean refreshUi() {
        final Context context = getPrefContext();
        final PreferenceScreen prefsGroup = getPreferenceScreen();
        prefsGroup.removeAll();

        final Map<String, ExternalVolume> externalVolumes = new HashMap<>();

        final Uri providerUri = new Uri.Builder().scheme(ContentResolver.SCHEME_CONTENT)
                .authority(AUTHORITY).appendPath(TABLE_PERMISSIONS).appendPath("*")
                .build();
        // Query provider for entries.
        try (Cursor cursor = context.getContentResolver().query(providerUri,
                TABLE_PERMISSIONS_COLUMNS, null, new String[]{mPackageName}, null)) {
            if (cursor == null) {
                Log.w(TAG, "Didn't get cursor for " + mPackageName);
                return true;
            }
            final int count = cursor.getCount();
            if (count == 0) {
                // This setting screen should not be reached if there was no permission, so just
                // ignore it
                Log.w(TAG, "No permissions for " + mPackageName);
                return true;
            }

            while (cursor.moveToNext()) {
                final String pkg = cursor.getString(TABLE_PERMISSIONS_COL_PACKAGE);
                final String uuid = cursor.getString(TABLE_PERMISSIONS_COL_VOLUME_UUID);
                final String dir = cursor.getString(TABLE_PERMISSIONS_COL_DIRECTORY);
                final boolean granted = cursor.getInt(TABLE_PERMISSIONS_COL_GRANTED) == 1;
                if (VERBOSE) {
                    Log.v(TAG, "Pkg:" + pkg + " uuid: " + uuid + " dir: " + dir
                            + " granted:" + granted);
                }

                if (!mPackageName.equals(pkg)) {
                    // Sanity check, shouldn't happen
                    Log.w(TAG, "Ignoring " + uuid + "/" + dir + " due to package mismatch: "
                            + "expected " + mPackageName + ", got " + pkg);
                    continue;
                }

                if (uuid == null) {
                    if (dir == null) {
                        // Sanity check, shouldn't happen
                        Log.wtf(TAG, "Ignoring permission on primary storage root");
                    } else {
                        // Primary storage entry: add right away
                        prefsGroup.addPreference(newPreference(context, dir, providerUri,
                                /* uuid= */ null, dir, granted, /* children= */ null));
                    }
                } else {
                    // External volume entry: save it for later.
                    ExternalVolume externalVolume = externalVolumes.get(uuid);
                    if (externalVolume == null) {
                        externalVolume = new ExternalVolume(uuid);
                        externalVolumes.put(uuid, externalVolume);
                    }
                    if (dir == null) {
                        // Whole volume
                        externalVolume.mGranted = granted;
                    } else {
                        // Directory only
                        externalVolume.mChildren.add(new Pair<>(dir, granted));
                    }
                }
            }
        }

        if (VERBOSE) {
            Log.v(TAG, "external volumes: " + externalVolumes);
        }

        if (externalVolumes.isEmpty()) {
            // We're done!
            return true;
        }

        // Add entries from external volumes

        // Query StorageManager to get the user-friendly volume names.
        final StorageManager sm = context.getSystemService(StorageManager.class);
        final List<VolumeInfo> volumes = sm.getVolumes();
        if (volumes.isEmpty()) {
            Log.w(TAG, "StorageManager returned no secondary volumes");
            return true;
        }
        final Map<String, String> volumeNames = new HashMap<>(volumes.size());
        for (VolumeInfo volume : volumes) {
            final String uuid = volume.getFsUuid();
            if (uuid == null) continue; // Primary storage; not used.

            String name = sm.getBestVolumeDescription(volume);
            if (name == null) {
                Log.w(TAG, "No description for " + volume + "; using uuid instead: " + uuid);
                name = uuid;
            }
            volumeNames.put(uuid, name);
        }
        if (VERBOSE) {
            Log.v(TAG, "UUID -> name mapping: " + volumeNames);
        }

        for (ExternalVolume volume : externalVolumes.values()) {
            final String volumeName = volumeNames.get(volume.mUuid);
            if (volumeName == null) {
                Log.w(TAG, "Ignoring entry for invalid UUID: " + volume.mUuid);
                continue;
            }
            // First add the pref for the whole volume...
            final PreferenceCategory category = new PreferenceCategory(context);
            prefsGroup.addPreference(category);
            final Set<SwitchPreference> children = new HashSet<>(volume.mChildren.size());
            category.addPreference(newPreference(context, volumeName, providerUri, volume.mUuid,
                    /* dir= */ null, volume.mGranted, children));

            // ... then the children prefs
            volume.mChildren.forEach((pair) -> {
                final String dir = pair.first;
                final String name = context.getResources()
                        .getString(R.string.directory_on_volume, volumeName, dir);
                final SwitchPreference childPref =
                        newPreference(context, name, providerUri, volume.mUuid, dir, pair.second,
                                /* children= */ null);
                category.addPreference(childPref);
                children.add(childPref);
            });
        }
        return true;
    }

    private SwitchPreference newPreference(Context context, String title, Uri providerUri,
            String uuid, String dir, boolean granted, @Nullable Set<SwitchPreference> children) {
        final SwitchPreference pref = new SwitchPreference(context);
        pref.setKey(String.format("%s:%s", uuid, dir));
        pref.setTitle(title);
        pref.setChecked(granted);
        pref.setOnPreferenceChangeListener((unused, value) -> {
            if (!Boolean.class.isInstance(value)) {
                // Sanity check
                Log.wtf(TAG, "Invalid value from switch: " + value);
                return true;
            }
            final boolean newValue = ((Boolean) value).booleanValue();

            resetDoNotAskAgain(context, newValue, providerUri, uuid, dir);
            if (children != null) {
                // When parent is granted, children should be hidden; and vice versa
                final boolean newChildValue = !newValue;
                for (SwitchPreference child : children) {
                    child.setVisible(newChildValue);
                }
            }
            return true;
        });
        return pref;
    }

    private void resetDoNotAskAgain(Context context, boolean newValue, Uri providerUri,
            @Nullable String uuid, @Nullable String directory) {
        if (DEBUG) {
            Log.d(TAG, "Asking " + providerUri + " to update " + uuid + "/" + directory + " to "
                    + newValue);
        }
        final ContentValues values = new ContentValues(1);
        values.put(COL_GRANTED, newValue);
        final int updated = context.getContentResolver().update(providerUri, values,
                null, new String[]{mPackageName, uuid, directory});
        if (DEBUG) {
            Log.d(TAG, "Updated " + updated + " entries for " + uuid + "/" + directory);
        }
    }

    @Override
    public int getMetricsCategory() {
        return MetricsEvent.APPLICATIONS_DIRECTORY_ACCESS_DETAIL;
    }

    private static class ExternalVolume {
        final String mUuid;
        final List<Pair<String, Boolean>> mChildren = new ArrayList<>();
        boolean mGranted;

        ExternalVolume(String uuid) {
            mUuid = uuid;
        }

        @Override
        public String toString() {
            return "ExternalVolume: [uuid=" + mUuid + ", granted=" + mGranted
                    + ", children=" + mChildren + "]";
        }
    }
}
