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
package com.android.loganalysis.parser;

import com.android.loganalysis.item.ActivityServiceItem;

import junit.framework.TestCase;

import java.util.Arrays;
import java.util.List;

/**
 * Unit tests for {@link ActivityServiceParser}
 */
public class ActivityServiceParserTest extends TestCase {

    /**
     * Test that normal input is parsed.
     */
    public void testActivityServiceParser() {
        List<String> inputBlock = Arrays.asList(
                "SERVICE com.google.android.gms/"
                + "com.google.android.location.internal.GoogleLocationManagerService f4c9e9d "
                + "pid=1494",
                "Client:",
                " nothing to dump",
                "Location Request History By Package:",
                "Interval effective/min/max 1/0/0[s] Duration: 140[minutes] "
                + "[com.google.android.gms, PRIORITY_NO_POWER, UserLocationProducer] "
                + "Num requests: 2 Active: true",
                "Interval effective/min/max 284/285/3600[s] Duration: 140[minutes] "
                + "[com.google.android.googlequicksearchbox, PRIORITY_BALANCED_POWER_ACCURACY] "
                + "Num requests: 5 Active: true",
                "FLP WakeLock Count:",
                "SERVICE com.android.server.telecom/.components.BluetoothPhoneService 98ab pid=802",
                "Interval effective/min/max 1/0/0[s] Duration: 140[minutes] "
                + "[com.google.android.gms, PRIORITY_NO_POWER, UserLocationProducer] "
                + "Num requests: 2 Active: true",
                "");

        ActivityServiceItem activityService = new ActivityServiceParser().parse(inputBlock);
        assertNotNull(activityService.getLocationDumps());
        assertEquals(activityService.getLocationDumps().getLocationClients().size(), 2);
    }
}

