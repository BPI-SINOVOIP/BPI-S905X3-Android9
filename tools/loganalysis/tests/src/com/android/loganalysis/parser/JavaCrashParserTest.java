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
import com.android.loganalysis.util.ArrayUtil;

import junit.framework.TestCase;

import java.util.Arrays;
import java.util.List;

/**
 * Unit tests for {@link JavaCrashParser}.
 */
public class JavaCrashParserTest extends TestCase {

    /**
     * Test that Java crashes are parsed with no message.
     */
    public void testParse_no_message() {
        List<String> lines = Arrays.asList(
                "java.lang.Exception",
                "\tat class.method1(Class.java:1)",
                "\tat class.method2(Class.java:2)",
                "\tat class.method3(Class.java:3)");

        JavaCrashItem jc = new JavaCrashParser().parse(lines);
        assertNotNull(jc);
        assertEquals("java.lang.Exception", jc.getException());
        assertNull(jc.getMessage());
        assertEquals(ArrayUtil.join("\n", lines), jc.getStack());
    }

    /**
     * Test that Java crashes are parsed with a message.
     */
    public void testParse_message() {
        List<String> lines = Arrays.asList(
                "java.lang.Exception: This is the message",
                "\tat class.method1(Class.java:1)",
                "\tat class.method2(Class.java:2)",
                "\tat class.method3(Class.java:3)");

        JavaCrashItem jc = new JavaCrashParser().parse(lines);
        assertNotNull(jc);
        assertEquals("java.lang.Exception", jc.getException());
        assertEquals("This is the message", jc.getMessage());
        assertEquals(ArrayUtil.join("\n", lines), jc.getStack());
    }

    /**
     * Test that Java crashes are parsed if the message spans multiple lines.
     */
    public void testParse_multiline_message() {
        List<String> lines = Arrays.asList(
                "java.lang.Exception: This message",
                "is many lines",
                "long.",
                "\tat class.method1(Class.java:1)",
                "\tat class.method2(Class.java:2)",
                "\tat class.method3(Class.java:3)");

        JavaCrashItem jc = new JavaCrashParser().parse(lines);
        assertNotNull(jc);
        assertEquals("java.lang.Exception", jc.getException());
        assertEquals("This message\nis many lines\nlong.", jc.getMessage());
        assertEquals(ArrayUtil.join("\n", lines), jc.getStack());
    }

    /**
     * Test that caused by sections of Java crashes are parsed, with no message or single or
     * multiline messages.
     */
    public void testParse_caused_by() {
        List<String> lines = Arrays.asList(
                "java.lang.Exception: This is the message",
                "\tat class.method1(Class.java:1)",
                "\tat class.method2(Class.java:2)",
                "\tat class.method3(Class.java:3)",
                "Caused by: java.lang.Exception",
                "\tat class.method4(Class.java:4)",
                "Caused by: java.lang.Exception: This is the caused by message",
                "\tat class.method5(Class.java:5)",
                "Caused by: java.lang.Exception: This is a multiline",
                "caused by message",
                "\tat class.method6(Class.java:6)");

        JavaCrashItem jc = new JavaCrashParser().parse(lines);
        assertNotNull(jc);
        assertEquals("java.lang.Exception", jc.getException());
        assertEquals("This is the message", jc.getMessage());
        assertEquals(ArrayUtil.join("\n", lines), jc.getStack());
    }

    /**
     * Test that the Java crash is cutoff if an unexpected line is handled.
     */
    public void testParse_cutoff() {
        List<String> lines = Arrays.asList(
                "java.lang.Exception: This is the message",
                "\tat class.method1(Class.java:1)",
                "\tat class.method2(Class.java:2)",
                "\tat class.method3(Class.java:3)",
                "Invalid line",
                "java.lang.Exception: This is the message");

        JavaCrashItem jc = new JavaCrashParser().parse(lines);
        assertNotNull(jc);
        assertEquals("java.lang.Exception", jc.getException());
        assertEquals("This is the message", jc.getMessage());
        assertEquals(ArrayUtil.join("\n", lines.subList(0, lines.size()-2)), jc.getStack());
    }

    /**
     * Tests that only parts between the markers are parsed.
     */
    public void testParse_begin_end_markers() {
        List<String> lines = Arrays.asList(
                "error: this message has begin and end",
                "----- begin exception -----",
                "java.lang.Exception: This message",
                "is many lines",
                "long.",
                "\tat class.method1(Class.java:1)",
                "\tat class.method2(Class.java:2)",
                "\tat class.method3(Class.java:3)",
                "----- end exception -----");

        JavaCrashItem jc = new JavaCrashParser().parse(lines);
        assertNotNull(jc);
        assertEquals("java.lang.Exception", jc.getException());
        assertEquals("This message\nis many lines\nlong.", jc.getMessage());
        assertNotNull(jc.getStack());
        assertFalse(jc.getStack().contains("begin exception"));
        assertFalse(jc.getStack().contains("end exception"));
    }
}
