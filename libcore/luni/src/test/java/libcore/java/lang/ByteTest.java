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

package libcore.java.lang;

public class ByteTest extends junit.framework.TestCase {
    public void test_compare() throws Exception {
        final byte min = Byte.MIN_VALUE;
        final byte zero = (byte) 0;
        final byte max = Byte.MAX_VALUE;
        assertTrue(Byte.compare(max,  max)  == 0);
        assertTrue(Byte.compare(min,  min)  == 0);
        assertTrue(Byte.compare(zero, zero) == 0);
        assertTrue(Byte.compare(max,  zero) > 0);
        assertTrue(Byte.compare(max,  min)  > 0);
        assertTrue(Byte.compare(zero, max)  < 0);
        assertTrue(Byte.compare(zero, min)  > 0);
        assertTrue(Byte.compare(min,  zero) < 0);
        assertTrue(Byte.compare(min,  max)  < 0);
    }

    public void testStaticHashCode() {
        assertEquals(new Byte((byte) 567).hashCode(), Byte.hashCode((byte) 567));
    }

    public void testBYTES() {
        assertEquals(1, Byte.BYTES);
    }

    public void testToUnsignedInt() {
        for(int i = Byte.MIN_VALUE; i < Byte.MAX_VALUE; i++) {
            final byte b = (byte) i;
            final int ui = Byte.toUnsignedInt(b);
            assertEquals(0, ui >>> Byte.BYTES * 8);
            assertEquals(b, Integer.valueOf(b).byteValue());
        }
    }

    public void testToUnsignedLong() {
        for(int i = Byte.MIN_VALUE; i < Byte.MAX_VALUE; i++) {
            final byte b = (byte) i;
            final long ul = Byte.toUnsignedLong(b);
            assertEquals(0, ul >>> Byte.BYTES * 8);
            assertEquals(b, Long.valueOf(b).byteValue());
        }
    }
}
