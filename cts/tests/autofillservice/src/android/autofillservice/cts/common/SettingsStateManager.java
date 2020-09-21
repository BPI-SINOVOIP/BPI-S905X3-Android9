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

package android.autofillservice.cts.common;

import android.content.Context;
import android.provider.Settings;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.google.common.base.Preconditions;

/**
 * Manages the state of a preference backed by {@link Settings}.
 */
public class SettingsStateManager implements StateManager<String> {

    private final Context mContext;
    private final String mNamespace;
    private final String mKey;

    /**
     * Default constructor.
     *
     * @param context context used to retrieve the {@link Settings} provider.
     * @param namespace settings namespace.
     * @param key prefence key.
     */
    public SettingsStateManager(@NonNull Context context, @NonNull String namespace,
            @NonNull String key) {
        mContext = Preconditions.checkNotNull(context);
        mNamespace = Preconditions.checkNotNull(namespace);
        mKey = Preconditions.checkNotNull(key);
    }

    @Override
    public void set(@Nullable String value) {
        SettingsHelper.syncSet(mContext, mNamespace, mKey, value);
    }

    @Override
    @Nullable
    public String get() {
        return SettingsHelper.get(mNamespace, mKey);
    }

    @Override
    public String toString() {
        return "SettingsStateManager[namespace=" + mNamespace + ", key=" + mKey + "]";
    }
}
