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

import com.android.loganalysis.item.CpuInfoItem;

import junit.framework.TestCase;

import java.util.Arrays;
import java.util.List;

public class CpuInfoParserTest extends TestCase {

    public void testSingleLine() {
        List<String> input = Arrays.asList("  0.1% 170/surfaceflinger: 0% user + 0% kernel");

        CpuInfoItem item = new CpuInfoParser().parse(input);

        assertEquals(1, item.getPids().size());
        assertEquals("surfaceflinger", item.getName(170));
        assertEquals(0.1, item.getPercent(170), 0.0001);
    }

    public void testMultipleLines() {
        List<String> input = Arrays.asList(
                "CPU usage from 35935ms to 26370ms ago:",
                "  57% 489/system_server: 37% user + 20% kernel / faults: 39754 minor 57 major",
                "  34% 853/com.google.android.leanbacklauncher: 30% user + 4.6% kernel / faults: 7838 minor 14 major",
                "  15% 19463/com.google.android.videos: 11% user + 3.3% kernel / faults: 21603 minor 141 major",
                "  8.2% 170/surfaceflinger: 3.4% user + 4.8% kernel / faults: 1 minor");
        CpuInfoItem item = new CpuInfoParser().parse(input);

        assertEquals(4, item.getPids().size());
        assertEquals("system_server", item.getName(489));
        assertEquals(57.0, item.getPercent(489), 0.0001);
        assertEquals("surfaceflinger", item.getName(170));
        assertEquals(8.2, item.getPercent(170), 0.0001);
    }

}

