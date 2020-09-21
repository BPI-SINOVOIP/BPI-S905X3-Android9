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
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anyString;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.FragmentManager;
import android.app.FragmentTransaction;
import android.view.View;
import android.widget.Button;

import com.android.settings.R;
import com.android.settings.testutils.SettingsRobolectricTestRunner;
import com.android.settings.testutils.shadow.SettingsShadowBluetoothDevice;
import com.android.settings.widget.ActionButtonPreference;
import com.android.settings.widget.ActionButtonPreferenceTest;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;

@RunWith(SettingsRobolectricTestRunner.class)
@Config(shadows = SettingsShadowBluetoothDevice.class)
public class BluetoothDetailsButtonsControllerTest extends BluetoothDetailsControllerTestBase {
    private BluetoothDetailsButtonsController mController;
    private ActionButtonPreference mButtonsPref;
    private Button mConnectButton;
    private Button mForgetButton;

    @Override
    public void setUp() {
        super.setUp();
        final View buttons = View.inflate(
                RuntimeEnvironment.application, R.layout.two_action_buttons, null /* parent */);
        mConnectButton = buttons.findViewById(R.id.button2_positive);
        mForgetButton = buttons.findViewById(R.id.button1_positive);
        mController =
            new BluetoothDetailsButtonsController(mContext, mFragment, mCachedDevice, mLifecycle);
        mButtonsPref = ActionButtonPreferenceTest.createMock();
        when(mButtonsPref.getKey()).thenReturn(mController.getPreferenceKey());
        when(mButtonsPref.setButton2OnClickListener(any(View.OnClickListener.class)))
                .thenAnswer(invocation -> {
                    final Object[] args = invocation.getArguments();
                    mConnectButton.setOnClickListener((View.OnClickListener) args[0]);
                    return mButtonsPref;
                });
        when(mButtonsPref.setButton1OnClickListener(any(View.OnClickListener.class)))
                .thenAnswer(invocation -> {
                    final Object[] args = invocation.getArguments();
                    mForgetButton.setOnClickListener((View.OnClickListener) args[0]);
                    return mButtonsPref;
                });
        mScreen.addPreference(mButtonsPref);
        setupDevice(mDeviceConfig);
        when(mCachedDevice.isBusy()).thenReturn(false);
    }

    @Test
    public void connected() {
        showScreen(mController);

        verify(mButtonsPref).setButton2Text(R.string.bluetooth_device_context_disconnect);
        verify(mButtonsPref).setButton1Text(R.string.forget);
    }

    @Test
    public void clickOnDisconnect() {
        showScreen(mController);
        mConnectButton.callOnClick();

        verify(mCachedDevice).disconnect();
    }

    @Test
    public void clickOnConnect() {
        when(mCachedDevice.isConnected()).thenReturn(false);
        showScreen(mController);

        verify(mButtonsPref).setButton2Text(R.string.bluetooth_device_context_connect);

        mConnectButton.callOnClick();
        verify(mCachedDevice).connect(eq(true));
    }

    @Test
    public void becomeDisconnected() {
        showScreen(mController);
        // By default we start out with the device connected.
        verify(mButtonsPref).setButton2Text(R.string.bluetooth_device_context_disconnect);

        // Now make the device appear to have changed to disconnected.
        when(mCachedDevice.isConnected()).thenReturn(false);
        mController.onDeviceAttributesChanged();
        verify(mButtonsPref).setButton2Text(R.string.bluetooth_device_context_connect);

        // Click the button and make sure that connect (not disconnect) gets called.
        mConnectButton.callOnClick();
        verify(mCachedDevice).connect(eq(true));
    }

    @Test
    public void becomeConnected() {
        // Start out with the device disconnected.
        when(mCachedDevice.isConnected()).thenReturn(false);
        showScreen(mController);

        verify(mButtonsPref).setButton2Text(R.string.bluetooth_device_context_connect);


        // Now make the device appear to have changed to connected.
        when(mCachedDevice.isConnected()).thenReturn(true);
        mController.onDeviceAttributesChanged();
        verify(mButtonsPref).setButton2Text(R.string.bluetooth_device_context_disconnect);

        // Click the button and make sure that disconnnect (not connect) gets called.
        mConnectButton.callOnClick();
        verify(mCachedDevice).disconnect();
    }

    @Test
    public void forgetDialog() {
        showScreen(mController);
        FragmentManager fragmentManager = mock(FragmentManager.class);
        when(mFragment.getFragmentManager()).thenReturn(fragmentManager);
        FragmentTransaction ft = mock(FragmentTransaction.class);
        when(fragmentManager.beginTransaction()).thenReturn(ft);
        mForgetButton.callOnClick();

        ArgumentCaptor<ForgetDeviceDialogFragment> dialogCaptor =
                ArgumentCaptor.forClass(ForgetDeviceDialogFragment.class);
        verify(ft).add(dialogCaptor.capture(), anyString());

        ForgetDeviceDialogFragment dialogFragment = dialogCaptor.getValue();
        assertThat(dialogFragment).isNotNull();
    }

    @Test
    public void startsOutBusy() {
        when(mCachedDevice.isBusy()).thenReturn(true);
        showScreen(mController);

        verify(mButtonsPref).setButton2Text(R.string.bluetooth_device_context_disconnect);
        verify(mButtonsPref).setButton2Enabled(false);
        verify(mButtonsPref).setButton1Text(R.string.forget);

        // Now pretend the device became non-busy.
        when(mCachedDevice.isBusy()).thenReturn(false);
        mController.onDeviceAttributesChanged();

        verify(mButtonsPref).setButton2Enabled(true);
    }

    @Test
    public void becomesBusy() {
        showScreen(mController);
        verify(mButtonsPref).setButton2Enabled(true);

        // Now pretend the device became busy.
        when(mCachedDevice.isBusy()).thenReturn(true);
        mController.onDeviceAttributesChanged();

        verify(mButtonsPref).setButton2Enabled(false);
    }
}
