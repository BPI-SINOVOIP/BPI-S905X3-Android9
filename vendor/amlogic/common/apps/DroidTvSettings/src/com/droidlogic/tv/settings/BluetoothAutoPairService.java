/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC BluetoothAutoPairService
 */

package com.droidlogic.tv.settings;

//import com.android.settingslib.bluetooth.CachedBluetoothDevice;
import android.annotation.SuppressLint;
import android.app.IntentService;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothClass.Device;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.bluetooth.BluetoothProfile;
import android.content.IntentFilter;
import android.os.SystemProperties;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.widget.Toast;
import android.view.Gravity;
import java.util.Timer;
import java.util.TimerTask;
import java.lang.reflect.Method;
import java.util.Iterator;
import java.util.Set;


@SuppressLint("NewApi")
public class BluetoothAutoPairService extends IntentService {
    private static final String TAG = "BluetoothAutoPairService";
    private static BluetoothDevice RemoteDevice;
    private String mBtMacPrefix;
    private String mBtNamePrefix;
    private String mBtClass;
    private String mBtCallback;
    private BluetoothProfile mService = null;
    private Context mContext = null;
    private static final boolean DEBUG = false;
    private BluetoothAdapter mBluetoothAdapter = null;
    private boolean mActionFlag = true;
    private boolean mScanFlag   = true;
    private boolean mBondFlag   = true;
    private static Timer timer  = null;
    static final long SNAPSHOT_INTERVAL = 30 * 1000L;
    private static final String CONNECTING_TO_REMOTE = "Auto Connecting to Amlogic Remote...";
    private Handler mHandler;
    private Toast toast;
    private int[] mRssi = {80,80,80};
    private int mRssitarge = 0;
    private int mRssiLimit = 0;
    private HandlerThread scanner = new HandlerThread("auto_bt");
    private Handler mAutoHandler;
    //private LeDeviceListAdapter mLeDeviceListAdapter;
    private void Log(String msg) {
        if (DEBUG) {
            Log.i(TAG, msg);
        }
    }
    @Override
    protected void onHandleIntent(Intent intent) {
        Log("onHandleIntent begin");
        mHandler = new Handler(Looper.getMainLooper());
        scanner.start();
        mAutoHandler = new Handler(scanner.getLooper());
        mAutoHandler.post(new Runnable(){
          public void run() {
            if (!getSpecialDeviceInfo()) {
                    Log("getSpecialDeviceInfo fail!");
                    return;
                }
                if (initBt(BluetoothAutoPairService.this)) {
                    try {
                        Thread.sleep(5000);
                    } catch(Exception e) {
                      e.printStackTrace();
                    }
                    timer = new Timer();
                    timer.schedule(new TimerTask() {
                    public void run() {
                        try{
                            mScanFlag=false;
                            mBondFlag=false;
                            Log("Scan BT Timeout!!!");
                        }catch(Exception e) {
                           e.printStackTrace();
                        }
                    }
                    }, SNAPSHOT_INTERVAL, SNAPSHOT_INTERVAL );
                    if (isSpecialDevicePaired())  {
                        Log("Hasfound SpecialDevicePaired!");
                    }
                    if ( mActionFlag == true ) {
                        while (mScanFlag) {
                            if (!mBluetoothAdapter.isDiscovering()) {
                                mBluetoothAdapter.startLeScan(mLeScanCallback);
                            }
                        }
                        Log("Cancel Scan!");
                        timer.cancel();
                        mBluetoothAdapter.stopLeScan(mLeScanCallback);
                        if ( mBondFlag == true ) {
                        try {
                                Thread.sleep(1000);
                                Log("Device start bond...");
                                if ( createBond( RemoteDevice.getClass(), RemoteDevice ) ) {
                                    Log("Remote Device bond ok!");
                                } else {
                                    Log("Remote Device bond failed!");
                                }
                                Thread.sleep(1000);
                                int bondState = RemoteDevice.getBondState();
                                if ( bondState == BluetoothDevice.BOND_BONDED ) {
                                    connected(RemoteDevice);
                                } else {
                                    Log("Remote Device has no bond!");
                                    int mConnetFail=0;
                                    while ( bondState != BluetoothDevice.BOND_BONDED ) {
                                        createBond( RemoteDevice.getClass(), RemoteDevice );
                                        Thread.sleep(1000);
                                        bondState = RemoteDevice.getBondState();
                                        Log.d(TAG,"Renjun.xu add waitting BT bond...");
                                        mConnetFail++;
                                        if ( mConnetFail > 5 ) {
                                            Log.d(TAG,"waitting BT bond fail ...");
                                            break;
                                        }
                                    }
                                    connected(RemoteDevice);
                                }
                            } catch (Exception e) {
                                e.printStackTrace();
                            }
                        }
                    }
                    Log("Exit BluetoothAutoPairService!");
                }
          }
        });

    }
    private boolean getSpecialDeviceInfo() {
        mBtMacPrefix = SystemProperties.get("ro.vendor.autoconnectbt.macprefix", "00:CD:FF");
        Log("getSpecialDeviceInfo mBtMacPrefix:"+mBtMacPrefix);
        mBtClass = SystemProperties.get("ro.vendor.autoconnectbt.btclass", "50c");
        Log("getSpecialDeviceInfo mBtClass:"+mBtClass);
        mBtNamePrefix = SystemProperties.get("ro.vendor.autoconnectbt.nameprefix", "Amlogic_RC");
        Log("getSpecialDeviceInfo mBtNamePrefix:"+mBtNamePrefix);
        mRssiLimit = Integer.parseInt(SystemProperties.get("ro.vendor.autoconnectbt.rssilimit","70"));
        Log("getSpecialDeviceInfo mRssiLimit:"+mRssiLimit);
        return (!mBtNamePrefix.isEmpty() && !mBtClass.isEmpty());
    }
    private boolean isSpecialDevice(BluetoothDevice bd) {
        //return bd.getAddress().startsWith(mBtMacPrefix) &&
        //    bd.getBluetoothClass().toString().equals(mBtClass);
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

    private BluetoothAdapter.LeScanCallback mLeScanCallback = new BluetoothAdapter.LeScanCallback() {
        @Override
        public void onLeScan(final BluetoothDevice device, int rssi, byte[] scanRecord) {
            Log("BluetoothDevice = " + device.getName() +
            " Address=" + device.getAddress() +
            " class=" + device.getBluetoothClass().toString() +" rssi:"+(short)rssi );
            // BluetoothDevice btDevice=device;
            final int frssi = rssi;
            mAutoHandler.post( new Runnable() {
                public void run() {
                    if ( (device != null) && isSpecialDevice(device) ) {
                          if (getAverageRssi(rssi) < mRssiLimit) {
                              Log("Scan result isSpecialDevice and in limit rssi value");
                              if (device.getBondState() == BluetoothDevice.BOND_NONE) {
                                  Log("Device no bond!");
                                  Log("Find remoteDevice and parm: name= " + device.getName() +
                                  " Address=" + device.getAddress() +
                                  " class=" + device.getBluetoothClass().toString()+"rssi:"+(short)frssi);
                                  RemoteDevice = mBluetoothAdapter.getRemoteDevice(device.getAddress());
                                  Intent intent = new Intent();
                                  intent.setAction(BluetoothDevice.ACTION_FOUND);
                                  intent.putExtra(BluetoothDevice.EXTRA_DEVICE, RemoteDevice);
                                  intent.putExtra(BluetoothDevice.EXTRA_RSSI, (short)frssi);
                                  intent.putExtra(BluetoothDevice.EXTRA_NAME, device.getName());
                                  intent.putExtra(BluetoothDevice.EXTRA_CLASS, device.getBluetoothClass());
                                  sendBroadcast(intent);
                                  timer.cancel();
                                  mScanFlag=false;
                              } else {
                                  Log("Device has bond");
                              }
                          }
                    }
                }//end run
            }//end post
            );
       }
    };
    private void connected(BluetoothDevice device) throws Exception {
        if ( mService != null && device != null ) {
            Log("Connecting to target: " + device.getAddress());
            try{
                Class clz = Class.forName("android.bluetooth.BluetoothHidHost");
                if (clz.cast(mService) == null) return;
                Method connect = clz.getMethod("connect",BluetoothDevice.class);
                Method setPriority = clz.getMethod("setPriority",BluetoothDevice.class, int.class);
                boolean ret = (boolean)connect.invoke(clz.cast(mService),device);
                if (ret) {
                    setPriority.invoke(clz.cast(mService),device,1000/*BluetoothProfile.PRIORITY_AUTO_CONNECT*/);
                    timer.cancel();
                    showToast(CONNECTING_TO_REMOTE);
                    Log("connect ok and show toast!");
                } else {
                    Log("connect no!");
                }
            } catch(Exception ex) {ex.printStackTrace();}
        } else {
            Log("mService or device no work!");
        }
    }
    static public boolean createBond(Class btClass,BluetoothDevice device) throws Exception {
        Method createBondMethod = btClass.getMethod("createBond");
        Boolean returnValue = (Boolean) createBondMethod.invoke(device);
        return returnValue.booleanValue();
    }
    private boolean initBt(Context context) {
        mContext = context;
        mBluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        if (mBluetoothAdapter == null) {
            Log("No bluetooth device!");
            return false;
        } else {
            Log("Bluetooth device exits!");
            if (!mBluetoothAdapter.isEnabled()) {
                mBluetoothAdapter.enable();
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

    public BluetoothAutoPairService() {
        super("HelloIntentService");
    }
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
                    timer.cancel();
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
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                if (toast != null)
                    toast.cancel();
                toast = Toast.makeText(getApplicationContext(), text, Toast.LENGTH_SHORT);
                toast.show();
                Log("Toast show ");
            }
        });
    }

    private int getAverageRssi(int mRssinew){
        mRssinew = Math.abs(mRssinew);
        mRssi[0] = mRssi[1];
        mRssi[1] = mRssi[2];
        mRssi[2] = mRssinew;
        Log("Average =" + (mRssi[0]+mRssi[1]+mRssi[2])/3);
        return (mRssi[0]+mRssi[1]+mRssi[2])/3;
    }
}
