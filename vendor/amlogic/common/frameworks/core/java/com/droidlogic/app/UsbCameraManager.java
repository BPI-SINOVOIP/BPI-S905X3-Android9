/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC UsbCameraManager
 */

package com.droidlogic.app;

import java.io.File;

import android.app.ActivityManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.hardware.Camera.CameraInfo;
import android.hardware.usb.UsbConstants;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbInterface;
//import android.hardware.ICameraService;
import android.os.IBinder;
import android.os.Binder;
import android.os.DeadObjectException;
import android.os.Parcel;
import android.os.RemoteException;
import android.os.UserHandle;
//import android.os.ServiceManager;
//import android.os.ServiceSpecificException;
import android.util.Log;

public class UsbCameraManager {
    private static final String TAG             = "UsbCameraManager";

    private static final boolean DEBUG          = false;

    private static final String PACKAGES[]      = {
        "com.android.camera2",
    };

    private static final String ACTIVITIES[]    = {
        "com.android.camera.CameraLauncher",
    };

    private Context mContext;
    //private ICameraService mCameraService = null;
    public UsbCameraManager(Context context){
        mContext = context;
    }

    private void usbCameraAttach(boolean isAttach){
        /*
        try {
            if (null == mCameraService) {
                IBinder cameraServiceBinder = ServiceManager.getService("media.camera");
                if (cameraServiceBinder == null) {
                    // Camera service is now down, leave mCameraService as null
                    return;
                }
                mCameraService = ICameraService.Stub.asInterface(cameraServiceBinder);
            }

            if (null != mCameraService) {
                try {
                    mCameraService.usbCameraAttach(isAttach);
                } catch(ServiceSpecificException e) {
                    Log.e(TAG, "USB camera attach:" + e);
                } catch (RemoteException e) {
                    // camera service just died - if no camera service, then no devices
                    Log.e(TAG, "USB camera attach:" + e);
                }
            }
        } catch (Exception ex) {
            Log.e(TAG, "USB camera attach:" + ex);
        }
        */
    }

    public void bootReady(){
        if (!hasCamera()) {
            Log.i(TAG, "bootReady disable all camera activities");
            for (int i = 0; i < ACTIVITIES.length; i++) {
                disableComponent(PACKAGES[i], ACTIVITIES[i]);
            }
        }
    }

    public void UsbDeviceAttach(UsbDevice device, boolean isAttach){
        if (isUsbCamera(device)) {
            Log.i(TAG, "usb camera attach: " + isAttach);
            new VideoDevThread(mContext, isAttach).start();
        }
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

    private void disableComponent(String pkg, String klass) {
        ComponentName name = new ComponentName(pkg, klass);
        PackageManager pm = mContext.getPackageManager();

        try {
            // We need the DONT_KILL_APP flag, otherwise we will be killed
            // immediately because we are in the same app.
            pm.setComponentEnabledSetting(name,
                PackageManager.COMPONENT_ENABLED_STATE_DISABLED,
                PackageManager.DONT_KILL_APP);
        } catch (Exception e) {
            Log.w(TAG, "Unable disable component: " + name, e);
        }
    }

    private void enableComponent(String pkg, String klass) {
        ComponentName name = new ComponentName(pkg, klass);
        PackageManager pm = mContext.getPackageManager();

        try {
            pm.setComponentEnabledSetting(name,
                PackageManager.COMPONENT_ENABLED_STATE_ENABLED,
                PackageManager.DONT_KILL_APP);
        } catch (Exception e) {
            Log.w(TAG, "Unable enable component: " + name, e);
        }
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
            /*
            if (intf.getInterfaceClass() == android.hardware.usb.UsbConstants.USB_CLASS_STILL_IMAGE &&
                    intf.getInterfaceSubclass() == 1 &&
                    intf.getInterfaceProtocol() == 1) {
                return true;
            }*/

            if (intf == null) {
                Log.e(TAG,"intf is null , return");
                continue;
            }
            if (intf.getInterfaceClass() == UsbConstants.USB_CLASS_VIDEO) {
                return true;
            }
        }
        return false;
    }

    class VideoDevThread extends Thread {
        private static final String DEV_VIDEO_prefix = "/dev/video";
        private static final int DEV_NUM = 5;
        Context mContext;
        int mCamNum = 0;
        boolean mIsAttach;

        public VideoDevThread(Context context, boolean isAttach) {
            mContext = context;
            mIsAttach = isAttach;
            mCamNum = android.hardware.Camera.getNumberOfCameras();

            Log.i(TAG, "VideoDevThread isAttach:" + isAttach + ", cur camera num:" + mCamNum);
        }

        @Override
        public void run() {
            boolean end = false;
            int loopCount = 0;
            while ( !end ) {
                try {
                    if (mIsAttach) {
                        Thread.sleep(500);//first delay 500ms, in order to wait kernel set up video device path
                    } else {
                        Thread.sleep(50);
                    }
                }
                catch (InterruptedException e){
                    Thread.currentThread().interrupt();
                }

                int devNum = 0;
                for ( int i = 0; i < DEV_NUM; i++ ) {
                    String path = DEV_VIDEO_prefix + i;
                    if (new File(path).exists()) {
                        devNum++;
                        Log.i(TAG, "/dev/video* num:" + devNum);
                    }
                }
                if (mIsAttach &&
                    //device path has been set up by kernel
                    ((devNum > mCamNum) ||
                    //video device was plugged in when boot
                    ((devNum > 0) && (mCamNum == devNum)))) {
                    usbCameraAttach(mIsAttach);
                    for (int i = 0; i < ACTIVITIES.length; i++) {
                        enableComponent(PACKAGES[i], ACTIVITIES[i]);
                    }
                    ActivityManager am = (ActivityManager) mContext.getSystemService(Context.ACTIVITY_SERVICE);
                    am.forceStopPackage("com.skype.rover");
                    end = true;
                }
                else if (!mIsAttach) {//device path has been deleted by kernel
                    usbCameraAttach(mIsAttach);
                    if (devNum < 1) {
                        for (int i = 0; i < ACTIVITIES.length; i++) {
                            disableComponent(PACKAGES[i], ACTIVITIES[i]);
                        }
                    }
                    end = true;

                    //if plug out the usb camera, need exit camera app
                    ActivityManager am = (ActivityManager) mContext.getSystemService(Context.ACTIVITY_SERVICE);
                    ComponentName cn = am.getRunningTasks (1).get (0).topActivity;
                    String name = cn.getClassName();

                    Log.i(TAG, "usb camera plug out top activity:" + name + " pkg:" + cn.getPackageName());
                    for (int i = 0; i < ACTIVITIES.length; i++) {
                        if (name.equals (ACTIVITIES[i])) {
                            Intent homeIntent = new Intent(Intent.ACTION_MAIN, null);
                            homeIntent.addCategory(Intent.CATEGORY_HOME);
                            homeIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK
                                    | Intent.FLAG_ACTIVITY_RESET_TASK_IF_NEEDED);
                            mContext.startActivity(homeIntent);

                            try{
                                Thread.sleep(500);
                            }catch (InterruptedException e){
                            }
                            Log.i(TAG, "usb camera plug out kill:" + cn.getPackageName());
                            am.forceStopPackage(cn.getPackageName());
                            //am.killBackgroundProcesses (cn.getPackageName());
                            break;
                        }
                    }
                }
                else if ((mCamNum > 0) && (mCamNum == devNum)) {//video device was plugged in when boot
                    loopCount++;
                    if (loopCount > 2) {//1s kernel has set up or delete the device path
                        end = true;
                    }
                }
            }
        }
    }
}
