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
 * limitations under the License.
 */

package com.android.car.media.common;

/**
 * Holds constants used when dealing with MediaBrowserServices that support the
 * content style API for media.
 */
public final class ContentStyleMediaConstants {

    /** Declares that ContentStyle is supported */
    public static final String CONTENT_STYLE_SUPPORTED =
            "android.auto.media.CONTENT_STYLE_SUPPORTED";

    /**
     * Bundle extra indicating the presentation hint for playable media items. See {@link
     * #CONTENT_STYLE_LIST_ITEM_HINT_VALUE} or {@link #CONTENT_STYLE_GRID_ITEM_HINT_VALUE}
     */
    public static final String CONTENT_STYLE_PLAYABLE_HINT =
            "android.auto.media.CONTENT_STYLE_PLAYABLE_HINT";

    /**
     * Bundle extra indicating the presentation hint for browsable media items. See {@link
     * #CONTENT_STYLE_LIST_ITEM_HINT_VALUE} or {@link #CONTENT_STYLE_GRID_ITEM_HINT_VALUE}
     */
    public static final String CONTENT_STYLE_BROWSABLE_HINT =
            "android.auto.media.CONTENT_STYLE_BROWSABLE_HINT";

    /**
     * Value for {@link #CONTENT_STYLE_PLAYABLE_HINT} and {@link #CONTENT_STYLE_BROWSABLE_HINT} that
     * hints the corresponding items should be presented as lists.
     */
    public static final int CONTENT_STYLE_LIST_ITEM_HINT_VALUE = 1;

    /**
     * Value for {@link #CONTENT_STYLE_PLAYABLE_HINT} and {@link #CONTENT_STYLE_BROWSABLE_HINT} that
     * hints the corresponding items should be presented as grids.
     */
    public static final int CONTENT_STYLE_GRID_ITEM_HINT_VALUE = 2;
}
