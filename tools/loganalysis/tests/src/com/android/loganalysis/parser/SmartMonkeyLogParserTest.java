/*
 * Copyright (C) 2013 The Android Open Source Project
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

import com.android.loganalysis.item.SmartMonkeyLogItem;
import com.android.loganalysis.parser.SmartMonkeyLogParser;

import java.text.ParseException;
import java.util.Arrays;
import java.util.List;
import junit.framework.TestCase;

/**
 * Unit tests for {@link SmartMonkeyLogParser} and {@link SmartMonkeyLogItem}
 */
public class SmartMonkeyLogParserTest extends TestCase {

    /**
     * Test for detecting UI exceptions
     * @throws ParseException
     */
    public void testExceptions() throws ParseException {
        List<String> lines = Arrays.asList(
            "2013-03-04 12:33:18.789: Starting [UiAutomator Tests][com.android.cts.uiautomator]",
            "2013-03-04 12:33:18.792: Target invocation count: 1000",
            "2013-03-04 12:33:18.793: Throttle: 0 ms",
            "2013-03-04 12:33:19.795: [  0](Seq: -1)-Launching UiAutomator Tests",
            "2013-03-04 12:33:37.211: [  0](Seq:  0)-Found 6 candidates. Using index: 1",
            "2013-03-04 12:33:37.336: [  0](Seq:  0)-Clicking: CheckBox (760,194)",
            "2013-03-04 12:33:38.443: [  1](Seq:  0)-Found 6 candidates. Using index: 5",
            "2013-03-04 12:33:38.533: [  1](Seq:  0)-Clicking: Button~Description for Button (723,874)",
            "2013-03-04 12:33:39.510: [  2](Seq:  0)-UI Exception: CRASH: Unfortunately, UiAutomator Test App has stopped.",
            "2013-03-04 12:43:39.510: [  2](Seq:  0)-UI Exception: ANR: UiAutomator is not responding.",
            "2013-03-04 12:53:39.513: Invocations requested: 1000",
            "2013-03-04 12:53:39.518: Invocations completed: 2",
            "2013-03-04 12:53:39.520: Device uptime: 608193 sec, Monkey run duration: 20 sec");

        SmartMonkeyLogItem monkeyLog = new SmartMonkeyLogParser().parse(lines);
        assertNotNull(monkeyLog);
        assertEquals(1, monkeyLog.getCrashTimes().size());
        assertEquals(1, monkeyLog.getAnrTimes().size());
        assertEquals(SmartMonkeyLogParser.parseTime("2013-03-04 12:33:39.510"),
                monkeyLog.getCrashTimes().toArray()[0]);
        assertEquals(SmartMonkeyLogParser.parseTime("2013-03-04 12:43:39.510"),
                monkeyLog.getAnrTimes().toArray()[0]);
    }

    /**
     * Tests for parsing smart monkey log header
     * @throws ParseException
     */
    public void testHeader() throws ParseException {
        List<String> lines = Arrays.asList(
            "2013-03-04 12:33:18.789: Starting [UiAutomator Tests|YouTube][com.android.cts.uiautomator|com.google.android.youtube]",
            "2013-03-04 12:33:18.792: Target invocation count: 1000",
            "2013-03-04 12:33:18.793: Throttle: 1500 ms",
            "2013-03-04 12:33:18.793: Device uptime: 608173 sec",
            "2013-03-04 12:33:19.795: [  0](Seq: -1)-Launching UiAutomator Tests",
            "2013-03-04 12:33:37.211: [  0](Seq:  0)-Found 6 candidates. Using index: 1",
            "2013-03-04 12:33:37.336: [  0](Seq:  0)-Clicking: CheckBox (760,194)",
            "2013-03-04 12:33:38.443: [  1](Seq:  0)-Found 6 candidates. Using index: 5",
            "2013-03-04 12:33:38.533: [  1](Seq:  0)-Clicking: Button~Description for Button (723,874)",
            "2013-03-04 12:33:39.510: [  2](Seq:  0)-UI Exception: CRASH: Unfortunately, UiAutomator Test App has stopped.",
            "2013-03-04 12:43:39.510: [  2](Seq:  0)-UI Exception: ANR: UiAutomator is not responding.",
            "2013-03-04 12:53:39.513: Invocations requested: 1000",
            "2013-03-04 12:53:39.518: Invocations completed: 2",
            "2013-03-04 12:53:39.520: Device uptime: 608193 sec, Monkey run duration: 20 sec");
        SmartMonkeyLogItem monkeyLog = new SmartMonkeyLogParser().parse(lines);
        assertNotNull(monkeyLog);
        assertEquals(2, monkeyLog.getApplications().size());
        assertEquals("UiAutomator Tests", monkeyLog.getApplications().get(0));
        assertEquals("YouTube", monkeyLog.getApplications().get(1));
        assertEquals(2, monkeyLog.getPackages().size());
        assertEquals("com.android.cts.uiautomator", monkeyLog.getPackages().get(0));
        assertEquals("com.google.android.youtube", monkeyLog.getPackages().get(1));
        assertEquals(SmartMonkeyLogParser.parseTime("2013-03-04 12:33:18.789"),
                monkeyLog.getStartTime());
        assertEquals(608173, monkeyLog.getStartUptimeDuration());
        assertEquals(1000, monkeyLog.getTargetInvocations());
        assertEquals(1500, monkeyLog.getThrottle());
    }

    /**
     * Test for parsing log in flight
     * @throws ParseException
     */
    public void testIntermidiateStop() throws ParseException {
        List<String> lines = Arrays.asList(
                "2013-03-04 12:33:18.789: Starting [UiAutomator Tests|YouTube][com.android.cts.uiautomator|com.google.android.youtube]",
                "2013-03-04 12:33:18.792: Target invocation count: 1000",
                "2013-03-04 12:33:18.793: Throttle: 1500 ms",
                "2013-03-04 12:33:19.795: [  0](Seq: -1)-Launching UiAutomator Tests",
                "2013-03-04 12:33:37.211: [  0](Seq:  0)-Found 6 candidates. Using index: 1",
                "2013-03-04 12:33:37.336: [  0](Seq:  0)-Clicking: CheckBox (760,194)",
                "2013-03-04 12:33:38.443: [  1](Seq:  0)-Found 6 candidates. Using index: 5",
                "2013-03-04 12:33:38.533: [ 12](Seq:  3)-Clicking: Button~Description for Button (723,874)",
                "2013-03-04 12:33:39.510: [ 12](Seq:  3)-UI Exception: CRASH: Unfortunately, UiAutomator Test App has stopped.");

        SmartMonkeyLogItem monkeyLog = new SmartMonkeyLogParser().parse(lines);
        assertNotNull(monkeyLog);
        assertEquals(12, monkeyLog.getIntermediateCount());
        assertEquals(SmartMonkeyLogParser.parseTime("2013-03-04 12:33:39.510"),
                monkeyLog.getIntermediateTime());
    }

    /**
     * Tests for parsing smart monkey log footer
     * @throws ParseException
     */
    public void testFooter() throws ParseException {
        List<String> lines = Arrays.asList(
                "2013-03-04 12:33:18.789: Starting [UiAutomator Tests|YouTube][com.android.cts.uiautomator|com.google.android.youtube]",
                "2013-03-04 12:33:18.792: Target invocation count: 1000",
                "2013-03-04 12:33:18.793: Throttle: 1500 ms",
                "2013-03-04 12:33:19.795: [  0](Seq: -1)-Launching UiAutomator Tests",
                "2013-03-04 12:33:37.211: [  0](Seq:  0)-Found 6 candidates. Using index: 1",
                "2013-03-04 12:33:37.336: [  0](Seq:  0)-Clicking: CheckBox (760,194)",
                "2013-03-04 12:33:38.443: [  1](Seq:  0)-Found 6 candidates. Using index: 5",
                "2013-03-04 12:33:38.533: [  1](Seq:  0)-Clicking: Button~Description for Button (723,874)",
                "2013-03-04 12:33:38.443: [  2](Seq:  0)-Found 6 candidates. Using index: 5",
                "2013-03-04 12:33:38.533: [  2](Seq:  0)-Clicking: Button~Description for Button (723,874)",
                "2013-03-04 12:53:39.513: Monkey aborted.",
                "2013-03-04 12:53:39.513: Invocations requested: 1000",
                "2013-03-04 12:53:39.518: Invocations completed: 999",
                "2013-03-04 12:53:39.520: Device uptime: 608193 sec, Monkey run duration: 20 sec");

        SmartMonkeyLogItem monkeyLog = new SmartMonkeyLogParser().parse(lines);
        assertNotNull(monkeyLog);
        assertEquals(999, monkeyLog.getFinalCount());
        assertEquals(20, monkeyLog.getTotalDuration());
        assertEquals(608193, monkeyLog.getStopUptimeDuration());
        assertEquals(true, monkeyLog.getIsAborted());
    }
}
