/*
 * Copyright (C) 2012 The Android Open Source Project
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

import com.android.loganalysis.parser.LogcatParser;

import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;

/**
 * An {@link IItem} used to store ANR info.
 */
public class AnrItem extends MiscLogcatItem {
    /**
     * An enum used to select the CPU usage category.
     */
    public enum CpuUsageCategory {
        TOTAL,
        USER,
        KERNEL,
        IOWAIT,
    }

    /**
     * An enum used to select the load category.
     */
    public enum LoadCategory {
        LOAD_1,
        LOAD_5,
        LOAD_15;
    }

    /** Constant for JSON output */
    public static final String ACTIVITY = "ACTIVITY";
    /** Constant for JSON output */
    public static final String REASON = "REASON";
    /** Constant for JSON output */
    public static final String TRACE = "TRACE";

    private static final Set<String> ATTRIBUTES = new HashSet<String>(Arrays.asList(
        CpuUsageCategory.TOTAL.toString(),
        CpuUsageCategory.USER.toString(),
        CpuUsageCategory.KERNEL.toString(),
        CpuUsageCategory.IOWAIT.toString(),
        LoadCategory.LOAD_1.toString(),
        LoadCategory.LOAD_5.toString(),
        LoadCategory.LOAD_15.toString(),
        ACTIVITY, REASON, TRACE));

    /**
     * The constructor for {@link AnrItem}.
     */
    public AnrItem() {
        super(ATTRIBUTES);
        setCategory(LogcatParser.ANR);
    }

    /**
     * Get the CPU usage for a given category.
     */
    public Double getCpuUsage(CpuUsageCategory category) {
        return (Double) getAttribute(category.toString());
    }

    /**
     * Set the CPU usage for a given category.
     */
    public void setCpuUsage(CpuUsageCategory category, Double usage) {
        setAttribute(category.toString(), usage);
    }

    /**
     * Get the load for a given category.
     */
    public Double getLoad(LoadCategory category) {
        return (Double) getAttribute(category.toString());
    }

    /**
     * Set the load for a given category.
     */
    public void setLoad(LoadCategory category, Double usage) {
        setAttribute(category.toString(), usage);
    }

    /**
     * Get the activity for the ANR.
     */
    public String getActivity() {
        return (String) getAttribute(ACTIVITY);
    }

    /**
     * Set the activity for the ANR.
     */
    public void setActivity(String activity) {
        setAttribute(ACTIVITY, activity);
    }

    /**
     * Get the reason for the ANR.
     */
    public String getReason() {
        return (String) getAttribute(REASON);
    }

    /**
     * Set the reason for the ANR.
     */
    public void setReason(String reason) {
        setAttribute(REASON, reason);
    }

    /**
     * Get the main trace for the ANR.
     */
    public String getTrace() {
        return (String) getAttribute(TRACE);
    }

    /**
     * Set the main trace for the ANR.
     */
    public void setTrace(String trace) {
        setAttribute(TRACE, trace);
    }
}
