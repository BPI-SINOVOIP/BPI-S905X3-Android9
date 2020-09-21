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
package com.android.car.vehiclehal.test;

import android.car.hardware.hvac.CarHvacManager;
import android.hardware.automotive.vehicle.V2_0.VehicleProperty;
import android.util.SparseIntArray;

class VhalPropMaps {

    private static final SparseIntArray HVAC_PROP_MAP;

    static {
        HVAC_PROP_MAP = new SparseIntArray();
        HVAC_PROP_MAP.put(CarHvacManager.ID_MIRROR_DEFROSTER_ON,
                          VehicleProperty.HVAC_SIDE_MIRROR_HEAT);
        HVAC_PROP_MAP.put(CarHvacManager.ID_STEERING_WHEEL_HEAT,
                          VehicleProperty.HVAC_STEERING_WHEEL_HEAT);
        HVAC_PROP_MAP.put(CarHvacManager.ID_OUTSIDE_AIR_TEMP,
                          VehicleProperty.ENV_OUTSIDE_TEMPERATURE);
        HVAC_PROP_MAP.put(CarHvacManager.ID_TEMPERATURE_DISPLAY_UNITS,
                          VehicleProperty.HVAC_TEMPERATURE_DISPLAY_UNITS);
        HVAC_PROP_MAP.put(CarHvacManager.ID_ZONED_TEMP_SETPOINT,
                          VehicleProperty.HVAC_TEMPERATURE_SET);
        HVAC_PROP_MAP.put(CarHvacManager.ID_ZONED_TEMP_ACTUAL,
                          VehicleProperty.HVAC_TEMPERATURE_CURRENT);
        HVAC_PROP_MAP.put(CarHvacManager.ID_ZONED_FAN_SPEED_SETPOINT,
                          VehicleProperty.HVAC_FAN_SPEED);
        HVAC_PROP_MAP.put(CarHvacManager.ID_ZONED_FAN_SPEED_RPM,
                          VehicleProperty.HVAC_ACTUAL_FAN_SPEED_RPM);
        HVAC_PROP_MAP.put(CarHvacManager.ID_ZONED_FAN_DIRECTION_AVAILABLE,
                          VehicleProperty.HVAC_FAN_DIRECTION_AVAILABLE);
        HVAC_PROP_MAP.put(CarHvacManager.ID_ZONED_FAN_DIRECTION,
                          VehicleProperty.HVAC_FAN_DIRECTION);
        HVAC_PROP_MAP.put(CarHvacManager.ID_ZONED_SEAT_TEMP,
                          VehicleProperty.HVAC_SEAT_TEMPERATURE);
        HVAC_PROP_MAP.put(CarHvacManager.ID_ZONED_AC_ON,
                          VehicleProperty.HVAC_AC_ON);
        HVAC_PROP_MAP.put(CarHvacManager.ID_ZONED_AUTOMATIC_MODE_ON,
                          VehicleProperty.HVAC_AUTO_ON);
        HVAC_PROP_MAP.put(CarHvacManager.ID_ZONED_AIR_RECIRCULATION_ON,
                          VehicleProperty.HVAC_RECIRC_ON);
        HVAC_PROP_MAP.put(CarHvacManager.ID_ZONED_MAX_AC_ON,
                          VehicleProperty.HVAC_MAX_AC_ON);
        HVAC_PROP_MAP.put(CarHvacManager.ID_ZONED_DUAL_ZONE_ON,
                          VehicleProperty.HVAC_DUAL_ON);
        HVAC_PROP_MAP.put(CarHvacManager.ID_ZONED_MAX_DEFROST_ON,
                          VehicleProperty.HVAC_MAX_DEFROST_ON);
        HVAC_PROP_MAP.put(CarHvacManager.ID_ZONED_HVAC_POWER_ON,
                          VehicleProperty.HVAC_POWER_ON);
        HVAC_PROP_MAP.put(CarHvacManager.ID_ZONED_HVAC_AUTO_RECIRC_ON,
                          VehicleProperty.HVAC_AUTO_RECIRC_ON);
        HVAC_PROP_MAP.put(CarHvacManager.ID_WINDOW_DEFROSTER_ON,
                          VehicleProperty.HVAC_DEFROSTER);
    }

    static int getHvacVhalProp(final int hvacProp) {
        return HVAC_PROP_MAP.get(hvacProp);
    }
}
