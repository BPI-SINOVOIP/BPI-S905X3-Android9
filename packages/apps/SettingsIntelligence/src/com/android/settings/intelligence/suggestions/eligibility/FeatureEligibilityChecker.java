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
import android.content.pm.ResolveInfo;
import android.support.annotation.VisibleForTesting;
import android.text.TextUtils;
import android.util.Log;

public class FeatureEligibilityChecker {

    private static final String TAG = "FeatureEligibility";

    /**
     * If defined, only returns this suggestion if the feature is supported.
     */
    @VisibleForTesting
    static final String META_DATA_REQUIRE_FEATURE = "com.android.settings.require_feature";

    public static boolean isEligible(Context context, String id, ResolveInfo info) {
        final String featuresRequired = info.activityInfo.metaData
                .getString(META_DATA_REQUIRE_FEATURE);
        if (featuresRequired != null) {
            for (String feature : featuresRequired.split(",")) {
                if (TextUtils.isEmpty(feature)) {
                    Log.i(TAG, "Found empty substring when parsing required features: "
                            + featuresRequired);
                } else if (!context.getPackageManager().hasSystemFeature(feature)) {
                    Log.i(TAG, id + " requires unavailable feature " + feature);
                    return false;
                }
            }
        }
        return true;
    }
}
