/*
 * Copyright (C) 2015 The Android Open Source Project
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

import static com.android.car.CarServiceUtils.toByteArray;
import static com.android.car.CarServiceUtils.toFloatArray;
import static com.android.car.CarServiceUtils.toIntArray;

import static java.lang.Integer.toHexString;

import android.annotation.CheckResult;
import android.hardware.automotive.vehicle.V2_0.IVehicle;
import android.hardware.automotive.vehicle.V2_0.IVehicleCallback;
import android.hardware.automotive.vehicle.V2_0.SubscribeFlags;
import android.hardware.automotive.vehicle.V2_0.SubscribeOptions;
import android.hardware.automotive.vehicle.V2_0.VehicleAreaConfig;
import android.hardware.automotive.vehicle.V2_0.VehiclePropConfig;
import android.hardware.automotive.vehicle.V2_0.VehiclePropValue;
import android.hardware.automotive.vehicle.V2_0.VehicleProperty;
import android.hardware.automotive.vehicle.V2_0.VehiclePropertyAccess;
import android.hardware.automotive.vehicle.V2_0.VehiclePropertyChangeMode;
import android.hardware.automotive.vehicle.V2_0.VehiclePropertyType;
import android.os.HandlerThread;
import android.os.RemoteException;
import android.os.SystemClock;
import android.util.ArraySet;
import android.util.Log;
import android.util.SparseArray;

import com.android.car.CarLog;
import com.android.internal.annotations.VisibleForTesting;

import com.google.android.collect.Lists;

import java.io.PrintWriter;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/**
 * Abstraction for vehicle HAL. This class handles interface with native HAL and do basic parsing
 * of received data (type check). Then each event is sent to corresponding {@link HalServiceBase}
 * implementation. It is responsibility of {@link HalServiceBase} to convert data to corresponding
 * Car*Service for Car*Manager API.
 */
public class VehicleHal extends IVehicleCallback.Stub {

    private static final boolean DBG = false;

    private static final int NO_AREA = -1;

    private final HandlerThread mHandlerThread;
    private final PowerHalService mPowerHal;
    private final PropertyHalService mPropertyHal;
    private final InputHalService mInputHal;
    private final VmsHalService mVmsHal;
    private DiagnosticHalService mDiagnosticHal = null;

    /** Might be re-assigned if Vehicle HAL is reconnected. */
    private volatile HalClient mHalClient;

    /** Stores handler for each HAL property. Property events are sent to handler. */
    private final SparseArray<HalServiceBase> mPropertyHandlers = new SparseArray<>();
    /** This is for iterating all HalServices with fixed order. */
    private final ArrayList<HalServiceBase> mAllServices = new ArrayList<>();
    private final HashMap<Integer, SubscribeOptions> mSubscribedProperties = new HashMap<>();
    private final HashMap<Integer, VehiclePropConfig> mAllProperties = new HashMap<>();
    private final HashMap<Integer, VehiclePropertyEventInfo> mEventLog = new HashMap<>();

    // Used by injectVHALEvent for testing purposes.  Delimiter for an array of data
    private static final String DATA_DELIMITER = ",";

    public VehicleHal(IVehicle vehicle) {
        mHandlerThread = new HandlerThread("VEHICLE-HAL");
        mHandlerThread.start();
        // passing this should be safe as long as it is just kept and not used in constructor
        mPowerHal = new PowerHalService(this);
        mPropertyHal = new PropertyHalService(this);
        mInputHal = new InputHalService(this);
        mVmsHal = new VmsHalService(this);
        mDiagnosticHal = new DiagnosticHalService(this);
        mAllServices.addAll(Arrays.asList(mPowerHal,
                mInputHal,
                mPropertyHal,
                mDiagnosticHal,
                mVmsHal));

        mHalClient = new HalClient(vehicle, mHandlerThread.getLooper(), this /*IVehicleCallback*/);
    }

    /** Dummy version only for testing */
    @VisibleForTesting
    public VehicleHal(PowerHalService powerHal, DiagnosticHalService diagnosticHal,
            HalClient halClient, PropertyHalService propertyHal) {
        mHandlerThread = null;
        mPowerHal = powerHal;
        mPropertyHal = propertyHal;
        mDiagnosticHal = diagnosticHal;
        mInputHal = null;
        mVmsHal = null;
        mHalClient = halClient;
        mDiagnosticHal = diagnosticHal;
    }

    public void vehicleHalReconnected(IVehicle vehicle) {
        synchronized (this) {
            mHalClient = new HalClient(vehicle, mHandlerThread.getLooper(),
                    this /*IVehicleCallback*/);

            SubscribeOptions[] options = mSubscribedProperties.values()
                    .toArray(new SubscribeOptions[0]);

            try {
                mHalClient.subscribe(options);
            } catch (RemoteException e) {
                throw new RuntimeException("Failed to subscribe: " + Arrays.asList(options), e);
            }
        }
    }

    public void init() {
        Set<VehiclePropConfig> properties;
        try {
            properties = new HashSet<>(mHalClient.getAllPropConfigs());
        } catch (RemoteException e) {
            throw new RuntimeException("Unable to retrieve vehicle property configuration", e);
        }

        synchronized (this) {
            // Create map of all properties
            for (VehiclePropConfig p : properties) {
                mAllProperties.put(p.prop, p);
            }
        }

        for (HalServiceBase service: mAllServices) {
            Collection<VehiclePropConfig> taken = service.takeSupportedProperties(properties);
            if (taken == null) {
                continue;
            }
            if (DBG) {
                Log.i(CarLog.TAG_HAL, "HalService " + service + " take properties " + taken.size());
            }
            synchronized (this) {
                for (VehiclePropConfig p: taken) {
                    mPropertyHandlers.append(p.prop, service);
                }
            }
            properties.removeAll(taken);
            service.init();
        }
    }

    public void release() {
        // release in reverse order from init
        for (int i = mAllServices.size() - 1; i >= 0; i--) {
            mAllServices.get(i).release();
        }
        synchronized (this) {
            for (int p : mSubscribedProperties.keySet()) {
                try {
                    mHalClient.unsubscribe(p);
                } catch (RemoteException e) {
                    //  Ignore exceptions on shutdown path.
                    Log.w(CarLog.TAG_HAL, "Failed to unsubscribe", e);
                }
            }
            mSubscribedProperties.clear();
            mAllProperties.clear();
        }
        // keep the looper thread as should be kept for the whole life cycle.
    }

    public DiagnosticHalService getDiagnosticHal() { return mDiagnosticHal; }

    public PowerHalService getPowerHal() {
        return mPowerHal;
    }

    public PropertyHalService getPropertyHal() {
        return mPropertyHal;
    }

    public InputHalService getInputHal() {
        return mInputHal;
    }

    public VmsHalService getVmsHal() { return mVmsHal; }

    private void assertServiceOwnerLocked(HalServiceBase service, int property) {
        if (service != mPropertyHandlers.get(property)) {
            throw new IllegalArgumentException("Property 0x" + toHexString(property)
                    + " is not owned by service: " + service);
        }
    }

    /**
     * Subscribes given properties with sampling rate defaults to 0 and no special flags provided.
     *
     * @see #subscribeProperty(HalServiceBase, int, float, int)
     */
    public void subscribeProperty(HalServiceBase service, int property)
            throws IllegalArgumentException {
        subscribeProperty(service, property, 0f, SubscribeFlags.EVENTS_FROM_CAR);
    }

    /**
     * Subscribes given properties with default subscribe flag.
     *
     * @see #subscribeProperty(HalServiceBase, int, float, int)
     */
    public void subscribeProperty(HalServiceBase service, int property, float sampleRateHz)
            throws IllegalArgumentException {
        subscribeProperty(service, property, sampleRateHz, SubscribeFlags.EVENTS_FROM_CAR);
    }

    /**
     * Subscribe given property. Only Hal service owning the property can subscribe it.
     *
     * @param service HalService that owns this property
     * @param property property id (VehicleProperty)
     * @param samplingRateHz sampling rate in Hz for continuous properties
     * @param flags flags from {@link android.hardware.automotive.vehicle.V2_0.SubscribeFlags}
     * @throws IllegalArgumentException thrown if property is not supported by VHAL
     */
    public void subscribeProperty(HalServiceBase service, int property,
            float samplingRateHz, int flags) throws IllegalArgumentException {
        if (DBG) {
            Log.i(CarLog.TAG_HAL, "subscribeProperty, service:" + service
                    + ", property: 0x" + toHexString(property));
        }
        VehiclePropConfig config;
        synchronized (this) {
            config = mAllProperties.get(property);
        }

        if (config == null) {
            throw new IllegalArgumentException("subscribe error: config is null for property 0x" +
                    toHexString(property));
        } else if (isPropertySubscribable(config)) {
            SubscribeOptions opts = new SubscribeOptions();
            opts.propId = property;
            opts.sampleRate = samplingRateHz;
            opts.flags = flags;
            synchronized (this) {
                assertServiceOwnerLocked(service, property);
                mSubscribedProperties.put(property, opts);
            }
            try {
                mHalClient.subscribe(opts);
            } catch (RemoteException e) {
                Log.e(CarLog.TAG_HAL, "Failed to subscribe to property: 0x" + property, e);
            }
        } else {
            Log.e(CarLog.TAG_HAL, "Cannot subscribe to property: " + property);
        }
    }

    public void unsubscribeProperty(HalServiceBase service, int property) {
        if (DBG) {
            Log.i(CarLog.TAG_HAL, "unsubscribeProperty, service:" + service
                    + ", property: 0x" + toHexString(property));
        }
        VehiclePropConfig config;
        synchronized (this) {
            config = mAllProperties.get(property);
        }

        if (config == null) {
            Log.e(CarLog.TAG_HAL, "unsubscribeProperty: property " + property + " does not exist");
        } else if (isPropertySubscribable(config)) {
            synchronized (this) {
                assertServiceOwnerLocked(service, property);
                mSubscribedProperties.remove(property);
            }
            try {
                mHalClient.unsubscribe(property);
            } catch (RemoteException e) {
                Log.e(CarLog.TAG_SERVICE, "Failed to unsubscribe from property: 0x"
                        + toHexString(property), e);
            }
        } else {
            Log.e(CarLog.TAG_HAL, "Cannot unsubscribe property: " + property);
        }
    }

    public boolean isPropertySupported(int propertyId) {
        return mAllProperties.containsKey(propertyId);
    }

    public Collection<VehiclePropConfig> getAllPropConfigs() {
        return mAllProperties.values();
    }

    public VehiclePropValue get(int propertyId) throws PropertyTimeoutException {
        return get(propertyId, NO_AREA);
    }

    public VehiclePropValue get(int propertyId, int areaId) throws PropertyTimeoutException {
        if (DBG) {
            Log.i(CarLog.TAG_HAL, "get, property: 0x" + toHexString(propertyId)
                    + ", areaId: 0x" + toHexString(areaId));
        }
        VehiclePropValue propValue = new VehiclePropValue();
        propValue.prop = propertyId;
        propValue.areaId = areaId;
        return mHalClient.getValue(propValue);
    }

    public <T> T get(Class clazz, int propertyId) throws PropertyTimeoutException {
        return get(clazz, createPropValue(propertyId, NO_AREA));
    }

    public <T> T get(Class clazz, int propertyId, int areaId) throws PropertyTimeoutException {
        return get(clazz, createPropValue(propertyId, areaId));
    }

    @SuppressWarnings("unchecked")
    public <T> T get(Class clazz, VehiclePropValue requestedPropValue)
            throws PropertyTimeoutException {
        VehiclePropValue propValue;
        propValue = mHalClient.getValue(requestedPropValue);

        if (clazz == Integer.class || clazz == int.class) {
            return (T) propValue.value.int32Values.get(0);
        } else if (clazz == Boolean.class || clazz == boolean.class) {
            return (T) Boolean.valueOf(propValue.value.int32Values.get(0) == 1);
        } else if (clazz == Float.class || clazz == float.class) {
            return (T) propValue.value.floatValues.get(0);
        } else if (clazz == Integer[].class) {
            Integer[] intArray = new Integer[propValue.value.int32Values.size()];
            return (T) propValue.value.int32Values.toArray(intArray);
        } else if (clazz == Float[].class) {
            Float[] floatArray = new Float[propValue.value.floatValues.size()];
            return (T) propValue.value.floatValues.toArray(floatArray);
        } else if (clazz == int[].class) {
            return (T) toIntArray(propValue.value.int32Values);
        } else if (clazz == float[].class) {
            return (T) toFloatArray(propValue.value.floatValues);
        } else if (clazz == byte[].class) {
            return (T) toByteArray(propValue.value.bytes);
        } else if (clazz == String.class) {
            return (T) propValue.value.stringValue;
        } else {
            throw new IllegalArgumentException("Unexpected type: " + clazz);
        }
    }

    public VehiclePropValue get(VehiclePropValue requestedPropValue)
            throws PropertyTimeoutException {
        return mHalClient.getValue(requestedPropValue);
    }

    /**
     *
     * @param propId Property ID to return the current sample rate for.
     *
     * @return float Returns the current sample rate of the specified propId, or -1 if the
     *                  property is not currently subscribed.
     */
    public float getSampleRate(int propId) {
        SubscribeOptions opts = mSubscribedProperties.get(propId);
        if (opts == null) {
            // No sample rate for this property
            return -1;
        } else {
            return opts.sampleRate;
        }
    }

    void set(VehiclePropValue propValue) throws PropertyTimeoutException {
        mHalClient.setValue(propValue);
    }

    @CheckResult
    VehiclePropValueSetter set(int propId) {
        return new VehiclePropValueSetter(mHalClient, propId, NO_AREA);
    }

    @CheckResult
    VehiclePropValueSetter set(int propId, int areaId) {
        return new VehiclePropValueSetter(mHalClient, propId, areaId);
    }

    static boolean isPropertySubscribable(VehiclePropConfig config) {
        if ((config.access & VehiclePropertyAccess.READ) == 0 ||
                (config.changeMode == VehiclePropertyChangeMode.STATIC)) {
            return false;
        }
        return true;
    }

    static void dumpProperties(PrintWriter writer, Collection<VehiclePropConfig> configs) {
        for (VehiclePropConfig config : configs) {
            writer.println(String.format("property 0x%x", config.prop));
        }
    }

    private final ArraySet<HalServiceBase> mServicesToDispatch = new ArraySet<>();

    @Override
    public void onPropertyEvent(ArrayList<VehiclePropValue> propValues) {
        synchronized (this) {
            for (VehiclePropValue v : propValues) {
                HalServiceBase service = mPropertyHandlers.get(v.prop);
                if(service == null) {
                    Log.e(CarLog.TAG_HAL, "HalService not found for prop: 0x"
                        + toHexString(v.prop));
                    continue;
                }
                service.getDispatchList().add(v);
                mServicesToDispatch.add(service);
                VehiclePropertyEventInfo info = mEventLog.get(v.prop);
                if (info == null) {
                    info = new VehiclePropertyEventInfo(v);
                    mEventLog.put(v.prop, info);
                } else {
                    info.addNewEvent(v);
                }
            }
        }
        for (HalServiceBase s : mServicesToDispatch) {
            s.handleHalEvents(s.getDispatchList());
            s.getDispatchList().clear();
        }
        mServicesToDispatch.clear();
    }

    @Override
    public void onPropertySet(VehiclePropValue value) {
        // No need to handle on-property-set events in HAL service yet.
    }

    @Override
    public void onPropertySetError(int errorCode, int propId, int areaId) {
        Log.e(CarLog.TAG_HAL, String.format("onPropertySetError, errorCode: %d, prop: 0x%x, "
                + "area: 0x%x", errorCode, propId, areaId));
        if (propId != VehicleProperty.INVALID) {
            HalServiceBase service = mPropertyHandlers.get(propId);
            if (service != null) {
                service.handlePropertySetError(propId, areaId);
            }
        }
    }

    public void dump(PrintWriter writer) {
        writer.println("**dump HAL services**");
        for (HalServiceBase service: mAllServices) {
            service.dump(writer);
        }

        List<VehiclePropConfig> configList;
        synchronized (this) {
            configList = new ArrayList<>(mAllProperties.values());
        }

        writer.println("**All properties**");
        for (VehiclePropConfig config : configList) {
            StringBuilder builder = new StringBuilder()
                    .append("Property:0x").append(toHexString(config.prop))
                    .append(",Property name:").append(VehicleProperty.toString(config.prop))
                    .append(",access:0x").append(toHexString(config.access))
                    .append(",changeMode:0x").append(toHexString(config.changeMode))
                    .append(",config:0x").append(Arrays.toString(config.configArray.toArray()))
                    .append(",fs min:").append(config.minSampleRate)
                    .append(",fs max:").append(config.maxSampleRate);
            for (VehicleAreaConfig area : config.areaConfigs) {
                builder.append(",areaId :").append(toHexString(area.areaId))
                        .append(",f min:").append(area.minFloatValue)
                        .append(",f max:").append(area.maxFloatValue)
                        .append(",i min:").append(area.minInt32Value)
                        .append(",i max:").append(area.maxInt32Value)
                        .append(",i64 min:").append(area.minInt64Value)
                        .append(",i64 max:").append(area.maxInt64Value);
            }
            writer.println(builder.toString());
        }
        writer.println(String.format("**All Events, now ns:%d**",
                SystemClock.elapsedRealtimeNanos()));
        for (VehiclePropertyEventInfo info : mEventLog.values()) {
            writer.println(String.format("event count:%d, lastEvent:%s",
                    info.eventCount, dumpVehiclePropValue(info.lastEvent)));
        }

        writer.println("**Property handlers**");
        for (int i = 0; i < mPropertyHandlers.size(); i++) {
            int propId = mPropertyHandlers.keyAt(i);
            HalServiceBase service = mPropertyHandlers.valueAt(i);
            writer.println(String.format("Prop: 0x%08X, service: %s", propId, service));
        }
    }

    /**
     * Inject a VHAL event
     *
     * @param property the Vehicle property Id as defined in the HAL
     * @param zone     Zone that this event services
     * @param value    Data value of the event
     */
    public void injectVhalEvent(String property, String zone, String value)
            throws NumberFormatException {
        if (value == null || zone == null || property == null) {
            return;
        }
        int propId = Integer.decode(property);
        int zoneId = Integer.decode(zone);
        VehiclePropValue v = createPropValue(propId, zoneId);
        int propertyType = propId & VehiclePropertyType.MASK;
        // Values can be comma separated list
        List<String> dataList = new ArrayList<>(Arrays.asList(value.split(DATA_DELIMITER)));
        switch (propertyType) {
            case VehiclePropertyType.BOOLEAN:
                boolean boolValue = Boolean.valueOf(value);
                v.value.int32Values.add(boolValue ? 1 : 0);
                break;
            case VehiclePropertyType.INT32:
            case VehiclePropertyType.INT32_VEC:
                for (String s : dataList) {
                    v.value.int32Values.add(Integer.decode(s));
                }
                break;
            case VehiclePropertyType.FLOAT:
            case VehiclePropertyType.FLOAT_VEC:
                for (String s : dataList) {
                    v.value.floatValues.add(Float.parseFloat(s));
                }
                break;
            default:
                Log.e(CarLog.TAG_HAL, "Property type unsupported:" + propertyType);
                return;
        }
        v.timestamp = SystemClock.elapsedRealtimeNanos();
        onPropertyEvent(Lists.newArrayList(v));
    }

    private static class VehiclePropertyEventInfo {
        private int eventCount;
        private VehiclePropValue lastEvent;

        private VehiclePropertyEventInfo(VehiclePropValue event) {
            eventCount = 1;
            lastEvent = event;
        }

        private void addNewEvent(VehiclePropValue event) {
            eventCount++;
            lastEvent = event;
        }
    }

    final class VehiclePropValueSetter {
        final WeakReference<HalClient> mClient;
        final VehiclePropValue mPropValue;

        private VehiclePropValueSetter(HalClient client, int propId, int areaId) {
            mClient = new WeakReference<>(client);
            mPropValue = new VehiclePropValue();
            mPropValue.prop = propId;
            mPropValue.areaId = areaId;
        }

        void to(boolean value) throws PropertyTimeoutException {
            to(value ? 1 : 0);
        }

        void to(int value) throws PropertyTimeoutException {
            mPropValue.value.int32Values.add(value);
            submit();
        }

        void to(int[] values) throws PropertyTimeoutException {
            for (int value : values) {
                mPropValue.value.int32Values.add(value);
            }
            submit();
        }

        void to(Collection<Integer> values) throws PropertyTimeoutException {
            mPropValue.value.int32Values.addAll(values);
            submit();
        }

        void submit() throws PropertyTimeoutException {
            HalClient client =  mClient.get();
            if (client != null) {
                if (DBG) {
                    Log.i(CarLog.TAG_HAL, "set, property: 0x" + toHexString(mPropValue.prop)
                            + ", areaId: 0x" + toHexString(mPropValue.areaId));
                }
                client.setValue(mPropValue);
            }
        }
    }

    private static String dumpVehiclePropValue(VehiclePropValue value) {
        final int MAX_BYTE_SIZE = 20;

        StringBuilder sb = new StringBuilder()
                .append("Property:0x").append(toHexString(value.prop))
                .append(",timestamp:").append(value.timestamp)
                .append(",zone:0x").append(toHexString(value.areaId))
                .append(",floatValues: ").append(Arrays.toString(value.value.floatValues.toArray()))
                .append(",int32Values: ").append(Arrays.toString(value.value.int32Values.toArray()))
                .append(",int64Values: ")
                .append(Arrays.toString(value.value.int64Values.toArray()));

        if (value.value.bytes.size() > MAX_BYTE_SIZE) {
            Object[] bytes = Arrays.copyOf(value.value.bytes.toArray(), MAX_BYTE_SIZE);
            sb.append(",bytes: ").append(Arrays.toString(bytes));
        } else {
            sb.append(",bytes: ").append(Arrays.toString(value.value.bytes.toArray()));
        }
        sb.append(",string: ").append(value.value.stringValue);

        return sb.toString();
    }

    private static VehiclePropValue createPropValue(int propId, int areaId) {
        VehiclePropValue propValue = new VehiclePropValue();
        propValue.prop = propId;
        propValue.areaId = areaId;
        return propValue;
    }
}
