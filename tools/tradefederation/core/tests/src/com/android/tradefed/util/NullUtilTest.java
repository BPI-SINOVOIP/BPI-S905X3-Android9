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
package com.android.tradefed.util;

import junit.framework.TestCase;

/**
 * Unit tests for {@link NullUtil}
 */
public class NullUtilTest extends TestCase {
    /**
     * Make sure countNulls works as expected
     */
    public void testCountNulls() throws Exception {
        assertEquals(0, NullUtil.countNulls());
        assertEquals(0, NullUtil.countNulls("hi"));

        assertEquals(1, NullUtil.countNulls((Object)null));
        assertEquals(1, NullUtil.countNulls(null, "hi"));
        assertEquals(1, NullUtil.countNulls("hi", null));

        assertEquals(2, NullUtil.countNulls(null, null));
    }

    /**
     * Make sure countNonNulls works as expected
     */
    public void testCountNonNulls() throws Exception {
        assertEquals(0, NullUtil.countNonNulls());
        assertEquals(0, NullUtil.countNonNulls((Object)null));

        assertEquals(1, NullUtil.countNonNulls("hi"));
        assertEquals(1, NullUtil.countNonNulls(null, "hi"));
        assertEquals(1, NullUtil.countNonNulls("hi", null));

        assertEquals(2, NullUtil.countNonNulls("hi", "hi"));
    }

    /**
     * Make sure allNull works as expected
     */
    public void testAllNull() throws Exception {
        assertTrue(NullUtil.allNull());
        assertTrue(NullUtil.allNull((Object)null));
        assertTrue(NullUtil.allNull(null, null));

        assertFalse(NullUtil.allNull(1));
        assertFalse(NullUtil.allNull(1, null, null));
        assertFalse(NullUtil.allNull(null, null, 1));
    }

    /**
     * Ensure that we return true only when exactly one passed value is non-null
     */
    public void testSingleNonNull() throws Exception {
        assertFalse(NullUtil.singleNonNull());

        assertFalse(NullUtil.singleNonNull(null, null, null));
        assertTrue(NullUtil.singleNonNull(1, null, null));
        assertTrue(NullUtil.singleNonNull(null, null, 1));
        assertFalse(NullUtil.singleNonNull(1, null, 1));
        assertFalse(NullUtil.singleNonNull(1, 1, 1));
    }

    /**
     * Ensure that we return true only when it is not the case that a null object is simultaneously
     * present with a non-null object
     */
    public void testHomogeneousSet() throws Exception {
        assertTrue(NullUtil.isHomogeneousSet());

        assertTrue(NullUtil.isHomogeneousSet(1));
        assertTrue(NullUtil.isHomogeneousSet((Object)null));

        assertTrue(NullUtil.isHomogeneousSet(1, 1));
        assertTrue(NullUtil.isHomogeneousSet(null, null));

        assertFalse(NullUtil.isHomogeneousSet(1, null));
        assertFalse(NullUtil.isHomogeneousSet(null, 1));
    }
}

