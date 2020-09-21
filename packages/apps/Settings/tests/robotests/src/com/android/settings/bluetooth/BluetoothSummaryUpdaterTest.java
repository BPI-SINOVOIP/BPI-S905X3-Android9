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

import static com.google.common.truth.Truth.assertThat;
import static org.mockito.Matchers.anyString;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doCallRealMethod;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.Context;

import com.android.settings.R;
import com.android.settings.testutils.SettingsRobolectricTestRunner;
import com.android.settings.widget.SummaryUpdater.OnSummaryChangeListener;
import com.android.settingslib.bluetooth.LocalBluetoothAdapter;
import com.android.settingslib.bluetooth.LocalBluetoothManager;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Answers;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.RuntimeEnvironment;

import java.util.HashSet;
import java.util.Set;

@RunWith(SettingsRobolectricTestRunner.class)
public class BluetoothSummaryUpdaterTest {

    private static final String DEVICE_NAME = "Nightshade";
    private static final String DEVICE_KEYBOARD_NAME = "Bluetooth Keyboard";

    private Context mContext;
    @Mock(answer = Answers.RETURNS_DEEP_STUBS)
    private LocalBluetoothManager mBluetoothManager;
    @Mock
    private LocalBluetoothAdapter mBtAdapter;
    @Mock
    private BluetoothDevice mConnectedDevice;
    @Mock
    private BluetoothDevice mConnectedKeyBoardDevice;
    @Mock
    private SummaryListener mListener;

    // Disabled by default
    private final boolean[] mAdapterEnabled = {false};
    // Not connected by default
    private final int[] mAdapterConnectionState = {BluetoothAdapter.STATE_DISCONNECTED};
    // Not connected by default
    private final boolean[] mDeviceConnected = {false, false};
    private final Set<BluetoothDevice> mBondedDevices = new HashSet<>();
    private BluetoothSummaryUpdater mSummaryUpdater;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mContext = RuntimeEnvironment.application.getApplicationContext();
        doCallRealMethod().when(mListener).onSummaryChanged(anyString());
        // Setup mock adapter
        when(mBluetoothManager.getBluetoothAdapter()).thenReturn(mBtAdapter);
        doAnswer(invocation -> mAdapterEnabled[0]).when(mBtAdapter).isEnabled();
        doAnswer(invocation -> mAdapterConnectionState[0]).when(mBtAdapter).getConnectionState();
        mSummaryUpdater = new BluetoothSummaryUpdater(mContext, mListener, mBluetoothManager);
        // Setup first device
        doReturn(DEVICE_NAME).when(mConnectedDevice).getName();
        doAnswer(invocation -> mDeviceConnected[0]).when(mConnectedDevice).isConnected();
        // Setup second device
        doReturn(DEVICE_KEYBOARD_NAME).when(mConnectedKeyBoardDevice).getName();
        doAnswer(invocation -> mDeviceConnected[1]).when(mConnectedKeyBoardDevice).isConnected();
        doReturn(mBondedDevices).when(mBtAdapter).getBondedDevices();
    }

    @Test
    public void register_true_shouldRegisterListener() {
        mSummaryUpdater.register(true);

        verify(mBluetoothManager.getEventManager()).registerCallback(mSummaryUpdater);
    }

    @Test
    public void register_false_shouldUnregisterListener() {
        mSummaryUpdater.register(false);

        verify(mBluetoothManager.getEventManager()).unregisterCallback(mSummaryUpdater);
    }

    @Test
    public void register_true_shouldSendSummaryChange() {
        mAdapterEnabled[0] = true;
        mAdapterConnectionState[0] = BluetoothAdapter.STATE_CONNECTED;
        mBondedDevices.add(mConnectedDevice);
        mDeviceConnected[0] = true;

        mSummaryUpdater.register(true);

        verify(mListener).onSummaryChanged(
                mContext.getString(R.string.bluetooth_connected_summary, DEVICE_NAME));
    }

    @Test
    public void onBluetoothStateChanged_btDisabled_shouldSendDisabledSummary() {
        // These states should be ignored
        mAdapterConnectionState[0] = BluetoothAdapter.STATE_CONNECTED;
        mBondedDevices.add(mConnectedDevice);
        mDeviceConnected[0] = true;

        mSummaryUpdater.onBluetoothStateChanged(BluetoothAdapter.STATE_OFF);

        verify(mListener).onSummaryChanged(mContext.getString(R.string.bluetooth_disabled));
    }

    @Test
    public void onBluetoothStateChanged_btEnabled_connected_shouldSendConnectedSummary() {
        mAdapterEnabled[0] = true;
        mAdapterConnectionState[0] = BluetoothAdapter.STATE_CONNECTED;
        mBondedDevices.add(mConnectedDevice);
        mDeviceConnected[0] = true;

        mSummaryUpdater.onBluetoothStateChanged(BluetoothAdapter.STATE_ON);

        verify(mListener).onSummaryChanged(
                mContext.getString(R.string.bluetooth_connected_summary, DEVICE_NAME));
    }

    @Test
    public void onBluetoothStateChanged_btEnabled_connectedMisMatch_shouldSendNotConnected() {
        mAdapterEnabled[0] = true;
        mAdapterConnectionState[0] = BluetoothAdapter.STATE_CONNECTED;
        mBondedDevices.add(mConnectedDevice);
        // State mismatch
        mDeviceConnected[0] = false;

        mSummaryUpdater.onBluetoothStateChanged(BluetoothAdapter.STATE_ON);

        verify(mListener).onSummaryChanged(mContext.getString(R.string.disconnected));
    }

    @Test
    public void onBluetoothStateChanged_btEnabled_notConnected_shouldSendDisconnectedMessage() {
        mAdapterEnabled[0] = true;
        mAdapterConnectionState[0] = BluetoothAdapter.STATE_DISCONNECTED;
        mBondedDevices.add(mConnectedDevice);
        // This should be ignored
        mDeviceConnected[0] = true;

        mSummaryUpdater.onBluetoothStateChanged(BluetoothAdapter.STATE_TURNING_ON);

        verify(mListener).onSummaryChanged(mContext.getString(R.string.disconnected));
    }

    @Test
    public void onBluetoothStateChanged_ConnectedDisabledEnabled_shouldSendDisconnectedSummary() {
        mAdapterEnabled[0] = true;
        mAdapterConnectionState[0] = BluetoothAdapter.STATE_DISCONNECTED;
        mBondedDevices.add(mConnectedDevice);
        mDeviceConnected[0] = false;

        mSummaryUpdater.register(true);
        verify(mListener).onSummaryChanged(mContext.getString(R.string.disconnected));

        mAdapterConnectionState[0] = BluetoothAdapter.STATE_CONNECTED;
        mDeviceConnected[0] = true;
        mSummaryUpdater.onConnectionStateChanged(null /* device */,
                BluetoothAdapter.STATE_CONNECTED);
        verify(mListener).onSummaryChanged(
                mContext.getString(R.string.bluetooth_connected_summary, DEVICE_NAME));

        mAdapterEnabled[0] = false;
        mSummaryUpdater.onBluetoothStateChanged(BluetoothAdapter.STATE_OFF);
        verify(mListener).onSummaryChanged(mContext.getString(R.string.bluetooth_disabled));

        // Turning ON means not enabled
        mSummaryUpdater.onBluetoothStateChanged(BluetoothAdapter.STATE_TURNING_ON);
        // There should still be only one invocation of disabled message
        verify(mListener).onSummaryChanged(mContext.getString(R.string.bluetooth_disabled));

        mAdapterEnabled[0] = true;
        mDeviceConnected[0] = false;
        mSummaryUpdater.onBluetoothStateChanged(BluetoothAdapter.STATE_ON);
        verify(mListener, times(2)).onSummaryChanged(mContext.getString(R.string.disconnected));
        verify(mListener, times(4)).onSummaryChanged(anyString());
    }

    @Test
    public void onConnectionStateChanged_connected_shouldSendConnectedMessage() {
        mAdapterEnabled[0] = true;
        mAdapterConnectionState[0] = BluetoothAdapter.STATE_CONNECTED;
        mBondedDevices.add(mConnectedDevice);
        mDeviceConnected[0] = true;

        mSummaryUpdater.onConnectionStateChanged(null /* device */,
                BluetoothAdapter.STATE_CONNECTED);

        verify(mListener).onSummaryChanged(
                mContext.getString(R.string.bluetooth_connected_summary, DEVICE_NAME));
    }

    @Test
    public void onConnectionStateChanged_inconsistentState_shouldSendDisconnectedMessage() {
        mAdapterEnabled[0] = true;
        mAdapterConnectionState[0] = BluetoothAdapter.STATE_DISCONNECTED;
        mBondedDevices.add(mConnectedDevice);
        mDeviceConnected[0] = false;

        mSummaryUpdater.onConnectionStateChanged(null /* device */,
                BluetoothAdapter.STATE_CONNECTED);

        verify(mListener).onSummaryChanged(mContext.getString(R.string.disconnected));
    }

    @Test
    public void onConnectionStateChanged_noBondedDevice_shouldSendDisconnectedMessage() {
        mAdapterEnabled[0] = true;
        mAdapterConnectionState[0] = BluetoothAdapter.STATE_CONNECTED;

        mSummaryUpdater.onConnectionStateChanged(null /* device */,
                BluetoothAdapter.STATE_CONNECTED);

        verify(mListener).onSummaryChanged(mContext.getString(R.string.disconnected));
    }

    @Test
    public void onConnectionStateChanged_connecting_shouldSendConnectingMessage() {
        // No need for bonded devices
        mAdapterEnabled[0] = true;
        mAdapterConnectionState[0] = BluetoothAdapter.STATE_CONNECTING;

        mSummaryUpdater.onConnectionStateChanged(null /* device */,
                BluetoothAdapter.STATE_CONNECTING);

        verify(mListener).onSummaryChanged(mContext.getString(R.string.bluetooth_connecting));
    }

    @Test
    public void onConnectionStateChanged_disconnecting_shouldSendDisconnectingMessage() {
        // No need for bonded devices
        mAdapterEnabled[0] = true;
        mAdapterConnectionState[0] = BluetoothAdapter.STATE_DISCONNECTING;

        mSummaryUpdater.onConnectionStateChanged(null /* device */,
                BluetoothAdapter.STATE_DISCONNECTING);

        verify(mListener).onSummaryChanged(mContext.getString(R.string.bluetooth_disconnecting));
    }

    @Test
    public void getConnectedDeviceSummary_hasConnectedDevice_returnOneDeviceSummary() {
        mBondedDevices.add(mConnectedDevice);
        mDeviceConnected[0] = true;
        final String expectedSummary =
            mContext.getString(R.string.bluetooth_connected_summary, DEVICE_NAME);

        assertThat(mSummaryUpdater.getConnectedDeviceSummary()).isEqualTo(expectedSummary);
    }

    @Test
    public void getConnectedDeviceSummary_multipleDevices_returnMultipleDevicesSummary() {
        mBondedDevices.add(mConnectedDevice);
        mBondedDevices.add(mConnectedKeyBoardDevice);
        mDeviceConnected[0] = true;
        mDeviceConnected[1] = true;
        final String expectedSummary =
            mContext.getString(R.string.bluetooth_connected_multiple_devices_summary);

        assertThat(mSummaryUpdater.getConnectedDeviceSummary()).isEqualTo(expectedSummary);
    }

    private class SummaryListener implements OnSummaryChangeListener {
        String summary;

        @Override
        public void onSummaryChanged(String summary) {
            this.summary = summary;
        }
    }
}
