/*
 * Copyright (C) 2012 The Android Open Source Project
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

package com.android.camera;

import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.hardware.Camera.CameraInfo;
import android.hardware.usb.UsbAccessory;
import android.hardware.usb.UsbConstants;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbInterface;
import android.hardware.usb.UsbManager;

import com.android.camera.settings.CameraPictureSizesCacher;
import com.android.camera.debug.Log;

// We want to disable camera-related activities if there is no camera. This
// receiver runs when BOOT_COMPLETED intent is received. After running once
// this receiver will be disabled, so it will not run again.
public class DisableCameraReceiver extends BroadcastReceiver {
    private static final Log.Tag TAG = new Log.Tag("DisableCamRcver");
    private static final boolean CHECK_BACK_CAMERA_ONLY = false;
    private static final boolean DEBUG = false;
    private static final String ACTIVITIES[] = {
        "com.android.camera.CameraLauncher",
    };

    @Override
    public void onReceive(Context context, Intent intent) {
        Log.d(TAG, "onReceive intent=" + intent);
        String action = intent.getAction();
        // Disable camera-related activities if there is no camera.
        boolean needCameraActivity = CHECK_BACK_CAMERA_ONLY
            ? hasBackCamera()
            : hasCamera();

        if (Intent.ACTION_BOOT_COMPLETED.equals(action)) {
            if (!needCameraActivity) {
                Log.i(TAG, "disable all camera activities");
                for (int i = 0; i < ACTIVITIES.length; i++) {
                    disableComponent(context, ACTIVITIES[i]);
                }
            }
        }
        // Disable this receiver so it won't run again.
        disableComponent(context, "com.android.camera.DisableCameraReceiver");
    }

    private boolean hasCamera() {
        int n = android.hardware.Camera.getNumberOfCameras();
        Log.i(TAG, "number of camera: " + n);
        return (n > 0);
    }

    private boolean hasBackCamera() {
        int n = android.hardware.Camera.getNumberOfCameras();
        CameraInfo info = new CameraInfo();
        for (int i = 0; i < n; i++) {
            android.hardware.Camera.getCameraInfo(i, info);
            if (info.facing == CameraInfo.CAMERA_FACING_BACK) {
                Log.i(TAG, "back camera found: " + i);
                return true;
            }
        }
        Log.i(TAG, "no back camera");
        return false;
    }

    private int getBackCameraId() {
        int n = android.hardware.Camera.getNumberOfCameras();
        CameraInfo info = new CameraInfo();
        for (int i = 0; i < n; i++) {
            android.hardware.Camera.getCameraInfo(i, info);
            if (info.facing == CameraInfo.CAMERA_FACING_BACK) {
                return i;
            }
        }
        return -1;
    }

    private void disableComponent(Context context, String klass) {
        ComponentName name = new ComponentName(context, klass);
        PackageManager pm = context.getPackageManager();

        // We need the DONT_KILL_APP flag, otherwise we will be killed
        // immediately because we are in the same app.
        pm.setComponentEnabledSetting(name,
            PackageManager.COMPONENT_ENABLED_STATE_DISABLED,
            PackageManager.DONT_KILL_APP);
    }

    public boolean isUsbCamera(UsbDevice device) {
        int count = device.getInterfaceCount();
        if (DEBUG) {
            for (int i = 0; i < count; i++) {
                UsbInterface intf = device.getInterface(i);
                Log.i(TAG, "isCamera UsbInterface:" + intf);
            }
        }

        for (int i = 0; i < count; i++) {
            UsbInterface intf = device.getInterface(i);
            if (intf.getInterfaceClass() == UsbConstants.USB_CLASS_VIDEO) {
                return true;
            }
        }
        return false;
    }
}
