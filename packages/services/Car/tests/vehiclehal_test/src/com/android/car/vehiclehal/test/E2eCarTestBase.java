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

import static org.junit.Assert.assertNotNull;
import static org.junit.Assume.assumeTrue;

import android.car.Car;
import android.content.ComponentName;
import android.content.Context;
import android.content.ServiceConnection;
import android.hardware.automotive.vehicle.V2_0.IVehicle;
import android.hardware.automotive.vehicle.V2_0.StatusCode;
import android.hardware.automotive.vehicle.V2_0.VehiclePropValue;
import android.os.ConditionVariable;
import android.os.FileUtils;
import android.os.IBinder;
import android.support.test.InstrumentationRegistry;
import android.util.Log;

import com.google.android.collect.Lists;

import org.json.JSONException;
import org.junit.After;
import org.junit.Before;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.List;


public class E2eCarTestBase {
    private static final String TAG = Utils.concatTag(E2eCarTestBase.class);
    private static final int DEFAULT_WAIT_TIMEOUT_MS = 1000;

    protected IVehicle mVehicle;
    protected Car mCar;
    protected Context mContext;
    private final CarConnectionListener mConnectionListener = new CarConnectionListener();

    @Before
    public void connectToVehicleHal() throws Exception {
        mVehicle = Utils.getVehicle();
        mVehicle.getPropConfigs(
                Lists.newArrayList(VhalEventGenerator.GENERATE_FAKE_DATA_CONTROLLING_PROPERTY),
                (status, propConfigs) -> assumeTrue(status == StatusCode.OK));
    }

    @Before
    public void connectToCarService() {
        mContext = InstrumentationRegistry.getContext();
        mCar = Car.createCar(mContext, mConnectionListener);
        assertNotNull(mCar);
        mCar.connect();
        mConnectionListener.waitForConnection(DEFAULT_WAIT_TIMEOUT_MS);
    }

    @After
    public void disconnect() {
        if (mVehicle != null) {
            mVehicle = null;
        }
        if (mCar != null) {
            mCar.disconnect();
            mCar = null;
        }
    }

    protected List<VehiclePropValue> getExpectedEvents(String fileName)
            throws IOException, JSONException {
        try (InputStream in = mContext.getAssets().open(fileName)) {
            Log.d(TAG, "Reading golden test data" + fileName);
            return VhalJsonReader.readFromJson(in);
        }
    }

    /**
     * The method copies the test data from assets/ to internal storage and make it publicly
     * readable, so that default VHAL can access and read the test data.
     */
    protected File makeShareable(String fileName) throws IOException {
        File filesDir = mContext.getFilesDir();
        // Set publicly executable permission to make sure app internal storage:
        // /data/user/0/<package> is accessible for default VHAL service
        if (!filesDir.getParentFile().setExecutable(true, false)) {
            Log.w(TAG, "Failed to set parent directory +x permission"
                    + filesDir.getParentFile().getAbsolutePath());
        }
        File internalFile = new File(filesDir, fileName);

        try (
            InputStream in = mContext.getAssets().open(fileName);
            OutputStream out = new FileOutputStream(internalFile)
        ) {
            Log.d(TAG, "Copying golden test data to " + internalFile.getAbsolutePath());
            FileUtils.copy(in, out);
        }
        // Make sure the copied test file is publicly readable for default VHAL service. This
        // operation is risky with security holes and should only be used for testing scenarios.
        if (!internalFile.setReadable(true, false)) {
            Log.w(TAG, "Failed to set read permission for " + internalFile.getAbsolutePath());
        }
        return internalFile;
    }

    private static class CarConnectionListener implements ServiceConnection {
        private final ConditionVariable mConnectionWait = new ConditionVariable();

        void waitForConnection(long timeoutMs) {
            mConnectionWait.block(timeoutMs);
        }

        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            mConnectionWait.open();
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {}
    }
}
