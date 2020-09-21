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
import android.content.Intent;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Log;

import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;

import com.android.cts.verifier.PassFailButtons;
import com.android.cts.verifier.R;
import com.android.cts.verifier.TestResult;

public class HidHostActivity extends PassFailButtons.Activity {
    private static final String TAG = HidHostActivity.class.getSimpleName();

    BluetoothAdapter mBluetoothAdapter;

    private static final int ENABLE_BLUETOOTH_REQUEST = 1;
    private static final int PICK_SERVER_DEVICE_REQUEST = 2;

    private String mDeviceAddress;

    private Button mPickDeviceButton;
    private EditText mEditText;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.bt_hid_host);
        setPassFailButtonClickListeners();
        setInfoResources(R.string.bt_hid_host_test_name, R.string.bt_hid_host_test_info, -1);
        mBluetoothAdapter = BluetoothAdapter.getDefaultAdapter();

        mEditText = (EditText) findViewById(R.id.bt_hid_host_edit_text);
        mEditText.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {}

            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {
                if (s.toString().equals(HidDeviceActivity.SAMPLE_INPUT)) {
                    getPassButton().setEnabled(true);
                }
            }

            @Override
            public void afterTextChanged(Editable s) {}
        });
        mPickDeviceButton = (Button) findViewById(R.id.bt_hid_host_pick_device_button);
        mPickDeviceButton.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = new Intent(HidHostActivity.this,
                        DevicePickerActivity.class);
                startActivityForResult(intent, PICK_SERVER_DEVICE_REQUEST);
                mEditText.requestFocus();
            }
        });
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        switch (requestCode) {
            case PICK_SERVER_DEVICE_REQUEST:
                Log.d(TAG, "pick device: " + resultCode);
                if (resultCode == RESULT_OK) {
                    mDeviceAddress = data.getStringExtra(DevicePickerActivity.EXTRA_DEVICE_ADDRESS);
                    connectToDevice();
                } else {
                    setResult(RESULT_CANCELED);
                    finish();
                }
                break;
        }
    }

    private boolean connectToDevice() {
        BluetoothDevice bluetoothDevice = mBluetoothAdapter.getRemoteDevice(mDeviceAddress);
        bluetoothDevice.createBond();
        return true;
    }
}
