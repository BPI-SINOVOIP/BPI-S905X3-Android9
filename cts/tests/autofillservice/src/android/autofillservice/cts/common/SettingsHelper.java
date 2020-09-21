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

import static android.autofillservice.cts.common.ShellHelper.runShellCommand;

import static com.google.common.truth.Truth.assertWithMessage;

import android.content.Context;
import android.provider.Settings;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

/**
 * Provides utilities to interact with the device's {@link Settings}.
 */
public final class SettingsHelper {

    public static final String NAMESPACE_SECURE = "secure";
    public static final String NAMESPACE_GLOBAL = "global";

    /**
     * Uses a Shell command to set the given preference.
     */
    public static void set(@NonNull String namespace, @NonNull String key, @Nullable String value) {
        if (value == null) {
            delete(namespace, key);
            return;
        }
        runShellCommand("settings put %s %s %s default", namespace, key, value);
    }

    public static void set(@NonNull String key, @Nullable String value) {
        set(NAMESPACE_SECURE, key, value);
    }

    /**
     * Uses a Shell command to set the given preference, and verifies it was correctly set.
     */
    public static void syncSet(@NonNull Context context, @NonNull String namespace,
            @NonNull String key, @Nullable String value) {
        if (value == null) {
            syncDelete(context, namespace, key);
            return;
        }

        final String currentValue = get(namespace, key);
        if (value.equals(currentValue)) {
            // Already set, ignore
            return;
        }

        final OneTimeSettingsListener observer =
                new OneTimeSettingsListener(context, namespace, key);
        set(namespace, key, value);
        observer.assertCalled();

        final String newValue = get(namespace, key);
        assertWithMessage("invalid value for '%s' settings", key).that(newValue).isEqualTo(value);
    }

    public static void syncSet(@NonNull Context context, @NonNull String key,
            @Nullable String value) {
        syncSet(context, NAMESPACE_SECURE, key, value);
    }

    /**
     * Uses a Shell command to delete the given preference.
     */
    public static void delete(@NonNull String namespace, @NonNull String key) {
        runShellCommand("settings delete %s %s", namespace, key);
    }

    public static void delete(@NonNull String key) {
        delete(NAMESPACE_SECURE, key);
    }

    /**
     * Uses a Shell command to delete the given preference, and verifies it was correctly deleted.
     */
    public static void syncDelete(@NonNull Context context, @NonNull String namespace,
            @NonNull String key) {

        final String currentValue = get(namespace, key);
        if (currentValue == null || currentValue.equals("null")) {
            // Already set, ignore
            return;
        }

        final OneTimeSettingsListener observer = new OneTimeSettingsListener(context, namespace,
                key);
        delete(namespace, key);
        observer.assertCalled();

        final String newValue = get(namespace, key);
        assertWithMessage("invalid value for '%s' settings", key).that(newValue).isEqualTo("null");
    }

    public static void syncDelete(@NonNull Context context, @NonNull String key) {
        syncDelete(context, NAMESPACE_SECURE, key);
    }

    /**
     * Gets the value of a given preference using Shell command.
     */
    @NonNull
    public static String get(@NonNull String namespace, @NonNull String key) {
        return runShellCommand("settings get %s %s", namespace, key);
    }

    @NonNull
    public static String get(@NonNull String key) {
        return get(NAMESPACE_SECURE, key);
    }

    private SettingsHelper() {
        throw new UnsupportedOperationException("contain static methods only");
    }
}
