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

/**
 * JUnit rule used to set a {@link Settings} preference before the test is run.
 *
 * <p>It stores the current value before the test, changes it (if necessary), then restores it after
 * the test (if necessary).
 */
public class SettingsStateChangerRule extends StateChangerRule<String> {

    /**
     * Default constructor for the 'secure' context.
     *
     * @param context context used to retrieve the {@link Settings} provider.
     * @param key prefence key.
     * @param value value to be set before the test is run.
     */
    public SettingsStateChangerRule(@NonNull Context context, @NonNull String key,
            @Nullable String value) {
        this(context, SettingsHelper.NAMESPACE_SECURE, key, value);
    }

    /**
     * Default constructor.
     *
     * @param context context used to retrieve the {@link Settings} provider.
     * @param namespace settings namespace.
     * @param key prefence key.
     * @param value value to be set before the test is run.
     */
    public SettingsStateChangerRule(@NonNull Context context, @NonNull String namespace,
            @NonNull String key, @Nullable String value) {
        super(new SettingsStateManager(context, namespace, key), value);
    }
}
