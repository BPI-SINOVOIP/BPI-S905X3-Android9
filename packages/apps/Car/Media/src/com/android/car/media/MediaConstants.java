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

package com.android.car.media;

import android.annotation.StringDef;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * Constants shared by SDK and 3rd party media apps to extend the media APIs in order to accommodate
 * car-specific requirements.
 * These constants are also shared with Android Auto Projected. They will be moved to the Car
 * Support Library on b/76108793.
 * Please refer to <a href="https://developer.android.com/training/auto/audio/index.html">Providing
 * Audio Playback for Auto</a> for a detailed explanation.
 */
public class MediaConstants {
    /**
     * Action along with the media connection broadcast, which contains the current media
     * connection status.
     */
    public static final String ACTION_MEDIA_STATUS = "com.google.android.gms.car.media.STATUS";

    /**
     * Key for media connection status in extra.
     */
    public static final String MEDIA_CONNECTION_STATUS = "media_connection_status";

    @Retention(RetentionPolicy.SOURCE)
    @StringDef({MEDIA_CONNECTED, MEDIA_DISCONNECTED})
    public @interface ConnectionType {}

    /**
     * Type of connection status: current media is connected.
     */
    public static final String MEDIA_CONNECTED = "media_connected";

    /**
     * Type of connection status: current media is disconnected.
     */
    public static final String MEDIA_DISCONNECTED = "media_disconnected";

    /**
     * Key for extra feedback message in extra.
     */
    public static final String EXTRA_CUSTOM_ACTION_STATUS = "media_custom_action_status";

    /**
     * Extra along with the Media Session, which contains if the slot of the action should be
     * always reserved for the queue action.
     */
    public static final String EXTRA_RESERVED_SLOT_QUEUE =
            "com.google.android.gms.car.media.ALWAYS_RESERVE_SPACE_FOR.ACTION_QUEUE";

    /**
     * Extra along with the Media Session, which contains if the slot of the action should be
     * always reserved for the skip to previous action.
     */
    public static final String EXTRA_RESERVED_SLOT_SKIP_TO_PREVIOUS =
            "com.google.android.gms.car.media.ALWAYS_RESERVE_SPACE_FOR.ACTION_SKIP_TO_PREVIOUS";

    /**
     * Extra along with the Media Session, which contains if the slot of the action should be
     * always reserved for the skip to next action.
     */
    public static final String EXTRA_RESERVED_SLOT_SKIP_TO_NEXT =
            "com.google.android.gms.car.media.ALWAYS_RESERVE_SPACE_FOR.ACTION_SKIP_TO_NEXT";

    /**
     * Extra along with custom action playback state to indicate a repeated action.
     */
    public static final String EXTRA_REPEATED_CUSTOM_ACTION_BUTTON =
            "com.google.android.gms.car.media.CUSTOM_ACTION.REPEATED_ACTIONS";

    /**
     * Extra along with custom action playback state to indicate a repeated custom action button
     * state.
     */
    public static final String EXTRA_REPEATED_CUSTOM_ACTION_BUTTON_ON_DOWN =
            "com.google.android.gms.car.media.CUSTOM_ACTION.ON_DOWN_EVENT";

    /**
     * Extra along with metadata to indicate whether this audio is advertisement.
     */
    public static final String EXTRA_METADATA_ADVERTISEMENT =
            "android.media.metadata.ADVERTISEMENT";
}
