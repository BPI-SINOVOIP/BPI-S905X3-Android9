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
public class BatteryStatsDetailedInfoItem implements IItem {

    /** Constant for JSON output */
    public static final String TIME_ON_BATTERY = "TIME_ON_BATTERY";
    /** Constant for JSON output */
    public static final String SCREEN_ON_TIME = "SCREEN_ON_TIME";
    /** Constant for JSON output */
    public static final String BATTERY_USAGE = "BATTERY_USAGE";
    /** Constant for JSON output */
    public static final String WAKELOCKS = "WAKELOCKS";
    /** Constant for JSON output */
    public static final String INTERRUPTS = "INTERRUPTS";
    /** Constant for JSON output */
    public static final String PROCESS_USAGE = "PROCESS_USAGE";

    private long mTimeOnBattery = 0;
    private long mScreenOnTime = 0;
    private BatteryUsageItem mBatteryUsageItem = null;
    private WakelockItem mWakelockItem = null;
    private InterruptItem mInterruptItem = null;
    private ProcessUsageItem mprocessUsageItem = null;

    /**
     * Set the time on battery
     */
    public void setTimeOnBattery(long timeOnBattery) {
        mTimeOnBattery = timeOnBattery;
    }

    /**
     * Set the time on battery
     */
    public void setScreenOnTime(long screenOnTime) {
        mScreenOnTime = screenOnTime;
    }

    /**
     * Set the wakelock summary {@link WakelockItem}
     */
    public void setWakelockItem(WakelockItem wakelockItem) {
        mWakelockItem = wakelockItem;
    }

    /**
     * Set the interrupt summary {@link InterruptItem}
     */
    public void setInterruptItem(InterruptItem interruptItem) {
        mInterruptItem = interruptItem;
    }

    /**
     * Set the process usage {@link ProcessUsageItem}
     */
    public void setProcessUsageItem(ProcessUsageItem processUsageItem) {
        mprocessUsageItem = processUsageItem;
    }

    /**
     * Set the process usage {@link BatteryUsageItem}
     */
    public void setBatteryUsageItem(BatteryUsageItem batteryUsageItem) {
        mBatteryUsageItem = batteryUsageItem;
    }

    /**
     * Get the time on battery
     */
    public long getTimeOnBattery() {
        return mTimeOnBattery;
    }

    /**
     * Get the screen on time
     */
    public long getScreenOnTime() {
        return mScreenOnTime;
    }

    /**
     * Get the wakelock summary {@link WakelockItem}
     */
    public WakelockItem getWakelockItem() {
        return mWakelockItem;
    }

    /**
     * Get the interrupt summary {@link InterruptItem}
     */
    public InterruptItem getInterruptItem() {
        return mInterruptItem;
    }

    /**
     * Get the process usage summary {@link ProcessUsageItem}
     */
    public ProcessUsageItem getProcessUsageItem() {
        return mprocessUsageItem;
    }

    /**
     * Get the battery usage summary {@link BatteryUsageItem}
     */
    public BatteryUsageItem getBatteryUsageItem() {
        return mBatteryUsageItem;
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
            if (mTimeOnBattery > 0) {
                batteryStatsComponent.put(TIME_ON_BATTERY, getTimeOnBattery());
            }
            if (mScreenOnTime > 0) {
                batteryStatsComponent.put(SCREEN_ON_TIME, getScreenOnTime());
            }
            if (mBatteryUsageItem != null) {
                batteryStatsComponent.put(BATTERY_USAGE, mBatteryUsageItem.toJson());
            }
            if (mWakelockItem != null) {
                batteryStatsComponent.put(WAKELOCKS, mWakelockItem.toJson());
            }
            if (mInterruptItem != null) {
                batteryStatsComponent.put(INTERRUPTS, mInterruptItem.toJson());
            }
            if (mprocessUsageItem != null) {
                batteryStatsComponent.put(PROCESS_USAGE, mprocessUsageItem.toJson());
            }
        } catch (JSONException e) {
            // ignore
        }
        return batteryStatsComponent;
    }
}
