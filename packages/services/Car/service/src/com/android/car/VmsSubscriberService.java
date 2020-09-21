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

import android.car.Car;
import android.car.vms.IVmsSubscriberClient;
import android.car.vms.IVmsSubscriberService;
import android.car.vms.VmsAvailableLayers;
import android.car.vms.VmsLayer;
import android.content.Context;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;

import com.android.car.hal.VmsHalService;
import com.android.internal.annotations.GuardedBy;

import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * + Receives HAL updates by implementing VmsHalService.VmsHalListener.
 * + Offers subscriber/publisher services by implementing IVmsService.Stub.
 */
public class VmsSubscriberService extends IVmsSubscriberService.Stub
        implements CarServiceBase, VmsHalService.VmsHalSubscriberListener {
    private static final boolean DBG = true;
    private static final String PERMISSION = Car.PERMISSION_VMS_SUBSCRIBER;
    private static final String TAG = "VmsSubscriberService";

    private final Context mContext;
    private final VmsHalService mHal;

    @GuardedBy("mSubscriberServiceLock")
    private final VmsSubscribersManager mSubscribersManager = new VmsSubscribersManager();
    private final Object mSubscriberServiceLock = new Object();

    /**
     * Keeps track of subscribers of this service.
     */
    class VmsSubscribersManager {
        /**
         * Allows to modify mSubscriberMap and mListenerDeathRecipientMap as a single unit.
         */
        private final Object mListenerManagerLock = new Object();
        @GuardedBy("mListenerManagerLock")
        private final Map<IBinder, ListenerDeathRecipient> mListenerDeathRecipientMap =
                new HashMap<>();
        @GuardedBy("mListenerManagerLock")
        private final Map<IBinder, IVmsSubscriberClient> mSubscriberMap = new HashMap<>();

        class ListenerDeathRecipient implements IBinder.DeathRecipient {
            private IBinder mSubscriberBinder;

            ListenerDeathRecipient(IBinder subscriberBinder) {
                mSubscriberBinder = subscriberBinder;
            }

            /**
             * Listener died. Remove it from this service.
             */
            @Override
            public void binderDied() {
                if (DBG) {
                    Log.d(TAG, "binderDied " + mSubscriberBinder);
                }

                // Get the Listener from the Binder
                IVmsSubscriberClient subscriber = mSubscriberMap.get(mSubscriberBinder);

                // Remove the subscriber subscriptions.
                if (subscriber != null) {
                    Log.d(TAG, "Removing subscriptions for dead subscriber: " + subscriber);
                    mHal.removeDeadSubscriber(subscriber);
                } else {
                    Log.d(TAG, "Handling dead binder with no matching subscriber");

                }

                // Remove binder
                VmsSubscribersManager.this.removeListener(mSubscriberBinder);
            }

            void release() {
                mSubscriberBinder.unlinkToDeath(this, 0);
            }
        }

        public void release() {
            for (ListenerDeathRecipient recipient : mListenerDeathRecipientMap.values()) {
                recipient.release();
            }
            mListenerDeathRecipientMap.clear();
            mSubscriberMap.clear();
        }

        /**
         * Adds the subscriber and a death recipient associated to it.
         *
         * @param subscriber to be added.
         * @throws IllegalArgumentException if the subscriber is null.
         * @throws IllegalStateException    if it was not possible to link a death recipient to the
         *                                  subscriber.
         */
        public void add(IVmsSubscriberClient subscriber) {
            ICarImpl.assertVmsSubscriberPermission(mContext);
            if (subscriber == null) {
                Log.e(TAG, "register: subscriber is null.");
                throw new IllegalArgumentException("subscriber cannot be null.");
            }
            if (DBG) {
                Log.d(TAG, "register: " + subscriber);
            }
            IBinder subscriberBinder = subscriber.asBinder();
            synchronized (mListenerManagerLock) {
                if (mSubscriberMap.containsKey(subscriberBinder)) {
                    // Already registered, nothing to do.
                    return;
                }
                ListenerDeathRecipient deathRecipient =
                        new ListenerDeathRecipient(subscriberBinder);
                try {
                    subscriberBinder.linkToDeath(deathRecipient, 0);
                } catch (RemoteException e) {
                    Log.e(TAG, "Failed to link death for recipient. ", e);
                    throw new IllegalStateException(Car.CAR_NOT_CONNECTED_EXCEPTION_MSG);
                }
                mListenerDeathRecipientMap.put(subscriberBinder, deathRecipient);
                mSubscriberMap.put(subscriberBinder, subscriber);
            }
        }

        /**
         * Removes the subscriber and associated death recipient.
         *
         * @param subscriber to be removed.
         * @throws IllegalArgumentException if subscriber is null.
         */
        public void remove(IVmsSubscriberClient subscriber) {
            if (DBG) {
                Log.d(TAG, "unregisterListener");
            }
            ICarImpl.assertPermission(mContext, PERMISSION);
            if (subscriber == null) {
                Log.e(TAG, "unregister: subscriber is null.");
                throw new IllegalArgumentException("Listener is null");
            }
            IBinder subscriberBinder = subscriber.asBinder();
            removeListener(subscriberBinder);
        }

        // Removes the subscriberBinder from the current state.
        // The function assumes that binder will exist both in subscriber and death recipients list.
        private void removeListener(IBinder subscriberBinder) {
            synchronized (mListenerManagerLock) {
                boolean found = mSubscriberMap.remove(subscriberBinder) != null;
                if (found) {
                    mListenerDeathRecipientMap.get(subscriberBinder).release();
                    mListenerDeathRecipientMap.remove(subscriberBinder);
                } else {
                    Log.e(TAG, "removeListener: subscriber was not previously registered.");
                }
            }
        }

        /**
         * Returns list of subscribers currently registered.
         *
         * @return list of subscribers.
         */
        public List<IVmsSubscriberClient> getListeners() {
            synchronized (mListenerManagerLock) {
                return new ArrayList<>(mSubscriberMap.values());
            }
        }
    }

    public VmsSubscriberService(Context context, VmsHalService hal) {
        mContext = context;
        mHal = hal;
    }

    // Implements CarServiceBase interface.
    @Override
    public void init() {
        mHal.addSubscriberListener(this);
    }

    @Override
    public void release() {
        mSubscribersManager.release();
        mHal.removeSubscriberListener(this);
    }

    @Override
    public void dump(PrintWriter writer) {
    }

    // Implements IVmsService interface.
    @Override
    public void addVmsSubscriberToNotifications(IVmsSubscriberClient subscriber) {
        ICarImpl.assertVmsSubscriberPermission(mContext);
        synchronized (mSubscriberServiceLock) {
            // Add the subscriber so it can subscribe.
            mSubscribersManager.add(subscriber);
        }
    }

    @Override
    public void removeVmsSubscriberToNotifications(IVmsSubscriberClient subscriber) {
        ICarImpl.assertVmsSubscriberPermission(mContext);
        synchronized (mSubscriberServiceLock) {
            if (mHal.containsSubscriber(subscriber)) {
                throw new IllegalArgumentException("Subscriber has active subscriptions.");
            }
            mSubscribersManager.remove(subscriber);
        }
    }

    @Override
    public void addVmsSubscriber(IVmsSubscriberClient subscriber, VmsLayer layer) {
        ICarImpl.assertVmsSubscriberPermission(mContext);
        synchronized (mSubscriberServiceLock) {
            // Add the subscriber so it can subscribe.
            mSubscribersManager.add(subscriber);

            // Add the subscription for the layer.
            mHal.addSubscription(subscriber, layer);
        }
    }

    @Override
    public void removeVmsSubscriber(IVmsSubscriberClient subscriber, VmsLayer layer) {
        ICarImpl.assertVmsSubscriberPermission(mContext);
        synchronized (mSubscriberServiceLock) {
            // Remove the subscription.
            mHal.removeSubscription(subscriber, layer);
        }
    }

    @Override
    public void addVmsSubscriberToPublisher(IVmsSubscriberClient subscriber,
                                            VmsLayer layer,
                                            int publisherId) {
        ICarImpl.assertVmsSubscriberPermission(mContext);
        synchronized (mSubscriberServiceLock) {
            // Add the subscriber so it can subscribe.
            mSubscribersManager.add(subscriber);

            // Add the subscription for the layer.
            mHal.addSubscription(subscriber, layer, publisherId);
        }
    }

    @Override
    public void removeVmsSubscriberToPublisher(IVmsSubscriberClient subscriber,
                                               VmsLayer layer,
                                               int publisherId) {
        ICarImpl.assertVmsSubscriberPermission(mContext);
        synchronized (mSubscriberServiceLock) {
            // Remove the subscription.
            mHal.removeSubscription(subscriber, layer, publisherId);
        }
    }

    @Override
    public void addVmsSubscriberPassive(IVmsSubscriberClient subscriber) {
        ICarImpl.assertVmsSubscriberPermission(mContext);
        synchronized (mSubscriberServiceLock) {
            mSubscribersManager.add(subscriber);
            mHal.addSubscription(subscriber);
        }
    }

    @Override
    public void removeVmsSubscriberPassive(IVmsSubscriberClient subscriber) {
        ICarImpl.assertVmsSubscriberPermission(mContext);
        synchronized (mSubscriberServiceLock) {
            // Remove the subscription.
            mHal.removeSubscription(subscriber);
        }
    }

    @Override
    public byte[] getPublisherInfo(int publisherId) {
        ICarImpl.assertVmsSubscriberPermission(mContext);
        synchronized (mSubscriberServiceLock) {
            return mHal.getPublisherInfo(publisherId);
        }
    }

    @Override
    public VmsAvailableLayers getAvailableLayers() {
        return mHal.getAvailableLayers();

    }

    // Implements VmsHalSubscriberListener interface
    @Override
    public void onDataMessage(VmsLayer layer, int publisherId, byte[] payload) {
        if (DBG) {
            Log.d(TAG, "Publishing a message for layer: " + layer);
        }

        Set<IVmsSubscriberClient> subscribers =
                mHal.getSubscribersForLayerFromPublisher(layer, publisherId);

        for (IVmsSubscriberClient subscriber : subscribers) {
            try {
                subscriber.onVmsMessageReceived(layer, payload);
            } catch (RemoteException e) {
                // If we could not send a record, its likely the connection snapped. Let the binder
                // death handle the situation.
                Log.e(TAG, "onVmsMessageReceived calling failed: ", e);
            }
        }
    }

    @Override
    public void onLayersAvaiabilityChange(VmsAvailableLayers availableLayers) {
        if (DBG) {
            Log.d(TAG, "Publishing layers availability change: " + availableLayers);
        }

        Set<IVmsSubscriberClient> subscribers;
        subscribers = new HashSet<>(mSubscribersManager.getListeners());

        for (IVmsSubscriberClient subscriber : subscribers) {
            try {
                subscriber.onLayersAvailabilityChanged(availableLayers);
            } catch (RemoteException e) {
                // If we could not send a record, its likely the connection snapped. Let the binder
                // death handle the situation.
                Log.e(TAG, "onLayersAvailabilityChanged calling failed: ", e);
            }
        }
    }
}
