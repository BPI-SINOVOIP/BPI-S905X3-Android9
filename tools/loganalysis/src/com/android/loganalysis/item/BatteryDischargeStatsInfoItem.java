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

package com.android.loganalysis.item;

import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;

/**
 * An {@link IItem} used to store information of the battery discharge.
 */
public class BatteryDischargeStatsInfoItem extends GenericItem {

    /** Constant for JSON output */
    public static final String MAX_PERCENTAGE = "MAX_PERCENTAGE";
    /** Constant for JSON output */
    public static final String MIN_PERCENTAGE = "MIN_PERCENTAGE";
    /** Constant for JSON output */
    public static final String DISCHARGE_PERCENTAGE = "DISCHARGE_PERCENTAGE";
    /** Constant for JSON output */
    public static final String DISCHARGE_DURATION = "DISCHARGE_DURATION";
    /** Constant for JSON output */
    public static final String PROJECTED_BATTERY_LIFE = "PROJECTED_BATTERY_LIFE";

    private static final Set<String> ATTRIBUTES = new HashSet<>(Arrays.asList(MAX_PERCENTAGE,
        MIN_PERCENTAGE, DISCHARGE_PERCENTAGE, DISCHARGE_DURATION, PROJECTED_BATTERY_LIFE));

    /**
     * The constructor for {@link BatteryDischargeStatsInfoItem}.
     */
    public BatteryDischargeStatsInfoItem() {
        super(ATTRIBUTES);
    }

    /**
     * Set the maximum percentage.
     */
    public void setMaxPercentage(int percentage) {
        setAttribute(MAX_PERCENTAGE, percentage);
    }

    /**
     * Set the minimum percentage.
     */
    public void setMinPercentage(int percentage) {
        setAttribute(MIN_PERCENTAGE, percentage);
    }

    /**
     * Set the discharge percentage.
     */
    public void setDischargePercentage(int dischargePercentage) {
        setAttribute(DISCHARGE_PERCENTAGE, dischargePercentage);
    }

    /**
     * Set the discharge duration.
     */
    public void setDischargeDuration(long dischargeDuration) {
        setAttribute(DISCHARGE_DURATION, dischargeDuration);
    }

    /**
     * Set the projected battery life.
     */
    public void setProjectedBatteryLife(long projectedBatteryLife) {
        setAttribute(PROJECTED_BATTERY_LIFE, projectedBatteryLife);
    }

    /**
     * Get the maximum percentage.
     */
    public int getMaxPercentage() {
        return (int) getAttribute(MAX_PERCENTAGE);
    }

    /**
     * Get the minimum percentage.
     */
    public int getMinPercentage() {
        return (int) getAttribute(MIN_PERCENTAGE);
    }

    /**
     * Get the discharge percentage.
     */
    public int getDischargePercentage() {
        return (int) getAttribute(DISCHARGE_PERCENTAGE);
    }

    /**
     * Get the discharge duration.
     */
    public long getDischargeDuration() {
        return (long) getAttribute(DISCHARGE_DURATION);
    }

    /**
     * Get the projected battery life.
     */
    public long getProjectedBatteryLife() {
        return (long) getAttribute(PROJECTED_BATTERY_LIFE);
    }

}
