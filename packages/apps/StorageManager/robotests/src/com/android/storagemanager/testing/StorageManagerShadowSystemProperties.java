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

package com.android.storagemanager.testing;

import android.os.SystemProperties;

import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;
import org.robolectric.shadows.ShadowSystemProperties;

import java.util.HashMap;
import java.util.Map;

/** TODO: Don't use this. Use an upstreamed version. */
@Implements(SystemProperties.class)
public class StorageManagerShadowSystemProperties extends ShadowSystemProperties {
    private static final Map<String, String> sValues = new HashMap<>();

    @Implementation
    public static synchronized boolean getBoolean(String key, boolean def) {
        if (sValues.containsKey(key)) {
            String val = sValues.get(key);
            return "y".equals(val)
                    || "yes".equals(val)
                    || "1".equals(val)
                    || "true".equals(val)
                    || "on".equals(val);
        }
        return ShadowSystemProperties.getBoolean(key, def);
    }

    @Implementation
    public static synchronized String get(String key) {
        if (sValues.containsKey(key)) {
            return sValues.get(key);
        }
        return ShadowSystemProperties.get(key);
    }

    public static synchronized void put(String key, String value) {
        sValues.put(key, value);
    }

    @Implementation
    public static String get(String key, String defaultValue) {
        String value = sValues.get(key);
        return value == null ? defaultValue : value;
    }

    public static synchronized void clear() {
        sValues.clear();
    }
}
