/*
 * Copyright (C) 2015 The Android Open Source Project
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
package com.android.car.pm;

import android.annotation.Nullable;
import android.content.Context;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.os.Bundle;
import android.util.Log;

import com.android.car.CarLog;

import java.util.ArrayList;
import java.util.List;

/**
 *  Read App meta data and look for Distraction Optimization(DO) tags.
 *  An app can tag a distraction optimized activity to be DO by adding the following meta-data
 *  to that <activity> element:
 *
 *  <pre>{@code
 *  <activity>
 *      <meta-data android:name="distractionOptimized" android:value="true"/>
 *  </activity>
 *  }</pre>
 *
 */
public class CarAppMetadataReader {
    /** Name of the meta-data attribute of the Activity that denotes distraction optimized */
    private static final String DO_METADATA_ATTRIBUTE = "distractionOptimized";

    /**
     * Parses the given package and returns Distraction Optimized information, if present.
     *
     * @return Array of DO activity names in the given package
     */
    @Nullable
    public static String[] findDistractionOptimizedActivities(Context context, String packageName)
            throws NameNotFoundException {
        final PackageManager pm = context.getPackageManager();

        // Check if any of the activities in the package are DO by checking all the
        // <activity> elements.
        PackageInfo pkgInfo =
                pm.getPackageInfo(
                        packageName, PackageManager.GET_ACTIVITIES
                                | PackageManager.GET_META_DATA
                                | PackageManager.MATCH_DIRECT_BOOT_AWARE
                                | PackageManager.MATCH_DIRECT_BOOT_UNAWARE);
        if (pkgInfo == null) {
            return null;
        }

        ActivityInfo[] activities = pkgInfo.activities;
        if (activities == null) {
            if (Log.isLoggable(CarLog.TAG_PACKAGE, Log.DEBUG)) {
                Log.d(CarLog.TAG_PACKAGE, "Null Activities for " + packageName);
            }
            return null;
        }
        List<String> optimizedActivityList = new ArrayList();
        for (ActivityInfo activity : activities) {
            Bundle mData = activity.metaData;
            if (mData != null && mData.getBoolean(DO_METADATA_ATTRIBUTE, false)) {
                if (Log.isLoggable(CarLog.TAG_PACKAGE, Log.DEBUG)) {
                    Log.d(CarLog.TAG_PACKAGE,
                            "DO Activity:" + activity.packageName + "/" + activity.name);
                }
                optimizedActivityList.add(activity.name);
            }
        }
        if (optimizedActivityList.isEmpty()) {
            return null;
        }
        return optimizedActivityList.toArray(new String[optimizedActivityList.size()]);
    }
}
