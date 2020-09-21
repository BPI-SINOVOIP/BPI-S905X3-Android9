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

import com.android.loganalysis.item.BatteryDischargeStatsInfoItem;
import com.android.loganalysis.item.BatteryStatsDetailedInfoItem;
import com.android.loganalysis.item.DumpsysBatteryStatsItem;
import com.android.loganalysis.item.BatteryStatsSummaryInfoItem;

import java.util.List;


/**
 * A {@link IParser} to parse the battery stats section of the bugreport
 */
public class DumpsysBatteryStatsParser extends AbstractSectionParser {

    private static final String SUMMARY_INFO_SECTION_REGEX =
            "Battery History \\(\\d+% used, \\d+(KB)? used of \\d+KB, \\d+ strings using "
            + "\\d+(KB)?\\):$";
    private static final String DISCHARGE_STATS_INFO_SECTION_REGEX = "^Discharge step durations:$";
    private static final String DETAILED_INFO_SECTION_REGEX = "^Statistics since last charge:$";

    // We are not using this sections and will be ignored.
    private static final String NOOP_SECTION_REGEX =
        "^(Statistics since last unplugged:|Daily stats:)$";

    private BatteryStatsSummaryInfoParser mSummaryParser = new BatteryStatsSummaryInfoParser();
    private BatteryStatsDetailedInfoParser mDetailedParser = new BatteryStatsDetailedInfoParser();
    private BatteryDischargeStatsInfoParser mDischargeStepsParser = new
        BatteryDischargeStatsInfoParser();

    private DumpsysBatteryStatsItem mDumpsysBatteryStatsItem = null;
    private boolean mParsedInput = false;

    /**
     * {@inheritDoc}
     *
     * @return The {@link DumpsysBatteryStatsItem}
     */
    @Override
    public DumpsysBatteryStatsItem parse(List<String> lines) {
        setup();
        for (String line : lines) {
            if (!mParsedInput && !"".equals(line.trim())) {
                mParsedInput = true;
            }
            parseLine(line);
        }
        commit();
        return mDumpsysBatteryStatsItem;
    }

    /**
     * Sets up the parser by adding the section parsers.
     */
    protected void setup() {
        addSectionParser(mSummaryParser, SUMMARY_INFO_SECTION_REGEX);
        addSectionParser(mDetailedParser, DETAILED_INFO_SECTION_REGEX);
        addSectionParser(mDischargeStepsParser, DISCHARGE_STATS_INFO_SECTION_REGEX);
        addSectionParser(new NoopParser(), NOOP_SECTION_REGEX);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    protected void commit() {
        // signal EOF
        super.commit();
        if (mParsedInput) {
            if (mDumpsysBatteryStatsItem == null) {
                mDumpsysBatteryStatsItem = new DumpsysBatteryStatsItem();
            }
        }

        if (mDumpsysBatteryStatsItem != null) {
            mDumpsysBatteryStatsItem.setBatteryStatsSummarytem(
                (BatteryStatsSummaryInfoItem) getSection(mSummaryParser));
            mDumpsysBatteryStatsItem.setDetailedBatteryStatsItem(
                (BatteryStatsDetailedInfoItem) getSection(mDetailedParser));
            mDumpsysBatteryStatsItem.setBatteryDischargeStatsItem(
                (BatteryDischargeStatsInfoItem) getSection(mDischargeStepsParser));
        }
    }
}
