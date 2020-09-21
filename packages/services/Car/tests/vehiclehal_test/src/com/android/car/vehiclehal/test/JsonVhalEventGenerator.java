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

import static org.junit.Assert.assertEquals;

import android.hardware.automotive.vehicle.V2_0.IVehicle;
import android.hardware.automotive.vehicle.V2_0.StatusCode;
import android.hardware.automotive.vehicle.V2_0.VehiclePropValue;
import android.os.RemoteException;

import com.android.car.vehiclehal.VehiclePropValueBuilder;

import java.io.File;

class JsonVhalEventGenerator implements VhalEventGenerator {

    private IVehicle mVehicle;
    private File mFile;

    JsonVhalEventGenerator(IVehicle vehicle) {
        mVehicle = vehicle;
    }

    public JsonVhalEventGenerator setJsonFile(File file) throws Exception {
        if (!file.exists()) {
            throw new Exception("JSON test data file does not exist: " + file.getAbsolutePath());
        }
        mFile = file;
        return this;
    }

    @Override
    public void start() throws RemoteException {
        VehiclePropValue request =
                VehiclePropValueBuilder.newBuilder(GENERATE_FAKE_DATA_CONTROLLING_PROPERTY)
                    .addIntValue(CMD_START_JSON)
                    .setStringValue(mFile.getAbsolutePath())
                    .build();
        assertEquals(StatusCode.OK, mVehicle.set(request));
    }

    @Override
    public void stop() throws RemoteException {
        VehiclePropValue request =
                VehiclePropValueBuilder.newBuilder(GENERATE_FAKE_DATA_CONTROLLING_PROPERTY)
                    .addIntValue(CMD_STOP_JSON)
                    .build();
        assertEquals(StatusCode.OK, mVehicle.set(request));
    }
}
