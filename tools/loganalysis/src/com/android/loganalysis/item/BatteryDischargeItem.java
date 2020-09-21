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
import java.util.Calendar;
import java.util.Collection;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.Set;

/**
 * An {@link IItem} used to store information related to Battery discharge
 */
public class BatteryDischargeItem implements IItem {

    /** Constant for JSON output */
    public static final String BATTERY_DISCHARGE = "BATTERY_DISCHARGE";

    private Collection<BatteryDischargeInfoItem> mBatteryDischargeInfo =
            new LinkedList<BatteryDischargeInfoItem>();

    public static class BatteryDischargeInfoItem extends GenericItem {
        /** Constant for JSON output */
        public static final String CLOCK_TIME_OF_DISCHARGE = "CLOCK_TIME_OF_DISCHARGE";
        /** Constant for JSON output */
        public static final String DISCHARGE_ELAPSED_TIME = "DISCHARGE_ELAPSED_TIME";
        /** Constant for JSON output */
        public static final String BATTERY_LEVEL = "BATTERY_LEVEL";

        private static final Set<String> ATTRIBUTES = new HashSet<String>(Arrays.asList(
                CLOCK_TIME_OF_DISCHARGE, DISCHARGE_ELAPSED_TIME, BATTERY_LEVEL));

        /**
         * The constructor for {@link BatteryDischargeInfoItem}
         *
         * @param clockTime Clock time when the battery discharge happened
         * @param elapsedTime Time it took to discharge to the current battery level
         * @param batteryLevel Current battery level
         */
        public BatteryDischargeInfoItem(Calendar clockTime, long elapsedTime, int batteryLevel) {
            super(ATTRIBUTES);

            setAttribute(CLOCK_TIME_OF_DISCHARGE, clockTime);
            setAttribute(DISCHARGE_ELAPSED_TIME, elapsedTime);
            setAttribute(BATTERY_LEVEL, batteryLevel);
        }

        /**
         * Get the clock time when the battery level dropped
         */
        public Calendar getClockTime() {
            return (Calendar) getAttribute(CLOCK_TIME_OF_DISCHARGE);
        }

        /**
         * Get the time elapsed to discharge to the current battery level
         */
        public long getElapsedTime() {
            return (long) getAttribute(DISCHARGE_ELAPSED_TIME);
        }

        /**
         * Get the current battery level
         */
        public int getBatteryLevel() {
            return (int) getAttribute(BATTERY_LEVEL);
        }
    }

    /**
     * Add a battery discharge step from battery stats
     *
     * @param clockTime Clock time when the battery discharge happened
     * @param elapsedTime Time it took to discharge to the current battery level
     * @param batteryLevel Current battery level
     */
    public void addBatteryDischargeInfo(Calendar clockTime, long elapsedTime, int batteryLevel) {
        mBatteryDischargeInfo.add(new BatteryDischargeInfoItem(clockTime,
                elapsedTime, batteryLevel));
    }

    public Collection<BatteryDischargeInfoItem> getDischargeStepsInfo() {
        return mBatteryDischargeInfo;
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
        try {
            JSONArray batteryDischargeSteps = new JSONArray();
            for (BatteryDischargeInfoItem batteryDischargeStep : mBatteryDischargeInfo) {
                batteryDischargeSteps.put(batteryDischargeStep.toJson());
            }
            object.put(BATTERY_DISCHARGE, batteryDischargeSteps);
        } catch (JSONException e) {
            // Ignore
        }
        return object;
    }
}
