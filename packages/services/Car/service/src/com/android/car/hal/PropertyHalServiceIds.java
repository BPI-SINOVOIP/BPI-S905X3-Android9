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

import android.annotation.Nullable;
import android.car.Car;
import android.hardware.automotive.vehicle.V2_0.VehicleProperty;
import android.hardware.automotive.vehicle.V2_0.VehiclePropertyGroup;
import android.util.Pair;
import android.util.SparseArray;

/**
 * Helper class to define which property IDs are used by PropertyHalService.  This class binds the
 * read and write permissions to the property ID.
 */
public class PropertyHalServiceIds {
    // Index (key is propertyId, and the value is readPermission, writePermission
    private final SparseArray<Pair<String, String>> mProps;

    public PropertyHalServiceIds() {
        mProps = new SparseArray<>();

        // Add propertyId and read/write permissions
        // Cabin Properties
        mProps.put(VehicleProperty.DOOR_POS, new Pair<>(
                Car.PERMISSION_CONTROL_CAR_DOORS,
                Car.PERMISSION_CONTROL_CAR_DOORS));
        mProps.put(VehicleProperty.DOOR_MOVE, new Pair<>(
                Car.PERMISSION_CONTROL_CAR_DOORS,
                Car.PERMISSION_CONTROL_CAR_DOORS));
        mProps.put(VehicleProperty.DOOR_LOCK, new Pair<>(
                Car.PERMISSION_CONTROL_CAR_DOORS,
                Car.PERMISSION_CONTROL_CAR_DOORS));
        mProps.put(VehicleProperty.MIRROR_Z_POS, new Pair<>(
                Car.PERMISSION_CONTROL_CAR_MIRRORS,
                Car.PERMISSION_CONTROL_CAR_MIRRORS));
        mProps.put(VehicleProperty.MIRROR_Z_MOVE, new Pair<>(
                Car.PERMISSION_CONTROL_CAR_MIRRORS,
                Car.PERMISSION_CONTROL_CAR_MIRRORS));
        mProps.put(VehicleProperty.MIRROR_Y_POS, new Pair<>(
                Car.PERMISSION_CONTROL_CAR_MIRRORS,
                Car.PERMISSION_CONTROL_CAR_MIRRORS));
        mProps.put(VehicleProperty.MIRROR_Y_MOVE, new Pair<>(
                Car.PERMISSION_CONTROL_CAR_MIRRORS,
                Car.PERMISSION_CONTROL_CAR_MIRRORS));
        mProps.put(VehicleProperty.MIRROR_LOCK, new Pair<>(
                Car.PERMISSION_CONTROL_CAR_MIRRORS,
                Car.PERMISSION_CONTROL_CAR_MIRRORS));
        mProps.put(VehicleProperty.MIRROR_FOLD, new Pair<>(
                Car.PERMISSION_CONTROL_CAR_MIRRORS,
                Car.PERMISSION_CONTROL_CAR_MIRRORS));
        mProps.put(VehicleProperty.SEAT_MEMORY_SELECT, new Pair<>(
                Car.PERMISSION_CONTROL_CAR_SEATS,
                Car.PERMISSION_CONTROL_CAR_SEATS));
        mProps.put(VehicleProperty.SEAT_MEMORY_SET, new Pair<>(
                Car.PERMISSION_CONTROL_CAR_SEATS,
                Car.PERMISSION_CONTROL_CAR_SEATS));
        mProps.put(VehicleProperty.SEAT_BELT_BUCKLED, new Pair<>(
                Car.PERMISSION_CONTROL_CAR_SEATS,
                Car.PERMISSION_CONTROL_CAR_SEATS));
        mProps.put(VehicleProperty.SEAT_BELT_HEIGHT_POS, new Pair<>(
                Car.PERMISSION_CONTROL_CAR_SEATS,
                Car.PERMISSION_CONTROL_CAR_SEATS));
        mProps.put(VehicleProperty.SEAT_BELT_HEIGHT_MOVE, new Pair<>(
                Car.PERMISSION_CONTROL_CAR_SEATS,
                Car.PERMISSION_CONTROL_CAR_SEATS));
        mProps.put(VehicleProperty.SEAT_FORE_AFT_POS, new Pair<>(
                Car.PERMISSION_CONTROL_CAR_SEATS,
                Car.PERMISSION_CONTROL_CAR_SEATS));
        mProps.put(VehicleProperty.SEAT_FORE_AFT_MOVE, new Pair<>(
                Car.PERMISSION_CONTROL_CAR_SEATS,
                Car.PERMISSION_CONTROL_CAR_SEATS));
        mProps.put(VehicleProperty.SEAT_BACKREST_ANGLE_1_POS, new Pair<>(
                Car.PERMISSION_CONTROL_CAR_SEATS,
                Car.PERMISSION_CONTROL_CAR_SEATS));
        mProps.put(VehicleProperty.SEAT_BACKREST_ANGLE_1_MOVE, new Pair<>(
                Car.PERMISSION_CONTROL_CAR_SEATS,
                Car.PERMISSION_CONTROL_CAR_SEATS));
        mProps.put(VehicleProperty.SEAT_BACKREST_ANGLE_2_POS, new Pair<>(
                Car.PERMISSION_CONTROL_CAR_SEATS,
                Car.PERMISSION_CONTROL_CAR_SEATS));
        mProps.put(VehicleProperty.SEAT_BACKREST_ANGLE_2_MOVE, new Pair<>(
                Car.PERMISSION_CONTROL_CAR_SEATS,
                Car.PERMISSION_CONTROL_CAR_SEATS));
        mProps.put(VehicleProperty.SEAT_HEIGHT_POS, new Pair<>(
                Car.PERMISSION_CONTROL_CAR_SEATS,
                Car.PERMISSION_CONTROL_CAR_SEATS));
        mProps.put(VehicleProperty.SEAT_HEIGHT_MOVE, new Pair<>(
                Car.PERMISSION_CONTROL_CAR_SEATS,
                Car.PERMISSION_CONTROL_CAR_SEATS));
        mProps.put(VehicleProperty.SEAT_DEPTH_POS, new Pair<>(
                Car.PERMISSION_CONTROL_CAR_SEATS,
                Car.PERMISSION_CONTROL_CAR_SEATS));
        mProps.put(VehicleProperty.SEAT_DEPTH_MOVE, new Pair<>(
                Car.PERMISSION_CONTROL_CAR_SEATS,
                Car.PERMISSION_CONTROL_CAR_SEATS));
        mProps.put(VehicleProperty.SEAT_TILT_POS, new Pair<>(
                Car.PERMISSION_CONTROL_CAR_SEATS,
                Car.PERMISSION_CONTROL_CAR_SEATS));
        mProps.put(VehicleProperty.SEAT_TILT_MOVE, new Pair<>(
                Car.PERMISSION_CONTROL_CAR_SEATS,
                Car.PERMISSION_CONTROL_CAR_SEATS));
        mProps.put(VehicleProperty.SEAT_LUMBAR_FORE_AFT_POS, new Pair<>(
                Car.PERMISSION_CONTROL_CAR_SEATS,
                Car.PERMISSION_CONTROL_CAR_SEATS));
        mProps.put(VehicleProperty.SEAT_LUMBAR_FORE_AFT_MOVE, new Pair<>(
                Car.PERMISSION_CONTROL_CAR_SEATS,
                Car.PERMISSION_CONTROL_CAR_SEATS));
        mProps.put(VehicleProperty.SEAT_LUMBAR_SIDE_SUPPORT_POS, new Pair<>(
                Car.PERMISSION_CONTROL_CAR_SEATS,
                Car.PERMISSION_CONTROL_CAR_SEATS));
        mProps.put(VehicleProperty.SEAT_LUMBAR_SIDE_SUPPORT_MOVE, new Pair<>(
                Car.PERMISSION_CONTROL_CAR_SEATS,
                Car.PERMISSION_CONTROL_CAR_SEATS));
        mProps.put(VehicleProperty.SEAT_HEADREST_HEIGHT_POS, new Pair<>(
                Car.PERMISSION_CONTROL_CAR_SEATS,
                Car.PERMISSION_CONTROL_CAR_SEATS));
        mProps.put(VehicleProperty.SEAT_HEADREST_HEIGHT_MOVE, new Pair<>(
                Car.PERMISSION_CONTROL_CAR_SEATS,
                Car.PERMISSION_CONTROL_CAR_SEATS));
        mProps.put(VehicleProperty.SEAT_HEADREST_ANGLE_POS, new Pair<>(
                Car.PERMISSION_CONTROL_CAR_SEATS,
                Car.PERMISSION_CONTROL_CAR_SEATS));
        mProps.put(VehicleProperty.SEAT_HEADREST_ANGLE_MOVE, new Pair<>(
                Car.PERMISSION_CONTROL_CAR_SEATS,
                Car.PERMISSION_CONTROL_CAR_SEATS));
        mProps.put(VehicleProperty.SEAT_HEADREST_FORE_AFT_POS, new Pair<>(
                Car.PERMISSION_CONTROL_CAR_SEATS,
                Car.PERMISSION_CONTROL_CAR_SEATS));
        mProps.put(VehicleProperty.SEAT_HEADREST_FORE_AFT_MOVE, new Pair<>(
                Car.PERMISSION_CONTROL_CAR_SEATS,
                Car.PERMISSION_CONTROL_CAR_SEATS));
        mProps.put(VehicleProperty.WINDOW_POS, new Pair<>(
                Car.PERMISSION_CONTROL_CAR_WINDOWS,
                Car.PERMISSION_CONTROL_CAR_WINDOWS));
        mProps.put(VehicleProperty.WINDOW_MOVE, new Pair<>(
                Car.PERMISSION_CONTROL_CAR_WINDOWS,
                Car.PERMISSION_CONTROL_CAR_WINDOWS));
        mProps.put(VehicleProperty.WINDOW_LOCK, new Pair<>(
                Car.PERMISSION_CONTROL_CAR_WINDOWS,
                Car.PERMISSION_CONTROL_CAR_WINDOWS));

        // HVAC properties
        mProps.put(VehicleProperty.HVAC_FAN_SPEED, new Pair<>(
                    Car.PERMISSION_CONTROL_CAR_CLIMATE,
                    Car.PERMISSION_CONTROL_CAR_CLIMATE));
        mProps.put(VehicleProperty.HVAC_FAN_DIRECTION, new Pair<>(
                    Car.PERMISSION_CONTROL_CAR_CLIMATE,
                    Car.PERMISSION_CONTROL_CAR_CLIMATE));
        mProps.put(VehicleProperty.HVAC_TEMPERATURE_CURRENT, new Pair<>(
                    Car.PERMISSION_CONTROL_CAR_CLIMATE,
                    Car.PERMISSION_CONTROL_CAR_CLIMATE));
        mProps.put(VehicleProperty.HVAC_TEMPERATURE_SET, new Pair<>(
                    Car.PERMISSION_CONTROL_CAR_CLIMATE,
                    Car.PERMISSION_CONTROL_CAR_CLIMATE));
        mProps.put(VehicleProperty.HVAC_DEFROSTER, new Pair<>(
                    Car.PERMISSION_CONTROL_CAR_CLIMATE,
                    Car.PERMISSION_CONTROL_CAR_CLIMATE));
        mProps.put(VehicleProperty.HVAC_AC_ON, new Pair<>(
                    Car.PERMISSION_CONTROL_CAR_CLIMATE,
                    Car.PERMISSION_CONTROL_CAR_CLIMATE));
        mProps.put(VehicleProperty.HVAC_MAX_AC_ON, new Pair<>(
                    Car.PERMISSION_CONTROL_CAR_CLIMATE,
                    Car.PERMISSION_CONTROL_CAR_CLIMATE));
        mProps.put(VehicleProperty.HVAC_MAX_DEFROST_ON, new Pair<>(
                    Car.PERMISSION_CONTROL_CAR_CLIMATE,
                    Car.PERMISSION_CONTROL_CAR_CLIMATE));
        mProps.put(VehicleProperty.HVAC_RECIRC_ON, new Pair<>(
                    Car.PERMISSION_CONTROL_CAR_CLIMATE,
                    Car.PERMISSION_CONTROL_CAR_CLIMATE));
        mProps.put(VehicleProperty.HVAC_DUAL_ON, new Pair<>(
                    Car.PERMISSION_CONTROL_CAR_CLIMATE,
                    Car.PERMISSION_CONTROL_CAR_CLIMATE));
        mProps.put(VehicleProperty.HVAC_AUTO_ON, new Pair<>(
                    Car.PERMISSION_CONTROL_CAR_CLIMATE,
                    Car.PERMISSION_CONTROL_CAR_CLIMATE));
        mProps.put(VehicleProperty.HVAC_SEAT_TEMPERATURE, new Pair<>(
                    Car.PERMISSION_CONTROL_CAR_CLIMATE,
                    Car.PERMISSION_CONTROL_CAR_CLIMATE));
        mProps.put(VehicleProperty.HVAC_SIDE_MIRROR_HEAT, new Pair<>(
                    Car.PERMISSION_CONTROL_CAR_CLIMATE,
                    Car.PERMISSION_CONTROL_CAR_CLIMATE));
        mProps.put(VehicleProperty.HVAC_STEERING_WHEEL_HEAT, new Pair<>(
                    Car.PERMISSION_CONTROL_CAR_CLIMATE,
                    Car.PERMISSION_CONTROL_CAR_CLIMATE));
        mProps.put(VehicleProperty.HVAC_TEMPERATURE_DISPLAY_UNITS, new Pair<>(
                    Car.PERMISSION_CONTROL_CAR_CLIMATE,
                    Car.PERMISSION_CONTROL_CAR_CLIMATE));
        mProps.put(VehicleProperty.HVAC_ACTUAL_FAN_SPEED_RPM, new Pair<>(
                    Car.PERMISSION_CONTROL_CAR_CLIMATE,
                    Car.PERMISSION_CONTROL_CAR_CLIMATE));
        mProps.put(VehicleProperty.HVAC_POWER_ON, new Pair<>(
                    Car.PERMISSION_CONTROL_CAR_CLIMATE,
                    Car.PERMISSION_CONTROL_CAR_CLIMATE));
        mProps.put(VehicleProperty.HVAC_FAN_DIRECTION_AVAILABLE, new Pair<>(
                    Car.PERMISSION_CONTROL_CAR_CLIMATE,
                    Car.PERMISSION_CONTROL_CAR_CLIMATE));
        mProps.put(VehicleProperty.HVAC_AUTO_RECIRC_ON, new Pair<>(
                    Car.PERMISSION_CONTROL_CAR_CLIMATE,
                    Car.PERMISSION_CONTROL_CAR_CLIMATE));
        mProps.put(VehicleProperty.HVAC_SEAT_VENTILATION, new Pair<>(
                    Car.PERMISSION_CONTROL_CAR_CLIMATE,
                    Car.PERMISSION_CONTROL_CAR_CLIMATE));
        mProps.put(VehicleProperty.ENV_OUTSIDE_TEMPERATURE, new Pair<>(
                    Car.PERMISSION_CONTROL_CAR_CLIMATE,
                    Car.PERMISSION_CONTROL_CAR_CLIMATE));

        // Info properties
        mProps.put(VehicleProperty.INFO_VIN, new Pair<>(
                    Car.PERMISSION_IDENTIFICATION,
                    Car.PERMISSION_IDENTIFICATION));
        mProps.put(VehicleProperty.INFO_MAKE, new Pair<>(
                    Car.PERMISSION_CAR_INFO,
                    Car.PERMISSION_CAR_INFO));
        mProps.put(VehicleProperty.INFO_MODEL, new Pair<>(
                    Car.PERMISSION_CAR_INFO,
                    Car.PERMISSION_CAR_INFO));
        mProps.put(VehicleProperty.INFO_MODEL_YEAR, new Pair<>(
                    Car.PERMISSION_CAR_INFO,
                    Car.PERMISSION_CAR_INFO));
        mProps.put(VehicleProperty.INFO_FUEL_CAPACITY, new Pair<>(
                    Car.PERMISSION_CAR_INFO,
                    Car.PERMISSION_CAR_INFO));
        mProps.put(VehicleProperty.INFO_FUEL_TYPE, new Pair<>(
                    Car.PERMISSION_CAR_INFO,
                    Car.PERMISSION_CAR_INFO));
        mProps.put(VehicleProperty.INFO_EV_BATTERY_CAPACITY, new Pair<>(
                    Car.PERMISSION_CAR_INFO,
                    Car.PERMISSION_CAR_INFO));
        mProps.put(VehicleProperty.INFO_EV_CONNECTOR_TYPE, new Pair<>(
                    Car.PERMISSION_CAR_INFO,
                    Car.PERMISSION_CAR_INFO));
        mProps.put(VehicleProperty.INFO_FUEL_DOOR_LOCATION, new Pair<>(
                    Car.PERMISSION_CAR_INFO,
                    Car.PERMISSION_CAR_INFO));
        mProps.put(VehicleProperty.INFO_EV_PORT_LOCATION, new Pair<>(
                    Car.PERMISSION_CAR_INFO,
                    Car.PERMISSION_CAR_INFO));
        mProps.put(VehicleProperty.INFO_DRIVER_SEAT, new Pair<>(
                    Car.PERMISSION_CAR_INFO,
                    Car.PERMISSION_CAR_INFO));

        // Sensor properties
        mProps.put(VehicleProperty.PERF_ODOMETER, new Pair<>(
                Car.PERMISSION_MILEAGE,
                Car.PERMISSION_MILEAGE));
        mProps.put(VehicleProperty.PERF_VEHICLE_SPEED, new Pair<>(
                Car.PERMISSION_SPEED,
                Car.PERMISSION_SPEED));
        mProps.put(VehicleProperty.ENGINE_COOLANT_TEMP, new Pair<>(
                Car.PERMISSION_CAR_ENGINE_DETAILED,
                Car.PERMISSION_CAR_ENGINE_DETAILED));
        mProps.put(VehicleProperty.ENGINE_OIL_LEVEL, new Pair<>(
                Car.PERMISSION_CAR_ENGINE_DETAILED,
                Car.PERMISSION_CAR_ENGINE_DETAILED));
        mProps.put(VehicleProperty.ENGINE_OIL_TEMP, new Pair<>(
                Car.PERMISSION_CAR_ENGINE_DETAILED,
                Car.PERMISSION_CAR_ENGINE_DETAILED));
        mProps.put(VehicleProperty.ENGINE_RPM, new Pair<>(
                Car.PERMISSION_CAR_ENGINE_DETAILED,
                Car.PERMISSION_CAR_ENGINE_DETAILED));
        mProps.put(VehicleProperty.WHEEL_TICK, new Pair<>(
                Car.PERMISSION_SPEED,
                Car.PERMISSION_SPEED));
        mProps.put(VehicleProperty.FUEL_LEVEL, new Pair<>(
                Car.PERMISSION_ENERGY,
                Car.PERMISSION_ENERGY));
        mProps.put(VehicleProperty.FUEL_DOOR_OPEN, new Pair<>(
                Car.PERMISSION_ENERGY_PORTS,
                Car.PERMISSION_ENERGY_PORTS));
        mProps.put(VehicleProperty.EV_BATTERY_LEVEL, new Pair<>(
                Car.PERMISSION_ENERGY,
                Car.PERMISSION_ENERGY));
        mProps.put(VehicleProperty.EV_CHARGE_PORT_OPEN, new Pair<>(
                Car.PERMISSION_ENERGY_PORTS,
                Car.PERMISSION_ENERGY_PORTS));
        mProps.put(VehicleProperty.EV_CHARGE_PORT_CONNECTED, new Pair<>(
                Car.PERMISSION_ENERGY_PORTS,
                Car.PERMISSION_ENERGY_PORTS));
        mProps.put(VehicleProperty.EV_BATTERY_INSTANTANEOUS_CHARGE_RATE, new Pair<>(
                Car.PERMISSION_ENERGY,
                Car.PERMISSION_ENERGY));
        mProps.put(VehicleProperty.RANGE_REMAINING, new Pair<>(
                Car.PERMISSION_ENERGY,
                Car.PERMISSION_ENERGY));
        mProps.put(VehicleProperty.TIRE_PRESSURE, new Pair<>(
                Car.PERMISSION_TIRES,
                Car.PERMISSION_TIRES));
        mProps.put(VehicleProperty.GEAR_SELECTION, new Pair<>(
                Car.PERMISSION_POWERTRAIN,
                Car.PERMISSION_POWERTRAIN));
        mProps.put(VehicleProperty.CURRENT_GEAR, new Pair<>(
                Car.PERMISSION_POWERTRAIN,
                Car.PERMISSION_POWERTRAIN));
        mProps.put(VehicleProperty.PARKING_BRAKE_ON, new Pair<>(
                Car.PERMISSION_POWERTRAIN,
                Car.PERMISSION_POWERTRAIN));
        mProps.put(VehicleProperty.PARKING_BRAKE_AUTO_APPLY, new Pair<>(
                Car.PERMISSION_POWERTRAIN,
                Car.PERMISSION_POWERTRAIN));
        mProps.put(VehicleProperty.FUEL_LEVEL_LOW, new Pair<>(
                Car.PERMISSION_ENERGY,
                Car.PERMISSION_ENERGY));
        mProps.put(VehicleProperty.NIGHT_MODE, new Pair<>(
                Car.PERMISSION_EXTERIOR_ENVIRONMENT,
                Car.PERMISSION_EXTERIOR_ENVIRONMENT));
        mProps.put(VehicleProperty.TURN_SIGNAL_STATE, new Pair<>(
                Car.PERMISSION_EXTERIOR_LIGHTS,
                Car.PERMISSION_EXTERIOR_LIGHTS));
        mProps.put(VehicleProperty.IGNITION_STATE, new Pair<>(
                Car.PERMISSION_CAR_POWER,
                Car.PERMISSION_CAR_POWER));
        mProps.put(VehicleProperty.ABS_ACTIVE, new Pair<>(
                Car.PERMISSION_CAR_DYNAMICS_STATE,
                Car.PERMISSION_CAR_DYNAMICS_STATE));
        mProps.put(VehicleProperty.TRACTION_CONTROL_ACTIVE, new Pair<>(
                Car.PERMISSION_CAR_DYNAMICS_STATE,
                Car.PERMISSION_CAR_DYNAMICS_STATE));
        mProps.put(VehicleProperty.ENV_OUTSIDE_TEMPERATURE, new Pair<>(
                Car.PERMISSION_EXTERIOR_ENVIRONMENT,
                Car.PERMISSION_EXTERIOR_ENVIRONMENT));
        mProps.put(VehicleProperty.HEADLIGHTS_STATE, new Pair<>(
                Car.PERMISSION_EXTERIOR_LIGHTS,
                Car.PERMISSION_EXTERIOR_LIGHTS));
        mProps.put(VehicleProperty.HIGH_BEAM_LIGHTS_STATE, new Pair<>(
                Car.PERMISSION_EXTERIOR_LIGHTS,
                Car.PERMISSION_EXTERIOR_LIGHTS));
        mProps.put(VehicleProperty.FOG_LIGHTS_STATE, new Pair<>(
                Car.PERMISSION_EXTERIOR_LIGHTS,
                Car.PERMISSION_EXTERIOR_LIGHTS));
        mProps.put(VehicleProperty.HAZARD_LIGHTS_STATE, new Pair<>(
                Car.PERMISSION_EXTERIOR_LIGHTS,
                Car.PERMISSION_EXTERIOR_LIGHTS));
        mProps.put(VehicleProperty.HEADLIGHTS_SWITCH, new Pair<>(
                Car.PERMISSION_EXTERIOR_LIGHTS,
                Car.PERMISSION_CONTROL_EXTERIOR_LIGHTS));
        mProps.put(VehicleProperty.HIGH_BEAM_LIGHTS_SWITCH, new Pair<>(
                Car.PERMISSION_EXTERIOR_LIGHTS,
                Car.PERMISSION_CONTROL_EXTERIOR_LIGHTS));
        mProps.put(VehicleProperty.FOG_LIGHTS_SWITCH, new Pair<>(
                Car.PERMISSION_EXTERIOR_LIGHTS,
                Car.PERMISSION_CONTROL_EXTERIOR_LIGHTS));
        mProps.put(VehicleProperty.HAZARD_LIGHTS_SWITCH, new Pair<>(
                Car.PERMISSION_EXTERIOR_LIGHTS,
                Car.PERMISSION_CONTROL_EXTERIOR_LIGHTS));
    }

    /**
     * Returns read permission string for given property ID.
     */
    @Nullable
    public String getReadPermission(int propId) {
        Pair<String, String> p = mProps.get(propId);
        if (p != null) {
            // Property ID exists.  Return read permission.
            return p.first;
        } else {
            return null;
        }
    }

    /**
     * Returns write permission string for given property ID.
     */
    @Nullable
    public String getWritePermission(int propId) {
        Pair<String, String> p = mProps.get(propId);
        if (p != null) {
            // Property ID exists.  Return write permission.
            return p.second;
        } else {
            return null;
        }
    }

    /**
     * Return true if property is a vendor property and was added
     */
    public boolean insertVendorProperty(int propId) {
        if ((propId & VehiclePropertyGroup.MASK) == VehiclePropertyGroup.VENDOR) {
            mProps.put(propId, new Pair<>(
                    Car.PERMISSION_VENDOR_EXTENSION, Car.PERMISSION_VENDOR_EXTENSION));
            return true;
        } else {
            // This is not a vendor extension property, it is not added
            return false;
        }
    }

    /**
     * Check if property ID is in the list of known IDs that PropertyHalService is interested it.
     */
    public boolean isSupportedProperty(int propId) {
        if (mProps.get(propId) != null) {
            // Property is in the list of supported properties
            return true;
        } else {
            // If it's a vendor property, insert it into the propId list and handle it
            return insertVendorProperty(propId);
        }
    }
}
