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
import java.util.Set;

/**
 * An {@link IItem} used to store information related to network bandwidth, sensor usage,
 * alarm usage by each processes
 */
public class ProcessUsageItem implements IItem {

    /** Constant for JSON output */
    public static final String PROCESS_USAGE = "PROCESS_USAGE";

    private Collection<ProcessUsageInfoItem> mProcessUsage =
            new LinkedList<ProcessUsageInfoItem>();

    public static class SensorInfoItem extends GenericItem {
        /** Constant for JSON output */
        public static final String  SENSOR_NAME = "SENSOR_NAME";
        /** Constant for JSON output */
        public static final String USAGE_DURATION = "USAGE_DURATION";

        private static final Set<String> ATTRIBUTES = new HashSet<String>(Arrays.asList(
            SENSOR_NAME, USAGE_DURATION));

        /**
         * The constructor for {@link SensorInfoItem}
         *
         * @param name The name of the sensor
         * @param usageDuration Duration of the usage
         */
        public SensorInfoItem(String name, long usageDuration) {
            super(ATTRIBUTES);

            setAttribute(SENSOR_NAME, name);
            setAttribute(USAGE_DURATION, usageDuration);
        }

        /**
         * Get the sensor name
         */
        public String getSensorName() {
            return (String) getAttribute(SENSOR_NAME);
        }

        /**
         * Get the sensor usage duration in milliseconds
         */
        public long getUsageDurationMs() {
            return (long) getAttribute(USAGE_DURATION);
        }
    }

    public static class ProcessUsageInfoItem extends GenericItem {
        /** Constant for JSON output */
        public static final String  ALARM_WAKEUPS = "ALARM_WAKEUPS";
        /** Constant for JSON output */
        public static final String SENSOR_USAGE = "SENSOR_USAGE";
        /** Constant for JSON output */
        public static final String  PROCESS_UID = "PROCESS_UID";

        private static final Set<String> ATTRIBUTES = new HashSet<String>(Arrays.asList(
                ALARM_WAKEUPS, SENSOR_USAGE, PROCESS_UID));

        /**
         * The constructor for {@link ProcessUsageItem}
         *
         * @param uid The name of the process
         * @param alarmWakeups Number of alarm wakeups
         * @param sensorUsage Different sensors used by the process
         */
        public ProcessUsageInfoItem(String uid, int alarmWakeups,
                LinkedList<SensorInfoItem> sensorUsage) {
            super(ATTRIBUTES);

            setAttribute(PROCESS_UID, uid);
            setAttribute(ALARM_WAKEUPS, alarmWakeups);
            setAttribute(SENSOR_USAGE, sensorUsage);
        }

        /**
         * Get the number of Alarm wakeups
         */
        public int getAlarmWakeups() {
            return (int) getAttribute(ALARM_WAKEUPS);
        }

        /**
         * Get the Sensor usage of the process
         */
        @SuppressWarnings("unchecked")
        public LinkedList<SensorInfoItem> getSensorUsage() {
            return (LinkedList<SensorInfoItem>) getAttribute(SENSOR_USAGE);
        }

        /**
         * Get the process name
         */
        public String getProcessUID() {
            return (String) getAttribute(PROCESS_UID);
        }

        /**
         * {@inheritDoc}
         */
        @Override
        public JSONObject toJson() {
            JSONObject object = new JSONObject();
            try {
                object.put(PROCESS_UID, getProcessUID());
                JSONArray sensorUsage = new JSONArray();
                for (SensorInfoItem usage : getSensorUsage()) {
                    sensorUsage.put(usage.toJson());
                }
                object.put(SENSOR_USAGE, sensorUsage);
                object.put(ALARM_WAKEUPS, getAlarmWakeups());

            } catch (JSONException e) {
                // Ignore
            }
            return object;
        }
    }

    /**
     * Add individual process usage from the battery stats section.
     *
     * @param processUID The name of the process
     * @param alarmWakeups The number of alarm wakeups
     * @param sensorUsage Sensor usage of the process
     */
    public void addProcessUsage(String processUID, int alarmWakeups,
            LinkedList<SensorInfoItem> sensorUsage) {
        mProcessUsage.add(new ProcessUsageInfoItem(processUID, alarmWakeups, sensorUsage));
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

    public Collection<ProcessUsageInfoItem> getProcessUsage() {
        return mProcessUsage;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public JSONObject toJson() {
        JSONObject object = new JSONObject();
        if (mProcessUsage != null) {
            try {
                JSONArray processUsage = new JSONArray();
                for (ProcessUsageInfoItem usage : mProcessUsage) {
                    processUsage.put(usage.toJson());
                }
                object.put(PROCESS_USAGE, processUsage);
            } catch (JSONException e) {
                // Ignore
            }
        }
        return object;
    }
}
