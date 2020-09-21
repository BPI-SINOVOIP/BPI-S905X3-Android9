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
package com.android.loganalysis.parser;

import com.android.loganalysis.item.BatteryUsageItem;

import junit.framework.TestCase;

import java.util.Arrays;
import java.util.List;

/**
 * Unit tests for {@link BatteryUsageParser}
 */
public class BatteryUsageParserTest extends TestCase {

    private static final double EPSILON = 1e-3;
    /**
     * Test that normal input is parsed.
     */
    public void testBatteryUsageParser() {
        List<String> inputBlock = Arrays.asList(
                "    Capacity: 3220, Computed drain: 11.0, actual drain: 0",
                "    Screen: 8.93",
                "    Idle: 1.23",
                "    Uid 0: 0.281",
                "    Uid u0a36: 0.200",
                "    Uid 1000: 0.165",
                "    Uid 1013: 0.0911",
                "    Uid u0a16: 0.0441");

        BatteryUsageItem usage = new BatteryUsageParser().parse(inputBlock);

        assertEquals(3220, usage.getBatteryCapacity());
        assertEquals(7, usage.getBatteryUsage().size());

        assertEquals("Screen", usage.getBatteryUsage().get(0).getName());
        assertEquals(8.93, usage.getBatteryUsage().get(0).getUsage(), EPSILON);
        assertEquals("Uid u0a16", usage.getBatteryUsage().get(6).getName());
        assertEquals(0.0441, usage.getBatteryUsage().get(6).getUsage());
    }
}

