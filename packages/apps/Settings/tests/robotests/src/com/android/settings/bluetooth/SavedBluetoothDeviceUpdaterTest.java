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
 * limitations under the License
 */
package com.android.settings.bluetooth;

import static org.mockito.Matchers.any;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.bluetooth.BluetoothProfile;
import android.bluetooth.BluetoothDevice;
import android.content.Context;

import com.android.settings.connecteddevice.DevicePreferenceCallback;
import com.android.settings.dashboard.DashboardFragment;
import com.android.settings.testutils.SettingsRobolectricTestRunner;
import com.android.settingslib.bluetooth.CachedBluetoothDevice;
import com.android.settingslib.bluetooth.LocalBluetoothManager;
import com.android.settingslib.bluetooth.LocalBluetoothProfileManager;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.RuntimeEnvironment;

@RunWith(SettingsRobolectricTestRunner.class)
public class SavedBluetoothDeviceUpdaterTest {

    @Mock
    private DashboardFragment mDashboardFragment;
    @Mock
    private DevicePreferenceCallback mDevicePreferenceCallback;
    @Mock
    private CachedBluetoothDevice mCachedBluetoothDevice;
    @Mock
    private BluetoothDevice mBluetoothDevice;
    @Mock
    private LocalBluetoothManager mLocalManager;
    @Mock
    private LocalBluetoothProfileManager mLocalBluetoothProfileManager;

    private Context mContext;
    private SavedBluetoothDeviceUpdater mBluetoothDeviceUpdater;
    private BluetoothDevicePreference mPreference;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);

        mContext = RuntimeEnvironment.application;
        doReturn(mContext).when(mDashboardFragment).getContext();
        when(mCachedBluetoothDevice.getDevice()).thenReturn(mBluetoothDevice);
        when(mLocalManager.getProfileManager()).thenReturn(mLocalBluetoothProfileManager);

        mBluetoothDeviceUpdater = spy(new SavedBluetoothDeviceUpdater(mDashboardFragment,
                mDevicePreferenceCallback, mLocalManager));
        mBluetoothDeviceUpdater.setPrefContext(mContext);
        mPreference = new BluetoothDevicePreference(mContext, mCachedBluetoothDevice, false);
        doNothing().when(mBluetoothDeviceUpdater).addPreference(any());
        doNothing().when(mBluetoothDeviceUpdater).removePreference(any());
    }

    @Test
    public void update_filterMatch_addPreference() {
        doReturn(BluetoothDevice.BOND_BONDED).when(mBluetoothDevice).getBondState();
        doReturn(false).when(mBluetoothDevice).isConnected();

        mBluetoothDeviceUpdater.update(mCachedBluetoothDevice);

        verify(mBluetoothDeviceUpdater).addPreference(mCachedBluetoothDevice);
    }

    @Test
    public void update_filterNotMatch_removePreference() {
        doReturn(BluetoothDevice.BOND_NONE).when(mBluetoothDevice).getBondState();
        doReturn(true).when(mBluetoothDevice).isConnected();

        mBluetoothDeviceUpdater.update(mCachedBluetoothDevice);

        verify(mBluetoothDeviceUpdater).removePreference(mCachedBluetoothDevice);
    }

    @Test
    public void onProfileConnectionStateChanged_deviceConnected_removePreference() {
        mBluetoothDeviceUpdater.onProfileConnectionStateChanged(mCachedBluetoothDevice,
                BluetoothProfile.STATE_CONNECTED, BluetoothProfile.A2DP);

        verify(mBluetoothDeviceUpdater).removePreference(mCachedBluetoothDevice);
    }

    @Test
    public void onProfileConnectionStateChanged_deviceDisconnected_addPreference() {
        mBluetoothDeviceUpdater.onProfileConnectionStateChanged(mCachedBluetoothDevice,
                BluetoothProfile.STATE_DISCONNECTED, BluetoothProfile.A2DP);

        verify(mBluetoothDeviceUpdater).addPreference(mCachedBluetoothDevice);
    }

    @Test
    public void onClick_Preference_setConnect() {
        mBluetoothDeviceUpdater.onPreferenceClick(mPreference);

        verify(mCachedBluetoothDevice).connect(true);
    }
}
