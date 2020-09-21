/*
 * Copyright 2015 The Android Open Source Project
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
import android.content.Intent;
import android.media.tv.TvInputInfo;
import android.os.Build;
import android.util.ArraySet;
import android.util.Log;
import com.android.tv.common.BuildConfig;
import com.android.tv.common.CommonConstants;
import com.android.tv.common.actions.InputSetupActionUtils;
import com.android.tv.common.experiments.Experiments;
import java.io.File;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;
import java.util.Set;

/** Util class for common use in TV app and inputs. */
@SuppressWarnings("AndroidApiChecker") // TODO(b/32513850) remove when error prone is updated
public final class CommonUtils {
    private static final String TAG = "CommonUtils";
    private static final ThreadLocal<SimpleDateFormat> ISO_8601 =
            new ThreadLocal() {
                private final SimpleDateFormat value =
                        new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ssZ", Locale.US);

                @Override
                protected SimpleDateFormat initialValue() {
                    return value;
                }
            };
    // Hardcoded list for known bundled inputs not written by OEM/SOCs.
    // Bundled (system) inputs not in the list will get the high priority
    // so they and their channels come first in the UI.
    private static final Set<String> BUNDLED_PACKAGE_SET = new ArraySet<>();

    static {
        BUNDLED_PACKAGE_SET.add("com.android.tv");
    }

    private static Boolean sRunningInTest;

    private CommonUtils() {}

    /**
     * Returns an intent to start the setup activity for the TV input using {@link
     * InputSetupActionUtils#INTENT_ACTION_INPUT_SETUP}.
     */
    public static Intent createSetupIntent(Intent originalSetupIntent, String inputId) {
        if (originalSetupIntent == null) {
            return null;
        }
        Intent setupIntent = new Intent(originalSetupIntent);
        if (!InputSetupActionUtils.hasInputSetupAction(originalSetupIntent)) {
            Intent intentContainer = new Intent(InputSetupActionUtils.INTENT_ACTION_INPUT_SETUP);
            intentContainer.putExtra(InputSetupActionUtils.EXTRA_SETUP_INTENT, originalSetupIntent);
            intentContainer.putExtra(InputSetupActionUtils.EXTRA_INPUT_ID, inputId);
            setupIntent = intentContainer;
        }
        return setupIntent;
    }

    /**
     * Returns an intent to start the setup activity for this TV input using {@link
     * InputSetupActionUtils#INTENT_ACTION_INPUT_SETUP}.
     */
    public static Intent createSetupIntent(TvInputInfo input) {
        return createSetupIntent(input.createSetupIntent(), input.getId());
    }

    /**
     * Checks if this application is running in tests.
     *
     * <p>{@link android.app.ActivityManager#isRunningInTestHarness} doesn't return {@code true} for
     * the usual devices even the application is running in tests. We need to figure it out by
     * checking whether the class in tv-tests-common module can be loaded or not.
     */
    public static synchronized boolean isRunningInTest() {
        if (sRunningInTest == null) {
            try {
                Class.forName("com.android.tv.testing.utils.Utils");
                Log.i(
                        TAG,
                        "Assumed to be running in a test because"
                                + " com.android.tv.testing.utils.Utils is found");
                sRunningInTest = true;
            } catch (ClassNotFoundException e) {
                sRunningInTest = false;
            }
        }
        return sRunningInTest;
    }

    /** Checks whether a given package is in our bundled package set. */
    public static boolean isInBundledPackageSet(String packageName) {
        return BUNDLED_PACKAGE_SET.contains(packageName);
    }

    /** Checks whether a given input is a bundled input. */
    public static boolean isBundledInput(String inputId) {
        for (String prefix : BUNDLED_PACKAGE_SET) {
            if (inputId.startsWith(prefix + "/")) {
                return true;
            }
        }
        return false;
    }

    /** Returns true if the application is packaged with Live TV. */
    public static boolean isPackagedWithLiveChannels(Context context) {
        return (CommonConstants.BASE_PACKAGE.equals(context.getPackageName()));
    }

    /** Returns true if the current user is a developer. */
    public static boolean isDeveloper() {
        return BuildConfig.ENG || Experiments.ENABLE_DEVELOPER_FEATURES.get();
    }

    /** Converts time in milliseconds to a ISO 8061 string. */
    public static String toIsoDateTimeString(long timeMillis) {
        return ISO_8601.get().format(new Date(timeMillis));
    }

    /** Deletes a file or a directory. */
    public static void deleteDirOrFile(File fileOrDirectory) {
        if (fileOrDirectory.isDirectory()) {
            for (File child : fileOrDirectory.listFiles()) {
                deleteDirOrFile(child);
            }
        }
        fileOrDirectory.delete();
    }

    public static boolean isRoboTest() {
        return "robolectric".equals(Build.FINGERPRINT);
    }
}
