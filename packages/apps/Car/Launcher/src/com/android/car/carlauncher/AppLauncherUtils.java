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

package com.android.car.carlauncher;

import android.annotation.Nullable;
import android.car.CarNotConnectedException;
import android.car.content.pm.CarPackageManager;
import android.content.Context;
import android.content.Intent;
import android.content.pm.LauncherActivityInfo;
import android.content.pm.LauncherApps;
import android.content.pm.PackageManager;
import android.graphics.ColorMatrix;
import android.graphics.ColorMatrixColorFilter;
import android.graphics.drawable.Drawable;
import android.os.Process;
import android.util.Log;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;

/**
 * Util class that contains helper method used by app launcher classes.
 */
class AppLauncherUtils {

    private static final String TAG = "AppLauncherUtils";

    private AppLauncherUtils() {
    }

    /**
     * Comparator for {@link AppMetaData} that sorts the list
     * by the "displayName" property in ascending order.
     */
    static final Comparator<AppMetaData> ALPHABETICAL_COMPARATOR = Comparator
            .comparing(AppMetaData::getDisplayName, String::compareToIgnoreCase);

    /**
     * Helper method that launches the app given the app's AppMetaData.
     *
     * @param app the requesting app's AppMetaData
     */
    static void launchApp(Context context, AppMetaData app) {
        Intent intent =
                context.getPackageManager().getLaunchIntentForPackage(app.getPackageName());
        context.startActivity(intent);
    }

    /**
     * Converts a {@link Drawable} to grey scale.
     *
     * @param drawable the original drawable
     * @return the grey scale drawable
     */
    static Drawable toGrayscale(Drawable drawable) {
        ColorMatrix matrix = new ColorMatrix();
        matrix.setSaturation(0);
        ColorMatrixColorFilter filter = new ColorMatrixColorFilter(matrix);
        // deep copy the original drawable
        Drawable newDrawable = drawable.getConstantState().newDrawable().mutate();
        newDrawable.setColorFilter(filter);
        return newDrawable;
    }

    /**
     * Gets all apps that support launching from launcher in unsorted order.
     *
     * @param launcherApps      The {@link LauncherApps} system service
     * @param carPackageManager The {@link CarPackageManager} system service
     * @param packageManager    The {@link PackageManager} system service
     * @return a list of all apps' metadata
     */
    @Nullable
    static List<AppMetaData> getAllLaunchableApps(
            LauncherApps launcherApps,
            CarPackageManager carPackageManager,
            PackageManager packageManager) {

        if (launcherApps == null || carPackageManager == null || packageManager == null) {
            return null;
        }

        List<AppMetaData> apps = new ArrayList<>();

        Intent intent = new Intent(Intent.ACTION_MAIN, null);
        intent.addCategory(Intent.CATEGORY_LAUNCHER);

        List<LauncherActivityInfo> availableActivities =
                launcherApps.getActivityList(null, Process.myUserHandle());
        for (LauncherActivityInfo info : availableActivities) {
            String packageName = info.getComponentName().getPackageName();
            boolean isDistractionOptimized =
                    isActivityDistractionOptimized(carPackageManager, packageName, info.getName());

            AppMetaData app = new AppMetaData(
                    info.getLabel(),
                    packageName,
                    info.getApplicationInfo().loadIcon(packageManager),
                    isDistractionOptimized);
            apps.add(app);
        }
        return apps;
    }

    /**
     * Gets if an activity is distraction optimized.
     *
     * @param carPackageManager The {@link CarPackageManager} system service
     * @param packageName       The package name of the app
     * @param activityName      The requested activity name
     * @return true if the supplied activity is distraction optimized
     */
    static boolean isActivityDistractionOptimized(
            CarPackageManager carPackageManager, String packageName, String activityName) {
        boolean isDistractionOptimized = false;
        // try getting distraction optimization info
        try {
            if (carPackageManager != null) {
                isDistractionOptimized =
                        carPackageManager.isActivityDistractionOptimized(packageName, activityName);
            }
        } catch (CarNotConnectedException e) {
            Log.e(TAG, "Car not connected when getting DO info", e);
        }
        return isDistractionOptimized;
    }
}
