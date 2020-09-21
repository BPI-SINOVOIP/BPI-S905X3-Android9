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

import com.android.loganalysis.item.BatteryStatsDetailedInfoItem;
import com.android.loganalysis.item.BatteryUsageItem;
import com.android.loganalysis.item.InterruptItem;
import com.android.loganalysis.item.ProcessUsageItem;
import com.android.loganalysis.item.WakelockItem;
import com.android.loganalysis.util.NumberFormattingUtil;

import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;


/**
 * A {@link IParser} to parse the battery stats section of the bugreport
 */
public class BatteryStatsDetailedInfoParser extends AbstractSectionParser {

    private static final String BATTERY_USAGE_SECTION_REGEX = "^\\s*Estimated power use \\(mAh\\):$";
    private static final String KERNEL_WAKELOCK_SECTION_REGEX = "^\\s*All kernel wake locks:$";
    private static final String PARTIAL_WAKELOCK_SECTION_REGEX = "^\\s*All partial wake locks:$";
    private static final String INTERRUPT_SECTION_REGEX = "^\\s*All wakeup reasons:$";
    private static final String PROCESS_USAGE_SECTION_REGEX = "^\\s*0:$";

    /**
     * Matches: Time on battery: 7h 45m 54s 332ms (98.3%) realtime, 4h 40m 51s 315ms (59.3%) uptime
     */
    private static final Pattern TIME_ON_BATTERY_PATTERN = Pattern.compile(
            "^\\s*Time on battery: (?:(\\d+)d)?\\s?(?:(\\d+)h)?\\s?(?:(\\d+)m)?\\s?(?:(\\d+)s)?" +
            "\\s?(?:(\\d+)ms)?.*");
    /**
     * Matches:Time on battery screen off: 1d 4h 6m 16s 46ms (99.1%) realtime, 6h 37m 49s 201ms
     */
    private static final Pattern SCREEN_OFF_TIME_PATTERN = Pattern.compile("^\\s*Time on battery "
        + "screen off: (?:(\\d+)d)?\\s?(?:(\\d+)h)?\\s?(?:(\\d+)m)?\\s?(?:(\\d+)s)?\\s?"
        + "(?:(\\d+)ms).*");

    private BatteryUsageParser mBatteryUsageParser = new BatteryUsageParser();
    private WakelockParser mWakelockParser = new WakelockParser();
    private InterruptParser mInterruptParser = new InterruptParser();
    private ProcessUsageParser mProcessUsageParser = new ProcessUsageParser();

    private IParser mBatteryTimeParser = new IParser() {
        @Override
        public BatteryStatsDetailedInfoItem parse(List<String> lines) {
            BatteryStatsDetailedInfoItem detailedInfo = null;
            long timeOnBattery = 0, screenOffTime = 0;
            Matcher m = null;
            for (String line : lines) {
                if (detailedInfo == null && !"".equals(line.trim())) {
                    detailedInfo = new BatteryStatsDetailedInfoItem();
                }
                m = TIME_ON_BATTERY_PATTERN.matcher(line);
                if (m.matches()) {
                    timeOnBattery = NumberFormattingUtil.getMs(
                            NumberFormattingUtil.parseIntOrZero(m.group(1)),
                            NumberFormattingUtil.parseIntOrZero(m.group(2)),
                            NumberFormattingUtil.parseIntOrZero(m.group(3)),
                            NumberFormattingUtil.parseIntOrZero(m.group(4)),
                            NumberFormattingUtil.parseIntOrZero(m.group(5)));
                    detailedInfo.setTimeOnBattery(timeOnBattery);
                } else {
                    m = SCREEN_OFF_TIME_PATTERN.matcher(line);
                    if (m.matches()) {
                        screenOffTime = NumberFormattingUtil.getMs(
                                NumberFormattingUtil.parseIntOrZero(m.group(1)),
                                NumberFormattingUtil.parseIntOrZero(m.group(2)),
                                NumberFormattingUtil.parseIntOrZero(m.group(3)),
                                NumberFormattingUtil.parseIntOrZero(m.group(4)),
                                NumberFormattingUtil.parseIntOrZero(m.group(5)));
                        detailedInfo.setScreenOnTime(getScreenOnTime(timeOnBattery, screenOffTime));
                        return detailedInfo;
                    }
                }
            }
            return detailedInfo;
        }

        private long getScreenOnTime(long timeOnBattery, long screenOffTime) {
            if (timeOnBattery > screenOffTime) {
                return (timeOnBattery - screenOffTime);
            }
            return 0;
        }
    };

    private BatteryStatsDetailedInfoItem mBatteryStatsDetailedInfoItem = null;
    private boolean mParsedInput = false;

    /**
     * {@inheritDoc}
     *
     * @return The {@link BatteryStatsDetailedInfoItem}
     */
    @Override
    public BatteryStatsDetailedInfoItem parse(List<String> lines) {
        setup();
        for (String line : lines) {
            if (!mParsedInput && !"".equals(line.trim())) {
                mParsedInput = true;
            }
            parseLine(line);
        }
        commit();
        return mBatteryStatsDetailedInfoItem;
    }

    /**
     * Sets up the parser by adding the section parsers.
     */
    protected void setup() {
        setParser(mBatteryTimeParser);
        addSectionParser(mBatteryUsageParser, BATTERY_USAGE_SECTION_REGEX);
        addSectionParser(mWakelockParser, KERNEL_WAKELOCK_SECTION_REGEX);
        addSectionParser(mWakelockParser, PARTIAL_WAKELOCK_SECTION_REGEX);
        addSectionParser(mInterruptParser, INTERRUPT_SECTION_REGEX);
        addSectionParser(mProcessUsageParser, PROCESS_USAGE_SECTION_REGEX);
    }

    /**
     * Set the {@link BatteryStatsDetailedInfoItem}
     *
     */
    @Override
    protected void onSwitchParser() {
        if (mBatteryStatsDetailedInfoItem == null) {
            mBatteryStatsDetailedInfoItem = (BatteryStatsDetailedInfoItem)
                    getSection(mBatteryTimeParser);
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    protected void commit() {
        // signal EOF
        super.commit();
        if (mParsedInput) {
            if (mBatteryStatsDetailedInfoItem == null) {
                mBatteryStatsDetailedInfoItem = new BatteryStatsDetailedInfoItem();
            }
        }

        if (mBatteryStatsDetailedInfoItem != null) {
            mBatteryStatsDetailedInfoItem.setBatteryUsageItem(
                    (BatteryUsageItem) getSection(mBatteryUsageParser));
            mBatteryStatsDetailedInfoItem.setWakelockItem(
                    (WakelockItem) getSection(mWakelockParser));
            mBatteryStatsDetailedInfoItem.setInterruptItem(
                    (InterruptItem) getSection(mInterruptParser));
            mBatteryStatsDetailedInfoItem.setProcessUsageItem(
                    (ProcessUsageItem) getSection(mProcessUsageParser));
        }
    }
}
