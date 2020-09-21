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
 * An {@link IItem} used to store information related to battery usage stats
 */
public class BatteryUsageItem implements IItem {

    /** Constant for JSON output */
    public static final String BATTERY_USAGE = "BATTERY_USAGE";
    /** Constant for JSON output */
    public static final String BATTERY_CAPACITY = "BATTERY_CAPACITY";

    private Collection<BatteryUsageInfoItem> mBatteryUsage = new LinkedList<BatteryUsageInfoItem>();

    private int mBatteryCapacity = 0;

    public static class BatteryUsageInfoItem extends GenericItem {
        /** Constant for JSON output */
        public static final String NAME = "NAME";
        /** Constant for JSON output */
        public static final String USAGE = "USAGE";

        private static final Set<String> ATTRIBUTES = new HashSet<String>(Arrays.asList(
                NAME, USAGE));

        /**
         * The constructor for {@link BatteryUsageItem}
         *
         * @param name The name of the wake lock
         * @param usage Usage in mAh
         */
        public BatteryUsageInfoItem(String name, double usage) {
            super(ATTRIBUTES);
            setAttribute(NAME, name);
            setAttribute(USAGE, usage);
        }

        /**
         * Get the name of the wake lock.
         */
        public String getName() {
            return (String) getAttribute(NAME);
        }

        /**
         * Get the battery usage
         */
        public double getUsage() {
            return (double) getAttribute(USAGE);
        }
    }

    /**
     * Add a battery usage from the battery stats section.
     *
     * @param name The name of the process
     * @param usage The usage in mAh
     */
    public void addBatteryUsage(String name, double usage) {
        mBatteryUsage.add(new BatteryUsageInfoItem(name, usage));
    }

    public int getBatteryCapacity() {
        return mBatteryCapacity;
    }

    public List<BatteryUsageInfoItem> getBatteryUsage() {
        return (List<BatteryUsageInfoItem>)mBatteryUsage;
    }

    public void setBatteryCapacity(int capacity) {
        mBatteryCapacity = capacity;
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
        if (mBatteryUsage != null) {
            try {
                object.put(BATTERY_CAPACITY, mBatteryCapacity);
                JSONArray usageInfo = new JSONArray();
                for (BatteryUsageInfoItem usage : mBatteryUsage) {
                    usageInfo.put(usage.toJson());
                }
                object.put(BATTERY_USAGE, usageInfo);
            } catch (JSONException e) {
                // Ignore
            }
        }
        return object;
    }
}
