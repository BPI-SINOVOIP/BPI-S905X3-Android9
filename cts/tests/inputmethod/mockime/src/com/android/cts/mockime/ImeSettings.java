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

package com.android.cts.mockime;

import android.os.Parcel;
import android.os.PersistableBundle;
import androidx.annotation.ColorInt;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

/**
 * An immutable data store to control the behavior of {@link MockIme}.
 */
public class ImeSettings {

    @NonNull
    private final String mEventCallbackActionName;

    private static final String BACKGROUND_COLOR_KEY = "BackgroundColor";
    private static final String NAVIGATION_BAR_COLOR_KEY = "NavigationBarColor";
    private static final String INPUT_VIEW_HEIGHT_WITHOUT_SYSTEM_WINDOW_INSET =
            "InputViewHeightWithoutSystemWindowInset";
    private static final String WINDOW_FLAGS = "WindowFlags";
    private static final String WINDOW_FLAGS_MASK = "WindowFlagsMask";
    private static final String FULLSCREEN_MODE_ALLOWED = "FullscreenModeAllowed";
    private static final String INPUT_VIEW_SYSTEM_UI_VISIBILITY = "InputViewSystemUiVisibility";
    private static final String HARD_KEYBOARD_CONFIGURATION_BEHAVIOR_ALLOWED =
            "HardKeyboardConfigurationBehaviorAllowed";

    @NonNull
    private final PersistableBundle mBundle;

    ImeSettings(@NonNull Parcel parcel) {
        mEventCallbackActionName = parcel.readString();
        mBundle = parcel.readPersistableBundle();
    }

    @Nullable
    String getEventCallbackActionName() {
        return mEventCallbackActionName;
    }

    public boolean fullscreenModeAllowed(boolean defaultValue) {
        return mBundle.getBoolean(FULLSCREEN_MODE_ALLOWED, defaultValue);
    }

    @ColorInt
    public int getBackgroundColor(@ColorInt int defaultColor) {
        return mBundle.getInt(BACKGROUND_COLOR_KEY, defaultColor);
    }

    public boolean hasNavigationBarColor() {
        return mBundle.keySet().contains(NAVIGATION_BAR_COLOR_KEY);
    }

    @ColorInt
    public int getNavigationBarColor() {
        return mBundle.getInt(NAVIGATION_BAR_COLOR_KEY);
    }

    public int getInputViewHeightWithoutSystemWindowInset(int defaultHeight) {
        return mBundle.getInt(INPUT_VIEW_HEIGHT_WITHOUT_SYSTEM_WINDOW_INSET, defaultHeight);
    }

    public int getWindowFlags(int defaultFlags) {
        return mBundle.getInt(WINDOW_FLAGS, defaultFlags);
    }

    public int getWindowFlagsMask(int defaultFlags) {
        return mBundle.getInt(WINDOW_FLAGS_MASK, defaultFlags);
    }

    public int getInputViewSystemUiVisibility(int defaultFlags) {
        return mBundle.getInt(INPUT_VIEW_SYSTEM_UI_VISIBILITY, defaultFlags);
    }

    public boolean getHardKeyboardConfigurationBehaviorAllowed(boolean defaultValue) {
        return mBundle.getBoolean(HARD_KEYBOARD_CONFIGURATION_BEHAVIOR_ALLOWED, defaultValue);
    }

    static void writeToParcel(@NonNull Parcel parcel, @NonNull String eventCallbackActionName,
            @Nullable Builder builder) {
        parcel.writeString(eventCallbackActionName);
        if (builder != null) {
            parcel.writePersistableBundle(builder.mBundle);
        } else {
            parcel.writePersistableBundle(PersistableBundle.EMPTY);
        }
    }

    /**
     * The builder class for {@link ImeSettings}.
     */
    public static final class Builder {
        private final PersistableBundle mBundle = new PersistableBundle();

        /**
         * Controls whether fullscreen mode is allowed or not.
         *
         * <p>By default, fullscreen mode is not allowed in {@link MockIme}.</p>
         *
         * @param allowed {@code true} if fullscreen mode is allowed
         * @see MockIme#onEvaluateFullscreenMode()
         */
        public Builder setFullscreenModeAllowed(boolean allowed) {
            mBundle.putBoolean(FULLSCREEN_MODE_ALLOWED, allowed);
            return this;
        }

        /**
         * Sets the background color of the {@link MockIme}.
         * @param color background color to be used
         */
        public Builder setBackgroundColor(@ColorInt int color) {
            mBundle.putInt(BACKGROUND_COLOR_KEY, color);
            return this;
        }

        /**
         * Sets the color to be passed to {@link android.view.Window#setNavigationBarColor(int)}.
         *
         * @param color color to be passed to {@link android.view.Window#setNavigationBarColor(int)}
         * @see android.view.View
         */
        public Builder setNavigationBarColor(@ColorInt int color) {
            mBundle.putInt(NAVIGATION_BAR_COLOR_KEY, color);
            return this;
        }

        /**
         * Sets the input view height measured from the bottom system window inset.
         * @param height height of the soft input view. This does not include the system window
         *               inset such as navigation bar
         */
        public Builder setInputViewHeightWithoutSystemWindowInset(int height) {
            mBundle.putInt(INPUT_VIEW_HEIGHT_WITHOUT_SYSTEM_WINDOW_INSET, height);
            return this;
        }

        /**
         * Sets window flags to be specified to {@link android.view.Window#setFlags(int, int)} of
         * the main {@link MockIme} window.
         *
         * <p>When {@link android.view.WindowManager.LayoutParams#FLAG_LAYOUT_IN_OVERSCAN} is set,
         * {@link MockIme} tries to render the navigation bar by itself.</p>
         *
         * @param flags flags to be specified
         * @param flagsMask mask bits that specify what bits need to be cleared before setting
         *                  {@code flags}
         * @see android.view.WindowManager
         */
        public Builder setWindowFlags(int flags, int flagsMask) {
            mBundle.putInt(WINDOW_FLAGS, flags);
            mBundle.putInt(WINDOW_FLAGS_MASK, flagsMask);
            return this;
        }

        /**
         * Sets flags to be specified to {@link android.view.View#setSystemUiVisibility(int)} of
         * the main soft input view (the returned view from {@link MockIme#onCreateInputView()}).
         *
         * @param visibilityFlags flags to be specified
         * @see android.view.View
         */
        public Builder setInputViewSystemUiVisibility(int visibilityFlags) {
            mBundle.putInt(INPUT_VIEW_SYSTEM_UI_VISIBILITY, visibilityFlags);
            return this;
        }

        /**
         * Controls whether {@link MockIme} is allowed to change the behavior based on
         * {@link android.content.res.Configuration#keyboard} and
         * {@link android.content.res.Configuration#hardKeyboardHidden}.
         *
         * <p>Methods in {@link android.inputmethodservice.InputMethodService} such as
         * {@link android.inputmethodservice.InputMethodService#onEvaluateInputViewShown()} and
         * {@link android.inputmethodservice.InputMethodService#onShowInputRequested(int, boolean)}
         * change their behaviors when a hardware keyboard is attached.  This is confusing when
         * writing tests so by default {@link MockIme} tries to cancel those behaviors.  This
         * settings re-enables such a behavior.</p>
         *
         * @param allowed {@code true} when {@link MockIme} is allowed to change the behavior when
         *                a hardware keyboard is attached
         *
         * @see android.inputmethodservice.InputMethodService#onEvaluateInputViewShown()
         * @see android.inputmethodservice.InputMethodService#onShowInputRequested(int, boolean)
         */
        public Builder setHardKeyboardConfigurationBehaviorAllowed(boolean allowed) {
            mBundle.putBoolean(HARD_KEYBOARD_CONFIGURATION_BEHAVIOR_ALLOWED, allowed);
            return this;
        }
    }
}
