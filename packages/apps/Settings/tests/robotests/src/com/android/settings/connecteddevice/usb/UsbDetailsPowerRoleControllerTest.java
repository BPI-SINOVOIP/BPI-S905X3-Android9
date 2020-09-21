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
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.Activity;
import android.content.Context;
import android.hardware.usb.UsbManager;
import android.hardware.usb.UsbPort;
import android.os.Handler;
import android.support.v14.preference.SwitchPreference;
import android.support.v7.preference.PreferenceCategory;
import android.support.v7.preference.PreferenceManager;
import android.support.v7.preference.PreferenceScreen;

import com.android.settings.R;
import com.android.settings.testutils.SettingsRobolectricTestRunner;
import com.android.settingslib.core.lifecycle.Lifecycle;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;

@RunWith(SettingsRobolectricTestRunner.class)
public class UsbDetailsPowerRoleControllerTest {

    private UsbDetailsPowerRoleController mDetailsPowerRoleController;
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

        mDetailsPowerRoleController = new UsbDetailsPowerRoleController(mContext, mFragment,
                mUsbBackend);
        mPreference = new PreferenceCategory(mContext);
        mPreference.setKey(mDetailsPowerRoleController.getPreferenceKey());
        mScreen.addPreference(mPreference);

        mDetailsPowerRoleController.mHandler = mHandler;
    }

    @Test
    public void displayRefresh_sink_shouldUncheck() {
        mDetailsPowerRoleController.displayPreference(mScreen);
        when(mUsbBackend.areAllRolesSupported()).thenReturn(true);

        mDetailsPowerRoleController.refresh(true, UsbManager.FUNCTION_NONE, UsbPort.POWER_ROLE_SINK,
                UsbPort.DATA_ROLE_DEVICE);

        SwitchPreference pref = getPreference();
        assertThat(pref.isChecked()).isFalse();
    }

    @Test
    public void displayRefresh_source_shouldCheck() {
        mDetailsPowerRoleController.displayPreference(mScreen);
        when(mUsbBackend.areAllRolesSupported()).thenReturn(true);

        mDetailsPowerRoleController.refresh(true, UsbManager.FUNCTION_NONE,
                UsbPort.POWER_ROLE_SOURCE, UsbPort.DATA_ROLE_HOST);

        SwitchPreference pref = getPreference();
        assertThat(pref.isChecked()).isTrue();
    }

    @Test
    public void displayRefresh_disconnected_shouldDisable() {
        mDetailsPowerRoleController.displayPreference(mScreen);
        when(mUsbBackend.areAllRolesSupported()).thenReturn(true);

        mDetailsPowerRoleController.refresh(false, UsbManager.FUNCTION_NONE,
                UsbPort.POWER_ROLE_SINK, UsbPort.DATA_ROLE_DEVICE);

        assertThat(mPreference.isEnabled()).isFalse();
        assertThat(mScreen.findPreference(mDetailsPowerRoleController.getPreferenceKey()))
                .isEqualTo(mPreference);
    }

    @Test
    public void displayRefresh_notSupported_shouldRemove() {
        mDetailsPowerRoleController.displayPreference(mScreen);
        when(mUsbBackend.areAllRolesSupported()).thenReturn(false);

        mDetailsPowerRoleController.refresh(true, UsbManager.FUNCTION_NONE, UsbPort.POWER_ROLE_SINK,
                UsbPort.DATA_ROLE_DEVICE);

        assertThat(mScreen.findPreference(mDetailsPowerRoleController.getPreferenceKey())).isNull();
    }

    @Test
    public void onClick_sink_shouldSetSource() {
        mDetailsPowerRoleController.displayPreference(mScreen);
        when(mUsbBackend.getPowerRole()).thenReturn(UsbPort.POWER_ROLE_SINK);

        SwitchPreference pref = getPreference();
        pref.performClick();

        verify(mUsbBackend).setPowerRole(UsbPort.POWER_ROLE_SOURCE);
        assertThat(pref.getSummary())
                .isEqualTo(mContext.getString(R.string.usb_switching));
    }

    @Test
    public void onClickTwice_sink_shouldSetSourceOnce() {
        mDetailsPowerRoleController.displayPreference(mScreen);
        when(mUsbBackend.getPowerRole()).thenReturn(UsbPort.POWER_ROLE_SINK);

        SwitchPreference pref = getPreference();
        pref.performClick();

        assertThat(pref.getSummary())
                .isEqualTo(mContext.getString(R.string.usb_switching));
        pref.performClick();
        verify(mUsbBackend, times(1)).setPowerRole(UsbPort.POWER_ROLE_SOURCE);
    }

    @Test
    public void onClickDeviceAndRefresh_success_shouldClearSubtext() {
        mDetailsPowerRoleController.displayPreference(mScreen);
        when(mUsbBackend.getPowerRole()).thenReturn(UsbPort.POWER_ROLE_SINK);

        SwitchPreference pref = getPreference();
        pref.performClick();

        verify(mUsbBackend).setPowerRole(UsbPort.POWER_ROLE_SOURCE);
        assertThat(pref.getSummary())
                .isEqualTo(mContext.getString(R.string.usb_switching));
        mDetailsPowerRoleController.refresh(false /* connected */, UsbManager.FUNCTION_NONE,
                UsbPort.POWER_ROLE_NONE, UsbPort.DATA_ROLE_NONE);
        mDetailsPowerRoleController.refresh(true /* connected */, UsbManager.FUNCTION_NONE,
                UsbPort.POWER_ROLE_SOURCE, UsbPort.DATA_ROLE_DEVICE);
        assertThat(pref.getSummary()).isEqualTo("");
    }

    @Test
    public void onClickDeviceAndRefresh_failed_shouldShowFailureText() {
        mDetailsPowerRoleController.displayPreference(mScreen);
        when(mUsbBackend.getPowerRole()).thenReturn(UsbPort.POWER_ROLE_SINK);

        SwitchPreference pref = getPreference();
        pref.performClick();

        verify(mUsbBackend).setPowerRole(UsbPort.POWER_ROLE_SOURCE);
        assertThat(pref.getSummary())
                .isEqualTo(mContext.getString(R.string.usb_switching));
        mDetailsPowerRoleController.refresh(false /* connected */, UsbManager.FUNCTION_NONE,
                UsbPort.POWER_ROLE_NONE, UsbPort.DATA_ROLE_NONE);
        mDetailsPowerRoleController.refresh(true /* connected */, UsbManager.FUNCTION_NONE,
                UsbPort.POWER_ROLE_SINK, UsbPort.DATA_ROLE_DEVICE);
        assertThat(pref.getSummary())
                .isEqualTo(mContext.getString(R.string.usb_switching_failed));
    }


    @Test
    public void onClickDevice_timedOut_shouldShowFailureText() {
        mDetailsPowerRoleController.displayPreference(mScreen);
        when(mUsbBackend.getPowerRole()).thenReturn(UsbPort.POWER_ROLE_SINK);

        SwitchPreference pref = getPreference();
        pref.performClick();

        verify(mUsbBackend).setPowerRole(UsbPort.POWER_ROLE_SOURCE);
        ArgumentCaptor<Runnable> captor = ArgumentCaptor.forClass(Runnable.class);
        verify(mHandler).postDelayed(captor.capture(), anyLong());
        assertThat(pref.getSummary())
                .isEqualTo(mContext.getString(R.string.usb_switching));
        mDetailsPowerRoleController.refresh(false /* connected */, UsbManager.FUNCTION_NONE,
                UsbPort.POWER_ROLE_NONE, UsbPort.DATA_ROLE_NONE);
        captor.getValue().run();
        assertThat(pref.getSummary())
                .isEqualTo(mContext.getString(R.string.usb_switching_failed));
    }


    private SwitchPreference getPreference() {
        return (SwitchPreference) mPreference.getPreference(0);
    }
}
