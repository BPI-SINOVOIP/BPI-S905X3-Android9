/*
 * Copyright (C) 2018 The Android Open Source Project
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
 *
 */

package com.android.settings.fuelgauge.batterysaver;

import static com.google.common.truth.Truth.assertThat;
import static org.mockito.Matchers.anyString;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.spy;

import android.content.Context;
import android.os.PowerManager;
import android.support.v7.preference.PreferenceScreen;
import android.widget.Button;

import com.android.settings.testutils.SettingsRobolectricTestRunner;
import com.android.settings.widget.TwoStateButtonPreference;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;
import org.robolectric.shadows.ShadowPowerManager;
import org.robolectric.util.ReflectionHelpers;

@RunWith(SettingsRobolectricTestRunner.class)
@Config(shadows = ShadowPowerManager.class)
public class BatterySaverButtonPreferenceControllerTest {

    private BatterySaverButtonPreferenceController mController;
    private Context mContext;
    private Button mButtonOn;
    private Button mButtonOff;
    private PowerManager mPowerManager;
    private TwoStateButtonPreference mPreference;

    @Mock
    private PreferenceScreen mPreferenceScreen;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mContext = spy(RuntimeEnvironment.application);
        mButtonOn = new Button(mContext);
        mButtonOff = new Button(mContext);
        mPowerManager = (PowerManager) mContext.getSystemService(Context.POWER_SERVICE);
        mPreference = spy(new TwoStateButtonPreference(mContext, null /* AttributeSet */));
        ReflectionHelpers.setField(mPreference, "mButtonOn", mButtonOn);
        ReflectionHelpers.setField(mPreference, "mButtonOff", mButtonOff);
        doReturn(mPreference).when(mPreferenceScreen).findPreference(anyString());

        mController = new BatterySaverButtonPreferenceController(mContext, "test_key");
        mController.displayPreference(mPreferenceScreen);
    }

    @Test
    public void updateState_lowPowerOn_preferenceIsChecked() {
        mPowerManager.setPowerSaveMode(true);

        mController.updateState(mPreference);

        assertThat(mPreference.isChecked()).isTrue();
    }

    @Test
    public void testUpdateState_lowPowerOff_preferenceIsUnchecked() {
        mPowerManager.setPowerSaveMode(false);

        mController.updateState(mPreference);

        assertThat(mPreference.isChecked()).isFalse();
    }

    @Test
    public void setChecked_on_setPowerSaveMode() {
        mController.setChecked(true);

        assertThat(mPowerManager.isPowerSaveMode()).isTrue();
    }

    @Test
    public void setChecked_off_unsetPowerSaveMode() {
        mController.setChecked(false);

        assertThat(mPowerManager.isPowerSaveMode()).isFalse();
    }
}
