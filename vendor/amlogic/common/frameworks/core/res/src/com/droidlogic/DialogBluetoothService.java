/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC DialogBluetoothService
 */

package com.droidlogic;

import android.app.Service;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothProfile;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Handler;
import android.os.IBinder;
import android.util.Log;
import android.widget.Toast;

import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.ConcurrentLinkedQueue;
import android.media.AudioManager;

import java.lang.reflect.AccessibleObject;
import java.lang.reflect.Field;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;


import android.bluetooth.BluetoothDevice;
//import android.bluetooth.BluetoothHidHost;

/**
 * Service for managing connection and data communication with a GATT server hosted on a
 * given Bluetooth LE device.
 */
public class DialogBluetoothService extends Service {
    private final static String TAG = "BLE_Service"; // BluetoothLeService.class.getSimpleName();

    // Supported devices GAP names
    private static final String SUPPORTED_DEVICES [] = {
        "DA14582 M&VRemote",
        "DA14582 IR&M&VRemote",
        "DA1458x RCU",
        "RemoteB008",
        "Amlogic_RC"
    };

    public static final int DEVICE_BIT_IN = 0x80000000;
    public static final int DEVICE_IN_WIRED_HEADSET = DEVICE_BIT_IN | 0x10;

    // Time to connect to bonded devices for boot
    private static final int CONNECT_DELAY_MS_BOOT = 100;
    // Time to wait before connecting to bonded devices
    private static final int CONNECTION_DELAY_MS = 1000;
    // Time to wait before canceling a connection attempt
    private static final int CONNECTING_TIMEOUT = 10000;

    // Writing to the client characteristic configuration descriptor of HID Report characteristics
    // may not be necessary, because the Android BLE stack should already have done this.
    private static final boolean WRITE_REPORT_CLIENT_CONFIGURATION = true;

    private static final int STATE_DISCONNECTED = 0;
    private static final int STATE_CONNECTING = 1;
    private static final int STATE_CONNECTED = 2;

    private static final String AUDIO_STREAM_TEXT_ON = "BTAudio stream ON";
    private static final String AUDIO_STREAM_TEXT_OFF = "BTAudio stream OFF";
    private static final String CONNECTING_TO_AUDIO_REMOTE = "Connecting to Audio Remote...";
    private static final String CONNECTED_TO_AUDIO_REMOTE = "Connected to Audio Remote";
    private static final String DISCONNECTED_FROM_AUDIO_REMOTE = "Disconnected from Audio Remote";
    //private static final String CONNECTION_FAILED_MSG = "Connection to Audio Remote failed";

    // Intent Actions and Intent Extra keys to send messages to MainDiaBleActivity
    public final static String ACTION_GATT_CONNECTED        = "com.diasemi.bleconnector.action.GATT_CONNECTED";
    public final static String ACTION_GATT_DISCONNECTED     = "com.diasemi.bleconnector.action.GATT_DISCONNECTED";
    public final static String ACTION_AUDIO_TRANSFER        = "com.diasemi.bleconnector.action.AUDIO_TRANSFER";
    public final static String EXTRA_DATA                   = "com.diasemi.bleconnector.extra.DATA";

    // HID UUIDs
    private static final UUID HID_SERVICE           = UUID.fromString("00001812-0000-1000-8000-00805f9b34fb");   // HID (Human Interface Device) Service
    private static final UUID HID_INFORMATION_CHAR  = UUID.fromString("00002a4a-0000-1000-8000-00805f9b34fb");   // HID Information Characteristic
    private static final UUID HID_REPORT_MAP        = UUID.fromString("00002a4b-0000-1000-8000-00805f9b34fb");   // HID Report Map Characteristic
    private static final UUID HID_CONTROL_POINT     = UUID.fromString("00002a4c-0000-1000-8000-00805f9b34fb");   // HID Control Point Characteristic
    private static final UUID HID_REPORT_CHAR       = UUID.fromString("00002a4d-0000-1000-8000-00805f9b34fb");   // HID Report Characteristic
    // Client Characteristic Configuration Descriptor
    private static final UUID CLIENT_CONFIG_DESCRIPTOR = UUID.fromString("00002902-0000-1000-8000-00805f9b34fb");

    // Ble Remote HID Report Characteristic instance IDs (Audio)
    //hugo modify for huitong rc;
    private static final int  HID_STREAM_ENABLE_WRITE_INSTANCE = 65;//59;
    private static final int  HID_STREAM_ENABLE_READ_INSTANCE  = 61;//62;
    private static final int  HID_STREAM_DATA_MIN_INSTANCE     = 41;//66;
    private static final int  HID_STREAM_DATA_MAX_INSTANCE     = 45;//74;
    private static final int  HID_STREAM_DATA_REP5_INSTANCE    = 53;//66;
    private static final int  HID_STREAM_DATA_REP6_INSTANCE    = 57;//70;
    private static final int  HID_STREAM_DATA_REP7_INSTANCE    = 49;//74;

    // Data
    private BluetoothManager mBluetoothManager = null;
    private BluetoothAdapter mBluetoothAdapter = null;
    private BluetoothGatt mBluetoothGatt = null;
    private int mConnectionState = STATE_DISCONNECTED;
    private HashSet<BluetoothDevice> pending = new HashSet<BluetoothDevice>();
    private Handler mHandler = null;
    private boolean isHidServiceInitialized = false;
    private BluetoothGattCharacteristic enableCharacteristic = null;
    private BluetoothGattCharacteristic rep5Characteristic = null;
    private BluetoothGattCharacteristic rep6Characteristic = null;
    private BluetoothGattCharacteristic rep7Characteristic = null;
    private int prevInstance = 0; // for checking audio data notifications sequence
    private AudioManager mAudioManager = null;
    private int amlrcstateflag  = 0;
    private int connectedState = 0;

    private boolean flagjni = false;
    /**
     * Used in order for the service to be notified about HID devices connection and bond state.
     */
    private final BroadcastReceiver receiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (mBluetoothAdapter == null || mBluetoothManager == null) {
                Log.i(TAG, "Re-initializing bluetooth handlers on service side...");
                if (!initializeBTManager())
                    return;
            }
            if (intent == null)
                return;

            final String action = intent.getAction();
            final BluetoothDevice device = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
            String macAddress = device.getAddress();
            String deviceName = device.getName();
            /*if (BluetoothHidHost.ACTION_CONNECTION_STATE_CHANGED.equals(action)) {
            }
            else */if (BluetoothDevice.ACTION_ACL_CONNECTED.equals(action)) {
                Log.i(TAG, ">ACL LINK CONNECTED ["+device.getName()+"] - checking for supported devices after delay");

                if (isRemoteAudioCapable(device)) {
                    Log.i(TAG, "pending.isEmpty()="+pending.isEmpty());
                    if (!pending.isEmpty()) {
                        mBluetoothGatt = null;
                         mConnectionState = STATE_DISCONNECTED;
                    }
                    pending.add(device);
                    mHandler.removeCallbacks(mConnRunnable);
                    mHandler.postDelayed(mConnRunnable, CONNECTION_DELAY_MS);
                     // waiting some time to not overload BLE device with requests
                }
            }
            else if (BluetoothDevice.ACTION_ACL_DISCONNECT_REQUESTED.equals(action)) {
                Log.i(TAG, ">ACL LINK DISCONNECT REQUEST ["+device.getName()+"]");
            }
            else if (BluetoothDevice.ACTION_ACL_DISCONNECTED.equals(action)) {
                Log.i(TAG, ">ACL LINK DISCONNECTED ["+device.getName()+"]");
                if (isRemoteAudioCapable(device)) {
                    pending.remove(device);
                    if (pending.isEmpty())
                        mHandler.removeCallbacks(mConnRunnable);
                }
            }
            else if (BluetoothDevice.ACTION_BOND_STATE_CHANGED.equals(action)) {
                int bondStateNow = intent.getIntExtra(BluetoothDevice.EXTRA_BOND_STATE, BluetoothDevice.BOND_NONE);
                int bondStatePrev = intent.getIntExtra(BluetoothDevice.EXTRA_PREVIOUS_BOND_STATE, BluetoothDevice.BOND_NONE);

                Log.i(TAG, "BOND STATE CHANGED ["+device.getName()+"] - was " + bondStatePrev + ", now is " + bondStateNow);
                if ((bondStateNow == 10) && isRemoteAudioCapable(device)) {
                    amlrcstateflag=0;
                    Set<BluetoothDevice> bondedDevices = mBluetoothAdapter.getBondedDevices();
                    for (Iterator<BluetoothDevice> re = bondedDevices.iterator(); re.hasNext();) {
                        BluetoothDevice rev = (BluetoothDevice) re.next();
                        if (isRemoteAudioCapable(rev))
                        amlrcstateflag++;
                    }
                    if (amlrcstateflag == 0) {
                        Log.i(TAG, "all amlogic rc remove");
                        connectedState = amlrcstateflag;
                        setWiredDeviceConnectionState(DEVICE_IN_WIRED_HEADSET, connectedState, device.getAddress(), device.getName());
                    }
                }
                  if (isRemoteAudioCapable(device) && bondStatePrev == BluetoothDevice.BOND_BONDING && bondStateNow == BluetoothDevice.BOND_BONDED) {
                     Log.i(TAG, "Bonding complete ["+device.getName()+"] - checking for supported devices after delay");
                    pending.add(device);
                    mHandler.removeCallbacks(mConnRunnable);
                    mHandler.postDelayed(mConnRunnable, CONNECTION_DELAY_MS);
                }
            }
        }
    };

    private Runnable mConnRunnable = new Runnable() {
        @Override
        public void run() {
            if (mConnectionState == STATE_DISCONNECTED && mBluetoothGatt == null) {
                Log. i(TAG, "mConnRunnable, looking on bonded devices in order to find connection target...");
                //pending.clear();
                connectToBondedDevices();
            } else {
                Log.e(TAG, "Ignoring connection attempt. State: " + mConnectionState);
            }
        }
    };

    private Runnable mDisconnRunnable = new Runnable() {
        @Override
        public void run() {
            if (mConnectionState == STATE_CONNECTING && mBluetoothGatt != null) {
                Log.d (TAG, "Connection attempt timeout!");
                mConnectionState = STATE_DISCONNECTED;
                mBluetoothGatt.disconnect();
                close();
            }
        }
    };


    /**
     * Connect to the first found bonded audio remote device
     */
    public void connectToBondedDevices()
    {
        Log.i(TAG, "connectToBondedDevices><");

        if (mBluetoothGatt != null && mConnectionState != STATE_DISCONNECTED) {
            Log.e(TAG, "Already connected to GATT instance. Aborting another connection attempt!");
            return;
        }

        if (mBluetoothAdapter == null) {
            Log.e(TAG, "Adapter not yet initialized, not continuing with connection!");
            return;
        }

        Set<BluetoothDevice> bondedDevices = mBluetoothAdapter.getBondedDevices();
		Log.i(TAG, "bondedDevices size:"+bondedDevices.size());
        for (Iterator<BluetoothDevice> it = bondedDevices.iterator(); it.hasNext();) {
            BluetoothDevice dev = (BluetoothDevice) it.next();
            if (isRemoteAudioCapable(dev)) {
                Log.i( TAG, "Initializing new connection....");
                if (connect(dev)) {
                    if (!flagjni) {
                        //load rc control lib
                        System.loadLibrary("remotecontrol_jni");
                        classInitNative();

                        Log.i(TAG, "start jni task");
                        //start jni task
                        initNative();
                        flagjni=true;
                    }
                    return; // only one audio device
                }
            }
        }
    }

    public boolean isRemoteAudioCapable(BluetoothDevice device)
    {
        String name = (device == null ? null : device.getName());
        String name1 = new String("Amlogic_RC_QW");
        int length = (name == null ? 0 : name.length());
        int length1 = name1.length();
        if (name == null)
            return false;
        else if ((length >= length1) && name.substring(0,length1).equals(name1))
            return false;

        for (String devName : SUPPORTED_DEVICES) {
            if (name.indexOf(devName) != -1) {
                return true;
            }
        }

        return false;
    }


    // Service lifecycle callbacks
    @Override
    public void onCreate() {

        Log.d(TAG, "Service onCreate");

        mHandler = new Handler();
        initializeBTManager();
        mAudioManager = (AudioManager)getSystemService(Context.AUDIO_SERVICE);

        IntentFilter filter = new IntentFilter();
        filter.addAction(BluetoothDevice.ACTION_ACL_CONNECTED);
        filter.addAction(BluetoothDevice.ACTION_ACL_DISCONNECTED);
        filter.addAction(BluetoothDevice.ACTION_ACL_DISCONNECT_REQUESTED);
        filter.addAction(BluetoothDevice.ACTION_BOND_STATE_CHANGED);
        //filter.addAction(BluetoothAdapter.ACTION_STATE_CHANGED);
        //filter.addAction(BluetoothHidHost.ACTION_CONNECTION_STATE_CHANGED);
        registerReceiver(receiver, filter);

        // On service start, check for supported devices
        mHandler.postDelayed(mConnRunnable, CONNECT_DELAY_MS_BOOT);

    }

    @Override
    public void onDestroy() {
        Log.d(TAG, "Service onDestroy");
        mHandler.removeCallbacksAndMessages(null);
        unregisterReceiver(receiver);
        close();
        cleanupNative();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        return START_STICKY;
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }


    /**
     * Initializes a reference to the local Bluetooth adapter.
     *
     * @return Return true if the initialization is successful.
     */
    private boolean initializeBTManager() {
        if (mBluetoothManager == null) {
            mBluetoothManager = (BluetoothManager) getSystemService(Context.BLUETOOTH_SERVICE);
            if (mBluetoothManager == null) {
                Log.e(TAG, "Unable to initialize BluetoothManager.");
                mBluetoothAdapter = null; //also re-setting bluetooth adapter for compatibility
                return false;
            }
        }

        mBluetoothAdapter = mBluetoothManager.getAdapter();
        if (mBluetoothAdapter == null) {
            Log.e(TAG, "Unable to obtain a BluetoothAdapter.");
            return false;
        }

        return true;
    }


    /**
     * Connects to the GATT server hosted on the Bluetooth LE device.
     *
     * @param btDevice The device to connect to.
     *
     * @return Return true if the connection is initiated successfully. The connection result
     *         is reported asynchronously through the
     *
     *         {@code BluetoothGattCallback#onConnectionStateChange(android.bluetooth.BluetoothGatt, int, int)}
     *         callback.
     */
    public boolean connect(BluetoothDevice btDevice) {
        if (btDevice == null) {
            Log.w(TAG, "Device not found.  Unable to connect.");
            return false;
        }

        // Connecting to GATT, not-allowing for auto-reconnect (mess with connection numbers at the same time)
        Log.d(TAG, "Trying to create a new connection...");
        showToast(CONNECTING_TO_AUDIO_REMOTE);
        mHandler.removeCallbacks(mDisconnRunnable);
        mConnectionState = STATE_CONNECTING;
        mBluetoothGatt = btDevice.connectGatt(this, false, mGattCallback);
        if (mBluetoothGatt == null) {
            Log.e(TAG, "btDevice connectGatt returned NULL! Aborting connect!");
            mConnectionState = STATE_DISCONNECTED;
            return false;
        }

        mHandler.postDelayed(mDisconnRunnable, CONNECTING_TIMEOUT);
        return true;
    }

    private void onConnectionSuccess(BluetoothGatt gatt) {
        if (mConnectionState == STATE_CONNECTED) {
            Log.e(TAG, "Already connected. Ignoring attempt to discover services again.");
            return;
        }

        if (mBluetoothGatt == null) {
            Log.e(TAG, "BluetoothGatt is NULL. Fixing that.");
            mBluetoothGatt = gatt;
        }

        mConnectionState = STATE_CONNECTED;
        broadcastUpdate(ACTION_GATT_CONNECTED);
        Log.i(TAG, "Connected to GATT server.");
        Log.i(TAG, "Starting service discovery...");
        mBluetoothGatt.discoverServices();
    }

    /**
     * After using a given BLE device, the app must call this method to ensure resources are
     * released properly.
     */
    public void close() {
        Log.w(TAG, "Close called!");
        if (mBluetoothGatt != null) {
            mBluetoothGatt.close();
        }
        mBluetoothGatt = null;
        enableCharacteristic = null;
        rep5Characteristic = null;
        rep6Characteristic = null;
        rep7Characteristic = null;
    }


    /****
     * Send enable report back to the Remote Control Unit, e.g. to enable/disable streaming.
     * @param flag Data to send to Remote Control Unit in order to enable/disable audio stream.
     */
    private void sendEnable(int flag) {
        if (enableCharacteristic == null)
            return;
        Log.i(TAG,"Write Enable: " + flag);

        /* masked by hugo
        if (flag != 0) {
            showToast(AUDIO_STREAM_TEXT_ON);
            broadcastUpdate(ACTION_AUDIO_TRANSFER, AUDIO_STREAM_TEXT_ON);
        } else {
            showToast(AUDIO_STREAM_TEXT_OFF);
            broadcastUpdate(ACTION_AUDIO_TRANSFER, AUDIO_STREAM_TEXT_OFF);
        }*/

        prevInstance = 0;
        byte packet[] = new byte[1];
        packet[0] = (byte) flag;
        enableCharacteristic.setValue(packet);
        mBluetoothGatt.writeCharacteristic(enableCharacteristic);
    }


    /**************************************************************************************
     * BluetoothGattCallback
     * Implements callback methods for GATT events that the app cares about.
     * For example, connection change and services discovered.
     */
    private final BluetoothGattCallback mGattCallback = new BluetoothGattCallback() {
        @Override
        public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {
            Log.i(TAG, "Connection state change: " + (newState == BluetoothGatt.STATE_CONNECTED ? "connected" : "disconnected") + ", status=" + status);

            if (newState == BluetoothProfile.STATE_CONNECTED) {
                // A ttempt to discover services after successful connection.
                onConnectionSuccess(gatt);
            }
            else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                if (gatt != mBluetoothGatt) {
                    Log.e(TAG, "Disconnected different instance of gatt. No change for main gatt session");
                    return;
                }

                Log.i(TAG, "Disconnected from GATT server.");
                mConnectionState = STATE_DISCONNECTED;
                isHidServiceInitialized = false;
                flagjni=false;
                close();
                showToast(DISCONNECTED_FROM_AUDIO_REMOTE);
                broadcastUpdate(ACTION_GATT_DISCONNECTED);
            }
        }

        @Override
        public void onServicesDiscovered(BluetoothGatt gatt, int status) {
            Log.i(TAG, "Services discovered.");
            if (status == BluetoothGatt.GATT_SUCCESS) {
                if (!isHidServiceInitialized) {
                    initHidService(gatt);
                    isHidServiceInitialized = true;
                    showToast(CONNECTED_TO_AUDIO_REMOTE);
                } else {
                    Log.e(TAG, "HID service already initialized!");
                }
            } else {
                Log.w(TAG, "onServicesDiscovered received: " + status + ". Disconnecting from GATT.");
                gatt.disconnect();
            }
        }

        @Override
        public void onCharacteristicRead(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, int status) {
            Log.i(TAG,"OnRead "+characteristic.getUuid().toString() + ", id=" + characteristic.getInstanceId() + ", status=" + status);
        }

        @Override
        public void onCharacteristicWrite(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, int status) {
            Log.i(TAG,"OnWrite "+characteristic.getUuid().toString() + ", id=" + characteristic.getInstanceId() + ", status=" + status);
        }

        @Override
        public void onCharacteristicChanged(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic) {
            int instance = characteristic.getInstanceId();
            // Audio data report
            if ((HID_REPORT_CHAR.compareTo(characteristic.getUuid()) == 0) && (instance >= HID_STREAM_DATA_MIN_INSTANCE) && (instance <= HID_STREAM_DATA_MAX_INSTANCE)) {
                //byte packet[] = characteristic.getValue();
                //int n = ((packet[0])&0xFF) + ((packet[1]&0xFF)<<8);
                //Log.i(TAG,String.format("HID data packet %d %02x %02x %d", instance, packet[0]&0xFF, packet[1]&0xFF, n));

                if (prevInstance == 0)
                    prevInstance = instance;
                else
                    ++prevInstance;
                if (prevInstance > HID_STREAM_DATA_MAX_INSTANCE)
                    prevInstance = HID_STREAM_DATA_MIN_INSTANCE;
                if (prevInstance != instance) {
                    Log.w(TAG,String.format("Error, packet sequence interruption, expected %d, received %d", prevInstance, instance));
                    prevInstance = instance;
                }
            }
            // Possible audio button report
            else if ((HID_REPORT_CHAR.compareTo(characteristic.getUuid()) == 0) && (instance == HID_STREAM_ENABLE_READ_INSTANCE)) {
                byte[] value = characteristic.getValue();
                Log.i(TAG,"rx key: "+ String.format("%x %x", value[0], value[1]));
                if (value[0] == 0x21 && value[1] == 0x02) {
                    //sendEnable(0x01);
                }
                else if (value[0] == 0x42 && value[1] == 0x00) {
                    //sendEnable(0x00);
                }
            }
        }

        @Override
        public void onDescriptorRead(BluetoothGatt gatt, BluetoothGattDescriptor descriptor, int status) {
            Log.i(TAG, "onDescriptorRead " + descriptor.getUuid()
                    + ", char " + descriptor.getCharacteristic().getUuid().toString() + ", id=" + descriptor.getCharacteristic().getInstanceId()
                    + ", status=" +  status);
        }

        @Override
        public void onDescriptorWrite(BluetoothGatt gatt, BluetoothGattDescriptor descriptor, int status) {
            Log.i(TAG, "onDescriptorWrite " + descriptor.getUuid()
                    + ", char " + descriptor.getCharacteristic().getUuid().toString() + ", id=" + descriptor.getCharacteristic().getInstanceId()
                    + ", status=" +  status);
            if (!mDescWriteQueue.isEmpty()) {
                gatt.writeDescriptor(mDescWriteQueue.poll());
            }
        }

        @Override
        public void onReadRemoteRssi(BluetoothGatt gatt, int rssi, int status) {
            Log.d(TAG, "Remote RSSI: " + rssi);
        }

        /**
         * Initialize HID service characteristics and enable notifications.
         *
         * @param gatt Gatt connection to use.
         */
        private void initHidService(BluetoothGatt gatt) {
            BluetoothGattService gattService;

            // Loop through available GATT Services.
            List<BluetoothGattService> serviceList =  gatt.getServices();
            for (BluetoothGattService s : serviceList) {
                Log.i(TAG,"Service "+s.getInstanceId()+" "+s.getUuid().toString());
            }

            // Find HID Service
            gattService = gatt.getService(HID_SERVICE);
            if (gattService == null)
                return;

            String uuid = gattService.getUuid().toString();
            int gsInstance = gattService.getInstanceId();

            Log.i(TAG, "Service UUID " + uuid + ", id=" + gsInstance);
            List<BluetoothGattCharacteristic> gattCharacteristics = gattService.getCharacteristics();

            // Loop through available Characteristics.
            for (BluetoothGattCharacteristic gattCharacteristic : gattCharacteristics) {
                boolean setNotify = false;
                UUID uuidc = gattCharacteristic.getUuid();
                int instance = gattCharacteristic.getInstanceId();
                // Find the enable, rep5, rep6, rep7 characteristics.
                if (uuidc.compareTo(HID_REPORT_CHAR) == 0) {
                    switch (instance) {
                    case HID_STREAM_ENABLE_WRITE_INSTANCE:
                        enableCharacteristic = gattCharacteristic;
                        Log.i(TAG, "Found Enable Write Characteristic");
                        break;
                    case HID_STREAM_ENABLE_READ_INSTANCE:
                        Log.i(TAG, "Found Enable Read Characteristic");
                        setNotify = true;
                        break;
                    case HID_STREAM_DATA_REP5_INSTANCE:
                        rep5Characteristic = gattCharacteristic;
                        Log.i(TAG, "Found REP5 Characteristic");
                        setNotify = true;
                        break;
                    case HID_STREAM_DATA_REP6_INSTANCE:
                        rep6Characteristic = gattCharacteristic;
                        Log.i(TAG, "Found REP6 Characteristic");
                        setNotify = true;
                        break;
                    case HID_STREAM_DATA_REP7_INSTANCE:
                        rep7Characteristic = gattCharacteristic;
                        Log.i(TAG, "Found REP7 Characteristic");
                        setNotify = true;
                        break;
                    }
                }
                Log.i(TAG, "Characteristic " + uuidc.toString() + ", id=" + instance + ", prop=" + gattCharacteristic.getProperties());

                // Only setNotify for our HID streaming characteristics. Some of the Android devices don't support
                // more than 4 local notifications (Nexus-5, Nexus-7 - API 19), although Samsung S5 does.
                if (setNotify) {
                    //Enable local notifications
                    gatt.setCharacteristicNotification(gattCharacteristic, true);
                    // Enable remote notifications
                    if (WRITE_REPORT_CLIENT_CONFIGURATION) {
                        BluetoothGattDescriptor ccc = gattCharacteristic.getDescriptor(CLIENT_CONFIG_DESCRIPTOR);
                        if (ccc != null) {
                            Log.i(TAG, "Found client configuration descriptor for report ID " + ccc.getCharacteristic().getInstanceId());
                            ccc.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);
                            mDescWriteQueue.add(ccc);
                        }
                    }
                }
            }

            if (!mDescWriteQueue.isEmpty()) {
                gatt.writeDescriptor(mDescWriteQueue.poll());
            }
        }

        // Descriptor write queue (needed because there can't be more than one pending gatt operations)
        ConcurrentLinkedQueue<BluetoothGattDescriptor> mDescWriteQueue = new ConcurrentLinkedQueue<BluetoothGattDescriptor>();

    };

    void onSearchStateChanged(int status) {
        Log.d(TAG, "onSearchStateChanged: " + status);
        sendEnable(status == 0 ? 0 : 1);
    }

    void onDeviceStateChanged(int status) {
        Log.d(TAG, "onDeviceStateChanged: status=" + status + ",connectedState="+connectedState);
        amlrcstateflag = 0;
        if (connectedState == status) return;
        //find the audio device
        Set<BluetoothDevice> bondedDevices = mBluetoothAdapter.getBondedDevices();
        for (Iterator<BluetoothDevice> it = bondedDevices.iterator(); it.hasNext();) {
            BluetoothDevice dev = (BluetoothDevice) it.next();
            if (isRemoteAudioCapable(dev) && (status == 1)) {
            // @hidden-api-issue-start
                Log.i(TAG, "amlogic rc input");
                connectedState = status;
                //mAudioManager.setWiredDeviceConnectionState(AudioManager.DEVICE_IN_WIRED_HEADSET, connectedState, dev.getAddress(), dev.getName());
                setWiredDeviceConnectionState(DEVICE_IN_WIRED_HEADSET, connectedState, dev.getAddress(), dev.getName());
           // @hidden-api-issue-end
                break;
            }
        }
    }

    private void setWiredDeviceConnectionState(int type, int state, String address, String name) {
        try {
            Class<?> audioManager = Class.forName("android.media.AudioManager");
            Method setwireState = audioManager.getMethod("setWiredDeviceConnectionState",
                                    int.class, int.class, String.class, String.class);

            setwireState.invoke(mAudioManager, type, state, address, name);

        } catch(ClassNotFoundException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        } catch (NoSuchMethodException ex) {
            ex.printStackTrace();
        } catch (IllegalArgumentException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
    }


    /*****
     * broadcastUpdate functions to send data back to BleActivity
     */

    private void broadcastUpdate(final String action) {
        final Intent intent = new Intent(action);
        sendBroadcast(intent);
    }

    private void broadcastUpdate(final String action, String data) {
        final Intent intent = new Intent(action);
        intent.putExtra(EXTRA_DATA, data);
        sendBroadcast(intent);
    }


    private Toast toast; // Cancel last toast in order to show a new one

    private void showToast(final String text) {
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                if (toast != null)
                    toast.cancel();
                toast = Toast.makeText(getApplicationContext(), text, Toast.LENGTH_SHORT);
                toast.show();
            }
        });
    }

     //native jni
     private native static void classInitNative();
     private native boolean initNative();
     private native boolean cleanupNative();

}

