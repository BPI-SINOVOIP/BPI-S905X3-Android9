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
 * A generic item containing attributes for time, process, and thread and can be extended for
 * items such as {@link AnrItem} and {@link JavaCrashItem}.
 */
public class MiscKernelLogItem extends GenericItem {

    /** Constant for JSON output */
    public static final String EVENT_TIME = "EVENT_TIME";
    /** Constant for JSON output */
    public static final String PREAMBLE = "LAST_PREAMBLE";
    /** Constant for JSON output */
    public static final String CATEGORY = "CATEGORY";
    /** Constant for JSON output */
    public static final String STACK = "STACK";

    private static final Set<String> ATTRIBUTES = new HashSet<String>(Arrays.asList(
            EVENT_TIME, PREAMBLE, CATEGORY, STACK));

    /**
     * Constructor for {@link MiscKernelLogItem}.
     */
    public MiscKernelLogItem() {
        super(ATTRIBUTES);
    }

    /**
     * Constructor for {@link MiscKernelLogItem}.
     *
     * @param attributes A list of allowed attributes.
     */
    protected MiscKernelLogItem(Set<String> attributes) {
        super(getAllAttributes(attributes));
    }

    /**
     * Get the time object when the event happened.
     */
    public Double getEventTime() {
        return (Double) getAttribute(EVENT_TIME);
    }

    /**
     * Set the time object when the event happened.
     */
    public void setEventTime(Double time) {
        setAttribute(EVENT_TIME, time);
    }

    /**
     * Get the preamble for the event.
     */
    public String getPreamble() {
        return (String) getAttribute(PREAMBLE);
    }

    /**
     * Set the preamble for the event.
     */
    public void setPreamble(String preamble) {
        setAttribute(PREAMBLE, preamble);
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

    /**
     * Combine an array of attributes with the internal list of attributes.
     */
    private static Set<String> getAllAttributes(Set<String> attributes) {
        Set<String> allAttributes = new HashSet<String>(ATTRIBUTES);
        allAttributes.addAll(attributes);
        return allAttributes;
    }
}
