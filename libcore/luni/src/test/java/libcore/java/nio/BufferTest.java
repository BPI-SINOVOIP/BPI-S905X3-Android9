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

package libcore.java.nio;

import junit.framework.TestCase;
import java.io.File;
import java.io.RandomAccessFile;
import java.lang.ref.PhantomReference;
import java.lang.ref.ReferenceQueue;
import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.nio.Buffer;
import java.nio.BufferOverflowException;
import java.nio.BufferUnderflowException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.CharBuffer;
import java.nio.DoubleBuffer;
import java.nio.FloatBuffer;
import java.nio.IntBuffer;
import java.nio.LongBuffer;
import java.nio.MappedByteBuffer;
import java.nio.NioUtils;
import java.nio.ReadOnlyBufferException;
import java.nio.ShortBuffer;
import java.nio.channels.FileChannel;
import java.util.Arrays;
import libcore.io.Memory;
import libcore.io.SizeOf;
import libcore.java.lang.ref.FinalizationTester;

public class BufferTest extends TestCase {

    static {
        System.loadLibrary("javacoretests");
    }

    private static ByteBuffer allocateMapped(int size) throws Exception {
        File f = File.createTempFile("mapped", "tmp");
        f.deleteOnExit();
        RandomAccessFile raf = new RandomAccessFile(f, "rw");
        raf.setLength(size);
        FileChannel ch = raf.getChannel();
        MappedByteBuffer result = ch.map(FileChannel.MapMode.READ_WRITE, 0, size);
        ch.close();
        return result;
    }

    /**
     * Try to create a {@link MappedByteBuffer} from /dev/zero, to see if
     * we support mapping UNIX character devices.
     */
    public void testDevZeroMap() throws Exception {
        RandomAccessFile raf = new RandomAccessFile("/dev/zero", "r");
        try {
            MappedByteBuffer mbb = raf.getChannel().map(FileChannel.MapMode.READ_ONLY, 0, 65536);

            // Create an array initialized to all "(byte) 1"
            byte[] buf1 = new byte[65536];
            Arrays.fill(buf1, (byte) 1);

            // Read from mapped /dev/zero, and overwrite this array.
            mbb.get(buf1);

            // Verify that everything is zero
            for (int i = 0; i < 65536; i++) {
                assertEquals((byte) 0, buf1[i]);
            }
        } finally {
            raf.close();
        }
    }

    /**
     * Same as {@link libcore.java.nio.BufferTest#testDevZeroMap()}, but try to see
     * if we can write to the UNIX character device.
     */
    public void testDevZeroMapRW() throws Exception {
        RandomAccessFile raf = new RandomAccessFile("/dev/zero", "rw");
        try {
            MappedByteBuffer mbb = raf.getChannel()
                    .map(FileChannel.MapMode.READ_WRITE, 65536, 131072);

            // Create an array initialized to all "(byte) 1"
            byte[] buf1 = new byte[65536];
            Arrays.fill(buf1, (byte) 1);

            // Put all "(byte) 1"s into the /dev/zero MappedByteBuffer.
            mbb.put(buf1);

            mbb.position(0);

            byte[] buf2 = new byte[65536];
            mbb.get(buf2);

            // Verify that everything is one
            for (int i = 0; i < 65536; i++) {
                assertEquals((byte) 1, buf2[i]);
            }
        } finally {
            raf.close();
        }
    }

    public void testByteSwappedBulkGetDirect() throws Exception {
        testByteSwappedBulkGet(ByteBuffer.allocateDirect(10));
    }

    public void testByteSwappedBulkGetHeap() throws Exception {
        testByteSwappedBulkGet(ByteBuffer.allocate(10));
    }

    public void testByteSwappedBulkGetMapped() throws Exception {
        testByteSwappedBulkGet(allocateMapped(10));
    }

    private void testByteSwappedBulkGet(ByteBuffer b) throws Exception {
        for (int i = 0; i < b.limit(); ++i) {
            b.put(i, (byte) i);
        }
        b.position(1);

        char[] chars = new char[6];
        b.order(ByteOrder.BIG_ENDIAN).asCharBuffer().get(chars, 1, 4);
        assertEquals("[\u0000, \u0102, \u0304, \u0506, \u0708, \u0000]", Arrays.toString(chars));
        b.order(ByteOrder.LITTLE_ENDIAN).asCharBuffer().get(chars, 1, 4);
        assertEquals("[\u0000, \u0201, \u0403, \u0605, \u0807, \u0000]", Arrays.toString(chars));

        double[] doubles = new double[3];
        b.order(ByteOrder.BIG_ENDIAN).asDoubleBuffer().get(doubles, 1, 1);
        assertEquals(0, Double.doubleToRawLongBits(doubles[0]));
        assertEquals(0x0102030405060708L, Double.doubleToRawLongBits(doubles[1]));
        assertEquals(0, Double.doubleToRawLongBits(doubles[2]));
        b.order(ByteOrder.LITTLE_ENDIAN).asDoubleBuffer().get(doubles, 1, 1);
        assertEquals(0, Double.doubleToRawLongBits(doubles[0]));
        assertEquals(0x0807060504030201L, Double.doubleToRawLongBits(doubles[1]));
        assertEquals(0, Double.doubleToRawLongBits(doubles[2]));

        float[] floats = new float[4];
        b.order(ByteOrder.BIG_ENDIAN).asFloatBuffer().get(floats, 1, 2);
        assertEquals(0, Float.floatToRawIntBits(floats[0]));
        assertEquals(0x01020304, Float.floatToRawIntBits(floats[1]));
        assertEquals(0x05060708, Float.floatToRawIntBits(floats[2]));
        assertEquals(0, Float.floatToRawIntBits(floats[3]));
        b.order(ByteOrder.LITTLE_ENDIAN).asFloatBuffer().get(floats, 1, 2);
        assertEquals(0, Float.floatToRawIntBits(floats[0]));
        assertEquals(0x04030201, Float.floatToRawIntBits(floats[1]));
        assertEquals(0x08070605, Float.floatToRawIntBits(floats[2]));
        assertEquals(0, Float.floatToRawIntBits(floats[3]));

        int[] ints = new int[4];
        b.order(ByteOrder.BIG_ENDIAN).asIntBuffer().get(ints, 1, 2);
        assertEquals(0, ints[0]);
        assertEquals(0x01020304, ints[1]);
        assertEquals(0x05060708, ints[2]);
        assertEquals(0, ints[3]);
        b.order(ByteOrder.LITTLE_ENDIAN).asIntBuffer().get(ints, 1, 2);
        assertEquals(0, ints[0]);
        assertEquals(0x04030201, ints[1]);
        assertEquals(0x08070605, ints[2]);
        assertEquals(0, ints[3]);

        long[] longs = new long[3];
        b.order(ByteOrder.BIG_ENDIAN).asLongBuffer().get(longs, 1, 1);
        assertEquals(0, longs[0]);
        assertEquals(0x0102030405060708L, longs[1]);
        assertEquals(0, longs[2]);
        b.order(ByteOrder.LITTLE_ENDIAN).asLongBuffer().get(longs, 1, 1);
        assertEquals(0, longs[0]);
        assertEquals(0x0807060504030201L, longs[1]);
        assertEquals(0, longs[2]);

        short[] shorts = new short[6];
        b.order(ByteOrder.BIG_ENDIAN).asShortBuffer().get(shorts, 1, 4);
        assertEquals(0, shorts[0]);
        assertEquals(0x0102, shorts[1]);
        assertEquals(0x0304, shorts[2]);
        assertEquals(0x0506, shorts[3]);
        assertEquals(0x0708, shorts[4]);
        assertEquals(0, shorts[5]);
        b.order(ByteOrder.LITTLE_ENDIAN).asShortBuffer().get(shorts, 1, 4);
        assertEquals(0, shorts[0]);
        assertEquals(0x0201, shorts[1]);
        assertEquals(0x0403, shorts[2]);
        assertEquals(0x0605, shorts[3]);
        assertEquals(0x0807, shorts[4]);
        assertEquals(0, shorts[5]);
    }

    private static String toString(ByteBuffer b) {
        StringBuilder result = new StringBuilder();
        for (int i = 0; i < b.limit(); ++i) {
            result.append(String.format("%02x", (int) b.get(i)));
        }
        return result.toString();
    }

    public void testByteSwappedBulkPutDirect() throws Exception {
        testByteSwappedBulkPut(ByteBuffer.allocateDirect(10));
    }

    public void testByteSwappedBulkPutHeap() throws Exception {
        testByteSwappedBulkPut(ByteBuffer.allocate(10));
    }

    public void testByteSwappedBulkPutMapped() throws Exception {
        testByteSwappedBulkPut(allocateMapped(10));
    }

    private void testByteSwappedBulkPut(ByteBuffer b) throws Exception {
        b.position(1);

        char[] chars = new char[] { '\u2222', '\u0102', '\u0304', '\u0506', '\u0708', '\u2222' };
        b.order(ByteOrder.BIG_ENDIAN).asCharBuffer().put(chars, 1, 4);
        assertEquals("00010203040506070800", toString(b));
        b.order(ByteOrder.LITTLE_ENDIAN).asCharBuffer().put(chars, 1, 4);
        assertEquals("00020104030605080700", toString(b));

        double[] doubles = new double[] { 0, Double.longBitsToDouble(0x0102030405060708L), 0 };
        b.order(ByteOrder.BIG_ENDIAN).asDoubleBuffer().put(doubles, 1, 1);
        assertEquals("00010203040506070800", toString(b));
        b.order(ByteOrder.LITTLE_ENDIAN).asDoubleBuffer().put(doubles, 1, 1);
        assertEquals("00080706050403020100", toString(b));

        float[] floats = new float[] { 0, Float.intBitsToFloat(0x01020304),
                Float.intBitsToFloat(0x05060708), 0 };
        b.order(ByteOrder.BIG_ENDIAN).asFloatBuffer().put(floats, 1, 2);
        assertEquals("00010203040506070800", toString(b));
        b.order(ByteOrder.LITTLE_ENDIAN).asFloatBuffer().put(floats, 1, 2);
        assertEquals("00040302010807060500", toString(b));

        int[] ints = new int[] { 0, 0x01020304, 0x05060708, 0 };
        b.order(ByteOrder.BIG_ENDIAN).asIntBuffer().put(ints, 1, 2);
        assertEquals("00010203040506070800", toString(b));
        b.order(ByteOrder.LITTLE_ENDIAN).asIntBuffer().put(ints, 1, 2);
        assertEquals("00040302010807060500", toString(b));

        long[] longs = new long[] { 0, 0x0102030405060708L, 0 };
        b.order(ByteOrder.BIG_ENDIAN).asLongBuffer().put(longs, 1, 1);
        assertEquals("00010203040506070800", toString(b));
        b.order(ByteOrder.LITTLE_ENDIAN).asLongBuffer().put(longs, 1, 1);
        assertEquals("00080706050403020100", toString(b));

        short[] shorts = new short[] { 0, 0x0102, 0x0304, 0x0506, 0x0708, 0 };
        b.order(ByteOrder.BIG_ENDIAN).asShortBuffer().put(shorts, 1, 4);
        assertEquals("00010203040506070800", toString(b));
        b.order(ByteOrder.LITTLE_ENDIAN).asShortBuffer().put(shorts, 1, 4);
        assertEquals("00020104030605080700", toString(b));
    }

    public void testByteBufferByteOrderDirectRW() throws Exception {
        testByteBufferByteOrder(ByteBuffer.allocateDirect(10), false);
    }

    public void testByteBufferByteOrderHeapRW() throws Exception {
        testByteBufferByteOrder(ByteBuffer.allocate(10), false);
    }

    public void testByteBufferByteOrderMappedRW() throws Exception {
        testByteBufferByteOrder(allocateMapped(10), false);
    }

    public void testByteBufferByteOrderDirectRO() throws Exception {
        testByteBufferByteOrder(ByteBuffer.allocateDirect(10), true);
    }

    public void testByteBufferByteOrderHeapRO() throws Exception {
        testByteBufferByteOrder(ByteBuffer.allocate(10), true);
    }

    public void testByteBufferByteOrderMappedRO() throws Exception {
        testByteBufferByteOrder(allocateMapped(10), true);
    }

    private void testByteBufferByteOrder(ByteBuffer b, boolean readOnly) throws Exception {
        if (readOnly) {
            b = b.asReadOnlyBuffer();
        }
        // allocate/allocateDirect/map always returns a big-endian buffer.
        assertEquals(ByteOrder.BIG_ENDIAN, b.order());

        // wrap always returns a big-endian buffer.
        assertEquals(ByteOrder.BIG_ENDIAN, b.wrap(new byte[10]).order());

        // duplicate always returns a big-endian buffer.
        b.order(ByteOrder.BIG_ENDIAN);
        assertEquals(ByteOrder.BIG_ENDIAN, b.duplicate().order());
        b.order(ByteOrder.LITTLE_ENDIAN);
        assertEquals(ByteOrder.BIG_ENDIAN, b.duplicate().order());

        // slice always returns a big-endian buffer.
        b.order(ByteOrder.BIG_ENDIAN);
        assertEquals(ByteOrder.BIG_ENDIAN, b.slice().order());
        b.order(ByteOrder.LITTLE_ENDIAN);
        assertEquals(ByteOrder.BIG_ENDIAN, b.slice().order());

        // asXBuffer always returns a current-endian buffer.
        b.order(ByteOrder.BIG_ENDIAN);
        assertEquals(ByteOrder.BIG_ENDIAN, b.asCharBuffer().order());
        assertEquals(ByteOrder.BIG_ENDIAN, b.asDoubleBuffer().order());
        assertEquals(ByteOrder.BIG_ENDIAN, b.asFloatBuffer().order());
        assertEquals(ByteOrder.BIG_ENDIAN, b.asIntBuffer().order());
        assertEquals(ByteOrder.BIG_ENDIAN, b.asLongBuffer().order());
        assertEquals(ByteOrder.BIG_ENDIAN, b.asShortBuffer().order());
        assertEquals(ByteOrder.BIG_ENDIAN, b.asReadOnlyBuffer().order());
        b.order(ByteOrder.LITTLE_ENDIAN);
        assertEquals(ByteOrder.LITTLE_ENDIAN, b.asCharBuffer().order());
        assertEquals(ByteOrder.LITTLE_ENDIAN, b.asDoubleBuffer().order());
        assertEquals(ByteOrder.LITTLE_ENDIAN, b.asFloatBuffer().order());
        assertEquals(ByteOrder.LITTLE_ENDIAN, b.asIntBuffer().order());
        assertEquals(ByteOrder.LITTLE_ENDIAN, b.asLongBuffer().order());
        assertEquals(ByteOrder.LITTLE_ENDIAN, b.asShortBuffer().order());
        // ...except for asReadOnlyBuffer, which always returns a big-endian buffer.
        assertEquals(ByteOrder.BIG_ENDIAN, b.asReadOnlyBuffer().order());
    }

    public void testCharBufferByteOrderWrapped() throws Exception {
        assertEquals(ByteOrder.nativeOrder(), CharBuffer.wrap(new char[10]).order());
        assertEquals(ByteOrder.nativeOrder(), CharBuffer.wrap(new char[10]).asReadOnlyBuffer().order());
    }

    private void testCharBufferByteOrder(CharBuffer b, ByteOrder bo) throws Exception {
        assertEquals(bo, b.order());
        assertEquals(bo, b.duplicate().order());
        assertEquals(bo, b.slice().order());
        b = b.asReadOnlyBuffer();
        assertEquals(bo, b.order());
        assertEquals(bo, b.duplicate().order());
        assertEquals(bo, b.slice().order());
    }

    private CharBuffer allocateCharBuffer(ByteOrder order) {
        return ByteBuffer.allocate(10).order(order).asCharBuffer();
    }

    public void testCharBufferByteOrderArray() throws Exception {
        testCharBufferByteOrder(CharBuffer.allocate(10), ByteOrder.nativeOrder());
    }

    public void testCharBufferByteOrderBE() throws Exception {
        testCharBufferByteOrder(allocateCharBuffer(ByteOrder.BIG_ENDIAN), ByteOrder.BIG_ENDIAN);
    }

    public void testCharBufferByteOrderLE() throws Exception {
        testCharBufferByteOrder(allocateCharBuffer(ByteOrder.LITTLE_ENDIAN), ByteOrder.LITTLE_ENDIAN);
    }

    public void testDoubleBufferByteOrderWrapped() throws Exception {
        assertEquals(ByteOrder.nativeOrder(), DoubleBuffer.wrap(new double[10]).order());
        assertEquals(ByteOrder.nativeOrder(), DoubleBuffer.wrap(new double[10]).asReadOnlyBuffer().order());
    }

    private void testDoubleBufferByteOrder(DoubleBuffer b, ByteOrder bo) throws Exception {
        assertEquals(bo, b.order());
        assertEquals(bo, b.duplicate().order());
        assertEquals(bo, b.slice().order());
        b = b.asReadOnlyBuffer();
        assertEquals(bo, b.order());
        assertEquals(bo, b.duplicate().order());
        assertEquals(bo, b.slice().order());
    }

    private DoubleBuffer allocateDoubleBuffer(ByteOrder order) {
        return ByteBuffer.allocate(10*8).order(order).asDoubleBuffer();
    }

    public void testDoubleBufferByteOrderArray() throws Exception {
        testDoubleBufferByteOrder(DoubleBuffer.allocate(10), ByteOrder.nativeOrder());
    }

    public void testDoubleBufferByteOrderBE() throws Exception {
        testDoubleBufferByteOrder(allocateDoubleBuffer(ByteOrder.BIG_ENDIAN), ByteOrder.BIG_ENDIAN);
    }

    public void testDoubleBufferByteOrderLE() throws Exception {
        testDoubleBufferByteOrder(allocateDoubleBuffer(ByteOrder.LITTLE_ENDIAN), ByteOrder.LITTLE_ENDIAN);
    }

    public void testFloatBufferByteOrderWrapped() throws Exception {
        assertEquals(ByteOrder.nativeOrder(), FloatBuffer.wrap(new float[10]).order());
        assertEquals(ByteOrder.nativeOrder(), FloatBuffer.wrap(new float[10]).asReadOnlyBuffer().order());
    }

    private void testFloatBufferByteOrder(FloatBuffer b, ByteOrder bo) throws Exception {
        assertEquals(bo, b.order());
        assertEquals(bo, b.duplicate().order());
        assertEquals(bo, b.slice().order());
        b = b.asReadOnlyBuffer();
        assertEquals(bo, b.order());
        assertEquals(bo, b.duplicate().order());
        assertEquals(bo, b.slice().order());
    }

    private FloatBuffer allocateFloatBuffer(ByteOrder order) {
        return ByteBuffer.allocate(10*8).order(order).asFloatBuffer();
    }

    public void testFloatBufferByteOrderArray() throws Exception {
        testFloatBufferByteOrder(FloatBuffer.allocate(10), ByteOrder.nativeOrder());
    }

    public void testFloatBufferByteOrderBE() throws Exception {
        testFloatBufferByteOrder(allocateFloatBuffer(ByteOrder.BIG_ENDIAN), ByteOrder.BIG_ENDIAN);
    }

    public void testFloatBufferByteOrderLE() throws Exception {
        testFloatBufferByteOrder(allocateFloatBuffer(ByteOrder.LITTLE_ENDIAN), ByteOrder.LITTLE_ENDIAN);
    }

    public void testIntBufferByteOrderWrapped() throws Exception {
        assertEquals(ByteOrder.nativeOrder(), IntBuffer.wrap(new int[10]).order());
        assertEquals(ByteOrder.nativeOrder(), IntBuffer.wrap(new int[10]).asReadOnlyBuffer().order());
    }

    private void testIntBufferByteOrder(IntBuffer b, ByteOrder bo) throws Exception {
        assertEquals(bo, b.order());
        assertEquals(bo, b.duplicate().order());
        assertEquals(bo, b.slice().order());
        b = b.asReadOnlyBuffer();
        assertEquals(bo, b.order());
        assertEquals(bo, b.duplicate().order());
        assertEquals(bo, b.slice().order());
    }

    private IntBuffer allocateIntBuffer(ByteOrder order) {
        return ByteBuffer.allocate(10*8).order(order).asIntBuffer();
    }

    public void testIntBufferByteOrderArray() throws Exception {
        testIntBufferByteOrder(IntBuffer.allocate(10), ByteOrder.nativeOrder());
    }

    public void testIntBufferByteOrderBE() throws Exception {
        testIntBufferByteOrder(allocateIntBuffer(ByteOrder.BIG_ENDIAN), ByteOrder.BIG_ENDIAN);
    }

    public void testIntBufferByteOrderLE() throws Exception {
        testIntBufferByteOrder(allocateIntBuffer(ByteOrder.LITTLE_ENDIAN), ByteOrder.LITTLE_ENDIAN);
    }

    public void testLongBufferByteOrderWrapped() throws Exception {
        assertEquals(ByteOrder.nativeOrder(), LongBuffer.wrap(new long[10]).order());
        assertEquals(ByteOrder.nativeOrder(), LongBuffer.wrap(new long[10]).asReadOnlyBuffer().order());
    }

    private void testLongBufferByteOrder(LongBuffer b, ByteOrder bo) throws Exception {
        assertEquals(bo, b.order());
        assertEquals(bo, b.duplicate().order());
        assertEquals(bo, b.slice().order());
        b = b.asReadOnlyBuffer();
        assertEquals(bo, b.order());
        assertEquals(bo, b.duplicate().order());
        assertEquals(bo, b.slice().order());
    }

    private LongBuffer allocateLongBuffer(ByteOrder order) {
        return ByteBuffer.allocate(10*8).order(order).asLongBuffer();
    }

    public void testLongBufferByteOrderArray() throws Exception {
        testLongBufferByteOrder(LongBuffer.allocate(10), ByteOrder.nativeOrder());
    }

    public void testLongBufferByteOrderBE() throws Exception {
        testLongBufferByteOrder(allocateLongBuffer(ByteOrder.BIG_ENDIAN), ByteOrder.BIG_ENDIAN);
    }

    public void testLongBufferByteOrderLE() throws Exception {
        testLongBufferByteOrder(allocateLongBuffer(ByteOrder.LITTLE_ENDIAN), ByteOrder.LITTLE_ENDIAN);
    }

    public void testShortBufferByteOrderWrapped() throws Exception {
        assertEquals(ByteOrder.nativeOrder(), ShortBuffer.wrap(new short[10]).order());
        assertEquals(ByteOrder.nativeOrder(), ShortBuffer.wrap(new short[10]).asReadOnlyBuffer().order());
    }

    private void testShortBufferByteOrder(ShortBuffer b, ByteOrder bo) throws Exception {
        assertEquals(bo, b.order());
        assertEquals(bo, b.duplicate().order());
        assertEquals(bo, b.slice().order());
        b = b.asReadOnlyBuffer();
        assertEquals(bo, b.order());
        assertEquals(bo, b.duplicate().order());
        assertEquals(bo, b.slice().order());
    }

    private ShortBuffer allocateShortBuffer(ByteOrder order) {
        return ByteBuffer.allocate(10*8).order(order).asShortBuffer();
    }

    public void testShortBufferByteOrderArray() throws Exception {
        testShortBufferByteOrder(ShortBuffer.allocate(10), ByteOrder.nativeOrder());
    }

    public void testShortBufferByteOrderBE() throws Exception {
        testShortBufferByteOrder(allocateShortBuffer(ByteOrder.BIG_ENDIAN), ByteOrder.BIG_ENDIAN);
    }

    public void testShortBufferByteOrderLE() throws Exception {
        testShortBufferByteOrder(allocateShortBuffer(ByteOrder.LITTLE_ENDIAN), ByteOrder.LITTLE_ENDIAN);
    }

    public void testRelativePositionsHeap() throws Exception {
        testRelativePositions(ByteBuffer.allocate(10));
    }

    public void testRelativePositionsDirect() throws Exception {
        testRelativePositions(ByteBuffer.allocateDirect(10));
    }

    public void testRelativePositionsMapped() throws Exception {
        testRelativePositions(allocateMapped(10));
    }

    // http://b/3291927 - ensure that the relative get and put methods advance 'position'.
    private void testRelativePositions(ByteBuffer b) throws Exception {
        // gets
        b.position(0);
        b.get();
        assertEquals(1, b.position());

        byte[] buf = new byte[5];
        b.position(0);
        b.get(buf);
        assertEquals(5, b.position());

        b.position(0);
        b.get(buf, 1, 3);
        assertEquals(3, b.position());

        b.position(0);
        b.getChar();
        assertEquals(2, b.position());

        b.position(0);
        b.getDouble();
        assertEquals(8, b.position());

        b.position(0);
        b.getFloat();
        assertEquals(4, b.position());

        b.position(0);
        b.getInt();
        assertEquals(4, b.position());

        b.position(0);
        b.getLong();
        assertEquals(8, b.position());

        b.position(0);
        b.getShort();
        assertEquals(2, b.position());

        // puts
        b.position(0);
        b.put((byte) 0);
        assertEquals(1, b.position());

        b.position(0);
        b.put(buf);
        assertEquals(5, b.position());

        b.position(0);
        b.put(buf, 1, 3);
        assertEquals(3, b.position());

        b.position(0);
        b.putChar('x');
        assertEquals(2, b.position());

        b.position(0);
        b.putDouble(0);
        assertEquals(8, b.position());

        b.position(0);
        b.putFloat(0);
        assertEquals(4, b.position());

        b.position(0);
        b.putInt(0);
        assertEquals(4, b.position());

        b.position(0);
        b.putLong(0);
        assertEquals(8, b.position());

        b.position(0);
        b.putShort((short) 0);
        assertEquals(2, b.position());
    }

    // This test will fail on the RI. Our direct buffers are cooler than theirs.
    // http://b/3384431
    public void testDirectByteBufferHasArray() throws Exception {
        ByteBuffer b = ByteBuffer.allocateDirect(10);
        assertTrue(b.isDirect());
        // Check the buffer has an array of the right size.
        assertTrue(b.hasArray());
        byte[] array = b.array();
        assertTrue(array.length >= b.capacity());
        assertEquals(10, b.capacity());
        // Check that writes to the array show up in the buffer.
        assertEquals(0, b.get(0));
        array[b.arrayOffset()] = 1;
        assertEquals(1, b.get(0));
        // Check that writes to the buffer show up in the array.
        assertEquals(1, array[b.arrayOffset()]);
        b.put(0, (byte) 0);
        assertEquals(0, array[b.arrayOffset()]);
    }

    // Test that direct byte buffers are 8 byte aligned.
    // http://b/16449607
    public void testDirectByteBufferAlignment() throws Exception {
        ByteBuffer b = ByteBuffer.allocateDirect(10);
        Field addressField = Buffer.class.getDeclaredField("address");
        assertTrue(addressField != null);
        addressField.setAccessible(true);
        long address = addressField.getLong(b);
        // Check that the address field is aligned by 8.
        // Normally reading this field happens in native code by calling
        // GetDirectBufferAddress.
        assertEquals(0, address % 8);
    }

    public static native long jniGetDirectBufferAddress(Buffer buf);
    public static native long jniGetDirectBufferCapacity(Buffer buf);

    // Test that JNI GetDirectBufferAddress and GetDirectBufferCapacity work as expected for
    // direct ByteBuffers.
    // http://b/26233076
    public void testDirectByteBufferJniGetDirectBufferAddressAndCapacity() throws Exception {
        // Create a direct ByteBuffer with the following contents.
        byte[] contents = "Testing, 1, 2, 3...".getBytes("US-ASCII");
        ByteBuffer original = ByteBuffer.allocateDirect(contents.length);
        original.put(contents);

        // Check the content of the original buffer by reading the memory it references.
        long originalAddress = jniGetDirectBufferAddress(original);
        if (originalAddress == 0) {
            fail("Obtaining address of direct ByteBuffer not supported");
        }
        long originalCapacity = jniGetDirectBufferCapacity(original);
        assertEquals(contents.length, originalCapacity);
        byte[] originalData = new byte[original.capacity()];
        Memory.peekByteArray(originalAddress, originalData, 0, originalData.length);
        assertTrue(new String(originalData, "US-ASCII"), Arrays.equals(contents, originalData));

        // Slice the buffer and check the content of the result.
        // The slice starts at offset 3 of the original and is 5 bytes shorter than the original.
        original.position(3);
        original.limit(original.capacity() - 2);
        ByteBuffer slice = original.slice();
        System.out.println("original: " + original + ", slice: " + slice);

        long sliceAddress = jniGetDirectBufferAddress(slice);
        System.out.println("originalAddress: 0x" + Long.toHexString(originalAddress)
            + ", sliceAddress: 0x" + Long.toHexString(sliceAddress));
        assertEquals(3 + originalAddress, sliceAddress);
        long sliceCapacity = jniGetDirectBufferCapacity(slice);
        assertEquals(originalCapacity - 5, sliceCapacity);
        byte[] actualSliceData = new byte[slice.capacity()];
        Memory.peekByteArray(sliceAddress, actualSliceData, 0, actualSliceData.length);
        byte[] expectedSliceData = new byte[slice.capacity()];
        System.arraycopy(originalData, 3, expectedSliceData, 0, expectedSliceData.length);
        assertTrue(
                new String(actualSliceData, "US-ASCII"),
                Arrays.equals(expectedSliceData, actualSliceData));
    }

    public void testSliceOffset() throws Exception {
        // Slicing changes the array offset.
        ByteBuffer buffer = ByteBuffer.allocate(10);
        buffer.get();
        ByteBuffer slice = buffer.slice();
        assertEquals(buffer.arrayOffset() + 1, slice.arrayOffset());

        ByteBuffer directBuffer = ByteBuffer.allocateDirect(10);
        directBuffer.get();
        ByteBuffer directSlice = directBuffer.slice();
        assertEquals(directBuffer.arrayOffset() + 1, directSlice.arrayOffset());
    }

    // http://code.google.com/p/android/issues/detail?id=16184
    public void testPutByteBuffer() throws Exception {
        ByteBuffer dst = ByteBuffer.allocate(10).asReadOnlyBuffer();

        // Can't put into a read-only buffer.
        try {
            dst.put(ByteBuffer.allocate(5));
            fail();
        } catch (ReadOnlyBufferException expected) {
        }

        // Can't put a buffer into itself.
        dst = ByteBuffer.allocate(10);
        try {
            dst.put(dst);
            fail();
        } catch (IllegalArgumentException expected) {
        }

        // Can't put the null ByteBuffer.
        try {
            dst.put((ByteBuffer) null);
            fail();
        } catch (NullPointerException expected) {
        }

        // Can't put a larger source into a smaller destination.
        try {
            dst.put(ByteBuffer.allocate(dst.capacity() + 1));
            fail();
        } catch (BufferOverflowException expected) {
        }

        assertPutByteBuffer(ByteBuffer.allocate(10), ByteBuffer.allocate(8), false);
        assertPutByteBuffer(ByteBuffer.allocate(10), ByteBuffer.allocateDirect(8), false);
        assertPutByteBuffer(ByteBuffer.allocate(10), allocateMapped(8), false);
        assertPutByteBuffer(ByteBuffer.allocate(10), ByteBuffer.allocate(8), true);
        assertPutByteBuffer(ByteBuffer.allocate(10), ByteBuffer.allocateDirect(8), true);
        assertPutByteBuffer(ByteBuffer.allocate(10), allocateMapped(8), true);

        assertPutByteBuffer(ByteBuffer.allocateDirect(10), ByteBuffer.allocate(8), false);
        assertPutByteBuffer(ByteBuffer.allocateDirect(10), ByteBuffer.allocateDirect(8), false);
        assertPutByteBuffer(ByteBuffer.allocateDirect(10), allocateMapped(8), false);
        assertPutByteBuffer(ByteBuffer.allocateDirect(10), ByteBuffer.allocate(8), true);
        assertPutByteBuffer(ByteBuffer.allocateDirect(10), ByteBuffer.allocateDirect(8), true);
        assertPutByteBuffer(ByteBuffer.allocateDirect(10), allocateMapped(8), true);
    }

    private void assertPutByteBuffer(ByteBuffer dst, ByteBuffer src, boolean readOnly) {
        // Start 'dst' off as the index-identity pattern.
        for (int i = 0; i < dst.capacity(); ++i) {
            dst.put(i, (byte) i);
        }
        // Deliberately offset the position we'll write to by 1.
        dst.position(1);

        // Make the source more interesting.
        for (int i = 0; i < src.capacity(); ++i) {
            src.put(i, (byte) (16 + i));
        }
        if (readOnly) {
            src = src.asReadOnlyBuffer();
        }

        ByteBuffer dst2 = dst.put(src);
        assertSame(dst, dst2);
        assertEquals(0, src.remaining());
        assertEquals(src.position(), src.capacity());
        assertEquals(dst.position(), src.capacity() + 1);
        for (int i = 0; i < src.capacity(); ++i) {
            assertEquals(src.get(i), dst.get(i + 1));
        }

        // No room for another.
        src.position(0);
        try {
            dst.put(src);
            fail();
        } catch (BufferOverflowException expected) {
        }
    }

    public void testCharBufferSubSequence() throws Exception {
        ByteBuffer b = ByteBuffer.allocateDirect(10).order(ByteOrder.nativeOrder());
        b.putChar('H');
        b.putChar('e');
        b.putChar('l');
        b.putChar('l');
        b.putChar('o');
        b.flip();

        assertEquals("Hello", b.asCharBuffer().toString());

        CharBuffer cb = b.asCharBuffer();
        CharSequence cs = cb.subSequence(0, cb.length());
        assertEquals("Hello", cs.toString());
    }

    public void testHasArrayOnJniDirectByteBuffer() throws Exception {
        // Simulate a call to JNI's NewDirectByteBuffer.
        Class<?> c = Class.forName("java.nio.DirectByteBuffer");
        Constructor<?> ctor = c.getDeclaredConstructor(long.class, int.class);
        ctor.setAccessible(true);
        ByteBuffer bb = (ByteBuffer) ctor.newInstance(0, 0);

        try {
            bb.array();
            fail();
        } catch (UnsupportedOperationException expected) {
        }
        try {
            bb.arrayOffset();
            fail();
        } catch (UnsupportedOperationException expected) {
        }
        assertFalse(bb.hasArray());
    }

    public void testBug6085292() {
        ByteBuffer b = ByteBuffer.allocateDirect(1);

        try {
            b.asCharBuffer().get();
            fail();
        } catch (BufferUnderflowException expected) {
        }
        try {
            b.asCharBuffer().get(0);
            fail();
        } catch (IndexOutOfBoundsException expected) {
            assertTrue(expected.getMessage().contains("limit=0"));
        }

        try {
            b.asDoubleBuffer().get();
            fail();
        } catch (BufferUnderflowException expected) {
        }
        try {
            b.asDoubleBuffer().get(0);
            fail();
        } catch (IndexOutOfBoundsException expected) {
            assertTrue(expected.getMessage().contains("limit=0"));
        }

        try {
            b.asFloatBuffer().get();
            fail();
        } catch (BufferUnderflowException expected) {
        }
        try {
            b.asFloatBuffer().get(0);
            fail();
        } catch (IndexOutOfBoundsException expected) {
            assertTrue(expected.getMessage().contains("limit=0"));
        }

        try {
            b.asIntBuffer().get();
            fail();
        } catch (BufferUnderflowException expected) {
        }
        try {
            b.asIntBuffer().get(0);
            fail();
        } catch (IndexOutOfBoundsException expected) {
            assertTrue(expected.getMessage().contains("limit=0"));
        }

        try {
            b.asLongBuffer().get();
            fail();
        } catch (BufferUnderflowException expected) {
        }
        try {
            b.asLongBuffer().get(0);
            fail();
        } catch (IndexOutOfBoundsException expected) {
            assertTrue(expected.getMessage().contains("limit=0"));
        }

        try {
            b.asShortBuffer().get();
            fail();
        } catch (BufferUnderflowException expected) {
        }
        try {
            b.asShortBuffer().get(0);
            fail();
        } catch (IndexOutOfBoundsException expected) {
            assertTrue(expected.getMessage().contains("limit=0"));
        }
    }

    public void testUsingDirectBufferAsMappedBuffer() throws Exception {
        MappedByteBuffer notMapped = (MappedByteBuffer) ByteBuffer.allocateDirect(1);
        try {
            notMapped.force();
            fail();
        } catch (UnsupportedOperationException expected) {
        }
        try {
            notMapped.isLoaded();
            fail();
        } catch (UnsupportedOperationException expected) {
        }
        try {
            notMapped.load();
            fail();
        } catch (UnsupportedOperationException expected) {
        }

        MappedByteBuffer mapped = (MappedByteBuffer) allocateMapped(1);
        mapped.force();
        mapped.isLoaded();
        mapped.load();
    }

    // https://code.google.com/p/android/issues/detail?id=53637
    public void testBug53637() throws Exception {
        MappedByteBuffer mapped = (MappedByteBuffer) allocateMapped(1);
        mapped.get();
        mapped.rewind();
        mapped.get();

        mapped.rewind();
        mapped.mark();
        mapped.get();
        mapped.reset();
        mapped.get();

        mapped.rewind();
        mapped.get();
        mapped.clear();
        mapped.get();

        mapped.rewind();
        mapped.get();
        mapped.flip();
        mapped.get();
    }

    public void testElementSizeShifts() {
        // Element size shifts are the log base 2 of the element size
        // of this buffer.
        assertEquals(1, 1 << ByteBuffer.allocate(0).getElementSizeShift());

        assertEquals(SizeOf.CHAR, 1 << CharBuffer.allocate(0).getElementSizeShift());
        assertEquals(SizeOf.SHORT, 1 << ShortBuffer.allocate(0).getElementSizeShift());

        assertEquals(SizeOf.INT, 1 << IntBuffer.allocate(0).getElementSizeShift());
        assertEquals(SizeOf.FLOAT, 1 << FloatBuffer.allocate(0).getElementSizeShift());

        assertEquals(SizeOf.LONG, 1 << LongBuffer.allocate(0).getElementSizeShift());
        assertEquals(SizeOf.DOUBLE, 1 << DoubleBuffer.allocate(0).getElementSizeShift());
    }

    public void testFreed() {
        ByteBuffer b1 = ByteBuffer.allocateDirect(1);
        ByteBuffer b2 = b1.duplicate();
        NioUtils.freeDirectBuffer(b1);
        for (ByteBuffer b: new ByteBuffer[] { b1, b2 }) {
          assertFalse(b.isAccessible());
            try {
                b.compact();
                fail();
            } catch (IllegalStateException expected) {
            }
            try {
                b.duplicate();
                fail();
            } catch (IllegalStateException expected) {
            }
            testFailForPutMethods(b);
            testFailForAsMethods(b);
            testFailForGetMethods(b);
            NioUtils.freeDirectBuffer(b); // should be able to free twice
        }
    }

    public void testAccess() {
        ByteBuffer b1 = ByteBuffer.allocate(1);
        ByteBuffer b2 = b1.duplicate();
        for (ByteBuffer b: new ByteBuffer[] { b1, b2 }) {
            try {
                b.setAccessible(true);
                fail();
            } catch (UnsupportedOperationException expected) {
            }
            try {
                b.setAccessible(false);
                fail();
            } catch (UnsupportedOperationException expected) {
            }
        }
        b1 = ByteBuffer.allocateDirect(8);
        b2 = b1.duplicate();
        b1.setAccessible(false);
        ByteBuffer b3 = b1.asReadOnlyBuffer();
        for (ByteBuffer b: new ByteBuffer[] { b1, b2, b3 }) {
            b.duplicate();
            assertFalse(b.isAccessible());
            // even read-only buffers should fail with IllegalStateException
            testFailForPutMethods(b);
            testAsMethods(b);
            testFailForGetMethods(b);
            b.position(0);
            b.limit(8);
            try {
                b.asCharBuffer().get(0);
                fail();
            } catch (IllegalStateException expected) {
            }
            try {
                b.asShortBuffer().get(0);
                fail();
            } catch (IllegalStateException expected) {
            }
            try {
                b.asIntBuffer().get(0);
                fail();
            } catch (IllegalStateException expected) {
            }
            try {
                b.asLongBuffer().get(0);
                fail();
            } catch (IllegalStateException expected) {
            }
            try {
                b.asFloatBuffer().get(0);
                fail();
            } catch (IllegalStateException expected) {
            }
            try {
                b.asDoubleBuffer().get(0);
                fail();
            } catch (IllegalStateException expected) {
            }
        }
        b2.setAccessible(true);
        for (ByteBuffer b: new ByteBuffer[] { b1, b2, b3 }) {
            assertTrue(b.isAccessible());
            b.position(0);
            b.limit(8);
            b.asCharBuffer().get(0);
            b.asShortBuffer().get(0);
            b.asIntBuffer().get(0);
            b.asLongBuffer().get(0);
            b.asFloatBuffer().get(0);
            b.asDoubleBuffer().get(0);
            if (!b.isReadOnly()) {
                testPutMethods(b);
                b.compact();
            } else {
                try {
                    b.put(0, (byte) 0);
                    fail();
                } catch (ReadOnlyBufferException expected) {
                }
            }
            testAsMethods(b);
            testGetMethods(b);
        }
    }

    private void testPutMethods(ByteBuffer b) {
        b.position(0);
        b.put((byte) 0);
        b.put(0, (byte) 0);
        b.put(new byte[1]);
        b.put(new byte[1], 0, 1);
        b.put(ByteBuffer.allocate(1));
        b.putChar('a');
        b.putChar(0, 'a');
        b.position(0);
        b.putDouble(0);
        b.putDouble(0, 0);
        b.position(0);
        b.putFloat(0);
        b.putFloat(0, 0);
        b.putInt(0);
        b.putInt(0, 0);
        b.position(0);
        b.putLong(0);
        b.putLong(0, 0);
        b.position(0);
        b.putShort((short) 0);
        b.putShort(0, (short) 0);
    }

    private void testFailForPutMethods(ByteBuffer b) {
        try {
            b.put((byte) 0);
            fail();
        } catch (IllegalStateException expected) {
        }
        try {
            b.put(0, (byte) 0);
            fail();
        } catch (IllegalStateException expected) {
        }
        try {
            b.put(new byte[1]);
            fail();
        } catch (IllegalStateException expected) {
        }
        try {
            b.put(new byte[1], 0, 1);
            fail();
        } catch (IllegalStateException expected) {
        }
        try {
            b.put(ByteBuffer.allocate(1));
            fail();
        } catch (IllegalStateException expected) {
        }
        try {
            b.putChar('a');
            fail();
        } catch (IllegalStateException expected) {
        }
        try {
            b.putChar(0, 'a');
            fail();
        } catch (IllegalStateException expected) {
        }
        try {
            b.putDouble(0);
            fail();
        } catch (IllegalStateException expected) {
        }
        try {
            b.putDouble(0, 0);
            fail();
        } catch (IllegalStateException expected) {
        }
        try {
            b.putFloat(0);
            fail();
        } catch (IllegalStateException expected) {
        }
        try {
            b.putFloat(0, 0);
            fail();
        } catch (IllegalStateException expected) {
        }
        try {
            b.putInt(0);
            fail();
        } catch (IllegalStateException expected) {
        }
        try {
            b.putInt(0, 0);
            fail();
        } catch (IllegalStateException expected) {
        }
        try {
            b.putLong(0);
            fail();
        } catch (IllegalStateException expected) {
        }
        try {
            b.putLong(0, 0);
            fail();
        } catch (IllegalStateException expected) {
        }
        try {
            b.putShort((short) 0);
            fail();
        } catch (IllegalStateException expected) {
        }
        try {
            b.putShort(0, (short) 0);
            fail();
        } catch (IllegalStateException expected) {
        }
    }

    private void testGetMethods(ByteBuffer b) {
        b.position(0);
        b.get();
        b.get(0);
        b.get(new byte[1]);
        b.get(new byte[1], 0, 1);
        b.getChar();
        b.getChar(0);
        b.position(0);
        b.getDouble();
        b.getDouble(0);
        b.position(0);
        b.getFloat();
        b.getFloat(0);
        b.getInt();
        b.getInt(0);
        b.position(0);
        b.getLong();
        b.getLong(0);
        b.position(0);
        b.getShort();
        b.getShort(0);
    }

    private void testFailForGetMethods(ByteBuffer b) {
        try {
            b.get();
            fail();
        } catch (IllegalStateException expected) {
        }
        try {
            b.get(0);
            fail();
        } catch (IllegalStateException expected) {
        }
        try {
            b.get(new byte[1]);
            fail();
        } catch (IllegalStateException expected) {
        }
        try {
            b.get(new byte[1], 0, 1);
            fail();
        } catch (IllegalStateException expected) {
        }
        try {
            b.getChar();
            fail();
        } catch (IllegalStateException expected) {
        }
        try {
            b.getChar(0);
            fail();
        } catch (IllegalStateException expected) {
        }
        try {
            b.getDouble();
            fail();
        } catch (IllegalStateException expected) {
        }
        try {
            b.getDouble(0);
            fail();
        } catch (IllegalStateException expected) {
        }
        try {
            b.getFloat();
            fail();
        } catch (IllegalStateException expected) {
        }
        try {
            b.getFloat(0);
            fail();
        } catch (IllegalStateException expected) {
        }
        try {
            b.getInt();
            fail();
        } catch (IllegalStateException expected) {
        }
        try {
            b.getInt(0);
            fail();
        } catch (IllegalStateException expected) {
        }
        try {
            b.getLong();
            fail();
        } catch (IllegalStateException expected) {
        }
        try {
            b.getLong(0);
            fail();
        } catch (IllegalStateException expected) {
        }
        try {
            b.getShort();
            fail();
        } catch (IllegalStateException expected) {
        }
        try {
            b.getShort(0);
            fail();
        } catch (IllegalStateException expected) {
        }
    }

    private void testAsMethods(ByteBuffer b) {
        b.asCharBuffer();
        b.asDoubleBuffer();
        b.asFloatBuffer();
        b.asIntBuffer();
        b.asLongBuffer();
        b.asReadOnlyBuffer();
        b.asShortBuffer();
    }

    private void testFailForAsMethods(ByteBuffer b) {
        try {
            b.asCharBuffer();
            fail();
        } catch (IllegalStateException expected) {
        }
        try {
            b.asDoubleBuffer();
            fail();
        } catch (IllegalStateException expected) {
        }
        try {
            b.asFloatBuffer();
            fail();
        } catch (IllegalStateException expected) {
        }
        try {
            b.asIntBuffer();
            fail();
        } catch (IllegalStateException expected) {
        }
        try {
            b.asLongBuffer();
            fail();
        } catch (IllegalStateException expected) {
        }
        try {
            b.asReadOnlyBuffer();
            fail();
        } catch (IllegalStateException expected) {
        }
        try {
            b.asShortBuffer();
            fail();
        } catch (IllegalStateException expected) {
        }
    }

    // b/26019694
    public void testIndependentLimit() {
        // Check if the limit parameter of original ByteBuffer
        // is not affecting buffer created by its as*Buffer
        testBuffersIndependentLimit(ByteBuffer.allocateDirect(16));
        testBuffersIndependentLimit(ByteBuffer.allocate(16));
    }

    private void testBuffersIndependentLimit(ByteBuffer b) {
        CharBuffer c = b.asCharBuffer();
        b.limit(b.capacity()); c.put(1, (char)1);
        b.limit(0);  c.put(1, (char)1);

        b.limit(b.capacity());
        ShortBuffer s = b.asShortBuffer();
        b.limit(b.capacity()); s.put(1, (short)1);
        b.limit(0);  s.put(1, (short)1);

        b.limit(b.capacity());
        IntBuffer i = b.asIntBuffer();
        i.put(1, (int)1);
        b.limit(0);  i.put(1, (int)1);

        b.limit(b.capacity());
        LongBuffer l = b.asLongBuffer();
        l.put(1, (long)1);
        b.limit(0);  l.put(1, (long)1);

        b.limit(b.capacity());
        FloatBuffer f = b.asFloatBuffer();
        f.put(1, (float)1);
        b.limit(0);  f.put(1, (float)1);

        b.limit(b.capacity());
        DoubleBuffer d = b.asDoubleBuffer();
        d.put(1, (double)1);
        b.limit(0);  d.put(1, (double)1);
    }

    // http://b/32655865
    public void test_ByteBufferAsXBuffer_ByteOrder() {
        ByteBuffer byteBuffer = ByteBuffer.allocate(10);
        // Fill a ByteBuffer with different bytes that make it easy to tell byte ordering issues.
        for (int i = 0; i < 10; i++) {
            byteBuffer.put((byte)i);
        }
        byteBuffer.rewind();
        // Obtain a big-endian and little-endian copy of the source array.
        ByteBuffer bigEndian = byteBuffer.duplicate().order(ByteOrder.BIG_ENDIAN);
        ByteBuffer littleEndian = byteBuffer.duplicate().order(ByteOrder.LITTLE_ENDIAN);

        // Check each type longer than a byte to confirm the ordering differs.
        // asXBuffer.
        assertFalse(bigEndian.asShortBuffer().get() == littleEndian.asShortBuffer().get());
        assertFalse(bigEndian.asIntBuffer().get() == littleEndian.asIntBuffer().get());
        assertFalse(bigEndian.asLongBuffer().get() == littleEndian.asLongBuffer().get());
        assertFalse(bigEndian.asDoubleBuffer().get() == littleEndian.asDoubleBuffer().get());
        assertFalse(bigEndian.asCharBuffer().get() == littleEndian.asCharBuffer().get());
        assertFalse(bigEndian.asFloatBuffer().get() == littleEndian.asFloatBuffer().get());

        // asXBuffer().asReadOnlyBuffer()
        assertFalse(bigEndian.asShortBuffer().asReadOnlyBuffer().get() ==
                littleEndian.asShortBuffer().asReadOnlyBuffer().get());
        assertFalse(bigEndian.asIntBuffer().asReadOnlyBuffer().get() ==
                littleEndian.asIntBuffer().asReadOnlyBuffer().get());
        assertFalse(bigEndian.asLongBuffer().asReadOnlyBuffer().get() ==
                littleEndian.asLongBuffer().asReadOnlyBuffer().get());
        assertFalse(bigEndian.asDoubleBuffer().asReadOnlyBuffer().get() ==
                littleEndian.asDoubleBuffer().asReadOnlyBuffer().get());
        assertFalse(bigEndian.asCharBuffer().asReadOnlyBuffer().get() ==
                littleEndian.asCharBuffer().asReadOnlyBuffer().get());
        assertFalse(bigEndian.asFloatBuffer().asReadOnlyBuffer().get() ==
                littleEndian.asFloatBuffer().asReadOnlyBuffer().get());

        // asXBuffer().duplicate()
        assertFalse(bigEndian.asShortBuffer().duplicate().get() ==
                littleEndian.asShortBuffer().duplicate().get());
        assertFalse(bigEndian.asIntBuffer().duplicate().get() ==
                littleEndian.asIntBuffer().duplicate().get());
        assertFalse(bigEndian.asLongBuffer().duplicate().get() ==
                littleEndian.asLongBuffer().duplicate().get());
        assertFalse(bigEndian.asDoubleBuffer().duplicate().get() ==
                littleEndian.asDoubleBuffer().duplicate().get());
        assertFalse(bigEndian.asCharBuffer().duplicate().get() ==
                littleEndian.asCharBuffer().duplicate().get());
        assertFalse(bigEndian.asFloatBuffer().duplicate().get() ==
                littleEndian.asFloatBuffer().duplicate().get());

        // asXBuffer().slice()
        assertFalse(bigEndian.asShortBuffer().slice().get() ==
                littleEndian.asShortBuffer().slice().get());
        assertFalse(bigEndian.asIntBuffer().slice().get() ==
                littleEndian.asIntBuffer().slice().get());
        assertFalse(bigEndian.asLongBuffer().slice().get() ==
                littleEndian.asLongBuffer().slice().get());
        assertFalse(bigEndian.asDoubleBuffer().slice().get() ==
                littleEndian.asDoubleBuffer().slice().get());
        assertFalse(bigEndian.asCharBuffer().slice().get() ==
                littleEndian.asCharBuffer().slice().get());
        assertFalse(bigEndian.asFloatBuffer().slice().get() ==
                littleEndian.asFloatBuffer().slice().get());
    }

    // http://b/32655865
    public void test_ByteBufferAsXBuffer_ByteOrder_2() {
        ByteBuffer byteBuffer = ByteBuffer.allocateDirect(10);
        byteBuffer.order(ByteOrder.BIG_ENDIAN);
        // Fill a ByteBuffer with different bytes that make it easy to tell byte ordering issues.
        for (int i = 0; i < 10; i++) {
            byteBuffer.put((byte)i);
        }
        byteBuffer.rewind();

        // Create BIG_ENDIAN views of the buffer.
        ShortBuffer sb_be = byteBuffer.asShortBuffer();
        LongBuffer lb_be = byteBuffer.asLongBuffer();
        IntBuffer ib_be = byteBuffer.asIntBuffer();
        DoubleBuffer db_be = byteBuffer.asDoubleBuffer();
        CharBuffer cb_be = byteBuffer.asCharBuffer();
        FloatBuffer fb_be = byteBuffer.asFloatBuffer();

        // Change the order of the underlying buffer.
        byteBuffer.order(ByteOrder.LITTLE_ENDIAN);

        // Create LITTLE_ENDIAN views of the buffer.
        ShortBuffer sb_le = byteBuffer.asShortBuffer();
        LongBuffer lb_le = byteBuffer.asLongBuffer();
        IntBuffer ib_le = byteBuffer.asIntBuffer();
        DoubleBuffer db_le = byteBuffer.asDoubleBuffer();
        CharBuffer cb_le = byteBuffer.asCharBuffer();
        FloatBuffer fb_le = byteBuffer.asFloatBuffer();

        assertFalse(sb_be.get() == sb_le.get());
        assertFalse(lb_be.get() == lb_le.get());
        assertFalse(ib_be.get() == ib_le.get());
        assertFalse(db_be.get() == db_le.get());
        assertFalse(cb_be.get() == cb_le.get());
        assertFalse(fb_be.get() == fb_le.get());
    }

    // Allocate new direct byte buffer using JNI NewDirectByteBuffer
    public static native ByteBuffer jniNewDirectByteBuffer();

    // http://b/62133955
    public void test_DirectByteBuffer_PhantomReference() throws Exception {
        ByteBuffer bb = jniNewDirectByteBuffer();
        ReferenceQueue rq = new ReferenceQueue();
        PhantomReference pr = new PhantomReference(bb, rq);

        // The phantom reference pr should still be valid while 'bb' still
        // has a strong reference to the ByteBuffer.
        FinalizationTester.induceFinalization();
        assertTrue(!pr.isEnqueued());

        // Create a read-only, derived buffer from the jni buffer and clear original
        // reference. The phantom reference should still be valid because the
        // 'otherBuffer' references the MemoryRef which contains reference to buffer
        // previously stored in 'bb'.
        ByteBuffer otherBuffer = bb.asReadOnlyBuffer();
        bb = null;
        FinalizationTester.induceFinalization();
        assertTrue(!pr.isEnqueued());

        // Clear reference to derived buffer. The phantom reference should be
        // eligible for collection because there are no strong references left.
        otherBuffer = null;
        FinalizationTester.induceFinalization();
        assertTrue(pr.isEnqueued());
    }
}
