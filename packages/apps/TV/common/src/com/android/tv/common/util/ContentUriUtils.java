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

package com.android.tv.common.util;

import android.content.ContentUris;
import android.net.Uri;
import android.util.Log;

/** Static utils for{@link android.content.ContentUris}. */
public class ContentUriUtils {
    private static final String TAG = "ContentUriUtils";

    /**
     * Converts the last path segment to a long.
     *
     * <p>This supports a common convention for content URIs where an ID is stored in the last
     * segment.
     *
     * @return the long conversion of the last segment or -1 if the path is empty or there is any
     *     error
     * @see ContentUris#parseId(Uri)
     */
    public static long safeParseId(Uri uri) {
        try {
            return ContentUris.parseId(uri);
        } catch (Exception e) {
            Log.d(TAG, "Error parsing " + uri, e);
            return -1;
        }
    }
}
