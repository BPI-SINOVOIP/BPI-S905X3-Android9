/*
 * Copyright 2018 The Android Open Source Project
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
package com.android.bluetooth.hfp;

import android.bluetooth.BluetoothDevice;
import android.os.Looper;
import android.util.Log;

import com.android.bluetooth.Utils;
import com.android.bluetooth.btservice.AdapterService;

/**
 * Factory class for object initialization to help with unit testing
 */
public class HeadsetObjectsFactory {
    private static final String TAG = HeadsetObjectsFactory.class.getSimpleName();
    private static HeadsetObjectsFactory sInstance;
    private static final Object INSTANCE_LOCK = new Object();

    private HeadsetObjectsFactory() {}

    /**
     * Get the singleton instance of object factory
     *
     * @return the singleton instance, guaranteed not null
     */
    public static HeadsetObjectsFactory getInstance() {
        synchronized (INSTANCE_LOCK) {
            if (sInstance == null) {
                sInstance = new HeadsetObjectsFactory();
            }
        }
        return sInstance;
    }

    /**
     * Allow unit tests to substitute HeadsetObjectsFactory with a test instance
     *
     * @param objectsFactory a test instance of the HeadsetObjectsFactory
     */
    private static void setInstanceForTesting(HeadsetObjectsFactory objectsFactory) {
        Utils.enforceInstrumentationTestMode();
        synchronized (INSTANCE_LOCK) {
            Log.d(TAG, "setInstanceForTesting(), set to " + objectsFactory);
            sInstance = objectsFactory;
        }
    }

    /**
     * Make a {@link HeadsetStateMachine}
     *
     * @param device the remote device associated with this state machine
     * @param looper the thread that the state machine is supposed to run on
     * @param headsetService the headset service
     * @param adapterService the adapter service
     * @param nativeInterface native interface
     * @param systemInterface system interface
     * @return a state machine that is initialized and started, ready to go
     */
    public HeadsetStateMachine makeStateMachine(BluetoothDevice device, Looper looper,
            HeadsetService headsetService, AdapterService adapterService,
            HeadsetNativeInterface nativeInterface, HeadsetSystemInterface systemInterface) {
        return HeadsetStateMachine.make(device, looper, headsetService, adapterService,
                nativeInterface, systemInterface);
    }

    /**
     * Destroy a state machine
     *
     * @param stateMachine to be destroyed. Cannot be used after this call.
     */
    public void destroyStateMachine(HeadsetStateMachine stateMachine) {
        HeadsetStateMachine.destroy(stateMachine);
    }

    /**
     * Get a system interface
     *
     * @param service headset service
     * @return a system interface
     */
    public HeadsetSystemInterface makeSystemInterface(HeadsetService service) {
        return new HeadsetSystemInterface(service);
    }

    /**
     * Get a singleton native interface
     *
     * @return the singleton native interface
     */
    public HeadsetNativeInterface getNativeInterface() {
        return HeadsetNativeInterface.getInstance();
    }
}
