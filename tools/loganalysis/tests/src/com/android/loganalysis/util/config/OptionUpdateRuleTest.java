/*
 * Copyright (C) 2010 The Android Open Source Project
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

package com.android.loganalysis.util.config;

import junit.framework.TestCase;

/**
 * Unit tests for {@link OptionUpdateRule}
 */
public class OptionUpdateRuleTest extends TestCase {
    private static final String OPTION_NAME = "option-name";
    private static final Object CURRENT = "5 current value";
    private static final Object UPDATE = "5 update value";
    private static final Object SMALL_UPDATE = "0 update value";
    private static final Object BIG_UPDATE = "9 update value";

    public void testFirst_simple() throws Exception {
        assertEquals(UPDATE, OptionUpdateRule.FIRST.update(OPTION_NAME, null, UPDATE));
        assertEquals(CURRENT, OptionUpdateRule.FIRST.update(OPTION_NAME, CURRENT, UPDATE));
    }

    public void testLast_simple() throws Exception {
        assertEquals(UPDATE, OptionUpdateRule.LAST.update(OPTION_NAME, null, UPDATE));
        assertEquals(UPDATE, OptionUpdateRule.LAST.update(OPTION_NAME, CURRENT, UPDATE));
    }

    public void testGreatest_simple() throws Exception {
        assertEquals(
                SMALL_UPDATE, OptionUpdateRule.GREATEST.update(OPTION_NAME, null, SMALL_UPDATE));
        assertEquals(CURRENT, OptionUpdateRule.GREATEST.update(OPTION_NAME, CURRENT, SMALL_UPDATE));
        assertEquals(
                BIG_UPDATE, OptionUpdateRule.GREATEST.update(OPTION_NAME, CURRENT, BIG_UPDATE));
    }

    public void testLeast_simple() throws Exception {
        assertEquals(BIG_UPDATE, OptionUpdateRule.LEAST.update(OPTION_NAME, null, BIG_UPDATE));
        assertEquals(
                SMALL_UPDATE, OptionUpdateRule.LEAST.update(OPTION_NAME, CURRENT, SMALL_UPDATE));
        assertEquals(CURRENT, OptionUpdateRule.LEAST.update(OPTION_NAME, CURRENT, BIG_UPDATE));
    }

    public void testImmutable_simple() throws Exception {
        assertEquals(UPDATE, OptionUpdateRule.IMMUTABLE.update(OPTION_NAME, null, UPDATE));
        try {
            OptionUpdateRule.IMMUTABLE.update(OPTION_NAME, CURRENT, UPDATE);
            fail("ConfigurationException not thrown when updating an IMMUTABLE option");
        } catch (ConfigurationException e) {
            // expected
        }
    }

    public void testInvalidComparison() throws Exception {
        try {
            // Strings aren't comparable with integers
            OptionUpdateRule.GREATEST.update(OPTION_NAME, 13, UPDATE);
            fail("ConfigurationException not thrown for invalid comparison.");
        } catch (ConfigurationException e) {
            // Expected.  Moreover, the exception should be actionable, so make sure we mention the
            // specific mismatching types.
            final String msg = e.getMessage();
            assertTrue(msg.contains("Integer"));
            assertTrue(msg.contains("String"));
        }
    }

    public void testNotComparable() throws Exception {
        try {
            OptionUpdateRule.LEAST.update(OPTION_NAME, new Exception("hi"), UPDATE);
        } catch (ConfigurationException e) {
            // expected
        }
    }
}

