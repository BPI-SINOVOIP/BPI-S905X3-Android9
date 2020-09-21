/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.server.telecom.components;

import com.android.server.telecom.TelecomSystem;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;

/**
 * Bluetooth headset manager for Telecom. This class shares the call state with the bluetooth device
 * and accepts call-related commands to perform on behalf of the BT device.
 */
public final class BluetoothPhoneService extends Service implements TelecomSystem.Component {

    @Override
    public IBinder onBind(Intent intent) {
        synchronized (getTelecomSystem().getLock()) {
            return getTelecomSystem().getBluetoothPhoneServiceImpl().getBinder();
        }
    }

    @Override
    public TelecomSystem getTelecomSystem() {
        return TelecomSystem.getInstance();
    }
}
