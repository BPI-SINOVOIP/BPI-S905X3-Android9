/*
 * Copyright (C) 2009 The Android Open Source Project
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

package android.os.cts;

import java.io.FileDescriptor;
import java.io.Serializable;
import java.util.ArrayList;
import java.util.HashMap;

import android.content.pm.Signature;
import android.os.BadParcelableException;
import android.os.Binder;
import android.os.Bundle;
import android.os.IBinder;
import android.os.IInterface;
import android.os.Parcel;
import android.os.ParcelFileDescriptor;
import android.os.Parcelable;
import android.test.AndroidTestCase;
import android.util.Log;
import android.util.SparseArray;
import android.util.SparseBooleanArray;

public class ParcelTest extends AndroidTestCase {

    public void testObtain() {
        Parcel p1 = Parcel.obtain();
        assertNotNull(p1);
        Parcel p2 = Parcel.obtain();
        assertNotNull(p2);
        Parcel p3 = Parcel.obtain();
        assertNotNull(p3);
        Parcel p4 = Parcel.obtain();
        assertNotNull(p4);
        Parcel p5 = Parcel.obtain();
        assertNotNull(p5);
        Parcel p6 = Parcel.obtain();
        assertNotNull(p6);
        Parcel p7 = Parcel.obtain();
        assertNotNull(p7);

        p1.recycle();
        p2.recycle();
        p3.recycle();
        p4.recycle();
        p5.recycle();
        p6.recycle();
        p7.recycle();
    }

    public void testAppendFrom() {
        Parcel p;
        Parcel p2;
        int d1;
        int d2;

        p = Parcel.obtain();
        d1 = p.dataPosition();
        p.writeInt(7);
        p.writeInt(5);
        d2 = p.dataPosition();
        p2 = Parcel.obtain();
        p2.appendFrom(p, d1, d2 - d1);
        p2.setDataPosition(0);
        assertEquals(7, p2.readInt());
        assertEquals(5, p2.readInt());
        p2.recycle();
        p.recycle();
    }

    public void testDataAvail() {
        Parcel p;

        p = Parcel.obtain();
        p.writeInt(7); // size 4
        p.writeInt(5); // size 4
        p.writeLong(7L); // size 8
        p.writeString("7L"); // size 12
        p.setDataPosition(0);
        assertEquals(p.dataSize(), p.dataAvail());
        p.readInt();
        assertEquals(p.dataSize() - p.dataPosition(), p.dataAvail());
        p.readInt();
        assertEquals(p.dataSize() - p.dataPosition(), p.dataAvail());
        p.readLong();
        assertEquals(p.dataSize() - p.dataPosition(), p.dataAvail());
        p.readString();
        assertEquals(p.dataSize() - p.dataPosition(), p.dataAvail());
        p.recycle();
    }

    public void testDataCapacity() {
        Parcel p;

        p = Parcel.obtain();
        assertEquals(0, p.dataCapacity());
        p.writeInt(7); // size 4
        int dC1 = p.dataCapacity();
        p.writeDouble(2.19);
        int dC2 = p.dataCapacity();
        assertTrue(dC2 > dC1);
        p.recycle();
    }

    public void testSetDataCapacity() {
        Parcel p;

        p = Parcel.obtain();
        assertEquals(0, p.dataCapacity());
        p.setDataCapacity(2);
        assertEquals(2, p.dataCapacity());
        p.setDataCapacity(1);
        assertEquals(2, p.dataCapacity());
        p.setDataCapacity(3);
        assertEquals(3, p.dataCapacity());
        p.recycle();
    }

    public void testDataPosition() {
        Parcel p;

        p = Parcel.obtain();
        assertEquals(0, p.dataPosition());
        p.writeInt(7); // size 4
        int dP1 = p.dataPosition();
        p.writeLong(7L); // size 8
        int dP2 = p.dataPosition();
        assertTrue(dP2 > dP1);
        p.recycle();
    }

    public void testSetDataPosition() {
        Parcel p;

        p = Parcel.obtain();
        assertEquals(0, p.dataSize());
        assertEquals(0, p.dataPosition());
        p.setDataPosition(4);
        assertEquals(4, p.dataPosition());
        p.setDataPosition(7);
        assertEquals(7, p.dataPosition());
        p.setDataPosition(0);
        p.writeInt(7);
        assertEquals(4, p.dataSize());
        p.setDataPosition(4);
        assertEquals(4, p.dataPosition());
        p.setDataPosition(7);
        assertEquals(7, p.dataPosition());
        p.recycle();
    }

    public void testDataSize() {
        Parcel p;

        p = Parcel.obtain();
        assertEquals(0, p.dataSize());
        p.writeInt(7); // size 4
        assertEquals(4, p.dataSize());
        p.writeInt(5); // size 4
        assertEquals(8, p.dataSize());
        p.writeLong(7L); // size 8
        assertEquals(16, p.dataSize());
        p.recycle();
    }

    public void testSetDataSize() {
        Parcel p;

        p = Parcel.obtain();
        assertEquals(0, p.dataSize());
        p.setDataSize(5);
        assertEquals(5, p.dataSize());
        p.setDataSize(3);
        assertEquals(3, p.dataSize());

        p.writeInt(3);
        assertEquals(4, p.dataSize());
        p.setDataSize(5);
        assertEquals(5, p.dataSize());
        p.setDataSize(3);
        assertEquals(3, p.dataSize());
        p.recycle();
    }

    public void testEnforceInterface() {
        Parcel p;
        String s = "IBinder interface token";

        p = Parcel.obtain();
        p.writeInterfaceToken(s);
        p.setDataPosition(0);
        try {
            p.enforceInterface("");
            fail("Should throw an SecurityException");
        } catch (SecurityException e) {
            //expected
        }
        p.recycle();

        p = Parcel.obtain();
        p.writeInterfaceToken(s);
        p.setDataPosition(0);
        p.enforceInterface(s);
        p.recycle();
    }

    public void testMarshall() {
        final byte[] c = {Byte.MAX_VALUE, (byte) 111, (byte) 11, (byte) 1, (byte) 0,
                    (byte) -1, (byte) -11, (byte) -111, Byte.MIN_VALUE};

        Parcel p1 = Parcel.obtain();
        p1.writeByteArray(c);
        p1.setDataPosition(0);
        byte[] d1 = p1.marshall();

        Parcel p2 = Parcel.obtain();
        p2.unmarshall(d1, 0, d1.length);
        p2.setDataPosition(0);
        byte[] d2 = new byte[c.length];
        p2.readByteArray(d2);

        for (int i = 0; i < c.length; i++) {
            assertEquals(c[i], d2[i]);
        }

        p1.recycle();
        p2.recycle();
    }

    @SuppressWarnings("unchecked")
    public void testReadValue() {
        Parcel p;
        MockClassLoader mcl = new MockClassLoader();

        // test null
        p = Parcel.obtain();
        p.writeValue(null);
        p.setDataPosition(0);
        assertNull(p.readValue(mcl));
        p.recycle();

        // test String
        p = Parcel.obtain();
        p.writeValue("String");
        p.setDataPosition(0);
        assertEquals("String", p.readValue(mcl));
        p.recycle();

        // test Integer
        p = Parcel.obtain();
        p.writeValue(Integer.MAX_VALUE);
        p.setDataPosition(0);
        assertEquals(Integer.MAX_VALUE, p.readValue(mcl));
        p.recycle();

        // test Map
        HashMap map = new HashMap();
        HashMap map2;
        map.put("string", "String");
        map.put("int", Integer.MAX_VALUE);
        map.put("boolean", true);
        p = Parcel.obtain();
        p.writeValue(map);
        p.setDataPosition(0);
        map2 = (HashMap) p.readValue(mcl);
        assertNotNull(map2);
        assertEquals(map.size(), map2.size());
        assertEquals("String", map.get("string"));
        assertEquals(Integer.MAX_VALUE, map.get("int"));
        assertEquals(true, map.get("boolean"));
        p.recycle();

        // test Bundle
        Bundle bundle = new Bundle();
        bundle.putBoolean("boolean", true);
        bundle.putInt("int", Integer.MAX_VALUE);
        bundle.putString("string", "String");
        Bundle bundle2;
        p = Parcel.obtain();
        p.writeValue(bundle);
        p.setDataPosition(0);
        bundle2 = (Bundle) p.readValue(mcl);
        assertNotNull(bundle2);
        assertEquals(true, bundle2.getBoolean("boolean"));
        assertEquals(Integer.MAX_VALUE, bundle2.getInt("int"));
        assertEquals("String", bundle2.getString("string"));
        p.recycle();

        // test Parcelable
        final String signatureString  = "1234567890abcdef";
        Signature s = new Signature(signatureString);
        p = Parcel.obtain();
        p.writeValue(s);
        p.setDataPosition(0);
        assertEquals(s, p.readValue(mcl));
        p.recycle();

        // test Short
        p = Parcel.obtain();
        p.writeValue(Short.MAX_VALUE);
        p.setDataPosition(0);
        assertEquals(Short.MAX_VALUE, p.readValue(mcl));
        p.recycle();

        // test Long
        p = Parcel.obtain();
        p.writeValue(Long.MAX_VALUE);
        p.setDataPosition(0);
        assertEquals(Long.MAX_VALUE, p.readValue(mcl));
        p.recycle();

        // test Float
        p = Parcel.obtain();
        p.writeValue(Float.MAX_VALUE);
        p.setDataPosition(0);
        assertEquals(Float.MAX_VALUE, p.readValue(mcl));
        p.recycle();

        // test Double
        p = Parcel.obtain();
        p.writeValue(Double.MAX_VALUE);
        p.setDataPosition(0);
        assertEquals(Double.MAX_VALUE, p.readValue(mcl));
        p.recycle();

        // test Boolean
        p = Parcel.obtain();
        p.writeValue(true);
        p.writeValue(false);
        p.setDataPosition(0);
        assertTrue((Boolean) p.readValue(mcl));
        assertFalse((Boolean) p.readValue(mcl));
        p.recycle();

        // test CharSequence
        p = Parcel.obtain();
        p.writeValue((CharSequence) "CharSequence");
        p.setDataPosition(0);
        assertEquals("CharSequence", p.readValue(mcl));
        p.recycle();

        // test List
        ArrayList arrayList2 = new ArrayList();
        arrayList2.add(Integer.MAX_VALUE);
        arrayList2.add(true);
        arrayList2.add(Long.MAX_VALUE);
        ArrayList arrayList = new ArrayList();
        p = Parcel.obtain();
        p.writeValue(arrayList2);
        p.setDataPosition(0);
        assertEquals(0, arrayList.size());
        arrayList = (ArrayList) p.readValue(mcl);
        assertEquals(3, arrayList.size());
        for (int i = 0; i < arrayList.size(); i++) {
            assertEquals(arrayList.get(i), arrayList2.get(i));
        }
        p.recycle();

        // test SparseArray
        SparseArray<Object> sparseArray = new SparseArray<Object>();
        sparseArray.put(3, "String");
        sparseArray.put(2, Long.MAX_VALUE);
        sparseArray.put(4, Float.MAX_VALUE);
        sparseArray.put(0, Integer.MAX_VALUE);
        sparseArray.put(1, true);
        sparseArray.put(10, true);
        SparseArray<Object> sparseArray2;
        p = Parcel.obtain();
        p.writeValue(sparseArray);
        p.setDataPosition(0);
        sparseArray2 = (SparseArray<Object>) p.readValue(mcl);
        assertNotNull(sparseArray2);
        assertEquals(sparseArray.size(), sparseArray2.size());
        assertEquals(sparseArray.get(0), sparseArray2.get(0));
        assertEquals(sparseArray.get(1), sparseArray2.get(1));
        assertEquals(sparseArray.get(2), sparseArray2.get(2));
        assertEquals(sparseArray.get(3), sparseArray2.get(3));
        assertEquals(sparseArray.get(4), sparseArray2.get(4));
        assertEquals(sparseArray.get(10), sparseArray2.get(10));
        p.recycle();

        // test boolean[]
        boolean[] booleanArray  = {true, false, true, false};
        boolean[] booleanArray2 = new boolean[booleanArray.length];
        p = Parcel.obtain();
        p.writeValue(booleanArray);
        p.setDataPosition(0);
        booleanArray2 = (boolean[]) p.readValue(mcl);
        for (int i = 0; i < booleanArray.length; i++) {
            assertEquals(booleanArray[i], booleanArray2[i]);
        }
        p.recycle();

        // test byte[]
        byte[] byteArray = {Byte.MAX_VALUE, (byte) 111, (byte) 11, (byte) 1, (byte) 0,
                (byte) -1, (byte) -11, (byte) -111, Byte.MIN_VALUE};
        byte[] byteArray2 = new byte[byteArray.length];
        p = Parcel.obtain();
        p.writeValue(byteArray);
        p.setDataPosition(0);
        byteArray2 = (byte[]) p.readValue(mcl);
        for (int i = 0; i < byteArray.length; i++) {
            assertEquals(byteArray[i], byteArray2[i]);
        }
        p.recycle();

        // test string[]
        String[] stringArray = {"",
                "a",
                "Hello, Android!",
                "A long string that is used to test the api readStringArray(),"};
        String[] stringArray2 = new String[stringArray.length];
        p = Parcel.obtain();
        p.writeValue(stringArray);
        p.setDataPosition(0);
        stringArray2 = (String[]) p.readValue(mcl);
        for (int i = 0; i < stringArray.length; i++) {
            assertEquals(stringArray[i], stringArray2[i]);
        }
        p.recycle();

        // test IBinder
        Binder binder;
        Binder binder2 = new Binder();
        p = Parcel.obtain();
        p.writeValue(binder2);
        p.setDataPosition(0);
        binder = (Binder) p.readValue(mcl);
        assertEquals(binder2, binder);
        p.recycle();

        // test Parcelable[]
        Signature[] signatures = {new Signature("1234"),
                new Signature("ABCD"),
                new Signature("abcd")};
        Parcelable[] signatures2;
        p = Parcel.obtain();
        p.writeValue(signatures);
        p.setDataPosition(0);
        signatures2 = (Parcelable[]) p.readValue(mcl);
        for (int i = 0; i < signatures.length; i++) {
            assertEquals(signatures[i], signatures2[i]);
        }
        p.recycle();

        // test Object
        Object[] objects = new Object[5];
        objects[0] = Integer.MAX_VALUE;
        objects[1] = true;
        objects[2] = Long.MAX_VALUE;
        objects[3] = "String";
        objects[4] = Float.MAX_VALUE;
        Object[] objects2;
        p = Parcel.obtain();
        p.writeValue(objects);
        p.setDataPosition(0);
        objects2 = (Object[]) p.readValue(mcl);
        assertNotNull(objects2);
        for (int i = 0; i < objects2.length; i++) {
            assertEquals(objects[i], objects2[i]);
        }
        p.recycle();

        // test int[]
        int[] intArray = {111, 11, 1, 0, -1, -11, -111};
        int[] intArray2 = new int[intArray.length];
        p = Parcel.obtain();
        p.writeValue(intArray);
        p.setDataPosition(0);
        intArray2= (int[]) p.readValue(mcl);
        assertNotNull(intArray2);
        for (int i = 0; i < intArray2.length; i++) {
            assertEquals(intArray[i], intArray2[i]);
        }
        p.recycle();

        // test long[]
        long[] longArray = {111L, 11L, 1L, 0L, -1L, -11L, -111L};
        long[] longArray2 = new long[longArray.length];
        p = Parcel.obtain();
        p.writeValue(longArray);
        p.setDataPosition(0);
        longArray2= (long[]) p.readValue(mcl);
        assertNotNull(longArray2);
        for (int i = 0; i < longArray2.length; i++) {
            assertEquals(longArray[i], longArray2[i]);
        }
        p.recycle();

        // test byte
        p = Parcel.obtain();
        p.writeValue(Byte.MAX_VALUE);
        p.setDataPosition(0);
        assertEquals(Byte.MAX_VALUE, p.readValue(mcl));
        p.recycle();

        // test Serializable
        p = Parcel.obtain();
        p.writeValue((Serializable) "Serializable");
        p.setDataPosition(0);
        assertEquals("Serializable", p.readValue(mcl));
        p.recycle();
    }

    public void testReadByte() {
        Parcel p;

        p = Parcel.obtain();
        p.writeByte((byte) 0);
        p.setDataPosition(0);
        assertEquals((byte) 0, p.readByte());
        p.recycle();

        p = Parcel.obtain();
        p.writeByte((byte) 1);
        p.setDataPosition(0);
        assertEquals((byte) 1, p.readByte());
        p.recycle();

        p = Parcel.obtain();
        p.writeByte((byte) -1);
        p.setDataPosition(0);
        assertEquals((byte) -1, p.readByte());
        p.recycle();

        p = Parcel.obtain();
        p.writeByte(Byte.MAX_VALUE);
        p.setDataPosition(0);
        assertEquals(Byte.MAX_VALUE, p.readByte());
        p.recycle();

        p = Parcel.obtain();
        p.writeByte(Byte.MIN_VALUE);
        p.setDataPosition(0);
        assertEquals(Byte.MIN_VALUE, p.readByte());
        p.recycle();

        p = Parcel.obtain();
        p.writeByte(Byte.MAX_VALUE);
        p.writeByte((byte) 11);
        p.writeByte((byte) 1);
        p.writeByte((byte) 0);
        p.writeByte((byte) -1);
        p.writeByte((byte) -11);
        p.writeByte(Byte.MIN_VALUE);
        p.setDataPosition(0);
        assertEquals(Byte.MAX_VALUE, p.readByte());
        assertEquals((byte) 11, p.readByte());
        assertEquals((byte) 1, p.readByte());
        assertEquals((byte) 0, p.readByte());
        assertEquals((byte) -1, p.readByte());
        assertEquals((byte) -11, p.readByte());
        assertEquals(Byte.MIN_VALUE, p.readByte());
        p.recycle();
    }

    public void testReadByteArray() {
        Parcel p;

        byte[] a = {(byte) 21};
        byte[] b = new byte[a.length];

        byte[] c = {Byte.MAX_VALUE, (byte) 111, (byte) 11, (byte) 1, (byte) 0,
                    (byte) -1, (byte) -11, (byte) -111, Byte.MIN_VALUE};
        byte[] d = new byte[c.length];

        // test write null
        p = Parcel.obtain();
        p.writeByteArray(null);
        p.setDataPosition(0);
        try {
            p.readByteArray(null);
            fail("Should throw a RuntimeException");
        } catch (RuntimeException e) {
            //expected
        }

        p.setDataPosition(0);
        try {
            p.readByteArray(b);
            fail("Should throw a RuntimeException");
        } catch (RuntimeException e) {
            //expected
        }
        p.recycle();

        // test write byte array with length: 1
        p = Parcel.obtain();
        p.writeByteArray(a);
        p.setDataPosition(0);
        try {
            p.readByteArray(d);
            fail("Should throw a RuntimeException");
        } catch (RuntimeException e) {
            //expected
        }

        p.setDataPosition(0);
        p.readByteArray(b);
        for (int i = 0; i < a.length; i++) {
            assertEquals(a[i], b[i]);
        }
        p.recycle();

        // test write byte array with length: 9
        p = Parcel.obtain();
        p.writeByteArray(c);
        p.setDataPosition(0);
        try {
            p.readByteArray(b);
            fail("Should throw a RuntimeException");
        } catch (RuntimeException e) {
            //expected
        }

        p.setDataPosition(0);
        p.readByteArray(d);
        for (int i = 0; i < c.length; i++) {
            assertEquals(c[i], d[i]);
        }
        p.recycle();

        // Test array bounds checks (null already checked above).
        p = Parcel.obtain();
        try {
            p.writeByteArray(c, -1, 1); // Negative offset.
            fail();
        } catch (RuntimeException expected) {
        }
        try {
            p.writeByteArray(c, 0, -1); // Negative count.
            fail();
        } catch (RuntimeException expected) {
        }
        try {
            p.writeByteArray(c, c.length + 1, 1); // High offset.
            fail();
        } catch (RuntimeException expected) {
        }
        try {
            p.writeByteArray(c, 0, c.length + 1); // High count.
            fail();
        } catch (RuntimeException expected) {
        }
        p.recycle();
    }

    public void testWriteByteArray() {
        Parcel p;

        byte[] a = {(byte) 21};
        byte[] b = new byte[a.length];

        byte[] c = {Byte.MAX_VALUE, (byte) 111, (byte) 11, (byte) 1, (byte) 0,
                    (byte) -1, (byte) -11, (byte) -111, Byte.MIN_VALUE};
        byte[] d = new byte[c.length - 2];

        // test write null
        p = Parcel.obtain();
        p.writeByteArray(null, 0, 2);
        p.setDataPosition(0);
        try {
            p.readByteArray(null);
            fail("Should throw a RuntimeException");
        } catch (RuntimeException e) {
            //expected
        }

        p.setDataPosition(0);
        try {
            p.readByteArray(b);
            fail("Should throw a RuntimeException");
        } catch (RuntimeException e) {
            //expected
        }
        p.recycle();

        // test with wrong offset and length
        p = Parcel.obtain();
        try {
            p.writeByteArray(a, 0, 2);
            fail("Should throw a ArrayIndexOutOfBoundsException");
        } catch (ArrayIndexOutOfBoundsException e) {
            //expected
        }
        p.recycle();

        p = Parcel.obtain();
        try {
            p.writeByteArray(a, -1, 1);
            fail("Should throw a ArrayIndexOutOfBoundsException");
        } catch (ArrayIndexOutOfBoundsException e) {
            //expected
        }
        p.recycle();

        p = Parcel.obtain();
        try {
            p.writeByteArray(a, 0, -1);
            fail("Should throw a ArrayIndexOutOfBoundsException");
        } catch (ArrayIndexOutOfBoundsException e) {
            //expected
        }
        p.recycle();

        // test write byte array with length: 1
        p = Parcel.obtain();
        p.writeByteArray(a, 0 , 1);
        p.setDataPosition(0);
        try {
            p.readByteArray(d);
            fail("Should throw a RuntimeException");
        } catch (RuntimeException e) {
            //expected
        }

        p.setDataPosition(0);
        p.readByteArray(b);
        for (int i = 0; i < a.length; i++) {
            assertEquals(a[i], b[i]);
        }
        p.recycle();

        // test write byte array with offset: 1, length: 7
        p = Parcel.obtain();
        p.writeByteArray(c, 1, 7);
        p.setDataPosition(0);
        try {
            p.readByteArray(b);
            fail("Should throw a RuntimeException");
        } catch (RuntimeException e) {
            //expected
        }

        d = new byte[c.length - 2];
        p.setDataPosition(0);
        p.readByteArray(d);
        for (int i = 0; i < d.length; i++) {
            Log.d("Trace", "i=" + i + " d[i]=" + d[i]);
        }
        for (int i = 0; i < 7; i++) {
            assertEquals(c[i + 1], d[i]);
        }
        p.recycle();
    }

    public void testCreateByteArray() {
        Parcel p;

        byte[] a = {(byte) 21};
        byte[] b;

        byte[] c = {Byte.MAX_VALUE, (byte) 111, (byte) 11, (byte) 1, (byte) 0,
                    (byte) -1, (byte) -11, (byte) -111, Byte.MIN_VALUE};
        byte[] d;

        byte[] e = {};
        byte[] f;

        // test write null
        p = Parcel.obtain();
        p.writeByteArray(null);
        p.setDataPosition(0);
        b = p.createByteArray();
        assertNull(b);
        p.recycle();

        // test write byte array with length: 0
        p = Parcel.obtain();
        p.writeByteArray(e);
        p.setDataPosition(0);
        f = p.createByteArray();
        assertNotNull(f);
        assertEquals(0, f.length);
        p.recycle();

        // test write byte array with length: 1
        p = Parcel.obtain();
        p.writeByteArray(a);
        p.setDataPosition(0);
        b = p.createByteArray();
        assertNotNull(b);
        for (int i = 0; i < a.length; i++) {
            assertEquals(a[i], b[i]);
        }
        p.recycle();

        // test write byte array with length: 9
        p = Parcel.obtain();
        p.writeByteArray(c);
        p.setDataPosition(0);
        d = p.createByteArray();
        assertNotNull(d);
        for (int i = 0; i < c.length; i++) {
            assertEquals(c[i], d[i]);
        }
        p.recycle();
    }

    public void testReadCharArray() {
        Parcel p;

        char[] a = {'a'};
        char[] b = new char[a.length];

        char[] c = {'a', Character.MAX_VALUE, Character.MIN_VALUE, Character.MAX_SURROGATE, Character.MIN_SURROGATE,
                    Character.MAX_HIGH_SURROGATE, Character.MAX_LOW_SURROGATE,
                    Character.MIN_HIGH_SURROGATE, Character.MIN_LOW_SURROGATE};
        char[] d = new char[c.length];

        // test write null
        p = Parcel.obtain();
        p.writeCharArray(null);
        p.setDataPosition(0);
        try {
            p.readCharArray(null);
            fail("Should throw a RuntimeException");
        } catch (RuntimeException e) {
            //expected
        }

        p.setDataPosition(0);
        try {
            p.readCharArray(b);
            fail("Should throw a RuntimeException");
        } catch (RuntimeException e) {
            //expected
        }
        p.recycle();

        // test write char array with length: 1
        p = Parcel.obtain();
        p.writeCharArray(a);
        p.setDataPosition(0);
        try {
            p.readCharArray(d);
            fail("Should throw a RuntimeException");
        } catch (RuntimeException e) {
            //expected
        }

        p.setDataPosition(0);
        p.readCharArray(b);
        for (int i = 0; i < a.length; i++) {
            assertEquals(a[i], b[i]);
        }
        p.recycle();

        // test write char array with length: 9
        p = Parcel.obtain();
        p.writeCharArray(c);
        p.setDataPosition(0);
        try {
            p.readCharArray(b);
            fail("Should throw a RuntimeException");
        } catch (RuntimeException e) {
            //expected
        }

        p.setDataPosition(0);
        p.readCharArray(d);
        for (int i = 0; i < c.length; i++) {
            assertEquals(c[i], d[i]);
        }
        p.recycle();
    }

    public void testCreateCharArray() {
        Parcel p;

        char[] a = {'a'};
        char[] b;

        char[] c = {'a', Character.MAX_VALUE, Character.MIN_VALUE, Character.MAX_SURROGATE, Character.MIN_SURROGATE,
                    Character.MAX_HIGH_SURROGATE, Character.MAX_LOW_SURROGATE,
                    Character.MIN_HIGH_SURROGATE, Character.MIN_LOW_SURROGATE};
        char[] d;

        char[] e = {};
        char[] f;

        // test write null
        p = Parcel.obtain();
        p.writeCharArray(null);
        p.setDataPosition(0);
        b = p.createCharArray();
        assertNull(b);
        p.recycle();

        // test write char array with length: 1
        p = Parcel.obtain();
        p.writeCharArray(e);
        p.setDataPosition(0);
        f = p.createCharArray();
        assertNotNull(e);
        assertEquals(0, f.length);
        p.recycle();

        // test write char array with length: 1
        p = Parcel.obtain();
        p.writeCharArray(a);
        p.setDataPosition(0);
        b = p.createCharArray();
        assertNotNull(b);
        for (int i = 0; i < a.length; i++) {
            assertEquals(a[i], b[i]);
        }
        p.recycle();

        // test write char array with length: 9
        p = Parcel.obtain();
        p.writeCharArray(c);
        p.setDataPosition(0);
        d = p.createCharArray();
        assertNotNull(d);
        for (int i = 0; i < c.length; i++) {
            assertEquals(c[i], d[i]);
        }
        p.recycle();
    }

    public void testReadInt() {
        Parcel p;

        p = Parcel.obtain();
        p.writeInt(0);
        p.setDataPosition(0);
        assertEquals(0, p.readInt());
        p.recycle();

        p = Parcel.obtain();
        p.writeInt(1);
        p.setDataPosition(0);
        assertEquals(1, p.readInt());
        p.recycle();

        p = Parcel.obtain();
        p.writeInt(-1);
        p.setDataPosition(0);
        assertEquals(-1, p.readInt());
        p.recycle();

        p = Parcel.obtain();
        p.writeInt(Integer.MAX_VALUE);
        p.setDataPosition(0);
        assertEquals(Integer.MAX_VALUE, p.readInt());
        p.recycle();

        p = Parcel.obtain();
        p.writeInt(Integer.MIN_VALUE);
        p.setDataPosition(0);
        assertEquals(Integer.MIN_VALUE, p.readInt());
        p.recycle();

        p = Parcel.obtain();
        p.writeInt(Integer.MAX_VALUE);
        p.writeInt(11);
        p.writeInt(1);
        p.writeInt(0);
        p.writeInt(-1);
        p.writeInt(-11);
        p.writeInt(Integer.MIN_VALUE);
        p.setDataPosition(0);
        assertEquals(Integer.MAX_VALUE, p.readInt());
        assertEquals(11, p.readInt());
        assertEquals(1, p.readInt());
        assertEquals(0, p.readInt());
        assertEquals(-1, p.readInt());
        assertEquals(-11, p.readInt());
        assertEquals(Integer.MIN_VALUE, p.readInt());
        p.recycle();
    }

    public void testReadIntArray() {
        Parcel p;

        int[] a = {21};
        int[] b = new int[a.length];

        int[] c = {Integer.MAX_VALUE, 111, 11, 1, 0, -1, -11, -111, Integer.MIN_VALUE};
        int[] d = new int[c.length];

        // test write null
        p = Parcel.obtain();
        p.writeIntArray(null);
        p.setDataPosition(0);
        try {
            p.readIntArray(null);
            fail("Should throw a RuntimeException");
        } catch (RuntimeException e) {
            //expected
        }

        p.setDataPosition(0);
        try {
            p.readIntArray(b);
            fail("Should throw a RuntimeException");
        } catch (RuntimeException e) {
            //expected
        }
        p.recycle();

        // test write int array with length: 1
        p = Parcel.obtain();
        p.writeIntArray(a);
        p.setDataPosition(0);
        try {
            p.readIntArray(d);
            fail("Should throw a RuntimeException");
        } catch (RuntimeException e) {
            //expected
        }

        p.setDataPosition(0);
        p.readIntArray(b);
        for (int i = 0; i < a.length; i++) {
            assertEquals(a[i], b[i]);
        }
        p.recycle();

        // test write int array with length: 9
        p = Parcel.obtain();
        p.writeIntArray(c);
        p.setDataPosition(0);
        try {
            p.readIntArray(b);
            fail("Should throw a RuntimeException");
        } catch (RuntimeException e) {
            //expected
        }

        p.setDataPosition(0);
        p.readIntArray(d);
        for (int i = 0; i < c.length; i++) {
            assertEquals(c[i], d[i]);
        }
        p.recycle();
    }

    public void testCreateIntArray() {
        Parcel p;

        int[] a = {21};
        int[] b;

        int[] c = {Integer.MAX_VALUE, 111, 11, 1, 0, -1, -11, -111, Integer.MIN_VALUE};
        int[] d;

        int[] e = {};
        int[] f;

        // test write null
        p = Parcel.obtain();
        p.writeIntArray(null);
        p.setDataPosition(0);
        b = p.createIntArray();
        assertNull(b);
        p.recycle();

        // test write int array with length: 0
        p = Parcel.obtain();
        p.writeIntArray(e);
        p.setDataPosition(0);
        f = p.createIntArray();
        assertNotNull(e);
        assertEquals(0, f.length);
        p.recycle();

        // test write int array with length: 1
        p = Parcel.obtain();
        p.writeIntArray(a);
        p.setDataPosition(0);
        b = p.createIntArray();
        assertNotNull(b);
        for (int i = 0; i < a.length; i++) {
            assertEquals(a[i], b[i]);
        }
        p.recycle();

        // test write int array with length: 9
        p = Parcel.obtain();
        p.writeIntArray(c);
        p.setDataPosition(0);
        d = p.createIntArray();
        assertNotNull(d);
        for (int i = 0; i < c.length; i++) {
            assertEquals(c[i], d[i]);
        }
        p.recycle();
    }

    public void testReadLong() {
        Parcel p;

        p = Parcel.obtain();
        p.writeLong(0L);
        p.setDataPosition(0);
        assertEquals(0, p.readLong());
        p.recycle();

        p = Parcel.obtain();
        p.writeLong(1L);
        p.setDataPosition(0);
        assertEquals(1, p.readLong());
        p.recycle();

        p = Parcel.obtain();
        p.writeLong(-1L);
        p.setDataPosition(0);
        assertEquals(-1L, p.readLong());
        p.recycle();

        p = Parcel.obtain();
        p.writeLong(Long.MAX_VALUE);
        p.writeLong(11L);
        p.writeLong(1L);
        p.writeLong(0L);
        p.writeLong(-1L);
        p.writeLong(-11L);
        p.writeLong(Long.MIN_VALUE);
        p.setDataPosition(0);
        assertEquals(Long.MAX_VALUE, p.readLong());
        assertEquals(11L, p.readLong());
        assertEquals(1L, p.readLong());
        assertEquals(0L, p.readLong());
        assertEquals(-1L, p.readLong());
        assertEquals(-11L, p.readLong());
        assertEquals(Long.MIN_VALUE, p.readLong());
        p.recycle();
    }

    public void testReadLongArray() {
        Parcel p;

        long[] a = {21L};
        long[] b = new long[a.length];

        long[] c = {Long.MAX_VALUE, 111L, 11L, 1L, 0L, -1L, -11L, -111L, Long.MIN_VALUE};
        long[] d = new long[c.length];

        // test write null
        p = Parcel.obtain();
        p.writeLongArray(null);
        p.setDataPosition(0);
        try {
            p.readLongArray(null);
            fail("Should throw a RuntimeException");
        } catch (RuntimeException e) {
            //expected
        }

        p.setDataPosition(0);
        try {
            p.readLongArray(b);
            fail("Should throw a RuntimeException");
        } catch (RuntimeException e) {
            //expected
        }
        p.recycle();

        // test write long array with length: 1
        p = Parcel.obtain();
        p.writeLongArray(a);
        p.setDataPosition(0);
        try {
            p.readLongArray(d);
            fail("Should throw a RuntimeException");
        } catch (RuntimeException e) {
            //expected
        }

        p.setDataPosition(0);
        p.readLongArray(b);
        for (int i = 0; i < a.length; i++) {
            assertEquals(a[i], b[i]);
        }
        p.recycle();

        // test write long array with length: 9
        p = Parcel.obtain();
        p.writeLongArray(c);
        p.setDataPosition(0);
        try {
            p.readLongArray(b);
            fail("Should throw a RuntimeException");
        } catch (RuntimeException e) {
            //expected
        }

        p.setDataPosition(0);
        p.readLongArray(d);
        for (int i = 0; i < c.length; i++) {
            assertEquals(c[i], d[i]);
        }
        p.recycle();
    }

    public void testCreateLongArray() {
        Parcel p;

        long[] a = {21L};
        long[] b;

        long[] c = {Long.MAX_VALUE, 111L, 11L, 1L, 0L, -1L, -11L, -111L, Long.MIN_VALUE};
        long[] d;

        long[] e = {};
        long[] f;

        // test write null
        p = Parcel.obtain();
        p.writeLongArray(null);
        p.setDataPosition(0);
        b = p.createLongArray();
        assertNull(b);
        p.recycle();

        // test write long array with length: 0
        p = Parcel.obtain();
        p.writeLongArray(e);
        p.setDataPosition(0);
        f = p.createLongArray();
        assertNotNull(e);
        assertEquals(0, f.length);
        p.recycle();

        // test write long array with length: 1
        p = Parcel.obtain();
        p.writeLongArray(a);
        p.setDataPosition(0);
        b = p.createLongArray();
        assertNotNull(b);
        for (int i = 0; i < a.length; i++) {
            assertEquals(a[i], b[i]);
        }
        p.recycle();

        // test write long array with length: 9
        p = Parcel.obtain();
        p.writeLongArray(c);
        p.setDataPosition(0);
        d = p.createLongArray();
        assertNotNull(d);
        for (int i = 0; i < c.length; i++) {
            assertEquals(c[i], d[i]);
        }
        p.recycle();
    }

    public void testReadFloat() {
        Parcel p;

        p = Parcel.obtain();
        p.writeFloat(.0f);
        p.setDataPosition(0);
        assertEquals(.0f, p.readFloat());
        p.recycle();

        p = Parcel.obtain();
        p.writeFloat(0.1f);
        p.setDataPosition(0);
        assertEquals(0.1f, p.readFloat());
        p.recycle();

        p = Parcel.obtain();
        p.writeFloat(-1.1f);
        p.setDataPosition(0);
        assertEquals(-1.1f, p.readFloat());
        p.recycle();

        p = Parcel.obtain();
        p.writeFloat(Float.MAX_VALUE);
        p.setDataPosition(0);
        assertEquals(Float.MAX_VALUE, p.readFloat());
        p.recycle();

        p = Parcel.obtain();
        p.writeFloat(Float.MIN_VALUE);
        p.setDataPosition(0);
        assertEquals(Float.MIN_VALUE, p.readFloat());
        p.recycle();

        p = Parcel.obtain();
        p.writeFloat(Float.MAX_VALUE);
        p.writeFloat(1.1f);
        p.writeFloat(0.1f);
        p.writeFloat(.0f);
        p.writeFloat(-0.1f);
        p.writeFloat(-1.1f);
        p.writeFloat(Float.MIN_VALUE);
        p.setDataPosition(0);
        assertEquals(Float.MAX_VALUE, p.readFloat());
        assertEquals(1.1f, p.readFloat());
        assertEquals(0.1f, p.readFloat());
        assertEquals(.0f, p.readFloat());
        assertEquals(-0.1f, p.readFloat());
        assertEquals(-1.1f, p.readFloat());
        assertEquals(Float.MIN_VALUE, p.readFloat());
        p.recycle();
    }

    public void testReadFloatArray() {
        Parcel p;

        float[] a = {2.1f};
        float[] b = new float[a.length];

        float[] c = {Float.MAX_VALUE, 11.1f, 1.1f, 0.1f, .0f, -0.1f, -1.1f, -11.1f, Float.MIN_VALUE};
        float[] d = new float[c.length];

        // test write null
        p = Parcel.obtain();
        p.writeFloatArray(null);
        p.setDataPosition(0);
        try {
            p.readFloatArray(null);
            fail("Should throw a RuntimeException");
        } catch (RuntimeException e) {
            //expected
        }

        p.setDataPosition(0);
        try {
            p.readFloatArray(b);
            fail("Should throw a RuntimeException");
        } catch (RuntimeException e) {
            //expected
        }
        p.recycle();

        // test write float array with length: 1
        p = Parcel.obtain();
        p.writeFloatArray(a);
        p.setDataPosition(0);
        try {
            p.readFloatArray(d);
            fail("Should throw a RuntimeException");
        } catch (RuntimeException e) {
            //expected
        }

        p.setDataPosition(0);
        p.readFloatArray(b);
        for (int i = 0; i < a.length; i++) {
            assertEquals(a[i], b[i]);
        }
        p.recycle();

        // test write float array with length: 9
        p = Parcel.obtain();
        p.writeFloatArray(c);
        p.setDataPosition(0);
        try {
            p.readFloatArray(b);
            fail("Should throw a RuntimeException");
        } catch (RuntimeException e) {
            //expected
        }

        p.setDataPosition(0);
        p.readFloatArray(d);
        for (int i = 0; i < c.length; i++) {
            assertEquals(c[i], d[i]);
        }
        p.recycle();
    }

    public void testCreateFloatArray() {
        Parcel p;

        float[] a = {2.1f};
        float[] b;

        float[] c = {Float.MAX_VALUE, 11.1f, 1.1f, 0.1f, .0f, -0.1f, -1.1f, -11.1f, Float.MIN_VALUE};
        float[] d;

        float[] e = {};
        float[] f;

        // test write null
        p = Parcel.obtain();
        p.writeFloatArray(null);
        p.setDataPosition(0);
        b = p.createFloatArray();
        assertNull(b);
        p.recycle();

        // test write float array with length: 0
        p = Parcel.obtain();
        p.writeFloatArray(e);
        p.setDataPosition(0);
        f = p.createFloatArray();
        assertNotNull(f);
        assertEquals(0, f.length);
        p.recycle();

        // test write float array with length: 1
        p = Parcel.obtain();
        p.writeFloatArray(a);
        p.setDataPosition(0);
        b = p.createFloatArray();
        assertNotNull(b);
        for (int i = 0; i < a.length; i++) {
            assertEquals(a[i], b[i]);
        }
        p.recycle();

        // test write float array with length: 9
        p = Parcel.obtain();
        p.writeFloatArray(c);
        p.setDataPosition(0);
        d = p.createFloatArray();
        assertNotNull(d);
        for (int i = 0; i < c.length; i++) {
            assertEquals(c[i], d[i]);
        }
        p.recycle();
    }

    public void testReadDouble() {
        Parcel p;

        p = Parcel.obtain();
        p.writeDouble(.0d);
        p.setDataPosition(0);
        assertEquals(.0d, p.readDouble());
        p.recycle();

        p = Parcel.obtain();
        p.writeDouble(0.1d);
        p.setDataPosition(0);
        assertEquals(0.1d, p.readDouble());
        p.recycle();

        p = Parcel.obtain();
        p.writeDouble(-1.1d);
        p.setDataPosition(0);
        assertEquals(-1.1d, p.readDouble());
        p.recycle();

        p = Parcel.obtain();
        p.writeDouble(Double.MAX_VALUE);
        p.setDataPosition(0);
        assertEquals(Double.MAX_VALUE, p.readDouble());
        p.recycle();

        p = Parcel.obtain();
        p.writeDouble(Double.MIN_VALUE);
        p.setDataPosition(0);
        assertEquals(Double.MIN_VALUE, p.readDouble());
        p.recycle();

        p = Parcel.obtain();
        p.writeDouble(Double.MAX_VALUE);
        p.writeDouble(1.1d);
        p.writeDouble(0.1d);
        p.writeDouble(.0d);
        p.writeDouble(-0.1d);
        p.writeDouble(-1.1d);
        p.writeDouble(Double.MIN_VALUE);
        p.setDataPosition(0);
        assertEquals(Double.MAX_VALUE, p.readDouble());
        assertEquals(1.1d, p.readDouble());
        assertEquals(0.1d, p.readDouble());
        assertEquals(.0d, p.readDouble());
        assertEquals(-0.1d, p.readDouble());
        assertEquals(-1.1d, p.readDouble());
        assertEquals(Double.MIN_VALUE, p.readDouble());
        p.recycle();
    }

    public void testReadDoubleArray() {
        Parcel p;

        double[] a = {2.1d};
        double[] b = new double[a.length];

        double[] c = {Double.MAX_VALUE, 11.1d, 1.1d, 0.1d, .0d, -0.1d, -1.1d, -11.1d, Double.MIN_VALUE};
        double[] d = new double[c.length];

        // test write null
        p = Parcel.obtain();
        p.writeDoubleArray(null);
        p.setDataPosition(0);
        try {
            p.readDoubleArray(null);
            fail("Should throw a RuntimeException");
        } catch (RuntimeException e) {
            //expected
        }

        p.setDataPosition(0);
        try {
            p.readDoubleArray(b);
            fail("Should throw a RuntimeException");
        } catch (RuntimeException e) {
            //expected
        }
        p.recycle();

        // test write double array with length: 1
        p = Parcel.obtain();
        p.writeDoubleArray(a);
        p.setDataPosition(0);
        try {
            p.readDoubleArray(d);
            fail("Should throw a RuntimeException");
        } catch (RuntimeException e) {
            //expected
        }

        p.setDataPosition(0);
        p.readDoubleArray(b);
        for (int i = 0; i < a.length; i++) {
            assertEquals(a[i], b[i]);
        }
        p.recycle();

        // test write double array with length: 9
        p = Parcel.obtain();
        p.writeDoubleArray(c);
        p.setDataPosition(0);
        try {
            p.readDoubleArray(b);
            fail("Should throw a RuntimeException");
        } catch (RuntimeException e) {
            //expected
        }

        p.setDataPosition(0);
        p.readDoubleArray(d);
        for (int i = 0; i < c.length; i++) {
            assertEquals(c[i], d[i]);
        }
        p.recycle();
    }

    public void testCreateDoubleArray() {
        Parcel p;

        double[] a = {2.1d};
        double[] b;

        double[] c = {
                Double.MAX_VALUE, 11.1d, 1.1d, 0.1d, .0d, -0.1d, -1.1d, -11.1d, Double.MIN_VALUE
        };
        double[] d;

        double[] e = {};
        double[] f;

        // test write null
        p = Parcel.obtain();
        p.writeDoubleArray(null);
        p.setDataPosition(0);
        b = p.createDoubleArray();
        assertNull(b);
        p.recycle();

        // test write double array with length: 0
        p = Parcel.obtain();
        p.writeDoubleArray(e);
        p.setDataPosition(0);
        f = p.createDoubleArray();
        assertNotNull(f);
        assertEquals(0, f.length);
        p.recycle();

        // test write double array with length: 1
        p = Parcel.obtain();
        p.writeDoubleArray(a);
        p.setDataPosition(0);
        b = p.createDoubleArray();
        assertNotNull(b);
        for (int i = 0; i < a.length; i++) {
            assertEquals(a[i], b[i]);
        }
        p.recycle();

        // test write double array with length: 9
        p = Parcel.obtain();
        p.writeDoubleArray(c);
        p.setDataPosition(0);
        d = p.createDoubleArray();
        assertNotNull(d);
        for (int i = 0; i < c.length; i++) {
            assertEquals(c[i], d[i]);
        }
        p.recycle();
    }

    public void testReadBooleanArray() {
        Parcel p;

        boolean[] a = {true};
        boolean[] b = new boolean[a.length];

        boolean[] c = {true, false, true, false};
        boolean[] d = new boolean[c.length];

        // test write null
        p = Parcel.obtain();
        p.writeBooleanArray(null);
        p.setDataPosition(0);
        try {
            p.readIntArray(null);
            fail("Should throw a RuntimeException");
        } catch (RuntimeException e) {
            //expected
        }

        p.setDataPosition(0);
        try {
            p.readBooleanArray(b);
            fail("Should throw a RuntimeException");
        } catch (RuntimeException e) {
            //expected
        }
        p.recycle();

        // test write boolean array with length: 1
        p = Parcel.obtain();
        p.writeBooleanArray(a);
        p.setDataPosition(0);
        try {
            p.readBooleanArray(d);
            fail("Should throw a RuntimeException");
        } catch (RuntimeException e) {
            //expected
        }

        p.setDataPosition(0);
        p.readBooleanArray(b);
        for (int i = 0; i < a.length; i++) {
            assertEquals(a[i], b[i]);
        }
        p.recycle();

        // test write boolean array with length: 4
        p = Parcel.obtain();
        p.writeBooleanArray(c);
        p.setDataPosition(0);
        try {
            p.readBooleanArray(b);
            fail("Should throw a RuntimeException");
        } catch (RuntimeException e) {
            //expected
        }

        p.setDataPosition(0);
        p.readBooleanArray(d);
        for (int i = 0; i < c.length; i++) {
            assertEquals(c[i], d[i]);
        }
        p.recycle();
    }

    public void testCreateBooleanArray() {
        Parcel p;

        boolean[] a = {true};
        boolean[] b;

        boolean[] c = {true, false, true, false};
        boolean[] d;

        boolean[] e = {};
        boolean[] f;

        // test write null
        p = Parcel.obtain();
        p.writeBooleanArray(null);
        p.setDataPosition(0);
        b = p.createBooleanArray();
        assertNull(b);
        p.recycle();

        // test write boolean array with length: 0
        p = Parcel.obtain();
        p.writeBooleanArray(e);
        p.setDataPosition(0);
        f = p.createBooleanArray();
        assertNotNull(f);
        assertEquals(0, f.length);
        p.recycle();

        // test write boolean array with length: 1
        p = Parcel.obtain();
        p.writeBooleanArray(a);

        p.setDataPosition(0);
        b = p.createBooleanArray();
        assertNotNull(b);
        for (int i = 0; i < a.length; i++) {
            assertEquals(a[i], b[i]);
        }
        p.recycle();

        // test write boolean array with length: 4
        p = Parcel.obtain();
        p.writeBooleanArray(c);
        p.setDataPosition(0);
        d = p.createBooleanArray();
        assertNotNull(d);
        for (int i = 0; i < c.length; i++) {
            assertEquals(c[i], d[i]);
        }
        p.recycle();
    }

    public void testReadString() {
        Parcel p;
        final String string = "Hello, Android!";

        // test write null
        p = Parcel.obtain();
        p.writeString(null);
        p.setDataPosition(0);
        assertNull(p.readString());
        p.recycle();

        p = Parcel.obtain();
        p.writeString("");
        p.setDataPosition(0);
        assertEquals("", p.readString());
        p.recycle();

        p = Parcel.obtain();
        p.writeString("a");
        p.setDataPosition(0);
        assertEquals("a", p.readString());
        p.recycle();

        p = Parcel.obtain();
        p.writeString(string);
        p.setDataPosition(0);
        assertEquals(string, p.readString());
        p.recycle();

        p = Parcel.obtain();
        p.writeString(string);
        p.writeString("a");
        p.writeString("");
        p.setDataPosition(0);
        assertEquals(string, p.readString());
        assertEquals("a", p.readString());
        assertEquals("", p.readString());
        p.recycle();
    }

    public void testReadStringArray() {
        Parcel p;

        String[] a = {"21"};
        String[] b = new String[a.length];

        String[] c = {"",
                "a",
                "Hello, Android!",
                "A long string that is used to test the api readStringArray(),"};
        String[] d = new String[c.length];

        // test write null
        p = Parcel.obtain();
        p.writeStringArray(null);
        p.setDataPosition(0);
        try {
            p.readStringArray(null);
            fail("Should throw a RuntimeException");
        } catch (RuntimeException e) {
            //expected
        }

        p.setDataPosition(0);
        try {
            p.readStringArray(b);
            fail("Should throw a RuntimeException");
        } catch (RuntimeException e) {
            //expected
        }
        p.recycle();

        // test write String array with length: 1
        p = Parcel.obtain();
        p.writeStringArray(a);
        p.setDataPosition(0);
        try {
            p.readStringArray(d);
            fail("Should throw a RuntimeException");
        } catch (RuntimeException e) {
            //expected
        }

        p.setDataPosition(0);
        p.readStringArray(b);
        for (int i = 0; i < a.length; i++) {
            assertEquals(a[i], b[i]);
        }
        p.recycle();

        // test write String array with length: 9
        p = Parcel.obtain();
        p.writeStringArray(c);
        p.setDataPosition(0);
        try {
            p.readStringArray(b);
            fail("Should throw a RuntimeException");
        } catch (RuntimeException e) {
            //expected
        }

        p.setDataPosition(0);
        p.readStringArray(d);
        for (int i = 0; i < c.length; i++) {
            assertEquals(c[i], d[i]);
        }
        p.recycle();
    }

    public void testCreateStringArray() {
        Parcel p;

        String[] a = {"21"};
        String[] b;

        String[] c = {"",
                "a",
                "Hello, Android!",
                "A long string that is used to test the api readStringArray(),"};
        String[] d;

        String[] e = {};
        String[] f;

        // test write null
        p = Parcel.obtain();
        p.writeStringArray(null);
        p.setDataPosition(0);
        b = p.createStringArray();
        assertNull(b);
        p.recycle();

        // test write String array with length: 0
        p = Parcel.obtain();
        p.writeStringArray(e);
        p.setDataPosition(0);
        f = p.createStringArray();
        assertNotNull(e);
        assertEquals(0, f.length);
        p.recycle();

        // test write String array with length: 1
        p = Parcel.obtain();
        p.writeStringArray(a);
        p.setDataPosition(0);
        b = p.createStringArray();
        assertNotNull(b);
        for (int i = 0; i < a.length; i++) {
            assertEquals(a[i], b[i]);
        }
        p.recycle();

        // test write String array with length: 9
        p = Parcel.obtain();
        p.writeStringArray(c);
        p.setDataPosition(0);
        d = p.createStringArray();
        assertNotNull(d);
        for (int i = 0; i < c.length; i++) {
            assertEquals(c[i], d[i]);
        }
        p.recycle();
    }

    public void testReadStringList() {
        Parcel p;

        ArrayList<String> a = new ArrayList<String>();
        a.add("21");
        ArrayList<String> b = new ArrayList<String>();

        ArrayList<String> c = new ArrayList<String>();
        c.add("");
        c.add("a");
        c.add("Hello, Android!");
        c.add("A long string that is used to test the api readStringList(),");
        ArrayList<String> d = new ArrayList<String>();

        // test write null
        p = Parcel.obtain();
        p.writeStringList(null);
        p.setDataPosition(0);
        try {
            p.readStringList(null);
            fail("Should throw a RuntimeException");
        } catch (RuntimeException e) {
            //expected
        }

        p.setDataPosition(0);
        p.readStringList(b);
        assertTrue(0 == b.size());
        p.recycle();

        // test write String array with length: 1
        p = Parcel.obtain();
        p.writeStringList(a);
        p.setDataPosition(0);
        assertTrue(c.size() > a.size());
        p.readStringList(c);
        assertTrue(c.size() == a.size());
        assertEquals(a, c);

        p.setDataPosition(0);
        assertTrue(0 == b.size() && 0 != a.size());
        p.readStringList(b);
        assertEquals(a, b);
        p.recycle();

        c = new ArrayList<String>();
        c.add("");
        c.add("a");
        c.add("Hello, Android!");
        c.add("A long string that is used to test the api readStringList(),");
        // test write String array with length: 4
        p = Parcel.obtain();
        p.writeStringList(c);
        p.setDataPosition(0);

        assertTrue(b.size() < c.size());
        p.readStringList(b);
        assertTrue(b.size() == c.size());
        assertEquals(c, b);

        p.setDataPosition(0);
        assertTrue(d.size() < c.size());
        p.readStringList(d);
        assertEquals(c, d);
        p.recycle();
    }

    public void testCreateStringArrayList() {
        Parcel p;

        ArrayList<String> a = new ArrayList<String>();
        a.add("21");
        ArrayList<String> b;

        ArrayList<String> c = new ArrayList<String>();
        c.add("");
        c.add("a");
        c.add("Hello, Android!");
        c.add("A long string that is used to test the api readStringList(),");
        ArrayList<String> d;

        ArrayList<String> e = new ArrayList<String>();
        ArrayList<String> f = null;

        // test write null
        p = Parcel.obtain();
        p.writeStringList(null);
        p.setDataPosition(0);
        b = p.createStringArrayList();
        assertNull(b);
        p.recycle();

        // test write String array with length: 0
        p = Parcel.obtain();
        p.writeStringList(e);
        p.setDataPosition(0);
        assertNull(f);
        f = p.createStringArrayList();
        assertNotNull(f);
        p.recycle();

        // test write String array with length: 1
        p = Parcel.obtain();
        p.writeStringList(a);
        p.setDataPosition(0);
        b = p.createStringArrayList();
        assertEquals(a, b);
        p.recycle();

        // test write String array with length: 4
        p = Parcel.obtain();
        p.writeStringList(c);
        p.setDataPosition(0);
        d = p.createStringArrayList();
        assertEquals(c, d);
        p.recycle();
    }

    public void testReadSerializable() {
        Parcel p;

        // test write null
        p = Parcel.obtain();
        p.writeSerializable(null);
        p.setDataPosition(0);
        assertNull(p.readSerializable());
        p.recycle();

        p = Parcel.obtain();
        p.writeSerializable("Hello, Android!");
        p.setDataPosition(0);
        assertEquals("Hello, Android!", p.readSerializable());
        p.recycle();
    }

    public void testReadParcelable() {
        Parcel p;
        MockClassLoader mcl = new MockClassLoader();
        final String signatureString  = "1234567890abcdef";
        Signature s = new Signature(signatureString);

        // test write null
        p = Parcel.obtain();
        p.writeParcelable(null, 0);
        p.setDataPosition(0);
        assertNull(p.readParcelable(mcl));
        p.recycle();

        p = Parcel.obtain();
        p.writeParcelable(s, 0);
        p.setDataPosition(0);
        assertEquals(s, p.readParcelable(mcl));
        p.recycle();
    }

    public void testReadParcelableArray() {
        Parcel p;
        MockClassLoader mcl = new MockClassLoader();
        Signature[] s = {new Signature("1234"),
                new Signature("ABCD"),
                new Signature("abcd")};

        Signature[] s2 = {new Signature("1234"),
                null,
                new Signature("abcd")};
        Parcelable[] s3;

        // test write null
        p = Parcel.obtain();
        p.writeParcelableArray(null, 0);
        p.setDataPosition(0);
        assertNull(p.readParcelableArray(mcl));
        p.recycle();

        p = Parcel.obtain();
        p.writeParcelableArray(s, 0);
        p.setDataPosition(0);
        s3 = p.readParcelableArray(mcl);
        for (int i = 0; i < s.length; i++) {
            assertEquals(s[i], s3[i]);
        }
        p.recycle();

        p = Parcel.obtain();
        p.writeParcelableArray(s2, 0);
        p.setDataPosition(0);
        s3 = p.readParcelableArray(mcl);
        for (int i = 0; i < s2.length; i++) {
            assertEquals(s2[i], s3[i]);
        }
        p.recycle();
    }

    public void testReadTypedArray() {
        Parcel p;
        Signature[] s = {new Signature("1234"),
                new Signature("ABCD"),
                new Signature("abcd")};

        Signature[] s2 = {new Signature("1234"),
                null,
                new Signature("abcd")};
        Signature[] s3 = new Signature[3];
        Signature[] s4 = new Signature[4];

        // test write null
        p = Parcel.obtain();
        p.writeTypedArray(null, 0);
        p.setDataPosition(0);
        try {
            p.readTypedArray(s3, Signature.CREATOR);
            fail("should throw a RuntimeException");
        } catch (RuntimeException e) {
            //expected
        }

        p.setDataPosition(0);
        try {
            p.readTypedArray(null, Signature.CREATOR);
            fail("should throw a RuntimeException");
        } catch (RuntimeException e) {
            //expected
        }
        p.recycle();

        // test write not null
        p = Parcel.obtain();
        p.writeTypedArray(s, 0);
        p.setDataPosition(0);
        p.readTypedArray(s3, Signature.CREATOR);
        for (int i = 0; i < s.length; i++) {
            assertEquals(s[i], s3[i]);
        }

        p.setDataPosition(0);
        try {
            p.readTypedArray(null, Signature.CREATOR);
            fail("should throw a RuntimeException");
        } catch (RuntimeException e) {
            //expected
        }

        p.setDataPosition(0);
        try {
            p.readTypedArray(s4, Signature.CREATOR);
            fail("should throw a RuntimeException");
        } catch (RuntimeException e) {
            //expected
        }
        p.recycle();

        s3 = new Signature[s2.length];
        p = Parcel.obtain();
        p.writeTypedArray(s2, 0);
        p.setDataPosition(0);
        p.readTypedArray(s3, Signature.CREATOR);
        for (int i = 0; i < s.length; i++) {
            assertEquals(s2[i], s3[i]);
        }
        p.recycle();
    }

    public void testReadTypedArray2() {
        Parcel p;
        Signature[] s = {
                new Signature("1234"), new Signature("ABCD"), new Signature("abcd")
        };

        Signature[] s2 = {
                new Signature("1234"), null, new Signature("abcd")
        };
        Signature[] s3 = {
                null, null, null
        };

        // test write null
        p = Parcel.obtain();
        p.writeTypedArray(null, 0);
        p.setDataPosition(0);
        p.recycle();

        // test write not null
        p = Parcel.obtain();
        p.writeTypedArray(s, 0);
        p.setDataPosition(0);
        p.readTypedArray(s3, Signature.CREATOR);
        for (int i = 0; i < s.length; i++) {
            assertEquals(s[i], s3[i]);
        }
        p.recycle();

        p = Parcel.obtain();
        p.writeTypedArray(s2, 0);
        p.setDataPosition(0);
        p.readTypedArray(s3, Signature.CREATOR);
        for (int i = 0; i < s.length; i++) {
            assertEquals(s2[i], s3[i]);
        }
        p.recycle();
    }

    public void testCreateTypedArray() {
        Parcel p;
        Signature[] s = {new Signature("1234"),
                new Signature("ABCD"),
                new Signature("abcd")};

        Signature[] s2 = {new Signature("1234"),
                null,
                new Signature("abcd")};
        Signature[] s3;

        // test write null
        p = Parcel.obtain();
        p.writeTypedArray(null, 0);
        p.setDataPosition(0);
        assertNull(p.createTypedArray(Signature.CREATOR));
        p.recycle();

        // test write not null
        p = Parcel.obtain();
        p.writeTypedArray(s, 0);
        p.setDataPosition(0);
        s3 = p.createTypedArray(Signature.CREATOR);
        for (int i = 0; i < s.length; i++) {
            assertEquals(s[i], s3[i]);
        }
        p.recycle();

        p = Parcel.obtain();
        p.writeTypedArray(s2, 0);
        p.setDataPosition(0);
        s3 = p.createTypedArray(Signature.CREATOR);
        for (int i = 0; i < s.length; i++) {
            assertEquals(s2[i], s3[i]);
        }
        p.recycle();
    }

    public void testReadTypedList() {
        Parcel p;
        ArrayList<Signature> s = new ArrayList<Signature>();
        s.add(new Signature("1234"));
        s.add(new Signature("ABCD"));
        s.add(new Signature("abcd"));

        ArrayList<Signature> s2 = new ArrayList<Signature>();
        s2.add(new Signature("1234"));
        s2.add(null);

        ArrayList<Signature> s3 = new ArrayList<Signature>();

        // test write null
        p = Parcel.obtain();
        p.writeTypedList(null);
        p.setDataPosition(0);
        p.readTypedList(s3, Signature.CREATOR);
        assertEquals(0, s3.size());

        p.setDataPosition(0);
        try {
            p.readTypedList(null, Signature.CREATOR);
            fail("should throw a RuntimeException");
        } catch (RuntimeException e) {
            //expected
        }
        p.recycle();

        // test write not null
        p = Parcel.obtain();
        p.writeTypedList(s);
        p.setDataPosition(0);
        p.readTypedList(s3, Signature.CREATOR);
        for (int i = 0; i < s.size(); i++) {
            assertEquals(s.get(i), s3.get(i));
        }

        p.setDataPosition(0);
        try {
            p.readTypedList(null, Signature.CREATOR);
            fail("should throw a RuntimeException");
        } catch (RuntimeException e) {
            //expected
        }

        p.setDataPosition(0);
        p.readTypedList(s2, Signature.CREATOR);
        assertEquals(s.size(), s2.size());
        for (int i = 0; i < s.size(); i++) {
            assertEquals(s.get(i), s2.get(i));
        }
        p.recycle();

        s2 = new ArrayList<Signature>();
        s2.add(new Signature("1234"));
        s2.add(null);
        p = Parcel.obtain();
        p.writeTypedList(s2);
        p.setDataPosition(0);
        p.readTypedList(s3, Signature.CREATOR);
        assertEquals(s3.size(), s2.size());
        for (int i = 0; i < s2.size(); i++) {
            assertEquals(s2.get(i), s3.get(i));
        }
        p.recycle();
    }

    public void testCreateTypedArrayList() {
        Parcel p;
        ArrayList<Signature> s = new ArrayList<Signature>();
        s.add(new Signature("1234"));
        s.add(new Signature("ABCD"));
        s.add(new Signature("abcd"));

        ArrayList<Signature> s2 = new ArrayList<Signature>();
        s2.add(new Signature("1234"));
        s2.add(null);

        ArrayList<Signature> s3;

        // test write null
        p = Parcel.obtain();
        p.writeTypedList(null);
        p.setDataPosition(0);
        assertNull(p.createTypedArrayList(Signature.CREATOR));
        p.recycle();

        // test write not null
        p = Parcel.obtain();
        p.writeTypedList(s);
        p.setDataPosition(0);
        s3 = p.createTypedArrayList(Signature.CREATOR);
        for (int i = 0; i < s.size(); i++) {
            assertEquals(s.get(i), s3.get(i));
        }

        p = Parcel.obtain();
        p.writeTypedList(s2);
        p.setDataPosition(0);
        s3 = p.createTypedArrayList(Signature.CREATOR);
        assertEquals(s3.size(), s2.size());
        for (int i = 0; i < s2.size(); i++) {
            assertEquals(s2.get(i), s3.get(i));
        }
        p.recycle();
    }

    public void testReadException() {
    }

    public void testReadException2() {
        Parcel p = Parcel.obtain();
        String msg = "testReadException2";

        p.writeException(new SecurityException(msg));
        p.setDataPosition(0);
        try {
            p.readException();
            fail("Should throw a SecurityException");
        } catch (SecurityException e) {
            assertEquals(msg, e.getMessage());
        }

        p.setDataPosition(0);
        p.writeException(new BadParcelableException(msg));
        p.setDataPosition(0);
        try {
            p.readException();
            fail("Should throw a BadParcelableException");
        } catch (BadParcelableException e) {
            assertEquals(msg, e.getMessage());
        }

        p.setDataPosition(0);
        p.writeException(new IllegalArgumentException(msg));
        p.setDataPosition(0);
        try {
            p.readException();
            fail("Should throw an IllegalArgumentException");
        } catch (IllegalArgumentException e) {
            assertEquals(msg, e.getMessage());
        }

        p.setDataPosition(0);
        p.writeException(new NullPointerException(msg));
        p.setDataPosition(0);
        try {
            p.readException();
            fail("Should throw a NullPointerException");
        } catch (NullPointerException e) {
            assertEquals(msg, e.getMessage());
        }

        p.setDataPosition(0);
        p.writeException(new IllegalStateException(msg));
        p.setDataPosition(0);
        try {
            p.readException();
            fail("Should throw an IllegalStateException");
        } catch (IllegalStateException e) {
            assertEquals(msg, e.getMessage());
        }

        p.setDataPosition(0);
        try {
            p.writeException(new RuntimeException());
            fail("Should throw an IllegalStateException");
        } catch (RuntimeException e) {
            //expected
        }
        p.recycle();
    }

    public void testWriteNoException() {
        Parcel p = Parcel.obtain();
        p.writeNoException();
        p.setDataPosition(0);
        p.readException();
        p.recycle();
    }

    public void testWriteFileDescriptor() {
        Parcel p;
        FileDescriptor fIn = FileDescriptor.in;
        ParcelFileDescriptor pfd;

        p = Parcel.obtain();
        pfd = p.readFileDescriptor();
        assertNull(pfd);
        p.recycle();

        p = Parcel.obtain();
        p.writeFileDescriptor(fIn);
        p.setDataPosition(0);
        pfd = p.readFileDescriptor();
        assertNotNull(pfd);
        assertNotNull(pfd.getFileDescriptor());
        p.recycle();
    }

    public void testHasFileDescriptor() {
        Parcel p;
        FileDescriptor fIn = FileDescriptor.in;

        p = Parcel.obtain();
        p.writeFileDescriptor(fIn);
        p.setDataPosition(0);
        assertTrue(p.hasFileDescriptors());
        p.recycle();

        p = Parcel.obtain();
        p.writeInt(111);
        p.setDataPosition(0);
        assertFalse(p.hasFileDescriptors());
        p.recycle();
    }

    public void testReadBundle() {
        Bundle bundle = new Bundle();
        bundle.putBoolean("boolean", true);
        bundle.putInt("int", Integer.MAX_VALUE);
        bundle.putString("string", "String");

        Bundle bundle2;
        Parcel p;

        // test null
        p = Parcel.obtain();
        p.writeBundle(null);
        p.setDataPosition(0);
        bundle2 = p.readBundle();
        assertNull(bundle2);
        p.recycle();

        // test not null
        bundle2 = null;
        p = Parcel.obtain();
        p.writeBundle(bundle);
        p.setDataPosition(0);
        bundle2 = p.readBundle();
        assertNotNull(bundle2);
        assertEquals(true, bundle2.getBoolean("boolean"));
        assertEquals(Integer.MAX_VALUE, bundle2.getInt("int"));
        assertEquals("String", bundle2.getString("string"));
        p.recycle();

        bundle2 = null;
        Parcel a = Parcel.obtain();
        bundle2 = new Bundle();
        bundle2.putString("foo", "test");
        a.writeBundle(bundle2);
        a.setDataPosition(0);
        bundle.readFromParcel(a);
        p = Parcel.obtain();
        p.setDataPosition(0);
        p.writeBundle(bundle);
        p.setDataPosition(0);
        bundle2 = p.readBundle();
        assertNotNull(bundle2);
        assertFalse(true == bundle2.getBoolean("boolean"));
        assertFalse(Integer.MAX_VALUE == bundle2.getInt("int"));
        assertFalse("String".equals( bundle2.getString("string")));
        a.recycle();
        p.recycle();
    }

    public void testReadBundle2() {
        Bundle b = new Bundle();
        b.putBoolean("boolean", true);
        b.putInt("int", Integer.MAX_VALUE);
        b.putString("string", "String");

        Bundle u;
        Parcel p;
        MockClassLoader m = new MockClassLoader();

        p = Parcel.obtain();
        p.writeBundle(null);
        p.setDataPosition(0);
        u = p.readBundle(m);
        assertNull(u);
        p.recycle();

        u = null;
        p = Parcel.obtain();
        p.writeBundle(b);
        p.setDataPosition(0);
        u = p.readBundle(m);
        assertNotNull(u);
        assertEquals(true, b.getBoolean("boolean"));
        assertEquals(Integer.MAX_VALUE, b.getInt("int"));
        assertEquals("String", b.getString("string"));
        p.recycle();
    }

    public void testWriteArray() {
        Parcel p;
        MockClassLoader mcl = new MockClassLoader();

        p = Parcel.obtain();
        p.writeArray(null);
        p.setDataPosition(0);
        assertNull(p.readArray(mcl));
        p.recycle();

        Object[] objects = new Object[5];
        objects[0] = Integer.MAX_VALUE;
        objects[1] = true;
        objects[2] = Long.MAX_VALUE;
        objects[3] = "String";
        objects[4] = Float.MAX_VALUE;
        Object[] objects2;

        p = Parcel.obtain();
        p.writeArray(objects);
        p.setDataPosition(0);
        objects2 = p.readArray(mcl);
        assertNotNull(objects2);
        for (int i = 0; i < objects2.length; i++) {
            assertEquals(objects[i], objects2[i]);
        }
        p.recycle();
    }

    public void testReadArrayList() {
        Parcel p;
        MockClassLoader mcl = new MockClassLoader();

        p = Parcel.obtain();
        p.writeArray(null);
        p.setDataPosition(0);
        assertNull(p.readArrayList(mcl));
        p.recycle();

        Object[] objects = new Object[5];
        objects[0] = Integer.MAX_VALUE;
        objects[1] = true;
        objects[2] = Long.MAX_VALUE;
        objects[3] = "String";
        objects[4] = Float.MAX_VALUE;
        ArrayList<?> objects2;

        p = Parcel.obtain();
        p.writeArray(objects);
        p.setDataPosition(0);
        objects2 = p.readArrayList(mcl);
        assertNotNull(objects2);
        for (int i = 0; i < objects2.size(); i++) {
            assertEquals(objects[i], objects2.get(i));
        }
        p.recycle();
    }

    @SuppressWarnings("unchecked")
    public void testWriteSparseArray() {
        Parcel p;
        MockClassLoader mcl = new MockClassLoader();

        p = Parcel.obtain();
        p.writeSparseArray(null);
        p.setDataPosition(0);
        assertNull(p.readSparseArray(mcl));
        p.recycle();

        SparseArray<Object> sparseArray = new SparseArray<Object>();
        sparseArray.put(3, "String");
        sparseArray.put(2, Long.MAX_VALUE);
        sparseArray.put(4, Float.MAX_VALUE);
        sparseArray.put(0, Integer.MAX_VALUE);
        sparseArray.put(1, true);
        sparseArray.put(10, true);
        SparseArray<Object> sparseArray2;

        p = Parcel.obtain();
        p.writeSparseArray(sparseArray);
        p.setDataPosition(0);
        sparseArray2 = p.readSparseArray(mcl);
        assertNotNull(sparseArray2);
        assertEquals(sparseArray.size(), sparseArray2.size());
        assertEquals(sparseArray.get(0), sparseArray2.get(0));
        assertEquals(sparseArray.get(1), sparseArray2.get(1));
        assertEquals(sparseArray.get(2), sparseArray2.get(2));
        assertEquals(sparseArray.get(3), sparseArray2.get(3));
        assertEquals(sparseArray.get(4), sparseArray2.get(4));
        assertEquals(sparseArray.get(10), sparseArray2.get(10));
        p.recycle();
    }

    public void testWriteSparseBooleanArray() {
        Parcel p;

        p = Parcel.obtain();
        p.writeSparseArray(null);
        p.setDataPosition(0);
        assertNull(p.readSparseBooleanArray());
        p.recycle();

        SparseBooleanArray sparseBooleanArray = new SparseBooleanArray();
        sparseBooleanArray.put(3, true);
        sparseBooleanArray.put(2, false);
        sparseBooleanArray.put(4, false);
        sparseBooleanArray.put(0, true);
        sparseBooleanArray.put(1, true);
        sparseBooleanArray.put(10, true);
        SparseBooleanArray sparseBoolanArray2;

        p = Parcel.obtain();
        p.writeSparseBooleanArray(sparseBooleanArray);
        p.setDataPosition(0);
        sparseBoolanArray2 = p.readSparseBooleanArray();
        assertNotNull(sparseBoolanArray2);
        assertEquals(sparseBooleanArray.size(), sparseBoolanArray2.size());
        assertEquals(sparseBooleanArray.get(0), sparseBoolanArray2.get(0));
        assertEquals(sparseBooleanArray.get(1), sparseBoolanArray2.get(1));
        assertEquals(sparseBooleanArray.get(2), sparseBoolanArray2.get(2));
        assertEquals(sparseBooleanArray.get(3), sparseBoolanArray2.get(3));
        assertEquals(sparseBooleanArray.get(4), sparseBoolanArray2.get(4));
        assertEquals(sparseBooleanArray.get(10), sparseBoolanArray2.get(10));
        p.recycle();
    }

    public void testWriteStrongBinder() {
        Parcel p;
        Binder binder;
        Binder binder2 = new Binder();

        p = Parcel.obtain();
        p.writeStrongBinder(null);
        p.setDataPosition(0);
        assertNull(p.readStrongBinder());
        p.recycle();

        p = Parcel.obtain();
        p.writeStrongBinder(binder2);
        p.setDataPosition(0);
        binder = (Binder) p.readStrongBinder();
        assertEquals(binder2, binder);
        p.recycle();
    }

    public void testWriteStrongInterface() {
        Parcel p;
        MockIInterface mockInterface = new MockIInterface();
        MockIInterface mockIInterface2 = new MockIInterface();

        p = Parcel.obtain();
        p.writeStrongInterface(null);
        p.setDataPosition(0);
        assertNull(p.readStrongBinder());
        p.recycle();

        p = Parcel.obtain();
        p.writeStrongInterface(mockInterface);
        p.setDataPosition(0);
        mockIInterface2.binder = (Binder) p.readStrongBinder();
        assertEquals(mockInterface.binder, mockIInterface2.binder);
        p.recycle();
    }

    public void testWriteBinderArray() {
        Parcel p;
        IBinder[] ibinder2 = {new Binder(), new Binder()};
        IBinder[] ibinder3 = new IBinder[2];
        IBinder[] ibinder4 = new IBinder[3];

        p = Parcel.obtain();
        p.writeBinderArray(null);
        p.setDataPosition(0);
        try {
            p.readBinderArray(null);
            fail("Should throw a RuntimeException");
        } catch (RuntimeException e) {
            //expected
        }

        p.setDataPosition(0);
        try {
            p.readBinderArray(ibinder3);
            fail("Should throw a RuntimeException");
        } catch (RuntimeException e) {
            //expected
        }

        p.setDataPosition(0);
        try {
            p.readBinderArray(ibinder2);
            fail("Should throw a RuntimeException");
        } catch (RuntimeException e) {
            //expected
        }
        p.recycle();

        p = Parcel.obtain();
        p.writeBinderArray(ibinder2);
        p.setDataPosition(0);
        try {
            p.readBinderArray(null);
            fail("Should throw a RuntimeException");
        } catch (RuntimeException e) {
            //expected
        }

        p.setDataPosition(0);
        try {
            p.readBinderArray(ibinder4);
            fail("Should throw a RuntimeException");
        } catch (RuntimeException e) {
            //expected
        }

        p.setDataPosition(0);
        p.readBinderArray(ibinder3);
        assertNotNull(ibinder3);
        for (int i = 0; i < ibinder3.length; i++) {
            assertNotNull(ibinder3[i]);
            assertEquals(ibinder2[i], ibinder3[i]);
        }
        p.recycle();
    }

    public void testCreateBinderArray() {
        Parcel p;
        IBinder[] ibinder  = {};
        IBinder[] ibinder2 = {new Binder(), new Binder()};
        IBinder[] ibinder3;
        IBinder[] ibinder4;

        p = Parcel.obtain();
        p.writeBinderArray(null);
        p.setDataPosition(0);
        ibinder3 = p.createBinderArray();
        assertNull(ibinder3);
        p.recycle();

        p = Parcel.obtain();
        p.writeBinderArray(ibinder);
        p.setDataPosition(0);
        ibinder4 = p.createBinderArray();
        assertNotNull(ibinder4);
        assertEquals(0, ibinder4.length);
        p.recycle();

        p = Parcel.obtain();
        p.writeBinderArray(ibinder2);
        p.setDataPosition(0);
        ibinder3 = p.createBinderArray();
        assertNotNull(ibinder3);
        for (int i = 0; i < ibinder3.length; i++) {
            assertNotNull(ibinder3[i]);
            assertEquals(ibinder2[i], ibinder3[i]);
        }
        p.recycle();
    }

    public void testWriteBinderList() {
        Parcel p;
        ArrayList<IBinder> arrayList = new ArrayList<IBinder>();
        ArrayList<IBinder> arrayList2 = new ArrayList<IBinder>();
        arrayList2.add(new Binder());
        arrayList2.add(new Binder());
        ArrayList<IBinder> arrayList3 = new ArrayList<IBinder>();
        arrayList3.add(new Binder());
        arrayList3.add(new Binder());
        arrayList3.add(new Binder());

        p = Parcel.obtain();
        p.writeBinderList(null);
        p.setDataPosition(0);
        try {
            p.readBinderList(null);
            fail("Should throw a RuntimeException");
        } catch (RuntimeException e) {
            //expected
        }
        p.setDataPosition(0);
        assertEquals(0, arrayList.size());
        p.readBinderList(arrayList);
        assertEquals(0, arrayList.size());
        p.recycle();

        p = Parcel.obtain();
        p.writeBinderList(arrayList2);
        p.setDataPosition(0);
        assertEquals(0, arrayList.size());
        p.readBinderList(arrayList);
        assertEquals(2, arrayList.size());
        assertEquals(arrayList2, arrayList);
        p.recycle();

        p = Parcel.obtain();
        p.writeBinderList(arrayList2);
        p.setDataPosition(0);
        assertEquals(3, arrayList3.size());
        p.readBinderList(arrayList3);
        assertEquals(2, arrayList3.size());
        assertEquals(arrayList2, arrayList3);
        p.recycle();
    }

    public void testCreateBinderArrayList() {
        Parcel p;
        ArrayList<IBinder> arrayList = new ArrayList<IBinder>();
        ArrayList<IBinder> arrayList2 = new ArrayList<IBinder>();
        arrayList2.add(new Binder());
        arrayList2.add(new Binder());
        ArrayList<IBinder> arrayList3;
        ArrayList<IBinder> arrayList4;

        p = Parcel.obtain();
        p.writeBinderList(null);
        p.setDataPosition(0);
        arrayList3 = p.createBinderArrayList();
        assertNull(arrayList3);
        p.recycle();

        p = Parcel.obtain();
        p.writeBinderList(arrayList);
        p.setDataPosition(0);
        arrayList3 = p.createBinderArrayList();
        assertNotNull(arrayList3);
        assertEquals(0, arrayList3.size());
        p.recycle();

        p = Parcel.obtain();
        p.writeBinderList(arrayList2);
        p.setDataPosition(0);
        arrayList4 = p.createBinderArrayList();
        assertNotNull(arrayList4);
        assertEquals(arrayList2, arrayList4);
        p.recycle();
    }

    @SuppressWarnings("unchecked")
    public void testWriteMap() {
        Parcel p;
        MockClassLoader mcl = new MockClassLoader();
        HashMap map = new HashMap();
        HashMap map2 = new HashMap();

        p = Parcel.obtain();
        p.writeMap(null);
        p.setDataPosition(0);
        assertEquals(0, map2.size());
        p.readMap(map2, mcl);
        assertEquals(0, map2.size());
        p.recycle();

        map.put("string", "String");
        map.put("int", Integer.MAX_VALUE);
        map.put("boolean", true);
        p = Parcel.obtain();
        p.writeMap(map);
        p.setDataPosition(0);
        assertEquals(0, map2.size());
        p.readMap(map2, mcl);
        assertEquals(3, map2.size());
        assertEquals("String", map.get("string"));
        assertEquals(Integer.MAX_VALUE, map.get("int"));
        assertEquals(true, map.get("boolean"));
        p.recycle();
    }

    @SuppressWarnings("unchecked")
    public void testReadHashMap() {
        Parcel p;
        MockClassLoader mcl = new MockClassLoader();
        HashMap map = new HashMap();
        HashMap map2;

        p = Parcel.obtain();
        p.writeMap(null);
        p.setDataPosition(0);
        map2 = p.readHashMap(mcl);
        assertNull(map2);
        p.recycle();

        map.put("string", "String");
        map.put("int", Integer.MAX_VALUE);
        map.put("boolean", true);
        map2 = null;
        p = Parcel.obtain();
        p.writeMap(map);
        p.setDataPosition(0);
        map2 = p.readHashMap(mcl);
        assertNotNull(map2);
        assertEquals(3, map2.size());
        assertEquals("String", map.get("string"));
        assertEquals(Integer.MAX_VALUE, map.get("int"));
        assertEquals(true, map.get("boolean"));
        p.recycle();
    }

    @SuppressWarnings("unchecked")
    public void testReadList() {
        Parcel p;
        MockClassLoader mcl = new MockClassLoader();
        ArrayList arrayList = new ArrayList();

        p = Parcel.obtain();
        p.writeList(null);
        p.setDataPosition(0);
        assertEquals(0, arrayList.size());
        p.readList(arrayList, mcl);
        assertEquals(0, arrayList.size());
        p.recycle();

        ArrayList arrayList2 = new ArrayList();
        arrayList2.add(Integer.MAX_VALUE);
        arrayList2.add(true);
        arrayList2.add(Long.MAX_VALUE);
        arrayList2.add("String");
        arrayList2.add(Float.MAX_VALUE);

        p = Parcel.obtain();
        p.writeList(arrayList2);
        p.setDataPosition(0);
        assertEquals(0, arrayList.size());
        p.readList(arrayList, mcl);
        assertEquals(5, arrayList.size());
        for (int i = 0; i < arrayList.size(); i++) {
            assertEquals(arrayList.get(i), arrayList2.get(i));
        }
        p.recycle();
    }

    public void testBinderDataProtection() {
        Parcel p;
        IBinder b = new Binder();

        p = Parcel.obtain();
        final int firstIntPos = p.dataPosition();
        p.writeInt(1);
        p.writeStrongBinder(b);
        final int secondIntPos = p.dataPosition();
        p.writeInt(2);
        p.writeStrongBinder(b);
        final int thirdIntPos = p.dataPosition();
        p.writeInt(3);

        for (int pos = 0; pos <= thirdIntPos; pos++) {
            p.setDataPosition(pos);
            int value = p.readInt();
            if (pos == firstIntPos) {
                assertEquals(1, value);
            } else if (pos == secondIntPos) {
                assertEquals(2, value);
            } else if (pos == thirdIntPos) {
                assertEquals(3, value);
            } else {
                // All other read attempts cross into protected data and will return 0
                assertEquals(0, value);
            }
        }

        p.recycle();
    }

    public void testBinderDataProtectionIncrements() {
        Parcel p;
        IBinder b = new Binder();

        p = Parcel.obtain();
        final int firstIntPos = p.dataPosition();
        p.writeInt(1);
        p.writeStrongBinder(b);
        final int secondIntPos = p.dataPosition();
        p.writeInt(2);
        p.writeStrongBinder(b);
        final int thirdIntPos = p.dataPosition();
        p.writeInt(3);
        final int end = p.dataPosition();

        p.setDataPosition(0);
        int pos;
        do {
            pos = p.dataPosition();
            int value = p.readInt();
            if (pos == firstIntPos) {
                assertEquals(1, value);
            } else if (pos == secondIntPos) {
                assertEquals(2, value);
            } else if (pos == thirdIntPos) {
                assertEquals(3, value);
            } else {
                // All other read attempts cross into protected data and will return 0
                assertEquals(0, value);
            }
        } while(pos < end);

        p.recycle();
    }

    private class MockClassLoader extends ClassLoader {
        public MockClassLoader() {
            super();
        }
    }

    private class MockIInterface implements IInterface {
        public Binder binder;

        public MockIInterface() {
            binder = new Binder();
        }

        public IBinder asBinder() {
            return binder;
        }
    }

    private static boolean parcelableWithBadCreatorInitializerHasRun;
    private static boolean invalidCreatorIntializerHasRun;

    /**
     * A class that would be Parcelable except that it doesn't have a CREATOR field declared to be
     * of the correct type.
     */
    @SuppressWarnings("unused") // Referenced via reflection only
    private static class ParcelableWithBadCreator implements Parcelable {

        static {
            ParcelTest.parcelableWithBadCreatorInitializerHasRun = true;
        }

        private static class InvalidCreator
                implements Parcelable.Creator<ParcelableWithBadCreator> {

            static {
                invalidCreatorIntializerHasRun = true;
            }

            @Override
            public ParcelableWithBadCreator createFromParcel(Parcel source) {
                return null;
            }

            @Override
            public ParcelableWithBadCreator[] newArray(int size) {
                return new ParcelableWithBadCreator[0];
            }

        }

        // Invalid declaration: Must be declared as Parcelable.Creator or a subclass.
        public static Object CREATOR = new InvalidCreator();

        @Override
        public int describeContents() {
            return 0;
        }

        @Override
        public void writeToParcel(Parcel dest, int flags) {

        }
    }

    // http://b/1171613
    public void testBadStream_invalidCreator() {
        Parcel parcel = Parcel.obtain();
        // Create an invalid stream by manipulating the Parcel.
        parcel.writeString(getClass().getName() + "$ParcelableWithBadCreator");
        byte[] badData = parcel.marshall();
        parcel.recycle();

        // Now try to read the bad data.
        parcel = Parcel.obtain();
        parcel.unmarshall(badData, 0, badData.length);
        parcel.setDataPosition(0);
        try {
            parcel.readParcelable(getClass().getClassLoader());
            fail();
        } catch (BadParcelableException expected) {
        } finally {
            parcel.recycle();
        }

        assertFalse(invalidCreatorIntializerHasRun);
        assertFalse(parcelableWithBadCreatorInitializerHasRun);
    }

    private static boolean doesNotImplementParcelableInitializerHasRun;

    /** A class that would be Parcelable except that it does not implement Parcelable. */
    @SuppressWarnings("unused") // Referenced via reflection only
    private static class DoesNotImplementParcelable {

        static {
            doesNotImplementParcelableInitializerHasRun = true;
        }

        public static Parcelable.Creator<Object> CREATOR = new Parcelable.Creator<Object>() {
            @Override
            public Object createFromParcel(Parcel source) {
                return new DoesNotImplementParcelable();
            }

            @Override
            public Object[] newArray(int size) {
                return new Object[size];
            }
        };
    }

    // http://b/1171613
    public void testBadStream_objectDoesNotImplementParcelable() {
        Parcel parcel = Parcel.obtain();
        // Create an invalid stream by manipulating the Parcel.
        parcel.writeString(getClass().getName() + "$DoesNotImplementParcelable");
        byte[] badData = parcel.marshall();
        parcel.recycle();

        // Now try to read the bad data.
        parcel = Parcel.obtain();
        parcel.unmarshall(badData, 0, badData.length);
        parcel.setDataPosition(0);
        try {
            parcel.readParcelable(getClass().getClassLoader());
            fail();
        } catch (BadParcelableException expected) {
        } finally {
            parcel.recycle();
        }

        assertFalse(doesNotImplementParcelableInitializerHasRun);
    }

    public static class SimpleParcelable implements Parcelable {
        private final int value;

        public SimpleParcelable(int value) {
            this.value = value;
        }

        private SimpleParcelable(Parcel in) {
            this.value = in.readInt();
        }

        public int getValue() {
            return value;
        }

        @Override
        public int describeContents() {
            return 0;
        }

        @Override
        public void writeToParcel(Parcel out, int flags) {
            out.writeInt(value);
        }

        public static Parcelable.Creator<SimpleParcelable> CREATOR =
                new Parcelable.Creator<SimpleParcelable>() {

            @Override
            public SimpleParcelable createFromParcel(Parcel source) {
                return new SimpleParcelable(source);
            }

            @Override
            public SimpleParcelable[] newArray(int size) {
                return new SimpleParcelable[size];
            }
        };
    }

    public void testReadWriteParcellableList() {
        Parcel parcel = Parcel.obtain();

        ArrayList<SimpleParcelable> list = new ArrayList<>();
        list.add(new SimpleParcelable(57));

        // Writing a |null| list to a parcel should work, and reading it back
        // from a parcel should clear the target list.
        parcel.writeParcelableList(null, 0);
        parcel.setDataPosition(0);
        parcel.readParcelableList(list, SimpleParcelable.class.getClassLoader());
        assertEquals(0, list.size());

        list.clear();
        list.add(new SimpleParcelable(42));
        list.add(new SimpleParcelable(56));

        parcel.setDataPosition(0);
        parcel.writeParcelableList(list, 0);

        // Populate the list with a value, we will later assert that the
        // value has been removed.
        list.clear();
        list.add(new SimpleParcelable(100));

        parcel.setDataPosition(0);
        parcel.readParcelableList(list, SimpleParcelable.class.getClassLoader());

        assertEquals(2, list.size());
        assertEquals(42, list.get(0).getValue());
        assertEquals(56, list.get(1).getValue());
    }
}
