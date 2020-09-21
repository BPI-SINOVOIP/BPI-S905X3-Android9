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
 * An {@link IItem} used to store Activity Service Dumps
 */
public class ActivityServiceItem extends GenericItem {

    /** Constant for JSON output */
    public static final String LOCATION_DUMPS = "LOCATION";

    private static final Set<String> ATTRIBUTES = new HashSet<String>(Arrays.asList(
            LOCATION_DUMPS));

    /**
     * The constructor for {@link ActivityServiceItem}.
     */
    public ActivityServiceItem() {
        super(ATTRIBUTES);
    }

    /**
     * Get the location dump
     */
    public LocationDumpsItem getLocationDumps() {
        return (LocationDumpsItem) getAttribute(LOCATION_DUMPS);
    }

    /**
     * Set the location dump
     */
    public void setLocationDumps(LocationDumpsItem location) {
        setAttribute(LOCATION_DUMPS, location);
    }
}
