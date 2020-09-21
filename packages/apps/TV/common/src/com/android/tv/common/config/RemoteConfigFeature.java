/*
 * Copyright (C) 2016 The Android Open Source Project
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

package com.android.tv.common.config;

import android.content.Context;
import com.android.tv.common.BaseApplication;
import com.android.tv.common.feature.Feature;

/**
 * A {@link Feature} controlled by a {@link com.android.tv.common.config.api.RemoteConfig} boolean.
 */
public class RemoteConfigFeature implements Feature {
    private final String mKey;

    /** Creates a {@link RemoteConfigFeature for the {@code key}. */
    public static RemoteConfigFeature fromKey(String key) {
        return new RemoteConfigFeature(key);
    }

    private RemoteConfigFeature(String key) {
        mKey = key;
    }

    @Override
    public boolean isEnabled(Context context) {
        return BaseApplication.getSingletons(context).getRemoteConfig().getBoolean(mKey);
    }
}
