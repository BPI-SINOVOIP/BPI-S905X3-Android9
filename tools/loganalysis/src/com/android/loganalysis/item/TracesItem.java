/*
 * Copyright (C) 2011 The Android Open Source Project
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
 * For now, this only stores info about the main stack trace from the first process. It is used to
 * get a stack from {@code /data/anr/traces.txt} which can be used to give some context about the
 * ANR. If there is a need, this item can be expanded to store all stacks from all processes.
 * </p>
 */
public class TracesItem extends GenericItem {

    /** Constant for JSON output */
    public static final String PID = "PID";
    /** Constant for JSON output */
    public static final String APP = "APP";
    /** Constant for JSON output */
    public static final String STACK = "STACK";

    private static final Set<String> ATTRIBUTES = new HashSet<String>(Arrays.asList(
            PID, APP, STACK));

    /**
     * The constructor for {@link TracesItem}.
     */
    public TracesItem() {
        super(ATTRIBUTES);
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
     * Get the stack for the crash.
     */
    public String getStack() {
        return (String) getAttribute(STACK);
    }

    /**
     * Set the stack for the crash.
     */
    public void setStack(String stack) {
        setAttribute(STACK, stack);
    }
}
