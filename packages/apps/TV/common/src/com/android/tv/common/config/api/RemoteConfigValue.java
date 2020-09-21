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
package com.android.tv.common.config.api;

/** Wrapper for a RemoteConfig key and default value. */
public abstract class RemoteConfigValue<T> {
    private final T defaultValue;
    private final String key;

    private RemoteConfigValue(String key, T defaultValue) {
        this.defaultValue = defaultValue;
        this.key = key;
    }

    /** Create with the given key and default value */
    public static RemoteConfigValue<Long> create(String key, long defaultValue) {
        return new RemoteConfigValue<Long>(key, defaultValue) {
            @Override
            public Long get(RemoteConfig remoteConfig) {
                return remoteConfig.getLong(key, defaultValue);
            }
        };
    }

    public abstract T get(RemoteConfig remoteConfig);

    public final T getDefaultValue() {
        return defaultValue;
    }

    @Override
    public final String toString() {
        return "RemoteConfigValue(key=" + key + ", defalutValue=" + defaultValue + "]";
    }
}
