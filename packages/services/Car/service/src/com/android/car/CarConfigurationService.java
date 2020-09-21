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

package com.android.car;

import android.annotation.Nullable;
import android.annotation.RawRes;
import android.car.settings.ICarConfigurationManager;
import android.car.settings.SpeedBumpConfiguration;
import android.content.Context;
import android.util.Log;

import com.android.internal.annotations.VisibleForTesting;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.PrintWriter;

/**
 * A service that will look at a default JSON configuration file on the system and parses out its
 * results.
 *
 * <p>This service will look for the JSON file that is mapped to {@code R.raw.car_config}. If this
 * value does not exist or is malformed, then this service will not fail; instead, it returns
 * default values for various configurations.
 */
public class CarConfigurationService extends ICarConfigurationManager.Stub
        implements CarServiceBase {
    private static final String TAG = "CarConfigurationService";

    // Keys for accessing data in the parsed JSON related to SpeedBump.
    @VisibleForTesting
    static final String SPEED_BUMP_CONFIG_KEY = "SpeedBump";
    @VisibleForTesting
    static final String SPEED_BUMP_ACQUIRED_PERMITS_PER_SECOND_KEY =
            "acquiredPermitsPerSecond";
    @VisibleForTesting
    static final String SPEED_BUMP_MAX_PERMIT_POOL_KEY = "maxPermitPool";
    @VisibleForTesting
    static final String SPEED_BUMP_PERMIT_FILL_DELAY_KEY = "permitFillDelay";

    // Default values for speed bump configuration.
    @VisibleForTesting
    static final double DEFAULT_SPEED_BUMP_ACQUIRED_PERMITS_PER_SECOND = 0.5d;
    @VisibleForTesting
    static final double DEFAULT_SPEED_BUMP_MAX_PERMIT_POOL = 5d;
    @VisibleForTesting
    static final long DEFAULT_SPEED_BUMP_PERMIT_FILL_DELAY = 600L;

    private final Context mContext;
    private final JsonReader mJsonReader;

    @VisibleForTesting
    @Nullable
    JSONObject mConfigFile;

    @Nullable
    private SpeedBumpConfiguration mSpeedBumpConfiguration;

    /**
     * An interface that abstracts away the parsing of a JSON file. This interface allows the
     * JSON file to be mocked away for testing.
     */
    @VisibleForTesting
    interface JsonReader {
        /**
         * Returns the contents of the JSON file that is pointed to by the given {@code resId} as
         * a string.
         *
         * @param context The current Context.
         * @param resId The resource id of the JSON file.
         * @return A string representation of the file or {@code null} if an error occurred.
         */
        @Nullable
        String jsonFileToString(Context context, @RawRes int resId);
    }

    CarConfigurationService(Context context, JsonReader reader) {
        mContext = context;
        mJsonReader = reader;
    }

    /**
     * Returns the configuration values for speed bump that is found in the configuration JSON on
     * the system. If there was an error reading this JSON or the JSON did not contain
     * speed bump configuration, then default values will be returned. This method does not return
     * {@code null}.
     */
    @Override
    public SpeedBumpConfiguration getSpeedBumpConfiguration() {
        if (mSpeedBumpConfiguration == null) {
            return getDefaultSpeedBumpConfiguration();
        }
        return mSpeedBumpConfiguration;
    }

    @Override
    public synchronized void init() {
        String jsonString = mJsonReader.jsonFileToString(mContext, R.raw.car_config);
        if (jsonString != null) {
            try {
                mConfigFile = new JSONObject(jsonString);
            } catch (JSONException e) {
                Log.e(TAG, "Error reading JSON file", e);
            }
        }

        mSpeedBumpConfiguration = createSpeedBumpConfiguration();
    }

    @Override
    public synchronized void release() {
        mConfigFile =  null;
        mSpeedBumpConfiguration = null;
    }

    @Override
    public void dump(PrintWriter writer) {
        writer.println("Config value initialized: " + (mConfigFile != null));
        if (mConfigFile != null) {
            try {
                writer.println("Config: " + mConfigFile.toString(/* indentSpaces= */ 2));
            } catch (JSONException e) {
                Log.e(TAG, "Error printing JSON config", e);
                writer.println("Config: " + mConfigFile);
            }
        }

        writer.println("SpeedBumpConfig initialized: " + (mSpeedBumpConfiguration != null));
        if (mSpeedBumpConfiguration != null) {
            writer.println("SpeedBumpConfig: " + mSpeedBumpConfiguration);
        }
    }

    /**
     * Reads the configuration for speed bump off of the parsed JSON stored in {@link #mConfigFile}.
     * If {@code mConfigFile} is {@code null} or a configuration does not exist for speed bump,
     * then return the default configuration created by {@link #getDefaultSpeedBumpConfiguration()}.
     */
    private SpeedBumpConfiguration createSpeedBumpConfiguration() {
        if (mConfigFile == null) {
            return getDefaultSpeedBumpConfiguration();
        }

        try {
            JSONObject speedBumpJson = mConfigFile.getJSONObject(SPEED_BUMP_CONFIG_KEY);

            if (speedBumpJson != null) {
                return new SpeedBumpConfiguration(
                        speedBumpJson.getDouble(SPEED_BUMP_ACQUIRED_PERMITS_PER_SECOND_KEY),
                        speedBumpJson.getDouble(SPEED_BUMP_MAX_PERMIT_POOL_KEY),
                        speedBumpJson.getLong(SPEED_BUMP_PERMIT_FILL_DELAY_KEY));
            }
        } catch (JSONException e) {
            Log.e(TAG, "Error parsing SpeedBumpConfiguration; returning default values", e);
        }

        // If an error is encountered or the JSON does not contain an entry for speed bump
        // configuration, then return default values.
        return getDefaultSpeedBumpConfiguration();
    }

    private SpeedBumpConfiguration getDefaultSpeedBumpConfiguration() {
        return new SpeedBumpConfiguration(
                DEFAULT_SPEED_BUMP_ACQUIRED_PERMITS_PER_SECOND,
                DEFAULT_SPEED_BUMP_MAX_PERMIT_POOL,
                DEFAULT_SPEED_BUMP_PERMIT_FILL_DELAY);
    }
}
