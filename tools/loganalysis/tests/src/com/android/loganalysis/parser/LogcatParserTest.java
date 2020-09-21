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

import com.android.loganalysis.item.JavaCrashItem;
import com.android.loganalysis.item.LogcatItem;
import com.android.loganalysis.item.MiscLogcatItem;
import com.android.loganalysis.util.ArrayUtil;

import junit.framework.TestCase;

import java.text.DateFormat;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Arrays;
import java.util.Date;
import java.util.List;
import java.util.regex.Pattern;

/**
 * Unit tests for {@link LogcatParserTest}.
 */
public class LogcatParserTest extends TestCase {

    /**
     * Test that an ANR is parsed in the log.
     */
    public void testParse_anr() throws ParseException {
        List<String> lines = Arrays.asList(
                "04-25 17:17:08.445   312   366 E ActivityManager: ANR (application not responding) in process: com.android.package",
                "04-25 17:17:08.445   312   366 E ActivityManager: Reason: keyDispatchingTimedOut",
                "04-25 17:17:08.445   312   366 E ActivityManager: Load: 0.71 / 0.83 / 0.51",
                "04-25 17:17:08.445   312   366 E ActivityManager: 33% TOTAL: 21% user + 11% kernel + 0.3% iowait");

        LogcatItem logcat = new LogcatParser("2012").parse(lines);
        assertNotNull(logcat);
        assertEquals(parseTime("2012-04-25 17:17:08.445"), logcat.getStartTime());
        assertEquals(parseTime("2012-04-25 17:17:08.445"), logcat.getStopTime());
        assertEquals(1, logcat.getEvents().size());
        assertEquals(1, logcat.getAnrs().size());
        assertEquals(312, logcat.getAnrs().get(0).getPid().intValue());
        assertEquals(366, logcat.getAnrs().get(0).getTid().intValue());
        assertEquals("", logcat.getAnrs().get(0).getLastPreamble());
        assertEquals("", logcat.getAnrs().get(0).getProcessPreamble());
        assertEquals(parseTime("2012-04-25 17:17:08.445"), logcat.getAnrs().get(0).getEventTime());
    }

    /**
     * Test that an ANR is parsed in the log.
     */
    public void testParse_anr_pid() throws ParseException {
        List<String> lines = Arrays.asList(
                "04-25 17:17:08.445   312   366 E ActivityManager: ANR (application not responding) in process: com.android.package",
                "04-25 17:17:08.445   312   366 E ActivityManager: PID: 1234",
                "04-25 17:17:08.445   312   366 E ActivityManager: Reason: keyDispatchingTimedOut",
                "04-25 17:17:08.445   312   366 E ActivityManager: Load: 0.71 / 0.83 / 0.51",
                "04-25 17:17:08.445   312   366 E ActivityManager: 33% TOTAL: 21% user + 11% kernel + 0.3% iowait");

        LogcatItem logcat = new LogcatParser("2012").parse(lines);
        assertNotNull(logcat);
        assertEquals(parseTime("2012-04-25 17:17:08.445"), logcat.getStartTime());
        assertEquals(parseTime("2012-04-25 17:17:08.445"), logcat.getStopTime());
        assertEquals(1, logcat.getEvents().size());
        assertEquals(1, logcat.getAnrs().size());
        assertEquals(1234, logcat.getAnrs().get(0).getPid().intValue());
        assertNull(logcat.getAnrs().get(0).getTid());
        assertEquals("", logcat.getAnrs().get(0).getLastPreamble());
        assertEquals("", logcat.getAnrs().get(0).getProcessPreamble());
        assertEquals(parseTime("2012-04-25 17:17:08.445"), logcat.getAnrs().get(0).getEventTime());
    }

    /**
     * Test that Java crashes can be parsed.
     */
    public void testParse_java_crash() throws ParseException {
        List<String> lines = Arrays.asList(
                "04-25 09:55:47.799  3064  3082 E AndroidRuntime: java.lang.Exception",
                "04-25 09:55:47.799  3064  3082 E AndroidRuntime: \tat class.method1(Class.java:1)",
                "04-25 09:55:47.799  3064  3082 E AndroidRuntime: \tat class.method2(Class.java:2)",
                "04-25 09:55:47.799  3064  3082 E AndroidRuntime: \tat class.method3(Class.java:3)");

        LogcatItem logcat = new LogcatParser("2012").parse(lines);
        assertNotNull(logcat);
        assertEquals(parseTime("2012-04-25 09:55:47.799"), logcat.getStartTime());
        assertEquals(parseTime("2012-04-25 09:55:47.799"), logcat.getStopTime());
        assertEquals(1, logcat.getEvents().size());
        assertEquals(1, logcat.getJavaCrashes().size());
        assertEquals(3064, logcat.getJavaCrashes().get(0).getPid().intValue());
        assertEquals(3082, logcat.getJavaCrashes().get(0).getTid().intValue());
        assertEquals("", logcat.getJavaCrashes().get(0).getLastPreamble());
        assertEquals("", logcat.getJavaCrashes().get(0).getProcessPreamble());
        assertEquals(parseTime("2012-04-25 09:55:47.799"),
                logcat.getJavaCrashes().get(0).getEventTime());
    }

    public void testParse_test_exception() {
        List<String> lines = Arrays.asList(
                "11-25 19:26:53.581  5832  7008 I TestRunner: ----- begin exception -----",
                "11-25 19:26:53.589  5832  7008 I TestRunner: ",
                "11-25 19:26:53.589  5832  7008 I TestRunner: java.util.concurrent.TimeoutException",
                "11-25 19:26:53.589  5832  7008 I TestRunner:    at android.support.test.uiautomator.WaitMixin.wait(WaitMixin.java:49)",
                "11-25 19:26:53.589  5832  7008 I TestRunner:    at android.support.test.uiautomator.WaitMixin.wait(WaitMixin.java:36)",
                "11-25 19:26:53.589  5832  7008 I TestRunner:    at android.support.test.uiautomator.UiDevice.wait(UiDevice.java:169)",
                "11-25 19:26:53.589  5832  7008 I TestRunner:    at com.android.test.uiautomator.common.helpers.MapsHelper.doSearch(MapsHelper.java:87)",
                "11-25 19:26:53.589  5832  7008 I TestRunner:    at com.android.test.uiautomator.aupt.MapsTest.testMaps(MapsTest.java:58)",
                "11-25 19:26:53.589  5832  7008 I TestRunner:    at java.lang.reflect.Method.invoke(Native Method)",
                "11-25 19:26:53.589  5832  7008 I TestRunner:    at java.lang.reflect.Method.invoke(Method.java:372)",
                "11-25 19:26:53.589  5832  7008 I TestRunner:    at android.test.InstrumentationTestCase.runMethod(InstrumentationTestCase.java:214)",
                "11-25 19:26:53.589  5832  7008 I TestRunner:    at android.test.InstrumentationTestCase.runTest(InstrumentationTestCase.java:199)",
                "11-25 19:26:53.589  5832  7008 I TestRunner:    at junit.framework.TestCase.runBare(TestCase.java:134)",
                "11-25 19:26:53.589  5832  7008 I TestRunner:    at junit.framework.TestResult$1.protect(TestResult.java:115)",
                "11-25 19:26:53.589  5832  7008 I TestRunner:    at junit.framework.TestResult.runProtected(TestResult.java:133)",
                "11-25 19:26:53.589  5832  7008 I TestRunner:    at junit.framework.TestResult.run(TestResult.java:118)",
                "11-25 19:26:53.589  5832  7008 I TestRunner:    at junit.framework.TestCase.run(TestCase.java:124)",
                "11-25 19:26:53.589  5832  7008 I TestRunner:    at android.support.test.aupt.AuptTestRunner$AuptPrivateTestRunner.runTest(AuptTestRunner.java:182)",
                "11-25 19:26:53.589  5832  7008 I TestRunner:    at android.test.AndroidTestRunner.runTest(AndroidTestRunner.java:176)",
                "11-25 19:26:53.589  5832  7008 I TestRunner:    at android.test.InstrumentationTestRunner.onStart(InstrumentationTestRunner.java:555)",
                "11-25 19:26:53.589  5832  7008 I TestRunner:    at android.app.Instrumentation$InstrumentationThread.run(Instrumentation.java:1848)",
                "11-25 19:26:53.589  5832  7008 I TestRunner: ----- end exception -----"
                );

        LogcatParser logcatParser = new LogcatParser("2012");
        logcatParser.addJavaCrashTag("I", "TestRunner", LogcatParser.JAVA_CRASH);
        LogcatItem logcat = logcatParser.parse(lines);
        assertNotNull(logcat);
        assertEquals(1, logcat.getEvents().size());
        assertEquals(1, logcat.getJavaCrashes().size());
        assertEquals(5832, logcat.getJavaCrashes().get(0).getPid().intValue());
        assertEquals(7008, logcat.getJavaCrashes().get(0).getTid().intValue());
        assertEquals("", logcat.getJavaCrashes().get(0).getLastPreamble());
        assertEquals("", logcat.getJavaCrashes().get(0).getProcessPreamble());
        assertEquals(LogcatParser.JAVA_CRASH, logcat.getJavaCrashes().get(0).getCategory());
    }

    public void testParse_test_exception_with_exras() {
        List<String> lines = Arrays.asList(
                "12-06 17:19:18.746  6598  7960 I TestRunner: failed: testYouTube(com.android.test.uiautomator.aupt.YouTubeTest)",
                "12-06 17:19:18.746  6598  7960 I TestRunner: ----- begin exception -----",
                "12-06 17:19:18.747  6598  7960 I TestRunner: ",
                "12-06 17:19:18.747  6598  7960 I TestRunner: java.util.concurrent.TimeoutException",
                "12-06 17:19:18.747  6598  7960 I TestRunner:    at android.support.test.uiautomator.WaitMixin.wait(WaitMixin.java:49)",
                "12-06 17:19:18.747  6598  7960 I TestRunner:    at android.support.test.uiautomator.WaitMixin.wait(WaitMixin.java:36)",
                "12-06 17:19:18.747  6598  7960 I TestRunner:    at android.support.test.uiautomator.UiDevice.wait(UiDevice.java:169)",
                "12-06 17:19:18.747  6598  7960 I TestRunner:    at android.support.test.aupt.AppLauncher.launchApp(AppLauncher.java:127)",
                "12-06 17:19:18.747  6598  7960 I TestRunner:    at com.android.test.uiautomator.common.helpers.YouTubeHelper.open(YouTubeHelper.java:49)",
                "12-06 17:19:18.747  6598  7960 I TestRunner:    at com.android.test.uiautomator.aupt.YouTubeTest.setUp(YouTubeTest.java:44)",
                "12-06 17:19:18.747  6598  7960 I TestRunner:    at junit.framework.TestCase.runBare(TestCase.java:132)",
                "12-06 17:19:18.747  6598  7960 I TestRunner:    at junit.framework.TestResult$1.protect(TestResult.java:115)",
                "12-06 17:19:18.747  6598  7960 I TestRunner:    at junit.framework.TestResult.runProtected(TestResult.java:133)",
                "12-06 17:19:18.747  6598  7960 I TestRunner:    at junit.framework.TestResult.run(TestResult.java:118)",
                "12-06 17:19:18.747  6598  7960 I TestRunner:    at junit.framework.TestCase.run(TestCase.java:124)",
                "12-06 17:19:18.747  6598  7960 I TestRunner:    at android.support.test.aupt.AuptTestRunner$AuptPrivateTestRunner.runTest(AuptTestRunner.java:182)",
                "12-06 17:19:18.747  6598  7960 I TestRunner:    at android.test.AndroidTestRunner.runTest(AndroidTestRunner.java:176)",
                "12-06 17:19:18.747  6598  7960 I TestRunner:    at android.test.InstrumentationTestRunner.onStart(InstrumentationTestRunner.java:555)",
                "12-06 17:19:18.747  6598  7960 I TestRunner:    at android.app.Instrumentation$InstrumentationThread.run(Instrumentation.java:1851)",
                "12-06 17:19:18.747  6598  7960 I TestRunner: ----- end exception -----"
                );

        LogcatParser logcatParser = new LogcatParser("2012");
        logcatParser.addJavaCrashTag("I", "TestRunner", LogcatParser.JAVA_CRASH);
        LogcatItem logcat = logcatParser.parse(lines);
        assertNotNull(logcat);
        assertEquals(1, logcat.getEvents().size());
        assertEquals(1, logcat.getJavaCrashes().size());
        assertEquals(6598, logcat.getJavaCrashes().get(0).getPid().intValue());
        assertEquals(7960, logcat.getJavaCrashes().get(0).getTid().intValue());
        assertEquals("", logcat.getJavaCrashes().get(0).getLastPreamble());
        assertEquals("", logcat.getJavaCrashes().get(0).getProcessPreamble());
        // Check that lines not related to java crash are absent
        assertFalse(logcat.getJavaCrashes().get(0).getStack().contains("begin exception"));
        assertFalse(logcat.getJavaCrashes().get(0).getStack().contains("end exception"));
        assertFalse(logcat.getJavaCrashes().get(0).getStack().contains("failed: testYouTube"));
        //System.out.println(logcat.getJavaCrashes().get(0).getStack());
    }

    /**
     * Test that Java crashes from system server can be parsed.
     */
    public void testParse_java_crash_system_server() throws ParseException {
        List<String> lines = Arrays.asList(
                "04-25 09:55:47.799  3064  3082 E AndroidRuntime: *** FATAL EXCEPTION IN SYSTEM PROCESS: message",
                "04-25 09:55:47.799  3064  3082 E AndroidRuntime: java.lang.Exception",
                "04-25 09:55:47.799  3064  3082 E AndroidRuntime: \tat class.method1(Class.java:1)",
                "04-25 09:55:47.799  3064  3082 E AndroidRuntime: \tat class.method2(Class.java:2)",
                "04-25 09:55:47.799  3064  3082 E AndroidRuntime: \tat class.method3(Class.java:3)");

        LogcatItem logcat = new LogcatParser("2012").parse(lines);
        assertNotNull(logcat);
        assertEquals(parseTime("2012-04-25 09:55:47.799"), logcat.getStartTime());
        assertEquals(parseTime("2012-04-25 09:55:47.799"), logcat.getStopTime());
        assertEquals(1, logcat.getEvents().size());
        assertEquals(1, logcat.getJavaCrashes().size());
        assertEquals("system_server", logcat.getJavaCrashes().get(0).getApp());
        assertEquals(3064, logcat.getJavaCrashes().get(0).getPid().intValue());
        assertEquals(3082, logcat.getJavaCrashes().get(0).getTid().intValue());
        assertEquals("", logcat.getJavaCrashes().get(0).getLastPreamble());
        assertEquals("", logcat.getJavaCrashes().get(0).getProcessPreamble());
        assertEquals(parseTime("2012-04-25 09:55:47.799"),
                logcat.getJavaCrashes().get(0).getEventTime());
    }

    /**
     * Test that Java crashes with process and pid can be parsed.
     */
    public void testParse_java_crash_process_pid() throws ParseException {
        List<String> lines = Arrays.asList(
                "04-25 09:55:47.799  3064  3082 E AndroidRuntime: FATAL EXCEPTION: main",
                "04-25 09:55:47.799  3064  3082 E AndroidRuntime: Process: com.android.package, PID: 1234",
                "04-25 09:55:47.799  3064  3082 E AndroidRuntime: java.lang.Exception",
                "04-25 09:55:47.799  3064  3082 E AndroidRuntime: \tat class.method1(Class.java:1)",
                "04-25 09:55:47.799  3064  3082 E AndroidRuntime: \tat class.method2(Class.java:2)",
                "04-25 09:55:47.799  3064  3082 E AndroidRuntime: \tat class.method3(Class.java:3)");

        LogcatItem logcat = new LogcatParser("2012").parse(lines);
        assertNotNull(logcat);
        assertEquals(parseTime("2012-04-25 09:55:47.799"), logcat.getStartTime());
        assertEquals(parseTime("2012-04-25 09:55:47.799"), logcat.getStopTime());
        assertEquals(1, logcat.getEvents().size());
        assertEquals(1, logcat.getJavaCrashes().size());
        assertEquals("com.android.package", logcat.getJavaCrashes().get(0).getApp());
        assertEquals(1234, logcat.getJavaCrashes().get(0).getPid().intValue());
        assertNull(logcat.getJavaCrashes().get(0).getTid());
        assertEquals("", logcat.getJavaCrashes().get(0).getLastPreamble());
        assertEquals("", logcat.getJavaCrashes().get(0).getProcessPreamble());
        assertEquals(parseTime("2012-04-25 09:55:47.799"),
                logcat.getJavaCrashes().get(0).getEventTime());
    }

    /**
     * Test that Java crashes with pid can be parsed.
     */
    public void testParse_java_crash_pid() throws ParseException {
        List<String> lines = Arrays.asList(
                "04-25 09:55:47.799  3064  3082 E AndroidRuntime: FATAL EXCEPTION: main",
                "04-25 09:55:47.799  3064  3082 E AndroidRuntime: PID: 1234",
                "04-25 09:55:47.799  3064  3082 E AndroidRuntime: java.lang.Exception",
                "04-25 09:55:47.799  3064  3082 E AndroidRuntime: \tat class.method1(Class.java:1)",
                "04-25 09:55:47.799  3064  3082 E AndroidRuntime: \tat class.method2(Class.java:2)",
                "04-25 09:55:47.799  3064  3082 E AndroidRuntime: \tat class.method3(Class.java:3)");

        LogcatItem logcat = new LogcatParser("2012").parse(lines);
        assertNotNull(logcat);
        assertEquals(parseTime("2012-04-25 09:55:47.799"), logcat.getStartTime());
        assertEquals(parseTime("2012-04-25 09:55:47.799"), logcat.getStopTime());
        assertEquals(1, logcat.getEvents().size());
        assertEquals(1, logcat.getJavaCrashes().size());
        assertNull(logcat.getJavaCrashes().get(0).getApp());
        assertEquals(1234, logcat.getJavaCrashes().get(0).getPid().intValue());
        assertNull(logcat.getJavaCrashes().get(0).getTid());
        assertEquals("", logcat.getJavaCrashes().get(0).getLastPreamble());
        assertEquals("", logcat.getJavaCrashes().get(0).getProcessPreamble());
        assertEquals(parseTime("2012-04-25 09:55:47.799"),
                logcat.getJavaCrashes().get(0).getEventTime());
    }

    /**
     * Test that Java crashes with process and pid without stack can be parsed.
     */
    public void testParse_java_crash_empty() throws ParseException {
        List<String> lines = Arrays.asList(
                "04-25 09:55:47.799  3064  3082 E AndroidRuntime: FATAL EXCEPTION: main",
                "04-25 09:55:47.799  3064  3082 E AndroidRuntime: PID: 1234");

        LogcatItem logcat = new LogcatParser("2012").parse(lines);
        assertNotNull(logcat);
        assertEquals(parseTime("2012-04-25 09:55:47.799"), logcat.getStartTime());
        assertEquals(parseTime("2012-04-25 09:55:47.799"), logcat.getStopTime());
        assertEquals(0, logcat.getEvents().size());
        assertEquals(0, logcat.getJavaCrashes().size());
    }

    /**
     * Test that native crashes can be parsed from the info log level.
     */
    public void testParse_native_crash_info() throws ParseException {
        List<String> lines = Arrays.asList(
                "04-25 18:33:27.273   115   115 I DEBUG   : Build fingerprint: 'product:build:target'",
                "04-25 18:33:27.273   115   115 I DEBUG   : pid: 3112, tid: 3112  >>> com.google.android.browser <<<",
                "04-25 18:33:27.273   115   115 I DEBUG   : signal 11 (SIGSEGV), code 1 (SEGV_MAPERR), fault addr 00000000");

        LogcatItem logcat = new LogcatParser("2012").parse(lines);
        assertNotNull(logcat);
        assertEquals(parseTime("2012-04-25 18:33:27.273"), logcat.getStartTime());
        assertEquals(parseTime("2012-04-25 18:33:27.273"), logcat.getStopTime());
        assertEquals(1, logcat.getEvents().size());
        assertEquals(1, logcat.getNativeCrashes().size());
        assertEquals(3112, logcat.getNativeCrashes().get(0).getPid().intValue());
        assertEquals(3112, logcat.getNativeCrashes().get(0).getTid().intValue());
        assertEquals("com.google.android.browser", logcat.getNativeCrashes().get(0).getApp());
        assertEquals("", logcat.getNativeCrashes().get(0).getLastPreamble());
        assertEquals("", logcat.getNativeCrashes().get(0).getProcessPreamble());
        assertEquals(parseTime("2012-04-25 18:33:27.273"),
                logcat.getNativeCrashes().get(0).getEventTime());
    }

    /**
     * Test that native crashes can be parsed from the fatal log level.
     */
    public void testParse_native_crash_fatal() throws ParseException {
        List<String> lines = Arrays.asList(
                "04-25 18:33:27.273   115   115 F DEBUG   : Build fingerprint: 'product:build:target'",
                "04-25 18:33:27.273   115   115 F DEBUG   : pid: 3112, tid: 3112, name: Name  >>> com.google.android.browser <<<",
                "04-25 18:33:27.273   115   115 F DEBUG   : signal 11 (SIGSEGV), code 1 (SEGV_MAPERR), fault addr 00000000");

        LogcatItem logcat = new LogcatParser("2012").parse(lines);
        assertNotNull(logcat);
        assertEquals(parseTime("2012-04-25 18:33:27.273"), logcat.getStartTime());
        assertEquals(parseTime("2012-04-25 18:33:27.273"), logcat.getStopTime());
        assertEquals(1, logcat.getEvents().size());
        assertEquals(1, logcat.getNativeCrashes().size());
        assertEquals(3112, logcat.getNativeCrashes().get(0).getPid().intValue());
        assertEquals(3112, logcat.getNativeCrashes().get(0).getTid().intValue());
        assertEquals("com.google.android.browser", logcat.getNativeCrashes().get(0).getApp());
        assertEquals("", logcat.getNativeCrashes().get(0).getLastPreamble());
        assertEquals("", logcat.getNativeCrashes().get(0).getProcessPreamble());
        assertEquals(parseTime("2012-04-25 18:33:27.273"),
                logcat.getNativeCrashes().get(0).getEventTime());
    }

    /**
     * Test that native crashes can be parsed if they have the same pid/tid.
     */
    public void testParse_native_crash_same_pid() throws ParseException {
        List<String> lines = Arrays.asList(
                "04-25 18:33:27.273   115   115 I DEBUG   : *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***",
                "04-25 18:33:27.273   115   115 I DEBUG   : Build fingerprint: 'product:build:target'",
                "04-25 18:33:27.273   115   115 I DEBUG   : pid: 3112, tid: 3112  >>> com.google.android.browser <<<",
                "04-25 18:33:27.273   115   115 I DEBUG   : signal 11 (SIGSEGV), code 1 (SEGV_MAPERR), fault addr 00000000",
                "04-25 18:33:27.273   115   115 I DEBUG   : *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***",
                "04-25 18:33:27.273   115   115 I DEBUG   : Build fingerprint: 'product:build:target'",
                "04-25 18:33:27.273   115   115 I DEBUG   : pid: 3113, tid: 3113  >>> com.google.android.browser2 <<<",
                "04-25 18:33:27.273   115   115 I DEBUG   : signal 11 (SIGSEGV), code 1 (SEGV_MAPERR), fault addr 00000000");

        LogcatItem logcat = new LogcatParser("2012").parse(lines);
        assertNotNull(logcat);
        assertEquals(parseTime("2012-04-25 18:33:27.273"), logcat.getStartTime());
        assertEquals(parseTime("2012-04-25 18:33:27.273"), logcat.getStopTime());
        assertEquals(2, logcat.getEvents().size());
        assertEquals(2, logcat.getNativeCrashes().size());
        assertEquals(3112, logcat.getNativeCrashes().get(0).getPid().intValue());
        assertEquals(3112, logcat.getNativeCrashes().get(0).getTid().intValue());
        assertEquals("com.google.android.browser", logcat.getNativeCrashes().get(0).getApp());
        assertEquals(3113, logcat.getNativeCrashes().get(1).getPid().intValue());
        assertEquals(3113, logcat.getNativeCrashes().get(1).getTid().intValue());
        assertEquals("com.google.android.browser2", logcat.getNativeCrashes().get(1).getApp());
    }

    public void testParse_misc_events() throws ParseException {
        List<String> lines = Arrays.asList(
                "04-25 18:33:27.273  1676  1821 W AudioTrack: obtainBuffer timed out (is the CPU pegged?) 0x361378 user=0000116a, server=00000000",
                "04-25 18:33:28.273  7813  7813 E gralloc : GetBufferLock timed out for thread 7813 buffer 0x61 usage 0x200 LockState 1",
                "04-25 18:33:29.273   395   637 W Watchdog: *** WATCHDOG KILLING SYSTEM PROCESS: null");

        LogcatItem logcat = new LogcatParser("2012").parse(lines);
        assertNotNull(logcat);
        assertEquals(parseTime("2012-04-25 18:33:27.273"), logcat.getStartTime());
        assertEquals(parseTime("2012-04-25 18:33:29.273"), logcat.getStopTime());
        assertEquals(3, logcat.getEvents().size());
        assertEquals(1, logcat.getMiscEvents(LogcatParser.HIGH_CPU_USAGE).size());
        assertEquals(1, logcat.getMiscEvents(LogcatParser.HIGH_MEMORY_USAGE).size());
        assertEquals(1, logcat.getMiscEvents(LogcatParser.RUNTIME_RESTART).size());

        MiscLogcatItem item = logcat.getMiscEvents(LogcatParser.HIGH_CPU_USAGE).get(0);

        assertEquals(1676, item.getPid().intValue());
        assertEquals(1821, item.getTid().intValue());
        assertEquals(parseTime("2012-04-25 18:33:27.273"), item.getEventTime());

        item = logcat.getMiscEvents(LogcatParser.HIGH_MEMORY_USAGE).get(0);

        assertEquals(7813, item.getPid().intValue());
        assertEquals(7813, item.getTid().intValue());
        assertEquals(parseTime("2012-04-25 18:33:28.273"), item.getEventTime());

        item = logcat.getMiscEvents(LogcatParser.RUNTIME_RESTART).get(0);

        assertEquals(395, item.getPid().intValue());
        assertEquals(637, item.getTid().intValue());
        assertEquals(parseTime("2012-04-25 18:33:29.273"), item.getEventTime());
    }

    /**
     * Test that multiple events can be parsed.
     */
    public void testParse_multiple_events() throws ParseException {
        List<String> lines = Arrays.asList(
                "04-25 09:55:47.799  3064  3082 E AndroidRuntime: java.lang.Exception",
                "04-25 09:55:47.799  3064  3082 E AndroidRuntime: \tat class.method1(Class.java:1)",
                "04-25 09:55:47.799  3064  3082 E AndroidRuntime: \tat class.method2(Class.java:2)",
                "04-25 09:55:47.799  3064  3082 E AndroidRuntime: \tat class.method3(Class.java:3)",
                "04-25 09:55:47.799  3065  3090 E AndroidRuntime: java.lang.Exception",
                "04-25 09:55:47.799  3065  3090 E AndroidRuntime: \tat class.method1(Class.java:1)",
                "04-25 09:55:47.799  3065  3090 E AndroidRuntime: \tat class.method2(Class.java:2)",
                "04-25 09:55:47.799  3065  3090 E AndroidRuntime: \tat class.method3(Class.java:3)",
                "04-25 17:17:08.445   312   366 E ActivityManager: ANR (application not responding) in process: com.android.package",
                "04-25 17:17:08.445   312   366 E ActivityManager: Reason: keyDispatchingTimedOut",
                "04-25 17:17:08.445   312   366 E ActivityManager: Load: 0.71 / 0.83 / 0.51",
                "04-25 17:17:08.445   312   366 E ActivityManager: 33% TOTAL: 21% user + 11% kernel + 0.3% iowait",
                "04-25 17:17:08.445   312   366 E ActivityManager: ANR (application not responding) in process: com.android.package",
                "04-25 17:17:08.445   312   366 E ActivityManager: Reason: keyDispatchingTimedOut",
                "04-25 17:17:08.445   312   366 E ActivityManager: Load: 0.71 / 0.83 / 0.51",
                "04-25 17:17:08.445   312   366 E ActivityManager: 33% TOTAL: 21% user + 11% kernel + 0.3% iowait",
                "04-25 18:33:27.273   115   115 I DEBUG   : *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***",
                "04-25 18:33:27.273   115   115 I DEBUG   : Build fingerprint: 'product:build:target'",
                "04-25 18:33:27.273   115   115 I DEBUG   : pid: 3112, tid: 3112  >>> com.google.android.browser <<<",
                "04-25 18:33:27.273   115   115 I DEBUG   : signal 11 (SIGSEGV), code 1 (SEGV_MAPERR), fault addr 00000000",
                "04-25 18:33:27.273   117   117 I DEBUG   : *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***",
                "04-25 18:33:27.273   117   117 I DEBUG   : Build fingerprint: 'product:build:target'",
                "04-25 18:33:27.273   117   117 I DEBUG   : pid: 3113, tid: 3113  >>> com.google.android.browser <<<",
                "04-25 18:33:27.273   117   117 I DEBUG   : signal 11 (SIGSEGV), code 1 (SEGV_MAPERR), fault addr 00000000");


        LogcatItem logcat = new LogcatParser("2012").parse(lines);
        assertNotNull(logcat);
        assertEquals(parseTime("2012-04-25 09:55:47.799"), logcat.getStartTime());
        assertEquals(parseTime("2012-04-25 18:33:27.273"), logcat.getStopTime());
        assertEquals(6, logcat.getEvents().size());
        assertEquals(2, logcat.getAnrs().size());
        assertEquals(2, logcat.getJavaCrashes().size());
        assertEquals(2, logcat.getNativeCrashes().size());

        assertEquals(312, logcat.getAnrs().get(0).getPid().intValue());
        assertEquals(366, logcat.getAnrs().get(0).getTid().intValue());
        assertEquals(parseTime("2012-04-25 17:17:08.445"), logcat.getAnrs().get(0).getEventTime());

        assertEquals(312, logcat.getAnrs().get(1).getPid().intValue());
        assertEquals(366, logcat.getAnrs().get(1).getTid().intValue());
        assertEquals(parseTime("2012-04-25 17:17:08.445"), logcat.getAnrs().get(1).getEventTime());

        assertEquals(3064, logcat.getJavaCrashes().get(0).getPid().intValue());
        assertEquals(3082, logcat.getJavaCrashes().get(0).getTid().intValue());
        assertEquals(
                parseTime("2012-04-25 09:55:47.799"),
                logcat.getJavaCrashes().get(0).getEventTime());

        assertEquals(3065, logcat.getJavaCrashes().get(1).getPid().intValue());
        assertEquals(3090, logcat.getJavaCrashes().get(1).getTid().intValue());
        assertEquals(
                parseTime("2012-04-25 09:55:47.799"),
                logcat.getJavaCrashes().get(1).getEventTime());

        assertEquals(3112, logcat.getNativeCrashes().get(0).getPid().intValue());
        assertEquals(3112, logcat.getNativeCrashes().get(0).getTid().intValue());
        assertEquals(
                parseTime("2012-04-25 18:33:27.273"),
                logcat.getNativeCrashes().get(0).getEventTime());

        assertEquals(3113, logcat.getNativeCrashes().get(1).getPid().intValue());
        assertEquals(3113, logcat.getNativeCrashes().get(1).getTid().intValue());
        assertEquals(
                parseTime("2012-04-25 18:33:27.273"),
                logcat.getNativeCrashes().get(1).getEventTime());
    }

    /** Test that including extra uid column still parses the logs. */
    public void testParse_uid() throws ParseException {
        List<String> lines =
                Arrays.asList(
                        "04-25 09:55:47.799  wifi  3064  3082 E AndroidRuntime: java.lang.Exception",
                        "04-25 09:55:47.799  wifi  3064  3082 E AndroidRuntime: \tat class.method1(Class.java:1)",
                        "04-25 09:55:47.799  wifi  3064  3082 E AndroidRuntime: \tat class.method2(Class.java:2)",
                        "04-25 09:55:47.799  wifi  3064  3082 E AndroidRuntime: \tat class.method3(Class.java:3)",
                        "04-25 09:55:47.799  wifi  3065  3090 E AndroidRuntime: java.lang.Exception",
                        "04-25 09:55:47.799  wifi  3065  3090 E AndroidRuntime: \tat class.method1(Class.java:1)",
                        "04-25 09:55:47.799  wifi  3065  3090 E AndroidRuntime: \tat class.method2(Class.java:2)",
                        "04-25 09:55:47.799  wifi  3065  3090 E AndroidRuntime: \tat class.method3(Class.java:3)",
                        "04-25 17:17:08.445  1337  312   366 E ActivityManager: ANR (application not responding) in process: com.android.package",
                        "04-25 17:17:08.445  1337  312   366 E ActivityManager: Reason: keyDispatchingTimedOut",
                        "04-25 17:17:08.445  1337  312   366 E ActivityManager: Load: 0.71 / 0.83 / 0.51",
                        "04-25 17:17:08.445  1337  312   366 E ActivityManager: 33% TOTAL: 21% user + 11% kernel + 0.3% iowait",
                        "04-25 17:17:08.445  1337  312   366 E ActivityManager: ANR (application not responding) in process: com.android.package",
                        "04-25 17:17:08.445  1337  312   366 E ActivityManager: Reason: keyDispatchingTimedOut",
                        "04-25 17:17:08.445  1337  312   366 E ActivityManager: Load: 0.71 / 0.83 / 0.51",
                        "04-25 17:17:08.445  1337  312   366 E ActivityManager: 33% TOTAL: 21% user + 11% kernel + 0.3% iowait",
                        "04-25 18:33:27.273  wifi123  115   115 I DEBUG   : *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***",
                        "04-25 18:33:27.273  wifi123  115   115 I DEBUG   : Build fingerprint: 'product:build:target'",
                        "04-25 18:33:27.273  wifi123  115   115 I DEBUG   : pid: 3112, tid: 3112  >>> com.google.android.browser <<<",
                        "04-25 18:33:27.273  wifi123  115   115 I DEBUG   : signal 11 (SIGSEGV), code 1 (SEGV_MAPERR), fault addr 00000000",
                        "04-25 18:33:27.273  wifi123  117   117 I DEBUG   : *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***",
                        "04-25 18:33:27.273  wifi123  117   117 I DEBUG   : Build fingerprint: 'product:build:target'",
                        "04-25 18:33:27.273  wifi123  117   117 I DEBUG   : pid: 3113, tid: 3113  >>> com.google.android.browser <<<",
                        "04-25 18:33:27.273  wifi123  117   117 I DEBUG   : signal 11 (SIGSEGV), code 1 (SEGV_MAPERR), fault addr 00000000");


        LogcatItem logcat = new LogcatParser("2012").parse(lines);
        assertNotNull(logcat);
        assertEquals(parseTime("2012-04-25 09:55:47.799"), logcat.getStartTime());
        assertEquals(parseTime("2012-04-25 18:33:27.273"), logcat.getStopTime());
        assertEquals(6, logcat.getEvents().size());
        assertEquals(2, logcat.getAnrs().size());
        assertEquals(2, logcat.getJavaCrashes().size());
        assertEquals(2, logcat.getNativeCrashes().size());

        assertEquals(312, logcat.getAnrs().get(0).getPid().intValue());
        assertEquals(366, logcat.getAnrs().get(0).getTid().intValue());
        assertEquals(parseTime("2012-04-25 17:17:08.445"), logcat.getAnrs().get(0).getEventTime());

        assertEquals(312, logcat.getAnrs().get(1).getPid().intValue());
        assertEquals(366, logcat.getAnrs().get(1).getTid().intValue());
        assertEquals(parseTime("2012-04-25 17:17:08.445"), logcat.getAnrs().get(1).getEventTime());

        assertEquals(3064, logcat.getJavaCrashes().get(0).getPid().intValue());
        assertEquals(3082, logcat.getJavaCrashes().get(0).getTid().intValue());
        assertEquals(parseTime("2012-04-25 09:55:47.799"),
                logcat.getJavaCrashes().get(0).getEventTime());

        assertEquals(3065, logcat.getJavaCrashes().get(1).getPid().intValue());
        assertEquals(3090, logcat.getJavaCrashes().get(1).getTid().intValue());
        assertEquals(parseTime("2012-04-25 09:55:47.799"),
                logcat.getJavaCrashes().get(1).getEventTime());

        assertEquals(3112, logcat.getNativeCrashes().get(0).getPid().intValue());
        assertEquals(3112, logcat.getNativeCrashes().get(0).getTid().intValue());
        assertEquals(parseTime("2012-04-25 18:33:27.273"),
                logcat.getNativeCrashes().get(0).getEventTime());

        assertEquals(3113, logcat.getNativeCrashes().get(1).getPid().intValue());
        assertEquals(3113, logcat.getNativeCrashes().get(1).getTid().intValue());
        assertEquals(parseTime("2012-04-25 18:33:27.273"),
                logcat.getNativeCrashes().get(1).getEventTime());
    }

    /**
     * Test that multiple java crashes and native crashes can be parsed even when interleaved.
     */
    public void testParse_multiple_events_interleaved() throws ParseException {
        List<String> lines = Arrays.asList(
                "04-25 09:55:47.799  3064  3082 E AndroidRuntime: java.lang.Exception",
                "04-25 09:55:47.799  3065  3090 E AndroidRuntime: java.lang.Exception",
                "04-25 09:55:47.799   115   115 I DEBUG   : *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***",
                "04-25 09:55:47.799   117   117 I DEBUG   : *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***",
                "04-25 09:55:47.799  3064  3082 E AndroidRuntime: \tat class.method1(Class.java:1)",
                "04-25 09:55:47.799  3065  3090 E AndroidRuntime: \tat class.method1(Class.java:1)",
                "04-25 09:55:47.799   115   115 I DEBUG   : Build fingerprint: 'product:build:target'",
                "04-25 09:55:47.799   117   117 I DEBUG   : Build fingerprint: 'product:build:target'",
                "04-25 09:55:47.799  3064  3082 E AndroidRuntime: \tat class.method2(Class.java:2)",
                "04-25 09:55:47.799  3065  3090 E AndroidRuntime: \tat class.method2(Class.java:2)",
                "04-25 09:55:47.799   115   115 I DEBUG   : pid: 3112, tid: 3112  >>> com.google.android.browser <<<",
                "04-25 09:55:47.799   117   117 I DEBUG   : pid: 3113, tid: 3113  >>> com.google.android.browser <<<",
                "04-25 09:55:47.799  3064  3082 E AndroidRuntime: \tat class.method3(Class.java:3)",
                "04-25 09:55:47.799  3065  3090 E AndroidRuntime: \tat class.method3(Class.java:3)",
                "04-25 09:55:47.799   115   115 I DEBUG   : signal 11 (SIGSEGV), code 1 (SEGV_MAPERR), fault addr 00000000",
                "04-25 09:55:47.799   117   117 I DEBUG   : signal 11 (SIGSEGV), code 1 (SEGV_MAPERR), fault addr 00000000");

        LogcatItem logcat = new LogcatParser("2012").parse(lines);
        assertNotNull(logcat);
        assertEquals(parseTime("2012-04-25 09:55:47.799"), logcat.getStartTime());
        assertEquals(parseTime("2012-04-25 09:55:47.799"), logcat.getStopTime());
        assertEquals(4, logcat.getEvents().size());
        assertEquals(0, logcat.getAnrs().size());
        assertEquals(2, logcat.getJavaCrashes().size());
        assertEquals(2, logcat.getNativeCrashes().size());

        assertEquals(3064, logcat.getJavaCrashes().get(0).getPid().intValue());
        assertEquals(3082, logcat.getJavaCrashes().get(0).getTid().intValue());
        assertEquals(parseTime("2012-04-25 09:55:47.799"),
                logcat.getJavaCrashes().get(0).getEventTime());

        assertEquals(3065, logcat.getJavaCrashes().get(1).getPid().intValue());
        assertEquals(3090, logcat.getJavaCrashes().get(1).getTid().intValue());
        assertEquals(parseTime("2012-04-25 09:55:47.799"),
                logcat.getJavaCrashes().get(1).getEventTime());

        assertEquals(3112, logcat.getNativeCrashes().get(0).getPid().intValue());
        assertEquals(3112, logcat.getNativeCrashes().get(0).getTid().intValue());
        assertEquals(parseTime("2012-04-25 09:55:47.799"),
                logcat.getNativeCrashes().get(0).getEventTime());

        assertEquals(3113, logcat.getNativeCrashes().get(1).getPid().intValue());
        assertEquals(3113, logcat.getNativeCrashes().get(1).getTid().intValue());
        assertEquals(parseTime("2012-04-25 09:55:47.799"),
                logcat.getNativeCrashes().get(1).getEventTime());
    }

    /**
     * Test that the preambles are set correctly.
     */
    public void testParse_preambles() throws ParseException {
        List<String> lines = Arrays.asList(
                "04-25 09:15:47.799   123  3082 I tag: message 1",
                "04-25 09:20:47.799  3064  3082 I tag: message 2",
                "04-25 09:25:47.799   345  3082 I tag: message 3",
                "04-25 09:30:47.799  3064  3082 I tag: message 4",
                "04-25 09:35:47.799   456  3082 I tag: message 5",
                "04-25 09:40:47.799  3064  3082 I tag: message 6",
                "04-25 09:45:47.799   567  3082 I tag: message 7",
                "04-25 09:50:47.799  3064  3082 I tag: message 8",
                "04-25 09:55:47.799  3064  3082 E AndroidRuntime: java.lang.Exception",
                "04-25 09:55:47.799  3064  3082 E AndroidRuntime: \tat class.method1(Class.java:1)",
                "04-25 09:55:47.799  3064  3082 E AndroidRuntime: \tat class.method2(Class.java:2)",
                "04-25 09:55:47.799  3064  3082 E AndroidRuntime: \tat class.method3(Class.java:3)");

        List<String> expectedLastPreamble = Arrays.asList(
                "04-25 09:15:47.799   123  3082 I tag: message 1",
                "04-25 09:20:47.799  3064  3082 I tag: message 2",
                "04-25 09:25:47.799   345  3082 I tag: message 3",
                "04-25 09:30:47.799  3064  3082 I tag: message 4",
                "04-25 09:35:47.799   456  3082 I tag: message 5",
                "04-25 09:40:47.799  3064  3082 I tag: message 6",
                "04-25 09:45:47.799   567  3082 I tag: message 7",
                "04-25 09:50:47.799  3064  3082 I tag: message 8");

        List<String> expectedProcPreamble = Arrays.asList(
                "04-25 09:20:47.799  3064  3082 I tag: message 2",
                "04-25 09:30:47.799  3064  3082 I tag: message 4",
                "04-25 09:40:47.799  3064  3082 I tag: message 6",
                "04-25 09:50:47.799  3064  3082 I tag: message 8");

        LogcatItem logcat = new LogcatParser("2012").parse(lines);
        assertNotNull(logcat);
        assertEquals(parseTime("2012-04-25 09:15:47.799"), logcat.getStartTime());
        assertEquals(parseTime("2012-04-25 09:55:47.799"), logcat.getStopTime());
        assertEquals(1, logcat.getEvents().size());
        assertEquals(1, logcat.getJavaCrashes().size());
        assertEquals(3064, logcat.getJavaCrashes().get(0).getPid().intValue());
        assertEquals(3082, logcat.getJavaCrashes().get(0).getTid().intValue());
        assertEquals(ArrayUtil.join("\n", expectedLastPreamble),
                logcat.getJavaCrashes().get(0).getLastPreamble());
        assertEquals(ArrayUtil.join("\n", expectedProcPreamble),
                logcat.getJavaCrashes().get(0).getProcessPreamble());
        assertEquals(parseTime("2012-04-25 09:55:47.799"),
                logcat.getJavaCrashes().get(0).getEventTime());
    }

    /**
     * Test that events while the device is rebooting are ignored.
     */
    public void testParse_reboot() throws ParseException {
        List<String> lines = Arrays.asList(
                "04-25 09:15:47.799   123  3082 I ShutdownThread: Rebooting, reason: null",
                "04-25 09:55:47.799  3064  3082 E AndroidRuntime: java.lang.Exception",
                "04-25 09:55:47.799  3064  3082 E AndroidRuntime: \tat class.method1(Class.java:1)",
                "04-25 09:55:47.799  3064  3082 E AndroidRuntime: \tat class.method2(Class.java:2)",
                "04-25 09:55:47.799  3064  3082 E AndroidRuntime: \tat class.method3(Class.java:3)");

        LogcatItem logcat = new LogcatParser("2012").parse(lines);
        assertNotNull(logcat);
        assertEquals(parseTime("2012-04-25 09:15:47.799"), logcat.getStartTime());
        assertEquals(parseTime("2012-04-25 09:55:47.799"), logcat.getStopTime());
        assertEquals(0, logcat.getEvents().size());
    }

    /**
     * Test that events while the device is rebooting are ignored, but devices after the reboot are
     * captured.
     */
    public void testParse_reboot_resume() throws ParseException {
        List<String> lines = Arrays.asList(
                "04-25 09:15:47.799   123  3082 I ShutdownThread: Rebooting, reason: null",
                "04-25 09:55:47.799  3064  3082 E AndroidRuntime: java.lang.Exception",
                "04-25 09:55:47.799  3064  3082 E AndroidRuntime: \tat class.method1(Class.java:1)",
                "04-25 09:55:47.799  3064  3082 E AndroidRuntime: \tat class.method2(Class.java:2)",
                "04-25 09:55:47.799  3064  3082 E AndroidRuntime: \tat class.method3(Class.java:3)",
                "logcat interrupted. May see duplicated content in log.--------- beginning of /dev/log/main",
                "04-25 09:59:47.799  3064  3082 E AndroidRuntime: java.lang.Exception2",
                "04-25 09:59:47.799  3064  3082 E AndroidRuntime: \tat class.method1(Class.java:1)",
                "04-25 09:59:47.799  3064  3082 E AndroidRuntime: \tat class.method2(Class.java:2)",
                "04-25 09:59:47.799  3064  3082 E AndroidRuntime: \tat class.method3(Class.java:3)");


        LogcatItem logcat = new LogcatParser("2012").parse(lines);
        assertNotNull(logcat);
        assertEquals(parseTime("2012-04-25 09:15:47.799"), logcat.getStartTime());
        assertEquals(parseTime("2012-04-25 09:59:47.799"), logcat.getStopTime());
        assertEquals(1, logcat.getEvents().size());
        assertEquals("java.lang.Exception2", logcat.getJavaCrashes().get(0).getException());

        lines = Arrays.asList(
                "04-25 09:15:47.799   123  3082 I ShutdownThread: Rebooting, reason: null",
                "04-25 09:55:47.799  3064  3082 E AndroidRuntime: java.lang.Exception",
                "04-25 09:55:47.799  3064  3082 E AndroidRuntime: \tat class.method1(Class.java:1)",
                "04-25 09:55:47.799  3064  3082 E AndroidRuntime: \tat class.method2(Class.java:2)",
                "04-25 09:55:47.799  3064  3082 E AndroidRuntime: \tat class.method3(Class.java:3)",
                "logcat interrupted. May see duplicated content in log.--------- beginning of main",
                "04-25 09:59:47.799  3064  3082 E AndroidRuntime: java.lang.Exception2",
                "04-25 09:59:47.799  3064  3082 E AndroidRuntime: \tat class.method1(Class.java:1)",
                "04-25 09:59:47.799  3064  3082 E AndroidRuntime: \tat class.method2(Class.java:2)",
                "04-25 09:59:47.799  3064  3082 E AndroidRuntime: \tat class.method3(Class.java:3)");


        logcat = new LogcatParser("2012").parse(lines);
        assertNotNull(logcat);
        assertEquals(parseTime("2012-04-25 09:15:47.799"), logcat.getStartTime());
        assertEquals(parseTime("2012-04-25 09:59:47.799"), logcat.getStopTime());
        assertEquals(1, logcat.getEvents().size());
        assertEquals("java.lang.Exception2", logcat.getJavaCrashes().get(0).getException());
    }

    /**
     * Test that the time logcat format can be parsed.
     */
    public void testParse_time() throws ParseException {
        List<String> lines = Arrays.asList(
                "04-25 09:55:47.799  E/AndroidRuntime(3064): java.lang.Exception",
                "04-25 09:55:47.799  E/AndroidRuntime(3064): \tat class.method1(Class.java:1)",
                "04-25 09:55:47.799  E/AndroidRuntime(3064): \tat class.method2(Class.java:2)",
                "04-25 09:55:47.799  E/AndroidRuntime(3064): \tat class.method3(Class.java:3)");

        LogcatItem logcat = new LogcatParser("2012").parse(lines);
        assertNotNull(logcat);
        assertEquals(parseTime("2012-04-25 09:55:47.799"), logcat.getStartTime());
        assertEquals(parseTime("2012-04-25 09:55:47.799"), logcat.getStopTime());
        assertEquals(1, logcat.getEvents().size());
        assertEquals(1, logcat.getJavaCrashes().size());
        assertEquals(3064, logcat.getJavaCrashes().get(0).getPid().intValue());
        assertNull(logcat.getJavaCrashes().get(0).getTid());
        assertEquals(parseTime("2012-04-25 09:55:47.799"),
                logcat.getJavaCrashes().get(0).getEventTime());
    }

    /**
     * Test that we can add and find custom patterns that match based on logcat Tags only.
     */
    public void testAddPattern_byTagOnly() {
        List<String> lines = Arrays.asList(
                "04-25 18:33:28.273  7813  7813 E HelloTag: Hello everyone!!!1",
                "04-25 18:33:29.273   395   637 I Watchdog: find me!",
                "04-25 18:33:39.273   395   637 W Watchdog: find me!");

        LogcatParser parser = new LogcatParser("2012");
        assertNotNull(parser);
        parser.addPattern(null, null, "HelloTag", "HelloCategory");
        parser.addPattern(null, null, "Watchdog", "WatchdogCategory");
        LogcatItem logcat = parser.parse(lines);
        assertNotNull(logcat);

        /* verify that we find the HelloTag entry */
        List<MiscLogcatItem> matchedEvents = logcat.getMiscEvents("HelloCategory");
        assertEquals(1, matchedEvents.size());
        assertEquals("HelloTag", matchedEvents.get(0).getTag());

        /* verify that we find both Watchdog entries */
        matchedEvents = logcat.getMiscEvents("WatchdogCategory");
        assertEquals(2, matchedEvents.size());
        assertEquals("Watchdog", matchedEvents.get(0).getTag());
    }

    /**
     * Test that we can add and find custom patterns that match based on Level AND Tag.
     */
    public void testAddPattern_byLevelAndTagOnly() {
        List<String> lines = Arrays.asList(
                "04-25 18:33:28.273  7813  7813 E HelloTag: GetBufferLock timed out for thread 7813 buffer 0x61 usage 0x200 LockState 1",
                "04-25 18:33:29.273   395   637 I Watchdog: Info",
                "04-25 18:33:29.273   395   637 W Watchdog: Warning");

        LogcatParser parser = new LogcatParser("2012");
        assertNotNull(parser);
        parser.addPattern(null, "I", "Watchdog", "WatchdogCategory");
        LogcatItem logcat = parser.parse(lines);
        assertNotNull(logcat);

        List<MiscLogcatItem> matchedEvents = logcat.getMiscEvents("WatchdogCategory");
        assertEquals(1, matchedEvents.size());
        assertEquals("Watchdog", matchedEvents.get(0).getTag());
        assertEquals("Info", matchedEvents.get(0).getStack());
    }

    /**
     * Test that we can add and find custom patterns that match based on Level, Tag, AND Message.
     */
    public void testAddPattern_byLevelTagAndMessageOnly() {
        List<String> lines = Arrays.asList(
                "04-25 18:33:29.273   395   637 W Watchdog: I'm the one you need to find!",
                "04-25 18:33:29.273   395   637 W Watchdog: my message doesn't match.",
                "04-25 18:33:29.273   395   637 I Watchdog: my Level doesn't match, try and find me!",
                "04-25 18:33:29.273   395   637 W NotMe: my Tag doesn't match, try and find me!");

        LogcatParser parser = new LogcatParser("2012");
        assertNotNull(parser);
        parser.addPattern(Pattern.compile(".*find*."), "W", "Watchdog", "WatchdogCategory");
        LogcatItem logcat = parser.parse(lines);
        assertNotNull(logcat);

        /* verify that we find the only entry that matches based on Level, Tag, AND Message */
        List<MiscLogcatItem> matchedEvents = logcat.getMiscEvents("WatchdogCategory");
        assertEquals(1, matchedEvents.size());
        assertEquals("Watchdog", matchedEvents.get(0).getTag());
        assertEquals("I'm the one you need to find!", matchedEvents.get(0).getStack());
    }

    public void testFatalException() {
        List<String> lines = Arrays.asList(
                "06-05 06:14:51.529  1712  1712 D AndroidRuntime: Calling main entry com.android.commands.input.Input",
                "06-05 06:14:51.709  1712  1712 E AndroidRuntime: *** FATAL EXCEPTION IN SYSTEM PROCESS: main",
                "06-05 06:14:51.709  1712  1712 E AndroidRuntime: java.lang.NullPointerException",
                "06-05 06:14:51.709  1712  1712 E AndroidRuntime: \tat android.hardware.input.InputManager.injectInputEvent(InputManager.java:641)",
                "06-05 06:14:51.709  1712  1712 E AndroidRuntime: \tat com.android.commands.input.Input.injectKeyEvent(Input.java:233)",
                "06-05 06:14:51.709  1712  1712 E AndroidRuntime: \tat com.android.commands.input.Input.sendKeyEvent(Input.java:184)",
                "06-05 06:14:51.709  1712  1712 E AndroidRuntime: \tat com.android.commands.input.Input.run(Input.java:96)",
                "06-05 06:14:51.709  1712  1712 E AndroidRuntime: \tat com.android.commands.input.Input.main(Input.java:59)",
                "06-05 06:14:51.709  1712  1712 E AndroidRuntime: \tat com.android.internal.os.RuntimeInit.nativeFinishInit(Native Method)",
                "06-05 06:14:51.709  1712  1712 E AndroidRuntime: \tat com.android.internal.os.RuntimeInit.main(RuntimeInit.java:243)",
                "06-05 06:14:51.709  1712  1712 E AndroidRuntime: \tat dalvik.system.NativeStart.main(Native Method)");
        LogcatParser parser = new LogcatParser("2014");
        LogcatItem logcat = parser.parse(lines);
        assertEquals(1, logcat.getJavaCrashes().size());
        JavaCrashItem crash = logcat.getJavaCrashes().get(0);
        assertEquals("com.android.commands.input.Input", crash.getApp());
    }

    /**
     * Test that an empty input returns {@code null}.
     */
    public void testEmptyInput() {
        LogcatItem item = new LogcatParser().parse(Arrays.asList(""));
        assertNull(item);
    }

    /**
     * Test that after clearing a parser, reusing it produces a new LogcatItem instead of
     * appending to the previous one.
     */
    public void testClear() {
        List<String> lines = Arrays.asList(
                "04-25 18:33:28.273  7813  7813 E HelloTag: GetBufferLock timed out for thread 7813 buffer 0x61 usage 0x200 LockState 1",
                "04-25 18:33:28.273  7813  7813 E GoodbyeTag: GetBufferLock timed out for thread 7813 buffer 0x61 usage 0x200 LockState 1"
                );
        LogcatParser parser = new LogcatParser();
        LogcatItem l1 = parser.parse(lines.subList(0, 1));
        parser.clear();
        LogcatItem l2 = parser.parse(lines.subList(1, 2));
        assertEquals(l1.getEvents().size(), 1);
        assertEquals(l2.getEvents().size(), 1);
        assertFalse(l1.getEvents().get(0).getTag().equals(l2.getEvents().get(0).getTag()));
    }

    private Date parseTime(String timeStr) throws ParseException {
        DateFormat formatter = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss.SSS");
        return formatter.parse(timeStr);
    }
}
