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
import com.android.loganalysis.item.InterruptItem;
import com.android.loganalysis.item.InterruptItem.InterruptCategory;

import junit.framework.TestCase;

import org.json.JSONObject;

/**
 * Unit tests for {@link InterruptRule}
 */
public class InterruptRuleTest extends TestCase {

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
     * Test interrupt analysis
     */
    public void testInterruptAnalysis() throws Exception {
        InterruptItem interrupt = new InterruptItem();
        interrupt.addInterrupt("2:bcmsdh_sdmmc:2:qcom,smd:2:msmgio", 40,
                InterruptCategory.WIFI_INTERRUPT);
        interrupt.addInterrupt("2:qcom,smd-rpm:2:fc4c.qcom,spmi", 7,
                InterruptCategory.UNKNOWN_INTERRUPT);

        mBatteryStatsDetailedInfo.setInterruptItem(interrupt);

        InterruptRule interruptRule = new InterruptRule(mBugreport);
        interruptRule.applyRule();
        JSONObject analysis = interruptRule.getAnalysis();
        assertNotNull(analysis);
        assertTrue(analysis.has("INTERRUPT_ANALYSIS"));
        assertEquals(analysis.getString("INTERRUPT_ANALYSIS"),
                "Frequent interrupts from WIFI_INTERRUPT (2:bcmsdh_sdmmc:2:qcom,smd:2:msmgio).");
    }


    public void testNoSignificantInterruptAnalysis() throws Exception {
        InterruptItem interrupt = new InterruptItem();
        interrupt.addInterrupt("2:bcmsdh_sdmmc:2:qcom,smd:2:msmgio", 5,
                InterruptCategory.WIFI_INTERRUPT);
        interrupt.addInterrupt("2:qcom,smd-rpm:2:fc4c.qcom,spmi", 7,
                InterruptCategory.UNKNOWN_INTERRUPT);

        mBatteryStatsDetailedInfo.setInterruptItem(interrupt);

        InterruptRule interruptRule = new InterruptRule(mBugreport);
        interruptRule.applyRule();
        JSONObject analysis = interruptRule.getAnalysis();
        assertNotNull(analysis);
        assertTrue(analysis.has("INTERRUPT_ANALYSIS"));
        assertEquals(analysis.getString("INTERRUPT_ANALYSIS"),
                "No interrupts woke up device more frequent than 120 secs.");
    }

    public void testMissingInterruptAnalysis() throws Exception {
        InterruptRule interruptRule = new InterruptRule(mBugreport);
        interruptRule.applyRule();
        JSONObject analysis = interruptRule.getAnalysis();
        assertNotNull(analysis);
        assertTrue(analysis.has("INTERRUPT_ANALYSIS"));
        assertEquals(analysis.getString("INTERRUPT_ANALYSIS"),
                "No interrupts woke up device more frequent than 120 secs.");
    }
}
