/*
 * Copyright (C) 2016 The Android Open Source Project
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
package com.android.loganalysis.rule;

import com.android.loganalysis.item.BatteryStatsDetailedInfoItem;
import com.android.loganalysis.item.BugreportItem;
import com.android.loganalysis.item.DumpsysBatteryStatsItem;
import com.android.loganalysis.item.DumpsysItem;
import com.android.loganalysis.item.WakelockItem;
import com.android.loganalysis.item.WakelockItem.WakeLockCategory;

import junit.framework.TestCase;

import org.json.JSONObject;

/**
 * Unit tests for {@link WakelockRule}
 */
public class WakelockRuleTest extends TestCase {

    BugreportItem mBugreport;
    DumpsysItem mDumpsys;
    DumpsysBatteryStatsItem mDumpsysBatteryStats;
    BatteryStatsDetailedInfoItem mBatteryStatsDetailedInfo;

    @Override
    public void setUp() {
        mBugreport =  new BugreportItem();
        mDumpsys = new DumpsysItem();
        mDumpsysBatteryStats = new DumpsysBatteryStatsItem();
        mBatteryStatsDetailedInfo = new BatteryStatsDetailedInfoItem();

        mBatteryStatsDetailedInfo.setTimeOnBattery(3902004);
        mDumpsysBatteryStats.setDetailedBatteryStatsItem(mBatteryStatsDetailedInfo);
        mDumpsys.setBatteryInfo(mDumpsysBatteryStats);
        mBugreport.setDumpsys(mDumpsys);
    }

    /**
     * Test wakelock analysis
     */
    public void testWakelockAnalysis() throws Exception {
        WakelockItem wakelock = new WakelockItem();
        wakelock.addWakeLock("PowerManagerService.WakeLocks", 310006, 2,
                WakeLockCategory.KERNEL_WAKELOCK);
        wakelock.addWakeLock("msm_serial_hs_rx", 133612, 258,
                WakeLockCategory.KERNEL_WAKELOCK);

        wakelock.addWakeLock("ProxyController", "1001", 3887565, 4,
                WakeLockCategory.PARTIAL_WAKELOCK);
        wakelock.addWakeLock("AudioMix", "1013", 1979, 3,
                WakeLockCategory.PARTIAL_WAKELOCK);

        mBatteryStatsDetailedInfo.setWakelockItem(wakelock);
        WakelockRule wakelockRule = new WakelockRule(mBugreport);
        wakelockRule.applyRule();
        JSONObject analysis = wakelockRule.getAnalysis();
        assertNotNull(analysis);
        assertTrue(analysis.has("WAKELOCK_ANALYSIS"));
        assertEquals(analysis.getString("WAKELOCK_ANALYSIS"),
                "ProxyController PARTIAL_WAKELOCK is held for 0d 1h 4m 47s.");
    }

    public void testNoSignificantWakelockAnalysis() throws Exception {
        WakelockItem wakelock = new WakelockItem();
        wakelock.addWakeLock("PowerManagerService.WakeLocks", 310006, 2,
                WakeLockCategory.KERNEL_WAKELOCK);
        wakelock.addWakeLock("msm_serial_hs_rx", 133612, 258,
                WakeLockCategory.KERNEL_WAKELOCK);

        wakelock.addWakeLock("ProxyController", "1001", 287565, 4,
                WakeLockCategory.PARTIAL_WAKELOCK);
        wakelock.addWakeLock("AudioMix", "1013", 1979, 3,
                WakeLockCategory.PARTIAL_WAKELOCK);

        mBatteryStatsDetailedInfo.setWakelockItem(wakelock);
        WakelockRule wakelockRule = new WakelockRule(mBugreport);
        wakelockRule.applyRule();
        JSONObject analysis = wakelockRule.getAnalysis();
        assertNotNull(analysis);
        assertTrue(analysis.has("WAKELOCK_ANALYSIS"));
        assertEquals(analysis.getString("WAKELOCK_ANALYSIS"),
                "No wakelocks were held for more than 10% of time on battery.");
    }

    public void testNoWakelockAnalysis() throws Exception {
        WakelockRule wakelockRule = new WakelockRule(mBugreport);
        wakelockRule.applyRule();
        JSONObject analysis = wakelockRule.getAnalysis();
        assertNotNull(analysis);
        assertTrue(analysis.has("WAKELOCK_ANALYSIS"));
        assertEquals(analysis.getString("WAKELOCK_ANALYSIS"),
                "No wakelocks were held for more than 10% of time on battery.");
    }
}
