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
 * An {@link IItem} used to store service info logged in dmesg.
 */
public class DmesgServiceInfoItem extends GenericItem {

    /** Constant for JSON output */
    public static final String SERVICE_NAME = "SERVICE_NAME";
    /** Constant for JSON output */
    public static final String SERVICE_START_TIME = "SERVICE_START_TIME";
    /** Constant for JSON output */
    public static final String SERVICE_END_TIME = "SERVICE_END_TIME";
    /** Constant for JSON output */
    public static final String SERVICE_DURATION = "SERVICE_DURATION";

    private static final Set<String> ATTRIBUTES = new HashSet<String>(Arrays.asList(
            SERVICE_NAME, SERVICE_START_TIME, SERVICE_END_TIME));

    /**
     * The constructor for {@link DmesgServiceInfoItem}.
     */
    public DmesgServiceInfoItem() {
        super(ATTRIBUTES);
    }

    /**
     * Get the name of the service
     */
    public String getServiceName() {
        return (String) getAttribute(SERVICE_NAME);
    }

    /**
     * Set the name of the service
     */
    public void setServiceName(String serviceName) {
        setAttribute(SERVICE_NAME, serviceName);
    }

    /**
     * Get the start time in msecs
     */
    public Long getStartTime() {
        return (Long) getAttribute(SERVICE_START_TIME);
    }

    /**
     * Set the start time in msecs
     */
    public void setStartTime(Long startTime) {
        setAttribute(SERVICE_START_TIME, startTime);
    }

    /**
     * Get the end time in msecs
     */
    public Long getEndTime() {
        return (Long) getAttribute(SERVICE_END_TIME);
    }

    /**
     * Set the end time in msecs
     */
    public void setEndTime(Long endTime) {
        setAttribute(SERVICE_END_TIME, endTime);
    }

    /**
     * Get the service duration in msecs If the start or end time is not present then return -1
     */
    public Long getServiceDuration() {
        if (null != getAttribute(SERVICE_END_TIME) && null != getAttribute(SERVICE_START_TIME)) {
            return getEndTime() - getStartTime();
        }
        return -1L;
    }

}
