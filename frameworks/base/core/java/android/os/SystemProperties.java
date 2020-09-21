/*
 * Copyright (C) 2006 The Android Open Source Project
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

package android.os;

import android.annotation.NonNull;
import android.annotation.Nullable;
import android.annotation.SystemApi;
import android.annotation.TestApi;
import android.util.Log;
import android.util.MutableInt;

import com.android.internal.annotations.GuardedBy;

import java.util.ArrayList;
import java.util.HashMap;


/**
 * Gives access to the system properties store.  The system properties
 * store contains a list of string key-value pairs.
 *
 * {@hide}
 */
@SystemApi
@TestApi
public class SystemProperties {
    private static final String TAG = "SystemProperties";
    private static final boolean TRACK_KEY_ACCESS = false;

    /**
     * Android O removed the property name length limit, but com.amazon.kindle 7.8.1.5
     * uses reflection to read this whenever text is selected (http://b/36095274).
     * @hide
     */
    public static final int PROP_NAME_MAX = Integer.MAX_VALUE;

    /** @hide */
    public static final int PROP_VALUE_MAX = 91;

    @GuardedBy("sChangeCallbacks")
    private static final ArrayList<Runnable> sChangeCallbacks = new ArrayList<Runnable>();

    @GuardedBy("sRoReads")
    private static final HashMap<String, MutableInt> sRoReads =
            TRACK_KEY_ACCESS ? new HashMap<>() : null;

    private static void onKeyAccess(String key) {
        if (!TRACK_KEY_ACCESS) return;

        if (key != null && key.startsWith("ro.")) {
            synchronized (sRoReads) {
                MutableInt numReads = sRoReads.getOrDefault(key, null);
                if (numReads == null) {
                    numReads = new MutableInt(0);
                    sRoReads.put(key, numReads);
                }
                numReads.value++;
                if (numReads.value > 3) {
                    Log.d(TAG, "Repeated read (count=" + numReads.value
                            + ") of a read-only system property '" + key + "'",
                            new Exception());
                }
            }
        }
    }

    private static native String native_get(String key);
    private static native String native_get(String key, String def);
    private static native int native_get_int(String key, int def);
    private static native long native_get_long(String key, long def);
    private static native boolean native_get_boolean(String key, boolean def);
    private static native void native_set(String key, String def);
    private static native void native_add_change_callback();
    private static native void native_report_sysprop_change();

    /**
     * Get the String value for the given {@code key}.
     *
     * @param key the key to lookup
     * @return an empty string if the {@code key} isn't found
     * @hide
     */
    @NonNull
    @SystemApi
    public static String get(@NonNull String key) {
        if (TRACK_KEY_ACCESS) onKeyAccess(key);
        return native_get(key);
    }

    /**
     * Get the String value for the given {@code key}.
     *
     * @param key the key to lookup
     * @param def the default value in case the property is not set or empty
     * @return if the {@code key} isn't found, return {@code def} if it isn't null, or an empty
     * string otherwise
     * @hide
     */
    @NonNull
    @SystemApi
    @TestApi
    public static String get(@NonNull String key, @Nullable String def) {
        if (TRACK_KEY_ACCESS) onKeyAccess(key);
        return native_get(key, def);
    }

    /**
     * Get the value for the given {@code key}, and return as an integer.
     *
     * @param key the key to lookup
     * @param def a default value to return
     * @return the key parsed as an integer, or def if the key isn't found or
     *         cannot be parsed
     * @hide
     */
    @SystemApi
    public static int getInt(@NonNull String key, int def) {
        if (TRACK_KEY_ACCESS) onKeyAccess(key);
        return native_get_int(key, def);
    }

    /**
     * Get the value for the given {@code key}, and return as a long.
     *
     * @param key the key to lookup
     * @param def a default value to return
     * @return the key parsed as a long, or def if the key isn't found or
     *         cannot be parsed
     * @hide
     */
    @SystemApi
    public static long getLong(@NonNull String key, long def) {
        if (TRACK_KEY_ACCESS) onKeyAccess(key);
        return native_get_long(key, def);
    }

    /**
     * Get the value for the given {@code key}, returned as a boolean.
     * Values 'n', 'no', '0', 'false' or 'off' are considered false.
     * Values 'y', 'yes', '1', 'true' or 'on' are considered true.
     * (case sensitive).
     * If the key does not exist, or has any other value, then the default
     * result is returned.
     *
     * @param key the key to lookup
     * @param def a default value to return
     * @return the key parsed as a boolean, or def if the key isn't found or is
     *         not able to be parsed as a boolean.
     * @hide
     */
    @SystemApi
    public static boolean getBoolean(@NonNull String key, boolean def) {
        if (TRACK_KEY_ACCESS) onKeyAccess(key);
        return native_get_boolean(key, def);
    }

    /**
     * Set the value for the given {@code key} to {@code val}.
     *
     * @throws IllegalArgumentException if the {@code val} exceeds 91 characters
     * @hide
     */
    public static void set(@NonNull String key, @Nullable String val) {
        if (val != null && !val.startsWith("ro.") && val.length() > PROP_VALUE_MAX) {
            throw new IllegalArgumentException("value of system property '" + key
                    + "' is longer than " + PROP_VALUE_MAX + " characters: " + val);
        }
        if (TRACK_KEY_ACCESS) onKeyAccess(key);
        native_set(key, val);
    }

    /**
     * Add a callback that will be run whenever any system property changes.
     *
     * @param callback The {@link Runnable} that should be executed when a system property
     * changes.
     * @hide
     */
    public static void addChangeCallback(@NonNull Runnable callback) {
        synchronized (sChangeCallbacks) {
            if (sChangeCallbacks.size() == 0) {
                native_add_change_callback();
            }
            sChangeCallbacks.add(callback);
        }
    }

    @SuppressWarnings("unused")  // Called from native code.
    private static void callChangeCallbacks() {
        synchronized (sChangeCallbacks) {
            //Log.i("foo", "Calling " + sChangeCallbacks.size() + " change callbacks!");
            if (sChangeCallbacks.size() == 0) {
                return;
            }
            ArrayList<Runnable> callbacks = new ArrayList<Runnable>(sChangeCallbacks);
            for (int i=0; i<callbacks.size(); i++) {
                try {
                    callbacks.get(i).run();
                } catch (Throwable t) {
                    Log.wtf(TAG, "Exception in SystemProperties change callback", t);
                    // Ignore and try to go on.
                }
            }
        }
    }

    /**
     * Notifies listeners that a system property has changed
     * @hide
     */
    public static void reportSyspropChanged() {
        native_report_sysprop_change();
    }

    private SystemProperties() {
    }
}
