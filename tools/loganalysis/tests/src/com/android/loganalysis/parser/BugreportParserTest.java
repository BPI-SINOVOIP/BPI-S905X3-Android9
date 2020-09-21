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

import com.android.loganalysis.item.BugreportItem;
import com.android.loganalysis.item.IItem;
import com.android.loganalysis.item.MiscKernelLogItem;
import com.android.loganalysis.util.ArrayUtil;

import junit.framework.TestCase;

import java.text.DateFormat;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Arrays;
import java.util.Date;
import java.util.List;

/**
 * Unit tests for {@link BugreportParser}
 */
public class BugreportParserTest extends TestCase {

    /**
     * Test that a bugreport can be parsed.
     */
    public void testParse() throws ParseException {
        List<String> lines =
                Arrays.asList(
                        "========================================================",
                        "== dumpstate: 2012-04-25 20:45:10",
                        "========================================================",
                        "------ SECTION ------",
                        "",
                        "------ MEMORY INFO (/proc/meminfo) ------",
                        "MemTotal:         353332 kB",
                        "MemFree:           65420 kB",
                        "Buffers:           20800 kB",
                        "Cached:            86204 kB",
                        "SwapCached:            0 kB",
                        "",
                        "------ CPU INFO (top -n 1 -d 1 -m 30 -t) ------",
                        "",
                        "User 3%, System 3%, IOW 0%, IRQ 0%",
                        "User 33 + Nice 0 + Sys 32 + Idle 929 + IOW 0 + IRQ 0 + SIRQ 0 = 994",
                        "",
                        "------ PROCRANK (procrank) ------",
                        "  PID      Vss      Rss      Pss      Uss  cmdline",
                        "  178   87136K   81684K   52829K   50012K  system_server",
                        " 1313   78128K   77996K   48603K   45812K  com.google.android.apps.maps",
                        " 3247   61652K   61492K   33122K   30972K  com.android.browser",
                        "                          ------   ------  ------",
                        "                          203624K  163604K  TOTAL",
                        "RAM: 731448K total, 415804K free, 9016K buffers, 108548K cached",
                        "[procrank: 1.6s elapsed]",
                        "",
                        "------ KERNEL LOG (dmesg) ------",
                        "<6>[    0.000000] Initializing cgroup subsys cpu",
                        "<3>[    1.000000] benign message",
                        "",
                        "",
                        "------ SYSTEM LOG (logcat -v threadtime -d *:v) ------",
                        "04-25 09:55:47.799  3064  3082 E AndroidRuntime: java.lang.Exception",
                        "04-25 09:55:47.799  3064  3082 E AndroidRuntime: \tat class.method1(Class.java:1)",
                        "04-25 09:55:47.799  3064  3082 E AndroidRuntime: \tat class.method2(Class.java:2)",
                        "04-25 09:55:47.799  3064  3082 E AndroidRuntime: \tat class.method3(Class.java:3)",
                        "04-25 17:17:08.445   312   366 E ActivityManager: ANR (application not responding) in process: com.android.package",
                        "04-25 17:17:08.445   312   366 E ActivityManager: Reason: keyDispatchingTimedOut",
                        "04-25 17:17:08.445   312   366 E ActivityManager: Load: 0.71 / 0.83 / 0.51",
                        "04-25 17:17:08.445   312   366 E ActivityManager: 33% TOTAL: 21% user + 11% kernel + 0.3% iowait",
                        "04-25 18:33:27.273   115   115 I DEBUG   : *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***",
                        "04-25 18:33:27.273   115   115 I DEBUG   : Build fingerprint: 'product:build:target'",
                        "04-25 18:33:27.273   115   115 I DEBUG   : pid: 3112, tid: 3112  >>> com.google.android.browser <<<",
                        "04-25 18:33:27.273   115   115 I DEBUG   : signal 11 (SIGSEGV), code 1 (SEGV_MAPERR), fault addr 00000000",
                        "",
                        "------ SYSTEM PROPERTIES ------",
                        "[dalvik.vm.dexopt-flags]: [m=y]",
                        "[dalvik.vm.heapgrowthlimit]: [48m]",
                        "[dalvik.vm.heapsize]: [256m]",
                        "[gsm.version.ril-impl]: [android moto-ril-multimode 1.0]",
                        "",
                        "------ LAST KMSG (/proc/last_kmsg) ------",
                        "[    0.000000] Initializing cgroup subsys cpu",
                        "[   16.203491] benign message",
                        "",
                        "------ SECTION ------",
                        "",
                        "------ VM TRACES AT LAST ANR (/data/anr/traces.txt: 2012-04-25 17:17:08) ------",
                        "",
                        "",
                        "----- pid 2887 at 2012-04-25 17:17:08 -----",
                        "Cmd line: com.android.package",
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
                        "",
                        "------ SECTION ------",
                        "",
                        "------ DUMPSYS (dumpsys) ------",
                        "DUMP OF SERVICE batterystats:",
                        "Battery History (0% used, 1636 used of 256KB, 15 strings using 794):",
                        "    0 (15) RESET:TIME: 1970-01-10-06-23-28",
                        "          +45s702ms (2) 001 80080000 volt=4187",
                        "          +1m15s525ms (2) 001 80080000 temp=299 volt=4155",
                        "Statistics since last charged:",
                        "  Time on battery: 1h 5m 2s 4ms (9%) realtime, 1h 5m 2s 4ms (9%) uptime",
                        " Time on battery screen off: 1h 4m 5s 8ms (9%) realtime, 1h 4m 5s 8ms (9%) uptime",
                        " All kernel wake locks:",
                        " Kernel Wake lock PowerManagerService.WakeLocks: 5m 10s 6ms (2 times) realtime",
                        " Kernel Wake lock msm_serial_hs_rx: 2m 13s 612ms (258 times) realtime",
                        "",
                        "  All partial wake locks:",
                        "  Wake lock 1001 ProxyController: 1h 4m 47s 565ms (4 times) realtime",
                        "  Wake lock 1013 AudioMix: 1s 979ms (3 times) realtime",
                        "",
                        "  All wakeup reasons:",
                        "  Wakeup reason 2:bcmsdh_sdmmc:2:qcom,smd:2:msmgio: 1m 5s 4ms (2 times) realtime",
                        "  Wakeup reason 2:qcom,smd-rpm:2:fc4c.qcom,spmi: 7m 1s 914ms (7 times) realtime",
                        "",
                        "DUMP OF SERVICE package:",
                        "Package [com.google.android.calculator] (e075c9d):",
                        "  userId=10071",
                        "  secondaryCpuAbi=null",
                        "  versionCode=73000302 minSdk=10000 targetSdk=10000",
                        "  versionName=7.3 (3821978)",
                        "  splits=[base]",
                        "========================================================",
                        "== Running Application Services",
                        "========================================================",
                        "------ APP SERVICES (dumpsys activity service all) ------",
                        "SERVICE com.google.android.gms/"
                                + "com.google.android.location.internal.GoogleLocationManagerService f4c9d pid=14",
                        " Location Request History By Package:",
                        "Interval effective/min/max 1/0/0[s] Duration: 140[minutes] ["
                                + "com.google.android.gms, PRIORITY_NO_POWER, UserLocationProducer] "
                                + "Num requests: 2 Active: true",
                        "Interval effective/min/max 284/285/3600[s] Duration: 140[minutes] "
                                + "[com.google.android.googlequicksearchbox, PRIORITY_BALANCED_POWER_ACCURACY] "
                                + "Num requests: 5 Active: true");

        BugreportItem bugreport = new BugreportParser().parse(lines);
        assertNotNull(bugreport);
        assertEquals(parseTime("2012-04-25 20:45:10.000"), bugreport.getTime());

        assertNotNull(bugreport.getMemInfo());
        assertEquals(5, bugreport.getMemInfo().size());

        assertNotNull(bugreport.getTop());
        assertEquals(994, bugreport.getTop().getTotal());

        assertNotNull(bugreport.getProcrank());
        assertEquals(3, bugreport.getProcrank().getPids().size());

        assertNotNull(bugreport.getKernelLog());
        assertEquals(1.0, bugreport.getKernelLog().getStopTime(), 0.000005);

        assertNotNull(bugreport.getSystemLog());
        assertEquals(parseTime("2012-04-25 09:55:47.799"), bugreport.getSystemLog().getStartTime());
        assertEquals(parseTime("2012-04-25 18:33:27.273"), bugreport.getSystemLog().getStopTime());
        assertEquals(3, bugreport.getSystemLog().getEvents().size());
        assertEquals(1, bugreport.getSystemLog().getAnrs().size());
        assertNotNull(bugreport.getSystemLog().getAnrs().get(0).getTrace());

        assertNotNull(bugreport.getLastKmsg());
        assertEquals(16.203491, bugreport.getLastKmsg().getStopTime(), 0.000005);

        assertNotNull(bugreport.getSystemProps());
        assertEquals(4, bugreport.getSystemProps().size());

        assertNotNull(bugreport.getDumpsys());
        assertNotNull(bugreport.getDumpsys().getBatteryStats());
        assertNotNull(bugreport.getDumpsys().getPackageStats());

        assertNotNull(bugreport.getActivityService());
        assertNotNull(bugreport.getActivityService().getLocationDumps().getLocationClients());
        assertEquals(bugreport.getActivityService().getLocationDumps().getLocationClients().size(),
                2);
    }

    /**
     * Test that the logcat year is set correctly from the bugreport timestamp.
     */
    public void testParse_set_logcat_year() throws ParseException {
        List<String> lines = Arrays.asList(
                "========================================================",
                "== dumpstate: 1999-01-01 02:03:04",
                "========================================================",
                "------ SYSTEM LOG (logcat -v threadtime -d *:v) ------",
                "01-01 01:02:03.000     1     1 I TAG     : message",
                "01-01 01:02:04.000     1     1 I TAG     : message",
                "");

        BugreportItem bugreport = new BugreportParser().parse(lines);
        assertNotNull(bugreport);
        assertEquals(parseTime("1999-01-01 02:03:04.000"), bugreport.getTime());
        assertNotNull(bugreport.getSystemLog());
        assertEquals(parseTime("1999-01-01 01:02:03.000"), bugreport.getSystemLog().getStartTime());
        assertEquals(parseTime("1999-01-01 01:02:04.000"), bugreport.getSystemLog().getStopTime());
    }

    /**
     * Test that the command line is parsed
     */
    public void testParse_command_line() {
        List<String> lines = Arrays.asList("Command line:");
        BugreportItem bugreport = new BugreportParser().parse(lines);
        assertTrue(bugreport.getCommandLine().isEmpty());

        lines = Arrays.asList("Command line: key=value");
        bugreport = new BugreportParser().parse(lines);
        assertEquals(1, bugreport.getCommandLine().size());
        assertEquals("value", bugreport.getCommandLine().get("key"));

        lines = Arrays.asList("Command line: key1=value1 key2=value2");
        bugreport = new BugreportParser().parse(lines);
        assertEquals(2, bugreport.getCommandLine().size());
        assertEquals("value1", bugreport.getCommandLine().get("key1"));
        assertEquals("value2", bugreport.getCommandLine().get("key2"));

        // Test a bad strings
        lines = Arrays.asList("Command line:   key1=value=withequals  key2=  ");
        bugreport = new BugreportParser().parse(lines);
        assertEquals(2, bugreport.getCommandLine().size());
        assertEquals("value=withequals", bugreport.getCommandLine().get("key1"));
        assertEquals("", bugreport.getCommandLine().get("key2"));

        lines = Arrays.asList("Command line: key1=value1 nonkey key2=");
        bugreport = new BugreportParser().parse(lines);
        assertEquals(3, bugreport.getCommandLine().size());
        assertEquals("value1", bugreport.getCommandLine().get("key1"));
        assertEquals("", bugreport.getCommandLine().get("key2"));
        assertNull(bugreport.getCommandLine().get("nonkey"));
    }

    /**
     * Test that a normal boot triggers a normal boot event and no unknown reason.
     */
    public void testParse_bootreason_kernel_good() {
        List<String> lines = Arrays.asList(
                "========================================================",
                "== dumpstate: 1999-01-01 02:03:04",
                "========================================================",
                "Command line: androidboot.bootreason=reboot",
                "");
        BugreportItem bugreport = new BugreportParser().parse(lines);
        assertNotNull(bugreport.getLastKmsg());
        assertEquals(1, bugreport.getLastKmsg().getEvents().size());
        assertEquals("Last boot reason: reboot",
                bugreport.getLastKmsg().getEvents().get(0).getStack());
        assertEquals("NORMAL_REBOOT", bugreport.getLastKmsg().getEvents().get(0).getCategory());
    }

    /**
     * Test that a kernel reset boot triggers a kernel reset event and no unknown reason.
     */
    public void testParse_bootreason_kernel_bad() {
        List<String> lines = Arrays.asList(
                "========================================================",
                "== dumpstate: 1999-01-01 02:03:04",
                "========================================================",
                "Command line: androidboot.bootreason=hw_reset",
                "");
        BugreportItem bugreport = new BugreportParser().parse(lines);
        assertNotNull(bugreport.getLastKmsg());
        assertEquals(1, bugreport.getLastKmsg().getEvents().size());
        assertEquals("Last boot reason: hw_reset",
                bugreport.getLastKmsg().getEvents().get(0).getStack());
        assertEquals("KERNEL_RESET", bugreport.getLastKmsg().getEvents().get(0).getCategory());
    }

    /**
     * Test that a normal boot triggers a normal boot event and no unknown reason.
     */
    public void testParse_bootreason_prop_good() {
        List<String> lines = Arrays.asList(
                "========================================================",
                "== dumpstate: 1999-01-01 02:03:04",
                "========================================================",
                "------ SYSTEM PROPERTIES ------",
                "[ro.boot.bootreason]: [reboot]",
                "");
        BugreportItem bugreport = new BugreportParser().parse(lines);
        assertNotNull(bugreport.getLastKmsg());
        assertEquals(1, bugreport.getLastKmsg().getEvents().size());
        assertEquals("Last boot reason: reboot",
                bugreport.getLastKmsg().getEvents().get(0).getStack());
        assertEquals("NORMAL_REBOOT", bugreport.getLastKmsg().getEvents().get(0).getCategory());
    }

    /**
     * Test that a kernel reset boot triggers a kernel reset event and no unknown reason.
     */
    public void testParse_bootreason_prop_bad() {
        List<String> lines = Arrays.asList(
                "========================================================",
                "== dumpstate: 1999-01-01 02:03:04",
                "========================================================",
                "------ SYSTEM PROPERTIES ------",
                "[ro.boot.bootreason]: [hw_reset]",
                "");
        BugreportItem bugreport = new BugreportParser().parse(lines);
        assertNotNull(bugreport.getLastKmsg());
        assertEquals(1, bugreport.getLastKmsg().getEvents().size());
        assertEquals("Last boot reason: hw_reset",
                bugreport.getLastKmsg().getEvents().get(0).getStack());
        assertEquals("KERNEL_RESET", bugreport.getLastKmsg().getEvents().get(0).getCategory());
        assertEquals("", bugreport.getLastKmsg().getEvents().get(0).getPreamble());
        assertEquals(0.0, bugreport.getLastKmsg().getEvents().get(0).getEventTime());
        assertEquals(0.0, bugreport.getLastKmsg().getStartTime());
        assertEquals(0.0, bugreport.getLastKmsg().getStopTime());
    }

    /**
     * Test that a kernel panic in the last kmsg and a kernel panic only triggers one kernel reset.
     */
    public void testParse_bootreason_duplicate() {
        List<String> lines = Arrays.asList(
                "========================================================",
                "== dumpstate: 1999-01-01 02:03:04",
                "========================================================",
                "Command line: androidboot.bootreason=hw_reset",
                "------ SYSTEM PROPERTIES ------",
                "[ro.boot.bootreason]: [hw_reset]",
                "",
                "------ LAST KMSG (/proc/last_kmsg) ------",
                "[    0.000000] Initializing cgroup subsys cpu",
                "[   16.203491] Kernel panic",
                "");
        BugreportItem bugreport = new BugreportParser().parse(lines);
        assertNotNull(bugreport.getLastKmsg());
        assertEquals(1, bugreport.getLastKmsg().getEvents().size());
        assertEquals("Kernel panic", bugreport.getLastKmsg().getEvents().get(0).getStack());
        assertEquals("KERNEL_RESET", bugreport.getLastKmsg().getEvents().get(0).getCategory());
    }

    /**
     * Test that the trace is set correctly if there is only one ANR.
     */
    public void testSetAnrTrace_single() {
        List<String> lines = Arrays.asList(
                "========================================================",
                "== dumpstate: 2012-04-25 20:45:10",
                "========================================================",
                "------ SYSTEM LOG (logcat -v threadtime -d *:v) ------",
                "04-25 17:17:08.445   312   366 E ActivityManager: ANR (application not responding) in process: com.android.package",
                "04-25 17:17:08.445   312   366 E ActivityManager: Reason: keyDispatchingTimedOut",
                "04-25 17:17:08.445   312   366 E ActivityManager: Load: 0.71 / 0.83 / 0.51",
                "04-25 17:17:08.445   312   366 E ActivityManager: 33% TOTAL: 21% user + 11% kernel + 0.3% iowait",
                "",
                "------ VM TRACES AT LAST ANR (/data/anr/traces.txt: 2012-04-25 17:17:08) ------",
                "",
                "----- pid 2887 at 2012-04-25 17:17:08 -----",
                "Cmd line: com.android.package",
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
                "");

        List<String> expectedStack = Arrays.asList(
                "\"main\" prio=5 tid=1 SUSPENDED",
                "  | group=\"main\" sCount=1 dsCount=0 obj=0x00000001 self=0x00000001",
                "  | sysTid=2887 nice=0 sched=0/0 cgrp=foreground handle=0000000001",
                "  | schedstat=( 0 0 0 ) utm=5954 stm=1017 core=0",
                "  at class.method1(Class.java:1)",
                "  at class.method2(Class.java:2)",
                "  at class.method2(Class.java:2)");

        BugreportItem bugreport = new BugreportParser().parse(lines);

        assertNotNull(bugreport.getSystemLog());
        assertEquals(1, bugreport.getSystemLog().getAnrs().size());
        assertEquals(ArrayUtil.join("\n", expectedStack),
                bugreport.getSystemLog().getAnrs().get(0).getTrace());
    }

    /**
     * Test that the trace is set correctly if there are multiple ANRs.
     */
    public void testSetAnrTrace_multiple() {
        List<String> lines = Arrays.asList(
                "========================================================",
                "== dumpstate: 2012-04-25 20:45:10",
                "========================================================",
                "------ SYSTEM LOG (logcat -v threadtime -d *:v) ------",
                "04-25 17:17:08.445   312   366 E ActivityManager: ANR (application not responding) in process: com.android.package",
                "04-25 17:17:08.445   312   366 E ActivityManager: Reason: keyDispatchingTimedOut",
                "04-25 17:17:08.445   312   366 E ActivityManager: Load: 0.71 / 0.83 / 0.51",
                "04-25 17:17:08.445   312   366 E ActivityManager: 33% TOTAL: 21% user + 11% kernel + 0.3% iowait",
                "04-25 17:18:08.445   312   366 E ActivityManager: ANR (application not responding) in process: com.android.package",
                "04-25 17:18:08.445   312   366 E ActivityManager: Reason: keyDispatchingTimedOut",
                "04-25 17:18:08.445   312   366 E ActivityManager: Load: 0.71 / 0.83 / 0.51",
                "04-25 17:18:08.445   312   366 E ActivityManager: 33% TOTAL: 21% user + 11% kernel + 0.3% iowait",
                "04-25 17:19:08.445   312   366 E ActivityManager: ANR (application not responding) in process: com.android.different.pacakge",
                "04-25 17:19:08.445   312   366 E ActivityManager: Reason: keyDispatchingTimedOut",
                "04-25 17:19:08.445   312   366 E ActivityManager: Load: 0.71 / 0.83 / 0.51",
                "04-25 17:19:08.445   312   366 E ActivityManager: 33% TOTAL: 21% user + 11% kernel + 0.3% iowait",
                "",
                "------ VM TRACES AT LAST ANR (/data/anr/traces.txt: 2012-04-25 17:18:08) ------",
                "",
                "----- pid 2887 at 2012-04-25 17:17:08 -----",
                "Cmd line: com.android.package",
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
                "");

        List<String> expectedStack = Arrays.asList(
                "\"main\" prio=5 tid=1 SUSPENDED",
                "  | group=\"main\" sCount=1 dsCount=0 obj=0x00000001 self=0x00000001",
                "  | sysTid=2887 nice=0 sched=0/0 cgrp=foreground handle=0000000001",
                "  | schedstat=( 0 0 0 ) utm=5954 stm=1017 core=0",
                "  at class.method1(Class.java:1)",
                "  at class.method2(Class.java:2)",
                "  at class.method2(Class.java:2)");

        BugreportItem bugreport = new BugreportParser().parse(lines);

        assertNotNull(bugreport.getSystemLog());
        assertEquals(3, bugreport.getSystemLog().getAnrs().size());
        assertNull(bugreport.getSystemLog().getAnrs().get(0).getTrace());
        assertEquals(ArrayUtil.join("\n", expectedStack),
                bugreport.getSystemLog().getAnrs().get(1).getTrace());
        assertNull(bugreport.getSystemLog().getAnrs().get(2).getTrace());
    }

    /**
     * Test that the trace is set correctly if there is not traces file.
     */
    public void testSetAnrTrace_no_traces() {
        List<String> lines = Arrays.asList(
                "========================================================",
                "== dumpstate: 2012-04-25 20:45:10",
                "========================================================",
                "------ SYSTEM LOG (logcat -v threadtime -d *:v) ------",
                "04-25 17:17:08.445   312   366 E ActivityManager: ANR (application not responding) in process: com.android.package",
                "04-25 17:17:08.445   312   366 E ActivityManager: Reason: keyDispatchingTimedOut",
                "04-25 17:17:08.445   312   366 E ActivityManager: Load: 0.71 / 0.83 / 0.51",
                "04-25 17:17:08.445   312   366 E ActivityManager: 33% TOTAL: 21% user + 11% kernel + 0.3% iowait",
                "",
                "*** NO ANR VM TRACES FILE (/data/anr/traces.txt): No such file or directory",
                "");

        BugreportItem bugreport = new BugreportParser().parse(lines);

        assertNotNull(bugreport.getSystemLog());
        assertEquals(1, bugreport.getSystemLog().getAnrs().size());
        assertNull(bugreport.getSystemLog().getAnrs().get(0).getTrace());
    }

    /**
     * Add a test that ensures that the "new" style of stack dumping works. Traces aren't written to
     * a global trace file. Instead, each ANR event is written to a separate trace file (note the
     * "/data/anr/anr_4376042170248254515" instead of "/data/anr/traces.txt").
     */
    public void testAnrTraces_not_global_traceFile() {
        List<String> lines =
                Arrays.asList(
                        "========================================================",
                        "== dumpstate: 2017-06-12 16:46:29",
                        "========================================================",
                        "------ SYSTEM LOG (logcat -v threadtime -v printable -v uid -d *:v) ------",
                        "--------- beginning of main  ",
                        "02-18 04:26:31.829  logd   468   468 W auditd  : type=2000 audit(0.0:1): initialized",
                        "02-18 04:26:33.783  logd   468   468 I auditd  : type=1403 audit(0.0:2): policy loaded auid=4294967295 ses=4294967295",
                        "02-18 04:26:33.783  logd   468   468 W auditd  : type=1404 audit(0.0:3): enforcing=1 old_enforcing=0 auid=4294967295 ses=4294967295",
                        "06-12 16:45:47.426  1000  1050  1124 E ActivityManager: ANR in com.example.android.helloactivity (com.example.android.helloactivity/.HelloActivity)",
                        "06-12 16:45:47.426  1000  1050  1124 E ActivityManager: PID: 7176",
                        "06-12 16:45:47.426  1000  1050  1124 E ActivityManager: Reason: Input dispatching timed out (Waiting because no window has focus but there is a focused application that may eventually add a window when it finishes starting up.)",
                        "06-12 16:45:47.426  1000  1050  1124 E ActivityManager: Load: 6.85 / 7.07 / 5.31",
                        "06-12 16:45:47.426  1000  1050  1124 E ActivityManager: CPU usage from 235647ms to 0ms ago (2017-06-12 16:41:49.415 to 2017-06-12 16:45:45.062):",
                        "06-12 16:45:47.426  1000  1050  1124 E ActivityManager:   7.7% 1848/com.ustwo.lwp: 4% user + 3.7% kernel / faults: 157 minor",
                        "06-12 16:45:47.426  1000  1050  1124 E ActivityManager:   7.7% 2536/com.google.android.googlequicksearchbox:search: 5.6% user + 2.1% kernel / faults: 195 minor",
                        "06-12 16:45:47.426  1000  1050  1124 E ActivityManager:   7.2% 1050/system_server: 4.5% user + 2.6% kernel / faults: 27117 minor ",
                        "06-12 16:45:47.426  1000  1050  1124 E ActivityManager:   5.3% 489/surfaceflinger: 2.9% user + 2.3% kernel / faults: 15 minor ",
                        "",
                        "------ VM TRACES AT LAST ANR (/data/anr/anr_4376042170248254515: 2017-06-12 16:45:47) ------",
                        "",
                        "----- pid 7176 at 2017-06-12 16:45:45 -----",
                        "Cmd line: com.example.android.helloactivity",
                        "",
                        "DALVIK THREADS:",
                        "(mutexes: tll=0 tsl=0 tscl=0 ghl=0)",
                        "",
                        "\"main\" daemon prio=5 tid=5 Waiting",
                        "  | group=\"system\" sCount=1 dsCount=0 flags=1 obj=0x140805e8 self=0x7caf4bf400",
                        "  | sysTid=7184 nice=4 cgrp=default sched=0/0 handle=0x7c9b4e44f0",
                        "  | state=S schedstat=( 507031 579062 19 ) utm=0 stm=0 core=3 HZ=100",
                        "  | stack=0x7c9b3e2000-0x7c9b3e4000 stackSize=1037KB",
                        "  | held mutexes=",
                        "  at java.lang.Object.wait(Native method)",
                        "  - waiting on <0x0281f7b7> (a java.lang.Class<java.lang.ref.ReferenceQueue>)",
                        "  at java.lang.Daemons$ReferenceQueueDaemon.runInternal(Daemons.java:178)",
                        "  - locked <0x0281f7b7> (a java.lang.Class<java.lang.ref.ReferenceQueue>)",
                        "  at java.lang.Daemons$Daemon.run(Daemons.java:103)",
                        "  at java.lang.Thread.run(Thread.java:764)",
                        "",
                        "----- end 7176 -----");

        // NOTE: The parser only extracts the main thread from the traces.
        List<String> expectedStack =
                Arrays.asList(
                        "\"main\" daemon prio=5 tid=5 Waiting",
                        "  | group=\"system\" sCount=1 dsCount=0 flags=1 obj=0x140805e8 self=0x7caf4bf400",
                        "  | sysTid=7184 nice=4 cgrp=default sched=0/0 handle=0x7c9b4e44f0",
                        "  | state=S schedstat=( 507031 579062 19 ) utm=0 stm=0 core=3 HZ=100",
                        "  | stack=0x7c9b3e2000-0x7c9b3e4000 stackSize=1037KB",
                        "  | held mutexes=",
                        "  at java.lang.Object.wait(Native method)",
                        "  - waiting on <0x0281f7b7> (a java.lang.Class<java.lang.ref.ReferenceQueue>)",
                        "  at java.lang.Daemons$ReferenceQueueDaemon.runInternal(Daemons.java:178)",
                        "  - locked <0x0281f7b7> (a java.lang.Class<java.lang.ref.ReferenceQueue>)",
                        "  at java.lang.Daemons$Daemon.run(Daemons.java:103)",
                        "  at java.lang.Thread.run(Thread.java:764)");

        BugreportItem bugreport = new BugreportParser().parse(lines);
        assertNotNull(bugreport.getSystemLog());
        assertEquals(1, bugreport.getSystemLog().getAnrs().size());
        assertEquals(
                ArrayUtil.join("\n", expectedStack),
                bugreport.getSystemLog().getAnrs().get(0).getTrace());
    }

    /**
     * Test that missing sections in bugreport are set to {@code null}, not empty {@link IItem}s.
     */
    public void testNoSections() {
        List<String> lines = Arrays.asList(
                "========================================================",
                "== dumpstate: 2012-04-25 20:45:10",
                "========================================================");

        BugreportItem bugreport = new BugreportParser().parse(lines);
        assertNotNull(bugreport);
        assertNull(bugreport.getDumpsys());
        assertNull(bugreport.getKernelLog());
        assertNull(bugreport.getMemInfo());
        assertNull(bugreport.getProcrank());
        assertNull(bugreport.getSystemLog());
        assertNull(bugreport.getSystemProps());
        assertNull(bugreport.getTop());
        assertNotNull(bugreport.getLastKmsg());
        List<MiscKernelLogItem> events = bugreport.getLastKmsg().getMiscEvents(
                KernelLogParser.KERNEL_RESET);
        assertEquals(events.size(), 1);
        assertEquals(events.get(0).getStack(), "Unknown reason");

        lines = Arrays.asList(
                "========================================================",
                "== dumpstate: 2012-04-25 20:45:10",
                "========================================================",
                "",
                "------ DUMPSYS (dumpsys) ------",
                "",
                "------ KERNEL LOG (dmesg) ------",
                "",
                "------ LAST KMSG (/proc/last_kmsg) ------",
                "",
                "------ MEMORY INFO (/proc/meminfo) ------",
                "",
                "------ PROCRANK (procrank) ------",
                "",
                "------ SYSTEM LOG (logcat -v threadtime -d *:v) ------",
                "",
                "------ SYSTEM PROPERTIES ------",
                "",
                "------ CPU INFO (top -n 1 -d 1 -m 30 -t) ------",
                "");

        bugreport = new BugreportParser().parse(lines);
        assertNotNull(bugreport);
        assertNotNull(bugreport.getDumpsys());
        assertNull(bugreport.getKernelLog());
        assertNull(bugreport.getMemInfo());
        assertNull(bugreport.getProcrank());
        assertNull(bugreport.getSystemLog());
        assertNull(bugreport.getSystemProps());
        assertNull(bugreport.getTop());
        assertNotNull(bugreport.getLastKmsg());
        events = bugreport.getLastKmsg().getMiscEvents(KernelLogParser.KERNEL_RESET);
        assertEquals(events.size(), 1);
        assertEquals(events.get(0).getStack(), "Unknown reason");
    }

    /**
     * Test that section headers are correctly parsed.
     */
    public void testSectionHeader() {
        List<String> lines =
                Arrays.asList(
                        "========================================================",
                        "== dumpstate: 2012-04-25 20:45:10",
                        "========================================================",
                        "------ DUMPSYS (dumpsys) ------",
                        "DUMP OF SERVICE SurfaceFlinger:",
                        "--DrmDisplayCompositor[0]: num_frames=1456 num_ms=475440 fps=3.06243",
                        "---- DrmDisplayCompositor Layers: num=3",
                        "------ DrmDisplayCompositor Layer: plane=17 crtc=20 crtc[x/y/w/h]=0/0/2560/1800",
                        "------ DrmDisplayCompositor Layer: plane=21 disabled",
                        "------ DrmDisplayCompositor Layer: plane=22 disabled",
                        "DUMP OF SERVICE batterystats:",
                        "Battery History (0% used, 1636 used of 256KB, 15 strings using 794):",
                        "    0 (15) RESET:TIME: 1970-01-10-06-23-28",
                        "          +45s702ms (2) 001 80080000 volt=4187",
                        "          +1m15s525ms (2) 001 80080000 temp=299 volt=4155",
                        "Statistics since last charged:",
                        "  Time on battery: 1h 5m 2s 4ms (9%) realtime, 1h 5m 2s 4ms (9%) uptime",
                        " Time on battery screen off: 1h 4m 5s 8ms (9%) realtime, 1h 4m 5s 8ms (9%) uptime",
                        " All kernel wake locks:",
                        " Kernel Wake lock PowerManagerService.WakeLocks: 5m 10s 6ms (2 times) realtime",
                        " Kernel Wake lock msm_serial_hs_rx: 2m 13s 612ms (258 times) realtime",
                        "",
                        "  All partial wake locks:",
                        "  Wake lock 1001 ProxyController: 1h 4m 47s 565ms (4 times) realtime",
                        "  Wake lock 1013 AudioMix: 1s 979ms (3 times) realtime",
                        "",
                        "  All wakeup reasons:",
                        "  Wakeup reason 2:bcmsdh_sdmmc:2:qcom,smd:2:msmgio: 1m 5s 4ms (2 times) realtime",
                        "  Wakeup reason 2:qcom,smd-rpm:2:fc4c.qcom,spmi: 7m 1s 914ms (7 times) realtime",
                        "DUMP OF SERVICE package:",
                        "Package [com.google.android.calculator] (e075c9d):",
                        "  use rId=10071",
                        "  secondaryCpuAbi=null",
                        "  versionCode=73000302 minSdk=10000 targetSdk=10000",
                        "  versionName=7.3 (3821978)",
                        "  splits=[base]",
                        "DUMP OF SERVICE procstats:",
                        "COMMITTED STATS FROM 2015-09-30-07-44-54:",
                        "  * system / 1000 / v23:",
                        "           TOTAL: 100% (118MB-118MB-118MB/71MB-71MB-71MB over 1)",
                        "      Persistent: 100% (118MB-118MB-118MB/71MB-71MB-71MB over 1)",
                        " * com.android.phone / 1001 / v23:",
                        "           TOTAL: 6.7%",
                        "      Persistent: 6.7%",
                        "");
        BugreportItem bugreport = new BugreportParser().parse(lines);
        assertNotNull(bugreport.getDumpsys());
        assertNotNull(bugreport.getDumpsys().getBatteryStats());
        assertNotNull(bugreport.getDumpsys().getPackageStats());
        assertNotNull(bugreport.getDumpsys().getProcStats());
    }

    /**
     * Test that an empty input returns {@code null}.
     */
    public void testEmptyInput() {
        BugreportItem item = new BugreportParser().parse(Arrays.asList(""));
        assertNull(item);
    }

    /**
     * Test that app names from logcat events are populated by matching the logcat PIDs with the
     * PIDs from the logcat.
     */
    public void testSetAppsFromProcrank() {
        List<String> lines = Arrays.asList(
                "========================================================",
                "== dumpstate: 2012-04-25 20:45:10",
                "========================================================",
                "------ PROCRANK (procrank) ------",
                "  PID      Vss      Rss      Pss      Uss  cmdline",
                " 3064   87136K   81684K   52829K   50012K  com.android.package1",
                " 3066   87136K   81684K   52829K   50012K  com.android.package2",
                "                          ------   ------  ------",
                "                          203624K  163604K  TOTAL",
                "RAM: 731448K total, 415804K free, 9016K buffers, 108548K cached",
                "[procrank: 1.6s elapsed]",
                "------ SYSTEM LOG (logcat -v threadtime -d *:v) ------",
                "04-25 09:55:47.799  3064  3082 E AndroidRuntime: java.lang.Exception",
                "04-25 09:55:47.799  3064  3082 E AndroidRuntime: \tat class.method1(Class.java:1)",
                "04-25 09:55:47.799  3064  3082 E AndroidRuntime: \tat class.method2(Class.java:2)",
                "04-25 09:55:47.799  3064  3082 E AndroidRuntime: \tat class.method3(Class.java:3)",
                "04-25 09:55:47.799  3065  3083 E AndroidRuntime: java.lang.Exception",
                "04-25 09:55:47.799  3065  3083 E AndroidRuntime: \tat class.method1(Class.java:1)",
                "04-25 09:55:47.799  3065  3083 E AndroidRuntime: \tat class.method2(Class.java:2)",
                "04-25 09:55:47.799  3065  3083 E AndroidRuntime: \tat class.method3(Class.java:3)",
                "04-25 09:55:47.799  3065  3084 E AndroidRuntime: FATAL EXCEPTION: main",
                "04-25 09:55:47.799  3066  3084 E AndroidRuntime: Process: com.android.package3, PID: 1234",
                "04-25 09:55:47.799  3066  3084 E AndroidRuntime: java.lang.Exception",
                "04-25 09:55:47.799  3066  3084 E AndroidRuntime: \tat class.method1(Class.java:1)",
                "04-25 09:55:47.799  3066  3084 E AndroidRuntime: \tat class.method2(Class.java:2)",
                "04-25 09:55:47.799  3066  3084 E AndroidRuntime: \tat class.method3(Class.java:3)");

        BugreportItem bugreport = new BugreportParser().parse(lines);
        assertNotNull(bugreport.getSystemLog());
        assertEquals(3, bugreport.getSystemLog().getJavaCrashes().size());
        assertEquals("com.android.package1",
                bugreport.getSystemLog().getJavaCrashes().get(0).getApp());
        assertNull(bugreport.getSystemLog().getJavaCrashes().get(1).getApp());
        assertEquals("com.android.package3",
                bugreport.getSystemLog().getJavaCrashes().get(2).getApp());
    }

    /**
     * Some Android devices refer to SYSTEM LOG as MAIN LOG. Check that parser recognizes this
     * alternate syntax.
     */
    public void testSystemLogAsMainLog() {
        List<String> lines = Arrays.asList(
                "------ MAIN LOG (logcat -b main -b system -v threadtime -d *:v) ------",
                "--------- beginning of /dev/log/system",
                "12-11 19:48:07.945  1484  1508 D BatteryService: update start");
        BugreportItem bugreport = new BugreportParser().parse(lines);
        assertNotNull(bugreport.getSystemLog());
    }

    /**
     * Some Android devices refer to SYSTEM LOG as MAIN AND SYSTEM LOG. Check that parser
     * recognizes this alternate syntax.
     */
    public void testSystemAndMainLog() {
        List<String> lines = Arrays.asList(
                "------ MAIN AND SYSTEM LOG (logcat -b main -b system -v threadtime -d *:v) ------",
                "--------- beginning of /dev/log/system",
                "12-17 15:15:12.877  1994  2019 D UiModeManager: updateConfigurationLocked: ");
        BugreportItem bugreport = new BugreportParser().parse(lines);
        assertNotNull(bugreport.getSystemLog());
    }

    private Date parseTime(String timeStr) throws ParseException {
        DateFormat formatter = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss.SSS");
        return formatter.parse(timeStr);
    }
}
