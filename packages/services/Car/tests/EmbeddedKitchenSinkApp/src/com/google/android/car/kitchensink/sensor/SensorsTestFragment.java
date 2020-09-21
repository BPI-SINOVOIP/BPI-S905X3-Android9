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

package com.google.android.car.kitchensink.sensor;

import static java.lang.Integer.toHexString;

import android.Manifest;
import android.annotation.Nullable;
import android.car.Car;
import android.car.CarNotConnectedException;
import android.car.hardware.CarSensorConfig;
import android.car.hardware.CarSensorEvent;
import android.car.hardware.CarSensorManager;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.Handler;
import android.support.v4.app.Fragment;
import android.text.TextUtils;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.google.android.car.kitchensink.KitchenSinkActivity;
import com.google.android.car.kitchensink.R;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;

public class SensorsTestFragment extends Fragment {
    private static final String TAG = "CAR.SENSOR.KS";
    private static final boolean DBG = true;
    private static final boolean DBG_VERBOSE = true;
    private static final int KS_PERMISSIONS_REQUEST = 1;

    private final static String[] REQUIRED_PERMISSIONS = new String[]{
        Manifest.permission.ACCESS_FINE_LOCATION,
        Manifest.permission.ACCESS_COARSE_LOCATION,
        Car.PERMISSION_MILEAGE,
        Car.PERMISSION_ENERGY,
        Car.PERMISSION_SPEED,
        Car.PERMISSION_CAR_DYNAMICS_STATE
    };

    private final CarSensorManager.OnSensorChangedListener mOnSensorChangedListener =
            new CarSensorManager.OnSensorChangedListener() {
                @Override
                public void onSensorChanged(CarSensorEvent event) {
                    if (DBG_VERBOSE) {
                        Log.v(TAG, "New car sensor event: " + event);
                    }
                    synchronized (SensorsTestFragment.this) {
                        mEventMap.put(event.sensorType, event);
                    }
                    refreshUi();
                }
            };
    private final Handler mHandler = new Handler();
    private final Map<Integer, CarSensorEvent> mEventMap = new ConcurrentHashMap<>();
    private final DateFormat mDateFormat = SimpleDateFormat.getDateTimeInstance();

    private KitchenSinkActivity mActivity;
    private TextView mSensorInfo;
    private Car mCar;
    private CarSensorManager mSensorManager;
    private String mNaString;
    private int[] supportedSensors = new int[0];
    private Set<String> mActivePermissions = new HashSet<String>();

    @Nullable
    @Override
    public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container,
                             @Nullable Bundle savedInstanceState) {
        if (DBG) {
            Log.i(TAG, "onCreateView");
        }

        View view = inflater.inflate(R.layout.sensors, container, false);
        mActivity = (KitchenSinkActivity) getHost();

        mSensorInfo = (TextView) view.findViewById(R.id.sensor_info);
        mNaString = getContext().getString(R.string.sensor_na);

        return view;
    }

    @Override
    public void onResume() {
        super.onResume();
        initPermissions();
    }

    @Override
    public void onPause() {
        super.onPause();
        if (mSensorManager != null) {
            mSensorManager.unregisterListener(mOnSensorChangedListener);
        }
    }

    private void initSensors() {
        try {
            mSensorManager =
                (CarSensorManager) ((KitchenSinkActivity) getActivity()).getSensorManager();
            supportedSensors = mSensorManager.getSupportedSensors();
            for (Integer sensor : supportedSensors) {
                mSensorManager.registerListener(mOnSensorChangedListener, sensor,
                        CarSensorManager.SENSOR_RATE_NORMAL);
            }
        } catch (CarNotConnectedException e) {
            Log.e(TAG, "Car not connected or not supported", e);
        } catch (Exception e) {
            Log.e(TAG, "initSensors() exception caught: ", e);
        }
    }

    private void initPermissions() {
        Set<String> missingPermissions = checkExistingPermissions();
        if (!missingPermissions.isEmpty()) {
            requestPermissions(missingPermissions);
        } else {
            initSensors();
        }
    }

    private Set<String> checkExistingPermissions() {
        Set<String> missingPermissions = new HashSet<String>();
        for (String permission : REQUIRED_PERMISSIONS) {
            if (mActivity.checkSelfPermission(permission)
                == PackageManager.PERMISSION_GRANTED) {
                mActivePermissions.add(permission);
            } else {
                missingPermissions.add(permission);
            }
        }
        return missingPermissions;
    }

    private void requestPermissions(Set<String> permissions) {
        Log.d(TAG, "requesting additional permissions=" + permissions);

        requestPermissions(permissions.toArray(new String[permissions.size()]),
                KS_PERMISSIONS_REQUEST);
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions,
            int[] grantResults) {
        Log.d(TAG, "onRequestPermissionsResult reqCode=" + requestCode);
        if (KS_PERMISSIONS_REQUEST == requestCode) {
            for (int i=0; i<permissions.length; i++) {
                if (grantResults[i] == PackageManager.PERMISSION_GRANTED) {
                    mActivePermissions.add(permissions[i]);
                }
            }
            initSensors();
        }
    }

    private void refreshUi() {
        String summaryString;
        synchronized (this) {
            List<String> summary = new ArrayList<>();
            for (Integer i : supportedSensors) {
                CarSensorEvent event = mEventMap.get(i);
                switch (i) {
                    case CarSensorManager.SENSOR_TYPE_CAR_SPEED:
                        summary.add(getContext().getString(R.string.sensor_speed,
                                getTimestamp(event),
                                event == null ? mNaString : event.getCarSpeedData(null).carSpeed));
                        break;
                    case CarSensorManager.SENSOR_TYPE_RPM:
                        summary.add(getContext().getString(R.string.sensor_rpm,
                                getTimestamp(event),
                                event == null ? mNaString : event.getRpmData(null).rpm));
                        break;
                    case CarSensorManager.SENSOR_TYPE_ODOMETER:
                        summary.add(getContext().getString(R.string.sensor_odometer,
                                getTimestamp(event),
                                event == null ? mNaString : event.getOdometerData(null).kms));
                        break;
                    case CarSensorManager.SENSOR_TYPE_FUEL_LEVEL:
                        summary.add(getFuelLevel(event));
                        break;
                    case CarSensorManager.SENSOR_TYPE_FUEL_DOOR_OPEN:
                        summary.add(getFuelDoorOpen(event));
                        break;
                    case CarSensorManager.SENSOR_TYPE_PARKING_BRAKE:
                        summary.add(getContext().getString(R.string.sensor_parking_brake,
                                getTimestamp(event),
                                event == null ? mNaString :
                                event.getParkingBrakeData(null).isEngaged));
                        break;
                    case CarSensorManager.SENSOR_TYPE_GEAR:
                        summary.add(getContext().getString(R.string.sensor_gear,
                                getTimestamp(event),
                                event == null ? mNaString : event.getGearData(null).gear));
                        break;
                    case CarSensorManager.SENSOR_TYPE_NIGHT:
                        summary.add(getContext().getString(R.string.sensor_night,
                                getTimestamp(event),
                                event == null ? mNaString : event.getNightData(null).isNightMode));
                        break;
                    case CarSensorManager.SENSOR_TYPE_ENVIRONMENT:
                        String temperature = mNaString;
                        String pressure = mNaString;
                        if (event != null) {
                            CarSensorEvent.EnvironmentData env = event.getEnvironmentData(null);
                            temperature = Float.isNaN(env.temperature) ? temperature :
                                    String.valueOf(env.temperature);
                            pressure = Float.isNaN(env.pressure) ? pressure :
                                    String.valueOf(env.pressure);
                        }
                        summary.add(getContext().getString(R.string.sensor_environment,
                                getTimestamp(event), temperature, pressure));
                        break;
                    case CarSensorManager.SENSOR_TYPE_WHEEL_TICK_DISTANCE:
                        if(event != null) {
                            CarSensorEvent.CarWheelTickDistanceData d =
                                    event.getCarWheelTickDistanceData(null);
                            summary.add(getContext().getString(R.string.sensor_wheel_ticks,
                                getTimestamp(event), d.sensorResetCount, d.frontLeftWheelDistanceMm,
                                d.frontRightWheelDistanceMm, d.rearLeftWheelDistanceMm,
                                d.rearRightWheelDistanceMm));
                        } else {
                            summary.add(getContext().getString(R.string.sensor_wheel_ticks,
                                getTimestamp(event), mNaString, mNaString, mNaString, mNaString,
                                mNaString));
                        }
                        // Get the config data
                        try {
                            CarSensorConfig c = mSensorManager.getSensorConfig(
                                CarSensorManager.SENSOR_TYPE_WHEEL_TICK_DISTANCE);
                            summary.add(getContext().getString(R.string.sensor_wheel_ticks_cfg,
                                c.getInt(CarSensorConfig.WHEEL_TICK_DISTANCE_SUPPORTED_WHEELS),
                                c.getInt(CarSensorConfig.WHEEL_TICK_DISTANCE_FRONT_LEFT_UM_PER_TICK),
                                c.getInt(CarSensorConfig.WHEEL_TICK_DISTANCE_FRONT_RIGHT_UM_PER_TICK),
                                c.getInt(CarSensorConfig.WHEEL_TICK_DISTANCE_REAR_LEFT_UM_PER_TICK),
                                c.getInt(CarSensorConfig.WHEEL_TICK_DISTANCE_REAR_RIGHT_UM_PER_TICK)));
                        } catch (CarNotConnectedException e) {
                            Log.e(TAG, "Car not connected or not supported", e);
                        }
                        break;
                    case CarSensorManager.SENSOR_TYPE_ABS_ACTIVE:
                        summary.add(getContext().getString(R.string.sensor_abs_is_active,
                            getTimestamp(event), event == null ? mNaString :
                                    event.getCarAbsActiveData(null).absIsActive));
                        break;

                    case CarSensorManager.SENSOR_TYPE_TRACTION_CONTROL_ACTIVE:
                        summary.add(
                            getContext().getString(R.string.sensor_traction_control_is_active,
                            getTimestamp(event), event == null ? mNaString :
                                    event.getCarTractionControlActiveData(null)
                                    .tractionControlIsActive));
                        break;
                    case CarSensorManager.SENSOR_TYPE_EV_BATTERY_LEVEL:
                        summary.add(getEvBatteryLevel(event));
                        break;
                    case CarSensorManager.SENSOR_TYPE_EV_CHARGE_PORT_OPEN:
                        summary.add(getEvChargePortOpen(event));
                        break;
                    case CarSensorManager.SENSOR_TYPE_EV_CHARGE_PORT_CONNECTED:
                        summary.add(getEvChargePortConnected(event));
                        break;
                    case CarSensorManager.SENSOR_TYPE_EV_BATTERY_CHARGE_RATE:
                        summary.add(getEvChargeRate(event));
                        break;
                    default:
                        // Should never happen.
                        Log.w(TAG, "Unrecognized event type: " + toHexString(i));
                }
            }
            summaryString = TextUtils.join("\n", summary);
        }
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                mSensorInfo.setText(summaryString);
            }
        });
    }

    private String getTimestamp(CarSensorEvent event) {
        if (event == null) {
            return mNaString;
        }
        return mDateFormat.format(new Date(event.timestamp / (1000L * 1000L)));
    }

    private String getFuelLevel(CarSensorEvent event) {
        String fuelLevel = mNaString;
        if(event != null) {
            fuelLevel = String.valueOf(event.getFuelLevelData(null).level);
        }
        return getContext().getString(R.string.sensor_fuel_level, getTimestamp(event), fuelLevel);
    }

    private String getFuelDoorOpen(CarSensorEvent event) {
        String fuelDoorOpen = mNaString;
        if(event != null) {
            fuelDoorOpen = String.valueOf(event.getCarFuelDoorOpenData(null).fuelDoorIsOpen);
        }
        return getContext().getString(R.string.sensor_fuel_door_open, getTimestamp(event),
            fuelDoorOpen);
    }

    private String getEvBatteryLevel(CarSensorEvent event) {
        String evBatteryLevel = mNaString;
        if(event != null) {
            evBatteryLevel = String.valueOf(event.getCarEvBatteryLevelData(null).evBatteryLevel);
        }
        return getContext().getString(R.string.sensor_ev_battery_level, getTimestamp(event),
            evBatteryLevel);
    }

    private String getEvChargePortOpen(CarSensorEvent event) {
        String evChargePortOpen = mNaString;
        if(event != null) {
            evChargePortOpen = String.valueOf(
                event.getCarEvChargePortOpenData(null).evChargePortIsOpen);
        }
        return getContext().getString(R.string.sensor_ev_charge_port_is_open, getTimestamp(event),
            evChargePortOpen);
    }

    private String getEvChargePortConnected(CarSensorEvent event) {
        String evChargePortConnected = mNaString;
        if(event != null) {
            evChargePortConnected = String.valueOf(
                event.getCarEvChargePortConnectedData(null).evChargePortIsConnected);
        }
        return getContext().getString(R.string.sensor_ev_charge_port_is_connected,
            getTimestamp(event), evChargePortConnected);
    }

    private String getEvChargeRate(CarSensorEvent event) {
        String evChargeRate = mNaString;
        if(event != null) {
            evChargeRate = String.valueOf(event.getCarEvBatteryChargeRateData(null).evChargeRate);
        }
        return getContext().getString(R.string.sensor_ev_charge_rate, getTimestamp(event),
            evChargeRate);
    }
}
