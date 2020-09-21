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
package com.android.car.hvac;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;

import android.car.VehicleAreaSeat;
import android.car.VehicleAreaType;
import android.car.VehicleAreaWindow;
import android.car.hardware.CarPropertyConfig;
import android.car.hardware.CarPropertyValue;
import android.car.hardware.hvac.CarHvacManager;
import android.car.hardware.property.CarPropertyEvent;
import android.car.hardware.property.ICarProperty;
import android.car.hardware.property.ICarPropertyEventListener;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Pair;

/**
 * A local {@link ICarProperty} that is used to mock up data for HVAC.
 */
public class LocalHvacPropertyService {
    private static final int DRIVER_ZONE_ID = VehicleAreaSeat.SEAT_ROW_1_LEFT |
            VehicleAreaSeat.SEAT_ROW_2_LEFT | VehicleAreaSeat.SEAT_ROW_2_CENTER;
    private static final int PASSENGER_ZONE_ID = VehicleAreaSeat.SEAT_ROW_1_RIGHT |
            VehicleAreaSeat.SEAT_ROW_2_RIGHT;

    private static final float MIN_TEMP = 16;
    private static final float MAX_TEMP = 32;

    private static final int MAX_FAN_SPEED = 7;
    private static final int MIN_FAN_SPEED = 1;

    private static final int DEFAULT_AREA_ID = 0;

    private static final boolean DEFAULT_POWER_ON = true;
    private static final boolean DEFAULT_DEFROSTER_ON = true;
    private static final boolean DEFAULT_AIR_CIRCULATION_ON = true;
    private static final boolean DEFAULT_AC_ON = true;
    private static final boolean DEFAULT_AUTO_MODE = false;
    private static final int DEFAULT_FAN_SPEED = 3;
    private static final int DEFAULT_FAN_DIRECTION = 2;
    private static final float DEFAULT_DRIVER_TEMP = 16;
    private static final float DEFAULT_PASSENGER_TEMP = 25;
    // Hardware specific value for the front seats
    public static final int SEAT_ALL = VehicleAreaSeat.SEAT_ROW_1_LEFT |
            VehicleAreaSeat.SEAT_ROW_1_RIGHT | VehicleAreaSeat.SEAT_ROW_2_LEFT |
            VehicleAreaSeat.SEAT_ROW_2_CENTER | VehicleAreaSeat.SEAT_ROW_2_RIGHT;

    private final List<CarPropertyConfig> mPropertyList;
    private final Map<Pair, Object> mProperties = new HashMap<>();
    private final List<ICarPropertyEventListener> mListeners = new ArrayList<>();

    public LocalHvacPropertyService() {
        CarPropertyConfig fanSpeedConfig = CarPropertyConfig.newBuilder(Integer.class,
                CarHvacManager.ID_ZONED_FAN_SPEED_SETPOINT,
                VehicleAreaType.VEHICLE_AREA_TYPE_SEAT)
                .addAreaConfig(DEFAULT_AREA_ID, MIN_FAN_SPEED, MAX_FAN_SPEED).build();

        CarPropertyConfig temperatureConfig = CarPropertyConfig.newBuilder(Float.class,
                CarHvacManager.ID_ZONED_TEMP_SETPOINT,
                VehicleAreaType.VEHICLE_AREA_TYPE_SEAT)
                .addAreaConfig(DEFAULT_AREA_ID, MIN_TEMP, MAX_TEMP).build();

        mPropertyList = new ArrayList<>(2);
        mPropertyList.addAll(Arrays.asList(fanSpeedConfig, temperatureConfig));
        setupDefaultValues();
    }

    private final IBinder mCarPropertyService = new ICarProperty.Stub(){
        @Override
        public void registerListener(int propId, float rate, ICarPropertyEventListener listener) throws RemoteException {
            mListeners.add(listener);
        }

        @Override
        public void unregisterListener(int propId, ICarPropertyEventListener listener) throws RemoteException {
            mListeners.remove(listener);
        }

        @Override
        public List<CarPropertyConfig> getPropertyList() throws RemoteException {
            return mPropertyList;
        }

        @Override
        public CarPropertyValue getProperty(int prop, int zone) throws RemoteException {
            return new CarPropertyValue(prop, zone, mProperties.get(new Pair(prop, zone)));
        }

        @Override
        public void setProperty(CarPropertyValue prop) throws RemoteException {
            mProperties.put(new Pair(prop.getPropertyId(), prop.getAreaId()), prop.getValue());
            for (ICarPropertyEventListener listener : mListeners) {
                LinkedList<CarPropertyEvent> l = new LinkedList<>();
                l.add(new CarPropertyEvent(CarPropertyEvent.PROPERTY_EVENT_PROPERTY_CHANGE, prop));
                listener.onEvent(l);
            }
        }
    };

    public IBinder getCarPropertyService() {
        return mCarPropertyService;
    }

    private void setupDefaultValues() {
        mProperties.put(new Pair<>(CarHvacManager.ID_ZONED_HVAC_POWER_ON,
                SEAT_ALL), DEFAULT_POWER_ON);
        mProperties.put(new Pair<>(CarHvacManager.ID_WINDOW_DEFROSTER_ON,
                VehicleAreaWindow.WINDOW_FRONT_WINDSHIELD), DEFAULT_DEFROSTER_ON);
        mProperties.put(new Pair<>(CarHvacManager.ID_WINDOW_DEFROSTER_ON,
                VehicleAreaWindow.WINDOW_REAR_WINDSHIELD), DEFAULT_DEFROSTER_ON);
        mProperties.put(new Pair<>(CarHvacManager.ID_ZONED_AIR_RECIRCULATION_ON,
                SEAT_ALL), DEFAULT_AIR_CIRCULATION_ON);
        mProperties.put(new Pair<>(CarHvacManager.ID_ZONED_AC_ON,
                SEAT_ALL), DEFAULT_AC_ON);
        mProperties.put(new Pair<>(CarHvacManager.ID_ZONED_AUTOMATIC_MODE_ON,
                SEAT_ALL), DEFAULT_AUTO_MODE);

        mProperties.put(new Pair<>(CarHvacManager.ID_ZONED_FAN_SPEED_SETPOINT,
                SEAT_ALL), DEFAULT_FAN_SPEED);
        mProperties.put(new Pair<>(CarHvacManager.ID_ZONED_FAN_DIRECTION,
                SEAT_ALL), DEFAULT_FAN_DIRECTION);

        mProperties.put(new Pair<>(CarHvacManager.ID_ZONED_TEMP_SETPOINT,
                DRIVER_ZONE_ID), DEFAULT_DRIVER_TEMP);
        mProperties.put(new Pair<>(CarHvacManager.ID_ZONED_TEMP_SETPOINT,
                PASSENGER_ZONE_ID), DEFAULT_PASSENGER_TEMP);
    }
}
