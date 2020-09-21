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

import static java.lang.Integer.toHexString;

import android.car.Car;
import android.car.hardware.CarPropertyConfig;
import android.car.hardware.CarPropertyValue;
import android.car.hardware.property.CarPropertyEvent;
import android.car.hardware.property.ICarProperty;
import android.car.hardware.property.ICarPropertyEventListener;
import android.content.Context;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;
import android.util.Pair;
import android.util.SparseArray;

import com.android.car.hal.PropertyHalService;

import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.CopyOnWriteArrayList;

/**
 * This class implements the binder interface for ICarProperty.aidl to make it easier to create
 * multiple managers that deal with Vehicle Properties. To create a new service, simply extend
 * this class and call the super() constructor with the appropriate arguments for the new service.
 * {@link CarHvacService} shows the basic usage.
 */
public class CarPropertyService extends ICarProperty.Stub
        implements CarServiceBase, PropertyHalService.PropertyHalListener {
    private static final boolean DBG = true;
    private static final String TAG = "Property.service";
    private final Context mContext;
    private final Map<IBinder, Client> mClientMap = new ConcurrentHashMap<>();
    private Map<Integer, CarPropertyConfig<?>> mConfigs;
    private final PropertyHalService mHal;
    private boolean mListenerIsSet = false;
    private final Map<Integer, List<Client>> mPropIdClientMap = new ConcurrentHashMap<>();
    private final Object mLock = new Object();

    public CarPropertyService(Context context, PropertyHalService hal) {
        if (DBG) {
            Log.d(TAG, "CarPropertyService started!");
        }
        mHal = hal;
        mContext = context;
    }

    // Helper class to keep track of listeners to this service
    private class Client implements IBinder.DeathRecipient {
        private final ICarPropertyEventListener mListener;
        private final IBinder mListenerBinder;
        private final SparseArray<Float> mRateMap = new SparseArray<Float>();   // key is propId

        Client(ICarPropertyEventListener listener) {
            mListener = listener;
            mListenerBinder = listener.asBinder();

            try {
                mListenerBinder.linkToDeath(this, 0);
            } catch (RemoteException e) {
                Log.e(TAG, "Failed to link death for recipient. " + e);
                throw new IllegalStateException(Car.CAR_NOT_CONNECTED_EXCEPTION_MSG);
            }
            mClientMap.put(mListenerBinder, this);
        }

        void addProperty(int propId, float rate) {
            mRateMap.put(propId, rate);
        }

        /**
         * Client died. Remove the listener from HAL service and unregister if this is the last
         * client.
         */
        @Override
        public void binderDied() {
            if (DBG) {
                Log.d(TAG, "binderDied " + mListenerBinder);
            }

            for (int i = 0; i < mRateMap.size(); i++) {
                int propId = mRateMap.keyAt(i);
                CarPropertyService.this.unregisterListenerBinderLocked(propId, mListenerBinder);
            }
            this.release();
        }

        ICarPropertyEventListener getListener() {
            return mListener;
        }

        IBinder getListenerBinder() {
            return mListenerBinder;
        }

        float getRate(int propId) {
            // Return 0 if no key found, since that is the slowest rate.
            return mRateMap.get(propId, (float) 0);
        }

        void release() {
            mListenerBinder.unlinkToDeath(this, 0);
            mClientMap.remove(mListenerBinder);
        }

        void removeProperty(int propId) {
            mRateMap.remove(propId);
            if (mRateMap.size() == 0) {
                // Last property was released, remove the client.
                this.release();
            }
        }
    }

    @Override
    public void init() {
    }

    @Override
    public void release() {
        for (Client c : mClientMap.values()) {
            c.release();
        }
        mClientMap.clear();
        mPropIdClientMap.clear();
        mHal.setListener(null);
        mListenerIsSet = false;
    }

    @Override
    public void dump(PrintWriter writer) {
    }

    @Override
    public void registerListener(int propId, float rate, ICarPropertyEventListener listener) {
        if (DBG) {
            Log.d(TAG, "registerListener: propId=0x" + toHexString(propId) + " rate=" + rate);
        }
        if (mConfigs.get(propId) == null) {
            // Do not attempt to register an invalid propId
            Log.e(TAG, "registerListener:  propId is not in config list:  " + propId);
            return;
        }
        ICarImpl.assertPermission(mContext, mHal.getReadPermission(propId));
        if (listener == null) {
            Log.e(TAG, "registerListener: Listener is null.");
            throw new IllegalArgumentException("listener cannot be null.");
        }

        IBinder listenerBinder = listener.asBinder();

        synchronized (mLock) {
            // Get the client for this listener
            Client client = mClientMap.get(listenerBinder);
            if (client == null) {
                client = new Client(listener);
            }
            client.addProperty(propId, rate);
            // Insert the client into the propId --> clients map
            List<Client> clients = mPropIdClientMap.get(propId);
            if (clients == null) {
                clients = new CopyOnWriteArrayList<Client>();
                mPropIdClientMap.put(propId, clients);
            }
            if (!clients.contains(client)) {
                clients.add(client);
            }
            // Set the HAL listener if necessary
            if (!mListenerIsSet) {
                mHal.setListener(this);
            }
            // Set the new rate
            if (rate > mHal.getSampleRate(propId)) {
                mHal.subscribeProperty(propId, rate);
            }
        }

        // Send the latest value(s) to the registering listener only
        List<CarPropertyEvent> events = new LinkedList<CarPropertyEvent>();
        for (int areaId : mConfigs.get(propId).getAreaIds()) {
            CarPropertyValue value = mHal.getProperty(propId, areaId);
            CarPropertyEvent event = new CarPropertyEvent(
                    CarPropertyEvent.PROPERTY_EVENT_PROPERTY_CHANGE, value);
            events.add(event);
        }
        try {
            listener.onEvent(events);
        } catch (RemoteException ex) {
            // If we cannot send a record, its likely the connection snapped. Let the binder
            // death handle the situation.
            Log.e(TAG, "onEvent calling failed: " + ex);
        }
    }

    @Override
    public void unregisterListener(int propId, ICarPropertyEventListener listener) {
        if (DBG) {
            Log.d(TAG, "unregisterListener propId=0x" + toHexString(propId));
        }
        ICarImpl.assertPermission(mContext, mHal.getReadPermission(propId));
        if (listener == null) {
            Log.e(TAG, "unregisterListener: Listener is null.");
            throw new IllegalArgumentException("Listener is null");
        }

        IBinder listenerBinder = listener.asBinder();
        synchronized (mLock) {
            unregisterListenerBinderLocked(propId, listenerBinder);
        }
    }

    private void unregisterListenerBinderLocked(int propId, IBinder listenerBinder) {
        Client client = mClientMap.get(listenerBinder);
        List<Client> propertyClients = mPropIdClientMap.get(propId);
        if (mConfigs.get(propId) == null) {
            // Do not attempt to register an invalid propId
            Log.e(TAG, "unregisterListener: propId is not in config list:0x" + toHexString(propId));
            return;
        }
        if ((client == null) || (propertyClients == null)) {
            Log.e(TAG, "unregisterListenerBinderLocked: Listener was not previously registered.");
        } else {
            if (propertyClients.remove(client)) {
                client.removeProperty(propId);
            } else {
                Log.e(TAG, "unregisterListenerBinderLocked: Listener was not registered for "
                           + "propId=0x" + toHexString(propId));
            }

            if (propertyClients.isEmpty()) {
                // Last listener for this property unsubscribed.  Clean up
                mHal.unsubscribeProperty(propId);
                mPropIdClientMap.remove(propId);
                if (mPropIdClientMap.isEmpty()) {
                    // No more properties are subscribed.  Turn off the listener.
                    mHal.setListener(null);
                    mListenerIsSet = false;
                }
            } else {
                // Other listeners are still subscribed.  Calculate the new rate
                float maxRate = 0;
                for (Client c : propertyClients) {
                    float rate = c.getRate(propId);
                    if (rate > maxRate) {
                        maxRate = rate;
                    }
                }
                // Set the new rate
                mHal.subscribeProperty(propId, maxRate);
            }
        }
    }

    /**
     * Return the list of properties that the caller may access.
     */
    @Override
    public List<CarPropertyConfig> getPropertyList() {
        List<CarPropertyConfig> returnList = new ArrayList<CarPropertyConfig>();
        if (mConfigs == null) {
            // Cache the configs list to avoid subsequent binder calls
            mConfigs = mHal.getPropertyList();
        }
        for (CarPropertyConfig c : mConfigs.values()) {
            if (ICarImpl.hasPermission(mContext, mHal.getReadPermission(c.getPropertyId()))) {
                // Only add properties the list if the process has permissions to read it
                returnList.add(c);
            }
        }
        if (DBG) {
            Log.d(TAG, "getPropertyList returns " + returnList.size() + " configs");
        }
        return returnList;
    }

    @Override
    public CarPropertyValue getProperty(int prop, int zone) {
        if (mConfigs.get(prop) == null) {
            // Do not attempt to register an invalid propId
            Log.e(TAG, "getProperty: propId is not in config list:0x" + toHexString(prop));
            return null;
        }
        ICarImpl.assertPermission(mContext, mHal.getReadPermission(prop));
        return mHal.getProperty(prop, zone);
    }

    @Override
    public void setProperty(CarPropertyValue prop) {
        int propId = prop.getPropertyId();
        if (mConfigs.get(propId) == null) {
            // Do not attempt to register an invalid propId
            Log.e(TAG, "setProperty:  propId is not in config list:0x" + toHexString(propId));
            return;
        }
        ICarImpl.assertPermission(mContext, mHal.getWritePermission(propId));
        mHal.setProperty(prop);
    }

    // Implement PropertyHalListener interface
    @Override
    public void onPropertyChange(List<CarPropertyEvent> events) {
        Map<IBinder, Pair<ICarPropertyEventListener, List<CarPropertyEvent>>> eventsToDispatch =
                new HashMap<>();

        for (CarPropertyEvent event : events) {
            int propId = event.getCarPropertyValue().getPropertyId();
            List<Client> clients = mPropIdClientMap.get(propId);
            if (clients == null) {
                Log.e(TAG, "onPropertyChange: no listener registered for propId=0x"
                        + toHexString(propId));
                continue;
            }

            for (Client c : clients) {
                IBinder listenerBinder = c.getListenerBinder();
                Pair<ICarPropertyEventListener, List<CarPropertyEvent>> p =
                        eventsToDispatch.get(listenerBinder);
                if (p == null) {
                    // Initialize the linked list for the listener
                    p = new Pair<>(c.getListener(), new LinkedList<CarPropertyEvent>());
                    eventsToDispatch.put(listenerBinder, p);
                }
                p.second.add(event);
            }
        }
        // Parse the dispatch list to send events
        for (Pair<ICarPropertyEventListener, List<CarPropertyEvent>> p: eventsToDispatch.values()) {
            try {
                p.first.onEvent(p.second);
            } catch (RemoteException ex) {
                // If we cannot send a record, its likely the connection snapped. Let binder
                // death handle the situation.
                Log.e(TAG, "onEvent calling failed: " + ex);
            }
        }
    }

    @Override
    public void onPropertySetError(int property, int area) {
        List<Client> clients = mPropIdClientMap.get(property);
        if (clients != null) {
            List<CarPropertyEvent> eventList = new LinkedList<>();
            eventList.add(createErrorEvent(property, area));
            for (Client c : clients) {
                try {
                    c.getListener().onEvent(eventList);
                } catch (RemoteException ex) {
                    // If we cannot send a record, its likely the connection snapped. Let the binder
                    // death handle the situation.
                    Log.e(TAG, "onEvent calling failed: " + ex);
                }
            }
        } else {
            Log.e(TAG, "onPropertySetError called with no listener registered for propId=0x"
                    + toHexString(property));
        }
    }

    private static CarPropertyEvent createErrorEvent(int property, int area) {
        return new CarPropertyEvent(CarPropertyEvent.PROPERTY_EVENT_ERROR,
                new CarPropertyValue<>(property, area, null));
    }
}
