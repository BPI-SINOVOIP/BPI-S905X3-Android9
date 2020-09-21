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
package com.android.car.vehiclehal.test;

import static org.junit.Assert.assertTrue;

import static java.lang.Integer.toHexString;

import android.car.Car;
import android.car.hardware.CarPropertyConfig;
import android.car.hardware.CarPropertyValue;
import android.car.hardware.hvac.CarHvacManager;
import android.car.hardware.hvac.CarHvacManager.CarHvacEventCallback;
import android.hardware.automotive.vehicle.V2_0.VehiclePropValue;
import android.hardware.automotive.vehicle.V2_0.VehicleProperty;
import android.support.test.filters.LargeTest;
import android.support.test.runner.AndroidJUnit4;
import android.util.ArraySet;
import android.util.Log;

import org.junit.Assert;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.File;
import java.time.Duration;
import java.util.Set;


/**
 * The test suite will execute end-to-end Car HVAC API test by generating HVAC property data from
 * default VHAL and verify those data on the fly. The test data is coming from assets/ folder in the
 * test APK and will be shared with VHAL to execute the test.
 */
@RunWith(AndroidJUnit4.class)
@LargeTest
public class CarHvacTest extends E2eCarTestBase {
    private static final String TAG = Utils.concatTag(CarHvacTest.class);

    // Test should be completed within 1 hour as it only covers a finite set of HVAC properties
    private static final Duration TEST_TIME_OUT = Duration.ofHours(1);

    private static final String CAR_HVAC_TEST_JSON = "car_hvac_test.json";

    // Referred to hardware/interfaces/automotive/vehicle/2.0/types.hal, some of the HVAC properties
    // are in CONTINUOUS mode. They should be omitted when testing ON_CHANGE properties.
    private static final Set<Integer> CONTINUOUS_HVAC_PROPS;

    private Integer mNumPropEventsToSkip;

    static {
        CONTINUOUS_HVAC_PROPS = new ArraySet<>();
        CONTINUOUS_HVAC_PROPS.add(VehicleProperty.ENV_OUTSIDE_TEMPERATURE);
    }


    private class CarHvacOnChangeEventListener implements CarHvacEventCallback {
        private VhalEventVerifier mVerifier;

        CarHvacOnChangeEventListener(VhalEventVerifier verifier) {
            mVerifier = verifier;
        }

        @Override
        public void onChangeEvent(CarPropertyValue carPropertyValue) {
            VehiclePropValue event = Utils.fromHvacPropertyValue(carPropertyValue);
            if (!CONTINUOUS_HVAC_PROPS.contains(event.prop)) {
                synchronized (mNumPropEventsToSkip) {
                    if (mNumPropEventsToSkip == 0) {
                        mVerifier.verify(Utils.fromHvacPropertyValue(carPropertyValue));
                    } else {
                        mNumPropEventsToSkip--;
                    }
                }
            }
        }

        @Override
        public void onErrorEvent(final int propertyId, final int zone) {
            Assert.fail("Error: propertyId=" + toHexString(propertyId) + " zone=" + zone);
        }
    }

    private Integer calculateNumPropEventsToSkip(CarHvacManager hvacMgr) {
        int numToSkip = 0;
        try {
            for (CarPropertyConfig c: hvacMgr.getPropertyList()) {
                if (!CONTINUOUS_HVAC_PROPS.contains(c.getPropertyId())) {
                    numToSkip += c.getAreaCount();
                }
            }
        } catch (Exception e) {
            Log.d(TAG, "Unhandled exception thrown: ", e);
        }
        return Integer.valueOf(numToSkip);
    }
    @Test
    public void testHvacOperations() throws Exception {
        Log.d(TAG, "Prepare HVAC test data");
        VhalEventVerifier verifier = new VhalEventVerifier(getExpectedEvents(CAR_HVAC_TEST_JSON));
        File sharedJson = makeShareable(CAR_HVAC_TEST_JSON);

        Log.d(TAG, "Start listening to the HAL");
        CarHvacManager hvacMgr = (CarHvacManager) mCar.getCarManager(Car.HVAC_SERVICE);
        // Calculate number of properties to skip due to registration event
        mNumPropEventsToSkip = calculateNumPropEventsToSkip(hvacMgr);
        CarHvacEventCallback callback = new CarHvacOnChangeEventListener(verifier);
        hvacMgr.registerCallback(callback);

        Log.d(TAG, "Send command to VHAL to start generation");
        VhalEventGenerator hvacGenerator =
                new JsonVhalEventGenerator(mVehicle).setJsonFile(sharedJson);
        hvacGenerator.start();

        Log.d(TAG, "Receiving and verifying VHAL events");
        verifier.waitForEnd(TEST_TIME_OUT.toMillis());

        Log.d(TAG, "Send command to VHAL to stop generation");
        hvacGenerator.stop();
        hvacMgr.unregisterCallback(callback);

        assertTrue("Detected mismatched events: " + verifier.getResultString(),
                    verifier.getMismatchedEvents().isEmpty());
    }
}
