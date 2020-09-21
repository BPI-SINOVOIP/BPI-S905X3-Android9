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

package com.android.settings.applications.appinfo;

import static com.android.internal.logging.nano.MetricsProto.MetricsEvent.ACTION_OPEN_APP_SETTING;

import android.content.Context;
import android.content.Intent;
import android.content.pm.ResolveInfo;
import android.support.v7.preference.Preference;
import android.text.TextUtils;

import com.android.settings.overlay.FeatureFactory;

public class AppSettingPreferenceController extends AppInfoPreferenceControllerBase {

    private String mPackageName;

    public AppSettingPreferenceController(Context context, String preferenceKey) {
        super(context, preferenceKey);
    }

    public AppSettingPreferenceController setPackageName(String packageName) {
        mPackageName = packageName;
        return this;
    }

    @Override
    public int getAvailabilityStatus() {
        if (TextUtils.isEmpty(mPackageName) || mParent == null) {
            return CONDITIONALLY_UNAVAILABLE;
        }
        final Intent intent = resolveIntent(
                new Intent(Intent.ACTION_APPLICATION_PREFERENCES).setPackage(mPackageName));
        return intent != null ? AVAILABLE : CONDITIONALLY_UNAVAILABLE;
    }

    @Override
    public boolean handlePreferenceTreeClick(Preference preference) {
        if (!TextUtils.equals(preference.getKey(), getPreferenceKey())) {
            return false;
        }
        final Intent intent = resolveIntent(
                new Intent(Intent.ACTION_APPLICATION_PREFERENCES).setPackage(mPackageName));
        if (intent == null) {
            return false;
        }
        FeatureFactory.getFactory(mContext).getMetricsFeatureProvider()
                .actionWithSource(mContext, mParent.getMetricsCategory(),
                        ACTION_OPEN_APP_SETTING);
        mContext.startActivity(intent);
        return true;
    }

    private Intent resolveIntent(Intent i) {
        ResolveInfo result = mContext.getPackageManager().resolveActivity(i, 0);
        if (result != null) {
            return new Intent(i.getAction())
                    .setClassName(result.activityInfo.packageName, result.activityInfo.name);
        }
        return null;
    }
}
