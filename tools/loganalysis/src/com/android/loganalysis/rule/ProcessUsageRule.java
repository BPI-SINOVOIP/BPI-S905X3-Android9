/*
 * Copyright (C) 2015 The Android Open Source Project
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

import com.android.loganalysis.item.BugreportItem;
import com.android.loganalysis.item.ProcessUsageItem;
import com.android.loganalysis.item.ProcessUsageItem.ProcessUsageInfoItem;
import com.android.loganalysis.item.ProcessUsageItem.SensorInfoItem;

import com.android.loganalysis.util.NumberFormattingUtil;

import java.util.ArrayList;
import java.util.List;

import org.json.JSONException;
import org.json.JSONObject;


/**
 * Rules definition for Process usage
 */
public class ProcessUsageRule extends AbstractPowerRule {

    private static final String ALARM_USAGE_ANALYSIS = "ALARM_USAGE_ANALYSIS";
    private static final String SENSOR_USAGE_ANALYSIS = "SENSOR_USAGE_ANALYSIS";
    private static final long ALARM_THRESHOLD = 60000;
    private static final float SENSOR_ACTIVE_TIME_THRESHOLD_PERCENTAGE = 0.1f; // 10%

    private List<ProcessUsageInfoItem> mOffendingAlarmList;
    private List<ProcessUsageInfoItem> mOffendingSensorList;

    public ProcessUsageRule (BugreportItem bugreportItem) {
        super(bugreportItem);
    }


    @Override
    public void applyRule() {
        mOffendingAlarmList = new ArrayList<ProcessUsageInfoItem>();
        mOffendingSensorList = new ArrayList<ProcessUsageInfoItem>();

        ProcessUsageItem processUsageItem = getDetailedAnalysisItem().getProcessUsageItem();
        if (processUsageItem != null && getTimeOnBattery() > 0) {
            for (ProcessUsageInfoItem usage : processUsageItem.getProcessUsage()) {
                if (usage.getAlarmWakeups() > 0) {
                    addAlarmAnalysis(usage);
                }
                if (usage.getSensorUsage() != null && usage.getSensorUsage().size() > 0) {
                    addSensorAnalysis(usage);
                }
            }
        }
    }

    private void addAlarmAnalysis(ProcessUsageInfoItem usage) {
        final long alarmsPerMs = getTimeOnBattery()/usage.getAlarmWakeups();
        if (alarmsPerMs < ALARM_THRESHOLD) {
            mOffendingAlarmList.add(usage);
        }
    }

    private void addSensorAnalysis(ProcessUsageInfoItem usage) {
        final long sensorUsageThresholdMs = (long) (getTimeOnBattery()
                * SENSOR_ACTIVE_TIME_THRESHOLD_PERCENTAGE);
        for (SensorInfoItem sensorInfo : usage.getSensorUsage()) {
            if (sensorInfo.getUsageDurationMs() > sensorUsageThresholdMs) {
                mOffendingSensorList.add(usage);
            }
        }
    }

    @Override
    public JSONObject getAnalysis() {
        JSONObject usageAnalysis = new JSONObject();
        StringBuilder alarmAnalysis = new StringBuilder();
        if (mOffendingAlarmList == null || mOffendingAlarmList.size() <= 0) {
            alarmAnalysis.append("No apps requested for alarms more frequent than 60 secs.");
        } else {
            for (ProcessUsageInfoItem alarmInfo : mOffendingAlarmList) {
                alarmAnalysis.append(String.format(
                        "UID %s has requested frequent repeating alarms. ",
                        alarmInfo.getProcessUID()));
            }
        }
        StringBuilder sensorAnalysis = new StringBuilder();
        if (mOffendingSensorList == null || mOffendingSensorList.size() <= 0) {
            sensorAnalysis.append("No apps used sensors more than 10% time on battery.");
        } else {
            for (ProcessUsageInfoItem sensorInfo : mOffendingSensorList) {
                for (SensorInfoItem sensors : sensorInfo.getSensorUsage()) {
                    sensorAnalysis.append(String.format("sensor %s was used for %s by UID %s. ",
                            sensors.getSensorName(),
                            NumberFormattingUtil.getDuration(sensors.getUsageDurationMs()),
                            sensorInfo.getProcessUID()));
                }
            }
        }
        try {
            usageAnalysis.put(ALARM_USAGE_ANALYSIS, alarmAnalysis.toString().trim());
            usageAnalysis.put(SENSOR_USAGE_ANALYSIS, sensorAnalysis.toString().trim());
        } catch (JSONException e) {
            // do nothing
        }
        return usageAnalysis;
    }
}
