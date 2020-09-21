/*
 * Copyright (C) 2011 The Android Open Source Project
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
import com.android.loganalysis.item.JavaCrashItem;
import com.android.loganalysis.item.MonkeyLogItem;
import com.android.loganalysis.item.MonkeyLogItem.DroppedCategory;

import junit.framework.TestCase;

import java.io.BufferedReader;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;

/**
 * Functional tests for {@link MonkeyLogParser}
 */
public class MonkeyLogParserFuncTest extends TestCase {
    // FIXME: Make monkey log file configurable.
    private static final String MONKEY_LOG_PATH = "/tmp/monkey_log.txt";

    /**
     * A test that is intended to force Brillopad to parse a monkey log. The purpose of this is to
     * assist a developer in checking why a given monkey log file might not be parsed correctly by
     * Brillopad.
     */
    public void testParse() {
        BufferedReader monkeyLogReader = null;
        try {
            monkeyLogReader = new BufferedReader(new FileReader(MONKEY_LOG_PATH));
        } catch (FileNotFoundException e) {
            fail(String.format("File not found at %s", MONKEY_LOG_PATH));
        }
        MonkeyLogItem monkeyLog = null;
        try {
            long start = System.currentTimeMillis();
            monkeyLog = new MonkeyLogParser().parse(monkeyLogReader);
            long stop = System.currentTimeMillis();
            System.out.println(String.format("Monkey log took %d ms to parse.", stop - start));
        } catch (IOException e) {
            fail(String.format("IOException: %s", e.toString()));
        } finally {
            if (monkeyLogReader != null) {
                try {
                    monkeyLogReader.close();
                } catch (IOException e) {
                    // Ignore
                }
            }        }

        assertNotNull(monkeyLog);
        assertNotNull(monkeyLog.getStartTime());
        assertNotNull(monkeyLog.getStopTime());
        assertNotNull(monkeyLog.getTargetCount());
        assertNotNull(monkeyLog.getThrottle());
        assertNotNull(monkeyLog.getSeed());
        assertNotNull(monkeyLog.getIgnoreSecurityExceptions());
        assertTrue(monkeyLog.getPackages().size() > 0);
        assertTrue(monkeyLog.getCategories().size() > 0);
        assertNotNull(monkeyLog.getIsFinished());
        assertNotNull(monkeyLog.getIntermediateCount());
        assertNotNull(monkeyLog.getTotalDuration());
        assertNotNull(monkeyLog.getStartUptimeDuration());
        assertNotNull(monkeyLog.getStopUptimeDuration());


        StringBuffer sb = new StringBuffer();
        sb.append("Stats for monkey log:\n");
        sb.append(String.format("  Start time: %s\n", monkeyLog.getStartTime()));
        sb.append(String.format("  Stop time: %s\n", monkeyLog.getStopTime()));
        sb.append(String.format("  Parameters: target-count=%d, throttle=%d, seed=%d, " +
                "ignore-security-exceptions=%b\n",
                monkeyLog.getTargetCount(), monkeyLog.getThrottle(), monkeyLog.getSeed(),
                monkeyLog.getIgnoreSecurityExceptions()));
        sb.append(String.format("  Packages: %s\n", monkeyLog.getPackages()));
        sb.append(String.format("  Categories: %s\n", monkeyLog.getCategories()));
        if (monkeyLog.getNoActivities()) {
            sb.append("  Status: no-activities=true\n");
        } else {
            sb.append(String.format("  Status: finished=%b, final-count=%d, " +
                    "intermediate-count=%d\n", monkeyLog.getIsFinished(), monkeyLog.getFinalCount(),
                    monkeyLog.getIntermediateCount()));

            sb.append("  Dropped events:");
            for (DroppedCategory drop : DroppedCategory.values()) {
                sb.append(String.format(" %s=%d,", drop.toString(),
                        monkeyLog.getDroppedCount(drop)));
            }
            sb.deleteCharAt(sb.length()-1);
            sb.append("\n");
        }
        sb.append(String.format("  Run time: duration=%d ms, delta-uptime=%d (%d - %d) ms\n",
                monkeyLog.getTotalDuration(),
                monkeyLog.getStopUptimeDuration() - monkeyLog.getStartUptimeDuration(),
                monkeyLog.getStopUptimeDuration(), monkeyLog.getStartUptimeDuration()));

        if (monkeyLog.getCrash() != null && monkeyLog.getCrash() instanceof AnrItem) {
            sb.append(String.format("  Stopped due to ANR\n"));
        }
        if (monkeyLog.getCrash() != null && monkeyLog.getCrash() instanceof JavaCrashItem) {
            sb.append(String.format("  Stopped due to Java crash\n"));
        }
        System.out.println(sb.toString());
    }
}

