/*
 * Copyright (C) 2015 The Android Open Source Project
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
 * An {@link IItem} used to cpuinfo.
 */
public class CpuInfoItem implements IItem {
    /** Constant for JSON output */
    public static final String PROCESSES_KEY = "processes";
    /** Constant for JSON output */
    public static final String PID_KEY = "pid";
    /** Constant for JSON output */
    public static final String PERCENT_KEY = "percent";
    /** Constant for JSON output */
    public static final String NAME_KEY = "name";

    private Map<Integer, Row> mRows = new HashMap<Integer, Row>();

    private static class Row {
        public double percent;
        public String name;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public IItem merge(IItem other) throws ConflictingItemException {
        throw new ConflictingItemException("CpuInfo items cannot be merged");
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean isConsistent(IItem other) {
        return false;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public JSONObject toJson() {
        JSONObject object = new JSONObject();
        JSONArray processes = new JSONArray();
        for (int pid : getPids()) {
            JSONObject proc = new JSONObject();
            try {
                proc.put(PID_KEY, pid);
                proc.put(PERCENT_KEY, getPercent(pid));
                proc.put(NAME_KEY, getName(pid));
                processes.put(proc);
            } catch (JSONException e) {
                // ignore
            }
        }

        try {
            object.put(PROCESSES_KEY, processes);
        } catch (JSONException e) {
            // ignore
        }
        return object;
    }

    /**
     * Get a set of PIDs seen in the cpuinfo output.
     */
    public Set<Integer> getPids() {
        return mRows.keySet();
    }

    /**
     * Add a row from the cpuinfo output to the {@link CpuInfoItem}.
     *
     * @param pid The PID from the output
     * @param percent The percentage of CPU usage by the process
     * @param name The process name
     */
    public void addRow(int pid, double percent, String name) {
        Row row = new Row();
        row.percent = percent;
        row.name = name;
        mRows.put(pid, row);
    }

    /**
     * Get the percentage of CPU usage by a given PID.
     */
    public double getPercent(int pid) {
        return mRows.get(pid).percent;
    }

    /**
     * Get the process name for a given PID.
     */
    public String getName(int pid) {
        return mRows.get(pid).name;
    }
}
