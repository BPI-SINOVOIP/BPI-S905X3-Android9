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
package com.android.tv.common.util;

import android.content.Context;
import android.content.pm.PackageManager;

/** Util class to handle permissions. */
public class PermissionUtils {
    /** Permission to read the TV listings. */
    public static final String PERMISSION_READ_TV_LISTINGS = "android.permission.READ_TV_LISTINGS";

    private static Boolean sHasAccessAllEpgPermission;
    private static Boolean sHasAccessWatchedHistoryPermission;
    private static Boolean sHasModifyParentalControlsPermission;

    public static boolean hasAccessAllEpg(Context context) {
        if (sHasAccessAllEpgPermission == null) {
            sHasAccessAllEpgPermission =
                    context.checkSelfPermission(
                                    "com.android.providers.tv.permission.ACCESS_ALL_EPG_DATA")
                            == PackageManager.PERMISSION_GRANTED;
        }
        return sHasAccessAllEpgPermission;
    }

    public static boolean hasAccessWatchedHistory(Context context) {
        if (sHasAccessWatchedHistoryPermission == null) {
            sHasAccessWatchedHistoryPermission =
                    context.checkSelfPermission(
                                    "com.android.providers.tv.permission.ACCESS_WATCHED_PROGRAMS")
                            == PackageManager.PERMISSION_GRANTED;
        }
        return sHasAccessWatchedHistoryPermission;
    }

    public static boolean hasModifyParentalControls(Context context) {
        if (sHasModifyParentalControlsPermission == null) {
            sHasModifyParentalControlsPermission =
                    context.checkSelfPermission("android.permission.MODIFY_PARENTAL_CONTROLS")
                            == PackageManager.PERMISSION_GRANTED;
        }
        return sHasModifyParentalControlsPermission;
    }

    public static boolean hasReadTvListings(Context context) {
        return context.checkSelfPermission(PERMISSION_READ_TV_LISTINGS)
                == PackageManager.PERMISSION_GRANTED;
    }

    public static boolean hasInternet(Context context) {
        return context.checkSelfPermission("android.permission.INTERNET")
                == PackageManager.PERMISSION_GRANTED;
    }
}
