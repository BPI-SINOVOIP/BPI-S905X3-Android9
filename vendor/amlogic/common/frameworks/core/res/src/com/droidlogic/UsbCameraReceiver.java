/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC UsbCameraReceiver
 */

package com.droidlogic;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import android.util.Log;

import com.droidlogic.app.UsbCameraManager;

public class UsbCameraReceiver extends BroadcastReceiver {
    private static final String TAG = "UsbCameraReceiver";

    @Override
    public void onReceive(Context context, Intent intent) {
        String action = intent.getAction();
        Log.i(TAG, "action: " + action);

        if(UsbManager.ACTION_USB_DEVICE_ATTACHED.equals(action)){
            UsbDevice device = (UsbDevice)intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);

            new UsbCameraManager(context).UsbDeviceAttach(device, true);
        }
        else if(UsbManager.ACTION_USB_DEVICE_DETACHED.equals(action)){
            UsbDevice device = (UsbDevice)intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);

            new UsbCameraManager(context).UsbDeviceAttach(device, false);
        }
    }
}
