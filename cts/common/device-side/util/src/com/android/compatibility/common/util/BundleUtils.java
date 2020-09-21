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
package com.android.compatibility.common.util;

import android.os.Bundle;

public class BundleUtils {
    private BundleUtils() {
    }

    public static Bundle makeBundle(Object... keysAndValues) {
        if ((keysAndValues.length % 2) != 0) {
            throw new IllegalArgumentException("Argument count not even.");
        }

        if (keysAndValues.length == 0) {
            return null;
        }
        final Bundle ret = new Bundle();

        for (int i = keysAndValues.length - 2; i >= 0; i -= 2) {
            final String key = keysAndValues[i].toString();
            final Object value = keysAndValues[i + 1];

            if (value == null) {
                ret.putString(key, null);

            } else if (value instanceof Boolean) {
                ret.putBoolean(key, (Boolean) value);

            } else if (value instanceof Integer) {
                ret.putInt(key, (Integer) value);

            } else if (value instanceof String) {
                ret.putString(key, (String) value);

            } else if (value instanceof Bundle) {
                ret.putBundle(key, (Bundle) value);
            } else {
                throw new IllegalArgumentException(
                        "Type not supported yet: " + value.getClass().getName());
            }
        }
        return ret;
    }
}
