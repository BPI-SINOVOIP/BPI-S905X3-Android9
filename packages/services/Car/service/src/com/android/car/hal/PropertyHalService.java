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
package com.android.car.hal;

import static com.android.car.hal.CarPropertyUtils.toCarPropertyValue;
import static com.android.car.hal.CarPropertyUtils.toVehiclePropValue;

import static java.lang.Integer.toHexString;

import android.annotation.Nullable;
import android.car.hardware.CarPropertyConfig;
import android.car.hardware.CarPropertyValue;
import android.car.hardware.property.CarPropertyEvent;
import android.hardware.automotive.vehicle.V2_0.VehiclePropConfig;
import android.hardware.automotive.vehicle.V2_0.VehiclePropValue;
import android.util.Log;
import android.util.SparseArray;

import com.android.car.CarLog;
import com.android.internal.annotations.GuardedBy;

import java.io.PrintWriter;
import java.util.Collection;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;

/**
 * Common interface for HAL services that send Vehicle Properties back and forth via ICarProperty.
 * Services that communicate by passing vehicle properties back and forth via ICarProperty should
 * extend this class.
 */
public class PropertyHalService extends HalServiceBase {
    private final boolean mDbg = true;
    private final LinkedList<CarPropertyEvent> mEventsToDispatch = new LinkedList<>();
    private final Map<Integer, CarPropertyConfig<?>> mProps =
            new ConcurrentHashMap<>();
    private final SparseArray<Float> mRates = new SparseArray<Float>();
    private static final String TAG = "PropertyHalService";
    private final VehicleHal mVehicleHal;
    private final PropertyHalServiceIds mPropIds;

    @GuardedBy("mLock")
    private PropertyHalListener mListener;

    private Set<Integer> mSubscribedPropIds;

    private final Object mLock = new Object();

    /**
     * Converts manager property ID to Vehicle HAL property ID.
     * If property is not supported, it will return {@link #NOT_SUPPORTED_PROPERTY}.
     */
    private int managerToHalPropId(int propId) {
        if (mProps.containsKey(propId)) {
            return propId;
        } else {
            return NOT_SUPPORTED_PROPERTY;
        }
    }

    /**
     * Converts Vehicle HAL property ID to manager property ID.
     * If property is not supported, it will return {@link #NOT_SUPPORTED_PROPERTY}.
     */
    private int halToManagerPropId(int halPropId) {
        if (mProps.containsKey(halPropId)) {
            return halPropId;
        } else {
            return NOT_SUPPORTED_PROPERTY;
        }
    }

    /**
     * PropertyHalListener used to send events to CarPropertyService
     */
    public interface PropertyHalListener {
        /**
         * This event is sent whenever the property value is updated
         * @param event
         */
        void onPropertyChange(List<CarPropertyEvent> events);
        /**
         * This event is sent when the set property call fails
         * @param property
         * @param area
         */
        void onPropertySetError(int property, int area);
    }

    public PropertyHalService(VehicleHal vehicleHal) {
        mPropIds = new PropertyHalServiceIds();
        mSubscribedPropIds = new HashSet<Integer>();
        mVehicleHal = vehicleHal;
        if (mDbg) {
            Log.d(TAG, "started PropertyHalService");
        }
    }

    /**
     * Set the listener for the HAL service
     * @param listener
     */
    public void setListener(PropertyHalListener listener) {
        synchronized (mLock) {
            mListener = listener;
        }
    }

    /**
     *
     * @return List<CarPropertyConfig> List of configs available.
     */
    public Map<Integer, CarPropertyConfig<?>> getPropertyList() {
        if (mDbg) {
            Log.d(TAG, "getPropertyList");
        }
        return mProps;
    }

    /**
     * Returns property or null if property is not ready yet.
     * @param mgrPropId
     * @param areaId
     */
    @Nullable
    public CarPropertyValue getProperty(int mgrPropId, int areaId) {
        int halPropId = managerToHalPropId(mgrPropId);
        if (halPropId == NOT_SUPPORTED_PROPERTY) {
            throw new IllegalArgumentException("Invalid property Id : 0x" + toHexString(mgrPropId));
        }

        VehiclePropValue value = null;
        try {
            value = mVehicleHal.get(halPropId, areaId);
        } catch (PropertyTimeoutException e) {
            Log.e(CarLog.TAG_PROPERTY, "get, property not ready 0x" + toHexString(halPropId), e);
        }

        return value == null ? null : toCarPropertyValue(value, mgrPropId);
    }

    /**
     * Returns sample rate for the property
     * @param propId
     */
    public float getSampleRate(int propId) {
        return mVehicleHal.getSampleRate(propId);
    }

    /**
     * Get the read permission string for the property.
     * @param propId
     */
    @Nullable
    public String getReadPermission(int propId) {
        return mPropIds.getReadPermission(propId);
    }

    /**
     * Get the write permission string for the property.
     * @param propId
     */
    @Nullable
    public String getWritePermission(int propId) {
        return mPropIds.getWritePermission(propId);
    }

    /**
     * Set the property value.
     * @param prop
     */
    public void setProperty(CarPropertyValue prop) {
        int halPropId = managerToHalPropId(prop.getPropertyId());
        if (halPropId == NOT_SUPPORTED_PROPERTY) {
            throw new IllegalArgumentException("Invalid property Id : 0x"
                    + toHexString(prop.getPropertyId()));
        }
        VehiclePropValue halProp = toVehiclePropValue(prop, halPropId);
        try {
            mVehicleHal.set(halProp);
        } catch (PropertyTimeoutException e) {
            Log.e(CarLog.TAG_PROPERTY, "set, property not ready 0x" + toHexString(halPropId), e);
            throw new RuntimeException(e);
        }
    }

    /**
     * Subscribe to this property at the specified update rate.
     * @param propId
     * @param rate
     */
    public void subscribeProperty(int propId, float rate) {
        if (mDbg) {
            Log.d(TAG, "subscribeProperty propId=0x" + toHexString(propId) + ", rate=" + rate);
        }
        int halPropId = managerToHalPropId(propId);
        if (halPropId == NOT_SUPPORTED_PROPERTY) {
            throw new IllegalArgumentException("Invalid property Id : 0x"
                    + toHexString(propId));
        }
        // Validate the min/max rate
        CarPropertyConfig cfg = mProps.get(propId);
        if (rate > cfg.getMaxSampleRate()) {
            rate = cfg.getMaxSampleRate();
        } else if (rate < cfg.getMinSampleRate()) {
            rate = cfg.getMinSampleRate();
        }
        synchronized (mSubscribedPropIds) {
            mSubscribedPropIds.add(halPropId);
        }
        mVehicleHal.subscribeProperty(this, halPropId, rate);
    }

    /**
     * Unsubscribe the property and turn off update events for it.
     * @param propId
     */
    public void unsubscribeProperty(int propId) {
        if (mDbg) {
            Log.d(TAG, "unsubscribeProperty propId=0x" + toHexString(propId));
        }
        int halPropId = managerToHalPropId(propId);
        if (halPropId == NOT_SUPPORTED_PROPERTY) {
            throw new IllegalArgumentException("Invalid property Id : 0x"
                    + toHexString(propId));
        }
        synchronized (mSubscribedPropIds) {
            if (mSubscribedPropIds.contains(halPropId)) {
                mSubscribedPropIds.remove(halPropId);
                mVehicleHal.unsubscribeProperty(this, halPropId);
            }
        }
    }

    @Override
    public void init() {
        if (mDbg) {
            Log.d(TAG, "init()");
        }
    }

    @Override
    public void release() {
        if (mDbg) {
            Log.d(TAG, "release()");
        }
        synchronized (mSubscribedPropIds) {
            for (Integer prop : mSubscribedPropIds) {
                mVehicleHal.unsubscribeProperty(this, prop);
            }
            mSubscribedPropIds.clear();
        }
        mProps.clear();

        synchronized (mLock) {
            mListener = null;
        }
    }

    @Override
    public Collection<VehiclePropConfig> takeSupportedProperties(
            Collection<VehiclePropConfig> allProperties) {
        List<VehiclePropConfig> taken = new LinkedList<>();

        for (VehiclePropConfig p : allProperties) {
            if (mPropIds.isSupportedProperty(p.prop)) {
                CarPropertyConfig config = CarPropertyUtils.toCarPropertyConfig(p, p.prop);
                taken.add(p);
                mProps.put(p.prop, config);
                if (mDbg) {
                    Log.d(TAG, "takeSupportedProperties: " + toHexString(p.prop));
                }
            }
        }
        if (mDbg) {
            Log.d(TAG, "takeSupportedProperties() took " + taken.size() + " properties");
        }
        return taken;
    }

    @Override
    public void handleHalEvents(List<VehiclePropValue> values) {
        PropertyHalListener listener;
        synchronized (mLock) {
            listener = mListener;
        }
        if (listener != null) {
            for (VehiclePropValue v : values) {
                int mgrPropId = halToManagerPropId(v.prop);
                if (mgrPropId == NOT_SUPPORTED_PROPERTY) {
                    Log.e(TAG, "Property is not supported: 0x" + toHexString(v.prop));
                    continue;
                }
                CarPropertyValue<?> propVal = toCarPropertyValue(v, mgrPropId);
                CarPropertyEvent event = new CarPropertyEvent(
                        CarPropertyEvent.PROPERTY_EVENT_PROPERTY_CHANGE, propVal);
                if (event != null) {
                    mEventsToDispatch.add(event);
                }
            }
            listener.onPropertyChange(mEventsToDispatch);
            mEventsToDispatch.clear();
        }
    }

    @Override
    public void handlePropertySetError(int property, int area) {
        PropertyHalListener listener;
        synchronized (mLock) {
            listener = mListener;
        }
        if (listener != null) {
            listener.onPropertySetError(property, area);
        }
    }

    @Override
    public void dump(PrintWriter writer) {
        writer.println(TAG);
        writer.println("  Properties available:");
        for (CarPropertyConfig prop : mProps.values()) {
            writer.println("    " + prop.toString());
        }
    }
}
