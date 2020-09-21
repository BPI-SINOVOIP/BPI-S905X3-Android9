/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC BluetoothDevicePairer
 */

package com.droidlogic.tv.settings;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothClass;
import android.bluetooth.BluetoothDevice;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.hardware.input.InputManager;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Message;
import android.os.SystemClock;
import android.util.Log;
import android.view.InputDevice;
import android.annotation.SuppressLint;
import android.app.IntentService;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;
import android.bluetooth.BluetoothClass.Device;
import android.bluetooth.BluetoothProfile;
import android.os.SystemProperties;
import android.os.Looper;
import android.widget.Toast;
import android.view.Gravity;
import java.util.Timer;
import java.util.TimerTask;
import java.lang.reflect.Method;
import java.util.Iterator;
import java.util.Set;
import java.util.ArrayList;
import java.util.List;

public class BluetoothDevicePairer {

    public static final String TAG = "BluetoothDevicePairer";
    public static final int STATUS_ERROR = -1;
    public static final int STATUS_NONE = 0;
    public static final int STATUS_SCANNING = 1;
    public static final int STATUS_BONDFAIL = 2;
    public static final int STATUS_CONNECTFAIL = 3;
    public static final int STATUS_CONNECTED = 4;
    public static final int STATUS_FINDED = 5;

    public static final int DELAY_BOUND = 2 * 1000;
    public static final int DELAY_RESCAN = 4 * 1000;
    public static final int DELAY_REFIND = 9 * 1000;

    private static final int MSG_PAIR = 1;
    private static final int MSG_START = 2;
    private static final int MSG_RESCANT = 3;
    private static final int MSG_UPLIST = 4;

    private final Context mContext;
    private EventListener mListener;
    private Receiver mReceiver;
    private int mStatus = STATUS_NONE;


    private BluetoothDevice mTarget = null;
    private final Handler mHandler;

    private static BluetoothDevice RemoteDevice;
    private String mBtMacPrefix;
    private String mBtNamePrefix;
    private String mBtClass;
    private BluetoothProfile mService = null;
    private BluetoothAdapter mBluetoothAdapter = null;
    private static boolean mActionFlag = true;
    private static boolean mScanFlag   = true;
    private static boolean mBondFlag   = true;
    private static boolean mFindFlag   = false;
    private static boolean mFindingFlag   = false;

    //add for HandlerThread
    public HandlerThread scanner = new HandlerThread("auto_bt");
    private Handler mAutoHandler;

    private static final String CONNECTING_TO_REMOTE = "Auto Connecting to Amlogic Remote...";
    private Handler msHandler;
    private Toast toast;
    private static int[] mRssi = {60,60};
    private int mRssitarge = 0;
    private static int mRssiLimit = 0;
    private static final boolean DEBUG = false;

    private void Log(String msg) {
        if (DEBUG) {
            Log.i(TAG, msg);
        }
    }

    public BluetoothDevicePairer(Context context, EventListener listener) {
        mContext = context.getApplicationContext();
        mListener = listener;
        mReceiver = new Receiver();
        scanner.start();
       //mAutoHandler = new Handler(scanner.getLooper());
        IntentFilter filter = new IntentFilter(BluetoothDevice.ACTION_FOUND);
        filter.addAction(BluetoothAdapter.ACTION_DISCOVERY_FINISHED);
        mContext.registerReceiver(mReceiver, filter);
        msHandler = new Handler(Looper.getMainLooper());
        mHandler = new Handler() {
            @Override
            public void handleMessage(Message msg) {
                switch (msg.what) {
                    case MSG_UPLIST:
                         updateListener();
                         break;
                    default:
                        Log.d(TAG, "No mHandler case available for message: " + msg.what);
                }
            }
        };

        mAutoHandler = new Handler(scanner.getLooper()){
            @Override
            public void handleMessage(Message msg) {
                switch (msg.what) {
                    case MSG_PAIR:
                       // startBonding();
                        break;
                    case MSG_START:
                        startBonding();
                        break;
                   case MSG_RESCANT:
                        RestartScan();
                        break;
                   default:
                        Log.d(TAG, "No mAutoHandler case available for message: " + msg.what);
                }
            }


        };
    }



     public interface EventListener {
        /**
         * The status of the {@link BluetoothDevicePairer} changed.
         */
        void statusChanged();
    }

    public void startAutoPair(){
        Log("startAutoPair begin");
        if (!getSpecialDeviceInfo()) {
            Log("getSpecialDeviceInfo fail!");
            return;
        }
        if (initBt(mContext)) {
            try {
               Thread.sleep(1000);
            } catch(Exception e) {
              e.printStackTrace();
            }
            if (isSpecialDevicePaired())  {
                Log("Hasfound SpecialDevicePaired!");
            }

            startScan();
            mAutoHandler.sendEmptyMessageDelayed(MSG_RESCANT, DELAY_RESCAN);
            Log("Exit BluetoothAutoPairService!");
        }
        else{
            setStatus(STATUS_ERROR);
            Log("no  Bluetooth");
       }
    }

    public void startScan() {
        if (mBluetoothAdapter.isDiscovering()) {
          Log(" isDiscovering stopLeScan!");
          mBluetoothAdapter.cancelDiscovery();
        }
        Log("startLeScan!");
        mFindFlag = false;
        mFindingFlag = true;
        setStatus(STATUS_SCANNING);
        mBluetoothAdapter.startDiscovery();;
    }

    public void RestartScan() {
        if (mFindFlag) {
            Log("find remote! dont rescan");
        }
        else{
            Log("RestartScan");
            mAutoHandler.removeMessages(MSG_RESCANT);
            mAutoHandler.sendEmptyMessageDelayed(MSG_RESCANT, DELAY_REFIND);
            startScan();
       }
    }

    public void stopScan() {
         Log("stopLeScan function!");
         mAutoHandler.removeMessages(MSG_RESCANT);
         if (mFindingFlag) {
             Log("stopLeScan!");
             mFindingFlag=false;
             mBluetoothAdapter.cancelDiscovery();
         }
    }

    public void stop() {
         Log("stop function!");
         mContext.unregisterReceiver(mReceiver);
         stopScan();
   }


    private boolean getSpecialDeviceInfo() {
        mBtMacPrefix = SystemProperties.get("ro.vendor.autoconnectbt.macprefix", "00:CD:FF");
        Log("getSpecialDeviceInfo mBtMacPrefix:" + mBtMacPrefix);
        mBtClass = SystemProperties.get("ro.vendor.autoconnectbt.btclass", "50c");
        Log("getSpecialDeviceInfo mBtClass:" + mBtClass);
        mBtNamePrefix = SystemProperties.get("ro.vendor.autoconnectbt.nameprefix", "Amlogic_RC");
        Log("getSpecialDeviceInfo mBtNamePrefix:" + mBtNamePrefix);
        mRssiLimit = Integer.parseInt(SystemProperties.get("ro.vendor.autoconnectbt.rssilimit","70"));
        Log("getSpecialDeviceInfo mRssiLimit:" + mRssiLimit);
        return (!mBtNamePrefix.isEmpty() && !mBtClass.isEmpty());
    }
    private boolean isSpecialDevice(BluetoothDevice bd) {
        Log("get bd.getName:"+bd.getName());
        Log("get bd.getAddress:"+bd.getAddress());
        if ( null == bd.getName() ) {
           Log("get bd.getName fail");
           return false;
        }
        if (bd.getBluetoothClass().getMajorDeviceClass() == Device.Major.PERIPHERAL) {
            return true;
        }
        return  (bd.getName().startsWith(mBtNamePrefix) || bd.getAddress().startsWith(mBtMacPrefix));
    }


    private class Receiver extends BroadcastReceiver {
       @Override
        public void onReceive(Context context, Intent intent) {
            final String action = intent.getAction();
            if (BluetoothDevice.ACTION_FOUND.equals(action)) {
                BluetoothDevice device = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
                if (null ==  device.getName())  return;
                short rssi = intent.getExtras().getShort(BluetoothDevice.EXTRA_RSSI);
                Log("BluetoothDevice = " + device.getName() +
                " Address=" + device.getAddress() +
                " class=" + device.getBluetoothClass().toString() +" rssi:"+(short)rssi );
                BluetoothDevice btDevice=device;
                if (isSpecialDevice(btDevice) && (!mFindFlag)) {
                    if (getAverageRssi(rssi) < mRssiLimit) {
                        Log("Scan result isSpecialDevice and in limit rssi value");
                        Log("Find remoteDevice and parm: name= " + btDevice.getName() +
                            " Address=" + btDevice.getAddress() +
                            " class=" + btDevice.getBluetoothClass().toString()+"rssi:"+(short)rssi);
                        RemoteDevice = mBluetoothAdapter.getRemoteDevice(btDevice.getAddress());
                        mTarget = RemoteDevice;
                        setStatus(STATUS_FINDED);
                        stopScan();
                        mAutoHandler.removeMessages(MSG_RESCANT);
                        mAutoHandler.sendEmptyMessageDelayed(MSG_START, DELAY_BOUND);
                        mFindFlag = true;
                   }
                }
             }
       }
    }

    private BluetoothAdapter.LeScanCallback mLeScanCallback =
    new BluetoothAdapter.LeScanCallback(){
        @Override
        public void onLeScan(final BluetoothDevice device, int rssi, byte[] scanRecord){
               if (null ==  device.getName())  return;
                Log("BluetoothDevice = " + device.getName() +
                " Address=" + device.getAddress() +
                " class=" + device.getBluetoothClass().toString() +" rssi:"+(short)rssi );
                BluetoothDevice btDevice=device;
                if ( (btDevice != null) && isSpecialDevice(btDevice) && (!mFindFlag)) {
                    if (getAverageRssi(rssi) < mRssiLimit) {
                        Log("Scan result isSpecialDevice and in limit rssi value");
                        if (btDevice.getBondState() == BluetoothDevice.BOND_NONE) {
                            Log("Device no bond!");
                            Log("Find remoteDevice and parm: name= " + btDevice.getName() +
                            " Address=" + btDevice.getAddress() +
                            " class=" + btDevice.getBluetoothClass().toString()+"rssi:"+(short)rssi);
                            RemoteDevice = mBluetoothAdapter.getRemoteDevice(btDevice.getAddress());
                            mTarget = RemoteDevice;
                            Intent intent = new Intent();
                            intent.setAction(BluetoothDevice.ACTION_FOUND);
                            intent.putExtra(BluetoothDevice.EXTRA_DEVICE, RemoteDevice);
                            intent.putExtra(BluetoothDevice.EXTRA_RSSI, (short)rssi);
                            intent.putExtra(BluetoothDevice.EXTRA_NAME, btDevice.getName());
                            intent.putExtra(BluetoothDevice.EXTRA_CLASS, btDevice.getBluetoothClass());
                            mContext.sendBroadcast(intent);
                            stopScan();
                            mAutoHandler.sendEmptyMessageDelayed(MSG_START, DELAY_BOUND);
                            mFindFlag = true;

                        } else {
                            Log("Device has bond");
                        }
                    }
                }
       }
    };
    private void connected(BluetoothDevice device) throws Exception {
        if ( mService != null && device != null ) {
            Log("Connecting to target: " + device.getAddress());
            try {
                Class clz = Class.forName("android.bluetooth.BluetoothHidHost");
                if (clz.cast(mService) == null)
		    return;
                Method connect = clz.getMethod("connect",BluetoothDevice.class);
                Method setPriority = clz.getMethod("setPriority",BluetoothDevice.class, int.class);
                boolean ret = (boolean)connect.invoke(clz.cast(mService),device);
                if (ret) {
                    setPriority.invoke(clz.cast(mService),device,1000/*BluetoothProfile.PRIORITY_AUTO_CONNECT*/);
                    showToast(CONNECTING_TO_REMOTE);
		    setStatus(STATUS_CONNECTED);
                    Log("connect ok and show toast!");
                } else {
                    Log("connect fail!");
		    setStatus(STATUS_CONNECTFAIL);
		    mAutoHandler.sendEmptyMessageDelayed(MSG_RESCANT, DELAY_RESCAN);
                }
            } catch (Exception e) {
                e.printStackTrace();
	    }
        } else {
            Log("mService or device no work!");
        }
    }
    static public boolean createBond(Class btClass,BluetoothDevice device) throws Exception {
        Method createBondMethod = btClass.getMethod("createBond");
        Boolean returnValue = (Boolean) createBondMethod.invoke(device);
        return returnValue.booleanValue();
    }

    static public boolean removeBond(Class btClass,BluetoothDevice device) throws Exception {
        Method removeBondMethod = btClass.getMethod("removeBond");
        Boolean returnValue = (Boolean) removeBondMethod.invoke(device);
        return returnValue.booleanValue();
    }

    private boolean initBt(Context context) {
        mBluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        if (mBluetoothAdapter == null) {
            Log("No bluetooth device!");
            return false;
        } else {
            Log("have Bluetooth device!");
            if (!mBluetoothAdapter.isEnabled()) {
                Log("Bluetooth device open by autoserver!");
                mBluetoothAdapter.enable();
            try {
                     Thread.sleep(3000);
                } catch(Exception e) {
                     e.printStackTrace();
                }
            }
        }
        if (!mBluetoothAdapter.getProfileProxy(mContext, mServiceConnection,
                4/*BluetoothProfile.HID_HOST*/)) {
            Log("Bluetooth getProfileProxy failed!");
        }
        return true;
    }
    private BluetoothProfile.ServiceListener mServiceConnection =
            new BluetoothProfile.ServiceListener() {

        @Override
        public void onServiceDisconnected(int profile) {
            Log("Bluetooth service proxy disconnected");
        }

        @Override
        public void onServiceConnected(int profile, BluetoothProfile proxy) {
            if (DEBUG) {
                Log("Bluetooth service proxy connected");
            }
            mService = proxy;
        }
    };


    private boolean isSpecialDevicePaired() {
        Set<BluetoothDevice> bts = mBluetoothAdapter.getBondedDevices();
        Iterator<BluetoothDevice> iterator = bts.iterator();
        while (iterator.hasNext()) {
            BluetoothDevice bd = iterator.next();
            if (isSpecialDevice(bd)) {
            Log("RemoteDevice has bond: name=" + bd.getName() +
                " Address=" + bd.getAddress() +
                " class=" + bd.getBluetoothClass().toString());
                try {
                     mActionFlag=false;
                } catch (Exception e) {
                    e.printStackTrace();
                }
                return true;
            }
        }
        return false;
    }


     private void showToast(final String text) {
        msHandler.post(new Runnable() {
            @Override
            public void run() {
                if (toast != null)
                    toast.cancel();
                toast = Toast.makeText(mContext, text, Toast.LENGTH_SHORT);
                toast.show();
                Log("Toast show ");
            }
        });
    }

    private int getAverageRssi(int mRssinew){
        mRssinew = Math.abs(mRssinew);
        mRssi[0] = mRssi[1];
        mRssi[1] = mRssinew;
        Log("Average =" + (mRssi[0]+mRssi[1])/2);
        return (mRssi[0]+mRssi[1])/2;
    }

    public void start() {
        mAutoHandler.post(new Runnable(){
          public void run() {
               startAutoPair();
          }
       });//end post
    }

    public BluetoothDevice getTargetDevice() {
        return mTarget;
    }

    public int getStatus() {
        return mStatus;
    }

    public void setListener(EventListener listener) {
        mListener = listener;
    }

    private void updateListener() {
        if (mListener != null) {
            mListener.statusChanged();
        }
    }

    private void setStatus(int status) {
        mStatus = status;
        mHandler.sendEmptyMessage(MSG_UPLIST);//updateListener();
    }

    private void startBonding() {
            try{
                        Log.d(TAG,"Device start bond...");
                        if (RemoteDevice.getBondState() == BluetoothDevice.BOND_BONDED) {
                             Log("Remote Device is bonded, remove bond !");
                             removeBond( RemoteDevice.getClass(), RemoteDevice );
                             Thread.sleep(3000);
                        }
                        if ( createBond( RemoteDevice.getClass(), RemoteDevice ) ) {
                            Log("Remote Device bond ok!");
                        } else {
                            Log("Remote Device bond failed!");
                        }

                        Thread.sleep(3000);
                        int bondState = RemoteDevice.getBondState();
                        if ( bondState == BluetoothDevice.BOND_BONDED ) {
                            connected(RemoteDevice);
                        } else {
                            Log.d(TAG,"Remote Device no bond! try again");
                            int mConnetFail=0;
                            while ( bondState != BluetoothDevice.BOND_BONDED ) {
                                //createBond( RemoteDevice.getClass(), RemoteDevice );
                                Log.d(TAG," add waitting BT bond...");
                                Thread.sleep(3000);
                                bondState = RemoteDevice.getBondState();
                                mConnetFail++;
                                if ( mConnetFail > 5 ) {
                                    Log.d(TAG,"BT bond fail ...");
                                    setStatus(STATUS_BONDFAIL);
                                    mFindFlag = false;
                                    mAutoHandler.sendEmptyMessageDelayed(MSG_RESCANT, DELAY_RESCAN);
                                    return;
                                }
                            }
                            connected(RemoteDevice);
                        }
                    }catch (Exception e) {
                        e.printStackTrace();
                    }
    }

}
