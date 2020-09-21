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

import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.Activity;
import android.arch.lifecycle.LifecycleOwner;
import android.content.Context;
import android.support.v14.preference.PreferenceFragment;
import android.hardware.usb.UsbManager;
import android.hardware.usb.UsbPort;
import android.support.v7.preference.PreferenceManager;
import android.support.v7.preference.PreferenceScreen;

import com.android.settings.R;
import com.android.settings.applications.LayoutPreference;
import com.android.settings.testutils.SettingsRobolectricTestRunner;
import com.android.settings.testutils.shadow.ShadowEntityHeaderController;
import com.android.settings.widget.EntityHeaderController;
import com.android.settingslib.core.lifecycle.Lifecycle;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Answers;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;

@RunWith(SettingsRobolectricTestRunner.class)
@Config(shadows = ShadowEntityHeaderController.class)
public class UsbDetailsHeaderControllerTest {

    private UsbDetailsHeaderController mDetailsHeaderController;
    private Context mContext;
    private Lifecycle mLifecycle;
    private LifecycleOwner mLifecycleOwner;
    private LayoutPreference mPreference;
    private PreferenceManager mPreferenceManager;
    private PreferenceScreen mScreen;

    @Mock
    private UsbBackend mUsbBackend;
    @Mock
    private UsbDetailsFragment mFragment;
    @Mock
    private Activity mActivity;
    @Mock(answer = Answers.RETURNS_DEEP_STUBS)
    private EntityHeaderController mHeaderController;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);

        mContext = RuntimeEnvironment.application;
        mLifecycleOwner = () -> mLifecycle;
        mLifecycle = new Lifecycle(mLifecycleOwner);
        mPreferenceManager = new PreferenceManager(mContext);
        mScreen = mPreferenceManager.createPreferenceScreen(mContext);

        when(mFragment.getActivity()).thenReturn(mActivity);
        when(mActivity.getApplicationContext()).thenReturn(mContext);
        when(mFragment.getContext()).thenReturn(mContext);
        when(mFragment.getPreferenceManager()).thenReturn(mPreferenceManager);
        when(mFragment.getPreferenceScreen()).thenReturn(mScreen);

        ShadowEntityHeaderController.setUseMock(mHeaderController);
        mDetailsHeaderController = new UsbDetailsHeaderController(mContext, mFragment, mUsbBackend);
        mPreference = new LayoutPreference(mContext, R.layout.settings_entity_header);
        mPreference.setKey(mDetailsHeaderController.getPreferenceKey());
        mScreen.addPreference(mPreference);
    }

    @After
    public void tearDown() {
        ShadowEntityHeaderController.reset();
    }

    @Test
    public void displayRefresh_charging_shouldSetHeader() {
        mDetailsHeaderController.displayPreference(mScreen);
        mDetailsHeaderController.refresh(true, UsbManager.FUNCTION_NONE, UsbPort.POWER_ROLE_SINK,
                UsbPort.DATA_ROLE_DEVICE);
        verify(mHeaderController).setLabel(mContext.getString(R.string.usb_pref));
        verify(mHeaderController).setIcon(mContext.getDrawable(R.drawable.ic_usb));
        verify(mHeaderController).done(mActivity, true);
    }
}
