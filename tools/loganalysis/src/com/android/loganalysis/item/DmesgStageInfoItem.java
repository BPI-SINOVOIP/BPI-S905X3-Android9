/*
 * Copyright (C) 2017 The Android Open Source Project
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
 * An {@link IItem} used to store stage info logged in dmesg.
 * For example: [   14.425056] init: init second stage started!
 */
public class DmesgStageInfoItem extends GenericItem {

    /** Constant for JSON output */
    public static final String STAGE_NAME = "STAGE_NAME";
    /** Constant for JSON output */
    public static final String STAGE_START_TIME = "STAGE_START_TIME";
    /** Constant for JSON output */
    public static final String STAGE_DURATION = "STAGE_DURATION";

    private static final Set<String> ATTRIBUTES = new HashSet<String>(Arrays.asList(
            STAGE_NAME, STAGE_START_TIME, STAGE_DURATION));

    /**
     * The constructor for {@link DmesgStageInfoItem}.
     */
    public DmesgStageInfoItem() {
        super(ATTRIBUTES);
    }

    /**
     * The constructor for {@link DmesgStageInfoItem}.
     */
    public DmesgStageInfoItem(String name, Long startTime, Long duration) {
        super(ATTRIBUTES);
        setAttribute(STAGE_NAME, name);
        setAttribute(STAGE_START_TIME, startTime);
        setAttribute(STAGE_DURATION, duration);
    }

    /**
     * Get the name of the stage
     */
    public String getStageName() {
        return (String) getAttribute(STAGE_NAME);
    }

    /**
     * Set the name of the stage
     */
    public void setStageName(String stageName) {
        setAttribute(STAGE_NAME, stageName);
    }

    /**
     * Get the start time in msecs
     */
    public Long getStartTime() {
        return (Long) getAttribute(STAGE_START_TIME);
    }

    /**
     * Set the start time in msecs
     */
    public void setStartTime(Long startTime) {
        setAttribute(STAGE_START_TIME, startTime);
    }

    /**
     * Get the duration in msecs
     */
    public Long getDuration() {
        return (Long) getAttribute(STAGE_DURATION);
    }

    /**
     * Set the duration in msecs
     */
    public void setDuration(Long duration) {
        setAttribute(STAGE_DURATION, duration);
    }

    @Override
    public String toString() {
        return "StageInfoItem [getStageName()=" + getStageName() + ", getStartTime()="
                + getStartTime() + ", getDuration()=" + getDuration() + "]";
    }

}
