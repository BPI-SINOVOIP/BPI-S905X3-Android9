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
import com.android.loganalysis.item.DumpsysWifiStatsItem;

import junit.framework.TestCase;

import org.json.JSONObject;

/**
 */
public class WifiStatsRuleTest extends TestCase {

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

        mBatteryStatsDetailedInfo.setTimeOnBattery(302004);
        mDumpsysBatteryStats.setDetailedBatteryStatsItem(mBatteryStatsDetailedInfo);
        mDumpsys.setBatteryInfo(mDumpsysBatteryStats);
        mBugreport.setDumpsys(mDumpsys);
    }

    /**
     * Test wifistats analysis
     */
    public void testWifiDisconnectAnalysis() throws Exception {
        DumpsysWifiStatsItem wifiStats = new DumpsysWifiStatsItem();
        wifiStats.setNumWifiDisconnect(1);
        wifiStats.setNumWifiScan(0);
        wifiStats.setNumWifiAssociation(0);

        mDumpsys.setWifiStats(wifiStats);
        WifiStatsRule wifiStatsRule = new WifiStatsRule(mBugreport);
        wifiStatsRule.applyRule();
        JSONObject analysis = wifiStatsRule.getAnalysis();
        assertNotNull(analysis);
        assertTrue(analysis.has("WIFI_STATS"));
        assertEquals(analysis.getString("WIFI_STATS"),
                "No apps requested for frequent wifi scans. Wifi got disconnected 1 times. "
                + "No frequent wifi associations were observed.");
    }

    public void testWifiScanAnalysis() throws Exception {
        DumpsysWifiStatsItem wifiStats = new DumpsysWifiStatsItem();
        wifiStats.setNumWifiDisconnect(0);
        wifiStats.setNumWifiScan(3);
        wifiStats.setNumWifiAssociation(0);

        mDumpsys.setWifiStats(wifiStats);
        WifiStatsRule wifiStatsRule = new WifiStatsRule(mBugreport);
        wifiStatsRule.applyRule();
        JSONObject analysis = wifiStatsRule.getAnalysis();
        assertNotNull(analysis);
        assertTrue(analysis.has("WIFI_STATS"));
        assertEquals(analysis.getString("WIFI_STATS"),
                "Wifi scans happened every 100 seconds. No frequent wifi disconnects were "
                + "observed. No frequent wifi associations were observed.");
    }

    public void testWifiAssociationAnalysis() throws Exception {
        DumpsysWifiStatsItem wifiStats = new DumpsysWifiStatsItem();
        wifiStats.setNumWifiDisconnect(0);
        wifiStats.setNumWifiScan(0);
        wifiStats.setNumWifiAssociation(3);

        mDumpsys.setWifiStats(wifiStats);
        WifiStatsRule wifiStatsRule = new WifiStatsRule(mBugreport);
        wifiStatsRule.applyRule();
        JSONObject analysis = wifiStatsRule.getAnalysis();
        assertNotNull(analysis);
        assertTrue(analysis.has("WIFI_STATS"));
        assertEquals(analysis.getString("WIFI_STATS"),
                "No apps requested for frequent wifi scans. No frequent wifi disconnects were "
                + "observed. Wifi got associated with AP 3 times.");
    }
}
