/*
 * Copyright (C) 2013 The Android Open Source Project
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
 * An {@link IItem} used to store the output of the dumpsys section of the bugreport.
 */
public class DumpsysItem extends GenericItem {

    /** Constant for JSON output */
    private static final String BATTERY_STATS = "BATTERY_STATS";
    /** Constant for JSON output */
    private static final String PACKAGE_STATS = "PACKAGE_STATS";
    /** Constant for JSON output */
    private static final String PROC_STATS = "PROC_STATS";
    /** Constant for JSON output */
    private static final String WIFI_STATS = "WIFI_STATS";

    private static final Set<String> ATTRIBUTES =
            new HashSet<String>(
                    Arrays.asList(BATTERY_STATS, PACKAGE_STATS, PROC_STATS, WIFI_STATS));

    /**
     * The constructor for {@link DumpsysItem}.
     */
    public DumpsysItem() {
        super(ATTRIBUTES);
    }

    /**
     * Set the {@link DumpsysBatteryStatsItem} of the bugreport.
     */
    public void setBatteryInfo(DumpsysBatteryStatsItem batteryStats) {
        setAttribute(BATTERY_STATS, batteryStats);
    }

    /** Set the {@link DumpsysPackageStatsItem} of the bugreport. */
    public void setPackageStats(DumpsysPackageStatsItem packageStats) {
        setAttribute(PACKAGE_STATS, packageStats);
    }

    /** Set the {@link DumpsysProcStatsItem} of the bugreport. */
    public void setProcStats(DumpsysProcStatsItem procStats) {
        setAttribute(PROC_STATS, procStats);
    }

    /**
     * Set the {@link DumpsysWifiStatsItem} of the bugreport.
     */
    public void setWifiStats(DumpsysWifiStatsItem wifiStats) {
        setAttribute(WIFI_STATS, wifiStats);
    }

    /**
     * Get the {@link DumpsysBatteryStatsItem} of the bugreport.
     */
    public DumpsysBatteryStatsItem getBatteryStats() {
        return (DumpsysBatteryStatsItem) getAttribute(BATTERY_STATS);
    }

    /** Get the {@link DumpsysPackageStatsItem} of the bugreport. */
    public DumpsysPackageStatsItem getPackageStats() {
        return (DumpsysPackageStatsItem) getAttribute(PACKAGE_STATS);
    }

    /** Get the {@link DumpsysProcStatsItem} of the bugreport. */
    public DumpsysProcStatsItem getProcStats() {
        return (DumpsysProcStatsItem) getAttribute(PROC_STATS);
    }

    /**
     * Get the {@link DumpsysWifiStatsItem} of the bugreport.
     */
    public DumpsysWifiStatsItem getWifiStats() {
        return (DumpsysWifiStatsItem) getAttribute(WIFI_STATS);
    }
}
