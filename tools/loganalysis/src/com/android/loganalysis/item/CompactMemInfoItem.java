/*
 * Copyright (C) 2014 The Android Open Source Project
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

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.HashMap;
import java.util.Map;
import java.util.Set;

/**
 * Contains a list of processes and how much memory they are using. Generated from parsing
 * compact mem info file. Refer to CompactMemInfoParser for more details.
 */
public class CompactMemInfoItem implements IItem {
    /** Constants for JSON output */
    public static final String PID_JSON_KEY = "pid";
    public static final String NAME_JSON_KEY = "name";
    public static final String PSS_JSON_KEY = "pss";
    public static final String SWAP_JSON_KEY = "swap";
    public static final String TYPE_JSON_KEY = "type";
    public static final String ACTIVITIES_JSON_KEY = "activities";
    public static final String PROCESSES_JSON_KEY = "processes";
    public static final String LOST_RAM_JSON_KEY = "lostRam";
    public static final String TOTAL_ZRAM_JSON_KEY = "totalZram";
    public static final String FREE_SWAP_ZRAM_JSON_KEY = "freeSwapZram";
    public static final String FREE_RAM_JSON_KEY = "freeRam";
    public static final String TUNING_LEVEL_JSON_KEY = "tuningLevel";
    /** Constants for attributes HashMap */
    private static final String NAME_ATTR_KEY = "name";
    private static final String PSS_ATTR_KEY = "pss";
    private static final String SWAP_ATTR_KEY = "swap";
    private static final String TYPE_ATTR_KEY = "type";
    private static final String ACTIVITIES_ATTR_KEY = "activities";

    private Map<Integer, Map<String, Object>> mPids = new HashMap<Integer, Map<String, Object>>();
    private long mFreeRam;
    private long mFreeSwapZram;
    private long mLostRam;
    private long mTotalZram;
    private long mTuningLevel;

    @Override
    public IItem merge(IItem other) throws ConflictingItemException {
        throw new ConflictingItemException("Compact meminfo items cannot be merged");
    }

    @Override
    public boolean isConsistent(IItem other) {
       return false;
    }

    @Override
    public JSONObject toJson() {
        JSONObject object = new JSONObject();
        JSONArray processes = new JSONArray();
        // Add processes and their attributes
        for(int pid : getPids()) {
            JSONObject proc = new JSONObject();
            try {
                proc.put(PID_JSON_KEY, pid);
                proc.put(NAME_JSON_KEY, getName(pid));
                proc.put(PSS_JSON_KEY, getPss(pid));
                proc.put(SWAP_JSON_KEY, getSwap(pid));
                proc.put(TYPE_JSON_KEY, getType(pid));
                proc.put(ACTIVITIES_JSON_KEY, hasActivities(pid));
                processes.put(proc);
            } catch (JSONException e) {
                // ignore
            }
        }
        try {
            object.put(PROCESSES_JSON_KEY, processes);
            // Add the additional non-process field
            object.put(LOST_RAM_JSON_KEY, getLostRam());
            object.put(TOTAL_ZRAM_JSON_KEY, getTotalZram());
            object.put(FREE_SWAP_ZRAM_JSON_KEY, getFreeSwapZram());
            object.put(FREE_RAM_JSON_KEY, getFreeRam());
            object.put(TUNING_LEVEL_JSON_KEY, getTuningLevel());
        } catch (JSONException e) {
            // ignore
        }

        return object;
    }

    /**
     * Get the list of pids of the processes that were added so far.
     * @return
     */
    public Set<Integer> getPids() {
        return mPids.keySet();
    }

    private Map<String, Object> get(int pid) {
        return mPids.get(pid);
    }

    /**
     * Adds a process to the list stored in this item.
     */
    public void addPid(int pid, String name, String type, long pss, long swap, boolean activities) {
        Map<String, Object> attributes = new HashMap<String, Object>();
        attributes.put(NAME_ATTR_KEY, name);
        attributes.put(TYPE_ATTR_KEY, type);
        attributes.put(PSS_ATTR_KEY, pss);
        attributes.put(SWAP_ATTR_KEY, swap);
        attributes.put(ACTIVITIES_ATTR_KEY, activities);
        mPids.put(pid, attributes);
    }

    /**
     * Returns the name of the process with a given pid.
     */
    public String getName(int pid) {
        return (String)get(pid).get(NAME_ATTR_KEY);
    }

    /**
     * Return pss of the process with a given name.
     */
    public long getPss(int pid) {
        return (Long)get(pid).get(PSS_ATTR_KEY);
    }

    /**
     * Return swap memory of the process with a given name.
     */
    public long getSwap(int pid) {
        return (Long)get(pid).get(SWAP_ATTR_KEY);
    }

    /**
     * Returns the type of the process with a given pid. Some possible types are native, cached,
     * foreground and etc.
     */
    public String getType(int pid) {
        return (String)get(pid).get(TYPE_ATTR_KEY);
    }

    /**
     * Returns true if a process has any activities assosiated with it. False otherwise.
     */
    public boolean hasActivities(int pid) {
        return (Boolean)get(pid).get(ACTIVITIES_ATTR_KEY);
    }

    /**
     * Sets the lost RAM field
     */
    public void setLostRam(long lostRam) {
        mLostRam = lostRam;
    }

    /**
     * Returns the lost RAM field
     */
    public long getLostRam() {
        return mLostRam;
    }

    /** Sets the free RAM field */
    public void setFreeRam(long freeRam) {
        mFreeRam = freeRam;
    }

    /** Returns the free RAM field */
    public long getFreeRam() {
        return mFreeRam;
    }

    /** Sets the total ZRAM field */
    public void setTotalZram(long totalZram) {
        mTotalZram = totalZram;
    }

    /** Returns the total ZRAM field */
    public long getTotalZram() {
        return mTotalZram;
    }

    /** Sets the free swap ZRAM field */
    public void setFreeSwapZram(long freeSwapZram) {
        mFreeSwapZram = freeSwapZram;
    }

    /** Returns the free swap ZRAM field */
    public long getFreeSwapZram() {
        return mFreeSwapZram;
    }

    /** Sets the tuning level field */
    public void setTuningLevel(long tuningLevel) {
        mTuningLevel = tuningLevel;
    }

    /** Returns the tuning level field */
    public long getTuningLevel() {
        return mTuningLevel;
    }
}
