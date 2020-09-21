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

import com.android.loganalysis.item.LocationDumpsItem;
import com.android.loganalysis.item.LocationDumpsItem.LocationInfoItem;

import junit.framework.TestCase;

import java.util.Arrays;
import java.util.List;

/**
 * Unit tests for {@link LocationServiceParser}
 */
public class LocationServiceParserTest extends TestCase {

    /**
     * Test that normal input is parsed.
     */
    public void testLocationClientsSize() {
        List<String> inputBlock = Arrays.asList(
               " Location Request History By Package:",
               " Interval effective/min/max 1/0/0[s] Duration: 140[minutes] "
               + "[com.google.android.gms, PRIORITY_NO_POWER, UserLocationProducer] "
               + "Num requests: 2 Active: true",
               " Interval effective/min/max 284/285/3600[s] Duration: 140[minutes] "
               + "[com.google.android.googlequicksearchbox, PRIORITY_BALANCED_POWER_ACCURACY] "
               + "Num requests: 5 Active: true",
               "  Interval effective/min/max 0/0/0[s] Duration: 0[minutes] "
               + "[com.google.android.apps.walletnfcrel, PRIORITY_BALANCED_POWER_ACCURACY] "
               + "Num requests: 1 Active: false",
               "  ",
               " FLP WakeLock Count");

        LocationDumpsItem locationClients = new LocationServiceParser().parse(inputBlock);
        assertNotNull(locationClients.getLocationClients());
        assertEquals(locationClients.getLocationClients().size(), 3);
    }

    /**
     * Test that normal input is parsed.
     */
    public void testLocationClientParser() {
        List<String> inputBlock = Arrays.asList(
               " Location Request History By Package:",
               " Interval effective/min/max 1/0/0[s] Duration: 140[minutes] "
               + "[com.google.android.gms, PRIORITY_NO_POWER, UserLocationProducer] "
               + "Num requests: 2 Active: true");

        LocationDumpsItem locationClients = new LocationServiceParser().parse(inputBlock);
        assertNotNull(locationClients.getLocationClients());
        LocationInfoItem client = locationClients.getLocationClients().iterator().next();
        assertEquals(client.getPackage(), "com.google.android.gms");
        assertEquals(client.getEffectiveInterval(), 1);
        assertEquals(client.getMinInterval(), 0);
        assertEquals(client.getMaxInterval(), 0);
        assertEquals(client.getPriority(), "PRIORITY_NO_POWER");
        assertEquals(client.getDuration(), 140);
    }

    /**
     * Test that invalid input is parsed.
     */
    public void testLocationClientParserInvalidInput() {
        List<String> inputBlock = Arrays.asList(
                " Location Request History By Package:",
                " Interval effective/min/max 1/0/0[s] Duration: 140[minutes] "
                + "[com.google.android.gms PRIORITY_NO_POWER UserLocationProducer] "
                + "Num requests: 2 Active: true");
        LocationDumpsItem locationClients = new LocationServiceParser().parse(inputBlock);
        assertEquals(locationClients.getLocationClients().size(), 0);
    }

}

