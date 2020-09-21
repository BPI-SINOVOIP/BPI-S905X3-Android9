/*
 * Copyright (C) 2016 The Android Open Source Project
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

package com.google.android.car.kitchensink.bluetooth;

import android.Manifest;
import android.annotation.TargetApi;
import android.app.PendingIntent;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothDevicePicker;
import android.bluetooth.BluetoothMapClient;
import android.bluetooth.BluetoothProfile;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.telecom.PhoneAccount;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import com.google.android.car.kitchensink.KitchenSinkActivity;
import com.google.android.car.kitchensink.R;

import java.util.List;

@TargetApi(Build.VERSION_CODES.LOLLIPOP)
public class MapMceTestFragment extends Fragment {
    static final String MESSAGE_TO_SEND = "Im Busy Driving";
    static final String NEW_MESSAGE_TO_SEND = "This is new msg";
    private static final String TAG = "CAR.BLUETOOTH.KS";
    private static final int SEND_SMS_PERMISSIONS_REQUEST = 1;
    BluetoothMapClient mMapProfile;
    BluetoothAdapter mBluetoothAdapter;
    Button mDevicePicker;
    Button mDeviceDisconnect;
    TextView mMessage;
    EditText mOriginator;
    EditText mSmsTelNum;
    TextView mOriginatorDisplayName;
    CheckBox mSent;
    CheckBox mDelivered;
    TextView mBluetoothDevice;
    PendingIntent mSentIntent;
    PendingIntent mDeliveredIntent;
    NotificationReceiver mTransmissionStatusReceiver;
    Object mLock = new Object();
    private KitchenSinkActivity mActivity;
    private Intent mSendIntent;
    private Intent mDeliveryIntent;

    @Override
    public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        View v = inflater.inflate(R.layout.sms_received, container, false);
        mActivity = (KitchenSinkActivity) getHost();
        Button reply = (Button) v.findViewById(R.id.reply);
        Button checkMessages = (Button) v.findViewById(R.id.check_messages);
        mBluetoothDevice = (TextView) v.findViewById(R.id.bluetoothDevice);
        Button sendNewMsg = (Button) v.findViewById(R.id.sms_new_message);
        mSmsTelNum = (EditText) v.findViewById(R.id.sms_tel_num);
        mOriginator = (EditText) v.findViewById(R.id.messageOriginator);
        mOriginatorDisplayName = (TextView) v.findViewById(R.id.messageOriginatorDisplayName);
        mSent = (CheckBox) v.findViewById(R.id.sent_checkbox);
        mDelivered = (CheckBox) v.findViewById(R.id.delivered_checkbox);
        mSendIntent = new Intent(BluetoothMapClient.ACTION_MESSAGE_SENT_SUCCESSFULLY);
        mDeliveryIntent = new Intent(BluetoothMapClient.ACTION_MESSAGE_DELIVERED_SUCCESSFULLY);
        mMessage = (TextView) v.findViewById(R.id.messageContent);
        mDevicePicker = (Button) v.findViewById(R.id.bluetooth_pick_device);
        mDeviceDisconnect = (Button) v.findViewById(R.id.bluetooth_disconnect_device);
        //TODO add manual entry option for phone number
        reply.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                sendMessage(new Uri[]{Uri.parse(mOriginator.getText().toString())},
                        MESSAGE_TO_SEND);
            }
        });

        sendNewMsg.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                String s = mSmsTelNum.getText().toString();
                Toast.makeText(getContext(), "sending msg to " + s, Toast.LENGTH_SHORT).show();
                Uri.Builder builder = new Uri.Builder();
                Uri uri = builder.appendPath(s).scheme(PhoneAccount.SCHEME_TEL).build();
                sendMessage(new Uri[]{uri}, NEW_MESSAGE_TO_SEND);
            }
        });

        checkMessages.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                getMessages();
            }
        });

        // Pick a bluetooth device
        mDevicePicker.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                launchDevicePicker();
            }
        });
        mDeviceDisconnect.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                disconnectDevice(mBluetoothDevice.getText().toString());
            }
        });

        mTransmissionStatusReceiver = new NotificationReceiver();
        return v;
    }

    void launchDevicePicker() {
        IntentFilter filter = new IntentFilter();
        filter.addAction(BluetoothDevicePicker.ACTION_DEVICE_SELECTED);
        getContext().registerReceiver(mPickerReceiver, filter);

        Intent intent = new Intent(BluetoothDevicePicker.ACTION_LAUNCH);
        intent.setFlags(Intent.FLAG_ACTIVITY_EXCLUDE_FROM_RECENTS);
        getContext().startActivity(intent);
    }

    void disconnectDevice(String device) {
        mMapProfile.disconnect(mBluetoothAdapter.getRemoteDevice((device)));
    }

    @Override
    public void onResume() {
        super.onResume();

        mBluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        mBluetoothAdapter.getProfileProxy(getContext(), new MapServiceListener(),
                BluetoothProfile.MAP_CLIENT);

        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(BluetoothMapClient.ACTION_MESSAGE_SENT_SUCCESSFULLY);
        intentFilter.addAction(BluetoothMapClient.ACTION_MESSAGE_DELIVERED_SUCCESSFULLY);
        intentFilter.addAction(BluetoothMapClient.ACTION_MESSAGE_RECEIVED);
        intentFilter.addAction(BluetoothMapClient.ACTION_CONNECTION_STATE_CHANGED);
        getContext().registerReceiver(mTransmissionStatusReceiver, intentFilter);
    }

    @Override
    public void onPause() {
        super.onPause();
        getContext().unregisterReceiver(mTransmissionStatusReceiver);
    }

    private void getMessages() {
        synchronized (mLock) {
            BluetoothDevice remoteDevice;
            try {
                remoteDevice = mBluetoothAdapter.getRemoteDevice(
                        mBluetoothDevice.getText().toString());
            } catch (java.lang.IllegalArgumentException e) {
                Log.e(TAG, e.toString());
                return;
            }

            if (mMapProfile != null) {
                Log.d(TAG, "Getting Messages");
                mMapProfile.getUnreadMessages(remoteDevice);
            }
        }
    }

    private void sendMessage(Uri[] recipients, String message) {
        if (mActivity.checkSelfPermission(Manifest.permission.SEND_SMS)
                != PackageManager.PERMISSION_GRANTED) {
            Log.d(TAG,"Don't have SMS permission in kitchesink app. Requesting it");
            mActivity.requestPermissions(new String[]{Manifest.permission.SEND_SMS},
                    SEND_SMS_PERMISSIONS_REQUEST);
            Toast.makeText(getContext(), "Try again after granting SEND_SMS perm!",
                    Toast.LENGTH_SHORT).show();
            return;
        }
        synchronized (mLock) {
            BluetoothDevice remoteDevice;
            try {
                remoteDevice = mBluetoothAdapter.getRemoteDevice(
                        mBluetoothDevice.getText().toString());
            } catch (java.lang.IllegalArgumentException e) {
                Log.e(TAG, e.toString());
                return;
            }
            mSent.setChecked(false);
            mDelivered.setChecked(false);
            if (mMapProfile != null) {
                Log.d(TAG, "Sending reply");
                if (recipients == null) {
                    Log.d(TAG, "Recipients is null");
                    return;
                }
                if (mBluetoothDevice == null) {
                    Log.d(TAG, "BluetoothDevice is null");
                    return;
                }

                mSentIntent = PendingIntent.getBroadcast(getContext(), 0, mSendIntent,
                        PendingIntent.FLAG_ONE_SHOT);
                mDeliveredIntent = PendingIntent.getBroadcast(getContext(), 0, mDeliveryIntent,
                        PendingIntent.FLAG_ONE_SHOT);
                Log.d(TAG,"Sending message in kitchesink app: " + message);
                mMapProfile.sendMessage(
                        remoteDevice,
                        recipients, message, mSentIntent, mDeliveredIntent);
            }
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions,
            int[] grantResults) {
        Log.d(TAG, "onRequestPermissionsResult reqCode=" + requestCode);
        if (SEND_SMS_PERMISSIONS_REQUEST == requestCode) {
            for (int i=0; i<permissions.length; i++) {
                if (grantResults[i] == PackageManager.PERMISSION_GRANTED) {
                    if (permissions[i] == Manifest.permission.SEND_SMS) {
                        Log.d(TAG, "Got the SEND_SMS perm");
                        return;
                    }
                }
            }
        }
    }

    class MapServiceListener implements BluetoothProfile.ServiceListener {
        @Override
        public void onServiceConnected(int profile, BluetoothProfile proxy) {
            synchronized (mLock) {
                mMapProfile = (BluetoothMapClient) proxy;
                List<BluetoothDevice> connectedDevices = proxy.getConnectedDevices();
                if (connectedDevices.size() > 0) {
                    mBluetoothDevice.setText(connectedDevices.get(0).getAddress());
                }
            }
        }

        @Override
        public void onServiceDisconnected(int profile) {
            synchronized (mLock) {
                mMapProfile = null;
            }
        }
    }

    private class NotificationReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            synchronized (mLock) {
                if (action.equals(BluetoothMapClient.ACTION_CONNECTION_STATE_CHANGED)) {
                    if (intent.getIntExtra(BluetoothProfile.EXTRA_STATE, 0)
                            == BluetoothProfile.STATE_CONNECTED) {
                        mBluetoothDevice.setText(((BluetoothDevice) intent.getParcelableExtra(
                                BluetoothDevice.EXTRA_DEVICE)).getAddress());
                    } else if (intent.getIntExtra(BluetoothProfile.EXTRA_STATE, 0)
                            == BluetoothProfile.STATE_DISCONNECTED) {
                        mBluetoothDevice.setText("Disconnected");
                    }
                } else if (action.equals(BluetoothMapClient.ACTION_MESSAGE_SENT_SUCCESSFULLY)) {
                    mSent.setChecked(true);
                } else if (action.equals(
                        BluetoothMapClient.ACTION_MESSAGE_DELIVERED_SUCCESSFULLY)) {
                    mDelivered.setChecked(true);
                } else if (action.equals(BluetoothMapClient.ACTION_MESSAGE_RECEIVED)) {
                    String senderUri =
                            intent.getStringExtra(BluetoothMapClient.EXTRA_SENDER_CONTACT_URI);
                    if (senderUri == null) {
                        senderUri = "<null>";
                    }

                    String senderName = intent.getStringExtra(
                            BluetoothMapClient.EXTRA_SENDER_CONTACT_NAME);
                    if (senderName == null) {
                        senderName = "<null>";
                    }

                    mMessage.setText(intent.getStringExtra(android.content.Intent.EXTRA_TEXT));
                    mOriginator.setText(senderUri);
                    mOriginatorDisplayName.setText(senderName);
                }
            }
        }
    }

    private final BroadcastReceiver mPickerReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();

            Log.v(TAG, "mPickerReceiver got " + action);

            if (BluetoothDevicePicker.ACTION_DEVICE_SELECTED.equals(action)) {
                BluetoothDevice device = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
                Log.v(TAG, "mPickerReceiver got " + device);
                mMapProfile.connect(device);

                // The receiver can now be disabled.
                getContext().unregisterReceiver(mPickerReceiver);
            }
        }
    };
}
