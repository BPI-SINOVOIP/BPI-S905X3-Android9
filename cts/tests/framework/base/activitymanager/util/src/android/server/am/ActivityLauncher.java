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
 * limitations under the License
 */

package android.server.am;

import static android.content.Intent.FLAG_ACTIVITY_LAUNCH_ADJACENT;
import static android.content.Intent.FLAG_ACTIVITY_MULTIPLE_TASK;
import static android.content.Intent.FLAG_ACTIVITY_NEW_TASK;
import static android.content.Intent.FLAG_ACTIVITY_REORDER_TO_FRONT;
import static android.server.am.Components.TEST_ACTIVITY;

import android.app.ActivityOptions;
import android.app.PendingIntent;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.text.TextUtils;
import android.util.Log;

/** Utility class which contains common code for launching activities. */
public class ActivityLauncher {
    private static final String TAG = ActivityLauncher.class.getSimpleName();

    /** Key for boolean extra, indicates whether it should launch an activity. */
    public static final String KEY_LAUNCH_ACTIVITY = "launch_activity";
    /**
     * Key for boolean extra, indicates whether it the activity should be launched to side in
     * split-screen.
     */
    public static final String KEY_LAUNCH_TO_SIDE = "launch_to_the_side";
    /**
     * Key for boolean extra, indicates if launch intent should include random data to be different
     * from other launch intents.
     */
    public static final String KEY_RANDOM_DATA = "random_data";
    /**
     * Key for boolean extra, indicates if launch intent should have
     * {@link Intent#FLAG_ACTIVITY_NEW_TASK}.
     */
    public static final String KEY_NEW_TASK = "new_task";
    /**
     * Key for boolean extra, indicates if launch intent should have
     * {@link Intent#FLAG_ACTIVITY_MULTIPLE_TASK}.
     */
    public static final String KEY_MULTIPLE_TASK = "multiple_task";
    /**
     * Key for boolean extra, indicates if launch intent should have
     * {@link Intent#FLAG_ACTIVITY_REORDER_TO_FRONT}.
     */
    public static final String KEY_REORDER_TO_FRONT = "reorder_to_front";
    /**
     * Key for string extra with string representation of target component.
     */
    public static final String KEY_TARGET_COMPONENT = "target_component";
    /**
     * Key for int extra with target display id where the activity should be launched. Adding this
     * automatically applies {@link Intent#FLAG_ACTIVITY_NEW_TASK} and
     * {@link Intent#FLAG_ACTIVITY_MULTIPLE_TASK} to the intent.
     */
    public static final String KEY_DISPLAY_ID = "display_id";
    /**
     * Key for boolean extra, indicates if launch should be done from application context of the one
     * passed in {@link #launchActivityFromExtras(Context, Bundle)}.
     */
    public static final String KEY_USE_APPLICATION_CONTEXT = "use_application_context";
    /**
     * Key for boolean extra, indicates if instrumentation context will be used for launch. This
     * means that {@link PendingIntent} should be used instead of a regular one, because application
     * switch will not be allowed otherwise.
     */
    public static final String KEY_USE_INSTRUMENTATION = "use_instrumentation";
    /**
     * Key for boolean extra, indicates if any exceptions thrown during launch other then
     * {@link SecurityException} should be suppressed. A {@link SecurityException} is never thrown,
     * it's always written to logs.
     */
    public static final String KEY_SUPPRESS_EXCEPTIONS = "suppress_exceptions";


    /** Perform an activity launch configured by provided extras. */
    public static void launchActivityFromExtras(final Context context, Bundle extras) {
        if (extras == null || !extras.getBoolean(KEY_LAUNCH_ACTIVITY)) {
            return;
        }

        Log.i(TAG, "launchActivityFromExtras: extras=" + extras);

        final String targetComponent = extras.getString(KEY_TARGET_COMPONENT);
        final Intent newIntent = new Intent().setComponent(TextUtils.isEmpty(targetComponent)
                ? TEST_ACTIVITY : ComponentName.unflattenFromString(targetComponent));

        if (extras.getBoolean(KEY_LAUNCH_TO_SIDE)) {
            newIntent.addFlags(FLAG_ACTIVITY_NEW_TASK | FLAG_ACTIVITY_LAUNCH_ADJACENT);
            if (extras.getBoolean(KEY_RANDOM_DATA)) {
                final Uri data = new Uri.Builder()
                        .path(String.valueOf(System.currentTimeMillis()))
                        .build();
                newIntent.setData(data);
            }
        }
        if (extras.getBoolean(KEY_MULTIPLE_TASK)) {
            newIntent.addFlags(FLAG_ACTIVITY_MULTIPLE_TASK);
        }
        if (extras.getBoolean(KEY_NEW_TASK)) {
            newIntent.addFlags(FLAG_ACTIVITY_NEW_TASK);
        }

        if (extras.getBoolean(KEY_REORDER_TO_FRONT)) {
            newIntent.addFlags(FLAG_ACTIVITY_REORDER_TO_FRONT);
        }

        ActivityOptions options = null;
        final int displayId = extras.getInt(KEY_DISPLAY_ID, -1);
        if (displayId != -1) {
            options = ActivityOptions.makeBasic();
            options.setLaunchDisplayId(displayId);
            newIntent.addFlags(FLAG_ACTIVITY_NEW_TASK | FLAG_ACTIVITY_MULTIPLE_TASK);
        }
        final Bundle optionsBundle = options != null ? options.toBundle() : null;

        final Context launchContext = extras.getBoolean(KEY_USE_APPLICATION_CONTEXT) ?
                context.getApplicationContext() : context;

        try {
            if (extras.getBoolean(KEY_USE_INSTRUMENTATION)) {
                // Using PendingIntent for Instrumentation launches, because otherwise we won't
                // be allowed to switch the current activity with ours with different uid.
                // android.permission.STOP_APP_SWITCHES is needed to do this directly.
                final PendingIntent pendingIntent = PendingIntent.getActivity(launchContext, 0,
                        newIntent, 0, optionsBundle);
                pendingIntent.send();
            } else {
                launchContext.startActivity(newIntent, optionsBundle);
            }
        } catch (SecurityException e) {
            Log.e(TAG, "SecurityException launching activity");
        } catch (PendingIntent.CanceledException e) {
            if (extras.getBoolean(KEY_SUPPRESS_EXCEPTIONS)) {
                Log.e(TAG, "Exception launching activity with pending intent");
            } else {
                throw new RuntimeException(e);
            }
            Log.e(TAG, "SecurityException launching activity");
        } catch (Exception e) {
            if (extras.getBoolean(KEY_SUPPRESS_EXCEPTIONS)) {
                Log.e(TAG, "Exception launching activity");
            } else {
                throw e;
            }
        }
    }
}
