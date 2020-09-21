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

import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;

/**
 * An {@link IItem} used to store wifi dumps.
 */
public class DumpsysWifiStatsItem extends GenericItem {
    /** Constant for JSON output */
    public static final String WIFI_DISCONNECT = "WIFI_DISCONNECT";
    /** Constant for JSON output */
    public static final String WIFI_SCAN = "WIFI_SCAN";
    /** Constant for JSON output */
    public static final String WIFI_ASSOCIATION = "WIFI_ASSOCIATION";

    private static final Set<String> ATTRIBUTES = new HashSet<String>(Arrays.asList(
            WIFI_DISCONNECT, WIFI_SCAN, WIFI_ASSOCIATION));

     /**
     * The constructor for {@link DumpsysWifiStatsItem}.
     */
    public DumpsysWifiStatsItem() {
        super(ATTRIBUTES);
    }

    /**
     * Set number of times of wifi disconnect
     */
    public void setNumWifiDisconnect(int numWifiDisconnects) {
        setAttribute(WIFI_DISCONNECT, numWifiDisconnects);
    }

    /**
     * Set number of times of wifi scan
     */
    public void setNumWifiScan(int numWifiScans) {
        setAttribute(WIFI_SCAN, numWifiScans);
    }

    /**
     * Set number of times of wifi associations
     */
    public void setNumWifiAssociation(int numWifiAssociations) {
        setAttribute(WIFI_ASSOCIATION, numWifiAssociations);
    }

    /**
     * Get the number of times wifi disconnected
     */
    public int getNumWifiDisconnects() {
        return (int) getAttribute(WIFI_DISCONNECT);
    }

    /**
     * Get the number of times wifi scan event triggered
     */
    public int getNumWifiScans() {
        return (int) getAttribute(WIFI_SCAN);
    }

    /**
     * Get the number of times wifi association event triggered
     */
    public int getNumWifiAssociations() {
        return (int) getAttribute(WIFI_ASSOCIATION);
    }
}
