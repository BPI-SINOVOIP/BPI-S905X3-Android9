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

package com.android.car;

import android.annotation.IntDef;
import android.app.UiModeManager;
import android.car.hardware.CarPropertyValue;
import android.car.hardware.property.CarPropertyEvent;
import android.car.hardware.property.ICarPropertyEventListener;
import android.content.Context;
import android.hardware.automotive.vehicle.V2_0.VehicleProperty;
import android.os.RemoteException;
import android.util.Log;

import java.io.PrintWriter;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.List;

public class CarNightService implements CarServiceBase {

    public static final boolean DBG = false;

    @IntDef({FORCED_SENSOR_MODE, FORCED_DAY_MODE, FORCED_NIGHT_MODE})
    @Retention(RetentionPolicy.SOURCE)
    public @interface DayNightSensorMode {}

    public static final int FORCED_SENSOR_MODE = 0;
    public static final int FORCED_DAY_MODE = 1;
    public static final int FORCED_NIGHT_MODE = 2;

    private int mNightSetting = UiModeManager.MODE_NIGHT_YES;
    private int mForcedMode = FORCED_SENSOR_MODE;
    private final Context mContext;
    private final UiModeManager mUiModeManager;
    private CarPropertyService mCarPropertyService;

    private final ICarPropertyEventListener mICarPropertyEventListener =
            new ICarPropertyEventListener.Stub() {
        @Override
        public void onEvent(List<CarPropertyEvent> events) throws RemoteException {
            for (CarPropertyEvent event : events) {
                handlePropertyEvent(event);
            }
        }
    };

    /**
     * Handle CarPropertyEvents
     * @param event
     */
    public synchronized void handlePropertyEvent(CarPropertyEvent event) {
        if (event == null) {
            return;
        }
        if (event.getEventType() == CarPropertyEvent.PROPERTY_EVENT_PROPERTY_CHANGE) {
            // Only handle onChange events
            CarPropertyValue value = event.getCarPropertyValue();
            if (value.getPropertyId() == VehicleProperty.NIGHT_MODE) {
                boolean nightMode = (Boolean) value.getValue();
                if (nightMode) {
                    mNightSetting = UiModeManager.MODE_NIGHT_YES;
                    if (DBG)  Log.d(CarLog.TAG_SENSOR, "CAR dayNight handleSensorEvent NIGHT");
                } else {
                    mNightSetting = UiModeManager.MODE_NIGHT_NO;
                    if (DBG)  Log.d(CarLog.TAG_SENSOR, "CAR dayNight handleSensorEvent DAY");
                }
                if (mUiModeManager != null && (mForcedMode == FORCED_SENSOR_MODE)) {
                    mUiModeManager.setNightMode(mNightSetting);
                    if (DBG)  Log.d(CarLog.TAG_SENSOR, "CAR dayNight handleSensorEvent APPLIED");
                } else {
                    if (DBG)  Log.d(CarLog.TAG_SENSOR, "CAR dayNight handleSensorEvent IGNORED");
                }
            }
        }
    }

    public synchronized int forceDayNightMode(@DayNightSensorMode int mode) {
        if (mUiModeManager == null) {
            return -1;
        }
        int resultMode;
        switch (mode) {
            case FORCED_SENSOR_MODE:
                resultMode = mNightSetting;
                mForcedMode = FORCED_SENSOR_MODE;
                break;
            case FORCED_DAY_MODE:
                resultMode = UiModeManager.MODE_NIGHT_NO;
                mForcedMode = FORCED_DAY_MODE;
                break;
            case FORCED_NIGHT_MODE:
                resultMode = UiModeManager.MODE_NIGHT_YES;
                mForcedMode = FORCED_NIGHT_MODE;
                break;
            default:
                Log.e(CarLog.TAG_SENSOR, "Unknown forced day/night mode " + mode);
                return -1;
        }
        mUiModeManager.setNightMode(resultMode);
        return mUiModeManager.getNightMode();
    }

    CarNightService(Context context, CarPropertyService propertyService) {
        mContext = context;
        mCarPropertyService = propertyService;
        mUiModeManager = (UiModeManager) mContext.getSystemService(Context.UI_MODE_SERVICE);
        if (mUiModeManager == null) {
            Log.w(CarLog.TAG_SENSOR, "Failed to get UI_MODE_SERVICE");
        }
    }

    @Override
    public synchronized void init() {
        if (DBG) {
            Log.d(CarLog.TAG_SENSOR,"CAR dayNight init.");
        }
        mCarPropertyService.registerListener(VehicleProperty.NIGHT_MODE, 0,
                mICarPropertyEventListener);
    }

    @Override
    public synchronized void release() {
    }

    @Override
    public synchronized void dump(PrintWriter writer) {
        writer.println("*DAY NIGHT POLICY*");
        writer.println("Mode:" +
                ((mNightSetting == UiModeManager.MODE_NIGHT_YES) ? "night" : "day"));
        writer.println("Forced Mode? " + (mForcedMode == FORCED_SENSOR_MODE ? "false"
                : (mForcedMode == FORCED_DAY_MODE ? "day" : "night")));
    }
}
