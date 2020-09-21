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
package com.android.loganalysis.item;

import org.json.JSONException;
import org.json.JSONObject;

/**
 * An {@link IItem} used to store BatteryStats Info
 */
public class DumpsysBatteryStatsItem implements IItem {

    /** Constant for JSON output */
    public static final String SUMMARY = "SUMMARY";
    /** Constant for JSON output */
    public static final String DETAILED_STATS = "DETAILED_STATS";
    /** Constant for JSON output */
    public static final String DISCHARGE_STATS = "DISCHARGE_STATS";

    private BatteryStatsSummaryInfoItem mBatteryStatsSummaryItem;
    private BatteryStatsDetailedInfoItem mDetailedBatteryStatsItem;
    private BatteryDischargeStatsInfoItem mDischargeStatsItem;

    /**
     * Set the battery stats summary {@link BatteryStatsSummaryInfoItem}
     */
    public void setBatteryStatsSummarytem(BatteryStatsSummaryInfoItem summaryItem) {
        mBatteryStatsSummaryItem = summaryItem;
    }

    /**
     * Set the detailed battery stats item {@link BatteryStatsDetailedInfoItem}
     */
    public void setDetailedBatteryStatsItem(BatteryStatsDetailedInfoItem detailedItem) {
        mDetailedBatteryStatsItem = detailedItem;
    }

    /**
     * Set the battery steps info item {@link BatteryDischargeStatsInfoItem}
     */
    public void setBatteryDischargeStatsItem(BatteryDischargeStatsInfoItem item){
        mDischargeStatsItem = item;
    }

    /**
     * Get the battery stats summary {@link BatteryStatsSummaryInfoItem}
     */
    public BatteryStatsSummaryInfoItem getBatteryStatsSummaryItem() {
        return mBatteryStatsSummaryItem;
    }

    /**
     * Get the detailed battery stats item {@link BatteryStatsDetailedInfoItem}
     */
    public BatteryStatsDetailedInfoItem getDetailedBatteryStatsItem() {
        return mDetailedBatteryStatsItem;
    }

    /**
     * Get the battery steps info item {@link BatteryDischargeStatsInfoItem}
     */
    public BatteryDischargeStatsInfoItem getBatteryDischargeStatsItem() {
        return mDischargeStatsItem;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public IItem merge(IItem other) throws ConflictingItemException {
        throw new ConflictingItemException("Dumpsys battery info items cannot be merged");
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean isConsistent(IItem other) {
        return false;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public JSONObject toJson() {
        JSONObject batteryStatsComponent = new JSONObject();
        try {
            if (mBatteryStatsSummaryItem != null) {
                batteryStatsComponent.put(SUMMARY, mBatteryStatsSummaryItem.toJson());
            }
            if (mDetailedBatteryStatsItem != null) {
                batteryStatsComponent.put(DETAILED_STATS, mDetailedBatteryStatsItem.toJson());
            }
            if (mDischargeStatsItem != null) {
                batteryStatsComponent.put(DISCHARGE_STATS, mDischargeStatsItem.toJson());
            }
        } catch (JSONException e) {
            // ignore
        }
        return batteryStatsComponent;
    }
}
