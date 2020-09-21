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

package com.android.tv.settings.autofill;

import android.Manifest;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.content.pm.ServiceInfo;
import android.provider.Settings;
import android.service.autofill.AutofillService;
import android.service.autofill.AutofillServiceInfo;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.text.TextUtils;
import android.util.Log;

import com.android.settingslib.applications.DefaultAppInfo;
import com.android.settingslib.wrapper.PackageManagerWrapper;

import java.util.ArrayList;
import java.util.List;

/**
 * Helper class for autofill related settings.
 */
public class AutofillHelper {

    private static final String TAG = "AutofillHelper";
    static final Intent AUTOFILL_PROBE = new Intent(AutofillService.SERVICE_INTERFACE);

    /**
     * Get a list of autofill services.
     */
    @NonNull
    public static List<DefaultAppInfo> getAutofillCandidates(@NonNull Context context,
            @NonNull PackageManagerWrapper pm, int myUserId) {
        final List<DefaultAppInfo> candidates = new ArrayList<>();
        final List<ResolveInfo> resolveInfos = pm.queryIntentServices(
                AUTOFILL_PROBE, PackageManager.GET_META_DATA);
        for (ResolveInfo info : resolveInfos) {
            final String permission = info.serviceInfo.permission;
            if (Manifest.permission.BIND_AUTOFILL_SERVICE.equals(permission)
                    || Manifest.permission.BIND_AUTOFILL.equals(permission)) {
                candidates.add(new DefaultAppInfo(context, pm, myUserId, new ComponentName(
                        info.serviceInfo.packageName, info.serviceInfo.name)));
            }
        }
        return candidates;
    }

    /**
     * Get flattened ComponentName of current autofill service
     */
    @Nullable
    public static String getCurrentAutofill(@NonNull Context context) {
        return Settings.Secure.getString(context.getContentResolver(),
                Settings.Secure.AUTOFILL_SERVICE);
    }

    /**
     * Get flattened ComponentName of current autofill service
     */
    @Nullable
    public static ComponentName getCurrentAutofillAsComponentName(@NonNull Context context) {
        String flattenedName = getCurrentAutofill(context);
        return TextUtils.isEmpty(flattenedName)
                ? null : ComponentName.unflattenFromString(flattenedName);
    }

    /**
     * Find the current autofill service from the list.
     */
    @Nullable
    public static DefaultAppInfo getCurrentAutofill(@NonNull Context context,
            @NonNull List<DefaultAppInfo> candidates) {
        final ComponentName name = getCurrentAutofillAsComponentName(context);
        for (int i = 0; i < candidates.size(); i++) {
            DefaultAppInfo appInfo = candidates.get(i);
            if ((name == null && appInfo.componentName == null)
                    || (name != null && name.equals(appInfo.componentName))) {
                return appInfo;
            }
        }
        return null;
    }

    /**
     * Get the Intent for settings activity of autofill service. Returns null if does not exist.
     */
    @Nullable
    public static Intent getAutofillSettingsIntent(@NonNull Context context,
            @NonNull PackageManagerWrapper pm, @Nullable DefaultAppInfo appInfo) {
        if (appInfo == null || appInfo.componentName == null) {
            return null;
        }
        String plattenString = appInfo.componentName.flattenToString();
        final List<ResolveInfo> resolveInfos = pm.queryIntentServices(
                AUTOFILL_PROBE, PackageManager.GET_META_DATA);

        for (ResolveInfo resolveInfo : resolveInfos) {
            final ServiceInfo serviceInfo = resolveInfo.serviceInfo;
            final String flattenKey = new ComponentName(
                    serviceInfo.packageName, serviceInfo.name).flattenToString();
            if (TextUtils.equals(plattenString, flattenKey)) {
                final String settingsActivity;
                try {
                    settingsActivity = new AutofillServiceInfo(context, serviceInfo)
                            .getSettingsActivity();
                } catch (SecurityException e) {
                    // Service does not declare the proper permission, ignore it.
                    Log.w(TAG, "Error getting info for " + serviceInfo + ": " + e);
                    return null;
                }
                if (TextUtils.isEmpty(settingsActivity)) {
                    return null;
                }
                return new Intent(Intent.ACTION_MAIN).setComponent(
                        new ComponentName(serviceInfo.packageName, settingsActivity));
            }
        }

        return null;
    }

    /**
     * Set autofill service and write to settings.
     */
    public static void setCurrentAutofill(@NonNull Context context, @Nullable String id) {
        if (id == null) {
            throw new IllegalArgumentException("Null ID");
        }
        Settings.Secure.putString(context.getContentResolver(), Settings.Secure.AUTOFILL_SERVICE,
                id);
    }

}
