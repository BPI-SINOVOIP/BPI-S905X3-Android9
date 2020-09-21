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

import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;

/**
 * An {@link GenericItem} used to store the power analysis summary
 */
public class BatteryStatsSummaryInfoItem extends GenericItem {

    /** Constant for JSON output */
    public static final String DISCHARGE_RATE = "DISCHARGE_RATE";
    /** Constant for JSON output */
    public static final String PEAK_DISCHARGE_TIME = "PEAK_DISCHARGE_TIME";

    private static final Set<String> ATTRIBUTES = new HashSet<String>(Arrays.asList(
            DISCHARGE_RATE, PEAK_DISCHARGE_TIME));

    /**
      * The constructor for {@link BatteryStatsSummaryInfoItem}.
      */
    public BatteryStatsSummaryInfoItem() {
        super(ATTRIBUTES);
    }

    /**
     * Get the battery discharge rate
     */
    public String getBatteryDischargeRate() {
        return (String) getAttribute(DISCHARGE_RATE);
    }

    /**
     * Set the battery discharge rate
     */
    public void setBatteryDischargeRate(String dischargeRate) {
        setAttribute(DISCHARGE_RATE, dischargeRate);
    }

    /**
     * Get the peak discharge time
     */
    public String getPeakDischargeTime() {
        return (String) getAttribute(PEAK_DISCHARGE_TIME);
    }

    /**
     * Set the peak discharge time
     */
    public void setPeakDischargeTime(String peakDischargeTime) {
        setAttribute(PEAK_DISCHARGE_TIME, peakDischargeTime);
    }
}
