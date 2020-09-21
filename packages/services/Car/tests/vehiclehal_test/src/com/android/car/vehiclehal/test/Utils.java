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

package com.android.car.vehiclehal.test;

import static java.lang.Integer.toHexString;

import android.car.hardware.CarPropertyValue;
import android.hardware.automotive.vehicle.V2_0.IVehicle;
import android.hardware.automotive.vehicle.V2_0.VehiclePropConfig;
import android.hardware.automotive.vehicle.V2_0.VehiclePropValue;
import android.os.HidlSupport;
import android.os.RemoteException;
import android.util.Log;

import com.android.car.vehiclehal.VehiclePropValueBuilder;

import java.util.NoSuchElementException;
import java.util.Objects;

final class Utils {
    private Utils() {}

    private static final String TAG = concatTag(Utils.class);

    static String concatTag(Class clazz) {
        return "VehicleHalTest." + clazz.getSimpleName();
    }

    static boolean isVhalPropertyAvailable(IVehicle vehicle, int prop) throws RemoteException {
        return vehicle.getAllPropConfigs()
                .stream()
                .anyMatch((VehiclePropConfig config) -> config.prop == prop);
    }

    static VehiclePropValue readVhalProperty(
        IVehicle vehicle,
        VehiclePropValue request,
        java.util.function.BiFunction<Integer, VehiclePropValue, Boolean> f) {
        Objects.requireNonNull(vehicle);
        Objects.requireNonNull(request);
        VehiclePropValue vpv[] = new VehiclePropValue[] {null};
        try {
            vehicle.get(
                request,
                (int status, VehiclePropValue propValue) -> {
                    if (f.apply(status, propValue)) {
                        vpv[0] = propValue;
                    }
                });
        } catch (RemoteException e) {
            Log.w(TAG, "attempt to read VHAL property " + request + " caused RemoteException: ", e);
        }
        return vpv[0];
    }

    static VehiclePropValue readVhalProperty(
        IVehicle vehicle,
        int propertyId,
        java.util.function.BiFunction<Integer, VehiclePropValue, Boolean> f) {
        return readVhalProperty(vehicle, propertyId, 0, f);
    }

    static VehiclePropValue readVhalProperty(
            IVehicle vehicle,
            int propertyId,
            int areaId,
            java.util.function.BiFunction<Integer, VehiclePropValue, Boolean> f) {
        VehiclePropValue request =
            VehiclePropValueBuilder.newBuilder(propertyId).setAreaId(areaId).build();
        return readVhalProperty(vehicle, request, f);
    }

    static IVehicle getVehicle() throws RemoteException {
        IVehicle service;
        try {
            service = IVehicle.getService();
        } catch (NoSuchElementException ex) {
            throw new RuntimeException("Couldn't connect to vehicle@2.0", ex);
        }
        Log.d(TAG, "Connected to IVehicle service: " + service);
        return service;
    }

    /**
     * Check the equality of two VehiclePropValue object ignoring timestamp and status.
     *
     * @param value1
     * @param value2
     * @return true if equal
     */
    static boolean areVehiclePropValuesEqual(final VehiclePropValue value1,
            final VehiclePropValue value2) {
        return value1 == value2
            || value1 != null
            && value2 != null
            && value1.prop == value2.prop
            && value1.areaId == value2.areaId
            && HidlSupport.deepEquals(value1.value, value2.value);
    }

    /**
     * The method will convert prop ID to hexadecimal format, and omit timestamp and status
     *
     * @param value
     * @return String
     */
    static String vehiclePropValueToString(final VehiclePropValue value) {
        return "{.prop = 0x" + toHexString(value.prop)
               + ", .areaId = " + value.areaId
               + ", .value = " + value.value + "}";
    }

    static VehiclePropValue fromHvacPropertyValue(CarPropertyValue value) {
        VehiclePropValueBuilder builder =
                VehiclePropValueBuilder.newBuilder(
                    VhalPropMaps.getHvacVhalProp(value.getPropertyId()));
        return fromCarPropertyValue(value, builder);
    }

    private static VehiclePropValue fromCarPropertyValue(
            CarPropertyValue value, VehiclePropValueBuilder builder) {
        builder.setAreaId(value.getAreaId()).setTimestamp(value.getTimestamp());

        //TODO: Consider move this conversion to VehiclePropValueBuilder
        Object o = value.getValue();
        if (o instanceof Boolean) {
            builder.addIntValue((boolean) o ? 1 : 0);
        } else if (o instanceof Integer) {
            builder.addIntValue((int) o);
        } else if (o instanceof Float) {
            builder.addFloatValue((float) o);
        } else if (o instanceof Long) {
            builder.setInt64Value((long) o);
        } else if (o instanceof String) {
            builder.setStringValue((String) o);
        } else { //TODO: Add support for MIXED type
            throw new IllegalArgumentException("Unrecognized car property value type, "
                    + o.getClass());
        }
        return builder.build();
    }
}
