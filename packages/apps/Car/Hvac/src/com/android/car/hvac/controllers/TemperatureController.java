/*
 * Copyright (c) 2016, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.android.car.hvac.controllers;

import android.car.hardware.CarPropertyValue;
import com.android.car.hvac.HvacController;
import com.android.car.hvac.ui.TemperatureBarOverlay;

/**
 * A controller that handles temperature updates for the driver and passenger.
 */
public class TemperatureController {
    private final TemperatureBarOverlay mDriverTempBarExpanded;
    private final TemperatureBarOverlay mPassengerTempBarExpanded;
    private final TemperatureBarOverlay mDriverTempBarCollapsed;
    private final TemperatureBarOverlay mPassengerTempBarCollapsed;
    private final HvacController mHvacController;

    //TODO: builder pattern for clarity
    public TemperatureController(TemperatureBarOverlay passengerTemperatureBarExpanded,
            TemperatureBarOverlay driverTemperatureBarExpanded,
            TemperatureBarOverlay passengerTemperatureBarCollapsed,
            TemperatureBarOverlay driverTemperatureBarCollapsed,
            HvacController controller) {
        mDriverTempBarExpanded = driverTemperatureBarExpanded;
        mPassengerTempBarExpanded = passengerTemperatureBarExpanded;
        mPassengerTempBarCollapsed = passengerTemperatureBarCollapsed;
        mDriverTempBarCollapsed = driverTemperatureBarCollapsed;
        mHvacController = controller;

        mHvacController.registerCallback(mCallback);
        mDriverTempBarExpanded.setTemperatureChangeListener(mDriverTempClickListener);
        mPassengerTempBarExpanded.setTemperatureChangeListener(mPassengerTempClickListener);

        final boolean isDriverTempControlAvailable =
                mHvacController.isDriverTemperatureControlAvailable();
        mDriverTempBarExpanded.setAvailable(isDriverTempControlAvailable);
        mDriverTempBarCollapsed.setAvailable(isDriverTempControlAvailable);
        if (isDriverTempControlAvailable) {
            mDriverTempBarExpanded.setTemperature(mHvacController.getDriverTemperature());
            mDriverTempBarCollapsed.setTemperature(mHvacController.getDriverTemperature());
        }

        final boolean isPassengerTempControlAvailable =
            mHvacController.isPassengerTemperatureControlAvailable();
        mPassengerTempBarExpanded.setAvailable(isPassengerTempControlAvailable);
        mPassengerTempBarCollapsed.setAvailable(isPassengerTempControlAvailable);
        if (isPassengerTempControlAvailable) {
            mPassengerTempBarExpanded.setTemperature(mHvacController.getPassengerTemperature());
            mPassengerTempBarCollapsed.setTemperature(mHvacController.getPassengerTemperature());
        }
    }

    private final HvacController.Callback mCallback = new HvacController.Callback() {
        @Override
        public void onPassengerTemperatureChange(CarPropertyValue value) {
            final boolean available = value.getStatus() == CarPropertyValue.STATUS_AVAILABLE;
            mPassengerTempBarExpanded.setAvailable(available);
            mPassengerTempBarCollapsed.setAvailable(available);
            if (available) {
                final int temp = ((Float)value.getValue()).intValue();
                mPassengerTempBarExpanded.setTemperature(temp);
                mPassengerTempBarCollapsed.setTemperature(temp);
            }
        }

        @Override
        public void onDriverTemperatureChange(CarPropertyValue value) {
            final boolean available = value.getStatus() == CarPropertyValue.STATUS_AVAILABLE;
            mDriverTempBarExpanded.setAvailable(available);
            mDriverTempBarExpanded.setAvailable(available);
            if (available) {
                final int temp = ((Float)value.getValue()).intValue();
                mDriverTempBarCollapsed.setTemperature(temp);
                mDriverTempBarExpanded.setTemperature(temp);
            }
        }
    };

    private final TemperatureBarOverlay.TemperatureAdjustClickListener mPassengerTempClickListener =
            new TemperatureBarOverlay.TemperatureAdjustClickListener() {
                @Override
                public void onTemperatureChanged(int temperature) {
                    mHvacController.setPassengerTemperature(temperature);
                    mPassengerTempBarCollapsed.setTemperature(temperature);
                }
            };

    private final TemperatureBarOverlay.TemperatureAdjustClickListener mDriverTempClickListener =
            new TemperatureBarOverlay.TemperatureAdjustClickListener() {
                @Override
                public void onTemperatureChanged(int temperature) {
                    mHvacController.setDriverTemperature(temperature);
                    mDriverTempBarCollapsed.setTemperature(temperature);
                }
            };
}
