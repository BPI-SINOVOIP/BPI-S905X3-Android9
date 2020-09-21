/*
 * Copyright (C) 2016 The Android Open Source Project
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
 * An {@link IItem} used to store location dumps.
 */
public class LocationDumpsItem implements IItem {

    /** Constant for JSON output */
    public static final String LOCATION_CLIENTS = "LOCATION_CLIENTS";

    private Collection<LocationInfoItem> mLocationClients =
            new LinkedList<LocationInfoItem>();

    public static class LocationInfoItem extends GenericItem {
        /** Constant for JSON output */
        public static final String PACKAGE = "PACKAGE";
        /** Constant for JSON output */
        public static final String EFFECTIVE_INTERVAL = "EFFECTIVE_INTERVAL";
        /** Constant for JSON output */
        public static final String MIN_INTERVAL = "MIN_INTERVAL";
        /** Constant for JSON output */
        public static final String MAX_INTERVAL = "MAX_INTERVAL";
        /** Constant for JSON output */
        public static final String REQUEST_PRIORITY = "PRIORITY";
        /** Constant for JSON output */
        public static final String LOCATION_DURATION = "LOCATION_DURATION";

        private static final Set<String> ATTRIBUTES = new HashSet<String>(Arrays.asList(
                PACKAGE, EFFECTIVE_INTERVAL, MIN_INTERVAL, MAX_INTERVAL, REQUEST_PRIORITY,
                LOCATION_DURATION ));
        /**
         * The constructor for {@link LocationInfoItem}
         *
         * @param packageName The package that requests location
         * @param effective Effective interval of location request
         * @param min Min interval of location request
         * @param max Max interval of location request
         * @param priority The priority of the request
         * @param duration Duration of the request
         */
        public LocationInfoItem(String packageName, int effective, int min, int max,
                String priority, int duration) {
            super(ATTRIBUTES);
            setAttribute(PACKAGE, packageName);
            setAttribute(EFFECTIVE_INTERVAL, effective);
            setAttribute(MIN_INTERVAL, min);
            setAttribute(MAX_INTERVAL, max);
            setAttribute(REQUEST_PRIORITY, priority);
            setAttribute(LOCATION_DURATION, duration);
        }

        /**
         * Get the name of the package
         */
        public String getPackage() {
            return (String) getAttribute(PACKAGE);
        }

        /**
         * Get the effective location interval
         */
        public int getEffectiveInterval() {
            return (int) getAttribute(EFFECTIVE_INTERVAL);
        }

        /**
         * Get the min location interval
         */
        public int getMinInterval() {
            return (int) getAttribute(MIN_INTERVAL);
        }

        /**
         * Get the max location interval
         */
        public int getMaxInterval() {
            return (int) getAttribute(MAX_INTERVAL);
        }

        /**
         * Get the priority of location request
         */
        public String getPriority() {
            return (String) getAttribute(REQUEST_PRIORITY);
        }

        /**
         * Get the location duration
         */
        public int getDuration() {
            return (int) getAttribute(LOCATION_DURATION);
        }

    }

    /**
     * Add a location client {@link LocationDumpsItem}.
     *
     * @param packageName The package that requests location
     * @param effective Effective interval of location request
     * @param min Min interval of location request
     * @param max Max interval of location request
     * @param priority The priority of the request
     * @param duration Duration of the request
     */
    public void addLocationClient(String packageName, int effective, int min, int max,
            String priority, int duration) {
        mLocationClients.add(new LocationInfoItem(packageName, effective, min, max, priority,
                duration));
    }

    public Collection<LocationInfoItem> getLocationClients() {
        return mLocationClients;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public IItem merge(IItem other) throws ConflictingItemException {
        throw new ConflictingItemException("Location dumps items cannot be merged");
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
        if (mLocationClients != null) {
            try {
                JSONArray locationClients = new JSONArray();
                for (LocationInfoItem locationClient : mLocationClients) {
                    locationClients.put(locationClient.toJson());
                }
                object.put(LOCATION_CLIENTS, locationClients);
            } catch (JSONException e) {
                // Ignore
            }
        }

        return object;
    }
}
