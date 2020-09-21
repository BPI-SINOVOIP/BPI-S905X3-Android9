/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */
package com.android.settings.core;

import android.annotation.IntDef;
import android.content.Context;
import android.content.IntentFilter;
import android.text.TextUtils;
import android.util.Log;

import com.android.settings.search.ResultPayload;
import com.android.settings.search.SearchIndexableRaw;
import com.android.settings.slices.SliceData;
import com.android.settingslib.core.AbstractPreferenceController;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;
import java.util.List;

import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceGroup;
import android.support.v7.preference.PreferenceScreen;

/**
 * Abstract class to consolidate utility between preference controllers and act as an interface
 * for Slices. The abstract classes that inherit from this class will act as the direct interfaces
 * for each type when plugging into Slices.
 */
public abstract class BasePreferenceController extends AbstractPreferenceController {

    private static final String TAG = "SettingsPrefController";

    /**
     * Denotes the availability of the Setting.
     * <p>
     * Used both explicitly and by the convenience methods {@link #isAvailable()} and
     * {@link #isSupported()}.
     */
    @Retention(RetentionPolicy.SOURCE)
    @IntDef({AVAILABLE, UNSUPPORTED_ON_DEVICE, DISABLED_FOR_USER, DISABLED_DEPENDENT_SETTING,
            CONDITIONALLY_UNAVAILABLE})
    public @interface AvailabilityStatus {
    }

    /**
     * The setting is available.
     */
    public static final int AVAILABLE = 0;

    /**
     * A generic catch for settings which are currently unavailable, but may become available in
     * the future. You should use {@link #DISABLED_FOR_USER} or {@link #DISABLED_DEPENDENT_SETTING}
     * if they describe the condition more accurately.
     */
    public static final int CONDITIONALLY_UNAVAILABLE = 1;

    /**
     * The setting is not, and will not supported by this device.
     * <p>
     * There is no guarantee that the setting page exists, and any links to the Setting should take
     * you to the home page of Settings.
     */
    public static final int UNSUPPORTED_ON_DEVICE = 2;


    /**
     * The setting cannot be changed by the current user.
     * <p>
     * Links to the Setting should take you to the page of the Setting, even if it cannot be
     * changed.
     */
    public static final int DISABLED_FOR_USER = 3;

    /**
     * The setting has a dependency in the Settings App which is currently blocking access.
     * <p>
     * It must be possible for the Setting to be enabled by changing the configuration of the device
     * settings. That is, a setting that cannot be changed because of the state of another setting.
     * This should not be used for a setting that would be hidden from the UI entirely.
     * <p>
     * Correct use: Intensity of night display should be {@link #DISABLED_DEPENDENT_SETTING} when
     * night display is off.
     * Incorrect use: Mobile Data is {@link #DISABLED_DEPENDENT_SETTING} when there is no
     * data-enabled sim.
     * <p>
     * Links to the Setting should take you to the page of the Setting, even if it cannot be
     * changed.
     */
    public static final int DISABLED_DEPENDENT_SETTING = 4;


    protected final String mPreferenceKey;

    /**
     * Instantiate a controller as specified controller type and user-defined key.
     * <p/>
     * This is done through reflection. Do not use this method unless you know what you are doing.
     */
    public static BasePreferenceController createInstance(Context context,
            String controllerName, String key) {
        try {
            final Class<?> clazz = Class.forName(controllerName);
            final Constructor<?> preferenceConstructor =
                    clazz.getConstructor(Context.class, String.class);
            final Object[] params = new Object[] {context, key};
            return (BasePreferenceController) preferenceConstructor.newInstance(params);
        } catch (ClassNotFoundException | NoSuchMethodException | InstantiationException |
                IllegalArgumentException | InvocationTargetException | IllegalAccessException e) {
            throw new IllegalStateException(
                    "Invalid preference controller: " + controllerName, e);
        }
    }

    /**
     * Instantiate a controller as specified controller type.
     * <p/>
     * This is done through reflection. Do not use this method unless you know what you are doing.
     */
    public static BasePreferenceController createInstance(Context context, String controllerName) {
        try {
            final Class<?> clazz = Class.forName(controllerName);
            final Constructor<?> preferenceConstructor = clazz.getConstructor(Context.class);
            final Object[] params = new Object[] {context};
            return (BasePreferenceController) preferenceConstructor.newInstance(params);
        } catch (ClassNotFoundException | NoSuchMethodException | InstantiationException |
                IllegalArgumentException | InvocationTargetException | IllegalAccessException e) {
            throw new IllegalStateException(
                    "Invalid preference controller: " + controllerName, e);
        }
    }

    public BasePreferenceController(Context context, String preferenceKey) {
        super(context);
        mPreferenceKey = preferenceKey;
        if (TextUtils.isEmpty(mPreferenceKey)) {
            throw new IllegalArgumentException("Preference key must be set");
        }
    }

    /**
     * @return {@AvailabilityStatus} for the Setting. This status is used to determine if the
     * Setting should be shown or disabled in Settings. Further, it can be used to produce
     * appropriate error / warning Slice in the case of unavailability.
     * </p>
     * The status is used for the convenience methods: {@link #isAvailable()},
     * {@link #isSupported()}
     */
    @AvailabilityStatus
    public abstract int getAvailabilityStatus();

    @Override
    public String getPreferenceKey() {
        return mPreferenceKey;
    }

    /**
     * @return {@code true} when the controller can be changed on the device.
     *
     * <p>
     * Will return true for {@link #AVAILABLE} and {@link #DISABLED_DEPENDENT_SETTING}.
     * <p>
     * When the availability status returned by {@link #getAvailabilityStatus()} is
     * {@link #DISABLED_DEPENDENT_SETTING}, then the setting will be disabled by default in the
     * DashboardFragment, and it is up to the {@link BasePreferenceController} to enable the
     * preference at the right time.
     *
     * TODO (mfritze) Build a dependency mechanism to allow a controller to easily define the
     * dependent setting.
     */
    @Override
    public final boolean isAvailable() {
        final int availabilityStatus = getAvailabilityStatus();
        return (availabilityStatus == AVAILABLE
                || availabilityStatus == DISABLED_DEPENDENT_SETTING);
    }

    /**
     * @return {@code false} if the setting is not applicable to the device. This covers both
     * settings which were only introduced in future versions of android, or settings that have
     * hardware dependencies.
     * </p>
     * Note that a return value of {@code true} does not mean that the setting is available.
     */
    public final boolean isSupported() {
        return getAvailabilityStatus() != UNSUPPORTED_ON_DEVICE;
    }

    /**
     * Displays preference in this controller.
     */
    @Override
    public void displayPreference(PreferenceScreen screen) {
        super.displayPreference(screen);
        if (getAvailabilityStatus() == DISABLED_DEPENDENT_SETTING) {
            // Disable preference if it depends on another setting.
            final Preference preference = screen.findPreference(getPreferenceKey());
            if (preference != null) {
                preference.setEnabled(false);
            }
        }
    }

    /**
     * @return the UI type supported by the controller.
     */
    @SliceData.SliceType
    public int getSliceType() {
        return SliceData.SliceType.INTENT;
    }

    /**
     * @return an {@link IntentFilter} that includes all broadcasts which can affect the state of
     * this Setting.
     */
    public IntentFilter getIntentFilter() {
        return null;
    }

    /**
     * Determines if the controller should be used as a Slice.
     * <p>
     *     Important criteria for a Slice are:
     *     - Must be secure
     *     - Must not be a privacy leak
     *     - Must be understandable as a stand-alone Setting.
     * <p>
     *     This does not guarantee the setting is available. {@link #isAvailable()} should sill be
     *     called.
     *
     * @return {@code true} if the controller should be used externally as a Slice.
     */
    public boolean isSliceable() {
        return false;
    }

    /**
     * @return {@code true} if the setting update asynchronously.
     * <p>
     * For example, a Wifi controller would return true, because it needs to update the radio
     * and wait for it to turn on.
     */
    public boolean hasAsyncUpdate() {
        return false;
    }

    /**
     * Updates non-indexable keys for search provider.
     *
     * Called by SearchIndexProvider#getNonIndexableKeys
     */
    public void updateNonIndexableKeys(List<String> keys) {
        if (this instanceof AbstractPreferenceController) {
            if (!isAvailable()) {
                final String key = getPreferenceKey();
                if (TextUtils.isEmpty(key)) {
                    Log.w(TAG,
                            "Skipping updateNonIndexableKeys due to empty key " + this.toString());
                    return;
                }
                keys.add(key);
            }
        }
    }

    /**
     * Updates raw data for search provider.
     *
     * Called by SearchIndexProvider#getRawDataToIndex
     */
    public void updateRawDataToIndex(List<SearchIndexableRaw> rawData) {
    }

    /**
     * @return the {@link ResultPayload} corresponding to the search result type for the preference.
     * TODO (b/69808376) Remove this method.
     * Do not extend this method. It will not launch with P.
     */
    @Deprecated
    public ResultPayload getResultPayload() {
        return null;
    }
}