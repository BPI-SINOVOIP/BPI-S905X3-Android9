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

package com.android.car.media.common;

import android.annotation.NonNull;
import android.annotation.Nullable;
import android.graphics.drawable.Drawable;
import android.os.Bundle;

/**
 * Abstract representation of a custom playback action. A custom playback action represents a
 * visual element that can be used to trigger playback actions not included in the standard
 * {@link PlaybackControls} class.
 * Custom actions for the current media source are exposed through
 * {@link PlaybackModel#getCustomActions()}
 */
public class CustomPlaybackAction {
    /** Icon to display for this custom action */
    @NonNull
    public final Drawable mIcon;
    /** Action identifier used to request this action to the media service */
    @NonNull
    public final String mAction;
    /** Any additional information to send along with the action identifier */
    @Nullable
    public final Bundle mExtras;

    /**
     * Creates a custom action
     */
    public CustomPlaybackAction(@NonNull Drawable icon, @NonNull String action,
            @Nullable Bundle extras) {
        mIcon = icon;
        mAction = action;
        mExtras = extras;
    }
}
