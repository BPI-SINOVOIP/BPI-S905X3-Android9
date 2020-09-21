/*
 * Copyright (C) 2018 The Android Open Source Project
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

package com.android.cts.verifier.bluetooth;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothHidDevice;
import android.bluetooth.BluetoothHidDeviceAppQosSettings;
import android.bluetooth.BluetoothHidDeviceAppSdpSettings;
import android.bluetooth.BluetoothProfile;
import android.content.Intent;
import android.os.Bundle;

import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import com.android.cts.verifier.PassFailButtons;
import com.android.cts.verifier.R;

import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class HidDeviceActivity extends PassFailButtons.Activity {
    private static final String TAG = HidDeviceActivity.class.getSimpleName();
    private static final int MSG_APP_STATUS_CHANGED = 0;
    private static final String SDP_NAME = "CtsVerifier";
    private static final String SDP_DESCRIPTION = "CtsVerifier HID Device test";
    private static final String SDP_PROVIDER = "Android";
    private static final int QOS_TOKEN_RATE = 800; // 9 bytes * 1000000 us / 11250 us
    private static final int QOS_TOKEN_BUCKET_SIZE = 9;
    private static final int QOS_PEAK_BANDWIDTH = 0;
    private static final int QOS_LATENCY = 11250;
    static final String SAMPLE_INPUT = "bluetooth";

    private BluetoothAdapter mBluetoothAdapter;
    private BluetoothHidDevice mBluetoothHidDevice;
    private BluetoothDevice mHidHost;
    private ExecutorService mExecutor;

    private Button mRegisterAppButton;
    private Button mMakeDiscoverableButton;
    private Button mUnregisterAppButton;
    private Button mSendReportButton;
    private Button mReplyReportButton;
    private Button mReportErrorButton;

    private BluetoothProfile.ServiceListener mProfileListener =
            new BluetoothProfile.ServiceListener() {
        public void onServiceConnected(int profile, BluetoothProfile proxy) {
            if (profile == BluetoothProfile.HID_DEVICE) {
                mBluetoothHidDevice = (BluetoothHidDevice) proxy;
            }
        }

        public void onServiceDisconnected(int profile) {
            if (profile == BluetoothProfile.HID_DEVICE) {
                mBluetoothHidDevice = null;
            }
        }
    };

    private final BluetoothHidDeviceAppSdpSettings mSdpSettings =
            new BluetoothHidDeviceAppSdpSettings(
                    SDP_NAME,
                    SDP_DESCRIPTION,
                    SDP_PROVIDER,
                    BluetoothHidDevice.SUBCLASS1_COMBO,
                    HidConstants.HIDD_REPORT_DESC);

    private final BluetoothHidDeviceAppQosSettings mOutQos =
            new BluetoothHidDeviceAppQosSettings(
                    BluetoothHidDeviceAppQosSettings.SERVICE_BEST_EFFORT,
                    QOS_TOKEN_RATE,
                    QOS_TOKEN_BUCKET_SIZE,
                    QOS_PEAK_BANDWIDTH,
                    QOS_LATENCY,
                    BluetoothHidDeviceAppQosSettings.MAX);

    private BluetoothHidDevice.Callback mCallback = new BluetoothHidDevice.Callback() {
        @Override
        public void onAppStatusChanged(BluetoothDevice pluggedDevice, boolean registered) {
            Log.d(TAG, "onAppStatusChanged: pluggedDevice=" + pluggedDevice + " registered="
                    + registered);
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.bt_hid_device);
        setPassFailButtonClickListeners();
        setInfoResources(R.string.bt_hid_device_test_name, R.string.bt_hid_device_test_info, -1);

        mBluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        mBluetoothAdapter.getProfileProxy(getApplicationContext(), mProfileListener,
                BluetoothProfile.HID_DEVICE);
        mExecutor = Executors.newSingleThreadExecutor();

        mRegisterAppButton = (Button) findViewById(R.id.bt_hid_device_register_button);
        mRegisterAppButton.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                register();
            }
        });

        mUnregisterAppButton = (Button) findViewById(R.id.bt_hid_device_unregister_button);
        mUnregisterAppButton.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                unregister();
            }
        });

        mMakeDiscoverableButton = (Button) findViewById(R.id.bt_hid_device_discoverable_button);
        mMakeDiscoverableButton.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                makeDiscoverable();
            }
        });

        mSendReportButton = (Button) findViewById(R.id.bt_hid_device_send_report_button);
        mSendReportButton.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                testSendReport();
            }
        });

        mReplyReportButton = (Button) findViewById(R.id.bt_hid_device_reply_report_button);
        mReplyReportButton.setEnabled(false);
        mReplyReportButton.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                testReplyReport();
            }
        });

        mReportErrorButton = (Button) findViewById(R.id.bt_hid_device_report_error_button);
        mReportErrorButton.setEnabled(false);
        mReportErrorButton.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                testReportError();
            }
        });
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        unregister();
    }

    private boolean register() {
        return mBluetoothHidDevice != null
                && mBluetoothHidDevice.registerApp(mSdpSettings, null, mOutQos, mExecutor,
                mCallback);
    }

    private void makeDiscoverable() {
        if (mBluetoothAdapter.getScanMode() !=
                BluetoothAdapter.SCAN_MODE_CONNECTABLE_DISCOVERABLE) {
            Intent intent = new Intent(BluetoothAdapter.ACTION_REQUEST_DISCOVERABLE);
            intent.putExtra(BluetoothAdapter.EXTRA_DISCOVERABLE_DURATION, 30);
            startActivity(intent);
        }
    }

    private boolean unregister() {
        return mBluetoothHidDevice != null && mBluetoothHidDevice.unregisterApp();
    }


    private boolean getConnectedDevice() {
        List<BluetoothDevice> connectedDevices = mBluetoothHidDevice.getConnectedDevices();
        if (connectedDevices.size() == 0) {
            return false;
        } else {
            return false;
        }
    }

    private void testSendReport() {
        if (mHidHost == null) {
            if (mBluetoothHidDevice.getConnectedDevices().size() == 0) {
                Log.w(TAG, "HID host not connected");
                return;
            } else {
                mHidHost = mBluetoothHidDevice.getConnectedDevices().get(0);
                Log.d(TAG, "connected to: " + mHidHost);
            }
        }
        for (char c : SAMPLE_INPUT.toCharArray()) {
            mBluetoothHidDevice.sendReport(mHidHost, BluetoothHidDevice.REPORT_TYPE_INPUT,
                    singleKeyHit(charToKeyCode(c)));
            mBluetoothHidDevice.sendReport(mHidHost, BluetoothHidDevice.REPORT_TYPE_INPUT,
                    singleKeyHit((byte) 0));
        }
        mReplyReportButton.setEnabled(true);

    }

    private void testReplyReport() {
        if (mHidHost == null) {
            if (mBluetoothHidDevice.getConnectedDevices().size() == 0) {
                Log.w(TAG, "HID host not connected");
                return;
            } else {
                mHidHost = mBluetoothHidDevice.getConnectedDevices().get(0);
                Log.d(TAG, "connected to: " + mHidHost);
            }
        }
        if (mBluetoothHidDevice.replyReport(mHidHost, (byte) 0, (byte) 0,
                singleKeyHit((byte) 0))) {
            mReportErrorButton.setEnabled(true);
        }
    }

    private void testReportError() {
        if (mHidHost == null) {
            if (mBluetoothHidDevice.getConnectedDevices().size() == 0) {
                Log.w(TAG, "HID host not connected");
                return;
            } else {
                mHidHost = mBluetoothHidDevice.getConnectedDevices().get(0);
                Log.d(TAG, "connected to: " + mHidHost);
            }
        }
        if (mBluetoothHidDevice.reportError(mHidHost, (byte) 0)) {
            getPassButton().setEnabled(true);
        }
    }


    private byte[] singleKeyHit(byte code) {
        byte[] keyboardData = new byte[8];
        keyboardData[0] = 0;
        keyboardData[1] = 0;
        keyboardData[2] = code;
        keyboardData[3] = 0;
        keyboardData[4] = 0;
        keyboardData[5] = 0;
        keyboardData[6] = 0;
        keyboardData[7] = 0;
        return keyboardData;
    }

    private byte charToKeyCode(char c) {
        if (c < 'a' || c > 'z') {
            return 0;
        }
        return (byte) (c - 'a' + 0x04);
    }
}
