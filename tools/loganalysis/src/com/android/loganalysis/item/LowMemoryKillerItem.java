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
 * An {@link IItem} used to store traces info.
 * <p>
 * This stores info about page allocation failures, i.e. order.
 * </p>
 */
public class LowMemoryKillerItem extends MiscKernelLogItem {
    /** Constant for JSON output */
    public static final String PID = "PID";
    /** Constant for JSON output */
    public static final String PROCESS_NAME = "PROCESS_NAME";
    /** Constant for JSON output */
    public static final String ADJUSTMENT = "ADJUSTMENT";

    private static final Set<String> ATTRIBUTES = new HashSet<String>(Arrays.asList(
            PID, PROCESS_NAME, ADJUSTMENT));

    /**
     * The constructor for {@link LowMemoryKillerItem}.
     */
    public LowMemoryKillerItem() {
        super(ATTRIBUTES);
    }

    /**
     * Get the pid of the killed process.
     */
    public int getPid() {
        return (int) getAttribute(PID);
    }

    /**
     * Set the pid of the killed process.
     */
    public void setPid(int pid) {
        setAttribute(PID, pid);
    }

    /**
     * Get the name of the killed process.
     */
    public String getProcessName() {
        return (String) getAttribute(PROCESS_NAME);
    }

    /**
     * Set the name of the killed process.
     */
    public void setProcessName(String name) {
        setAttribute(PROCESS_NAME, name);
    }

    /**
     * Get the adjustment score of the LMK action.
     */
    public int getAdjustment() {
        return (int) getAttribute(ADJUSTMENT);
    }

    /**
     * Set the adjustment score of the LMK action.
     */
    public void setAdjustment(int adjustment) {
        setAttribute(ADJUSTMENT, adjustment);
    }
}
