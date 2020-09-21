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

package com.android.settings.connecteddevice.usb;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.ArgumentMatchers.anyLong;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.Activity;
import android.content.Context;
import android.hardware.usb.UsbManager;
import android.hardware.usb.UsbPort;
import android.os.Handler;
import android.support.v7.preference.PreferenceCategory;
import android.support.v7.preference.PreferenceManager;
import android.support.v7.preference.PreferenceScreen;

import com.android.settings.R;
import com.android.settings.testutils.SettingsRobolectricTestRunner;
import com.android.settings.widget.RadioButtonPreference;
import com.android.settingslib.core.lifecycle.Lifecycle;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.RuntimeEnvironment;

@RunWith(SettingsRobolectricTestRunner.class)
public class UsbDetailsDataRoleControllerTest {

    private UsbDetailsDataRoleController mDetailsDataRoleController;
    private Context mContext;
    private Lifecycle mLifecycle;
    private PreferenceCategory mPreference;
    private PreferenceManager mPreferenceManager;
    private PreferenceScreen mScreen;

    @Mock
    private UsbBackend mUsbBackend;
    @Mock
    private UsbDetailsFragment mFragment;
    @Mock
    private Activity mActivity;
    @Mock
    private Handler mHandler;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);

        mContext = RuntimeEnvironment.application;
        mLifecycle = new Lifecycle(() -> mLifecycle);
        mPreferenceManager = new PreferenceManager(mContext);
        mScreen = mPreferenceManager.createPreferenceScreen(mContext);

        when(mFragment.getActivity()).thenReturn(mActivity);
        when(mActivity.getApplicationContext()).thenReturn(mContext);
        when(mFragment.getContext()).thenReturn(mContext);
        when(mFragment.getPreferenceManager()).thenReturn(mPreferenceManager);
        when(mFragment.getPreferenceScreen()).thenReturn(mScreen);

        mDetailsDataRoleController = new UsbDetailsDataRoleController(mContext, mFragment,
                mUsbBackend);
        mPreference = new PreferenceCategory(mContext);
        mPreference.setKey(mDetailsDataRoleController.getPreferenceKey());
        mScreen.addPreference(mPreference);

        mDetailsDataRoleController.mHandler = mHandler;
    }

    @Test
    public void displayRefresh_deviceRole_shouldCheckDevice() {
        mDetailsDataRoleController.displayPreference(mScreen);

        mDetailsDataRoleController.refresh(true, UsbManager.FUNCTION_NONE, UsbPort.POWER_ROLE_SINK,
                UsbPort.DATA_ROLE_DEVICE);

        final RadioButtonPreference devicePref = getRadioPreference(UsbPort.DATA_ROLE_DEVICE);
        final RadioButtonPreference hostPref = getRadioPreference(UsbPort.DATA_ROLE_HOST);
        assertThat(devicePref.isChecked()).isTrue();
        assertThat(hostPref.isChecked()).isFalse();
    }

    @Test
    public void displayRefresh_hostRole_shouldCheckHost() {
        mDetailsDataRoleController.displayPreference(mScreen);

        mDetailsDataRoleController.refresh(true, UsbManager.FUNCTION_NONE, UsbPort.POWER_ROLE_SINK,
                UsbPort.DATA_ROLE_HOST);

        final RadioButtonPreference devicePref = getRadioPreference(UsbPort.DATA_ROLE_DEVICE);
        final RadioButtonPreference hostPref = getRadioPreference(UsbPort.DATA_ROLE_HOST);
        assertThat(devicePref.isChecked()).isFalse();
        assertThat(hostPref.isChecked()).isTrue();
    }

    @Test
    public void displayRefresh_disconnected_shouldDisable() {
        mDetailsDataRoleController.displayPreference(mScreen);

        mDetailsDataRoleController.refresh(false, UsbManager.FUNCTION_NONE, UsbPort.POWER_ROLE_SINK,
                UsbPort.DATA_ROLE_DEVICE);

        assertThat(mPreference.isEnabled()).isFalse();
    }

    @Test
    public void onClickDevice_hostEnabled_shouldSetDevice() {
        mDetailsDataRoleController.displayPreference(mScreen);
        when(mUsbBackend.getDataRole()).thenReturn(UsbPort.DATA_ROLE_HOST);

        final RadioButtonPreference devicePref = getRadioPreference(UsbPort.DATA_ROLE_DEVICE);
        devicePref.performClick();

        verify(mUsbBackend).setDataRole(UsbPort.DATA_ROLE_DEVICE);
        assertThat(devicePref.getSummary())
                .isEqualTo(mContext.getString(R.string.usb_switching));
    }

    @Test
    public void onClickDeviceTwice_hostEnabled_shouldSetDeviceOnce() {
        mDetailsDataRoleController.displayPreference(mScreen);
        when(mUsbBackend.getDataRole()).thenReturn(UsbPort.DATA_ROLE_HOST);

        final RadioButtonPreference devicePref = getRadioPreference(UsbPort.DATA_ROLE_DEVICE);
        devicePref.performClick();

        assertThat(devicePref.getSummary())
                .isEqualTo(mContext.getString(R.string.usb_switching));
        devicePref.performClick();
        verify(mUsbBackend).setDataRole(UsbPort.DATA_ROLE_DEVICE);
    }

    @Test
    public void onClickDeviceAndRefresh_success_shouldClearSubtext() {
        mDetailsDataRoleController.displayPreference(mScreen);
        when(mUsbBackend.getDataRole()).thenReturn(UsbPort.DATA_ROLE_HOST);

        final RadioButtonPreference devicePref = getRadioPreference(UsbPort.DATA_ROLE_DEVICE);
        devicePref.performClick();

        verify(mUsbBackend).setDataRole(UsbPort.DATA_ROLE_DEVICE);
        assertThat(devicePref.getSummary())
                .isEqualTo(mContext.getString(R.string.usb_switching));
        mDetailsDataRoleController.refresh(false /* connected */, UsbManager.FUNCTION_NONE,
                UsbPort.POWER_ROLE_NONE, UsbPort.DATA_ROLE_NONE);
        mDetailsDataRoleController.refresh(true /* connected */, UsbManager.FUNCTION_NONE,
                UsbPort.POWER_ROLE_SINK, UsbPort.DATA_ROLE_DEVICE);
        assertThat(devicePref.getSummary()).isEqualTo("");
    }

    @Test
    public void onClickDeviceAndRefresh_failed_shouldShowFailureText() {
        mDetailsDataRoleController.displayPreference(mScreen);
        when(mUsbBackend.getDataRole()).thenReturn(UsbPort.DATA_ROLE_HOST);

        final RadioButtonPreference devicePref = getRadioPreference(UsbPort.DATA_ROLE_DEVICE);
        devicePref.performClick();

        verify(mUsbBackend).setDataRole(UsbPort.DATA_ROLE_DEVICE);
        assertThat(devicePref.getSummary())
                .isEqualTo(mContext.getString(R.string.usb_switching));
        mDetailsDataRoleController.refresh(false /* connected */, UsbManager.FUNCTION_NONE,
                UsbPort.POWER_ROLE_NONE, UsbPort.DATA_ROLE_NONE);
        mDetailsDataRoleController.refresh(true /* connected */, UsbManager.FUNCTION_NONE,
                UsbPort.POWER_ROLE_SINK, UsbPort.DATA_ROLE_HOST);
        assertThat(devicePref.getSummary())
                .isEqualTo(mContext.getString(R.string.usb_switching_failed));
    }

    @Test
    public void onClickDevice_timedOut_shouldShowFailureText() {
        mDetailsDataRoleController.displayPreference(mScreen);
        when(mUsbBackend.getDataRole()).thenReturn(UsbPort.DATA_ROLE_HOST);

        final RadioButtonPreference devicePref = getRadioPreference(UsbPort.DATA_ROLE_DEVICE);
        devicePref.performClick();

        verify(mUsbBackend).setDataRole(UsbPort.DATA_ROLE_DEVICE);
        ArgumentCaptor<Runnable> captor = ArgumentCaptor.forClass(Runnable.class);
        verify(mHandler).postDelayed(captor.capture(), anyLong());
        assertThat(devicePref.getSummary())
                .isEqualTo(mContext.getString(R.string.usb_switching));
        mDetailsDataRoleController.refresh(false /* connected */, UsbManager.FUNCTION_NONE,
                UsbPort.POWER_ROLE_NONE, UsbPort.DATA_ROLE_NONE);
        captor.getValue().run();

        assertThat(devicePref.getSummary())
                .isEqualTo(mContext.getString(R.string.usb_switching_failed));
    }

    private RadioButtonPreference getRadioPreference(int role) {
        return (RadioButtonPreference)
                mPreference.findPreference(UsbBackend.dataRoleToString(role));
    }
}
