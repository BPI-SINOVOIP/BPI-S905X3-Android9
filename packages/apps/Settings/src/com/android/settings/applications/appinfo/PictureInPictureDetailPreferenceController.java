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

package com.android.settings.applications.appinfo;

import android.content.Context;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.os.UserHandle;
import android.support.annotation.VisibleForTesting;
import android.support.v7.preference.Preference;
import android.util.Log;

import com.android.settings.SettingsPreferenceFragment;

public class PictureInPictureDetailPreferenceController extends AppInfoPreferenceControllerBase {

    private static final String TAG = "PicInPicDetailControl";

    private final PackageManager mPackageManager;

    private String mPackageName;

    public PictureInPictureDetailPreferenceController(Context context, String key) {
        super(context, key);
        mPackageManager = context.getPackageManager();
    }

    @Override
    public int getAvailabilityStatus() {
        return hasPictureInPictureActivites() ? AVAILABLE : DISABLED_FOR_USER;
    }

    @Override
    public void updateState(Preference preference) {
        preference.setSummary(getPreferenceSummary());
    }

    @Override
    protected Class<? extends SettingsPreferenceFragment> getDetailFragmentClass() {
        return PictureInPictureDetails.class;
    }

    @VisibleForTesting
    boolean hasPictureInPictureActivites() {
        // Get the package info with the activities
        PackageInfo packageInfoWithActivities = null;
        try {
            packageInfoWithActivities = mPackageManager.getPackageInfoAsUser(mPackageName,
                    PackageManager.GET_ACTIVITIES, UserHandle.myUserId());
        } catch (PackageManager.NameNotFoundException e) {
            Log.e(TAG, "Exception while retrieving the package info of " + mPackageName, e);
        }

        return packageInfoWithActivities != null
                && PictureInPictureSettings.checkPackageHasPictureInPictureActivities(
                packageInfoWithActivities.packageName,
                packageInfoWithActivities.activities);
    }

    @VisibleForTesting
    int getPreferenceSummary() {
        return PictureInPictureDetails.getPreferenceSummary(mContext,
                mParent.getPackageInfo().applicationInfo.uid, mPackageName);
    }

    public void setPackageName(String packageName) {
        mPackageName = packageName;
    }
}
