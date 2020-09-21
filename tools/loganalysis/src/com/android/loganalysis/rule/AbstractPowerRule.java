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

import com.android.loganalysis.item.BatteryStatsDetailedInfoItem;
import com.android.loganalysis.item.BugreportItem;
import com.android.loganalysis.item.BatteryStatsSummaryInfoItem;
import com.android.loganalysis.item.DumpsysProcStatsItem;
import com.android.loganalysis.item.DumpsysWifiStatsItem;

import org.json.JSONObject;

/**
 * Base class for all power rules
 */
public abstract class AbstractPowerRule implements IRule {

    private BugreportItem mBugreportItem;
    private BatteryStatsSummaryInfoItem mPowerSummaryAnalysisItem;
    private BatteryStatsDetailedInfoItem mPowerDetailedAnalysisItem;
    private DumpsysProcStatsItem mProcStatsItem;
    private DumpsysWifiStatsItem mWifiStatsItem;

    public AbstractPowerRule(BugreportItem bugreportItem) {
        mBugreportItem = bugreportItem;
        mPowerSummaryAnalysisItem = mBugreportItem.getDumpsys().getBatteryStats().
                getBatteryStatsSummaryItem();
        mPowerDetailedAnalysisItem = mBugreportItem.getDumpsys().getBatteryStats().
                getDetailedBatteryStatsItem();
        mProcStatsItem = mBugreportItem.getDumpsys().getProcStats();
        mWifiStatsItem = mBugreportItem.getDumpsys().getWifiStats();
    }

    protected long getTimeOnBattery() {
        return mPowerDetailedAnalysisItem.getTimeOnBattery();
    }

    protected BatteryStatsSummaryInfoItem getSummaryItem() {
        return mPowerSummaryAnalysisItem;
    }

    protected BatteryStatsDetailedInfoItem getDetailedAnalysisItem() {
        return mPowerDetailedAnalysisItem;
    }

    protected DumpsysProcStatsItem getProcStatsItem() {
        return mProcStatsItem;
    }

    protected DumpsysWifiStatsItem getWifiStatsItem() {
        return mWifiStatsItem;
    }

    @Override
    public abstract void applyRule();

    @Override
    public abstract JSONObject getAnalysis();

}
