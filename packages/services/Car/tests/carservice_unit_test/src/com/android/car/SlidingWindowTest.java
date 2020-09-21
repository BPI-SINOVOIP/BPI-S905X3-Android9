/*
 * Copyright (C) 2017 The Android Open Source Project
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
package com.android.car;

import android.test.suitebuilder.annotation.MediumTest;

import java.util.Arrays;
import java.util.Iterator;
import java.util.List;
import junit.framework.TestCase;

@MediumTest
public class SlidingWindowTest extends TestCase {
    static final String TAG = SlidingWindowTest.class.getSimpleName();

    private static <T> boolean sameContents(Iterator<T> t1, Iterator<T> t2) {
        while(t1.hasNext()) {
            if (!t2.hasNext()) {
                return false;
            }
            T e1 = t1.next();
            T e2 = t2.next();
            if (!e1.equals(e2)) {
                return false;
            }
        }
        return !t2.hasNext();
    }

    public void testAdd() {
        final List<Integer> elements = Arrays.asList(3, 4);
        SlidingWindow<Integer> window = new SlidingWindow<>(5);
        elements.forEach(window::add);
        assertTrue(sameContents(elements.iterator(), window.iterator()));
    }

    public void testAddOverflow() {
        final List<Integer> elements = Arrays.asList(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
        final List<Integer> expectedElements = Arrays.asList(6, 7, 8, 9, 10);
        SlidingWindow<Integer> window = new SlidingWindow<>(5);
        window.addAll(elements);
        assertTrue(sameContents(expectedElements.iterator(), window.iterator()));
    }

    public void testStream() {
        final List<Integer> elements = Arrays.asList(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
        final List<Integer> expectedElements = Arrays.asList(6, 7, 8, 9, 10);
        SlidingWindow<Integer> window = new SlidingWindow<>(5);
        window.addAll(elements);
        for (Integer e : expectedElements) {
            assertTrue(window.stream().anyMatch(e::equals));
        }
    }

    public void testCount() {
        final List<Integer> elements = Arrays.asList(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
        SlidingWindow<Integer> window = new SlidingWindow<>(5);
        window.addAll(elements);
        assertEquals(3, window.count((Integer e) -> (e % 2) == 0));
    }
}
