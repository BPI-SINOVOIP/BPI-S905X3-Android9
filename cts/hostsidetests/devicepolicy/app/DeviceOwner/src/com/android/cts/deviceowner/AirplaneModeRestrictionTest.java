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

package com.android.cts.deviceowner;

import static android.provider.Settings.Global.AIRPLANE_MODE_ON;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.UserManager;
import android.provider.Settings;
import android.provider.Settings.SettingNotFoundException;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

/**
 * Test interaction between {@link UserManager#DISALLOW_AIRPLANE_MODE} user restriction and
 * {@link Settings.Global#AIRPLANE_MODE_ON}.
 */
public class AirplaneModeRestrictionTest extends BaseDeviceOwnerTest {
    private static final int TIMEOUT_SEC = 5;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mDevicePolicyManager.clearUserRestriction(getWho(), UserManager.DISALLOW_AIRPLANE_MODE);
    }

    @Override
    protected void tearDown() throws Exception {
        mDevicePolicyManager.clearUserRestriction(getWho(), UserManager.DISALLOW_AIRPLANE_MODE);
        super.tearDown();
    }

    public void testAirplaneModeTurnedOffWhenRestrictionSet() throws Exception {
        final CountDownLatch latch = new CountDownLatch(1);
        // Using array so that it can be modified in broadcast receiver.
        boolean value[] = new boolean[1];
        BroadcastReceiver receiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                value[0] = intent.getBooleanExtra("state", true);
                latch.countDown();
            }
        };
        mContext.registerReceiver(receiver, new IntentFilter(Intent.ACTION_AIRPLANE_MODE_CHANGED));

        try {
            Settings.Global.putInt(mContext.getContentResolver(), AIRPLANE_MODE_ON, 1);
            mDevicePolicyManager.addUserRestriction(getWho(), UserManager.DISALLOW_AIRPLANE_MODE);
            assertTrue(latch.await(TIMEOUT_SEC, TimeUnit.SECONDS));
            assertFalse(value[0]);
            assertEquals(0, Settings.Global.getInt(
                    mContext.getContentResolver(), Settings.Global.AIRPLANE_MODE_ON));
        } finally {
            mContext.unregisterReceiver(receiver);
        }
    }

    public void testAirplaneModeCannotBeTurnedOnWithRestrictionOn()
            throws SettingNotFoundException {
        mDevicePolicyManager.addUserRestriction(getWho(), UserManager.DISALLOW_AIRPLANE_MODE);
        Settings.Global.putInt(mContext.getContentResolver(), AIRPLANE_MODE_ON, 1);
        assertEquals(0, Settings.Global.getInt(
                mContext.getContentResolver(), AIRPLANE_MODE_ON));
    }

    public void testAirplaneModeCanBeTurnedOnWithRestrictionOff() throws SettingNotFoundException {
        mDevicePolicyManager.clearUserRestriction(getWho(), UserManager.DISALLOW_AIRPLANE_MODE);
        Settings.Global.putInt(mContext.getContentResolver(), AIRPLANE_MODE_ON, 1);
        assertEquals(1, Settings.Global.getInt(
                mContext.getContentResolver(), AIRPLANE_MODE_ON));
    }
}
