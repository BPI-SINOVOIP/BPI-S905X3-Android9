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
 * An {@link IItem} used to gfxinfo.
 */
public class GfxInfoItem implements IItem {
    /** Constant for JSON output */
    public static final String PROCESSES_KEY = "processes";
    /** Constant for JSON output */
    public static final String PID_KEY = "pid";
    /** Constant for JSON output */
    public static final String NAME_KEY = "name";
    /** Constant for JSON output */
    public static final String TOTAL_FRAMES_KEY = "total_frames";
    /** Constant for JSON output */
    public static final String JANKY_FRAMES_KEY = "janky_frames";
    /** Constant for JSON output */
    public static final String PERCENTILE_90_KEY = "percentile_90";
    /** Constant for JSON output */
    public static final String PERCENTILE_95_KEY = "percentile_95";
    /** Constant for JSON output */
    public static final String PERCENTILE_99_KEY = "percentile_99";

    private Map<Integer, Row> mRows = new HashMap<Integer, Row>();

    private static class Row {
        public String name;
        public long totalFrames;
        public long jankyFrames;
        public int percentile90;
        public int percentile95;
        public int percentile99;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public IItem merge(IItem other) throws ConflictingItemException {
        throw new ConflictingItemException("GfxInfo items cannot be merged");
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
                proc.put(NAME_KEY, getName(pid));
                proc.put(TOTAL_FRAMES_KEY, getTotalFrames(pid));
                proc.put(PERCENTILE_90_KEY, getPrecentile90(pid));
                proc.put(PERCENTILE_95_KEY, getPrecentile95(pid));
                proc.put(PERCENTILE_99_KEY, getPrecentile99(pid));
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
     * Get a set of PIDs seen in the gfxinfo output.
     */
    public Set<Integer> getPids() {
        return mRows.keySet();
    }

    /**
     * Add a row from the gfxinfo output to the {@link GfxInfoItem}.
     *
     * @param pid The PID from the output
     * @param name The process name
     * @param totalFrames The number of total frames rendered by the process
     * @param jankyFrames The number of janky frames rendered by the process
     */
    public void addRow(
            int pid,
            String name,
            long totalFrames,
            long jankyFrames,
            int percentile90,
            int percentile95,
            int percentile99) {
        Row row = new Row();
        row.name = name;
        row.totalFrames = totalFrames;
        row.jankyFrames = jankyFrames;
        row.percentile90 = percentile90;
        row.percentile95 = percentile95;
        row.percentile99 = percentile99;
        mRows.put(pid, row);
    }

    /**
     * Get the process name for a given PID.
     */
    public String getName(int pid) {
        return mRows.get(pid).name;
    }

    /**
     * Get the number of total frames rendered by a given PID.
     */
    public long getTotalFrames(int pid) {
        return mRows.get(pid).totalFrames;
    }

    /**
     * Get the number of janky frames rendered by a given PID.
     */
    public long getJankyFrames(int pid) {
        return mRows.get(pid).jankyFrames;
    }

    /** Get the 90th percentile value of frame times (ms) */
    public int getPrecentile90(int pid) {
        return mRows.get(pid).percentile90;
    }

    /** Get the 95th percentile value of frame times (ms) */
    public int getPrecentile95(int pid) {
        return mRows.get(pid).percentile95;
    }

    /** Get the 99th percentile value of frame times (ms) */
    public int getPrecentile99(int pid) {
        return mRows.get(pid).percentile99;
    }
}
