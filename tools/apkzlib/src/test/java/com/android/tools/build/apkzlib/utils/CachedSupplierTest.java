/*
 * Copyright (C) 2016 The Android Open Source Project
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

package com.android.tools.build.apkzlib.utils;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import java.util.function.Supplier;
import org.junit.Test;

public class CachedSupplierTest {

    @Test
    public void testGetsOnlyOnce() {
        TestSupplier ts = new TestSupplier();
        CachedSupplier<String> cs = new CachedSupplier<>(ts);
        assertFalse(cs.isValid());

        ts.value = "foo";
        assertEquals(0, ts.invocationCount);
        assertEquals("foo", cs.get());
        assertEquals(1, ts.invocationCount);
        assertTrue(cs.isValid());

        ts.value = "bar";
        assertEquals("foo", cs.get());
        assertEquals(1, ts.invocationCount);
        assertTrue(cs.isValid());
    }

    @Test
    public void cacheCanBePreset() {
        TestSupplier ts = new TestSupplier();
        ts.value = "foo";
        CachedSupplier<String> cs = new CachedSupplier<>(ts);
        cs.precomputed("bar");
        assertTrue(cs.isValid());

        assertEquals("bar", cs.get());
        assertEquals(0, ts.invocationCount);
    }

    @Test
    public void exceptionThrownBySupplier() {
        CachedSupplier<String> cs = new CachedSupplier<>(() -> {
            throw new RuntimeException("foo");
        });
        assertFalse(cs.isValid());

        try {
            cs.get();
            fail();
        } catch (RuntimeException e) {
            assertEquals("foo", e.getMessage());
        }

        assertFalse(cs.isValid());

        try {
            cs.get();
            fail();
        } catch (RuntimeException e) {
            assertEquals("foo", e.getMessage());
        }
    }

    @Test
    public void reset() {
        TestSupplier ts = new TestSupplier();
        ts.value = "foo";
        CachedSupplier<String> cs = new CachedSupplier<>(ts);
        assertFalse(cs.isValid());

        assertEquals("foo", cs.get());
        assertEquals(1, ts.invocationCount);
        assertTrue(cs.isValid());
        ts.value = "bar";

        cs.reset();
        assertFalse(cs.isValid());
        assertEquals("bar", cs.get());
        assertEquals(2, ts.invocationCount);
    }

    static class TestSupplier implements Supplier<String> {
        int invocationCount = 0;
        String value;

        @Override
        public String get() {
            invocationCount++;
            return value;
        }
    }
}
