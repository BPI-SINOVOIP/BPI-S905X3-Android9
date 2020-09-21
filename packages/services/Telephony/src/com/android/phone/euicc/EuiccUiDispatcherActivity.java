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
package com.android.phone.euicc;

import android.annotation.NonNull;
import android.annotation.Nullable;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.pm.IPackageManager;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.os.Bundle;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.service.euicc.EuiccService;
import android.telephony.euicc.EuiccManager;
import android.util.Log;

import com.android.internal.annotations.VisibleForTesting;
import com.android.internal.telephony.euicc.EuiccConnector;

import java.util.HashSet;
import java.util.List;
import java.util.Set;

/** Trampoline activity to forward eUICC intents from apps to the active UI implementation. */
public class EuiccUiDispatcherActivity extends Activity {
    private static final String TAG = "EuiccUiDispatcher";

    /** Flags to use when querying PackageManager for Euicc component implementations. */
    private static final int EUICC_QUERY_FLAGS =
            PackageManager.MATCH_SYSTEM_ONLY | PackageManager.MATCH_DEBUG_TRIAGED_MISSING
                    | PackageManager.GET_RESOLVED_FILTER;

    private final IPackageManager mPackageManager = IPackageManager.Stub
            .asInterface(ServiceManager.getService("package"));

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        try {
            Intent euiccUiIntent = resolveEuiccUiIntent();
            if (euiccUiIntent == null) {
                setResult(RESULT_CANCELED);
                onDispatchFailure();
                return;
            }

            euiccUiIntent.setFlags(Intent.FLAG_ACTIVITY_FORWARD_RESULT);
            startActivity(euiccUiIntent);
        } finally {
            // Since we're using Theme.NO_DISPLAY, we must always finish() at the end of onCreate().
            finish();
        }
    }

    @VisibleForTesting
    @Nullable
    Intent resolveEuiccUiIntent() {
        EuiccManager euiccManager = (EuiccManager) getSystemService(Context.EUICC_SERVICE);
        if (!euiccManager.isEnabled()) {
            Log.w(TAG, "eUICC not enabled");
            return null;
        }

        Intent euiccUiIntent = getEuiccUiIntent();
        if (euiccUiIntent == null) {
            Log.w(TAG, "Unable to handle intent");
            return null;
        }

        revokePermissionFromLuiApps(euiccUiIntent);

        ActivityInfo activityInfo = findBestActivity(euiccUiIntent);
        if (activityInfo == null) {
            Log.w(TAG, "Could not resolve activity for intent: " + euiccUiIntent);
            return null;
        }

        grantDefaultPermissionsToActiveLuiApp(activityInfo);

        euiccUiIntent.setComponent(activityInfo.getComponentName());
        return euiccUiIntent;
    }

    /** Called when dispatch fails. May be overridden to perform some operation here. */
    protected void onDispatchFailure() {
    }

    /**
     * Return an Intent to start the Euicc app's UI for the given intent, or null if given intent
     * cannot be handled.
     */
    @Nullable
    protected Intent getEuiccUiIntent() {
        String action = getIntent().getAction();

        Intent intent = new Intent();
        intent.putExtras(getIntent());
        switch (action) {
            case EuiccManager.ACTION_MANAGE_EMBEDDED_SUBSCRIPTIONS:
                intent.setAction(EuiccService.ACTION_MANAGE_EMBEDDED_SUBSCRIPTIONS);
                break;
            case EuiccManager.ACTION_PROVISION_EMBEDDED_SUBSCRIPTION:
                intent.setAction(EuiccService.ACTION_PROVISION_EMBEDDED_SUBSCRIPTION);
                break;
            default:
                Log.w(TAG, "Unsupported action: " + action);
                return null;
        }

        return intent;
    }

    @VisibleForTesting
    @Nullable
    ActivityInfo findBestActivity(Intent euiccUiIntent) {
        return EuiccConnector.findBestActivity(getPackageManager(), euiccUiIntent);
    }

    /** Grants default permissions to the active LUI app. */
    @VisibleForTesting
    protected void grantDefaultPermissionsToActiveLuiApp(ActivityInfo activityInfo) {
        try {
            mPackageManager.grantDefaultPermissionsToActiveLuiApp(
                    activityInfo.packageName, getUserId());
        } catch (RemoteException e) {
            Log.e(TAG, "Failed to grant permissions to active LUI app.", e);
        }
    }

    /** Cleans up all the packages that shouldn't have permission. */
    @VisibleForTesting
    protected void revokePermissionFromLuiApps(Intent intent) {
        try {
            Set<String> luiApps = getAllLuiAppPackageNames(intent);
            String[] luiAppsArray = luiApps.toArray(new String[luiApps.size()]);
            mPackageManager.revokeDefaultPermissionsFromLuiApps(luiAppsArray, getUserId());
        } catch (RemoteException e) {
            Log.e(TAG, "Failed to revoke LUI app permissions.");
            throw e.rethrowAsRuntimeException();
        }
    }

    @NonNull
    private Set<String> getAllLuiAppPackageNames(Intent intent) {
        List<ResolveInfo> luiPackages =
                getPackageManager().queryIntentServices(intent, EUICC_QUERY_FLAGS);
        HashSet<String> packageNames = new HashSet<>();
        for (ResolveInfo info : luiPackages) {
            if (info.serviceInfo == null) continue;
            packageNames.add(info.serviceInfo.packageName);
        }
        return packageNames;
    }
}
