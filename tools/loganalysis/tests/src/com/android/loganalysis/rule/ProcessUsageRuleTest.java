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
import com.android.loganalysis.item.ProcessUsageItem;
import com.android.loganalysis.item.ProcessUsageItem.SensorInfoItem;

import java.util.LinkedList;
import junit.framework.TestCase;

import org.json.JSONObject;

/**
 * Unit tests for {@link ProcessUsageRule}
 */
public class ProcessUsageRuleTest extends TestCase {

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
     * Test alarm usage analysis
     */
    public void testAlarmAnalysis() throws Exception {
        ProcessUsageItem processUsage = new ProcessUsageItem();
        LinkedList<SensorInfoItem> uid0Sensor = new LinkedList<SensorInfoItem>();
        uid0Sensor.add(new SensorInfoItem("0", 9908));
        uid0Sensor.add(new SensorInfoItem("1", 9997));

        LinkedList<SensorInfoItem> uidU0a9Sensor = new LinkedList<SensorInfoItem>();
        uidU0a9Sensor.add(new SensorInfoItem("2", 1315));

        processUsage.addProcessUsage("0", 0, uid0Sensor);
        processUsage.addProcessUsage("u0a9", 180, uidU0a9Sensor);
        processUsage.addProcessUsage("u0a8", 0, null);

        mBatteryStatsDetailedInfo.setProcessUsageItem(processUsage);
        ProcessUsageRule usage = new ProcessUsageRule(mBugreport);
        usage.applyRule();
        JSONObject analysis = usage.getAnalysis();
        assertNotNull(analysis);
        assertTrue(analysis.has("ALARM_USAGE_ANALYSIS"));
        assertEquals(analysis.getString("ALARM_USAGE_ANALYSIS"),
                "UID u0a9 has requested frequent repeating alarms.");
        assertTrue(analysis.has("SENSOR_USAGE_ANALYSIS"));
        assertEquals(analysis.getString("SENSOR_USAGE_ANALYSIS"),
                "No apps used sensors more than 10% time on battery.");
    }

    /**
     * Test sensor usage analysis
     */
    public void testSensorAnalysis() throws Exception {
        ProcessUsageItem processUsage = new ProcessUsageItem();
        LinkedList<SensorInfoItem> uid0Sensor = new LinkedList<SensorInfoItem>();
        uid0Sensor.add(new SensorInfoItem("0", 9908));
        uid0Sensor.add(new SensorInfoItem("1", 9997));

        LinkedList<SensorInfoItem> uidU0a9Sensor = new LinkedList<SensorInfoItem>();
        uidU0a9Sensor.add(new SensorInfoItem("2", 913015));

        processUsage.addProcessUsage("0", 0, uid0Sensor);
        processUsage.addProcessUsage("u0a9", 15, uidU0a9Sensor);
        processUsage.addProcessUsage("u0a8", 0, null);

        mBatteryStatsDetailedInfo.setProcessUsageItem(processUsage);
        ProcessUsageRule usage = new ProcessUsageRule(mBugreport);
        usage.applyRule();
        JSONObject analysis = usage.getAnalysis();
        assertNotNull(analysis);
        assertTrue(analysis.has("SENSOR_USAGE_ANALYSIS"));
        assertEquals(analysis.getString("SENSOR_USAGE_ANALYSIS"),
                "sensor 2 was used for 0d 0h 15m 13s by UID u0a9.");

        assertTrue(analysis.has("ALARM_USAGE_ANALYSIS"));
        assertEquals(analysis.getString("ALARM_USAGE_ANALYSIS"),
                "No apps requested for alarms more frequent than 60 secs.");
    }
}
