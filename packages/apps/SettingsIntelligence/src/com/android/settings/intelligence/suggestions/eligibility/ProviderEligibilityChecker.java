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

package com.android.settings.intelligence.suggestions.eligibility;

import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.content.res.Resources;
import android.util.Log;

public class ProviderEligibilityChecker {
    /**
     * If defined and not true, do not should optional step.
     */
    private static final String META_DATA_IS_SUPPORTED = "com.android.settings.is_supported";
    private static final String TAG = "ProviderEligibility";

    public static boolean isEligible(Context context, String id, ResolveInfo info) {
        return isSystemApp(id, info)
                && isEnabledInMetadata(context, id, info);
    }

    private static boolean isSystemApp(String id, ResolveInfo info) {
        final boolean isSystemApp = info != null
                && info.activityInfo != null
                && info.activityInfo.applicationInfo != null
                && (info.activityInfo.applicationInfo.flags
                & ApplicationInfo.FLAG_SYSTEM) != 0;
        if (!isSystemApp) {
            Log.i(TAG, id + " is not system app, not eligible for suggestion");
        }
        return isSystemApp;
    }

    private static boolean isEnabledInMetadata(Context context, String id, ResolveInfo info) {
        final int isSupportedResource = info.activityInfo.metaData.getInt(META_DATA_IS_SUPPORTED);
        try {
            final Resources res = context.getPackageManager()
                    .getResourcesForApplication(info.activityInfo.applicationInfo);
            boolean isSupported =
                    isSupportedResource != 0 ? res.getBoolean(isSupportedResource) : true;
            if (!isSupported) {
                Log.i(TAG, id + " requires unsupported resource " + isSupportedResource);
            }
            return isSupported;
        } catch (PackageManager.NameNotFoundException e) {
            Log.w(TAG, "Cannot find resources for " + id, e);
            return false;
        } catch (Resources.NotFoundException e) {
            Log.w(TAG, "Cannot find resources for " + id, e);
            return false;
        }
    }
}
