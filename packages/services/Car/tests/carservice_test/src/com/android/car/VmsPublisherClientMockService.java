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

import android.car.vms.VmsLayer;
import android.car.vms.VmsLayerDependency;
import android.car.vms.VmsLayersOffering;
import android.car.vms.VmsPublisherClientService;
import android.car.vms.VmsSubscriptionState;
import android.util.Log;

import java.util.HashSet;
import java.util.Set;

/**
 * This service is launched during the tests in VmsPublisherSubscriberTest. It publishes a property
 * that is going to be verified in the test.
 *
 * The service makes offering for pre-defined layers which verifies availability notifications for
 * subscribers without them actively being subscribed to a layer, and also echos all the
 * subscription requests with an offering for that layer.
 * For example, without any subscription request from any client, this service will make offering
 * to layer X. If a client will subscribe later to layer Y, this service will respond with offering
 * to both layers X and Y.
 *
 * Note that the subscriber can subscribe before the publisher finishes initialization. To cover
 * both potential scenarios, this service publishes the test message in onVmsSubscriptionChange
 * and in onVmsPublisherServiceReady. See comments below.
 */
public class VmsPublisherClientMockService extends VmsPublisherClientService {
    private static final String TAG = "VmsPublisherClientMockService";

    @Override
    public void onVmsSubscriptionChange(VmsSubscriptionState subscriptionState) {
        // Case when the publisher finished initialization before the subscription request.
        initializeMockPublisher(subscriptionState);
    }

    @Override
    public void onVmsPublisherServiceReady() {
        // Case when the subscription request was sent before the publisher was ready.
        VmsSubscriptionState subscriptionState = getSubscriptions();
        initializeMockPublisher(subscriptionState);
    }

    private void initializeMockPublisher(VmsSubscriptionState subscriptionState) {
        Log.d(TAG, "Initializing Mock publisher");
        int publisherId = getPublisherId(VmsPublisherSubscriberTest.PAYLOAD);
        publishIfNeeded(subscriptionState);
        declareOffering(subscriptionState, publisherId);
    }

    private void publishIfNeeded(VmsSubscriptionState subscriptionState) {
        for (VmsLayer layer : subscriptionState.getLayers()) {
            if (layer.equals(VmsPublisherSubscriberTest.LAYER)) {
                publish(VmsPublisherSubscriberTest.LAYER,
                        VmsPublisherSubscriberTest.EXPECTED_PUBLISHER_ID,
                        VmsPublisherSubscriberTest.PAYLOAD);
            }
        }
    }

    private void declareOffering(VmsSubscriptionState subscriptionState, int publisherId) {
        Set<VmsLayerDependency> dependencies = new HashSet<>();

        // Add all layers from the subscription state.
        for( VmsLayer layer : subscriptionState.getLayers()) {
            dependencies.add(new VmsLayerDependency(layer));
        }

        // Add default test layers.
        dependencies.add(new VmsLayerDependency(VmsPublisherSubscriberTest.LAYER));

        VmsLayersOffering offering = new VmsLayersOffering(dependencies, publisherId);
        setLayersOffering(offering);
    }
}
