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
import com.android.loganalysis.item.JavaCrashItem;
import com.android.loganalysis.item.MonkeyLogItem;
import com.android.loganalysis.item.MonkeyLogItem.DroppedCategory;
import com.android.loganalysis.item.NativeCrashItem;
import com.android.loganalysis.util.ArrayUtil;

import junit.framework.TestCase;

import java.text.DateFormat;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Arrays;
import java.util.Date;
import java.util.List;

/**
 * Unit tests for {@link MonkeyLogParser}
 */
public class MonkeyLogParserTest extends TestCase {

    /**
     * Test that a monkey can be parsed if there are no crashes.
     */
    public void testParse_success() {
        List<String> lines = Arrays.asList(
                "# Wednesday, 04/25/2012 01:37:12 AM - device uptime = 242.13: Monkey command used for this test:",
                "adb shell monkey -p com.google.android.browser  -c android.intent.category.SAMPLE_CODE -c android.intent.category.CAR_DOCK -c android.intent.category.LAUNCHER -c android.intent.category.MONKEY -c android.intent.category.INFO  --ignore-security-exceptions --throttle 100  -s 528 -v -v -v 10000 ",
                "",
                ":Monkey: seed=528 count=10000",
                ":AllowPackage: com.google.android.browser",
                ":IncludeCategory: android.intent.category.LAUNCHER",
                ":Switch: #Intent;action=android.intent.action.MAIN;category=android.intent.category.LAUNCHER;launchFlags=0x10200000;component=com.google.android.browser/com.android.browser.BrowserActivity;end",
                "    // Allowing start of Intent { act=android.intent.action.MAIN cat=[android.intent.category.LAUNCHER] cmp=com.google.android.browser/com.android.browser.BrowserActivity } in package com.google.android.browser",
                "Sleeping for 100 milliseconds",
                ":Sending Key (ACTION_DOWN): 23    // KEYCODE_DPAD_CENTER",
                ":Sending Key (ACTION_UP): 23    // KEYCODE_DPAD_CENTER",
                "Sleeping for 100 milliseconds",
                ":Sending Trackball (ACTION_MOVE): 0:(-5.0,3.0)",
                ":Sending Trackball (ACTION_MOVE): 0:(3.0,3.0)",
                ":Sending Trackball (ACTION_MOVE): 0:(-1.0,3.0)",
                ":Sending Trackball (ACTION_MOVE): 0:(4.0,-2.0)",
                ":Sending Trackball (ACTION_MOVE): 0:(1.0,4.0)",
                ":Sending Trackball (ACTION_MOVE): 0:(-4.0,2.0)",
                "    //[calendar_time:2012-04-25 01:42:20.140  system_uptime:535179]",
                "    // Sending event #9900",
                ":Sending Trackball (ACTION_MOVE): 0:(2.0,-4.0)",
                ":Sending Trackball (ACTION_MOVE): 0:(-2.0,0.0)",
                ":Sending Trackball (ACTION_MOVE): 0:(2.0,2.0)",
                ":Sending Trackball (ACTION_MOVE): 0:(-5.0,4.0)",
                "Events injected: 10000",
                ":Dropped: keys=5 pointers=6 trackballs=7 flips=8 rotations=9",
                "// Monkey finished",
                "",
                "# Wednesday, 04/25/2012 01:42:09 AM - device uptime = 539.21: Monkey command ran for: 04:57 (mm:ss)",
                "",
                "----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------");

        MonkeyLogItem monkeyLog = new MonkeyLogParser().parse(lines);
        assertNotNull(monkeyLog);
        // FIXME: Add test back once time situation has been worked out.
        // assertEquals(parseTime("2012-04-25 01:37:12"), monkeyLog.getStartTime());
        // assertEquals(parseTime("2012-04-25 01:42:09"), monkeyLog.getStopTime());
        assertEquals(1, monkeyLog.getPackages().size());
        assertTrue(monkeyLog.getPackages().contains("com.google.android.browser"));
        assertEquals(1, monkeyLog.getCategories().size());
        assertTrue(monkeyLog.getCategories().contains("android.intent.category.LAUNCHER"));
        assertEquals(100, monkeyLog.getThrottle());
        assertEquals(528, monkeyLog.getSeed().intValue());
        assertEquals(10000, monkeyLog.getTargetCount().intValue());
        assertTrue(monkeyLog.getIgnoreSecurityExceptions());
        assertEquals(4 * 60 * 1000 + 57 * 1000, monkeyLog.getTotalDuration().longValue());
        assertEquals(242130, monkeyLog.getStartUptimeDuration().longValue());
        assertEquals(539210, monkeyLog.getStopUptimeDuration().longValue());
        assertTrue(monkeyLog.getIsFinished());
        assertFalse(monkeyLog.getNoActivities());
        assertEquals(9900, monkeyLog.getIntermediateCount());
        assertEquals(10000, monkeyLog.getFinalCount().intValue());
        assertEquals(5, monkeyLog.getDroppedCount(DroppedCategory.KEYS).intValue());
        assertEquals(6, monkeyLog.getDroppedCount(DroppedCategory.POINTERS).intValue());
        assertEquals(7, monkeyLog.getDroppedCount(DroppedCategory.TRACKBALLS).intValue());
        assertEquals(8, monkeyLog.getDroppedCount(DroppedCategory.FLIPS).intValue());
        assertEquals(9, monkeyLog.getDroppedCount(DroppedCategory.ROTATIONS).intValue());
        assertNull(monkeyLog.getCrash());
    }

    /**
     * Test that a monkey can be parsed if there is an ANR.
     */
    public void testParse_anr() {
        List<String> lines = Arrays.asList(
                "# Tuesday, 04/24/2012 05:23:30 PM - device uptime = 216.48: Monkey command used for this test:",
                "adb shell monkey -p com.google.android.youtube  -c android.intent.category.SAMPLE_CODE -c android.intent.category.CAR_DOCK -c android.intent.category.LAUNCHER -c android.intent.category.MONKEY -c android.intent.category.INFO  --ignore-security-exceptions --throttle 100  -s 993 -v -v -v 10000 ",
                "",
                ":Monkey: seed=993 count=10000",
                ":AllowPackage: com.google.android.youtube",
                ":IncludeCategory: android.intent.category.LAUNCHER",
                ":Switch: #Intent;action=android.intent.action.MAIN;category=android.intent.category.LAUNCHER;launchFlags=0x10200000;component=com.google.android.youtube/.app.honeycomb.Shell%24HomeActivity;end",
                "    // Allowing start of Intent { act=android.intent.action.MAIN cat=[android.intent.category.LAUNCHER] cmp=com.google.android.youtube/.app.honeycomb.Shell$HomeActivity } in package com.google.android.youtube",
                "Sleeping for 100 milliseconds",
                ":Sending Key (ACTION_UP): 21    // KEYCODE_DPAD_LEFT",
                "Sleeping for 100 milliseconds",
                ":Sending Key (ACTION_DOWN): 22    // KEYCODE_DPAD_RIGHT",
                ":Sending Key (ACTION_UP): 22    // KEYCODE_DPAD_RIGHT",
                "    //[calendar_time:2012-04-25 00:27:27.155  system_uptime:454996]",
                "    // Sending event #5300",
                ":Sending Key (ACTION_UP): 19    // KEYCODE_DPAD_UP",
                "Sleeping for 100 milliseconds",
                ":Sending Trackball (ACTION_MOVE): 0:(4.0,3.0)",
                ":Sending Key (ACTION_DOWN): 20    // KEYCODE_DPAD_DOWN",
                ":Sending Key (ACTION_UP): 20    // KEYCODE_DPAD_DOWN",
                "// NOT RESPONDING: com.google.android.youtube (pid 3301)",
                "ANR in com.google.android.youtube (com.google.android.youtube/.app.honeycomb.phone.WatchActivity)",
                "Reason: keyDispatchingTimedOut",
                "Load: 1.0 / 1.05 / 0.6",
                "CPU usage from 4794ms to -1502ms ago with 99% awake:",
                "  18% 3301/com.google.android.youtube: 16% user + 2.3% kernel / faults: 268 minor 9 major",
                "  13% 313/system_server: 9.2% user + 4.4% kernel / faults: 906 minor 3 major",
                "  10% 117/surfaceflinger: 4.9% user + 5.5% kernel / faults: 1 minor",
                "  10% 120/mediaserver: 6.8% user + 3.6% kernel / faults: 1189 minor",
                "34% TOTAL: 19% user + 13% kernel + 0.2% iowait + 1% softirq",
                "",
                "procrank:",
                "// procrank status was 0",
                "anr traces:",
                "",
                "",
                "----- pid 2887 at 2012-04-25 17:17:08 -----",
                "Cmd line: com.google.android.youtube",
                "",
                "DALVIK THREADS:",
                "(mutexes: tll=0 tsl=0 tscl=0 ghl=0)",
                "",
                "\"main\" prio=5 tid=1 SUSPENDED",
                "  | group=\"main\" sCount=1 dsCount=0 obj=0x00000001 self=0x00000001",
                "  | sysTid=2887 nice=0 sched=0/0 cgrp=foreground handle=0000000001",
                "  | schedstat=( 0 0 0 ) utm=5954 stm=1017 core=0",
                "  at class.method1(Class.java:1)",
                "  at class.method2(Class.java:2)",
                "  at class.method2(Class.java:2)",
                "",
                "----- end 2887 -----",
                "// anr traces status was 0",
                "** Monkey aborted due to error.",
                "Events injected: 5322",
                ":Sending rotation degree=0, persist=false",
                ":Dropped: keys=1 pointers=0 trackballs=0 flips=0 rotations=0",
                "## Network stats: elapsed time=252942ms (0ms mobile, 252942ms wifi, 0ms not connected)",
                "** System appears to have crashed at event 5322 of 10000 using seed 993",
                "",
                "# Tuesday, 04/24/2012 05:27:44 PM - device uptime = 471.37: Monkey command ran for: 04:14 (mm:ss)",
                "",
                "----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------",
                "");

        List<String> expectedStack = Arrays.asList(
                "\"main\" prio=5 tid=1 SUSPENDED",
                "  | group=\"main\" sCount=1 dsCount=0 obj=0x00000001 self=0x00000001",
                "  | sysTid=2887 nice=0 sched=0/0 cgrp=foreground handle=0000000001",
                "  | schedstat=( 0 0 0 ) utm=5954 stm=1017 core=0",
                "  at class.method1(Class.java:1)",
                "  at class.method2(Class.java:2)",
                "  at class.method2(Class.java:2)");

        MonkeyLogItem monkeyLog = new MonkeyLogParser().parse(lines);
        assertNotNull(monkeyLog);
        // FIXME: Add test back once time situation has been worked out.
        // assertEquals(parseTime("2012-04-24 17:23:30"), monkeyLog.getStartTime());
        // assertEquals(parseTime("2012-04-24 17:27:44"), monkeyLog.getStopTime());
        assertEquals(1, monkeyLog.getPackages().size());
        assertTrue(monkeyLog.getPackages().contains("com.google.android.youtube"));
        assertEquals(1, monkeyLog.getCategories().size());
        assertTrue(monkeyLog.getCategories().contains("android.intent.category.LAUNCHER"));
        assertEquals(100, monkeyLog.getThrottle());
        assertEquals(993, monkeyLog.getSeed().intValue());
        assertEquals(10000, monkeyLog.getTargetCount().intValue());
        assertTrue(monkeyLog.getIgnoreSecurityExceptions());
        assertEquals(4 * 60 * 1000 + 14 * 1000, monkeyLog.getTotalDuration().longValue());
        assertEquals(216480, monkeyLog.getStartUptimeDuration().longValue());
        assertEquals(471370, monkeyLog.getStopUptimeDuration().longValue());
        assertFalse(monkeyLog.getIsFinished());
        assertFalse(monkeyLog.getNoActivities());
        assertEquals(5300, monkeyLog.getIntermediateCount());
        assertEquals(5322, monkeyLog.getFinalCount().intValue());
        assertNotNull(monkeyLog.getCrash());
        assertTrue(monkeyLog.getCrash() instanceof AnrItem);
        assertEquals("com.google.android.youtube", monkeyLog.getCrash().getApp());
        assertEquals(3301, monkeyLog.getCrash().getPid().intValue());
        assertEquals("keyDispatchingTimedOut", ((AnrItem) monkeyLog.getCrash()).getReason());
        assertEquals(ArrayUtil.join("\n", expectedStack),
                ((AnrItem) monkeyLog.getCrash()).getTrace());
    }

    /**
     * Test that a monkey can be parsed if there is a Java crash.
     */
    public void testParse_java_crash() {
        List<String> lines = Arrays.asList(
                "# Tuesday, 04/24/2012 05:05:50 PM - device uptime = 232.65: Monkey command used for this test:",
                "adb shell monkey -p com.google.android.apps.maps  -c android.intent.category.SAMPLE_CODE -c android.intent.category.CAR_DOCK -c android.intent.category.LAUNCHER -c android.intent.category.MONKEY -c android.intent.category.INFO  --ignore-security-exceptions --throttle 100  -s 501 -v -v -v 10000 ",
                "",
                ":Monkey: seed=501 count=10000",
                ":AllowPackage: com.google.android.apps.maps",
                ":IncludeCategory: android.intent.category.LAUNCHER",
                ":Switch: #Intent;action=android.intent.action.MAIN;category=android.intent.category.LAUNCHER;launchFlags=0x10200000;component=com.google.android.apps.maps/com.google.android.maps.LatitudeActivity;end",
                "    // Allowing start of Intent { act=android.intent.action.MAIN cat=[android.intent.category.LAUNCHER] cmp=com.google.android.apps.maps/com.google.android.maps.LatitudeActivity } in package com.google.android.apps.maps",
                "Sleeping for 100 milliseconds",
                ":Sending Touch (ACTION_DOWN): 0:(332.0,70.0)",
                ":Sending Touch (ACTION_UP): 0:(332.55292,76.54678)",
                "    //[calendar_time:2012-04-25 00:06:38.419  system_uptime:280799]",
                "    // Sending event #1600",
                ":Sending Touch (ACTION_MOVE): 0:(1052.2666,677.64594)",
                ":Sending Touch (ACTION_UP): 0:(1054.7593,687.3757)",
                "Sleeping for 100 milliseconds",
                "// CRASH: com.google.android.apps.maps (pid 3161)",
                "// Short Msg: java.lang.Exception",
                "// Long Msg: java.lang.Exception: This is the message",
                "// Build Label: google/yakju/maguro:JellyBean/JRN24B/338896:userdebug/dev-keys",
                "// Build Changelist: 338896",
                "// Build Time: 1335309051000",
                "// java.lang.Exception: This is the message",
                "// \tat class.method1(Class.java:1)",
                "// \tat class.method2(Class.java:2)",
                "// \tat class.method3(Class.java:3)",
                "// ",
                "** Monkey aborted due to error.",
                "Events injected: 1649",
                ":Sending rotation degree=0, persist=false",
                ":Dropped: keys=0 pointers=0 trackballs=0 flips=0 rotations=0",
                "## Network stats: elapsed time=48897ms (0ms mobile, 48897ms wifi, 0ms not connected)",
                "** System appears to have crashed at event 1649 of 10000 using seed 501",
                "",
                "# Tuesday, 04/24/2012 05:06:40 PM - device uptime = 282.53: Monkey command ran for: 00:49 (mm:ss)",
                "",
                "----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------",
                "");

        MonkeyLogItem monkeyLog = new MonkeyLogParser().parse(lines);
        assertNotNull(monkeyLog);
        // FIXME: Add test back once time situation has been worked out.
        // assertEquals(parseTime("2012-04-24 17:05:50"), monkeyLog.getStartTime());
        // assertEquals(parseTime("2012-04-24 17:06:40"), monkeyLog.getStopTime());
        assertEquals(1, monkeyLog.getPackages().size());
        assertTrue(monkeyLog.getPackages().contains("com.google.android.apps.maps"));
        assertEquals(1, monkeyLog.getCategories().size());
        assertTrue(monkeyLog.getCategories().contains("android.intent.category.LAUNCHER"));
        assertEquals(100, monkeyLog.getThrottle());
        assertEquals(501, monkeyLog.getSeed().intValue());
        assertEquals(10000, monkeyLog.getTargetCount().intValue());
        assertTrue(monkeyLog.getIgnoreSecurityExceptions());
        assertEquals(49 * 1000, monkeyLog.getTotalDuration().longValue());
        assertEquals(232650, monkeyLog.getStartUptimeDuration().longValue());
        assertEquals(282530, monkeyLog.getStopUptimeDuration().longValue());
        assertFalse(monkeyLog.getIsFinished());
        assertFalse(monkeyLog.getNoActivities());
        assertEquals(1600, monkeyLog.getIntermediateCount());
        assertEquals(1649, monkeyLog.getFinalCount().intValue());
        assertNotNull(monkeyLog.getCrash());
        assertTrue(monkeyLog.getCrash() instanceof JavaCrashItem);
        assertEquals("com.google.android.apps.maps", monkeyLog.getCrash().getApp());
        assertEquals(3161, monkeyLog.getCrash().getPid().intValue());
        assertEquals("java.lang.Exception", ((JavaCrashItem) monkeyLog.getCrash()).getException());
    }

    /**
     * Test that a monkey can be parsed if there is a Java crash even if monkey lines are mixed in
     * the crash.
     */
    public void testParse_java_crash_mixed() {
        List<String> lines = Arrays.asList(
                "# Tuesday, 04/24/2012 05:05:50 PM - device uptime = 232.65: Monkey command used for this test:",
                "adb shell monkey -p com.google.android.apps.maps  -c android.intent.category.SAMPLE_CODE -c android.intent.category.CAR_DOCK -c android.intent.category.LAUNCHER -c android.intent.category.MONKEY -c android.intent.category.INFO  --ignore-security-exceptions --throttle 100  -s 501 -v -v -v 10000 ",
                "",
                ":Monkey: seed=501 count=10000",
                ":AllowPackage: com.google.android.apps.maps",
                ":IncludeCategory: android.intent.category.LAUNCHER",
                ":Switch: #Intent;action=android.intent.action.MAIN;category=android.intent.category.LAUNCHER;launchFlags=0x10200000;component=com.google.android.apps.maps/com.google.android.maps.LatitudeActivity;end",
                "    // Allowing start of Intent { act=android.intent.action.MAIN cat=[android.intent.category.LAUNCHER] cmp=com.google.android.apps.maps/com.google.android.maps.LatitudeActivity } in package com.google.android.apps.maps",
                "Sleeping for 100 milliseconds",
                ":Sending Touch (ACTION_DOWN): 0:(332.0,70.0)",
                ":Sending Touch (ACTION_UP): 0:(332.55292,76.54678)",
                "    //[calendar_time:2012-04-25 00:06:38.419  system_uptime:280799]",
                "    // Sending event #1600",
                ":Sending Touch (ACTION_MOVE): 0:(1052.2666,677.64594)",
                "// CRASH: com.google.android.apps.maps (pid 3161)",
                "// Short Msg: java.lang.Exception",
                ":Sending Touch (ACTION_UP): 0:(1054.7593,687.3757)",
                "// Long Msg: java.lang.Exception: This is the message",
                "// Build Label: google/yakju/maguro:JellyBean/JRN24B/338896:userdebug/dev-keys",
                "// Build Changelist: 338896",
                "Sleeping for 100 milliseconds",
                "// Build Time: 1335309051000",
                "// java.lang.Exception: This is the message",
                "// \tat class.method1(Class.java:1)",
                "// \tat class.method2(Class.java:2)",
                "// \tat class.method3(Class.java:3)",
                "// ",
                "** Monkey aborted due to error.",
                "Events injected: 1649",
                ":Sending rotation degree=0, persist=false",
                ":Dropped: keys=0 pointers=0 trackballs=0 flips=0 rotations=0",
                "## Network stats: elapsed time=48897ms (0ms mobile, 48897ms wifi, 0ms not connected)",
                "** System appears to have crashed at event 1649 of 10000 using seed 501",
                "",
                "# Tuesday, 04/24/2012 05:06:40 PM - device uptime = 282.53: Monkey command ran for: 00:49 (mm:ss)",
                "",
                "----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------",
                "");

        MonkeyLogItem monkeyLog = new MonkeyLogParser().parse(lines);
        assertNotNull(monkeyLog);
        // FIXME: Add test back once time situation has been worked out.
        // assertEquals(parseTime("2012-04-24 17:05:50"), monkeyLog.getStartTime());
        // assertEquals(parseTime("2012-04-24 17:06:40"), monkeyLog.getStopTime());
        assertEquals(1, monkeyLog.getPackages().size());
        assertTrue(monkeyLog.getPackages().contains("com.google.android.apps.maps"));
        assertEquals(1, monkeyLog.getCategories().size());
        assertTrue(monkeyLog.getCategories().contains("android.intent.category.LAUNCHER"));
        assertEquals(100, monkeyLog.getThrottle());
        assertEquals(501, monkeyLog.getSeed().intValue());
        assertEquals(10000, monkeyLog.getTargetCount().intValue());
        assertTrue(monkeyLog.getIgnoreSecurityExceptions());
        assertEquals(49 * 1000, monkeyLog.getTotalDuration().longValue());
        assertEquals(232650, monkeyLog.getStartUptimeDuration().longValue());
        assertEquals(282530, monkeyLog.getStopUptimeDuration().longValue());
        assertFalse(monkeyLog.getIsFinished());
        assertFalse(monkeyLog.getNoActivities());
        assertEquals(1600, monkeyLog.getIntermediateCount());
        assertEquals(1649, monkeyLog.getFinalCount().intValue());
        assertNotNull(monkeyLog.getCrash());
        assertTrue(monkeyLog.getCrash() instanceof JavaCrashItem);
        assertEquals("com.google.android.apps.maps", monkeyLog.getCrash().getApp());
        assertEquals(3161, monkeyLog.getCrash().getPid().intValue());
        assertEquals("java.lang.Exception", ((JavaCrashItem) monkeyLog.getCrash()).getException());
    }


    /**
     * Test that a monkey can be parsed if there is a native crash.
     */
    public void testParse_native_crash() {
        List<String> lines = Arrays.asList(
                "# Tuesday, 04/24/2012 05:05:50 PM - device uptime = 232.65: Monkey command used for this test:",
                "adb shell monkey -p com.google.android.apps.maps  -c android.intent.category.SAMPLE_CODE -c android.intent.category.CAR_DOCK -c android.intent.category.LAUNCHER -c android.intent.category.MONKEY -c android.intent.category.INFO  --ignore-security-exceptions --throttle 100  -s 501 -v -v -v 10000 ",
                "",
                ":Monkey: seed=501 count=10000",
                ":AllowPackage: com.google.android.apps.maps",
                ":IncludeCategory: android.intent.category.LAUNCHER",
                ":Switch: #Intent;action=android.intent.action.MAIN;category=android.intent.category.LAUNCHER;launchFlags=0x10200000;component=com.google.android.apps.maps/com.google.android.maps.LatitudeActivity;end",
                "    // Allowing start of Intent { act=android.intent.action.MAIN cat=[android.intent.category.LAUNCHER] cmp=com.google.android.apps.maps/com.google.android.maps.LatitudeActivity } in package com.google.android.apps.maps",
                "Sleeping for 100 milliseconds",
                ":Sending Touch (ACTION_DOWN): 0:(332.0,70.0)",
                ":Sending Touch (ACTION_UP): 0:(332.55292,76.54678)",
                "    //[calendar_time:2012-04-25 00:06:38.419  system_uptime:280799]",
                "    // Sending event #1600",
                ":Sending Touch (ACTION_MOVE): 0:(1052.2666,677.64594)",
                ":Sending Touch (ACTION_UP): 0:(1054.7593,687.3757)",
                "Sleeping for 100 milliseconds",
                "// CRASH: com.android.chrome (pid 2162)",
                "// Short Msg: Native crash",
                "// Long Msg: Native crash: Segmentation fault",
                "// Build Label: google/mantaray/manta:JellyBeanMR2/JWR02/624470:userdebug/dev-keys",
                "// Build Changelist: 624470",
                "// Build Time: 1364920502000",
                "// *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***",
                "// Build fingerprint: 'google/mantaray/manta:4.1/JRO01/12345:userdebug/dev-keys'",
                "// Revision: '7'",
                "// pid: 2162, tid: 2216, name: .android.chrome  >>> com.android.chrome <<<",
                "// signal 11 (SIGSEGV), code 1 (SEGV_MAPERR), fault addr deadbaad",
                "//     r0 00000027  r1 00001000  r2 00000008  r3 deadbaad",
                "//     r4 00000000  r5 7af65e64  r6 00000000  r7 7af65ea4",
                "//     r8 401291f4  r9 00200000  sl 7784badc  fp 00001401",
                "//     ip 7af65ea4  sp 7af65e60  lr 400fed6b  pc 400fc2d4  cpsr 600f0030",
                "//     d0  3332303033312034  d1  6361707320737332",
                "//     d2  632e6c6f6f705f34  d3  205d29383231280a",
                "//     scr 60000010",
                "// ",
                "// backtrace:",
                "//     #00  pc 0001e2d4  /system/lib/libc.so",
                "//     #01  pc 0001c4bc  /system/lib/libc.so (abort+4)",
                "//     #02  pc 0023a515  /system/lib/libchromeview.so",
                "//     #03  pc 006f8a27  /system/lib/libchromeview.so",
                "// ",
                "// stack:",
                "//     7af65e20  77856cf8  ",
                "//     7af65e24  7af65e64  [stack:2216]",
                "//     7af65e28  00000014  ",
                "//     7af65e2c  76a88e6c  /system/lib/libchromeview.so",
                "** Monkey aborted due to error.",
                "Events injected: 1649",
                ":Sending rotation degree=0, persist=false",
                ":Dropped: keys=0 pointers=0 trackballs=0 flips=0 rotations=0",
                "## Network stats: elapsed time=48897ms (0ms mobile, 48897ms wifi, 0ms not connected)",
                "** System appears to have crashed at event 1649 of 10000 using seed 501",
                "",
                "# Tuesday, 04/24/2012 05:06:40 PM - device uptime = 282.53: Monkey command ran for: 00:49 (mm:ss)",
                "",
                "----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------",
                "");

        MonkeyLogItem monkeyLog = new MonkeyLogParser().parse(lines);
        assertNotNull(monkeyLog);
        // FIXME: Add test back once time situation has been worked out.
        // assertEquals(parseTime("2012-04-24 17:05:50"), monkeyLog.getStartTime());
        // assertEquals(parseTime("2012-04-24 17:06:40"), monkeyLog.getStopTime());
        assertEquals(1, monkeyLog.getPackages().size());
        assertTrue(monkeyLog.getPackages().contains("com.google.android.apps.maps"));
        assertEquals(1, monkeyLog.getCategories().size());
        assertTrue(monkeyLog.getCategories().contains("android.intent.category.LAUNCHER"));
        assertEquals(100, monkeyLog.getThrottle());
        assertEquals(501, monkeyLog.getSeed().intValue());
        assertEquals(10000, monkeyLog.getTargetCount().intValue());
        assertTrue(monkeyLog.getIgnoreSecurityExceptions());
        assertEquals(49 * 1000, monkeyLog.getTotalDuration().longValue());
        assertEquals(232650, monkeyLog.getStartUptimeDuration().longValue());
        assertEquals(282530, monkeyLog.getStopUptimeDuration().longValue());
        assertFalse(monkeyLog.getIsFinished());
        assertFalse(monkeyLog.getNoActivities());
        assertEquals(1600, monkeyLog.getIntermediateCount());
        assertEquals(1649, monkeyLog.getFinalCount().intValue());
        assertNotNull(monkeyLog.getCrash());
        assertTrue(monkeyLog.getCrash() instanceof NativeCrashItem);
        assertEquals("com.android.chrome", monkeyLog.getCrash().getApp());
        assertEquals(2162, monkeyLog.getCrash().getPid().intValue());
        assertEquals("google/mantaray/manta:4.1/JRO01/12345:userdebug/dev-keys",
                ((NativeCrashItem) monkeyLog.getCrash()).getFingerprint());
        // Make sure that the entire stack is included.
        assertEquals(23, ((NativeCrashItem) monkeyLog.getCrash()).getStack().split("\n").length);
    }

    /**
     * Test that a monkey can be parsed if there is a native crash with extra info at the end.
     */
    public void testParse_native_crash_strip_extra() {
        List<String> lines = Arrays.asList(
                "# Tuesday, 04/24/2012 05:05:50 PM - device uptime = 232.65: Monkey command used for this test:",
                "adb shell monkey -p com.google.android.apps.maps  -c android.intent.category.SAMPLE_CODE -c android.intent.category.CAR_DOCK -c android.intent.category.LAUNCHER -c android.intent.category.MONKEY -c android.intent.category.INFO  --ignore-security-exceptions --throttle 100  -s 501 -v -v -v 10000 ",
                "",
                ":Monkey: seed=501 count=10000",
                ":AllowPackage: com.google.android.apps.maps",
                ":IncludeCategory: android.intent.category.LAUNCHER",
                ":Switch: #Intent;action=android.intent.action.MAIN;category=android.intent.category.LAUNCHER;launchFlags=0x10200000;component=com.google.android.apps.maps/com.google.android.maps.LatitudeActivity;end",
                "    // Allowing start of Intent { act=android.intent.action.MAIN cat=[android.intent.category.LAUNCHER] cmp=com.google.android.apps.maps/com.google.android.maps.LatitudeActivity } in package com.google.android.apps.maps",
                "Sleeping for 100 milliseconds",
                ":Sending Touch (ACTION_DOWN): 0:(332.0,70.0)",
                ":Sending Touch (ACTION_UP): 0:(332.55292,76.54678)",
                "    //[calendar_time:2012-04-25 00:06:38.419  system_uptime:280799]",
                "    // Sending event #1600",
                ":Sending Touch (ACTION_MOVE): 0:(1052.2666,677.64594)",
                ":Sending Touch (ACTION_UP): 0:(1054.7593,687.3757)",
                "Sleeping for 100 milliseconds",
                "// CRASH: com.android.chrome (pid 2162)",
                "// Short Msg: Native crash",
                "// Long Msg: Native crash: Segmentation fault",
                "// Build Label: google/mantaray/manta:JellyBeanMR2/JWR02/624470:userdebug/dev-keys",
                "// Build Changelist: 624470",
                "// Build Time: 1364920502000",
                "// *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***",
                "// Build fingerprint: 'google/mantaray/manta:4.1/JRO01/12345:userdebug/dev-keys'",
                "// Revision: '7'",
                "// pid: 2162, tid: 2216, name: .android.chrome  >>> com.android.chrome <<<",
                "// signal 11 (SIGSEGV), code 1 (SEGV_MAPERR), fault addr deadbaad",
                "//     r0 00000027  r1 00001000  r2 00000008  r3 deadbaad",
                "//     r4 00000000  r5 7af65e64  r6 00000000  r7 7af65ea4",
                "//     r8 401291f4  r9 00200000  sl 7784badc  fp 00001401",
                "//     ip 7af65ea4  sp 7af65e60  lr 400fed6b  pc 400fc2d4  cpsr 600f0030",
                "//     d0  3332303033312034  d1  6361707320737332",
                "//     d2  632e6c6f6f705f34  d3  205d29383231280a",
                "//     scr 60000010",
                "// ",
                "// backtrace:",
                "//     #00  pc 0001e2d4  /system/lib/libc.so",
                "//     #01  pc 0001c4bc  /system/lib/libc.so (abort+4)",
                "//     #02  pc 0023a515  /system/lib/libchromeview.so",
                "//     #03  pc 006f8a27  /system/lib/libchromeview.so",
                "// ",
                "// stack:",
                "//     7af65e20  77856cf8  ",
                "//     7af65e24  7af65e64  [stack:2216]",
                "//     7af65e28  00000014  ",
                "//     7af65e2c  76a88e6c  /system/lib/libchromeview.so",
                "// ** New native crash detected.",
                "** Monkey aborted due to error.",
                "Events injected: 1649",
                ":Sending rotation degree=0, persist=false",
                ":Dropped: keys=0 pointers=0 trackballs=0 flips=0 rotations=0",
                "## Network stats: elapsed time=48897ms (0ms mobile, 48897ms wifi, 0ms not connected)",
                "** System appears to have crashed at event 1649 of 10000 using seed 501",
                "",
                "# Tuesday, 04/24/2012 05:06:40 PM - device uptime = 282.53: Monkey command ran for: 00:49 (mm:ss)",
                "",
                "----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------",
                "");

        MonkeyLogItem monkeyLog = new MonkeyLogParser().parse(lines);
        assertNotNull(monkeyLog);
        // FIXME: Add test back once time situation has been worked out.
        // assertEquals(parseTime("2012-04-24 17:05:50"), monkeyLog.getStartTime());
        // assertEquals(parseTime("2012-04-24 17:06:40"), monkeyLog.getStopTime());
        assertEquals(1, monkeyLog.getPackages().size());
        assertTrue(monkeyLog.getPackages().contains("com.google.android.apps.maps"));
        assertEquals(1, monkeyLog.getCategories().size());
        assertTrue(monkeyLog.getCategories().contains("android.intent.category.LAUNCHER"));
        assertEquals(100, monkeyLog.getThrottle());
        assertEquals(501, monkeyLog.getSeed().intValue());
        assertEquals(10000, monkeyLog.getTargetCount().intValue());
        assertTrue(monkeyLog.getIgnoreSecurityExceptions());
        assertEquals(49 * 1000, monkeyLog.getTotalDuration().longValue());
        assertEquals(232650, monkeyLog.getStartUptimeDuration().longValue());
        assertEquals(282530, monkeyLog.getStopUptimeDuration().longValue());
        assertFalse(monkeyLog.getIsFinished());
        assertFalse(monkeyLog.getNoActivities());
        assertEquals(1600, monkeyLog.getIntermediateCount());
        assertEquals(1649, monkeyLog.getFinalCount().intValue());
        assertNotNull(monkeyLog.getCrash());
        assertTrue(monkeyLog.getCrash() instanceof NativeCrashItem);
        assertEquals("com.android.chrome", monkeyLog.getCrash().getApp());
        assertEquals(2162, monkeyLog.getCrash().getPid().intValue());
        NativeCrashItem nc = (NativeCrashItem) monkeyLog.getCrash();
        assertEquals("google/mantaray/manta:4.1/JRO01/12345:userdebug/dev-keys",
                nc.getFingerprint());
        // Make sure that the stack with the last line stripped is included.
        assertEquals(23, nc.getStack().split("\n").length);
        assertFalse(nc.getStack().contains("New native crash detected"));
    }

    /**
     * Test that a monkey can be parsed if there is an empty native crash.
     */
    public void testParse_native_crash_empty() {
        List<String> lines = Arrays.asList(
                "# Tuesday, 04/24/2012 05:05:50 PM - device uptime = 232.65: Monkey command used for this test:",
                "adb shell monkey -p com.google.android.apps.maps  -c android.intent.category.SAMPLE_CODE -c android.intent.category.CAR_DOCK -c android.intent.category.LAUNCHER -c android.intent.category.MONKEY -c android.intent.category.INFO  --ignore-security-exceptions --throttle 100  -s 501 -v -v -v 10000 ",
                "",
                ":Monkey: seed=501 count=10000",
                ":AllowPackage: com.google.android.apps.maps",
                ":IncludeCategory: android.intent.category.LAUNCHER",
                ":Switch: #Intent;action=android.intent.action.MAIN;category=android.intent.category.LAUNCHER;launchFlags=0x10200000;component=com.google.android.apps.maps/com.google.android.maps.LatitudeActivity;end",
                "    // Allowing start of Intent { act=android.intent.action.MAIN cat=[android.intent.category.LAUNCHER] cmp=com.google.android.apps.maps/com.google.android.maps.LatitudeActivity } in package com.google.android.apps.maps",
                "Sleeping for 100 milliseconds",
                ":Sending Touch (ACTION_DOWN): 0:(332.0,70.0)",
                ":Sending Touch (ACTION_UP): 0:(332.55292,76.54678)",
                "    //[calendar_time:2012-04-25 00:06:38.419  system_uptime:280799]",
                "    // Sending event #1600",
                ":Sending Touch (ACTION_MOVE): 0:(1052.2666,677.64594)",
                ":Sending Touch (ACTION_UP): 0:(1054.7593,687.3757)",
                "Sleeping for 100 milliseconds",
                "** New native crash detected.",
                "** Monkey aborted due to error.",
                "Events injected: 1649",
                ":Sending rotation degree=0, persist=false",
                ":Dropped: keys=0 pointers=0 trackballs=0 flips=0 rotations=0",
                "## Network stats: elapsed time=48897ms (0ms mobile, 48897ms wifi, 0ms not connected)",
                "** System appears to have crashed at event 1649 of 10000 using seed 501",
                "",
                "# Tuesday, 04/24/2012 05:06:40 PM - device uptime = 282.53: Monkey command ran for: 00:49 (mm:ss)",
                "",
                "----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------",
                "");

        MonkeyLogItem monkeyLog = new MonkeyLogParser().parse(lines);
        assertNotNull(monkeyLog);
        // FIXME: Add test back once time situation has been worked out.
        // assertEquals(parseTime("2012-04-24 17:05:50"), monkeyLog.getStartTime());
        // assertEquals(parseTime("2012-04-24 17:06:40"), monkeyLog.getStopTime());
        assertEquals(1, monkeyLog.getPackages().size());
        assertTrue(monkeyLog.getPackages().contains("com.google.android.apps.maps"));
        assertEquals(1, monkeyLog.getCategories().size());
        assertTrue(monkeyLog.getCategories().contains("android.intent.category.LAUNCHER"));
        assertEquals(100, monkeyLog.getThrottle());
        assertEquals(501, monkeyLog.getSeed().intValue());
        assertEquals(10000, monkeyLog.getTargetCount().intValue());
        assertTrue(monkeyLog.getIgnoreSecurityExceptions());
        assertEquals(49 * 1000, monkeyLog.getTotalDuration().longValue());
        assertEquals(232650, monkeyLog.getStartUptimeDuration().longValue());
        assertEquals(282530, monkeyLog.getStopUptimeDuration().longValue());
        assertFalse(monkeyLog.getIsFinished());
        assertFalse(monkeyLog.getNoActivities());
        assertEquals(1600, monkeyLog.getIntermediateCount());
        assertEquals(1649, monkeyLog.getFinalCount().intValue());
        assertNotNull(monkeyLog.getCrash());
        assertTrue(monkeyLog.getCrash() instanceof NativeCrashItem);
        assertNull(monkeyLog.getCrash().getApp());
        assertNull(monkeyLog.getCrash().getPid());
        NativeCrashItem nc = (NativeCrashItem) monkeyLog.getCrash();
        assertNull(nc.getFingerprint());
        assertEquals("", nc.getStack());
    }

    /**
     * Test that a monkey can be parsed if there are no activities to run.
     */
    public void testParse_no_activities() {
        List<String> lines = Arrays.asList(
                "# Wednesday, 04/25/2012 01:37:12 AM - device uptime = 242.13: Monkey command used for this test:",
                "adb shell monkey -p com.google.android.browser  -c android.intent.category.SAMPLE_CODE -c android.intent.category.CAR_DOCK -c android.intent.category.LAUNCHER -c android.intent.category.MONKEY -c android.intent.category.INFO  --ignore-security-exceptions --throttle 100  -s 528 -v -v -v 10000 ",
                "",
                ":Monkey: seed=528 count=10000",
                ":AllowPackage: com.google.android.browser",
                ":IncludeCategory: android.intent.category.LAUNCHER",
                "** No activities found to run, monkey aborted.",
                "",
                "# Wednesday, 04/25/2012 01:42:09 AM - device uptime = 539.21: Monkey command ran for: 04:57 (mm:ss)",
                "",
                "----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------");

        MonkeyLogItem monkeyLog = new MonkeyLogParser().parse(lines);
        assertNotNull(monkeyLog);
        // FIXME: Add test back once time situation has been worked out.
        // assertEquals(parseTime("2012-04-25 01:37:12"), monkeyLog.getStartTime());
        // assertEquals(parseTime("2012-04-25 01:42:09"), monkeyLog.getStopTime());
        assertEquals(1, monkeyLog.getPackages().size());
        assertTrue(monkeyLog.getPackages().contains("com.google.android.browser"));
        assertEquals(1, monkeyLog.getCategories().size());
        assertTrue(monkeyLog.getCategories().contains("android.intent.category.LAUNCHER"));
        assertEquals(100, monkeyLog.getThrottle());
        assertEquals(528, monkeyLog.getSeed().intValue());
        assertEquals(10000, monkeyLog.getTargetCount().intValue());
        assertTrue(monkeyLog.getIgnoreSecurityExceptions());
        assertEquals(4 * 60 * 1000 + 57 * 1000, monkeyLog.getTotalDuration().longValue());
        assertEquals(242130, monkeyLog.getStartUptimeDuration().longValue());
        assertEquals(539210, monkeyLog.getStopUptimeDuration().longValue());
        assertFalse(monkeyLog.getIsFinished());
        assertTrue(monkeyLog.getNoActivities());
        assertEquals(0, monkeyLog.getIntermediateCount());
        assertNull(monkeyLog.getFinalCount());
        assertNull(monkeyLog.getDroppedCount(DroppedCategory.KEYS));
        assertNull(monkeyLog.getDroppedCount(DroppedCategory.POINTERS));
        assertNull(monkeyLog.getDroppedCount(DroppedCategory.TRACKBALLS));
        assertNull(monkeyLog.getDroppedCount(DroppedCategory.FLIPS));
        assertNull(monkeyLog.getDroppedCount(DroppedCategory.ROTATIONS));
        assertNull(monkeyLog.getCrash());
    }

    /**
     * Test that a monkey can be parsed if there is an ANR in the middle of the traces.
     */
    public void testParse_malformed_anr() {
        List<String> lines = Arrays.asList(
                "# Tuesday, 04/24/2012 05:23:30 PM - device uptime = 216.48: Monkey command used for this test:",
                "adb shell monkey -p com.google.android.youtube  -c android.intent.category.SAMPLE_CODE -c android.intent.category.CAR_DOCK -c android.intent.category.LAUNCHER -c android.intent.category.MONKEY -c android.intent.category.INFO  --ignore-security-exceptions --throttle 100  -s 993 -v -v -v 10000 ",
                "",
                ":Monkey: seed=993 count=10000",
                ":AllowPackage: com.google.android.youtube",
                ":IncludeCategory: android.intent.category.LAUNCHER",
                ":Switch: #Intent;action=android.intent.action.MAIN;category=android.intent.category.LAUNCHER;launchFlags=0x10200000;component=com.google.android.youtube/.app.honeycomb.Shell%24HomeActivity;end",
                "    // Allowing start of Intent { act=android.intent.action.MAIN cat=[android.intent.category.LAUNCHER] cmp=com.google.android.youtube/.app.honeycomb.Shell$HomeActivity } in package com.google.android.youtube",
                "Sleeping for 100 milliseconds",
                ":Sending Key (ACTION_UP): 21    // KEYCODE_DPAD_LEFT",
                "Sleeping for 100 milliseconds",
                ":Sending Key (ACTION_DOWN): 22    // KEYCODE_DPAD_RIGHT",
                ":Sending Key (ACTION_UP): 22    // KEYCODE_DPAD_RIGHT",
                "    //[calendar_time:2012-04-25 00:27:27.155  system_uptime:454996]",
                "    // Sending event #5300",
                ":Sending Key (ACTION_UP): 19    // KEYCODE_DPAD_UP",
                "Sleeping for 100 milliseconds",
                ":Sending Trackball (ACTION_MOVE): 0:(4.0,3.0)",
                ":Sending Key (ACTION_DOWN): 20    // KEYCODE_DPAD_DOWN",
                ":Sending Key (ACTION_UP): 20    // KEYCODE_DPAD_DOWN",
                "// NOT RESPONDING: com.google.android.youtube (pid 0)",
                "ANR in com.google.android.youtube (com.google.android.youtube/.app.honeycomb.phone.WatchActivity)",
                "PID: 3301",
                "Reason: keyDispatchingTimedOut",
                "Load: 1.0 / 1.05 / 0.6",
                "CPU usage from 4794ms to -1502ms ago with 99% awake:",
                "  18% 3301/com.google.android.youtube: 16% user + 2.3% kernel / faults: 268 minor 9 major",
                "  13% 313/system_server: 9.2% user + 4.4% kernel / faults: 906 minor 3 major",
                "  10% 117/surfaceflinger: 4.9% user + 5.5% kernel / faults: 1 minor",
                "  10% 120/mediaserver: 6.8% user + 3.6% kernel / faults: 1189 minor",
                "34% TOTAL: 19% user + 13% kernel + 0.2% iowait + 1% softirq",
                "",
                "procrank:",
                "// procrank status was 0",
                "anr traces:",
                "",
                "",
                "----- pid 3301 at 2012-04-25 17:17:08 -----",
                "Cmd line: com.google.android.youtube",
                "",
                "DALVIK THREADS:",
                "(mutexes: tll=0 tsl=0 tscl=0 ghl=0)",
                "",
                "\"main\" prio=5 tid=1 SUSPENDED",
                "  | group=\"main\" sCount=1 dsCount=0 obj=0x00000001 self=0x00000001",
                "  | sysTid=2887 nice=0 sched=0/0 cgrp=foreground handle=0000000001",
                "  | schedstat=( 0 0 0 ) utm=5954 stm=1017 core=0",
                "  at class.method1(Class.java:1)",
                "// NOT RESPONDING: com.google.android.youtube (pid 3302)",
                "ANR in com.google.android.youtube (com.google.android.youtube/.app.honeycomb.phone.WatchActivity)",
                "Reason: keyDispatchingTimedOut",
                "Load: 1.0 / 1.05 / 0.6",
                "CPU usage from 4794ms to -1502ms ago with 99% awake:",
                "  18% 3301/com.google.android.youtube: 16% user + 2.3% kernel / faults: 268 minor 9 major",
                "  13% 313/system_server: 9.2% user + 4.4% kernel / faults: 906 minor 3 major",
                "  10% 117/surfaceflinger: 4.9% user + 5.5% kernel / faults: 1 minor",
                "  10% 120/mediaserver: 6.8% user + 3.6% kernel / faults: 1189 minor",
                "34% TOTAL: 19% user + 13% kernel + 0.2% iowait + 1% softirq",
                "",
                "  at class.method2(Class.java:2)",
                "  at class.method2(Class.java:2)",
                "",
                "----- end 3301 -----",
                "// anr traces status was 0",
                "** Monkey aborted due to error.",
                "Events injected: 5322",
                ":Sending rotation degree=0, persist=false",
                ":Dropped: keys=1 pointers=0 trackballs=0 flips=0 rotations=0",
                "## Network stats: elapsed time=252942ms (0ms mobile, 252942ms wifi, 0ms not connected)",
                "** System appears to have crashed at event 5322 of 10000 using seed 993",
                "",
                "# Tuesday, 04/24/2012 05:27:44 PM - device uptime = 471.37: Monkey command ran for: 04:14 (mm:ss)",
                "",
                "----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------",
                "");

        MonkeyLogItem monkeyLog = new MonkeyLogParser().parse(lines);
        assertNotNull(monkeyLog);
        // FIXME: Add test back once time situation has been worked out.
        // assertEquals(parseTime("2012-04-24 17:23:30"), monkeyLog.getStartTime());
        // assertEquals(parseTime("2012-04-24 17:27:44"), monkeyLog.getStopTime());
        assertEquals(1, monkeyLog.getPackages().size());
        assertTrue(monkeyLog.getPackages().contains("com.google.android.youtube"));
        assertEquals(1, monkeyLog.getCategories().size());
        assertTrue(monkeyLog.getCategories().contains("android.intent.category.LAUNCHER"));
        assertEquals(100, monkeyLog.getThrottle());
        assertEquals(993, monkeyLog.getSeed().intValue());
        assertEquals(10000, monkeyLog.getTargetCount().intValue());
        assertTrue(monkeyLog.getIgnoreSecurityExceptions());
        assertEquals(4 * 60 * 1000 + 14 * 1000, monkeyLog.getTotalDuration().longValue());
        assertEquals(216480, monkeyLog.getStartUptimeDuration().longValue());
        assertEquals(471370, monkeyLog.getStopUptimeDuration().longValue());
        assertFalse(monkeyLog.getIsFinished());
        assertFalse(monkeyLog.getNoActivities());
        assertEquals(5300, monkeyLog.getIntermediateCount());
        assertEquals(5322, monkeyLog.getFinalCount().intValue());
        assertNotNull(monkeyLog.getCrash());
        assertTrue(monkeyLog.getCrash() instanceof AnrItem);
        assertEquals("com.google.android.youtube", monkeyLog.getCrash().getApp());
        assertEquals(3301, monkeyLog.getCrash().getPid().intValue());
        assertEquals("keyDispatchingTimedOut", ((AnrItem) monkeyLog.getCrash()).getReason());
    }

    /**
     * Test that the other date format can be parsed.
     */
    public void testAlternateDateFormat() {
        List<String> lines = Arrays.asList(
                "# Tue Apr 24 17:05:50 PST 2012 - device uptime = 232.65: Monkey command used for this test:",
                "adb shell monkey -p com.google.android.apps.maps  -c android.intent.category.SAMPLE_CODE -c android.intent.category.CAR_DOCK -c android.intent.category.LAUNCHER -c android.intent.category.MONKEY -c android.intent.category.INFO  --ignore-security-exceptions --throttle 100  -s 501 -v -v -v 10000 ",
                "",
                "# Tue Apr 24 17:06:40 PST 2012 - device uptime = 282.53: Monkey command ran for: 00:49 (mm:ss)",
                "",
                "----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------",
                "");

        MonkeyLogItem monkeyLog = new MonkeyLogParser().parse(lines);
        assertNotNull(monkeyLog);
        // FIXME: Add test back once time situation has been worked out.
        // assertEquals(parseTime("2012-04-24 17:05:50"), monkeyLog.getStartTime());
        // assertEquals(parseTime("2012-04-24 17:06:40"), monkeyLog.getStopTime());
    }

    @SuppressWarnings("unused")
    private Date parseTime(String timeStr) throws ParseException {
        DateFormat formatter = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
        return formatter.parse(timeStr);
    }
}

