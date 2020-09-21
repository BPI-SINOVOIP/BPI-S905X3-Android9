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

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.HashMap;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Set;


/**
 * An {@link IItem} used to procrank info.
 */
public class ProcrankItem implements IItem {
    public static final String TYPE = "PROCRANK";

    /** Constant for JSON output */
    public static final String LINES = "LINES";
    /** Constant for JSON output */
    public static final String PID = "PID";
    /** Constant for JSON output */
    public static final String PROCESS_NAME = "PROCESS_NAME";
    /** Constant for JSON output */
    public static final String VSS = "VSS";
    /** Constant for JSON output */
    public static final String RSS = "RSS";
    /** Constant for JSON output */
    public static final String PSS = "PSS";
    /** Constant for JSON output */
    public static final String USS = "USS";
    /** Constant for JSON output */
    public static final String TEXT = "TEXT";

    private class ProcrankValue {
        public String mProcessName;
        public int mVss;
        public int mRss;
        public int mPss;
        public int mUss;

        public ProcrankValue(String processName, int vss, int rss, int pss, int uss) {
            mProcessName = processName;
            mVss = vss;
            mRss = rss;
            mPss = pss;
            mUss = uss;
        }
    }

    private String mText = null;
    private Map<Integer, ProcrankValue> mProcrankLines = new HashMap<Integer, ProcrankValue>();

    /**
     * Add a line from the procrank output to the {@link ProcrankItem}.
     *
     * @param pid The PID from the output
     * @param processName The process name from the cmdline column
     * @param vss The VSS in KB
     * @param rss The RSS in KB
     * @param pss The PSS in KB
     * @param uss The USS in KB
     */
    public void addProcrankLine(int pid, String processName, int vss, int rss, int pss, int uss) {
        mProcrankLines.put(pid, new ProcrankValue(processName, vss, rss, pss, uss));
    }

    /**
     * Get a set of PIDs seen in the procrank output.
     */
    public Set<Integer> getPids() {
        return mProcrankLines.keySet();
    }

    /**
     * Get the process name for a given PID.
     */
    public String getProcessName(int pid) {
        if (!mProcrankLines.containsKey(pid)) {
            return null;
        }

        return mProcrankLines.get(pid).mProcessName;
    }

    /**
     * Get the VSS for a given PID.
     */
    public Integer getVss(int pid) {
        if (!mProcrankLines.containsKey(pid)) {
            return null;
        }

        return mProcrankLines.get(pid).mVss;
    }

    /**
     * Get the RSS for a given PID.
     */
    public Integer getRss(int pid) {
        if (!mProcrankLines.containsKey(pid)) {
            return null;
        }

        return mProcrankLines.get(pid).mRss;
    }

    /**
     * Get the PSS for a given PID.
     */
    public Integer getPss(int pid) {
        if (!mProcrankLines.containsKey(pid)) {
            return null;
        }

        return mProcrankLines.get(pid).mPss;
    }

    /**
     * Get the USS for a given PID.
     */
    public Integer getUss(int pid) {
        if (!mProcrankLines.containsKey(pid)) {
            return null;
        }

        return mProcrankLines.get(pid).mUss;
    }

    /**
     * Get the raw text of the procrank command.
     */
    public String getText() {
        return mText;
    }

    /**
     * Set the raw text of the procrank command.
     */
    public void setText(String text) {
        mText = text;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public IItem merge(IItem other) throws ConflictingItemException {
        throw new ConflictingItemException("Procrank items cannot be merged");
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
        JSONArray lines = new JSONArray();
        try {
            for (Entry<Integer, ProcrankValue> entry : mProcrankLines.entrySet()) {
                final ProcrankValue procrankValue = entry.getValue();
                JSONObject line = new JSONObject();
                line.put(PID, entry.getKey());
                line.put(PROCESS_NAME, procrankValue.mProcessName);
                line.put(VSS, procrankValue.mVss);
                line.put(RSS, procrankValue.mRss);
                line.put(PSS, procrankValue.mPss);
                line.put(USS, procrankValue.mUss);
                lines.put(line);
            }
            object.put(LINES, lines);
            object.put(TEXT, getText());
        } catch (JSONException e) {
            // Ignore
        }
        return object;
    }
}
