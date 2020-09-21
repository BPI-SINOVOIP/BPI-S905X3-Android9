
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

package com.google.android.car.kitchensink;

import android.car.Car;
import android.content.Context;
import android.hardware.automotive.vehicle.V2_0.VehicleHwKeyInputAction;
import android.hardware.automotive.vehicle.V2_0.VehiclePropValue;
import android.hardware.automotive.vehicle.V2_0.VehicleProperty;
import android.hardware.automotive.vehicle.V2_0.VehiclePropertyAccess;
import android.os.SystemClock;

import com.android.car.ICarImpl;
import com.android.car.systeminterface.SystemInterface;
import com.android.car.vehiclehal.VehiclePropValueBuilder;
import com.android.car.vehiclehal.test.MockedVehicleHal;
import com.android.car.vehiclehal.test.MockedVehicleHal.VehicleHalPropertyHandler;
import com.android.car.vehiclehal.test.VehiclePropConfigBuilder;

public class CarEmulator {

    private final Car mCar;
    private final MockedVehicleHal mHalEmulator;

    public static CarEmulator create(Context context) {
        CarEmulator emulator = new CarEmulator(context);
        emulator.init();
        return emulator;
    }

    private CarEmulator(Context context) {
        mHalEmulator = new MockedVehicleHal();
        ICarImpl carService = new ICarImpl(context, mHalEmulator,
                SystemInterface.Builder.defaultSystemInterface(context).build(),
                null /* error notifier */, "CarEmulator");
        mCar = new Car(context, carService, null /* Handler */);
    }

    public Car getCar() {
        return mCar;
    }

    private void init() {
        mHalEmulator.addProperty(
                VehiclePropConfigBuilder.newBuilder(VehicleProperty.HW_KEY_INPUT)
                        .setAccess(VehiclePropertyAccess.READ)
                        .build(),
                mHWKeyHandler);
    }

    public void start() {
    }

    public void stop() {
    }

    public void injectKey(int keyCode, int action) {
        VehiclePropValue injectValue =
                VehiclePropValueBuilder.newBuilder(VehicleProperty.HW_KEY_INPUT)
                        .setTimestamp()
                        .addIntValue(action, keyCode, 0, 0)
                        .build();

        mHalEmulator.injectEvent(injectValue);
    }

    public void injectKey(int keyCode) {
        injectKey(keyCode, VehicleHwKeyInputAction.ACTION_DOWN);
        injectKey(keyCode, VehicleHwKeyInputAction.ACTION_UP);
    }

    private final VehicleHalPropertyHandler mHWKeyHandler =
            new VehicleHalPropertyHandler() {
                @Override
                public VehiclePropValue onPropertyGet(VehiclePropValue value) {
                    return VehiclePropValueBuilder.newBuilder(VehicleProperty.HW_KEY_INPUT)
                            .setTimestamp(SystemClock.elapsedRealtimeNanos())
                            .addIntValue(0, 0, 0, 0)
                            .build();

                }
            };
}
