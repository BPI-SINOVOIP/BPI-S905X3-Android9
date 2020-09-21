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

package android.car.hardware.property;

import static java.lang.Integer.toHexString;

import android.car.CarApiUtil;
import android.car.CarManagerBase;
import android.car.CarNotConnectedException;
import android.car.hardware.CarPropertyConfig;
import android.car.hardware.CarPropertyValue;
import android.os.Handler;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.ArraySet;
import android.util.Log;
import android.util.SparseArray;

import com.android.car.internal.CarRatedFloatListeners;
import com.android.car.internal.SingleMessageHandler;

import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.List;
import java.util.function.Consumer;


/**
 * API for creating Car*Manager
 * @hide
 */
public class CarPropertyManager implements CarManagerBase {
    private final boolean mDbg;
    private final SingleMessageHandler<CarPropertyEvent> mHandler;
    private final ICarProperty mService;
    private final String mTag;
    private static final int MSG_GENERIC_EVENT = 0;

    private CarPropertyEventListenerToService mCarPropertyEventToService;


    /** Record of locally active properties. Key is propertyId */
    private final SparseArray<CarPropertyListeners> mActivePropertyListener =
            new SparseArray<>();

    /** Callback functions for property events */
    public interface CarPropertyEventListener {
        /** Called when a property is updated */
        void onChangeEvent(CarPropertyValue value);

        /** Called when an error is detected with a property */
        void onErrorEvent(int propId, int zone);
    }

    /**
     * Get an instance of the CarPropertyManager.
     */
    public CarPropertyManager(IBinder service, Handler handler, boolean dbg, String tag) {
        mDbg = dbg;
        mTag = tag;
        mService = ICarProperty.Stub.asInterface(service);
        mHandler = new SingleMessageHandler<CarPropertyEvent>(handler.getLooper(),
                MSG_GENERIC_EVENT) {
            @Override
            protected void handleEvent(CarPropertyEvent event) {
                CarPropertyListeners listeners;
                synchronized (mActivePropertyListener) {
                    listeners = mActivePropertyListener.get(
                            event.getCarPropertyValue().getPropertyId());
                }
                if (listeners != null) {
                    switch (event.getEventType()) {
                        case CarPropertyEvent.PROPERTY_EVENT_PROPERTY_CHANGE:
                            listeners.onPropertyChanged(event);
                            break;
                        case CarPropertyEvent.PROPERTY_EVENT_ERROR:
                            listeners.onErrorEvent(event);
                            break;
                        default:
                            throw new IllegalArgumentException();
                    }
                }
            }
        };
    }

    /** Use to register or update Callback for properties */
    public boolean registerListener(CarPropertyEventListener listener, int propertyId, float rate)
            throws CarNotConnectedException {
        synchronized (mActivePropertyListener) {
            if (mCarPropertyEventToService == null) {
                mCarPropertyEventToService = new CarPropertyEventListenerToService(this);
            }
            boolean needsServerUpdate = false;
            CarPropertyListeners listeners;
            listeners = mActivePropertyListener.get(propertyId);
            if (listeners == null) {
                listeners = new CarPropertyListeners(rate);
                mActivePropertyListener.put(propertyId, listeners);
                needsServerUpdate = true;
            }
            if (listeners.addAndUpdateRate(listener, rate)) {
                needsServerUpdate = true;
            }
            if (needsServerUpdate) {
                if (!registerOrUpdatePropertyListener(propertyId, rate)) {
                    return false;
                }
            }
        }
        return true;
    }

    private boolean registerOrUpdatePropertyListener(int propertyId, float rate)
            throws CarNotConnectedException {
        try {
            mService.registerListener(propertyId, rate, mCarPropertyEventToService);
        } catch (IllegalStateException e) {
            CarApiUtil.checkCarNotConnectedExceptionFromCarService(e);
        } catch (RemoteException e) {
            throw new CarNotConnectedException(e);
        }
        return true;
    }

    private class CarPropertyEventListenerToService extends ICarPropertyEventListener.Stub{
        private final WeakReference<CarPropertyManager> mMgr;

        CarPropertyEventListenerToService(CarPropertyManager mgr) {
            mMgr = new WeakReference<>(mgr);
        }

        @Override
        public void onEvent(List<CarPropertyEvent> events) throws RemoteException {
            CarPropertyManager manager = mMgr.get();
            if (manager != null) {
                manager.handleEvent(events);
            }
        }
    }

    private void handleEvent(List<CarPropertyEvent> events) {
        mHandler.sendEvents(events);
    }

    /**
     * Stop getting sensor update for the given listener. If there are multiple registrations for
     * this listener, all listening will be stopped.
     * @param listener
     */
    public void unregisterListener(CarPropertyEventListener listener) {
        synchronized (mActivePropertyListener) {
            for (int i = 0; i < mActivePropertyListener.size(); i++) {
                doUnregisterListenerLocked(listener, mActivePropertyListener.keyAt(i));
            }
        }
    }

    /**
     * Stop getting sensor update for the given listener and sensor. If the same listener is used
     * for other sensors, those subscriptions will not be affected.
     * @param listener
     * @param propertyId
     */
    public void unregisterListener(CarPropertyEventListener listener, int propertyId) {
        synchronized (mActivePropertyListener) {
            doUnregisterListenerLocked(listener, propertyId);
        }
    }

    private void doUnregisterListenerLocked(CarPropertyEventListener listener, int propertyId) {
        CarPropertyListeners listeners = mActivePropertyListener.get(propertyId);
        if (listeners != null) {
            boolean needsServerUpdate = false;
            if (listeners.contains(listener)) {
                needsServerUpdate = listeners.remove(listener);
            }
            if (listeners.isEmpty()) {
                try {
                    mService.unregisterListener(propertyId, mCarPropertyEventToService);
                } catch (RemoteException e) {
                    //ignore
                }
                mActivePropertyListener.remove(propertyId);
            } else if (needsServerUpdate) {
                try {
                    registerOrUpdatePropertyListener(propertyId, listeners.getRate());
                } catch (CarNotConnectedException e) {
                    // ignore
                }
            }
        }
    }

    /**
     * Returns the list of properties implemented by this car.
     *
     * @return Caller must check the property type and typecast to the appropriate subclass
     * (CarPropertyBooleanProperty, CarPropertyFloatProperty, CarrPropertyIntProperty)
     */
    public List<CarPropertyConfig> getPropertyList() throws CarNotConnectedException {
        try {
            return mService.getPropertyList();
        } catch (RemoteException e) {
            Log.e(mTag, "getPropertyList exception ", e);
            throw new CarNotConnectedException(e);
        }
    }

    /**
     * Returns the list of properties implemented by this car in given property id list.
     *
     * @return Caller must check the property type and typecast to the appropriate subclass
     * (CarPropertyBooleanProperty, CarPropertyFloatProperty, CarrPropertyIntProperty)
     */
    public List<CarPropertyConfig> getPropertyList(ArraySet<Integer> propertyIds)
            throws CarNotConnectedException {
        try {
            List<CarPropertyConfig> configs = new ArrayList<>();
            for (CarPropertyConfig c : mService.getPropertyList()) {
                if (propertyIds.contains(c.getPropertyId())) {
                    configs.add(c);
                }
            }
            return configs;
        } catch (RemoteException e) {
            Log.e(mTag, "getPropertyList exception ", e);
            throw new CarNotConnectedException(e);
        }

    }

    /**
     * Check whether a given property is available or disabled based on the car's current state.
     * @return true if STATUS_AVAILABLE, false otherwise (eg STATUS_UNAVAILABLE)
     * @throws CarNotConnectedException
     */
    public boolean isPropertyAvailable(int propId, int area) throws CarNotConnectedException {
        try {
            CarPropertyValue propValue = mService.getProperty(propId, area);
            return (propValue != null)
                    && (propValue.getStatus() == CarPropertyValue.STATUS_AVAILABLE);
        } catch (RemoteException e) {
            Log.e(mTag, "isPropertyAvailable failed with " + e.toString()
                    + ", propId: 0x" + toHexString(propId) + ", area: 0x" + toHexString(area), e);
            throw new CarNotConnectedException(e);
        }
    }

    /**
     * Returns value of a bool property
     *
     * @param prop Property ID to get
     * @param area Area of the property to get
     */
    public boolean getBooleanProperty(int prop, int area) throws CarNotConnectedException {
        CarPropertyValue<Boolean> carProp = getProperty(Boolean.class, prop, area);
        return carProp != null ? carProp.getValue() : false;
    }

    /**
     * Returns value of a float property
     *
     * @param prop Property ID to get
     * @param area Area of the property to get
     */
    public float getFloatProperty(int prop, int area) throws CarNotConnectedException {
        CarPropertyValue<Float> carProp = getProperty(Float.class, prop, area);
        return carProp != null ? carProp.getValue() : 0f;
    }

    /**
     * Returns value of a integer property
     *
     * @param prop Property ID to get
     * @param area Zone of the property to get
     */
    public int getIntProperty(int prop, int area) throws CarNotConnectedException {
        CarPropertyValue<Integer> carProp = getProperty(Integer.class, prop, area);
        return carProp != null ? carProp.getValue() : 0;
    }

    /** Return CarPropertyValue */
    @SuppressWarnings("unchecked")
    public <E> CarPropertyValue<E> getProperty(Class<E> clazz, int propId, int area)
            throws CarNotConnectedException {
        if (mDbg) {
            Log.d(mTag, "getProperty, propId: 0x" + toHexString(propId)
                    + ", area: 0x" + toHexString(area) + ", class: " + clazz);
        }
        try {
            CarPropertyValue<E> propVal = mService.getProperty(propId, area);
            if (propVal != null && propVal.getValue() != null) {
                Class<?> actualClass = propVal.getValue().getClass();
                if (actualClass != clazz) {
                    throw new IllegalArgumentException("Invalid property type. " + "Expected: "
                            + clazz + ", but was: " + actualClass);
                }
            }
            return propVal;
        } catch (RemoteException e) {
            Log.e(mTag, "getProperty failed with " + e.toString()
                    + ", propId: 0x" + toHexString(propId) + ", area: 0x" + toHexString(area), e);
            throw new CarNotConnectedException(e);
        }
    }

    /** Return raw CarPropertyValue */
    public <E> CarPropertyValue<E> getProperty(int propId, int area)
            throws CarNotConnectedException {
        try {
            CarPropertyValue<E> propVal = mService.getProperty(propId, area);
            return propVal;
        } catch (RemoteException e) {
            Log.e(mTag, "getProperty failed with " + e.toString()
                    + ", propId: 0x" + toHexString(propId) + ", area: 0x" + toHexString(area), e);
            throw new CarNotConnectedException(e);
        }
    }

    /** Set CarPropertyValue */
    public <E> void setProperty(Class<E> clazz, int propId, int area, E val)
            throws CarNotConnectedException {
        if (mDbg) {
            Log.d(mTag, "setProperty, propId: 0x" + toHexString(propId)
                    + ", area: 0x" + toHexString(area) + ", class: " + clazz + ", val: " + val);
        }
        try {
            mService.setProperty(new CarPropertyValue<>(propId, area, val));
        } catch (RemoteException e) {
            Log.e(mTag, "setProperty failed with " + e.toString(), e);
            throw new CarNotConnectedException(e);
        }
    }

    /**
     * Modifies a property.  If the property modification doesn't occur, an error event shall be
     * generated and propagated back to the application.
     *
     * @param prop Property ID to modify
     * @param area Area to apply the modification.
     * @param val Value to set
     */
    public void setBooleanProperty(int prop, int area, boolean val)
            throws CarNotConnectedException {
        setProperty(Boolean.class, prop, area, val);
    }

    /** Set float value of property*/
    public void setFloatProperty(int prop, int area, float val) throws CarNotConnectedException {
        setProperty(Float.class, prop, area, val);
    }
    /** Set int value of property*/
    public void setIntProperty(int prop, int area, int val) throws CarNotConnectedException {
        setProperty(Integer.class, prop, area, val);
    }


    private class CarPropertyListeners extends CarRatedFloatListeners<CarPropertyEventListener> {
        CarPropertyListeners(float rate) {
            super(rate);
        }
        void onPropertyChanged(final CarPropertyEvent event) {
            // throw away old sensor data as oneway binder call can change order.
            long updateTime = event.getCarPropertyValue().getTimestamp();
            if (updateTime < mLastUpdateTime) {
                Log.w(mTag, "dropping old property data");
                return;
            }
            mLastUpdateTime = updateTime;
            List<CarPropertyEventListener> listeners;
            synchronized (mActivePropertyListener) {
                listeners = new ArrayList<>(getListeners());
            }
            listeners.forEach(new Consumer<CarPropertyEventListener>() {
                @Override
                public void accept(CarPropertyEventListener listener) {
                    listener.onChangeEvent(event.getCarPropertyValue());
                }
            });
        }

        void onErrorEvent(final CarPropertyEvent event) {
            List<CarPropertyEventListener> listeners;
            CarPropertyValue value = event.getCarPropertyValue();
            synchronized (mActivePropertyListener) {
                listeners = new ArrayList<>(getListeners());
            }
            listeners.forEach(new Consumer<CarPropertyEventListener>() {
                @Override
                public void accept(CarPropertyEventListener listener) {
                    listener.onErrorEvent(value.getPropertyId(), value.getAreaId());
                }
            });
        }
    }

    /** @hide */
    @Override
    public void onCarDisconnected() {
        synchronized (mActivePropertyListener) {
            mActivePropertyListener.clear();
            mCarPropertyEventToService = null;
        }
    }
}
