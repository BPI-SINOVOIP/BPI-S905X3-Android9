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

import static com.google.common.truth.Truth.assertThat;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.provider.Settings;
import android.support.v14.preference.SwitchPreference;
import android.support.v7.preference.PreferenceScreen;

import com.android.settings.testutils.SettingsRobolectricTestRunner;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.RuntimeEnvironment;

@RunWith(SettingsRobolectricTestRunner.class)
public class RtlLayoutPreferenceControllerTest {

    @Mock
    private PreferenceScreen mScreen;
    @Mock
    private SwitchPreference mPreference;

    private Context mContext;

    private RtlLayoutPreferenceController mController;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mContext = RuntimeEnvironment.application;
        mController = new RtlLayoutPreferenceController(mContext);
        when(mScreen.findPreference(mController.getPreferenceKey())).thenReturn(mPreference);
        mController.displayPreference(mScreen);
    }

    @Test
    public void updateState_forceRtlEnabled_shouldCheckedPreference() {
        Settings.Global.putInt(mContext.getContentResolver(), Settings.Global.DEVELOPMENT_FORCE_RTL,
                RtlLayoutPreferenceController.SETTING_VALUE_ON);

        mController.updateState(mPreference);

        verify(mPreference).setChecked(true);
    }

    @Test
    public void updateState_forceRtlDisabled_shouldUncheckedPreference() {
        Settings.Global.putInt(mContext.getContentResolver(), Settings.Global.DEVELOPMENT_FORCE_RTL,
                RtlLayoutPreferenceController.SETTING_VALUE_OFF);

        mController.updateState(mPreference);

        verify(mPreference).setChecked(false);
    }

    @Test
    public void onPreferenceChange_preferenceChecked_shouldEnableForceRtl() {
        mController = spy(mController);
        doNothing().when(mController).updateLocales();
        mController.onPreferenceChange(mPreference, true /* new value */);

        final int rtlLayoutMode = Settings.Global.getInt(mContext.getContentResolver(),
                Settings.Global.DEVELOPMENT_FORCE_RTL, -1 /* default */);

        assertThat(rtlLayoutMode).isEqualTo(RtlLayoutPreferenceController.SETTING_VALUE_ON);
    }

    @Test
    public void onPreferenceChange__preferenceUnchecked_shouldDisableForceRtl() {
        mController = spy(mController);
        doNothing().when(mController).updateLocales();
        mController.onPreferenceChange(mPreference, false /* new value */);

        final int rtlLayoutMode = Settings.Global.getInt(mContext.getContentResolver(),
                Settings.Global.DEVELOPMENT_FORCE_RTL, -1 /* default */);

        assertThat(rtlLayoutMode).isEqualTo(RtlLayoutPreferenceController.SETTING_VALUE_OFF);
    }

    @Test
    public void onDeveloperOptionsSwitchDisabled_preferenceShouldBeEnabled() {
        mController = spy(mController);
        doNothing().when(mController).updateLocales();
        mController.onDeveloperOptionsSwitchDisabled();

        final int rtlLayoutMode = Settings.Global.getInt(mContext.getContentResolver(),
                Settings.Global.DEVELOPMENT_FORCE_RTL, -1 /* default */);

        assertThat(rtlLayoutMode).isEqualTo(RtlLayoutPreferenceController.SETTING_VALUE_OFF);
        verify(mPreference).setEnabled(false);
        verify(mPreference).setChecked(false);
    }
}
