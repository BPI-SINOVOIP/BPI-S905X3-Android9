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

package com.android.loganalysis.rule;

import com.android.loganalysis.item.BugreportItem;
import com.android.loganalysis.item.LocationDumpsItem;
import com.android.loganalysis.item.LocationDumpsItem.LocationInfoItem;

import java.util.concurrent.TimeUnit;
import java.util.ArrayList;
import java.util.List;

import org.json.JSONException;
import org.json.JSONObject;


/**
 * Rules definition for Location usage
 */
public class LocationUsageRule extends AbstractPowerRule {

    private static final String LOCATION_USAGE_ANALYSIS = "LOCATION_USAGE_ANALYSIS";
    private static final float LOCATION_REQUEST_DURATION_THRESHOLD = 0.1f; // 10%
    // GSA requests for location every 285 seconds, anything more frequent is an issue
    private static final int LOCATION_INTERVAL_THRESHOLD = 285;

    private List<LocationInfoItem> mOffendingLocationRequestList;
    private BugreportItem mBugreportItem;

    public LocationUsageRule (BugreportItem bugreportItem) {
        super(bugreportItem);
        mBugreportItem = bugreportItem;
    }

    @Override
    public void applyRule() {
        mOffendingLocationRequestList = new ArrayList<LocationInfoItem>();
        if (mBugreportItem.getActivityService() == null || getTimeOnBattery() <= 0) {
            return;
        }
        LocationDumpsItem locationDumpsItem = mBugreportItem.getActivityService()
                .getLocationDumps();
        if (locationDumpsItem == null) {
            return;
        }
        final long locationRequestThresholdMs = (long) (getTimeOnBattery() *
                LOCATION_REQUEST_DURATION_THRESHOLD);

        for (LocationInfoItem locationClient : locationDumpsItem.getLocationClients()) {
            final String priority = locationClient.getPriority();
            final int effectiveIntervalSec = locationClient.getEffectiveInterval();
            if (effectiveIntervalSec < LOCATION_INTERVAL_THRESHOLD &&
                    !priority.equals("PRIORITY_NO_POWER") &&
                    (TimeUnit.MINUTES.toMillis(locationClient.getDuration()) >
                    locationRequestThresholdMs)) {
                mOffendingLocationRequestList.add(locationClient);
            }
        }
    }

    @Override
    public JSONObject getAnalysis() {
        JSONObject locationAnalysis = new JSONObject();
        StringBuilder analysis = new StringBuilder();
        if (mOffendingLocationRequestList == null || mOffendingLocationRequestList.size() <= 0) {
            analysis.append("No apps requested for frequent location updates. ");
        } else {
            for (LocationInfoItem locationClient : mOffendingLocationRequestList) {
                analysis.append(String.format("Package %s is requesting for location "
                        +"updates every %d secs with priority %s.", locationClient.getPackage(),
                        locationClient.getEffectiveInterval(), locationClient.getPriority()));
            }
        }
        try {
            locationAnalysis.put(LOCATION_USAGE_ANALYSIS, analysis.toString().trim());
        } catch (JSONException e) {
          // do nothing
        }
        return locationAnalysis;
    }
}
