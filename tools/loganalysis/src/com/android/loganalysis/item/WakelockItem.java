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

import java.util.Arrays;
import java.util.Collection;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Set;

/**
 * An {@link IItem} used to store information related to wake locks and kernel wake locks
 */
public class WakelockItem implements IItem {

    /** Constant for JSON output */
    public static final String WAKELOCKS = "WAKELOCKS_INFO";

    private Collection<WakelockInfoItem> mWakeLocks = new LinkedList<WakelockInfoItem>();

    /**
     * Enum for describing the type of wakelock
     */
    public enum WakeLockCategory {
        KERNEL_WAKELOCK,
        PARTIAL_WAKELOCK,
    }

    public static class WakelockInfoItem extends GenericItem {
        /** Constant for JSON output */
        public static final String NAME = "NAME";
        /** Constant for JSON output */
        public static final String PROCESS_UID = "PROCESS_UID";
        /** Constant for JSON output */
        public static final String PROCESS_NAME = "PROCESS_NAME";
        /** Constant for JSON output */
        public static final String HELD_TIME = "HELD_TIME";
        /** Constant for JSON output */
        public static final String LOCKED_COUNT = "LOCKED_COUNT";
        /** Constant for JSON output */
        public static final String CATEGORY = "CATEGORY";

        private static final Set<String> ATTRIBUTES = new HashSet<String>(Arrays.asList(
                NAME, PROCESS_UID, PROCESS_NAME, HELD_TIME, LOCKED_COUNT, CATEGORY));

        /**
         * The constructor for {@link WakelockItem}
         *
         * @param name The name of the wake lock
         * @param heldTime The amount of time held in milliseconds
         * @param lockedCount The number of times the wake lock was locked
         * @param category The {@link WakeLockCategory} of the wake lock
         */
        public WakelockInfoItem(String name, long heldTime, int lockedCount, WakeLockCategory category) {
            this(name, null, heldTime, lockedCount, category);
        }

        /**
         * The constructor for {@link WakelockItem}
         *
         * @param name The name of the wake lock
         * @param processUID The number of the wake lock
         * @param heldTime The amount of time held in milliseconds
         * @param lockedCount The number of times the wake lock was locked
         * @param category The {@link WakeLockCategory} of the wake lock
         */
        public WakelockInfoItem(String name, String processUID, long heldTime, int lockedCount,
                WakeLockCategory category) {
            super(ATTRIBUTES);

            setAttribute(NAME, name);
            setAttribute(PROCESS_UID, processUID);
            setAttribute(HELD_TIME, heldTime);
            setAttribute(LOCKED_COUNT, lockedCount);
            setAttribute(CATEGORY, category);
        }

        /**
         * Get the name of the wake lock.
         */
        public String getName() {
            return (String) getAttribute(NAME);
        }

        /**
         * Get the process UID holding the wake lock.
         */
        public String getProcessUID() {
            return (String) getAttribute(PROCESS_UID);
        }

        /**
         * Get the time the wake lock was held in milliseconds.
         */
        public long getHeldTime() {
            return (Long) getAttribute(HELD_TIME);
        }

        /**
         * Get the number of times the wake lock was locked.
         */
        public int getLockedCount() {
            return (Integer) getAttribute(LOCKED_COUNT);
        }

        /**
         * Get the {@link WakeLockCategory} of the wake lock.
         */
        public WakeLockCategory getCategory() {
            return (WakeLockCategory) getAttribute(CATEGORY);
        }

        /**
         * Set the process name holding the wake lock
         */
        public void setWakelockProcessName(String processName) {
            setAttribute(PROCESS_NAME, processName);
        }
    }

    /**
     * Add a wakelock from the battery stats section.
     *
     * @param name The name of the wake lock.
     * @param processUID The number of the wake lock.
     * @param heldTime The held time of the wake lock.
     * @param timesCalled The number of times the wake lock has been called.
     * @param category The {@link WakeLockCategory} of the wake lock.
     */
    public void addWakeLock(String name, String processUID, long heldTime, int timesCalled,
            WakeLockCategory category) {
        mWakeLocks.add(new WakelockInfoItem(name, processUID, heldTime, timesCalled, category));
    }

    /**
     * Add a wakelock from the battery stats section.
     *
     * @param name The name of the wake lock.
     * @param heldTime The held time of the wake lock.
     * @param timesCalled The number of times the wake lock has been called.
     * @param category The {@link WakeLockCategory} of the wake lock.
     */
    public void addWakeLock(String name, long heldTime, int timesCalled,
            WakeLockCategory category) {
        addWakeLock(name, null, heldTime, timesCalled, category);
    }

    /**
     * Get a list of {@link WakelockInfoItem} objects matching a given {@link WakeLockCategory}.
     */
    public List<WakelockInfoItem> getWakeLocks(WakeLockCategory category) {
        LinkedList<WakelockInfoItem> wakeLocks = new LinkedList<WakelockInfoItem>();
        if (category == null) {
            return wakeLocks;
        }

        for (WakelockInfoItem wakeLock : mWakeLocks) {
            if (category.equals(wakeLock.getCategory())) {
                wakeLocks.add(wakeLock);
            }
        }
        return wakeLocks;
    }

    /**
     * Get a list of {@link WakelockInfoItem} .
     */
    public List<WakelockInfoItem> getWakeLocks() {
        LinkedList<WakelockInfoItem> wakeLocks = new LinkedList<WakelockInfoItem>();

        for (WakelockInfoItem wakeLock : mWakeLocks) {
            wakeLocks.add(wakeLock);
        }
        return wakeLocks;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public IItem merge(IItem other) throws ConflictingItemException {
        throw new ConflictingItemException("Wakelock items cannot be merged");
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
        if (mWakeLocks != null) {
            try {
                JSONArray wakeLocks = new JSONArray();
                for (WakelockInfoItem wakeLock : mWakeLocks) {
                    wakeLocks.put(wakeLock.toJson());
                }
                object.put(WAKELOCKS, wakeLocks);
            } catch (JSONException e) {
                // Ignore
            }
        }
        return object;
    }
}
