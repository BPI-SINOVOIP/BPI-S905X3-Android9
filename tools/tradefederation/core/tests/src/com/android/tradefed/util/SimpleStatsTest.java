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

/**
 * Unit tests for {@link SimpleStats}
 */
public class SimpleStatsTest extends TestCase {
    private SimpleStats mStats = null;

    /**
     * {@inheritDoc}
     */
    @Override
    public void setUp() throws Exception {
        mStats = new SimpleStats();
    }

    /**
     * Make sure that the class behaves as expected when the dataset is empty
     */
    public void testStats_empty() {
        assertTrue(mStats.isEmpty());
        assertEquals(0, mStats.size());
        assertNull(mStats.mean());
        assertNull(mStats.median());
        assertNull(mStats.min());
        assertNull(mStats.max());
        assertNull(mStats.stdev());
    }

    /**
     * Make sure that the class behaves as expected with an even count of uniformly-distributed
     * measurements.
     * <p />
     * The even/odd distinction applies mainly to the median (which uses a different calculation
     * depending on whether the "center" element is unique or not).
     */
    public void testStats_evenCount() {
        // [1, 10]
        for (int i = 1; i <= 10; ++i) {
            mStats.add(i);
        }
        assertEquals(10, mStats.size());
        assertEquals(1, mStats.min(), 0.1);
        assertEquals(10, mStats.max(), 0.1);
        assertEquals(5.5, mStats.mean(), 0.1);
        assertEquals(5.5, mStats.median(), 0.1);
        assertEquals(2.872281, mStats.stdev(), 0.000001);
    }

    /**
     * Make sure that the class behaves as expected with an even count of uniformly-distributed
     * measurements.
     * <p />
     * The even/odd distinction applies mainly to the median (which uses a different calculation
     * depending on whether the "center" element is unique or not).
     */
    public void testStats_oddCount() {
        // [0, 10]
        for (int i = 0; i <= 10; ++i) {
            mStats.add(i);
        }
        assertEquals(11, mStats.size());
        assertEquals(0, mStats.min(), 0.1);
        assertEquals(10, mStats.max(), 0.1);
        assertEquals(5.0, mStats.mean(), 0.1);
        assertEquals(5.0, mStats.median(), 0.1);
        assertEquals(3.162278, mStats.stdev(), 0.000001);
    }

    /**
     * The skewed distribution will cause mean and median to diverge
     */
    public void testStats_skewedMedian() {
        for (int i = 1; i <= 5; ++i) {
            for (int j = 1; j <= i; ++j) {
                mStats.add(i);
            }
        }

        // 1 + 2 + 3 + 4 + 5 = 15 elements
        assertEquals(15, mStats.size());
        assertEquals(1, mStats.min(), 0.1);
        assertEquals(5, mStats.max(), 0.1);
        // sum = 55, count = 15
        assertEquals(55.0 / 15.0, mStats.mean(), 0.1);
        assertEquals(4, mStats.median(), 0.1);
        assertEquals(1.247219, mStats.stdev(), 0.000001);
    }
}

