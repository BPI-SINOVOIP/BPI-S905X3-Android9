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
 * limitations under the License
 */
package com.android.car.settings.quicksettings;


import android.annotation.DrawableRes;
import android.annotation.Nullable;
import android.bluetooth.BluetoothAdapter;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.drawable.Drawable;
import android.view.View;

import com.android.car.settings.R;
import com.android.car.settings.common.Logger;
import com.android.settingslib.bluetooth.LocalBluetoothAdapter;
import com.android.settingslib.bluetooth.LocalBluetoothManager;

/**
 * Controls Bluetooth tile on quick setting page.
 */
public class BluetoothTile implements QuickSettingGridAdapter.Tile {
    private static final Logger LOG = new Logger(BluetoothTile.class);
    private final Context mContext;
    private final StateChangedListener mStateChangedListener;
    private LocalBluetoothAdapter mLocalAdapter;
    private LocalBluetoothManager mLocalManager;

    @DrawableRes
    private int mIconRes = R.drawable.ic_settings_bluetooth;

    private String mText;

    private State mState = State.OFF;

    private final BroadcastReceiver mBtStateReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();

            if (action.equals(BluetoothAdapter.ACTION_STATE_CHANGED)) {
                int state = intent.getIntExtra(BluetoothAdapter.EXTRA_STATE,
                        BluetoothAdapter.ERROR);
                switch (state) {
                    case BluetoothAdapter.STATE_TURNING_OFF:
                        // TODO show a different status icon?
                    case BluetoothAdapter.STATE_OFF:
                        mIconRes = R.drawable.ic_settings_bluetooth_disabled;
                        mState = State.OFF;
                        break;
                    default:
                        mIconRes = R.drawable.ic_settings_bluetooth;
                        mText = mContext.getString(R.string.bluetooth_settings);
                        mState = State.ON;
                }
            } else if (action.equals(BluetoothAdapter.ACTION_CONNECTION_STATE_CHANGED)) {
                int state = intent.getIntExtra(BluetoothAdapter.EXTRA_CONNECTION_STATE,
                        BluetoothAdapter.ERROR);
                switch (state) {
                    case BluetoothAdapter.STATE_CONNECTED:
                        mIconRes = R.drawable.ic_settings_bluetooth_connected;
                        break;
                    case BluetoothAdapter.STATE_DISCONNECTED:
                    default:
                        mIconRes = R.drawable.ic_settings_bluetooth;
                }
            }
            mStateChangedListener.onStateChanged();
        }
    };

    BluetoothTile(Context context, StateChangedListener stateChangedListener) {
        mStateChangedListener = stateChangedListener;
        mContext = context;
        IntentFilter mBtStateChangeFilter = new IntentFilter();
        mBtStateChangeFilter.addAction(BluetoothAdapter.ACTION_STATE_CHANGED);
        mBtStateChangeFilter.addAction(BluetoothAdapter.ACTION_CONNECTION_STATE_CHANGED);
        mContext.registerReceiver(mBtStateReceiver, mBtStateChangeFilter);
        mLocalManager = LocalBluetoothManager.getInstance(
                mContext, /* onInitCallback= */ null);
        if (mLocalManager == null) {
            LOG.e("Bluetooth is not supported on this device");
            return;
        }
        mText = mContext.getString(R.string.bluetooth_settings);
        mLocalAdapter = mLocalManager.getBluetoothAdapter();
        if (mLocalAdapter.isEnabled()) {
            mIconRes = R.drawable.ic_settings_bluetooth;
            mState = State.ON;
        } else {
            mIconRes = R.drawable.ic_settings_bluetooth_disabled;
            mState = State.OFF;
        }
    }

    @Nullable
    public View.OnClickListener getDeepDiveListener() {
        return null;
    }

    @Override
    public boolean isAvailable() {
        return mLocalManager != null;
    }

    @Override
    public Drawable getIcon() {
        return mContext.getDrawable(mIconRes);
    }

    @Override
    @Nullable
    public String getText() {
        // TODO: return connected ssid
        return mText;
    }

    @Override
    public State getState() {
        return mState;
    }

    @Override
    public void stop() {
        mContext.unregisterReceiver(mBtStateReceiver);
    }

    @Override
    public void onClick(View v) {
        if (mLocalAdapter == null) {
            return;
        }
        mLocalAdapter.setBluetoothEnabled(mLocalAdapter.isEnabled() ? false : true);
    }
}
