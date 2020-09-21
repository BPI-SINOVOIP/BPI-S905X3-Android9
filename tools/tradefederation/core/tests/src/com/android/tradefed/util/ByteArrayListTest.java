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
 * Unit test suite to verify the behavior of the {@link ByteArrayList}
 */
public class ByteArrayListTest extends TestCase {
    private ByteArrayList mList = null;

    @Override
    public void setUp() throws Exception {
        super.setUp();
        mList = new ByteArrayList();
    }

    public void testAdd() {
        mList.add((byte) 1);
        mList.add((byte) 2);
        assertEquals(2, mList.size());
    }

    public void testAddAll() {
        byte[] byteAry = new byte[] {0, 1, 2, 3, 4, 5};
        mList.addAll(byteAry);
        assertEquals(byteAry.length, mList.size());
        assertEquals(0, mList.get(0));
        assertEquals(5, mList.get(5));
    }

    public void testAddAll_append() {
        byte[] byteAry1 = new byte[] {0, 1, 2};
        byte[] byteAry2 = new byte[] {3, 4, 5};
        mList.add((byte) 0xa);
        mList.addAll(byteAry1);
        mList.add((byte) 0xb);
        mList.addAll(byteAry2);
        assertEquals(byteAry1.length + byteAry2.length + 2, mList.size());
        assertEquals(0xa, mList.get(0));
        assertEquals(0xb, mList.get(1 + byteAry1.length));
        assertEquals(5, mList.get(mList.size()-1));
    }

    public void testAddAll_limits() {
        byte[] byteAry1 = new byte[] {0, 1, 2};
        byte[] byteAry2 = new byte[] {3, 4, 5};
        mList.add((byte) 0xa);
        mList.addAll(byteAry1, 1, 2);
        mList.add((byte) 0xb);
        mList.addAll(byteAry2, 3, 0);
        assertEquals(1 + 2 + 1 + 0, mList.size());
        assertEquals(0xa, mList.get(0));
        assertEquals(0xb, mList.get(1 + 2));
        assertEquals(1, mList.get(1));
    }

    public void testClear() {
        mList.add((byte) 1);
        mList.add((byte) 2);
        assertEquals(2, mList.size());
        mList.clear();
        assertEquals(0, mList.size());
    }

    public void testEnsure() {
        mList.setSize(0);
        assertEquals(0, mList.getMaxSize());
        mList.ensureCapacity(5);
        assertTrue(mList.getMaxSize() >= 5);
    }

    public void testEnsure_afterAdd() {
        mList.setSize(0);
        assertEquals(0, mList.getMaxSize());
        mList.add((byte) 1);
        assertTrue(mList.getMaxSize() >= 1);
    }

    public void testEnsure_oddGrowthFactor() {
        ByteArrayList list = new ByteArrayList(1, 1.1f);
        assertEquals(1, list.getMaxSize());
        list.ensureCapacity(2);
        assertTrue(list.getMaxSize() >= 2);
    }

    /**
     * Verify the fix for a bug in the implementation which would cause the List to try to allocate
     * infinity memory :o)
     */
    public void testEnsure_bugfix() {
        mList.ensureCapacity(931);
        assertTrue(mList.getMaxSize() >= 931);
        assertTrue(mList.getMaxSize() <= 931*3);
    }

    public void testEquals() {
        ByteArrayList list2 = new ByteArrayList();
        mList.add((byte) 1);
        mList.add((byte) 2);
        list2.add((byte) 1);
        list2.add((byte) 2);
        assertTrue(mList.equals(list2));
        assertTrue(list2.equals(mList));
    }

    public void testEquals_wrongLength() {
        ByteArrayList list2 = new ByteArrayList();
        mList.add((byte) 1);
        mList.add((byte) 2);
        list2.add((byte) 1);
        assertFalse(mList.equals(list2));
        assertFalse(list2.equals(mList));
    }

    public void testEquals_mismatch() {
        ByteArrayList list2 = new ByteArrayList();
        mList.add((byte) 1);
        mList.add((byte) 2);
        list2.add((byte) 1);
        list2.add((byte) 17);
        assertFalse(mList.equals(list2));
        assertFalse(list2.equals(mList));
    }

    public void testGetContents() {
        mList.add((byte) 1);
        mList.add((byte) 2);
        byte[] val = mList.getContents();
        assertEquals(2, val.length);
        assertEquals(1, val[0]);
    }

    public void testRetrieve() {
        mList.add((byte) 1);
        mList.add((byte) 2);
        assertEquals(1, mList.get(0));
        assertEquals(2, mList.get(1));
        try {
            mList.get(2);
            fail("IndexOutOfBoundsException not thrown");
        } catch (IndexOutOfBoundsException e) {
            // expected
        }
    }

    public void testSet() {
        mList.add((byte) 1);
        mList.add((byte) 2);
        mList.add((byte) 3);

        assertEquals(2, mList.set(1, (byte) 12));
        assertEquals(12, mList.get(1));

        assertEquals(3, mList.set(2, (byte) 13));
        assertEquals(13, mList.get(2));
    }

    public void testSetSize() {
        mList.setSize(256);
        assertEquals(256, mList.getMaxSize());
    }

    public void testSetSize_truncate() {
        mList.setSize(2);
        mList.add((byte) 1);
        mList.add((byte) 2);
        assertEquals(2, mList.getMaxSize());
        assertEquals(2, mList.size());

        mList.setSize(1);
        assertEquals(1, mList.getMaxSize());
        assertEquals(1, mList.size());
        assertEquals(1, mList.get(0));
    }

    public void testTrimToSize() {
        mList.add((byte) 1);
        mList.trimToSize();
        assertEquals(1, mList.size());
        assertEquals(1, mList.getMaxSize());
    }

    public void testLimits_setSize() {
        try {
            mList.setSize(-1);
            fail("IllegalArgumentException not thrown");
        } catch (IllegalArgumentException e) {
            // expected
            assertTrue(e.getMessage().contains("New size"));
            assertTrue(e.getMessage().contains("non-negative"));
        }
    }

    public void testLimits_ensureCapacity() {
        try {
            mList.ensureCapacity(-1);
            fail("IllegalArgumentException not thrown");
        } catch (IllegalArgumentException e) {
            // expected
            assertTrue(e.getMessage().contains("Minimum capacity"));
            assertTrue(e.getMessage().contains("non-negative"));
        }
    }

    public void testLimits_get() {
        try {
            mList.get(0);
            fail("IndexOutOfBoundsException not thrown");
        } catch (IndexOutOfBoundsException e) {
            // expected
        }

        mList.add((byte) 1);
        try {
            mList.get(-1);
            fail("IndexOutOfBoundsException not thrown");
        } catch (IndexOutOfBoundsException e) {
            // expected
        }
    }

    public void testLimits_set() {
        try {
            mList.set(0, (byte) 1);
            fail("IndexOutOfBoundsException not thrown");
        } catch (IndexOutOfBoundsException e) {
            // expected
        }

        mList.add((byte) 1);
        try {
            mList.get(-1);
            fail("IndexOutOfBoundsException not thrown");
        } catch (IndexOutOfBoundsException e) {
            // expected
        }
    }

    public void testLimits_constructor() {
        try {
            new ByteArrayList(-1);
            fail("IllegalArgumentException not thrown");
        } catch (IllegalArgumentException e) {
            // expected
            assertTrue(e.getMessage().contains("New size"));
            assertTrue(e.getMessage().contains("non-negative"));
        }

        try {
            new ByteArrayList(-1, 2.0f);
            fail("IllegalArgumentException not thrown");
        } catch (IllegalArgumentException e) {
            // expected
            assertTrue(e.getMessage().contains("New size"));
            assertTrue(e.getMessage().contains("non-negative"));
        }

        try {
            new ByteArrayList(1, 1.0f);
            fail("IllegalArgumentException not thrown");
        } catch (IllegalArgumentException e) {
            // expected
            assertTrue(e.getMessage().contains("Growth factor"));
            assertTrue(e.getMessage().contains("1.1"));
        }
    }
}

