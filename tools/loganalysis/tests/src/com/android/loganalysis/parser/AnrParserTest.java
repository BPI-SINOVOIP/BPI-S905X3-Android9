/*
 * Copyright (C) 2012 The Android Open Source Project
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

import com.android.loganalysis.item.AnrItem;
import com.android.loganalysis.util.ArrayUtil;

import junit.framework.TestCase;

import java.util.Arrays;
import java.util.List;

/**
 * Unit tests for {@link AnrParser}.
 */
public class AnrParserTest extends TestCase {

    /**
     * Test that ANRs are parsed for the header "ANR (application not responding) in process: app"
     */
    public void testParse_application_not_responding() {
        List<String> lines = Arrays.asList(
                "ANR (application not responding) in process: com.android.package",
                "Reason: keyDispatchingTimedOut",
                "Load: 0.71 / 0.83 / 0.51",
                "CPU usage from 4357ms to -1434ms ago:",
                "  22% 3378/com.android.package: 19% user + 3.6% kernel / faults: 73 minor 1 major",
                "  16% 312/system_server: 12% user + 4.1% kernel / faults: 1082 minor 6 major",
                "33% TOTAL: 21% user + 11% kernel + 0.3% iowait",
                "CPU usage from 907ms to 1431ms later:",
                "  14% 121/mediaserver: 11% user + 3.7% kernel / faults: 17 minor",
                "    3.7% 183/AudioOut_2: 3.7% user + 0% kernel",
                "  12% 312/system_server: 5.5% user + 7.4% kernel / faults: 6 minor",
                "    5.5% 366/InputDispatcher: 0% user + 5.5% kernel",
                "18% TOTAL: 11% user + 7.5% kernel");

        AnrItem anr = new AnrParser().parse(lines);
        assertNotNull(anr);
        assertEquals("com.android.package", anr.getApp());
        assertEquals("keyDispatchingTimedOut", anr.getReason());
        assertEquals(0.71, anr.getLoad(AnrItem.LoadCategory.LOAD_1));
        assertEquals(0.83, anr.getLoad(AnrItem.LoadCategory.LOAD_5));
        assertEquals(0.51, anr.getLoad(AnrItem.LoadCategory.LOAD_15));
        assertEquals(33.0, anr.getCpuUsage(AnrItem.CpuUsageCategory.TOTAL));
        assertEquals(21.0, anr.getCpuUsage(AnrItem.CpuUsageCategory.USER));
        assertEquals(11.0, anr.getCpuUsage(AnrItem.CpuUsageCategory.KERNEL));
        assertEquals(0.3, anr.getCpuUsage(AnrItem.CpuUsageCategory.IOWAIT));
        assertEquals(ArrayUtil.join("\n", lines), anr.getStack());
    }

    /**
     * Test that ANRs are parsed for the header "ANR in app"
     */
    public void testParse_anr_in_app() {
        List<String> lines = Arrays.asList(
                "ANR in com.android.package",
                "Reason: keyDispatchingTimedOut",
                "Load: 0.71 / 0.83 / 0.51",
                "CPU usage from 4357ms to -1434ms ago:",
                "  22% 3378/com.android.package: 19% user + 3.6% kernel / faults: 73 minor 1 major",
                "  16% 312/system_server: 12% user + 4.1% kernel / faults: 1082 minor 6 major",
                "33% TOTAL: 21% user + 11% kernel + 0.3% iowait",
                "CPU usage from 907ms to 1431ms later:",
                "  14% 121/mediaserver: 11% user + 3.7% kernel / faults: 17 minor",
                "    3.7% 183/AudioOut_2: 3.7% user + 0% kernel",
                "  12% 312/system_server: 5.5% user + 7.4% kernel / faults: 6 minor",
                "    5.5% 366/InputDispatcher: 0% user + 5.5% kernel",
                "18% TOTAL: 11% user + 7.5% kernel");

        AnrItem anr = new AnrParser().parse(lines);
        assertNotNull(anr);
        assertEquals("com.android.package", anr.getApp());
        assertEquals("keyDispatchingTimedOut", anr.getReason());
        assertEquals(0.71, anr.getLoad(AnrItem.LoadCategory.LOAD_1));
        assertEquals(0.83, anr.getLoad(AnrItem.LoadCategory.LOAD_5));
        assertEquals(0.51, anr.getLoad(AnrItem.LoadCategory.LOAD_15));
        assertEquals(33.0, anr.getCpuUsage(AnrItem.CpuUsageCategory.TOTAL));
        assertEquals(21.0, anr.getCpuUsage(AnrItem.CpuUsageCategory.USER));
        assertEquals(11.0, anr.getCpuUsage(AnrItem.CpuUsageCategory.KERNEL));
        assertEquals(0.3, anr.getCpuUsage(AnrItem.CpuUsageCategory.IOWAIT));
        assertEquals(ArrayUtil.join("\n", lines), anr.getStack());
    }

    /**
     * Test that ANRs are parsed for the header "ANR in app (class/package)"
     */
    public void testParse_anr_in_app_class_package() {
        List<String> lines = Arrays.asList(
                "ANR in com.android.package (com.android.package/.Activity)",
                "Reason: keyDispatchingTimedOut",
                "Load: 0.71 / 0.83 / 0.51",
                "CPU usage from 4357ms to -1434ms ago:",
                "  22% 3378/com.android.package: 19% user + 3.6% kernel / faults: 73 minor 1 major",
                "  16% 312/system_server: 12% user + 4.1% kernel / faults: 1082 minor 6 major",
                "33% TOTAL: 21% user + 11% kernel + 0.3% iowait",
                "CPU usage from 907ms to 1431ms later:",
                "  14% 121/mediaserver: 11% user + 3.7% kernel / faults: 17 minor",
                "    3.7% 183/AudioOut_2: 3.7% user + 0% kernel",
                "  12% 312/system_server: 5.5% user + 7.4% kernel / faults: 6 minor",
                "    5.5% 366/InputDispatcher: 0% user + 5.5% kernel",
                "18% TOTAL: 11% user + 7.5% kernel");

        AnrItem anr = new AnrParser().parse(lines);
        assertNotNull(anr);
        assertEquals("com.android.package", anr.getApp());
        assertEquals("keyDispatchingTimedOut", anr.getReason());
        assertEquals(0.71, anr.getLoad(AnrItem.LoadCategory.LOAD_1));
        assertEquals(0.83, anr.getLoad(AnrItem.LoadCategory.LOAD_5));
        assertEquals(0.51, anr.getLoad(AnrItem.LoadCategory.LOAD_15));
        assertEquals(33.0, anr.getCpuUsage(AnrItem.CpuUsageCategory.TOTAL));
        assertEquals(21.0, anr.getCpuUsage(AnrItem.CpuUsageCategory.USER));
        assertEquals(11.0, anr.getCpuUsage(AnrItem.CpuUsageCategory.KERNEL));
        assertEquals(0.3, anr.getCpuUsage(AnrItem.CpuUsageCategory.IOWAIT));
        assertEquals(ArrayUtil.join("\n", lines), anr.getStack());
    }

    /**
     * Test that ANRs with PID are parsed.
     */
    public void testParse_anr_in_app_class_package_pid() {
        List<String> lines = Arrays.asList(
                "ANR in com.android.package (com.android.package/.Activity)",
                "PID: 1234",
                "Reason: keyDispatchingTimedOut",
                "Load: 0.71 / 0.83 / 0.51",
                "CPU usage from 4357ms to -1434ms ago:",
                "  22% 3378/com.android.package: 19% user + 3.6% kernel / faults: 73 minor 1 major",
                "  16% 312/system_server: 12% user + 4.1% kernel / faults: 1082 minor 6 major",
                "33% TOTAL: 21% user + 11% kernel + 0.3% iowait",
                "CPU usage from 907ms to 1431ms later:",
                "  14% 121/mediaserver: 11% user + 3.7% kernel / faults: 17 minor",
                "    3.7% 183/AudioOut_2: 3.7% user + 0% kernel",
                "  12% 312/system_server: 5.5% user + 7.4% kernel / faults: 6 minor",
                "    5.5% 366/InputDispatcher: 0% user + 5.5% kernel",
                "18% TOTAL: 11% user + 7.5% kernel");

        AnrItem anr = new AnrParser().parse(lines);
        assertNotNull(anr);
        assertEquals("com.android.package", anr.getApp());
        assertEquals("keyDispatchingTimedOut", anr.getReason());
        assertEquals(1234, anr.getPid().intValue());
        assertEquals(0.71, anr.getLoad(AnrItem.LoadCategory.LOAD_1));
        assertEquals(0.83, anr.getLoad(AnrItem.LoadCategory.LOAD_5));
        assertEquals(0.51, anr.getLoad(AnrItem.LoadCategory.LOAD_15));
        assertEquals(33.0, anr.getCpuUsage(AnrItem.CpuUsageCategory.TOTAL));
        assertEquals(21.0, anr.getCpuUsage(AnrItem.CpuUsageCategory.USER));
        assertEquals(11.0, anr.getCpuUsage(AnrItem.CpuUsageCategory.KERNEL));
        assertEquals(0.3, anr.getCpuUsage(AnrItem.CpuUsageCategory.IOWAIT));
        assertEquals(ArrayUtil.join("\n", lines), anr.getStack());
    }
}
