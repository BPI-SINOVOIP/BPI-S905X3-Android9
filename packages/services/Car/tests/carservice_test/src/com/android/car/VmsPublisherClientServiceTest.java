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

package com.android.car;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import android.car.VehicleAreaType;
import android.car.vms.VmsLayer;
import android.hardware.automotive.vehicle.V2_0.VehiclePropValue;
import android.hardware.automotive.vehicle.V2_0.VehicleProperty;
import android.hardware.automotive.vehicle.V2_0.VehiclePropertyAccess;
import android.hardware.automotive.vehicle.V2_0.VehiclePropertyChangeMode;
import android.hardware.automotive.vehicle.V2_0.VmsBaseMessageIntegerValuesIndex;
import android.hardware.automotive.vehicle.V2_0.VmsMessageType;
import android.hardware.automotive.vehicle.V2_0.VmsMessageWithLayerIntegerValuesIndex;
import android.support.test.filters.MediumTest;
import android.support.test.runner.AndroidJUnit4;
import android.util.Log;

import com.android.car.vehiclehal.VehiclePropValueBuilder;
import com.android.car.vehiclehal.test.MockedVehicleHal;
import com.android.car.vehiclehal.test.MockedVehicleHal.VehicleHalPropertyHandler;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

@RunWith(AndroidJUnit4.class)
@MediumTest
public class VmsPublisherClientServiceTest extends MockedCarTestBase {
    private static final String TAG = "VmsPublisherTest";
    private static final int MOCK_PUBLISHER_LAYER_ID = 12;
    private static final int MOCK_PUBLISHER_LAYER_VERSION = 34;
    private static final int MOCK_PUBLISHER_LAYER_SUBTYPE = 56;
    public static final int MOCK_PUBLISHER_ID = 1234;
    public static final VmsLayer MOCK_PUBLISHER_LAYER =
            new VmsLayer(MOCK_PUBLISHER_LAYER_ID,
                    MOCK_PUBLISHER_LAYER_SUBTYPE,
                    MOCK_PUBLISHER_LAYER_VERSION);
    public static final byte[] PAYLOAD = new byte[]{1, 1, 2, 3, 5, 8, 13};

    private HalHandler mHalHandler;
    // Used to block until the HAL property is updated in HalHandler.onPropertySet.
    private Semaphore mHalHandlerSemaphore;

    @Override
    protected synchronized void configureMockedHal() {
        mHalHandler = new HalHandler();
        addProperty(VehicleProperty.VEHICLE_MAP_SERVICE, mHalHandler)
                .setChangeMode(VehiclePropertyChangeMode.ON_CHANGE)
                .setAccess(VehiclePropertyAccess.READ_WRITE)
                .addAreaConfig(VehicleAreaType.VEHICLE_AREA_TYPE_GLOBAL, 0, 0);
    }

    @Override
    protected synchronized void configureResourceOverrides(MockResources resources) {
        resources.overrideResource(R.array.vmsPublisherClients,
            new String[]{ getFlattenComponent(SimpleVmsPublisherClientService.class) });
    }

    private VehiclePropValue getHalSubscriptionRequest() {
        return VehiclePropValueBuilder.newBuilder(VehicleProperty.VEHICLE_MAP_SERVICE)
                .addIntValue(VmsMessageType.SUBSCRIBE)
                .addIntValue(MOCK_PUBLISHER_LAYER_ID)
                .addIntValue(MOCK_PUBLISHER_LAYER_SUBTYPE)
                .addIntValue(MOCK_PUBLISHER_LAYER_VERSION)
                .build();
    }

    @Override
    public void setUp() throws Exception {
        /**
         * First init the semaphore, setUp will start a series of events that will ultimately
         * update the HAL layer and release this semaphore.
         */
        mHalHandlerSemaphore = new Semaphore(0);
        super.setUp();

        // Inject a subscribe event which simulates the HAL is subscribed to the Mock Publisher.
        MockedVehicleHal mHal = getMockedVehicleHal();
        mHal.injectEvent(getHalSubscriptionRequest());
    }

    /**
     * The method setUp initializes all the Car services, including the VmsPublisherService.
     * The VmsPublisherService will start and configure its list of clients. This list was
     * overridden in the method getCarServiceContext.
     * Therefore, only SimpleVmsPublisherClientService will be started.
     * The service SimpleVmsPublisherClientService will publish one message, which is validated in
     * this test.
     */
    @Test
    public void testPublish() throws Exception {
        //TODO: This test is using minial synchronisation between clients.
        //      If more complexity is added this may result in publisher
        //      publishing before the subscriber subscribed, in which case
        //      the semaphore will not be released.
        assertTrue(mHalHandlerSemaphore.tryAcquire(2L, TimeUnit.SECONDS));
        VehiclePropValue.RawValue rawValue = mHalHandler.getValue().value;
        int messageType = rawValue.int32Values.get(VmsMessageWithLayerIntegerValuesIndex.MESSAGE_TYPE);
        int layerId = rawValue.int32Values.get(VmsMessageWithLayerIntegerValuesIndex.LAYER_TYPE);
        int layerVersion = rawValue.int32Values.get(VmsMessageWithLayerIntegerValuesIndex.LAYER_VERSION);
        byte[] payload = new byte[rawValue.bytes.size()];
        for (int i = 0; i < rawValue.bytes.size(); ++i) {
            payload[i] = rawValue.bytes.get(i);
        }
        assertEquals(VmsMessageType.DATA, messageType);
        assertEquals(MOCK_PUBLISHER_LAYER_ID, layerId);
        assertEquals(MOCK_PUBLISHER_LAYER_VERSION, layerVersion);
        assertTrue(Arrays.equals(PAYLOAD, payload));
    }

    private class HalHandler implements VehicleHalPropertyHandler {
        private VehiclePropValue mValue;

        @Override
        public synchronized void onPropertySet(VehiclePropValue value) {
            mValue = value;

            // If this is the data message release the semaphore so the test can continue.
            ArrayList<Integer> int32Values = value.value.int32Values;
            if (int32Values.get(VmsBaseMessageIntegerValuesIndex.MESSAGE_TYPE) ==
                    VmsMessageType.DATA) {
                mHalHandlerSemaphore.release();
            }
        }

        @Override
        public synchronized VehiclePropValue onPropertyGet(VehiclePropValue value) {
            return mValue != null ? mValue : value;
        }

        @Override
        public synchronized void onPropertySubscribe(int property, float sampleRate) {
            Log.d(TAG, "onPropertySubscribe property " + property + " sampleRate " + sampleRate);
        }

        @Override
        public synchronized void onPropertyUnsubscribe(int property) {
            Log.d(TAG, "onPropertyUnSubscribe property " + property);
        }

        public VehiclePropValue getValue() {
            return mValue;
        }
    }
}