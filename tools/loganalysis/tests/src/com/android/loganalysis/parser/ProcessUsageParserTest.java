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

import com.android.loganalysis.item.ProcessUsageItem;
import com.android.loganalysis.item.ProcessUsageItem.ProcessUsageInfoItem;
import com.android.loganalysis.item.ProcessUsageItem.SensorInfoItem;

import junit.framework.TestCase;

import java.util.Arrays;
import java.util.LinkedList;
import java.util.List;

/**
 * Unit tests for {@link ProcessUsageParser}
 */
public class ProcessUsageParserTest extends TestCase {

    /**
     * Test that normal input is parsed.
     */
    public void testProcessUsageParser() {
        List<String> inputBlock = Arrays.asList(
               " 0:",
               "    Mobile network: 173.70KB received, 102.55KB sent (packets 129)",
               "    Mobile radio active: 6m 5s 80ms (14.9%) 80x @ 139 mspp",
               " 1000:",
               "  Mobile network: 16.43KB received, 26.26KB sent",
               "  Mobile radio active: 1m 17s 489ms (3.2%) 61x @ 179 mspp",
               "  Sensor 44: 27m 18s 207ms realtime (22 times)",
               "  Sensor 36: 6s 483ms realtime (3 times)",
               "  Proc servicemanager:",
               "      CPU: 2s 20ms usr + 4s 60ms krn ; 0ms fg",
               "    Apk android:",
               "      266 wakeup alarms",
               " u0a2:",
               "  Mobile network: 16.43KB received, 26.26KB sent",
               "  Mobile radio active: 1m 17s 489ms (3.2%) 61x @ 179 mspp",
               "  Sensor 0: 5s 207ms realtime (2 times)",
               "  Proc servicemanager:",
               "      CPU: 2s 20ms usr + 4s 60ms krn ; 0ms fg",
               "    Apk android:",
               "      2 wakeup alarms",
               "  ");

        ProcessUsageItem processUsage = new ProcessUsageParser().parse(inputBlock);

        assertEquals(3, processUsage.getProcessUsage().size());

        LinkedList<ProcessUsageInfoItem> processUsageInfo =
                (LinkedList<ProcessUsageInfoItem>)processUsage.getProcessUsage();

        assertEquals("1000", processUsageInfo.get(1).getProcessUID());
        assertEquals(266, processUsageInfo.get(1).getAlarmWakeups());

        LinkedList<SensorInfoItem> sensor = processUsageInfo.get(1).getSensorUsage();
        assertEquals("44", sensor.get(0).getSensorName());
        assertEquals("36", sensor.get(1).getSensorName());

        sensor = processUsageInfo.get(2).getSensorUsage();
        assertEquals("0", sensor.get(0).getSensorName());
    }
}

