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

import com.android.loganalysis.item.TracesItem;
import com.android.loganalysis.util.ArrayUtil;

import junit.framework.TestCase;

import java.util.Arrays;
import java.util.List;

/**
 * Unit tests for {@link TracesParser}
 */
public class TracesParserTest extends TestCase {

    /**
     * Test that the parser parses the correct stack.
     */
    public void testTracesParser() {
        List<String> lines = Arrays.asList(
                "",
                "",
                "----- pid 2887 at 2012-05-02 16:43:41 -----",
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
                "\"Task_1\" prio=5 tid=27 WAIT",
                "  | group=\"main\" sCount=1 dsCount=0 obj=0x00000001 self=0x00000001",
                "  | sysTid=4789 nice=10 sched=0/0 cgrp=bg_non_interactive handle=0000000001",
                "  | schedstat=( 0 0 0 ) utm=0 stm=0 core=0",
                "  at class.method1(Class.java:1)",
                "  - waiting on <0x00000001> (a java.lang.Thread) held by tid=27 (Task_1)",
                "  at class.method2(Class.java:2)",
                "  at class.method2(Class.java:2)",
                "",
                "\"Task_2\" prio=5 tid=26 NATIVE",
                "  | group=\"main\" sCount=1 dsCount=0 obj=0x00000001 self=0x00000001",
                "  | sysTid=4343 nice=0 sched=0/0 cgrp=foreground handle=0000000001",
                "  | schedstat=( 0 0 0 ) utm=6 stm=3 core=0",
                "  #00  pc 00001234  /system/lib/lib.so (addr+8)",
                "  #01  pc 00001235  /system/lib/lib.so (addr+16)",
                "  #02  pc 00001236  /system/lib/lib.so (addr+24)",
                "  at class.method1(Class.java:1)",
                "",
                "----- end 2887 -----",
                "",
                "",
                "----- pid 256 at 2012-05-02 16:43:41 -----",
                "Cmd line: system",
                "",
                "DALVIK THREADS:",
                "(mutexes: tll=0 tsl=0 tscl=0 ghl=0)",
                "",
                "\"main\" prio=5 tid=1 NATIVE",
                "  | group=\"main\" sCount=1 dsCount=0 obj=0x00000001 self=0x00000001",
                "  | sysTid=256 nice=0 sched=0/0 cgrp=foreground handle=0000000001",
                "  | schedstat=( 0 0 0 ) utm=175 stm=41 core=0",
                "  #00  pc 00001234  /system/lib/lib.so (addr+8)",
                "  #01  pc 00001235  /system/lib/lib.so (addr+16)",
                "  #02  pc 00001236  /system/lib/lib.so (addr+24)",
                "  at class.method1(Class.java:1)",
                "  at class.method2(Class.java:2)",
                "  at class.method2(Class.java:2)",
                "",
                "----- end 256 -----",
                "");

        List<String> expectedStack = Arrays.asList(
                "\"main\" prio=5 tid=1 SUSPENDED",
                "  | group=\"main\" sCount=1 dsCount=0 obj=0x00000001 self=0x00000001",
                "  | sysTid=2887 nice=0 sched=0/0 cgrp=foreground handle=0000000001",
                "  | schedstat=( 0 0 0 ) utm=5954 stm=1017 core=0",
                "  at class.method1(Class.java:1)",
                "  at class.method2(Class.java:2)",
                "  at class.method2(Class.java:2)");

        TracesItem traces = new TracesParser().parse(lines);
        assertEquals(2887, traces.getPid().intValue());
        assertEquals("com.android.package", traces.getApp());
        assertEquals(ArrayUtil.join("\n", expectedStack), traces.getStack());
    }

    /**
     * Test that both forms of cmd line match for the trace.
     */
    public void testTracesParser_cmdline() {
        List<String> expectedStack = Arrays.asList(
                "\"main\" prio=5 tid=1 SUSPENDED",
                "  | group=\"main\" sCount=1 dsCount=0 obj=0x00000001 self=0x00000001",
                "  | sysTid=2887 nice=0 sched=0/0 cgrp=foreground handle=0000000001",
                "  | schedstat=( 0 0 0 ) utm=5954 stm=1017 core=0",
                "  at class.method1(Class.java:1)",
                "  at class.method2(Class.java:2)",
                "  at class.method2(Class.java:2)");

        List<String> lines = Arrays.asList(
                "",
                "",
                "----- pid 2887 at 2012-05-02 16:43:41 -----",
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
                "");

        TracesItem traces = new TracesParser().parse(lines);
        assertEquals(2887, traces.getPid().intValue());
        assertEquals("com.android.package", traces.getApp());
        assertEquals(ArrayUtil.join("\n", expectedStack), traces.getStack());

        lines = Arrays.asList(
                "",
                "",
                "----- pid 2887 at 2012-05-02 16:43:41 -----",
                "Cmdline: com.android.package                                            Original command line: <unset>",
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
                "");

        traces = new TracesParser().parse(lines);
        assertEquals(2887, traces.getPid().intValue());
        assertEquals("com.android.package", traces.getApp());
        assertEquals(ArrayUtil.join("\n", expectedStack), traces.getStack());
    }
}
