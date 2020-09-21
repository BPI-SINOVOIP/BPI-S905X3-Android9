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

import junit.framework.TestCase;

import java.util.Arrays;
import java.util.List;

/**
 * Unit tests for {@link ArrayUtil}
 */
public class ArrayUtilTest extends TestCase {

    /**
     * Simple test for {@link ArrayUtil#buildArray(String[])}
     */
    public void testBuildArray_arrays() {
        String[] newArray = ArrayUtil.buildArray(new String[] {"1", "2"}, new String[] {"3"},
                new String[] {"4"});
        assertEquals(4, newArray.length);
        for (int i = 0; i < 4; i++) {
            assertEquals(Integer.toString(i+1), newArray[i]);
        }
    }

    /**
     * Make sure that Collections aren't double-wrapped
     */
    public void testJoinCollection() {
        List<String> list = Arrays.asList("alpha", "beta", "gamma");
        final String expected = "alpha, beta, gamma";
        String str = ArrayUtil.join(", ", list);
        assertEquals(expected, str);
    }

    /**
     * Make sure that Arrays aren't double-wrapped
     */
    public void testJoinArray() {
        String[] ary = new String[] {"alpha", "beta", "gamma"};
        final String expected = "alpha, beta, gamma";
        String str = ArrayUtil.join(", ", (Object[]) ary);
        assertEquals(expected, str);
    }

    /**
     * Make sure that join on varargs arrays work as expected
     */
    public void testJoinNormal() {
        final String expected = "alpha, beta, gamma";
        String str = ArrayUtil.join(", ", "alpha", "beta", "gamma");
        assertEquals(expected, str);
    }
}
