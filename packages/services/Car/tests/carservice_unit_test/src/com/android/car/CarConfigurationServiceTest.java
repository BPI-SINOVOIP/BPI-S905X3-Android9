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

import static com.android.car.CarConfigurationService.DEFAULT_SPEED_BUMP_ACQUIRED_PERMITS_PER_SECOND;
import static com.android.car.CarConfigurationService.DEFAULT_SPEED_BUMP_MAX_PERMIT_POOL;
import static com.android.car.CarConfigurationService.DEFAULT_SPEED_BUMP_PERMIT_FILL_DELAY;
import static com.android.car.CarConfigurationService.SPEED_BUMP_ACQUIRED_PERMITS_PER_SECOND_KEY;
import static com.android.car.CarConfigurationService.SPEED_BUMP_CONFIG_KEY;
import static com.android.car.CarConfigurationService.SPEED_BUMP_MAX_PERMIT_POOL_KEY;
import static com.android.car.CarConfigurationService.SPEED_BUMP_PERMIT_FILL_DELAY_KEY;

import static com.google.common.truth.Truth.assertThat;

import android.car.settings.SpeedBumpConfiguration;
import android.support.test.InstrumentationRegistry;
import android.support.test.runner.AndroidJUnit4;

import org.json.JSONException;
import org.json.JSONObject;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.MockitoAnnotations;

/**
 * Tests for {@link CarConfigurationService}.
 */
@RunWith(AndroidJUnit4.class)
public class CarConfigurationServiceTest {
    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
    }

    @Test
    public void testJsonResourceSuccessfullyRead() {
        // Use the default JsonReader to check that the resource JSON can be retrieved.
        CarConfigurationService service = new CarConfigurationService(
                InstrumentationRegistry.getTargetContext(),
                new JsonReaderImpl());
        service.init();

        assertThat(service.mConfigFile).isNotNull();

        // Default values should be stored in the JSON file as well.
        SpeedBumpConfiguration expectedConfiguration = new SpeedBumpConfiguration(
                DEFAULT_SPEED_BUMP_ACQUIRED_PERMITS_PER_SECOND,
                DEFAULT_SPEED_BUMP_MAX_PERMIT_POOL,
                DEFAULT_SPEED_BUMP_PERMIT_FILL_DELAY);

        assertThat(service.getSpeedBumpConfiguration()).isEqualTo(expectedConfiguration);
    }

    @Test
    public void testJsonFileSuccessfullyRead() {
        // Return an empty json file.
        CarConfigurationService service = new CarConfigurationService(
                InstrumentationRegistry.getTargetContext(),
                (context, resId) -> "{}");
        service.init();

        // The configuration should still be initialized.
        assertThat(service.mConfigFile).isNotNull();
    }

    @Test
    public void testNullJsonStringResultsInNullConfigFile() {
        // Return null as the string representation.
        CarConfigurationService service = new CarConfigurationService(
                InstrumentationRegistry.getTargetContext(),
                (context, resId) -> null);
        service.init();

        // No config file should be created.
        assertThat(service.mConfigFile).isNull();
    }

    @Test
    public void testSpeedBumpConfigurationSuccessfullyRead() throws JSONException {
        double acquiredPermitsPerSecond = 5d;
        double maxPermitPool = 10d;
        long permitFillDelay = 500L;

        String jsonFile = new SpeedBumpJsonBuilder()
                .setAcquiredPermitsPerSecond(acquiredPermitsPerSecond)
                .setMaxPermitPool(maxPermitPool)
                .setPermitFillDelay(permitFillDelay)
                .build();

        CarConfigurationService service = new CarConfigurationService(
                InstrumentationRegistry.getTargetContext(),
                (context, resId) -> jsonFile);
        service.init();

        SpeedBumpConfiguration expectedConfiguration = new SpeedBumpConfiguration(
                acquiredPermitsPerSecond,
                maxPermitPool,
                permitFillDelay);

        assertThat(service.getSpeedBumpConfiguration()).isEqualTo(expectedConfiguration);
    }

    @Test
    public void testDefaultSpeedBumpConfigurationReturned() {
        // Return null as the JSON representation.
        CarConfigurationService service = new CarConfigurationService(
                InstrumentationRegistry.getTargetContext(),
                (context, resId) -> null);
        service.init();

        // Default values should be used.
        SpeedBumpConfiguration expectedConfiguration = new SpeedBumpConfiguration(
                DEFAULT_SPEED_BUMP_ACQUIRED_PERMITS_PER_SECOND,
                DEFAULT_SPEED_BUMP_MAX_PERMIT_POOL,
                DEFAULT_SPEED_BUMP_PERMIT_FILL_DELAY);

        assertThat(service.getSpeedBumpConfiguration()).isEqualTo(expectedConfiguration);
    }

    /**
     * Builder for a string that represents the contents of a JSON file with speed bump
     * configuration.
     */
    private class SpeedBumpJsonBuilder {
        private double mAcquiredPermitsPerSecond;
        private double mMaxPermitPool;
        private long mPermitFillDelay;

        SpeedBumpJsonBuilder setAcquiredPermitsPerSecond(double acquiredPermitsPerSecond) {
            mAcquiredPermitsPerSecond = acquiredPermitsPerSecond;
            return this;
        }

        SpeedBumpJsonBuilder setMaxPermitPool(double maxPermitPool) {
            mMaxPermitPool = maxPermitPool;
            return this;
        }

        SpeedBumpJsonBuilder setPermitFillDelay(long permitFillDelay) {
            mPermitFillDelay = permitFillDelay;
            return this;
        }

        String build() throws JSONException {
            JSONObject speedBump = new JSONObject();
            speedBump.put(SPEED_BUMP_ACQUIRED_PERMITS_PER_SECOND_KEY, mAcquiredPermitsPerSecond);
            speedBump.put(SPEED_BUMP_MAX_PERMIT_POOL_KEY, mMaxPermitPool);
            speedBump.put(SPEED_BUMP_PERMIT_FILL_DELAY_KEY, mPermitFillDelay);

            JSONObject container = new JSONObject();
            container.put(SPEED_BUMP_CONFIG_KEY, speedBump);

            return container.toString();
        }
    }
}
