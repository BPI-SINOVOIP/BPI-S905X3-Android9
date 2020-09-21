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

package com.android.settings.fuelgauge.batterytip.detectors;

import static com.google.common.truth.Truth.assertThat;
import static org.mockito.Mockito.spy;

import android.content.ContentResolver;
import android.content.Context;
import android.provider.Settings;

import com.android.settings.fuelgauge.batterytip.BatteryTipPolicy;
import com.android.settings.fuelgauge.batterytip.tips.BatteryTip;
import com.android.settings.testutils.SettingsRobolectricTestRunner;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.MockitoAnnotations;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.util.ReflectionHelpers;

@RunWith(SettingsRobolectricTestRunner.class)
public class SmartBatteryDetectorTest {

    private Context mContext;
    private ContentResolver mContentResolver;
    private SmartBatteryDetector mSmartBatteryDetector;
    private BatteryTipPolicy mPolicy;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);

        mContext = RuntimeEnvironment.application;
        mContentResolver = mContext.getContentResolver();
        mPolicy = spy(new BatteryTipPolicy(mContext));
        mSmartBatteryDetector = new SmartBatteryDetector(mPolicy, mContentResolver);
    }

    @Test
    public void testDetect_testFeatureOn_tipNew() {
        ReflectionHelpers.setField(mPolicy, "testSmartBatteryTip", true);

        assertThat(mSmartBatteryDetector.detect().getState()).isEqualTo(BatteryTip.StateType.NEW);
    }

    @Test
    public void testDetect_smartBatteryOff_tipVisible() {
        Settings.Global.putInt(mContentResolver,
                Settings.Global.ADAPTIVE_BATTERY_MANAGEMENT_ENABLED, 0);

        assertThat(mSmartBatteryDetector.detect().isVisible()).isTrue();
    }

    @Test
    public void testDetect_smartBatteryOn_tipInvisible() {
        Settings.Global.putInt(mContentResolver,
                Settings.Global.ADAPTIVE_BATTERY_MANAGEMENT_ENABLED, 1);

        assertThat(mSmartBatteryDetector.detect().isVisible()).isFalse();
    }
}
