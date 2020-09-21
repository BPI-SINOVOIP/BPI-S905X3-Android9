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

import android.car.Car;
import android.car.VehicleAreaType;
import android.car.vms.VmsAssociatedLayer;
import android.car.vms.VmsAvailableLayers;
import android.car.vms.VmsLayer;
import android.car.vms.VmsSubscriberManager;
import android.hardware.automotive.vehicle.V2_0.VehicleProperty;
import android.hardware.automotive.vehicle.V2_0.VehiclePropertyAccess;
import android.hardware.automotive.vehicle.V2_0.VehiclePropertyChangeMode;
import android.support.test.filters.MediumTest;
import android.support.test.runner.AndroidJUnit4;

import com.android.car.vehiclehal.test.MockedVehicleHal;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.concurrent.Executor;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

@RunWith(AndroidJUnit4.class)
@MediumTest
public class VmsPublisherSubscriberTest extends MockedCarTestBase {
    private static final int LAYER_ID = 88;
    private static final int LAYER_VERSION = 19;
    private static final int LAYER_SUBTYPE = 55;
    private static final String TAG = "VmsPubSubTest";

    // The expected publisher ID is 0 since it the expected assigned ID from the VMS core.
    public static final int EXPECTED_PUBLISHER_ID = 0;
    public static final VmsLayer LAYER = new VmsLayer(LAYER_ID, LAYER_SUBTYPE, LAYER_VERSION);
    public static final VmsAssociatedLayer ASSOCIATED_LAYER =
            new VmsAssociatedLayer(LAYER, new HashSet<>(Arrays.asList(EXPECTED_PUBLISHER_ID)));
    public static final byte[] PAYLOAD = new byte[]{2, 3, 5, 7, 11, 13, 17};

    private static final List<VmsAssociatedLayer> AVAILABLE_ASSOCIATED_LAYERS =
            new ArrayList<>(Arrays.asList(ASSOCIATED_LAYER));
    private static final VmsAvailableLayers AVAILABLE_LAYERS_WITH_SEQ =
            new VmsAvailableLayers(
                    new HashSet(AVAILABLE_ASSOCIATED_LAYERS), 1);


    private static final int SUBSCRIBED_LAYER_ID = 89;
    public static final VmsLayer SUBSCRIBED_LAYER =
            new VmsLayer(SUBSCRIBED_LAYER_ID, LAYER_SUBTYPE, LAYER_VERSION);
    public static final VmsAssociatedLayer ASSOCIATED_SUBSCRIBED_LAYER =
            new VmsAssociatedLayer(SUBSCRIBED_LAYER,
                    new HashSet<>(Arrays.asList(EXPECTED_PUBLISHER_ID)));
    private static final List<VmsAssociatedLayer>
            AVAILABLE_ASSOCIATED_LAYERS_WITH_SUBSCRIBED_LAYER =
            new ArrayList<>(Arrays.asList(ASSOCIATED_LAYER, ASSOCIATED_SUBSCRIBED_LAYER));
    private static final VmsAvailableLayers AVAILABLE_LAYERS_WITH_SUBSCRIBED_LAYER_WITH_SEQ =
            new VmsAvailableLayers(
                    new HashSet(AVAILABLE_ASSOCIATED_LAYERS_WITH_SUBSCRIBED_LAYER), 1);
    VmsSubscriberManager mVmsSubscriberManager;
    TestClientCallback mClientCallback;
    private HalHandler mHalHandler;
    // Used to block until a value is propagated to the TestClientCallback.onVmsMessageReceived.
    private Semaphore mSubscriberSemaphore;
    private Semaphore mAvailabilitySemaphore;
    private Executor mExecutor;

    private class ThreadPerTaskExecutor implements Executor {
        public void execute(Runnable r) {
            new Thread(r).start();
        }
    }


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
        resources.overrideResource(com.android.car.R.array.vmsPublisherClients,
                new String[]{getFlattenComponent(VmsPublisherClientMockService.class)});
    }

    /*
     * The method setUp initializes all the Car services, including the VmsPublisherService.
     * The VmsPublisherService will start and configure its list of clients. This list was
     * overridden in the method getCarServiceContext. Therefore, only VmsPublisherClientMockService
     * will be started. Method setUp() subscribes to a layer and triggers
     * VmsPublisherClientMockService.onVmsSubscriptionChange.
     * There is a race condition between publisher and subscriber starting time.
     * So we wanted to make sure that the tests start only after the publisher is up. We achieve
     * this by subscribing in setUp() and waiting on the semaphore at the beginning of each test
     * case.
     */

    @Override
    public void setUp() throws Exception {
        mExecutor = new ThreadPerTaskExecutor();
        super.setUp();
        mSubscriberSemaphore = new Semaphore(0);
        mAvailabilitySemaphore = new Semaphore(0);

        mVmsSubscriberManager = (VmsSubscriberManager) getCar().getCarManager(
                Car.VMS_SUBSCRIBER_SERVICE);
        mClientCallback = new TestClientCallback();
        mVmsSubscriberManager.setVmsSubscriberClientCallback(mExecutor, mClientCallback);
        mVmsSubscriberManager.subscribe(LAYER);
    }

    public void postSetup() throws Exception {
        assertTrue(mAvailabilitySemaphore.tryAcquire(2L, TimeUnit.SECONDS));
    }

    /*
     * This test method subscribes to a layer and triggers
     * VmsPublisherClientMockService.onVmsSubscriptionChange. In turn, the mock service will publish
     * a message, which is validated in this test.
     */

    @Test
    public void testPublisherToSubscriber() throws Exception {
        postSetup();
        assertEquals(LAYER, mClientCallback.getLayer());
        assertTrue(Arrays.equals(PAYLOAD, mClientCallback.getPayload()));
    }

    /**
     * The Mock service will get a publisher ID by sending its information when it will get
     * ServiceReady as well as on SubscriptionChange. Since clients are not notified when
     * publishers are assigned IDs, this test waits until the availability is changed which
     * indicates
     * that the Mock service has gotten its ServiceReady and publisherId.
     */


    @Test
    public void testPublisherInfo() throws Exception {
        postSetup();

        // Inject a value and wait for its callback in TestClientCallback.onVmsMessageReceived.
        byte[] info = mVmsSubscriberManager.getPublisherInfo(EXPECTED_PUBLISHER_ID);
        assertTrue(Arrays.equals(PAYLOAD, info));
    }

    /*
     * The Mock service offers all the subscribed layers as available layers.
     * In this test the client subscribes to a layer and verifies that it gets the
     * notification that it is available.
     */

    @Test
    public void testAvailabilityWithSubscription() throws Exception {
        postSetup();
        mVmsSubscriberManager.subscribe(SUBSCRIBED_LAYER);
        assertTrue(mAvailabilitySemaphore.tryAcquire(2L, TimeUnit.SECONDS));

        final Set<VmsAssociatedLayer> associatedLayers =
                AVAILABLE_LAYERS_WITH_SUBSCRIBED_LAYER_WITH_SEQ.getAssociatedLayers();
        assertEquals(associatedLayers, mClientCallback.getAvailableLayers().getAssociatedLayers());
        assertEquals(associatedLayers,
                mVmsSubscriberManager.getAvailableLayers().getAssociatedLayers());
    }

    private class HalHandler implements MockedVehicleHal.VehicleHalPropertyHandler {
    }

    private class TestClientCallback implements VmsSubscriberManager.VmsSubscriberClientCallback {
        private VmsLayer mLayer;
        private byte[] mPayload;
        private VmsAvailableLayers mAvailableLayers;

        @Override
        public void onVmsMessageReceived(VmsLayer layer, byte[] payload) {
            assertEquals(LAYER, layer);
            assertTrue(Arrays.equals(PAYLOAD, payload));
            mLayer = layer;
            mPayload = payload;
            mSubscriberSemaphore.release();
        }

        @Override
        public void onLayersAvailabilityChanged(VmsAvailableLayers availableLayers) {
            mAvailableLayers = availableLayers;
            mAvailabilitySemaphore.release();
        }

        public VmsLayer getLayer() {
            return mLayer;
        }

        public byte[] getPayload() {
            return mPayload;
        }

        public VmsAvailableLayers getAvailableLayers() {
            return mAvailableLayers;
        }
    }
}
