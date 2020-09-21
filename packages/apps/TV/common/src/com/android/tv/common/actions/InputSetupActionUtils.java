/*
 * Copyright 2018 The Android Open Source Project
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

package com.android.tv.common.actions;

import android.content.Intent;
import android.media.tv.TvInputInfo;
import android.os.Bundle;
import android.support.annotation.Nullable;

/** Constants and static utilities for the Input Setup Action. */
public class InputSetupActionUtils {

    /**
     * An intent action to launch setup activity of a TV input.
     *
     * <p>The intent should include TV input ID in the value of {@link #EXTRA_INPUT_ID}. Optionally,
     * given the value of {@link #EXTRA_ACTIVITY_AFTER_COMPLETION}, the activity will be launched
     * after the setup activity successfully finishes.
     */
    public static final String INTENT_ACTION_INPUT_SETUP =
            "com.android.tv.action.LAUNCH_INPUT_SETUP";
    /**
     * A constant of the key to indicate a TV input ID for the intent action {@link
     * #INTENT_ACTION_INPUT_SETUP}.
     *
     * <p>Value type: String
     */
    public static final String EXTRA_INPUT_ID = TvInputInfo.EXTRA_INPUT_ID;
    /**
     * A constant of the key for intent to launch actual TV input setup activity used with {@link
     * #INTENT_ACTION_INPUT_SETUP}.
     *
     * <p>Value type: Intent (Parcelable)
     */
    public static final String EXTRA_SETUP_INTENT = "com.android.tv.extra.SETUP_INTENT";
    /**
     * A constant of the key to indicate an Activity launch intent for the intent action {@link
     * #INTENT_ACTION_INPUT_SETUP}.
     *
     * <p>Value type: Intent (Parcelable)
     */
    public static final String EXTRA_ACTIVITY_AFTER_COMPLETION =
            "com.android.tv.intent.extra.ACTIVITY_AFTER_COMPLETION";
    /**
     * An intent action to launch setup activity of a TV input.
     *
     * <p>The intent should include TV input ID in the value of {@link #EXTRA_INPUT_ID}. Optionally,
     * given the value of {@link #EXTRA_GOOGLE_ACTIVITY_AFTER_COMPLETION}, the activity will be
     * launched after the setup activity successfully finishes.
     *
     * <p>Value type: Intent (Parcelable)
     *
     * @deprecated Use {@link #INTENT_ACTION_INPUT_SETUP} instead
     */
    @Deprecated
    private static final String INTENT_GOOGLE_ACTION_INPUT_SETUP =
            "com.google.android.tv.action.LAUNCH_INPUT_SETUP";
    /**
     * A Google specific constant of the key for intent to launch actual TV input setup activity
     * used with {@link #INTENT_ACTION_INPUT_SETUP}.
     *
     * <p>Value type: Intent (Parcelable)
     *
     * @deprecated Use {@link #EXTRA_SETUP_INTENT} instead
     */
    @Deprecated
    private static final String EXTRA_GOOGLE_SETUP_INTENT =
            "com.google.android.tv.extra.SETUP_INTENT";
    /**
     * A Google specific constant of the key to indicate an Activity launch intent for the intent
     * action {@link #INTENT_ACTION_INPUT_SETUP}.
     *
     * <p>Value type: Intent (Parcelable)
     *
     * @deprecated Use {@link #EXTRA_ACTIVITY_AFTER_COMPLETION} instead
     */
    @Deprecated
    private static final String EXTRA_GOOGLE_ACTIVITY_AFTER_COMPLETION =
            "com.google.android.tv.intent.extra.ACTIVITY_AFTER_COMPLETION";

    public static void removeSetupIntent(Bundle extras) {
        extras.remove(EXTRA_SETUP_INTENT);
        extras.remove(EXTRA_GOOGLE_SETUP_INTENT);
    }

    @Nullable
    public static Intent getExtraSetupIntent(Intent intent) {
        Bundle extras = intent.getExtras();
        if (extras == null) {
            return null;
        }
        Intent setupIntent = extras.getParcelable(EXTRA_SETUP_INTENT);
        return setupIntent != null ? setupIntent : extras.getParcelable(EXTRA_GOOGLE_SETUP_INTENT);
    }

    @Nullable
    public static Intent getExtraActivityAfter(Intent intent) {
        Bundle extras = intent.getExtras();
        if (extras == null) {
            return null;
        }
        Intent setupIntent = extras.getParcelable(EXTRA_ACTIVITY_AFTER_COMPLETION);
        return setupIntent != null
                ? setupIntent
                : extras.getParcelable(EXTRA_GOOGLE_ACTIVITY_AFTER_COMPLETION);
    }

    public static boolean hasInputSetupAction(Intent intent) {
        String action = intent.getAction();
        return INTENT_ACTION_INPUT_SETUP.equals(action)
                || INTENT_GOOGLE_ACTION_INPUT_SETUP.equals(action);
    }
}
