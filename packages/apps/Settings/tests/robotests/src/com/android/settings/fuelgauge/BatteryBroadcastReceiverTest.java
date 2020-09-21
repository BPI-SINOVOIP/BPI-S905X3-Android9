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
package com.android.settings.fuelgauge;

import static com.android.settings.fuelgauge.BatteryBroadcastReceiver.BatteryUpdateType;

import static com.google.common.truth.Truth.assertThat;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anyInt;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import android.content.Context;
import android.content.Intent;
import android.os.BatteryManager;
import android.os.PowerManager;

import com.android.settings.Utils;
import com.android.settings.testutils.SettingsRobolectricTestRunner;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.RuntimeEnvironment;

@RunWith(SettingsRobolectricTestRunner.class)
public class BatteryBroadcastReceiverTest {

    private static final String BATTERY_INIT_LEVEL = "100%";
    private static final String BATTERY_INIT_STATUS = "Not charging";
    private static final int BATTERY_INTENT_LEVEL = 80;
    private static final int BATTERY_INTENT_SCALE = 100;

    @Mock
    private BatteryBroadcastReceiver.OnBatteryChangedListener mBatteryListener;
    private BatteryBroadcastReceiver mBatteryBroadcastReceiver;
    private Context mContext;
    private Intent mChargingIntent;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mContext = spy(RuntimeEnvironment.application);

        mBatteryBroadcastReceiver = new BatteryBroadcastReceiver(mContext);
        mBatteryBroadcastReceiver.mBatteryLevel = BATTERY_INIT_LEVEL;
        mBatteryBroadcastReceiver.mBatteryStatus = BATTERY_INIT_STATUS;
        mBatteryBroadcastReceiver.setBatteryChangedListener(mBatteryListener);

        mChargingIntent = new Intent(Intent.ACTION_BATTERY_CHANGED);
        mChargingIntent.putExtra(BatteryManager.EXTRA_LEVEL, BATTERY_INTENT_LEVEL);
        mChargingIntent.putExtra(BatteryManager.EXTRA_SCALE, BATTERY_INTENT_SCALE);
        mChargingIntent
            .putExtra(BatteryManager.EXTRA_STATUS, BatteryManager.BATTERY_STATUS_CHARGING);
    }

    @Test
    public void testOnReceive_batteryLevelChanged_dataUpdated() {
        mBatteryBroadcastReceiver.onReceive(mContext, mChargingIntent);

        assertThat(mBatteryBroadcastReceiver.mBatteryLevel)
            .isEqualTo(Utils.getBatteryPercentage(mChargingIntent));
        assertThat(mBatteryBroadcastReceiver.mBatteryStatus)
            .isEqualTo(Utils.getBatteryStatus(mContext.getResources(), mChargingIntent));
        verify(mBatteryListener).onBatteryChanged(BatteryUpdateType.BATTERY_LEVEL);
    }

    @Test
    public void testOnReceive_powerSaveModeChanged_listenerInvoked() {
        mBatteryBroadcastReceiver.onReceive(mContext,
                new Intent(PowerManager.ACTION_POWER_SAVE_MODE_CHANGED));

        verify(mBatteryListener).onBatteryChanged(BatteryUpdateType.BATTERY_SAVER);
    }

    @Test
    public void testOnReceive_batteryDataNotChanged_listenerNotInvoked() {
        final String batteryLevel = Utils.getBatteryPercentage(mChargingIntent);
        final String batteryStatus =
            Utils.getBatteryStatus(mContext.getResources(), mChargingIntent);
        mBatteryBroadcastReceiver.mBatteryLevel = batteryLevel;
        mBatteryBroadcastReceiver.mBatteryStatus = batteryStatus;

        mBatteryBroadcastReceiver.onReceive(mContext, mChargingIntent);

        assertThat(mBatteryBroadcastReceiver.mBatteryLevel).isEqualTo(batteryLevel);
        assertThat(mBatteryBroadcastReceiver.mBatteryStatus).isEqualTo(batteryStatus);
        verify(mBatteryListener, never()).onBatteryChanged(anyInt());
    }

    @Test
    public void testRegister_updateBatteryStatus() {
        doReturn(mChargingIntent).when(mContext).registerReceiver(any(), any());

        mBatteryBroadcastReceiver.register();
        mBatteryBroadcastReceiver.register();

        assertThat(mBatteryBroadcastReceiver.mBatteryLevel)
            .isEqualTo(Utils.getBatteryPercentage(mChargingIntent));
        assertThat(mBatteryBroadcastReceiver.mBatteryStatus)
            .isEqualTo(Utils.getBatteryStatus(mContext.getResources(), mChargingIntent));
        // 2 times because register will force update the battery
        verify(mBatteryListener, times(2)).onBatteryChanged(BatteryUpdateType.MANUAL);
    }
}
