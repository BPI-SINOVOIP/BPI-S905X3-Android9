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
package com.android.tradefed.util;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertArrayEquals;

import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.junit.Test;

import java.util.ArrayList;
import java.util.List;

/**
 * Unit tests for {@link StringEscapeUtils}
 */
@RunWith(JUnit4.class)
public class StringEscapeUtilsTest {
    /**
     * Simple test that {@link StringEscapeUtils#escapeShell(String)} escapes the dollar sign.
     */
    @Test
    public void testEscapesDollarSigns() {
        String escaped_str = StringEscapeUtils.escapeShell("$money$signs");
        assertEquals("\\$money\\$signs", escaped_str);
    }

    /**
     * Test {@link StringEscapeUtils#paramsToArgs(List)} returns proper result with no quoting or
     * spaces
     */
    @Test
    public void testParams_noQuotesNoSpaces() {
        List<String> expected = new ArrayList<>();
        expected.add("foo");
        expected.add("bar");
        assertArrayEquals(expected.toArray(),
                StringEscapeUtils.paramsToArgs(expected).toArray());
    }

    /**
     * Test {@link StringEscapeUtils#paramsToArgs(List)} returns proper result with no quoting but
     * with spaces
     */
    @Test
    public void testParams_noQuotesWithSpaces() {
        List<String> expected = new ArrayList<>();
        expected.add("foo");
        expected.add("bar bar");
        assertArrayEquals(new String[]{"foo", "bar", "bar"},
                StringEscapeUtils.paramsToArgs(expected).toArray());
    }

    /**
     * Test {@link StringEscapeUtils#paramsToArgs(List)} returns proper result with plain quoting
     */
    @Test
    public void testParams_plainQuotes() {
        List<String> expected = new ArrayList<>();
        expected.add("foo");
        expected.add("\"bar bar\"");
        assertArrayEquals(new String[]{"foo", "bar bar"},
                StringEscapeUtils.paramsToArgs(expected).toArray());
    }

    /**
     * Test {@link StringEscapeUtils#paramsToArgs(List)} returns proper result with escaped quoting
     */
    @Test
    public void testParams_escapedQuotes() {
        List<String> expected = new ArrayList<>();
        expected.add("foo");
        expected.add("\\\"bar bar\\\"");
        assertArrayEquals(new String[]{"foo", "bar bar"},
                StringEscapeUtils.paramsToArgs(expected).toArray());
    }
}
