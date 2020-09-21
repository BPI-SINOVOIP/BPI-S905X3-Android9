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
import android.car.vms.VmsLayer;
import android.car.vms.VmsOperationRecorder;
import android.car.vms.VmsSubscriptionState;

import com.android.internal.annotations.GuardedBy;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.stream.Collectors;

/**
 * Manages all the VMS subscriptions:
 * + Subscriptions to data messages of individual layer + version.
 * + Subscriptions to all data messages.
 * + HAL subscriptions to layer + version.
 */

public class VmsRouting {
    private final Object mLock = new Object();
    // A map of Layer + Version to subscribers.
    @GuardedBy("mLock")
    private Map<VmsLayer, Set<IVmsSubscriberClient>> mLayerSubscriptions = new HashMap<>();

    @GuardedBy("mLock")
    private Map<VmsLayer, Map<Integer, Set<IVmsSubscriberClient>>> mLayerSubscriptionsToPublishers =
            new HashMap<>();
    // A set of subscribers that are interested in any layer + version.
    @GuardedBy("mLock")
    private Set<IVmsSubscriberClient> mPromiscuousSubscribers = new HashSet<>();

    // A set of all the layers + versions the HAL is subscribed to.
    @GuardedBy("mLock")
    private Set<VmsLayer> mHalSubscriptions = new HashSet<>();

    @GuardedBy("mLock")
    private Map<VmsLayer, Set<Integer>> mHalSubscriptionsToPublishers = new HashMap<>();
    // A sequence number that is increased every time the subscription state is modified. Note that
    // modifying the list of promiscuous subscribers does not affect the subscription state.
    @GuardedBy("mLock")
    private int mSequenceNumber = 0;

    /**
     * Add a subscriber subscription to data messages from a VMS layer.
     *
     * @param subscriber a VMS subscriber.
     * @param layer      the layer subscribing to.
     */
    public void addSubscription(IVmsSubscriberClient subscriber, VmsLayer layer) {
        //TODO(b/36902947): revise if need to sync, and return value.
        synchronized (mLock) {
            ++mSequenceNumber;
            // Get or create the list of subscribers for layer and version.
            Set<IVmsSubscriberClient> subscribers = mLayerSubscriptions.get(layer);

            if (subscribers == null) {
                subscribers = new HashSet<>();
                mLayerSubscriptions.put(layer, subscribers);
            }
            // Add the subscriber to the list.
            subscribers.add(subscriber);
            VmsOperationRecorder.get().addSubscription(mSequenceNumber, layer);
        }
    }

    /**
     * Add a subscriber subscription to all data messages.
     *
     * @param subscriber a VMS subscriber.
     */
    public void addSubscription(IVmsSubscriberClient subscriber) {
        synchronized (mLock) {
            ++mSequenceNumber;
            mPromiscuousSubscribers.add(subscriber);
            VmsOperationRecorder.get().addPromiscuousSubscription(mSequenceNumber);
        }
    }

    /**
     * Add a subscriber subscription to data messages from a VMS layer from a specific publisher.
     *
     * @param subscriber  a VMS subscriber.
     * @param layer       the layer to subscribing to.
     * @param publisherId the publisher ID.
     */
    public void addSubscription(IVmsSubscriberClient subscriber, VmsLayer layer, int publisherId) {
        synchronized (mLock) {
            ++mSequenceNumber;

            Map<Integer, Set<IVmsSubscriberClient>> publisherIdsToSubscribersForLayer =
                    mLayerSubscriptionsToPublishers.get(layer);

            if (publisherIdsToSubscribersForLayer == null) {
                publisherIdsToSubscribersForLayer = new HashMap<>();
                mLayerSubscriptionsToPublishers.put(layer, publisherIdsToSubscribersForLayer);
            }

            Set<IVmsSubscriberClient> subscribersForPublisher =
                    publisherIdsToSubscribersForLayer.get(publisherId);

            if (subscribersForPublisher == null) {
                subscribersForPublisher = new HashSet<>();
                publisherIdsToSubscribersForLayer.put(publisherId, subscribersForPublisher);
            }

            // Add the subscriber to the list.
            subscribersForPublisher.add(subscriber);
        }
    }

    /**
     * Remove a subscription for a layer + version and make sure to remove the key if there are no
     * more subscribers.
     *
     * @param subscriber to remove.
     * @param layer      of the subscription.
     */
    public void removeSubscription(IVmsSubscriberClient subscriber, VmsLayer layer) {
        synchronized (mLock) {
            ++mSequenceNumber;
            Set<IVmsSubscriberClient> subscribers = mLayerSubscriptions.get(layer);

            // If there are no subscribers we are done.
            if (subscribers == null) {
                return;
            }
            subscribers.remove(subscriber);
            VmsOperationRecorder.get().removeSubscription(mSequenceNumber, layer);

            // If there are no more subscribers then remove the list.
            if (subscribers.isEmpty()) {
                mLayerSubscriptions.remove(layer);
            }
        }
    }

    /**
     * Remove a subscriber subscription to all data messages.
     *
     * @param subscriber a VMS subscriber.
     */
    public void removeSubscription(IVmsSubscriberClient subscriber) {
        synchronized (mLock) {
            ++mSequenceNumber;
            mPromiscuousSubscribers.remove(subscriber);
            VmsOperationRecorder.get().removePromiscuousSubscription(mSequenceNumber);
        }
    }

    /**
     * Remove a subscription to data messages from a VMS layer from a specific publisher.
     *
     * @param subscriber  a VMS subscriber.
     * @param layer       the layer to unsubscribing from.
     * @param publisherId the publisher ID.
     */
    public void removeSubscription(IVmsSubscriberClient subscriber,
                                   VmsLayer layer,
                                   int publisherId) {
        synchronized (mLock) {
            ++mSequenceNumber;

            Map<Integer, Set<IVmsSubscriberClient>> subscribersToPublishers =
                    mLayerSubscriptionsToPublishers.get(layer);

            if (subscribersToPublishers == null) {
                return;
            }

            Set<IVmsSubscriberClient> subscribers = subscribersToPublishers.get(publisherId);

            if (subscribers == null) {
                return;
            }
            subscribers.remove(subscriber);

            if (subscribers.isEmpty()) {
                subscribersToPublishers.remove(publisherId);
            }

            if (subscribersToPublishers.isEmpty()) {
                mLayerSubscriptionsToPublishers.remove(layer);
            }
        }
    }

    /**
     * Remove a subscriber from all routes (optional operation).
     *
     * @param subscriber a VMS subscriber.
     */
    public void removeDeadSubscriber(IVmsSubscriberClient subscriber) {
        synchronized (mLock) {
            // Remove the subscriber from all the routes.
            for (VmsLayer layer : mLayerSubscriptions.keySet()) {
                removeSubscription(subscriber, layer);
            }
            // Remove the subscriber from the loggers.
            removeSubscription(subscriber);
        }
    }

    /**
     * Returns a list of all the subscribers for a layer from a publisher. This includes
     * subscribers that subscribed to this layer from all publishers, subscribed to this layer
     * from a specific publisher, and the promiscuous subscribers.
     *
     * @param layer       The layer of the message.
     * @param publisherId the ID of the client that published the message to be routed.
     * @return a list of the subscribers.
     */
    public Set<IVmsSubscriberClient> getSubscribersForLayerFromPublisher(VmsLayer layer,
                                                                         int publisherId) {
        Set<IVmsSubscriberClient> subscribers = new HashSet<>();
        synchronized (mLock) {
            // Add the subscribers which explicitly subscribed to this layer
            if (mLayerSubscriptions.containsKey(layer)) {
                subscribers.addAll(mLayerSubscriptions.get(layer));
            }

            // Add the subscribers which explicitly subscribed to this layer and publisher
            if (mLayerSubscriptionsToPublishers.containsKey(layer)) {
                if (mLayerSubscriptionsToPublishers.get(layer).containsKey(publisherId)) {
                    subscribers.addAll(mLayerSubscriptionsToPublishers.get(layer).get(publisherId));
                }
            }

            // Add the promiscuous subscribers.
            subscribers.addAll(mPromiscuousSubscribers);
        }
        return subscribers;
    }

    /**
     * Returns a list with all the subscribers.
     */
    public Set<IVmsSubscriberClient> getAllSubscribers() {
        Set<IVmsSubscriberClient> subscribers = new HashSet<>();
        synchronized (mLock) {
            for (VmsLayer layer : mLayerSubscriptions.keySet()) {
                subscribers.addAll(mLayerSubscriptions.get(layer));
            }
            // Add the promiscuous subscribers.
            subscribers.addAll(mPromiscuousSubscribers);
        }
        return subscribers;
    }

    /**
     * Checks if a subscriber is subscribed to any messages.
     *
     * @param subscriber that may have subscription.
     * @return true if the subscriber uis subscribed to messages.
     */
    public boolean containsSubscriber(IVmsSubscriberClient subscriber) {
        synchronized (mLock) {
            // Check if subscriber is subscribed to a layer.
            for (Set<IVmsSubscriberClient> layerSubscribers : mLayerSubscriptions.values()) {
                if (layerSubscribers.contains(subscriber)) {
                    return true;
                }
            }
            // Check is subscriber is subscribed to all data messages.
            return mPromiscuousSubscribers.contains(subscriber);
        }
    }

    /**
     * Add a layer and version to the HAL subscriptions.
     *
     * @param layer the HAL subscribes to.
     */
    public void addHalSubscription(VmsLayer layer) {
        synchronized (mLock) {
            ++mSequenceNumber;
            mHalSubscriptions.add(layer);
            VmsOperationRecorder.get().addHalSubscription(mSequenceNumber, layer);
        }
    }

    public void addHalSubscriptionToPublisher(VmsLayer layer, int publisherId) {
        synchronized (mLock) {
            ++mSequenceNumber;

            Set<Integer> publisherIdsForLayer = mHalSubscriptionsToPublishers.get(layer);
            if (publisherIdsForLayer == null) {
                publisherIdsForLayer = new HashSet<>();
                mHalSubscriptionsToPublishers.put(layer, publisherIdsForLayer);
            }
            publisherIdsForLayer.add(publisherId);
        }
    }

    /**
     * remove a layer and version to the HAL subscriptions.
     *
     * @param layer the HAL unsubscribes from.
     */
    public void removeHalSubscription(VmsLayer layer) {
        synchronized (mLock) {
            ++mSequenceNumber;
            mHalSubscriptions.remove(layer);
            VmsOperationRecorder.get().removeHalSubscription(mSequenceNumber, layer);
        }
    }

    public void removeHalSubscriptionToPublisher(VmsLayer layer, int publisherId) {
        synchronized (mLock) {
            ++mSequenceNumber;

            Set<Integer> publisherIdsForLayer = mHalSubscriptionsToPublishers.get(layer);
            if (publisherIdsForLayer == null) {
                return;
            }
            publisherIdsForLayer.remove(publisherId);

            if (publisherIdsForLayer.isEmpty()) {
                mHalSubscriptionsToPublishers.remove(layer);
            }
        }
    }

    /**
     * checks if the HAL is subscribed to a layer.
     *
     * @param layer
     * @return true if the HAL is subscribed to layer.
     */
    public boolean isHalSubscribed(VmsLayer layer) {
        synchronized (mLock) {
            return mHalSubscriptions.contains(layer);
        }
    }

    /**
     * checks if there are subscribers to a layer.
     *
     * @param layer
     * @return true if there are subscribers to layer.
     */
    public boolean hasLayerSubscriptions(VmsLayer layer) {
        synchronized (mLock) {
            return mLayerSubscriptions.containsKey(layer) || mHalSubscriptions.contains(layer);
        }
    }

    /**
     * returns true if there is already a subscription for the layer from publisherId.
     *
     * @param layer
     * @param publisherId
     * @return
     */
    public boolean hasLayerFromPublisherSubscriptions(VmsLayer layer, int publisherId) {
        synchronized (mLock) {
            boolean hasClientSubscription =
                    mLayerSubscriptionsToPublishers.containsKey(layer) &&
                            mLayerSubscriptionsToPublishers.get(layer).containsKey(publisherId);

            boolean hasHalSubscription = mHalSubscriptionsToPublishers.containsKey(layer) &&
                    mHalSubscriptionsToPublishers.get(layer).contains(publisherId);

            return hasClientSubscription || hasHalSubscription;
        }
    }

    /**
     * @return a Set of layers and versions which VMS clients are subscribed to.
     */
    public VmsSubscriptionState getSubscriptionState() {
        synchronized (mLock) {
            Set<VmsLayer> layers = new HashSet<>();
            layers.addAll(mLayerSubscriptions.keySet());
            layers.addAll(mHalSubscriptions);


            Set<VmsAssociatedLayer> layersFromPublishers = new HashSet<>();
            layersFromPublishers.addAll(mLayerSubscriptionsToPublishers.entrySet()
                    .stream()
                    .map(e -> new VmsAssociatedLayer(e.getKey(), e.getValue().keySet()))
                    .collect(Collectors.toSet()));
            layersFromPublishers.addAll(mHalSubscriptionsToPublishers.entrySet()
                    .stream()
                    .map(e -> new VmsAssociatedLayer(e.getKey(), e.getValue()))
                    .collect(Collectors.toSet()));

            return new VmsSubscriptionState(mSequenceNumber, layers, layersFromPublishers);
        }
    }
}