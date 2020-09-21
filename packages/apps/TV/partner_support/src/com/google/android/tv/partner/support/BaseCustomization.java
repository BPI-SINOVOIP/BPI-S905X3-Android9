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
 * limitations under the License
 */

package com.google.android.tv.partner.support;

import android.annotation.TargetApi;
import android.content.Context;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.os.Build;
import android.util.Log;
import java.util.Arrays;
import java.util.List;
import java.util.Optional;

/**
 * Abstract class for TV Customization support.
 *
 * <p>Implement customization resources as needed in a child class.
 */
@TargetApi(Build.VERSION_CODES.N)
@SuppressWarnings("AndroidApiChecker") // TODO(b/32513850) remove when error prone is updated
public class BaseCustomization {
    private static final boolean DEBUG = false;
    private static final String TAG = "BaseCustomization";

    // TODO move common aosp library

    // TODO cache resource if performance suffers.

    private static final String RES_TYPE_BOOLEAN = "bool";

    private final String packageName;

    protected BaseCustomization(Context context, String[] permissions) {
        packageName = getFirstPackageNameWithPermissions(context, permissions);
    }

    public final String getPackageName() {
        return packageName;
    }

    private static String getFirstPackageNameWithPermissions(
            Context context, String[] permissions) {

        List<PackageInfo> packageInfos =
                context.getPackageManager().getPackagesHoldingPermissions(permissions, 0);
        if (DEBUG) {
            Log.d(TAG, "These packages have " + Arrays.toString(permissions) + ": " + packageInfos);
        }
        return packageInfos == null || packageInfos.size() == 0
                ? ""
                : packageInfos.get(0).packageName;
    }

    private static Resources getResourceForPackage(Context context, String packageName)
            throws PackageManager.NameNotFoundException {
        return context.getPackageManager().getResourcesForApplication(packageName);
    }

    public final Optional<Boolean> getBooleanResource(Context context, String resourceName) {
        if (resourceName.isEmpty()) {
            return Optional.empty();
        }
        try {
            Resources res = getResourceForPackage(context, packageName);
            int resId =
                    res == null
                            ? 0
                            : res.getIdentifier(resourceName, RES_TYPE_BOOLEAN, packageName);
            if (DEBUG) {
                Log.d(TAG, "Boolean resource  " + resourceName + " has  " + resId);
            }
            return resId == 0 ? Optional.empty() : Optional.of(res.getBoolean(resId));
        } catch (PackageManager.NameNotFoundException e) {
            return Optional.empty();
        }
    }
}
