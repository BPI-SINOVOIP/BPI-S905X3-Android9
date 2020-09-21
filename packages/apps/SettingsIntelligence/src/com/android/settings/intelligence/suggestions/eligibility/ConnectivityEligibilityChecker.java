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
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.support.annotation.VisibleForTesting;
import android.util.Log;

public class ConnectivityEligibilityChecker {

    /**
     * If defined, only display this optional step if a connection is available.
     */
    @VisibleForTesting
    static final String META_DATA_IS_CONNECTION_REQUIRED =
            "com.android.settings.require_connection";
    private static final String TAG = "ConnectivityEligibility";

    public static boolean isEligible(Context context, String id, ResolveInfo info) {
        final boolean isConnectionRequired =
                info.activityInfo.metaData.getBoolean(META_DATA_IS_CONNECTION_REQUIRED);
        if (!isConnectionRequired) {
            return true;
        }
        final ConnectivityManager cm =
                (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkInfo netInfo = cm.getActiveNetworkInfo();
        boolean satisfiesConnectivity = netInfo != null && netInfo.isConnectedOrConnecting();
        if (!satisfiesConnectivity) {
            Log.i(TAG, id + " is missing required connection.");
        }
        return satisfiesConnectivity;
    }
}
