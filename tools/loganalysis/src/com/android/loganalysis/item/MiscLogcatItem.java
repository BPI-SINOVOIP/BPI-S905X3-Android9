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
import java.util.Date;
import java.util.HashSet;
import java.util.Set;

/**
 * An {@link IItem} used to store miscellaneous logcat info.
 */
public class MiscLogcatItem extends GenericItem {
    /** Constant for JSON output */
    public static final String EVENT_TIME = "EVENT_TIME";
    /** Constant for JSON output */
    public static final String PID = "PID";
    /** Constant for JSON output */
    public static final String TID = "TID";
    /** Constant for JSON output */
    public static final String APP = "APP";
    /** Constant for JSON output */
    public static final String TAG = "TAG";
    /** Constant for JSON output */
    public static final String LAST_PREAMBLE = "LAST_PREAMBLE";
    /** Constant for JSON output */
    public static final String PROCESS_PREAMBLE = "PROCESS_PREAMBLE";
    /** Constant for JSON output */
    public static final String CATEGORY = "CATEGORY";
    /** Constant for JSON output */
    public static final String STACK = "STACK";

    private static final Set<String> ATTRIBUTES = new HashSet<String>(Arrays.asList(
            EVENT_TIME, PID, TID, APP, TAG, LAST_PREAMBLE, PROCESS_PREAMBLE, CATEGORY, STACK));

    /**
     * Constructor for {@link MiscLogcatItem}.
     */
    public MiscLogcatItem() {
        super(ATTRIBUTES);
    }

    /**
     * Constructor for {@link MiscLogcatItem}.
     *
     * @param attributes A list of allowed attributes.
     */
    protected MiscLogcatItem(Set<String> attributes) {
        super(getAllAttributes(attributes));
    }

    /**
     * Get the {@link Date} object when the event happened.
     */
    public Date getEventTime() {
        return (Date) getAttribute(EVENT_TIME);
    }

    /**
     * Set the {@link Date} object when the event happened.
     */
    public void setEventTime(Date time) {
        setAttribute(EVENT_TIME, time);
    }

    /**
     * Get the PID of the event.
     */
    public Integer getPid() {
        return (Integer) getAttribute(PID);
    }

    /**
     * Set the PID of the event.
     */
    public void setPid(Integer pid) {
        setAttribute(PID, pid);
    }

    /**
     * Get the TID of the event.
     */
    public Integer getTid() {
        return (Integer) getAttribute(TID);
    }

    /**
     * Set the TID of the event.
     */
    public void setTid(Integer tid) {
        setAttribute(TID, tid);
    }

    /**
     * Get the app or package name of the event.
     */
    public String getApp() {
        return (String) getAttribute(APP);
    }

    /**
     * Set the app or package name of the event.
     */
    public void setApp(String app) {
        setAttribute(APP, app);
    }

    /**
     * Get the tag of the event.
     */
    public String getTag() {
        return (String) getAttribute(TAG);
    }

    /**
     * Set the tag of the event.
     */
    public void setTag(String tag) {
        setAttribute(TAG, tag);
    }

    /**
     * Get the last preamble for the event.
     */
    public String getLastPreamble() {
        return (String) getAttribute(LAST_PREAMBLE);
    }

    /**
     * Set the last preamble for the event.
     */
    public void setLastPreamble(String preamble) {
        setAttribute(LAST_PREAMBLE, preamble);
    }

    /**
     * Get the process preamble for the event.
     */
    public String getProcessPreamble() {
        return (String) getAttribute(PROCESS_PREAMBLE);
    }

    /**
     * Set the process preamble for the event.
     */
    public void setProcessPreamble(String preamble) {
        setAttribute(PROCESS_PREAMBLE, preamble);
    }

    /**
     * Combine an array of attributes with the internal list of attributes.
     */
    private static Set<String> getAllAttributes(Set<String> attributes) {
        Set<String> allAttributes = new HashSet<String>(ATTRIBUTES);
        allAttributes.addAll(attributes);
        return allAttributes;
    }

    /**
     * Get the category of the event.
     */
    public String getCategory() {
        return (String) getAttribute(CATEGORY);
    }

    /**
     * Set the category of the event.
     */
    public void setCategory(String category) {
        setAttribute(CATEGORY, category);
    }

    /**
     * Get the stack for the event.
     */
    public String getStack() {
        return (String) getAttribute(STACK);
    }

    /**
     * Set the stack for the event.
     */
    public void setStack(String stack) {
        setAttribute(STACK, stack);
    }
}
