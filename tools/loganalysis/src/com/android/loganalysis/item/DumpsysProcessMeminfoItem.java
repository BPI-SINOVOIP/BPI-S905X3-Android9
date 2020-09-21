/*
 * Copyright (C) 2018 The Android Open Source Project
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

import org.json.JSONException;
import org.json.JSONObject;

import java.util.HashMap;
import java.util.Map;

/**
 * An {@link IItem} used to store output from `dumpsys meminfo --checkin PROCESS` where PROCESS is
 * from the output of `dumpsys meminfo`. Data is stored as a map of categories to a map of
 * measurement types to values.
 */
public class DumpsysProcessMeminfoItem extends GenericMapItem<Map<String, Long>> {
    // Should match value from ActivityThread
    public static final int ACTIVITY_THREAD_CHECKIN_VERSION = 4;

    // Default Categories
    public static final String NATIVE = "NATIVE";
    public static final String DALVIK = "DALVIK";
    public static final String OTHER = "OTHER";
    public static final String TOTAL = "TOTAL";

    // Memory Measurement Types
    public static final String PSS = "PSS";
    public static final String SWAPPABLE_PSS = "SWAPPABLE_PSS";
    public static final String SHARED_DIRTY = "SHARED_DIRTY";
    public static final String SHARED_CLEAN = "SHARED_CLEAN";
    public static final String PRIVATE_DIRTY = "PRIVATE_DIRTY";
    public static final String PRIVATE_CLEAN = "PRIVATE_CLEAN";
    public static final String SWAPPED_OUT = "SWAPPED_OUT";
    public static final String SWAPPED_OUT_PSS = "SWAPPED_OUT_PSS";
    // NATIVE, DALVIK, TOTAL only
    public static final String MAX = "MAX";
    public static final String ALLOCATED = "ALLOCATED";
    public static final String FREE = "FREE";

    public static final String[] MAIN_OUTPUT_ORDER = {
        MAX,
        ALLOCATED,
        FREE,
        PSS,
        SWAPPABLE_PSS,
        SHARED_DIRTY,
        SHARED_CLEAN,
        PRIVATE_DIRTY,
        PRIVATE_CLEAN,
        SWAPPED_OUT,
        SWAPPED_OUT_PSS
    };
    public static final String[] OTHER_OUTPUT_ORDER = {
        PSS,
        SWAPPABLE_PSS,
        SHARED_DIRTY,
        SHARED_CLEAN,
        PRIVATE_DIRTY,
        PRIVATE_CLEAN,
        SWAPPED_OUT,
        SWAPPED_OUT_PSS
    };

    private int mPid;
    private String mProcessName;

    public DumpsysProcessMeminfoItem() {
        this.put(NATIVE, new HashMap<>());
        this.put(DALVIK, new HashMap<>());
        this.put(OTHER, new HashMap<>());
        this.put(TOTAL, new HashMap<>());
    }

    /** Get the pid */
    public int getPid() {
        return mPid;
    }

    /** Set the pid */
    public void setPid(int pid) {
        mPid = pid;
    }

    /** Get the process name */
    public String getProcessName() {
        return mProcessName;
    }

    /** Set the process name */
    public void setProcessName(String processName) {
        mProcessName = processName;
    }

    /** {@inheritDoc} */
    @Override
    public JSONObject toJson() {
        JSONObject result = super.toJson();
        try {
            result.put("pid", mPid);
            result.put("process_name", mProcessName);
        } catch (JSONException e) {
            //ignore
        }
        return result;
    }
}
