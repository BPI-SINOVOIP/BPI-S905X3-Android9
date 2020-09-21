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

package com.google.android.car.vms.subscriber;

import android.app.Activity;
import android.car.vms.VmsAvailableLayers;
import android.car.vms.VmsLayer;
import android.car.vms.VmsSubscriberManager;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.support.car.Car;
import android.support.car.CarConnectionCallback;
import android.util.Log;
import android.widget.TextView;
import java.util.concurrent.Executor;

/**
 * Connects to the Car service during onCreate. CarConnectionCallback.onConnected is invoked when
 * the connection is ready. Then, it subscribes to a VMS layer/version and updates the TextView when
 * a message is received.
 */
public class VmsSubscriberClientSampleActivity extends Activity {
    private static final String TAG = "VmsSampleActivity";
    // The layer id and version should match the ones defined in
    // com.google.android.car.vms.publisher.VmsPublisherClientSampleService
    public static final VmsLayer TEST_LAYER = new VmsLayer(0, 0, 0);

    private Car mCarApi;
    private TextView mTextView;
    private VmsSubscriberManager mVmsSubscriberManager;
    private Executor mExecutor;

    private class ThreadPerTaskExecutor implements Executor {
        public void execute(Runnable r) {
            new Thread(r).start();
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        mExecutor = new ThreadPerTaskExecutor();
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        mTextView = (TextView) findViewById(R.id.textview);
        if (getPackageManager().hasSystemFeature(PackageManager.FEATURE_AUTOMOTIVE)) {
            mCarApi = Car.createCar(this, mCarConnectionCallback);
            mCarApi.connect();
        } else {
            Log.d(TAG, "No automotive feature.");
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (mCarApi != null) {
            mCarApi.disconnect();
        }
        Log.i(TAG, "onDestroy");
    }

    private final CarConnectionCallback mCarConnectionCallback = new CarConnectionCallback() {
        @Override
        public void onConnected(Car car) {
            Log.d(TAG, "Connected to Car Service");
            mVmsSubscriberManager = getVmsSubscriberManager();
            configureSubscriptions(mVmsSubscriberManager);
        }

        @Override
        public void onDisconnected(Car car) {
            Log.d(TAG, "Disconnect from Car Service");
        }

        private VmsSubscriberManager getVmsSubscriberManager() {
            try {
                return (VmsSubscriberManager) mCarApi.getCarManager(
                        android.car.Car.VMS_SUBSCRIBER_SERVICE);
            } catch (android.support.car.CarNotConnectedException e) {
                Log.e(TAG, "Car is not connected!", e);
            }
            return null;
        }

        private void configureSubscriptions(VmsSubscriberManager vmsSubscriberManager) {
            try {
                vmsSubscriberManager.setVmsSubscriberClientCallback(mExecutor, mClientCallback);
                vmsSubscriberManager.subscribe(TEST_LAYER);
            } catch (android.car.CarNotConnectedException e) {
                Log.e(TAG, "Car is not connected!", e);
            }
        }

    };

    private final VmsSubscriberManager.VmsSubscriberClientCallback mClientCallback =
        new VmsSubscriberManager.VmsSubscriberClientCallback() {
            @Override
            public void onVmsMessageReceived(VmsLayer layer, byte[] payload) {
                mTextView.setText(String.valueOf(payload[0]));
            }

            @Override
            public void onLayersAvailabilityChanged(VmsAvailableLayers availableLayers) {
                mTextView.setText(String.valueOf(availableLayers));
            }
        };
}
