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

package com.android.settings.development;

import static com.android.settings.development.HardwareOverlaysPreferenceController.SURFACE_FLINGER_READ_CODE;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyBoolean;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.os.IBinder;
import android.os.RemoteException;
import android.support.v14.preference.SwitchPreference;
import android.support.v7.preference.PreferenceScreen;

import com.android.settings.testutils.SettingsRobolectricTestRunner;
import com.android.settings.testutils.shadow.ShadowParcel;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.annotation.Config;
import org.robolectric.util.ReflectionHelpers;

@RunWith(SettingsRobolectricTestRunner.class)
public class HardwareOverlaysPreferenceControllerTest {

    @Mock
    private Context mContext;
    @Mock
    private PreferenceScreen mScreen;
    @Mock
    private SwitchPreference mPreference;
    @Mock
    private IBinder mSurfaceFlinger;

    private HardwareOverlaysPreferenceController mController;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mController = spy(new HardwareOverlaysPreferenceController(mContext));
        ReflectionHelpers.setField(mController, "mSurfaceFlinger", mSurfaceFlinger);
        doNothing().when(mController).writeHardwareOverlaysSetting(anyBoolean());
        when(mScreen.findPreference(mController.getPreferenceKey())).thenReturn(mPreference);
        mController.displayPreference(mScreen);
    }

    @Test
    public void onPreferenceChange_settingToggledOn_shouldWriteTrueToHardwareOverlaysSetting() {
        mController.onPreferenceChange(mPreference, true /* new value */);

        verify(mController).writeHardwareOverlaysSetting(true);
    }

    @Test
    public void onPreferenceChange_settingToggledOff_shouldWriteFalseToHardwareOverlaysSetting() {
        mController.onPreferenceChange(mPreference, false /* new value */);

        verify(mController).writeHardwareOverlaysSetting(false);
    }

    @Test
    @Config(shadows = {ShadowParcel.class})
    public void updateState_settingEnabled_shouldCheckPreference() throws RemoteException {
        ShadowParcel.sReadIntResult = 1;
        doReturn(true).when(mSurfaceFlinger)
            .transact(eq(SURFACE_FLINGER_READ_CODE), any(), any(), eq(0 /* flags */));
        mController.updateState(mPreference);

        verify(mPreference).setChecked(true);
    }

    @Test
    @Config(shadows = {ShadowParcel.class})
    public void updateState_settingDisabled_shouldUnCheckPreference() throws RemoteException {
        ShadowParcel.sReadIntResult = 0;
        doReturn(true).when(mSurfaceFlinger)
            .transact(eq(SURFACE_FLINGER_READ_CODE), any(), any(), eq(0 /* flags */));
        mController.updateState(mPreference);

        verify(mPreference).setChecked(false);
    }

    @Test
    public void onDeveloperOptionsSwitchDisabled_preferenceChecked_shouldTurnOffPreference() {
        when(mPreference.isChecked()).thenReturn(true);
        mController.onDeveloperOptionsSwitchDisabled();

        verify(mController).writeHardwareOverlaysSetting(false);
        verify(mPreference).setChecked(false);
        verify(mPreference).setEnabled(false);
    }

    @Test
    public void onDeveloperOptionsSwitchDisabled_preferenceUnchecked_shouldNotTurnOffPreference() {
        when(mPreference.isChecked()).thenReturn(false);
        mController.onDeveloperOptionsSwitchDisabled();

        verify(mController, never()).writeHardwareOverlaysSetting(anyBoolean());
        verify(mPreference, never()).setChecked(anyBoolean());
        verify(mPreference).setEnabled(false);
    }
}
