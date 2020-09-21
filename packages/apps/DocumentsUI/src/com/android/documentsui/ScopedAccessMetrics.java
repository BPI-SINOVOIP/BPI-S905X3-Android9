/*
 * Copyright (C) 2016 The Android Open Source Project
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

package com.android.documentsui;

import static android.os.Environment.STANDARD_DIRECTORIES;

import static com.android.documentsui.base.SharedMinimal.DEBUG;
import static com.android.documentsui.base.SharedMinimal.DIRECTORY_ROOT;

import android.annotation.IntDef;
import android.annotation.StringDef;
import android.app.Activity;
import android.content.Context;
import android.util.Log;

import com.android.internal.logging.MetricsLogger;
import com.android.internal.logging.nano.MetricsProto.MetricsEvent;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * Methods for logging scoped directory access metrics.
 */
public final class ScopedAccessMetrics {
    private static final String TAG = "ScopedAccessMetrics";

    // Types for logInvalidScopedAccessRequest
    public static final String SCOPED_DIRECTORY_ACCESS_INVALID_ARGUMENTS =
            "docsui_scoped_directory_access_invalid_args";
    public static final String SCOPED_DIRECTORY_ACCESS_INVALID_DIRECTORY =
            "docsui_scoped_directory_access_invalid_dir";
    public static final String SCOPED_DIRECTORY_ACCESS_ERROR =
            "docsui_scoped_directory_access_error";

    @StringDef(value = {
            SCOPED_DIRECTORY_ACCESS_INVALID_ARGUMENTS,
            SCOPED_DIRECTORY_ACCESS_INVALID_DIRECTORY,
            SCOPED_DIRECTORY_ACCESS_ERROR
    })
    @Retention(RetentionPolicy.SOURCE)
    public @interface InvalidScopedAccess{}

    public static void logInvalidScopedAccessRequest(Context context,
            @InvalidScopedAccess String type) {
        switch (type) {
            case SCOPED_DIRECTORY_ACCESS_INVALID_ARGUMENTS:
            case SCOPED_DIRECTORY_ACCESS_INVALID_DIRECTORY:
            case SCOPED_DIRECTORY_ACCESS_ERROR:
                logCount(context, type);
                break;
            default:
                Log.wtf(TAG, "invalid InvalidScopedAccess: " + type);
        }
    }

    // Types for logValidScopedAccessRequest
    public static final int SCOPED_DIRECTORY_ACCESS_ALREADY_GRANTED = 0;
    public static final int SCOPED_DIRECTORY_ACCESS_GRANTED = 1;
    public static final int SCOPED_DIRECTORY_ACCESS_DENIED = 2;
    public static final int SCOPED_DIRECTORY_ACCESS_DENIED_AND_PERSIST = 3;
    public static final int SCOPED_DIRECTORY_ACCESS_ALREADY_DENIED = 4;

    @IntDef(flag = true, value = {
            SCOPED_DIRECTORY_ACCESS_ALREADY_GRANTED,
            SCOPED_DIRECTORY_ACCESS_GRANTED,
            SCOPED_DIRECTORY_ACCESS_DENIED,
            SCOPED_DIRECTORY_ACCESS_DENIED_AND_PERSIST,
            SCOPED_DIRECTORY_ACCESS_ALREADY_DENIED
    })
    @Retention(RetentionPolicy.SOURCE)
    public @interface ScopedAccessGrant {}

    public static void logValidScopedAccessRequest(Activity activity, String directory,
            @ScopedAccessGrant int type) {
        int index = -1;
        if (DIRECTORY_ROOT.equals(directory)) {
            index = -2;
        } else {
            for (int i = 0; i < STANDARD_DIRECTORIES.length; i++) {
                if (STANDARD_DIRECTORIES[i].equals(directory)) {
                    index = i;
                    break;
                }
            }
        }
        final String packageName = activity.getCallingPackage();
        switch (type) {
            case SCOPED_DIRECTORY_ACCESS_ALREADY_GRANTED:
                MetricsLogger.action(activity, MetricsEvent
                        .ACTION_SCOPED_DIRECTORY_ACCESS_ALREADY_GRANTED_BY_PACKAGE, packageName);
                MetricsLogger.action(activity, MetricsEvent
                        .ACTION_SCOPED_DIRECTORY_ACCESS_ALREADY_GRANTED_BY_FOLDER, index);
                break;
            case SCOPED_DIRECTORY_ACCESS_GRANTED:
                MetricsLogger.action(activity, MetricsEvent
                        .ACTION_SCOPED_DIRECTORY_ACCESS_GRANTED_BY_PACKAGE, packageName);
                MetricsLogger.action(activity, MetricsEvent
                        .ACTION_SCOPED_DIRECTORY_ACCESS_GRANTED_BY_FOLDER, index);
                break;
            case SCOPED_DIRECTORY_ACCESS_DENIED:
                MetricsLogger.action(activity, MetricsEvent
                        .ACTION_SCOPED_DIRECTORY_ACCESS_DENIED_BY_PACKAGE, packageName);
                MetricsLogger.action(activity, MetricsEvent
                        .ACTION_SCOPED_DIRECTORY_ACCESS_DENIED_BY_FOLDER, index);
                break;
            case SCOPED_DIRECTORY_ACCESS_DENIED_AND_PERSIST:
                MetricsLogger.action(activity, MetricsEvent
                        .ACTION_SCOPED_DIRECTORY_ACCESS_DENIED_AND_PERSIST_BY_PACKAGE, packageName);
                MetricsLogger.action(activity, MetricsEvent
                        .ACTION_SCOPED_DIRECTORY_ACCESS_DENIED_AND_PERSIST_BY_FOLDER, index);
                break;
            case SCOPED_DIRECTORY_ACCESS_ALREADY_DENIED:
                MetricsLogger.action(activity, MetricsEvent
                        .ACTION_SCOPED_DIRECTORY_ACCESS_ALREADY_DENIED_BY_PACKAGE, packageName);
                MetricsLogger.action(activity, MetricsEvent
                        .ACTION_SCOPED_DIRECTORY_ACCESS_ALREADY_DENIED_BY_FOLDER, index);
                break;
            default:
                Log.wtf(TAG, "invalid ScopedAccessGrant: " + type);
        }
    }

    /**
     * Internal method for making a MetricsLogger.count call. Increments the given counter by 1.
     *
     * @param context
     * @param name The counter to increment.
     */
    private static void logCount(Context context, String name) {
        if (DEBUG) Log.d(TAG, name + ": " + 1);
        MetricsLogger.count(context, name, 1);
    }
}
