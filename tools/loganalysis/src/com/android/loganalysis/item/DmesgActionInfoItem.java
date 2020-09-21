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
 * An {@link IItem} used to store action info logged in dmesg
 * For example: [   14.942872] init: processing action (early-init)
 */
public class DmesgActionInfoItem extends GenericItem {

    /** Constant for JSON output */
    public static final String ACTION_NAME = "ACTION_NAME";
    /** Constant for JSON output */
    public static final String ACTION_START_TIME = "ACTION_START_TIME";

    private static final Set<String> ATTRIBUTES = new HashSet<String>(Arrays.asList(
            ACTION_NAME, ACTION_START_TIME));

    /**
     * The constructor for {@link DmesgActionInfoItem}.
     */
    public DmesgActionInfoItem() {
        super(ATTRIBUTES);
    }

    /**
     * The constructor for {@link DmesgActionInfoItem}.
     */
    public DmesgActionInfoItem(String name, Long startTime) {
        super(ATTRIBUTES);
        setAttribute(ACTION_NAME, name);
        setAttribute(ACTION_START_TIME, startTime);
    }

    /**
     * Get the name of the action
     */
    public String getActionName() {
        return (String) getAttribute(ACTION_NAME);
    }

    /**
     * Set the name of the action
     */
    public void setActionName(String stageName) {
        setAttribute(ACTION_NAME, stageName);
    }

    /**
     * Get the start time in msecs
     */
    public Long getStartTime() {
        return (Long) getAttribute(ACTION_START_TIME);
    }

    /**
     * Set the start time in msecs
     */
    public void setStartTime(Long startTime) {
        setAttribute(ACTION_START_TIME, startTime);
    }

    @Override
    public String toString() {
        return "ActionInfoItem [getActionName()=" + getActionName() + ", getStartTime()="
                + getStartTime() + "]";
    }

}
