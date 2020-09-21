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

import com.android.loganalysis.item.ProcessUsageItem;
import com.android.loganalysis.item.ProcessUsageItem.SensorInfoItem;
import com.android.loganalysis.util.NumberFormattingUtil;

import java.util.LinkedList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * A {@link IParser} to handle the parsing of process usage information
 */
public class ProcessUsageParser implements IParser {

    private ProcessUsageItem mItem = new ProcessUsageItem();
    private LinkedList<SensorInfoItem> mSensorUsage = new LinkedList<SensorInfoItem>();

    /**
     * Matches: 1000:
     */
    private static final Pattern UID_PATTERN = Pattern.compile("^\\s*(\\w+):$");

    /**
     * Matches: Sensor 1: 12m 52s 311ms realtime (29 times)
     */
    private static final Pattern SENSOR_PATTERN = Pattern.compile(
            "^\\s*Sensor (\\d+): (?:(\\d+)d\\s)?"
            + "(?:(\\d+)h\\s)?(?:(\\d+)m\\s)?(?:(\\d+)s\\s)?(\\d+)ms "
            + "realtime \\((\\d+) times\\)$");

    /**
     * Matches: 507 wakeup alarms
     */
    private static final Pattern ALARM_PATTERN = Pattern.compile("^\\s*(\\d+) wakeup alarms$");

    /**
     * {@inheritDoc}
     */
    @Override
    public ProcessUsageItem parse(List<String> lines) {
        String processUid = null;
        int alarmWakeups = 0;
        for (String line : lines) {
            Matcher m = UID_PATTERN.matcher(line);
            if (m.matches()) {
                if (processUid != null) {
                    // Save the process usage info for the previous process
                    mItem.addProcessUsage(processUid, alarmWakeups, mSensorUsage);
                }
                processUid = m.group(1);
                mSensorUsage = new LinkedList<SensorInfoItem>();
                continue;
            }
            m = SENSOR_PATTERN.matcher(line);
            if (m.matches()) {
                final long duration = NumberFormattingUtil.getMs(
                        NumberFormattingUtil.parseIntOrZero(m.group(2)),
                        NumberFormattingUtil.parseIntOrZero(m.group(3)),
                        NumberFormattingUtil.parseIntOrZero(m.group(4)),
                        NumberFormattingUtil.parseIntOrZero(m.group(5)),
                        NumberFormattingUtil.parseIntOrZero(m.group(6)));
                mSensorUsage.add(new SensorInfoItem(m.group(1), duration));
                continue;
            }
            m = ALARM_PATTERN.matcher(line);
            if (m.matches()) {
                alarmWakeups = Integer.parseInt(m.group(1));
            }
        }
        // Add the last process usage stats to the list
        if (processUid != null) {
            // Save the process usage info for the previous process
            mItem.addProcessUsage(processUid, alarmWakeups, mSensorUsage);
        }

        return mItem;
    }

    /**
     * Get the {@link ProcessUsageItem}.
     * <p>
     * Exposed for unit testing.
     * </p>
     */
    ProcessUsageItem getItem() {
        return mItem;
    }
}
