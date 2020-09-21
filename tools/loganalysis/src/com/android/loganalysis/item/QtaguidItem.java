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
 * An {@link IItem} used to qtaguid info.
 */
public class QtaguidItem implements IItem {
    /** Constant for JSON output */
    public static final String USERS_KEY = "users";
    /** Constant for JSON output */
    public static final String UID_KEY = "uid";
    /** Constant for JSON output */
    public static final String RX_BYTES_KEY = "rx_bytes";
    /** Constant for JSON output */
    public static final String TX_BYTES_KEY = "tx_bytes";

    private Map<Integer, Row> mRows = new HashMap<Integer, Row>();

    private static class Row {
        public int rxBytes = 0;
        public int txBytes = 0;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public IItem merge(IItem other) throws ConflictingItemException {
        throw new ConflictingItemException("Qtaguid items cannot be merged");
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
        JSONArray users = new JSONArray();
        for (int uid : getUids()) {
            try {
                JSONObject user = new JSONObject();
                user.put(UID_KEY, uid);
                user.put(RX_BYTES_KEY, getRxBytes(uid));
                user.put(TX_BYTES_KEY, getTxBytes(uid));
                users.put(user);
            } catch (JSONException e) {
                // ignore
            }
        }

        try {
            object.put(USERS_KEY, users);
        } catch (JSONException e) {
            // ignore
        }
        return object;
    }

    /**
     * Get a set of UIDs seen in the qtaguid output.
     */
    public Set<Integer> getUids() {
        return mRows.keySet();
    }

    /**
     * Add a row from the qtaguid output to the {@link QtaguidItem}.
     *
     * @param uid The UID from the output
     * @param rxBytes the number of received bytes
     * @param txBytes the number of sent bytes
     */
    public void addRow(int uid, int rxBytes, int txBytes) {
        Row row = new Row();
        row.rxBytes = rxBytes;
        row.txBytes = txBytes;
        mRows.put(uid, row);
    }

    /**
     * Update a row from the qtaguid output to the {@link QtaguidParser}.
     * It adds rxBytes and txBytes to the previously added row.
     * contains(uid) should be true before calling this method.
     *
     * @param uid The UID from the output
     * @param rxBytes the number of received bytes
     * @param txBytes the number of sent bytes
     * @throws IllegalArgumentException if the given UID is not added
     */
    public void updateRow(int uid, int rxBytes, int txBytes) {
        if (!mRows.containsKey(uid)) {
            throw new IllegalArgumentException("Given UID " + uid + " is not added to QtaguidItem");
        }

        Row row = mRows.get(uid);
        row.rxBytes += rxBytes;
        row.txBytes += txBytes;
    }

    /**
     * Returns if the {@link QtaguidItem} contains a given UID.
     */
    public boolean contains(int uid) {
        return mRows.containsKey(uid);
    }

    /**
     * Get the number of received bytes for a given UID.
     */
    public int getRxBytes(int uid) {
        return mRows.get(uid).rxBytes;
    }

    /**
     * Get the number of sent bytes for a given UID.
     */
    public int getTxBytes(int uid) {
        return mRows.get(uid).txBytes;
    }
}
