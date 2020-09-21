/*
 * Copyright (C) 2016 The Android Open Source Project
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

import static java.lang.Integer.toHexString;

import android.car.VehicleAreaType;
import android.car.hardware.CarPropertyConfig;
import android.car.hardware.CarPropertyValue;
import android.hardware.automotive.vehicle.V2_0.VehicleArea;
import android.hardware.automotive.vehicle.V2_0.VehicleAreaConfig;
import android.hardware.automotive.vehicle.V2_0.VehiclePropConfig;
import android.hardware.automotive.vehicle.V2_0.VehiclePropValue;
import android.hardware.automotive.vehicle.V2_0.VehiclePropertyType;

import java.util.Collections;
import java.util.List;

/**
 * Utility functions to work with {@link CarPropertyConfig} and {@link CarPropertyValue}
 */
/*package*/ final class CarPropertyUtils {

    /* Utility class has no public constructor */
    private CarPropertyUtils() {}

    /** Converts {@link VehiclePropValue} to {@link CarPropertyValue} */
    static CarPropertyValue<?> toCarPropertyValue(
            VehiclePropValue halValue, int propertyId) {
        Class<?> clazz = getJavaClass(halValue.prop & VehiclePropertyType.MASK);
        int areaId = halValue.areaId;
        int status = halValue.status;
        long timestamp = halValue.timestamp;
        VehiclePropValue.RawValue v = halValue.value;

        if (Boolean.class == clazz) {
            return new CarPropertyValue<>(propertyId, areaId, status, timestamp,
                                          v.int32Values.get(0) == 1);
        } else if (Boolean[].class == clazz) {
            Boolean[] values = new Boolean[v.int32Values.size()];
            for (int i = 0; i < values.length; i++) {
                values[i] = v.int32Values.get(i) == 1;
            }
            return new CarPropertyValue<>(propertyId, areaId, status, timestamp, values);
        } else if (String.class == clazz) {
            return new CarPropertyValue<>(propertyId, areaId, status, timestamp, v.stringValue);
        } else if (byte[].class == clazz) {
            byte[] halData = toByteArray(v.bytes);
            return new CarPropertyValue<>(propertyId, areaId, status, timestamp, halData);
        } else if (Long[].class == clazz) {
            Long[] values = new Long[v.int64Values.size()];
            for (int i = 0; i < values.length; i++) {
                values[i] = v.int64Values.get(i);
            }
            return new CarPropertyValue<>(propertyId, areaId, status, timestamp, values);
        } else /* All list properties */ {
            Object[] values = getRawValueList(clazz, v).toArray();
            return new CarPropertyValue<>(propertyId, areaId, status, timestamp,
                    values.length == 1 ? values[0] : values);
        }
    }

    /** Converts {@link CarPropertyValue} to {@link VehiclePropValue} */
    static VehiclePropValue toVehiclePropValue(CarPropertyValue carProp, int halPropId) {
        VehiclePropValue vehicleProp = new VehiclePropValue();
        vehicleProp.prop = halPropId;
        vehicleProp.areaId = carProp.getAreaId();
        VehiclePropValue.RawValue v = vehicleProp.value;

        Object o = carProp.getValue();

        if (o instanceof Boolean) {
            v.int32Values.add(((Boolean) o) ? 1 : 0);
        } else if (o instanceof Boolean[]) {
            for (Boolean b : (Boolean[]) o) {
                v.int32Values.add(((Boolean) o) ? 1 : 0);
            }
        } else if (o instanceof Integer) {
            v.int32Values.add((Integer) o);
        } else if (o instanceof Integer[]) {
            Collections.addAll(v.int32Values, (Integer[]) o);
        } else if (o instanceof Float) {
            v.floatValues.add((Float) o);
        } else if (o instanceof Float[]) {
            Collections.addAll(v.floatValues, (Float[]) o);
        } else if (o instanceof Long) {
            v.int64Values.add((Long) o);
        } else if (o instanceof Long[]) {
            Collections.addAll(v.int64Values, (Long[]) o);
        } else if (o instanceof String) {
            v.stringValue = (String) o;
        } else if (o instanceof byte[]) {
            for (byte b : (byte[]) o) {
                v.bytes.add(b);
            }
        } else {
            throw new IllegalArgumentException("Unexpected type in: " + carProp);
        }

        return vehicleProp;
    }

    /**
     * Converts {@link VehiclePropConfig} to {@link CarPropertyConfig}.
     */
    static CarPropertyConfig<?> toCarPropertyConfig(VehiclePropConfig p, int propertyId) {
        int areaType = getVehicleAreaType(p.prop & VehicleArea.MASK);
        // Create list of areaIds for this property
        int[] areas = new int[p.areaConfigs.size()];
        for (int i=0; i<p.areaConfigs.size(); i++) {
            areas[i] = p.areaConfigs.get(i).areaId;
        }

        Class<?> clazz = getJavaClass(p.prop & VehiclePropertyType.MASK);
        if (p.areaConfigs.isEmpty()) {
            return CarPropertyConfig
                    .newBuilder(clazz, propertyId, areaType, /* capacity */ 1)
                    .addAreas(areas)
                    .setAccess(p.access)
                    .setChangeMode(p.changeMode)
                    .setConfigArray(p.configArray)
                    .setConfigString(p.configString)
                    .setMaxSampleRate(p.maxSampleRate)
                    .setMinSampleRate(p.minSampleRate)
                    .build();
        } else {
            CarPropertyConfig.Builder builder = CarPropertyConfig
                    .newBuilder(clazz, propertyId, areaType, /* capacity */ p.areaConfigs.size())
                    .setAccess(p.access)
                    .setChangeMode(p.changeMode)
                    .setConfigArray(p.configArray)
                    .setConfigString(p.configString)
                    .setMaxSampleRate(p.maxSampleRate)
                    .setMinSampleRate(p.minSampleRate);

            for (VehicleAreaConfig area : p.areaConfigs) {
                if (classMatched(Integer.class, clazz)) {
                    builder.addAreaConfig(area.areaId, area.minInt32Value, area.maxInt32Value);
                } else if (classMatched(Float.class, clazz)) {
                    builder.addAreaConfig(area.areaId, area.minFloatValue, area.maxFloatValue);
                } else if (classMatched(Long.class, clazz)) {
                    builder.addAreaConfig(area.areaId, area.minInt64Value, area.maxInt64Value);
                } else if (classMatched(Boolean.class, clazz) ||
                           classMatched(Float[].class, clazz) ||
                           classMatched(Integer[].class, clazz) ||
                           classMatched(Long[].class, clazz) ||
                           classMatched(String.class, clazz) ||
                           classMatched(byte[].class, clazz) ||
                           classMatched(Object.class, clazz)) {
                    // These property types do not have min/max values
                    builder.addArea(area.areaId);
                } else {
                    throw new IllegalArgumentException("Unexpected type: " + clazz);
                }
            }

            return builder.build();
        }
    }

    private static @VehicleAreaType.VehicleAreaTypeValue int getVehicleAreaType(int halArea) {
        switch (halArea) {
            case VehicleArea.GLOBAL:
                return VehicleAreaType.VEHICLE_AREA_TYPE_GLOBAL;
            case VehicleArea.SEAT:
                return VehicleAreaType.VEHICLE_AREA_TYPE_SEAT;
            case VehicleArea.DOOR:
                return VehicleAreaType.VEHICLE_AREA_TYPE_DOOR;
            case VehicleArea.WINDOW:
                return VehicleAreaType.VEHICLE_AREA_TYPE_WINDOW;
            case VehicleArea.MIRROR:
                return VehicleAreaType.VEHICLE_AREA_TYPE_MIRROR;
            case VehicleArea.WHEEL:
                return VehicleAreaType.VEHICLE_AREA_TYPE_WHEEL;
            default:
                throw new RuntimeException("Unsupported area type " + halArea);
        }
    }

    private static Class<?> getJavaClass(int halType) {
        switch (halType) {
            case VehiclePropertyType.BOOLEAN:
                return Boolean.class;
            case VehiclePropertyType.FLOAT:
                return Float.class;
            case VehiclePropertyType.INT32:
                return Integer.class;
            case VehiclePropertyType.INT64:
                return Long.class;
            case VehiclePropertyType.FLOAT_VEC:
                return Float[].class;
            case VehiclePropertyType.INT32_VEC:
                return Integer[].class;
            case VehiclePropertyType.INT64_VEC:
                return Long[].class;
            case VehiclePropertyType.STRING:
                return String.class;
            case VehiclePropertyType.BYTES:
                return byte[].class;
            case VehiclePropertyType.MIXED:
                return Object.class;
            default:
                throw new IllegalArgumentException("Unexpected type: " + toHexString(halType));
        }
    }

    private static List getRawValueList(Class<?> clazz, VehiclePropValue.RawValue value) {
        if (classMatched(Float.class, clazz) || classMatched(Float[].class, clazz)) {
            return value.floatValues;
        } else if (classMatched(Integer.class, clazz) || classMatched(Integer[].class, clazz)) {
            return value.int32Values;
        } else if (classMatched(Long.class, clazz) || classMatched(Long[].class, clazz)) {
            return value.int64Values;
        } else {
            throw new IllegalArgumentException("Unexpected type: " + clazz);
        }
    }

    private static boolean classMatched(Class<?> class1, Class<?> class2) {
        return class1 == class2 || class1.getComponentType() == class2;
    }
}
