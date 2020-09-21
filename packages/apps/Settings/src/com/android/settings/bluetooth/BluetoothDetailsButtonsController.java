/*
 * Copyright (C) 2017 The Android Open Source Project
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

package com.android.settings.bluetooth;

import android.content.Context;
import android.support.v14.preference.PreferenceFragment;
import android.support.v7.preference.PreferenceScreen;

import com.android.settings.R;
import com.android.settings.widget.ActionButtonPreference;
import com.android.settingslib.bluetooth.CachedBluetoothDevice;
import com.android.settingslib.core.lifecycle.Lifecycle;

/**
 * This class adds two buttons: one to connect/disconnect from a device (depending on the current
 * connected state), and one to "forget" (ie unpair) the device.
 */
public class BluetoothDetailsButtonsController extends BluetoothDetailsController {
    private static final String KEY_ACTION_BUTTONS = "action_buttons";
    private boolean mIsConnected;

    private boolean mConnectButtonInitialized;
    private ActionButtonPreference mActionButtons;

    public BluetoothDetailsButtonsController(Context context, PreferenceFragment fragment,
            CachedBluetoothDevice device, Lifecycle lifecycle) {
        super(context, fragment, device, lifecycle);
        mIsConnected = device.isConnected();
    }

    private void onForgetButtonPressed() {
        ForgetDeviceDialogFragment fragment =
                ForgetDeviceDialogFragment.newInstance(mCachedDevice.getAddress());
        fragment.show(mFragment.getFragmentManager(), ForgetDeviceDialogFragment.TAG);
    }

    @Override
    protected void init(PreferenceScreen screen) {
        mActionButtons = ((ActionButtonPreference) screen.findPreference(getPreferenceKey()))
                .setButton1Text(R.string.forget)
                .setButton1OnClickListener((view) -> onForgetButtonPressed())
                .setButton1Positive(false)
                .setButton1Enabled(true);
    }

    @Override
    protected void refresh() {
        mActionButtons.setButton2Enabled(!mCachedDevice.isBusy());

        boolean previouslyConnected = mIsConnected;
        mIsConnected = mCachedDevice.isConnected();
        if (mIsConnected) {
            if (!mConnectButtonInitialized || !previouslyConnected) {
                mActionButtons
                        .setButton2Text(R.string.bluetooth_device_context_disconnect)
                        .setButton2OnClickListener(view -> mCachedDevice.disconnect())
                        .setButton2Positive(false);
                mConnectButtonInitialized = true;
            }
        } else {
            if (!mConnectButtonInitialized || previouslyConnected) {
                mActionButtons
                        .setButton2Text(R.string.bluetooth_device_context_connect)
                        .setButton2OnClickListener(
                                view -> mCachedDevice.connect(true /* connectAllProfiles */))
                        .setButton2Positive(true);
                mConnectButtonInitialized = true;
            }
        }
    }

    @Override
    public String getPreferenceKey() {
        return KEY_ACTION_BUTTONS;
    }

}
