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
package com.android.loganalysis.parser;

import com.android.loganalysis.item.BatteryDischargeItem;
import com.android.loganalysis.item.BatteryDischargeItem.BatteryDischargeInfoItem;
import com.android.loganalysis.item.BatteryStatsSummaryInfoItem;
import com.android.loganalysis.util.NumberFormattingUtil;

import java.util.Calendar;
import java.util.GregorianCalendar;
import java.util.LinkedList;
import java.util.List;
import java.util.Queue;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * A {@link IParser} to parse batterystats summary
 */
public class BatteryStatsSummaryInfoParser implements IParser{

    /**
     * Matches: 0 (15) RESET:TIME: 2015-01-18-12-56-57
     */
    private static final Pattern RESET_TIME_PATTERN = Pattern.compile("^\\s*"
            + "\\d\\s*\\(\\d+\\)\\s*RESET:TIME:\\s*(\\d+)-(\\d+)-(\\d+)-(\\d+)-(\\d+)-(\\d+)$");

    /**
     * Matches: +1d01h03m37s246ms (1) 028 c10400010 -running -wake_lock
     */
    private static final Pattern BATTERY_DISCHARGE_PATTERN = Pattern.compile(
            "^\\s*\\+(?:(\\d+)d)?(?:(\\d+)h)?(?:(\\d+)m)?(?:(\\d+)s)?(?:(\\d+)ms)? \\(\\d+\\) "
            + "(\\d+) \\w+ .*");

    private BatteryDischargeItem mBatteryDischarge = new BatteryDischargeItem();
    private BatteryStatsSummaryInfoItem mItem = new BatteryStatsSummaryInfoItem();
    private long mBatteryDischargeRateAvg = 0;
    private int mBatteryDischargeSamples = 0;
    private Calendar mResetTime;
    private static final int BATTERY_GROUP_LIMIT = 10;

    /**
     * {@inheritDoc}
     *
     * @return The {@link BatteryStatsSummaryInfoItem}.
     */
    @Override
    public BatteryStatsSummaryInfoItem parse(List<String> lines) {
        Matcher resetTimeMatcher = null;
        Matcher dischargeMatcher = null;

        long previousDischargeElapsedTime= 0;
        int previousBatteryLevel = 0;
        boolean batteryDischargedFully = false;
        for (String line : lines) {
            resetTimeMatcher = RESET_TIME_PATTERN.matcher(line);
            dischargeMatcher = BATTERY_DISCHARGE_PATTERN.matcher(line);
            if (resetTimeMatcher.matches()) {
                mResetTime = new GregorianCalendar();
                final int year = Integer.parseInt(resetTimeMatcher.group(1));
                final int month = Integer.parseInt(resetTimeMatcher.group(2));
                final int day = Integer.parseInt(resetTimeMatcher.group(3));
                final int hour = Integer.parseInt(resetTimeMatcher.group(4));
                final int minute = Integer.parseInt(resetTimeMatcher.group(5));
                final int second = Integer.parseInt(resetTimeMatcher.group(6));
                // Calendar month is zero indexed but the parsed date is 1-12
                mResetTime.set(year, (month - 1), day, hour, minute, second);
            } else if (dischargeMatcher.matches()) {
                final int days = NumberFormattingUtil.parseIntOrZero(dischargeMatcher.group(1));
                final int hours = NumberFormattingUtil.parseIntOrZero(dischargeMatcher.group(2));
                final int mins = NumberFormattingUtil.parseIntOrZero(dischargeMatcher.group(3));
                final int secs = NumberFormattingUtil.parseIntOrZero(dischargeMatcher.group(4));
                final int msecs = NumberFormattingUtil.parseIntOrZero(dischargeMatcher.group(5));
                final int batteryLevel = Integer.parseInt(dischargeMatcher.group(6));
                if (batteryLevel == 0) {
                    // Ignore the subsequent battery drop readings
                    batteryDischargedFully = true;
                    continue;
                } else if (previousBatteryLevel == 0) {
                    // Ignore the first drop
                    previousBatteryLevel = batteryLevel;
                    continue;
                } else if (!batteryDischargedFully && previousBatteryLevel != batteryLevel) {
                    long elapsedTime = NumberFormattingUtil.getMs(days, hours, mins, secs, msecs);
                    mBatteryDischargeRateAvg += (elapsedTime  - previousDischargeElapsedTime);
                    mBatteryDischargeSamples++;
                    mBatteryDischarge.addBatteryDischargeInfo(
                            getDischargeClockTime(days, hours, mins, secs),
                            (elapsedTime - previousDischargeElapsedTime), batteryLevel);
                    previousDischargeElapsedTime = elapsedTime;
                    previousBatteryLevel = batteryLevel;
                }
            }
        }
        mItem.setBatteryDischargeRate(getAverageDischargeRate());
        mItem.setPeakDischargeTime(getPeakDischargeTime());
        return mItem;
    }

    private Calendar getDischargeClockTime(int days, int hours, int mins, int secs) {
        Calendar dischargeClockTime = new GregorianCalendar();

        dischargeClockTime.setTime(mResetTime.getTime());
        dischargeClockTime.add(Calendar.DATE, days);
        dischargeClockTime.add(Calendar.HOUR, hours);
        dischargeClockTime.add(Calendar.MINUTE, mins);
        dischargeClockTime.add(Calendar.SECOND, secs);
        return dischargeClockTime;
    }

    private String getAverageDischargeRate() {
        if (mBatteryDischargeSamples == 0) {
            return "The battery did not discharge";
        }

        final long minsPerLevel = mBatteryDischargeRateAvg / (mBatteryDischargeSamples * 60 * 1000);
        return String.format("The battery dropped a level %d mins on average", minsPerLevel);
    }

    private String getPeakDischargeTime() {

        int peakDischargeStartBatteryLevel = 0, peakDischargeStopBatteryLevel = 0;
        long minDischargeDuration = 0;
        Calendar peakDischargeStartTime= null, peakDischargeStopTime = null;
        Queue <BatteryDischargeInfoItem> batteryDischargeWindow =
                new LinkedList <BatteryDischargeInfoItem>();
        long sumDischargeDuration = 0;
        for (BatteryDischargeInfoItem dischargeSteps : mBatteryDischarge.getDischargeStepsInfo()) {
            batteryDischargeWindow.add(dischargeSteps);
            sumDischargeDuration += dischargeSteps.getElapsedTime();
            if (batteryDischargeWindow.size() >= BATTERY_GROUP_LIMIT) {
                final long averageDischargeDuration = sumDischargeDuration/BATTERY_GROUP_LIMIT;
                final BatteryDischargeInfoItem startNode = batteryDischargeWindow.remove();
                sumDischargeDuration -= startNode.getElapsedTime();

                if (minDischargeDuration == 0 || averageDischargeDuration < minDischargeDuration) {
                    minDischargeDuration = averageDischargeDuration;
                    peakDischargeStartBatteryLevel = startNode.getBatteryLevel();
                    peakDischargeStopBatteryLevel = dischargeSteps.getBatteryLevel();
                    peakDischargeStartTime = startNode.getClockTime();
                    peakDischargeStopTime = dischargeSteps.getClockTime();
                }
            }
        }
        if (peakDischargeStartTime != null && peakDischargeStopTime != null &&
                peakDischargeStartBatteryLevel > 0 && peakDischargeStopBatteryLevel > 0) {
            return String.format(
                    "The peak discharge time was during %s to %s where battery dropped from %d to "
                    + "%d", peakDischargeStartTime.getTime().toString(),
                    peakDischargeStopTime.getTime().toString(), peakDischargeStartBatteryLevel,
                    peakDischargeStopBatteryLevel);

        } else {
            return "The battery did not discharge";
        }
    }

    /**
     * Get the {@link BatteryStatsSummaryInfoItem}.
     * <p>
     * Exposed for unit testing.
     * </p>
     */
    BatteryStatsSummaryInfoItem getItem() {
        return mItem;
    }
}
