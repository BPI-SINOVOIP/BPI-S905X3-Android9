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
package com.android.loganalysis.util;

import junit.framework.TestCase;

import java.util.regex.Pattern;

/**
 * Unit tests for {@link LogPatternUtil}.
 */
public class LogPatternUtilTest extends TestCase {

    /**
     * Test basic pattern matching.
     */
    public void testPatternMatching() {
        LogPatternUtil patternUtil = new LogPatternUtil();
        patternUtil.addPattern(Pattern.compile("abc"), "cat1");
        patternUtil.addPattern(Pattern.compile("123"), "cat2");

        assertNull(patternUtil.checkMessage("xyz"));
        assertEquals("cat1", patternUtil.checkMessage("abc"));
        assertEquals("cat2", patternUtil.checkMessage("123"));
    }

    /**
     * Test pattern matching with extras.
     */
    public void testExtrasMatching() {
        LogPatternUtil patternUtil = new LogPatternUtil();
        patternUtil.addPattern(Pattern.compile("abc"), null, "cat1");
        patternUtil.addPattern(Pattern.compile("123"), "E/tag1", "cat2");
        patternUtil.addPattern(Pattern.compile("123"), "E/tag2", "cat3");

        assertNull(patternUtil.checkMessage("xyz"));
        assertEquals("cat1", patternUtil.checkMessage("abc"));
        assertEquals("cat1", patternUtil.checkMessage("abc", "E/tag1"));
        assertEquals("cat1", patternUtil.checkMessage("abc", "E/tag2"));
        assertNull(patternUtil.checkMessage("123"));
        assertEquals("cat2", patternUtil.checkMessage("123", "E/tag1"));
        assertEquals("cat3", patternUtil.checkMessage("123", "E/tag2"));
    }
}
