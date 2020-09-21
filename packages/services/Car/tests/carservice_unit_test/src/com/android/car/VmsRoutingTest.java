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

import android.car.vms.IVmsSubscriberClient;
import android.car.vms.VmsAssociatedLayer;
import android.car.vms.VmsAvailableLayers;
import android.car.vms.VmsLayer;
import android.car.vms.VmsSubscriptionState;
import android.test.AndroidTestCase;
import android.test.suitebuilder.annotation.SmallTest;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.Map;

@SmallTest
public class VmsRoutingTest extends AndroidTestCase {
    private static VmsLayer LAYER_WITH_SUBSCRIPTION_1 = new VmsLayer(1, 1, 2);
    private static VmsLayer LAYER_WITH_SUBSCRIPTION_2 = new VmsLayer(1, 3, 3);
    private static VmsLayer LAYER_WITHOUT_SUBSCRIPTION =
            new VmsLayer(1, 7, 4);
    private static int PUBLISHER_ID_1 = 123;
    private static int PUBLISHER_ID_2 = 456;
    private static int PUBLISHER_ID_UNLISTED = 789;
    private VmsRouting mRouting;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mRouting = new VmsRouting();
    }

    public void testAddingSubscribersAndHalLayersNoOverlap() throws Exception {
        // Add a subscription to a layer.
        MockVmsSubscriber subscriber = new MockVmsSubscriber();
        mRouting.addSubscription(subscriber, LAYER_WITH_SUBSCRIPTION_1);

        // Add a HAL subscription.
        mRouting.addHalSubscription(LAYER_WITH_SUBSCRIPTION_2);

        // Verify expected subscriptions are in routing manager.
        Set<VmsLayer> expectedSubscriptions = new HashSet<>();
        expectedSubscriptions.add(LAYER_WITH_SUBSCRIPTION_1);
        expectedSubscriptions.add(LAYER_WITH_SUBSCRIPTION_2);
        VmsSubscriptionState subscriptionState = mRouting.getSubscriptionState();
        assertEquals(2, subscriptionState.getSequenceNumber());
        assertEquals(expectedSubscriptions,
                new HashSet<>(subscriptionState.getLayers()));

        // Verify there is only a single subscriber.
        assertEquals(1,
                mRouting.getSubscribersForLayerFromPublisher(
                        LAYER_WITH_SUBSCRIPTION_1, PUBLISHER_ID_1).size());
    }

    public void testAddingSubscribersAndHalLayersWithOverlap() throws Exception {
        // Add a subscription to a layer.
        MockVmsSubscriber subscriber = new MockVmsSubscriber();
        mRouting.addSubscription(subscriber, LAYER_WITH_SUBSCRIPTION_1);
        mRouting.addSubscription(subscriber, LAYER_WITH_SUBSCRIPTION_2);

        // Add a HAL subscription to a layer there is already another subscriber for.
        mRouting.addHalSubscription(LAYER_WITH_SUBSCRIPTION_2);

        // Verify expected subscriptions are in routing manager.
        Set<VmsLayer> expectedSubscriptions = new HashSet<>();
        expectedSubscriptions.add(LAYER_WITH_SUBSCRIPTION_1);
        expectedSubscriptions.add(LAYER_WITH_SUBSCRIPTION_2);
        VmsSubscriptionState subscriptionState = mRouting.getSubscriptionState();
        assertEquals(3, subscriptionState.getSequenceNumber());
        assertEquals(expectedSubscriptions,
                new HashSet<>(subscriptionState.getLayers()));
    }

    public void testAddingAndRemovingLayers() throws Exception {
        // Add a subscription to a layer.
        MockVmsSubscriber subscriber = new MockVmsSubscriber();
        mRouting.addSubscription(subscriber, LAYER_WITH_SUBSCRIPTION_1);

        // Add a HAL subscription.
        mRouting.addHalSubscription(LAYER_WITH_SUBSCRIPTION_2);

        // Remove a subscription to a layer.
        mRouting.removeSubscription(subscriber, LAYER_WITH_SUBSCRIPTION_1);

        // Update the HAL subscription
        mRouting.removeHalSubscription(LAYER_WITH_SUBSCRIPTION_2);

        // Verify there are no subscribers in the routing manager.
        VmsSubscriptionState subscriptionState = mRouting.getSubscriptionState();
        assertEquals(4, subscriptionState.getSequenceNumber());
        assertTrue(subscriptionState.getLayers().isEmpty());
    }

    public void testAddingBothTypesOfSubscribers() throws Exception {
        // Add a subscription to a layer.
        MockVmsSubscriber subscriberForLayer = new MockVmsSubscriber();
        mRouting.addSubscription(subscriberForLayer, LAYER_WITH_SUBSCRIPTION_1);

        // Add a subscription without a layer.
        MockVmsSubscriber subscriberWithoutLayer = new MockVmsSubscriber();
        mRouting.addSubscription(subscriberWithoutLayer);

        // Verify 2 subscribers for the layer.
        assertEquals(2,
                mRouting.getSubscribersForLayerFromPublisher(
                        LAYER_WITH_SUBSCRIPTION_1, PUBLISHER_ID_1).size());

        // Add the subscriber with layer as also a subscriber without layer
        mRouting.addSubscription(subscriberForLayer);

        // The number of subscribers for the layer should remain the same as before.
        assertEquals(2,
                mRouting.getSubscribersForLayerFromPublisher(
                        LAYER_WITH_SUBSCRIPTION_1, PUBLISHER_ID_1).size());
    }

    public void testOnlyRelevantSubscribers() throws Exception {
        // Add a subscription to a layer.
        MockVmsSubscriber subscriberForLayer = new MockVmsSubscriber();
        mRouting.addSubscription(subscriberForLayer, LAYER_WITH_SUBSCRIPTION_1);

        // Add a subscription without a layer.
        MockVmsSubscriber subscriberWithoutLayer = new MockVmsSubscriber();
        mRouting.addSubscription(subscriberWithoutLayer);

        // Verify that only the subscriber without layer is returned.
        Set<MockVmsSubscriber> expectedListeneres = new HashSet<MockVmsSubscriber>();
        expectedListeneres.add(subscriberWithoutLayer);
        assertEquals(expectedListeneres,
                mRouting.getSubscribersForLayerFromPublisher(
                        LAYER_WITHOUT_SUBSCRIPTION, PUBLISHER_ID_1));
    }

    public void testAddingSubscribersAndHalLayersAndSubscribersToPublishers() throws Exception {
        // Add a subscription to a layer.
        MockVmsSubscriber subscriber = new MockVmsSubscriber();
        mRouting.addSubscription(subscriber, LAYER_WITH_SUBSCRIPTION_1);

        // Add a HAL subscription.
        mRouting.addHalSubscription(LAYER_WITH_SUBSCRIPTION_2);

        mRouting.addSubscription(subscriber, LAYER_WITH_SUBSCRIPTION_1, PUBLISHER_ID_1);
        mRouting.addSubscription(subscriber, LAYER_WITH_SUBSCRIPTION_1, PUBLISHER_ID_2);
        mRouting.addSubscription(subscriber, LAYER_WITH_SUBSCRIPTION_2, PUBLISHER_ID_2);

        // Verify expected subscriptions are in routing manager.
        Set<VmsLayer> expectedSubscriptions = new HashSet<>();
        expectedSubscriptions.add(LAYER_WITH_SUBSCRIPTION_1);
        expectedSubscriptions.add(LAYER_WITH_SUBSCRIPTION_2);

        Set<VmsAssociatedLayer> expectedSubscriptionsToPublishers = new HashSet<>();
        expectedSubscriptionsToPublishers.add(new VmsAssociatedLayer(LAYER_WITH_SUBSCRIPTION_1,
                new HashSet(Arrays.asList(PUBLISHER_ID_1, PUBLISHER_ID_2))));
        expectedSubscriptionsToPublishers.add(new VmsAssociatedLayer(LAYER_WITH_SUBSCRIPTION_2,
                new HashSet(Arrays.asList(PUBLISHER_ID_2))));

        VmsSubscriptionState subscriptionState = mRouting.getSubscriptionState();
        assertEquals(5, subscriptionState.getSequenceNumber());
        assertEquals(expectedSubscriptions,
                new HashSet<>(subscriptionState.getLayers()));

        assertEquals(expectedSubscriptionsToPublishers,
                subscriptionState.getAssociatedLayers());

        // Verify there is only a single subscriber.
        assertEquals(1,
                mRouting.getSubscribersForLayerFromPublisher(
                        LAYER_WITH_SUBSCRIPTION_1, PUBLISHER_ID_1).size());
    }

    public void testAddingSubscriberToPublishersAndGetListeneresToDifferentPublisher()
            throws Exception {
        // Add a subscription to a layer.
        MockVmsSubscriber subscriber = new MockVmsSubscriber();

        mRouting.addSubscription(subscriber, LAYER_WITH_SUBSCRIPTION_2);
        mRouting.addSubscription(subscriber, LAYER_WITH_SUBSCRIPTION_1, PUBLISHER_ID_1);

        Set<IVmsSubscriberClient> subscribers;
        // Need to route a layer 1 message from publisher 2 so there are no subscribers.
        subscribers =
                mRouting.getSubscribersForLayerFromPublisher(
                        LAYER_WITH_SUBSCRIPTION_1,
                        PUBLISHER_ID_2);
        assertEquals(0, subscribers.size());

        // Need to route a layer 1 message from publisher 1 so there is one subscriber.
        subscribers =
                mRouting.getSubscribersForLayerFromPublisher(
                        LAYER_WITH_SUBSCRIPTION_1,
                        PUBLISHER_ID_1);
        assertEquals(1, subscribers.size());
        assertTrue(subscribers.contains(subscriber));

        // Verify all the messages for LAYER_WITH_SUBSCRIPTION_2 have subscribers since the
        // subscription was done without specifying a specific publisher.
        subscribers =
                mRouting.getSubscribersForLayerFromPublisher(
                        LAYER_WITH_SUBSCRIPTION_2,
                        PUBLISHER_ID_UNLISTED);
        assertEquals(1, subscribers.size());
        assertTrue(subscribers.contains(subscriber));
    }


    public void testRemovalOfSubscribersToPublishers() throws Exception {
        // Add a subscription to a layer.
        MockVmsSubscriber subscriber = new MockVmsSubscriber();
        mRouting.addSubscription(subscriber, LAYER_WITH_SUBSCRIPTION_1);

        // Add a HAL subscription.
        mRouting.addHalSubscription(LAYER_WITH_SUBSCRIPTION_2);

        mRouting.addSubscription(subscriber, LAYER_WITH_SUBSCRIPTION_1, PUBLISHER_ID_1);
        mRouting.addSubscription(subscriber, LAYER_WITH_SUBSCRIPTION_1, PUBLISHER_ID_2);
        mRouting.addSubscription(subscriber, LAYER_WITH_SUBSCRIPTION_2, PUBLISHER_ID_2);
        mRouting.removeSubscription(subscriber, LAYER_WITH_SUBSCRIPTION_2, PUBLISHER_ID_2);

        // Verify expected subscriptions are in routing manager.
        Set<VmsLayer> expectedSubscriptions = new HashSet<>();
        expectedSubscriptions.add(LAYER_WITH_SUBSCRIPTION_1);
        expectedSubscriptions.add(LAYER_WITH_SUBSCRIPTION_2);


        Set<VmsAssociatedLayer> expectedSubscriptionsToPublishers = new HashSet<>();
        expectedSubscriptionsToPublishers.add(new VmsAssociatedLayer(LAYER_WITH_SUBSCRIPTION_1,
                new HashSet(Arrays.asList(PUBLISHER_ID_1, PUBLISHER_ID_2))));

        VmsSubscriptionState subscriptionState = mRouting.getSubscriptionState();
        assertEquals(6, subscriptionState.getSequenceNumber());
        assertEquals(expectedSubscriptions,
                new HashSet<>(subscriptionState.getLayers()));

        assertEquals(expectedSubscriptionsToPublishers,
                subscriptionState.getAssociatedLayers());

        // Verify there is only a single subscriber.
        assertEquals(1,
                mRouting.getSubscribersForLayerFromPublisher(
                        LAYER_WITH_SUBSCRIPTION_1, PUBLISHER_ID_1).size());
    }

    public void testRemovalOfSubscribersToPublishersClearListForPublisher() throws Exception {
        // Add a subscription to a layer.
        MockVmsSubscriber subscriber = new MockVmsSubscriber();
        mRouting.addSubscription(subscriber, LAYER_WITH_SUBSCRIPTION_1);

        // Add a HAL subscription.
        mRouting.addHalSubscription(LAYER_WITH_SUBSCRIPTION_2);

        mRouting.addSubscription(subscriber, LAYER_WITH_SUBSCRIPTION_1, PUBLISHER_ID_1);
        mRouting.addSubscription(subscriber, LAYER_WITH_SUBSCRIPTION_1, PUBLISHER_ID_2);
        mRouting.addSubscription(subscriber, LAYER_WITH_SUBSCRIPTION_2, PUBLISHER_ID_2);
        mRouting.removeSubscription(subscriber, LAYER_WITH_SUBSCRIPTION_1, PUBLISHER_ID_1);

        // Verify expected subscriptions are in routing manager.
        Set<VmsLayer> expectedSubscriptions = new HashSet<>();
        expectedSubscriptions.add(LAYER_WITH_SUBSCRIPTION_1);
        expectedSubscriptions.add(LAYER_WITH_SUBSCRIPTION_2);

        Set<VmsAssociatedLayer> expectedSubscriptionsToPublishers = new HashSet<>();
        expectedSubscriptionsToPublishers.add(new VmsAssociatedLayer(LAYER_WITH_SUBSCRIPTION_1,
                new HashSet(Arrays.asList(PUBLISHER_ID_2))));
        expectedSubscriptionsToPublishers.add(new VmsAssociatedLayer(LAYER_WITH_SUBSCRIPTION_2,
                new HashSet(Arrays.asList(PUBLISHER_ID_2))));

        VmsSubscriptionState subscriptionState = mRouting.getSubscriptionState();
        assertEquals(6, subscriptionState.getSequenceNumber());
        assertEquals(expectedSubscriptions,
                new HashSet<>(subscriptionState.getLayers()));

        assertEquals(expectedSubscriptionsToPublishers,
                subscriptionState.getAssociatedLayers());

        // Verify there is only a single subscriber.
        assertEquals(1,
                mRouting.getSubscribersForLayerFromPublisher(
                        LAYER_WITH_SUBSCRIPTION_1, PUBLISHER_ID_1).size());
    }

    class MockVmsSubscriber extends IVmsSubscriberClient.Stub {
        @Override
        public void onVmsMessageReceived(VmsLayer layer, byte[] payload) {
        }

        @Override
        public void onLayersAvailabilityChanged(VmsAvailableLayers availableLayers) {
        }
    }
}