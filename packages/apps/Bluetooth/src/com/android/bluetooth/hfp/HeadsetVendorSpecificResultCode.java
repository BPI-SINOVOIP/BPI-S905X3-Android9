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

class HeadsetVendorSpecificResultCode extends HeadsetMessageObject {
    BluetoothDevice mDevice;
    String mCommand;
    String mArg;

    HeadsetVendorSpecificResultCode(BluetoothDevice device, String command, String arg) {
        mDevice = device;
        mCommand = command;
        mArg = arg;
    }

    @Override
    public void buildString(StringBuilder builder) {
        if (builder == null) {
            return;
        }
        builder.append(this.getClass().getSimpleName())
                .append("[device=")
                .append(mDevice)
                .append(", command=")
                .append(mCommand)
                .append(", arg=")
                .append(mArg)
                .append("]");
    }
}
