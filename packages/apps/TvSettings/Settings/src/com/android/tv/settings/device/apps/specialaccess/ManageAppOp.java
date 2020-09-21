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

package com.android.tv.settings.device.apps.specialaccess;

import android.app.ActivityThread;
import android.app.AppOpsManager;
import android.content.Context;
import android.content.pm.IPackageManager;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.RemoteException;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v7.preference.Preference;
import android.util.Log;

import com.android.internal.util.ArrayUtils;
import com.android.settingslib.applications.ApplicationsState;
import com.android.tv.settings.R;
import com.android.tv.settings.SettingsPreferenceFragment;

import java.util.Comparator;

/**
 * Base class for managing app ops
 */
public abstract class ManageAppOp extends SettingsPreferenceFragment
        implements ManageApplicationsController.Callback {
    private static final String TAG = "ManageAppOps";

    private IPackageManager mIPackageManager;
    private AppOpsManager mAppOpsManager;

    private ManageApplicationsController mManageApplicationsController;

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        mManageApplicationsController = new ManageApplicationsController(context, this,
                getLifecycle(), getAppFilter(), getAppComparator());
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        mIPackageManager = ActivityThread.getPackageManager();
        mAppOpsManager = getContext().getSystemService(AppOpsManager.class);
        super.onCreate(savedInstanceState);
    }

    /**
     * Subclasses may override this to provide an alternate app filter. The default filter inserts
     * {@link PermissionState} objects into the {@link ApplicationsState.AppEntry#extraInfo} field.
     * @return {@link ApplicationsState.AppFilter}
     */
    @NonNull
    public ApplicationsState.AppFilter getAppFilter() {
        return new ApplicationsState.AppFilter() {
            @Override
            public void init() {
            }

            @Override
            public boolean filterApp(ApplicationsState.AppEntry entry) {
                entry.extraInfo = createPermissionStateFor(entry.info.packageName, entry.info.uid);
                return !shouldIgnorePackage(getContext(), entry.info.packageName)
                        && ((PermissionState) entry.extraInfo).isPermissible();
            }
        };
    }

    /**
     * Subclasses may override this to provide an alternate comparator for sorting apps
     * @return {@link Comparator} for {@link ApplicationsState.AppEntry} objects.
     */
    @Nullable
    public Comparator<ApplicationsState.AppEntry> getAppComparator() {
        return ApplicationsState.ALPHA_COMPARATOR;
    }

    /**
     * Call to trigger the app list to update
     */
    public void updateAppList() {
        mManageApplicationsController.updateAppList();
    }

    /**
     * @return AppOps code
     */
    public abstract int getAppOpsOpCode();

    /**
     * @return Manifest permission string
     */
    public abstract String getPermission();

    private boolean hasRequestedAppOpPermission(String permission, String packageName) {
        try {
            String[] packages = mIPackageManager.getAppOpPermissionPackages(permission);
            return ArrayUtils.contains(packages, packageName);
        } catch (RemoteException exc) {
            Log.e(TAG, "PackageManager dead. Cannot get permission info");
            return false;
        }
    }

    private boolean hasPermission(int uid) {
        try {
            int result = mIPackageManager.checkUidPermission(getPermission(), uid);
            return result == PackageManager.PERMISSION_GRANTED;
        } catch (RemoteException e) {
            Log.e(TAG, "PackageManager dead. Cannot get permission info");
            return false;
        }
    }

    private int getAppOpMode(int uid, String packageName) {
        return mAppOpsManager.checkOpNoThrow(getAppOpsOpCode(), uid, packageName);
    }

    private PermissionState createPermissionStateFor(String packageName, int uid) {
        return new PermissionState(
                hasRequestedAppOpPermission(getPermission(), packageName),
                hasPermission(uid),
                getAppOpMode(uid, packageName));
    }

    /**
     * Checks for packages that should be ignored for further processing
     */
    static boolean shouldIgnorePackage(Context context, String packageName) {
        return context == null
                || packageName.equals("android")
                || packageName.equals("com.android.systemui")
                || packageName.equals(context.getPackageName());
    }

    /**
     * Collection of information to be used as {@link ApplicationsState.AppEntry#extraInfo} objects
     */
    public static class PermissionState {
        public final boolean permissionRequested;
        public final boolean permissionGranted;
        public final int appOpMode;

        private PermissionState(boolean permissionRequested, boolean permissionGranted,
                int appOpMode) {
            this.permissionRequested = permissionRequested;
            this.permissionGranted = permissionGranted;
            this.appOpMode = appOpMode;
        }

        /**
         * @return True if the permission is granted
         */
        public boolean isAllowed() {
            if (appOpMode == AppOpsManager.MODE_DEFAULT) {
                return permissionGranted;
            } else {
                return appOpMode == AppOpsManager.MODE_ALLOWED;
            }
        }

        /**
         * @return True if the permission is relevant
         */
        public boolean isPermissible() {
            return appOpMode != AppOpsManager.MODE_DEFAULT || permissionRequested;
        }

        @Override
        public String toString() {
            return "[permissionGranted: " + permissionGranted
                    + ", permissionRequested: " + permissionRequested
                    + ", appOpMode: " + appOpMode
                    + "]";
        }
    }

    @Override
    @NonNull
    public Preference getEmptyPreference() {
        final Preference empty = new Preference(getPreferenceManager().getContext());
        empty.setKey("empty");
        empty.setTitle(R.string.noApplications);
        empty.setEnabled(false);
        return empty;
    }
}
