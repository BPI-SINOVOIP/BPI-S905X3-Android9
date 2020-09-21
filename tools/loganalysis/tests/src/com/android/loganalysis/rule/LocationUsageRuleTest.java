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

import com.android.loganalysis.item.ActivityServiceItem;
import com.android.loganalysis.item.BatteryStatsDetailedInfoItem;
import com.android.loganalysis.item.BugreportItem;
import com.android.loganalysis.item.DumpsysBatteryStatsItem;
import com.android.loganalysis.item.DumpsysItem;
import com.android.loganalysis.item.LocationDumpsItem;

import junit.framework.TestCase;

import org.json.JSONObject;

/**
 * Unit tests for {@link LocationUsageRule}
 */
public class LocationUsageRuleTest extends TestCase {

    BugreportItem mBugreport;
    DumpsysItem mDumpsys;
    DumpsysBatteryStatsItem mDumpsysBatteryStats;
    BatteryStatsDetailedInfoItem mBatteryStatsDetailedInfo;
    ActivityServiceItem mActivityService;

    @Override
    public void setUp() {
        mBugreport =  new BugreportItem();
        mDumpsys = new DumpsysItem();
        mDumpsysBatteryStats = new DumpsysBatteryStatsItem();
        mBatteryStatsDetailedInfo = new BatteryStatsDetailedInfoItem();
        mActivityService = new ActivityServiceItem();

        mBatteryStatsDetailedInfo.setTimeOnBattery(3902004);
        mDumpsysBatteryStats.setDetailedBatteryStatsItem(mBatteryStatsDetailedInfo);
        mDumpsys.setBatteryInfo(mDumpsysBatteryStats);
        mBugreport.setDumpsys(mDumpsys);
        mBugreport.setActivityService(mActivityService);
    }

    /**
     * Test location usage analysis
     */
    public void testLocationUsageAnalysis() throws Exception {
        LocationDumpsItem location = new LocationDumpsItem();
        location.addLocationClient("com.google.android.gms", 1, 0, 0, "PRIORITY_NO_POWER", 140);
        location.addLocationClient("com.google.android.gms", 5, 5, 5,
                "PRIORITY_BALANCED_POWER_ACCURACY", 140);
        mActivityService.setLocationDumps(location);

        LocationUsageRule locationUsageRule = new LocationUsageRule(mBugreport);
        locationUsageRule.applyRule();
        JSONObject analysis = locationUsageRule.getAnalysis();
        assertNotNull(analysis);
        assertTrue(analysis.has("LOCATION_USAGE_ANALYSIS"));
        assertEquals(analysis.getString("LOCATION_USAGE_ANALYSIS"),
                "Package com.google.android.gms is requesting for location updates every 5 secs "
                + "with priority PRIORITY_BALANCED_POWER_ACCURACY.");
    }

    public void testNoSignificantLocationUsageAnalysis() throws Exception {
        LocationDumpsItem location = new LocationDumpsItem();
        location.addLocationClient("com.google.android.gms", 1, 0, 0, "PRIORITY_NO_POWER", 140);
        location.addLocationClient("com.google.android.gms", 285, 285, 285,
                "PRIORITY_BALANCED_POWER_ACCURACY", 140);
        mActivityService.setLocationDumps(location);

        LocationUsageRule locationUsageRule = new LocationUsageRule(mBugreport);
        locationUsageRule.applyRule();
        JSONObject analysis = locationUsageRule.getAnalysis();
        assertNotNull(analysis);
        assertTrue(analysis.has("LOCATION_USAGE_ANALYSIS"));
        assertEquals(analysis.getString("LOCATION_USAGE_ANALYSIS"),
                "No apps requested for frequent location updates.");
    }

    public void testNoLocationUsageAnalysis() throws Exception {
        LocationUsageRule locationUsageRule = new LocationUsageRule(mBugreport);
        locationUsageRule.applyRule();
        JSONObject analysis = locationUsageRule.getAnalysis();
        assertNotNull(analysis);
        assertTrue(analysis.has("LOCATION_USAGE_ANALYSIS"));
        assertEquals(analysis.getString("LOCATION_USAGE_ANALYSIS"),
                "No apps requested for frequent location updates.");
    }
}
