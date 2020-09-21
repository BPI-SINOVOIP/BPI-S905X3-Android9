package com.xtremelabs.robolectric.shadows;

import static org.hamcrest.CoreMatchers.equalTo;
import static org.hamcrest.CoreMatchers.notNullValue;
import static org.hamcrest.CoreMatchers.nullValue;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertThat;
import static org.junit.Assert.assertTrue;

import android.accounts.Account;
import android.content.Intent;
import android.os.Bundle;
import android.os.Parcel;

import com.xtremelabs.robolectric.Robolectric;
import com.xtremelabs.robolectric.WithTestDefaultsRunner;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

@RunWith(WithTestDefaultsRunner.class)
public class ParcelTest {

    private Parcel parcel;
    private ShadowParcel shadowParcel;

    @Before
    public void setup() {
        parcel = Parcel.obtain();
        shadowParcel = Robolectric.shadowOf(parcel);
    }

    @Test
    public void testObtain() {
        assertThat(parcel, notNullValue());
        assertThat(shadowParcel.getIndex(), equalTo(0));
        assertThat(shadowParcel.getParcelData().size(), equalTo(0));
    }

    @Test
    public void testReadIntWhenEmpty() {
        assertThat(parcel.readInt(), equalTo(0));
    }

    @Test
    public void testReadLongWhenEmpty() {
        assertThat(parcel.readLong(), equalTo(0l));
    }

    @Test
    public void testReadStringWhenEmpty() {
        assertThat(parcel.readString(), nullValue());
    }

    @Test
    public void testReadWriteSingleString() {
        String val = "test";
        parcel.writeString(val);
        parcel.setDataPosition(0);
        assertThat(parcel.readString(), equalTo(val));
    }

	@Test
	public void testWriteNullString() {
		parcel.writeString( null );
		parcel.setDataPosition(0);
		assertThat( parcel.readString(), nullValue() );
		assertThat( shadowParcel.getIndex(), equalTo( 1 ) );
		assertThat( shadowParcel.getParcelData().size(), equalTo( 1 ) );
	}

    @Test
    public void testReadWriteMultipleStrings() {
        for (int i = 0; i < 10; ++i) {
            parcel.writeString(Integer.toString(i));
        }
        parcel.setDataPosition(0);
        for (int i = 0; i < 10; ++i) {
            assertThat(parcel.readString(), equalTo(Integer.toString(i)));
        }
        // now try to read past the number of items written and see what happens
        assertThat(parcel.readString(), nullValue());
    }

    @Test
    public void testReadWriteSingleInt() {
        int val = 5;
        parcel.writeInt(val);
        parcel.setDataPosition(0);
        assertThat(parcel.readInt(), equalTo(val));
    }

    @Test
    public void testReadWriteIntArray() throws Exception {
        final int[] ints = {1, 2};
        parcel.writeIntArray(ints);
        parcel.setDataPosition(0);
        final int[] ints2 = new int[ints.length];
        parcel.readIntArray(ints2);
        assertTrue(Arrays.equals(ints, ints2));
    }

    @Test
    public void testReadWriteLongArray() throws Exception {
        final long[] longs = {1, 2};
        parcel.writeLongArray(longs);
        parcel.setDataPosition(0);
        final long[] longs2 = new long[longs.length];
        parcel.readLongArray(longs2);
        assertTrue(Arrays.equals(longs, longs2));
    }

    @Test
    public void testReadWriteSingleFloat() {
        float val = 5.2f;
        parcel.writeFloat(val);
        parcel.setDataPosition(0);
        assertThat(parcel.readFloat(), equalTo(val));
    }

    @Test
    public void testReadWriteFloatArray() throws Exception {
        final float[] floats = {1.1f, 2.0f};
        parcel.writeFloatArray(floats);
        parcel.setDataPosition(0);
        final float[] floats2 = new float[floats.length];
        parcel.readFloatArray(floats2);
        assertTrue(Arrays.equals(floats, floats2));
    }

    @Test
    public void testReadWriteDoubleArray() throws Exception {
        final double[] doubles = {1.1f, 2.0f};
        parcel.writeDoubleArray(doubles);
        parcel.setDataPosition(0);
        final double[] doubles2 = new double[doubles.length];
        parcel.readDoubleArray(doubles2);
        assertTrue(Arrays.equals(doubles, doubles2));
    }

    @Test
    public void testReadWriteStringArray() throws Exception {
        final String[] strings = {"foo", "bar"};
        parcel.writeStringArray(strings);
        parcel.setDataPosition(0);
        final String[] strings2 = new String[strings.length];
        parcel.readStringArray(strings2);
        assertTrue(Arrays.equals(strings, strings2));
    }

    @Test
    public void testReadWriteMultipleInts() {
        for (int i = 0; i < 10; ++i) {
            parcel.writeInt(i);
        }
        parcel.setDataPosition(0);
        for (int i = 0; i < 10; ++i) {
            assertThat(parcel.readInt(), equalTo(i));
        }
        // now try to read past the number of items written and see what happens
        assertThat(parcel.readInt(), equalTo(0));
    }

    @Test
    public void testReadWriteSingleByte() {
        byte val = 1;
        parcel.writeByte(val);
        parcel.setDataPosition(0);
        assertThat(parcel.readByte(), equalTo(val));
    }

    @Test
    public void testReadWriteMultipleBytes() {
        for (byte i = Byte.MIN_VALUE; i < Byte.MAX_VALUE; ++i) {
            parcel.writeByte(i);
        }
        parcel.setDataPosition(0);
        for (byte i = Byte.MIN_VALUE; i < Byte.MAX_VALUE; ++i) {
            assertThat(parcel.readByte(), equalTo(i));
        }
        // now try to read past the number of items written and see what happens
        assertThat(parcel.readByte(), equalTo((byte) 0));
    }


    @Test
    public void testReadWriteStringInt() {
        for (int i = 0; i < 10; ++i) {
            parcel.writeString(Integer.toString(i));
            parcel.writeInt(i);
        }
        parcel.setDataPosition(0);
        for (int i = 0; i < 10; ++i) {
            assertThat(parcel.readString(), equalTo(Integer.toString(i)));
            assertThat(parcel.readInt(), equalTo(i));
        }
        // now try to read past the number of items written and see what happens
        assertThat(parcel.readString(), nullValue());
        assertThat(parcel.readInt(), equalTo(0));
    }

    @Test
    public void testReadWriteSingleLong() {
        long val = 5;
        parcel.writeLong(val);
        parcel.setDataPosition(0);
        assertThat(parcel.readLong(), equalTo(val));
    }

    @Test
    public void testReadWriteMultipleLongs() {
        for (long i = 0; i < 10; ++i) {
            parcel.writeLong(i);
        }
        parcel.setDataPosition(0);
        for (long i = 0; i < 10; ++i) {
            assertThat(parcel.readLong(), equalTo(i));
        }
        // now try to read past the number of items written and see what happens
        assertThat(parcel.readLong(), equalTo(0l));
    }

    @Test
    public void testReadWriteStringLong() {
        for (long i = 0; i < 10; ++i) {
            parcel.writeString(Long.toString(i));
            parcel.writeLong(i);
        }
        parcel.setDataPosition(0);
        for (long i = 0; i < 10; ++i) {
            assertThat(parcel.readString(), equalTo(Long.toString(i)));
            assertThat(parcel.readLong(), equalTo(i));
        }
        // now try to read past the number of items written and see what happens
        assertThat(parcel.readString(), nullValue());
        assertThat(parcel.readLong(), equalTo(0l));
    }

    @Test
    public void testReadWriteParcelable() {
        Intent i1 = new Intent("anAction");
        parcel.writeParcelable(i1, 0);

        parcel.setDataPosition(0);

        Intent i2 = parcel.readParcelable(Intent.class.getClassLoader());
        assertEquals(i1, i2);
    }

    @Test
    public void testReadWriteSimpleBundle() {
        Bundle b1 = new Bundle();
        b1.putString("hello", "world");
        parcel.writeBundle(b1);
        parcel.setDataPosition(0);
        Bundle b2 = parcel.readBundle();

        assertEquals(b1, b2);
        assertEquals("world", b2.getString("hello"));

        parcel.setDataPosition(0);
        parcel.writeBundle(b1);
        parcel.setDataPosition(0);
        b2 = parcel.readBundle(null /* ClassLoader */);
        assertEquals(b1, b2);
        assertEquals("world", b2.getString("hello"));
    }

    @Test
    public void testReadWriteNestedBundles() {
        Account account = new Account("accountName", "accountType");
        Bundle innerBundle = new Bundle();
        innerBundle.putString("hello", "world");
        innerBundle.putParcelable("account", account);
        Bundle b1 = new Bundle();
        b1.putBundle("bundle", innerBundle);
        b1.putInt("int", 23);
        parcel.writeBundle(b1);
        parcel.setDataPosition(0);
        Bundle b2 = parcel.readBundle();

        assertEquals(b1, b2);
        assertEquals(innerBundle, b2.getBundle("bundle"));
        assertEquals(23, b2.getInt("int"));
        assertEquals("world", b2.getBundle("bundle").getString("hello"));

        parcel.setDataPosition(0);
        parcel.writeBundle(b1);
        parcel.setDataPosition(0);
        b2 = parcel.readBundle(null /* ClassLoader */);
        assertEquals(b1, b2);
        assertEquals(innerBundle, b2.getBundle("bundle"));
        assertEquals(23, b2.getInt("int"));
        assertEquals("world", b2.getBundle("bundle").getString("hello"));
    }

    @Test
    public void testReadWriteBundleWithDifferentValueTypes() {
        Bundle b1 = new Bundle();
        b1.putString("hello", "world");
        b1.putBoolean("boolean", true);
        b1.putByte("byte", (byte) 0xAA);
        b1.putShort("short", (short)0xBABE);
        b1.putInt("int", 1);
        b1.putFloat("float", 0.5f);
        b1.putDouble("double", 1.25);
        parcel.writeBundle(b1);
        parcel.setDataPosition(0);

        Bundle b2 = parcel.readBundle();

        assertEquals(b1, b2);
        assertEquals("world", b2.getString("hello"));
        assertEquals(true, b2.getBoolean("boolean"));
        assertEquals((byte) 0xAA, b2.getByte("byte"));
        assertEquals((short) 0xBABE, b2.getShort("short"));
        assertEquals(1, b2.getInt("int"));
        assertEquals(0.5f, b2.getFloat("float"), 0.05);
        assertEquals(1.25, b2.getDouble("double"), 0.05);

        parcel.setDataPosition(0);
        parcel.writeBundle(b1);
        parcel.setDataPosition(0);
        b2 = parcel.readBundle(null /* ClassLoader */);
        assertEquals(b1, b2);
        assertEquals("world", b2.getString("hello"));
        assertEquals(true, b2.getBoolean("boolean"));
        assertEquals((byte) 0xAA, b2.getByte("byte"));
        assertEquals((short) 0xBABE, b2.getShort("short"));
        assertEquals(1, b2.getInt("int"));
        assertEquals(0.5f, b2.getFloat("float"), 0.05);
        assertEquals(1.25, b2.getDouble("double"), 0.05);
    }

    @Test
    public void testWriteCreateStringArray() {
      final String[] strings = { "foo", "bar" };
      parcel.writeStringArray(strings);
      parcel.setDataPosition(0);
      final String[] strings2 = parcel.createStringArray();
      assertTrue(Arrays.equals(strings, strings2));
    }

    @Test
    public void testReadWriteStringList() {
        final List<String> strings = Arrays.asList( "foo", "bar" );
        parcel.writeStringList(strings);
        parcel.setDataPosition(0);
        List<String> extractedStrings = new ArrayList<String>();
        parcel.readStringList(extractedStrings);
        assertEquals(strings, extractedStrings);
    }

    @Test
    public void testWriteCreateStringArrayList() {
        final List<String> strings = Arrays.asList( "foo", "bar" );
        parcel.writeStringList(strings);
        parcel.setDataPosition(0);
        List<String> extractedStrings = parcel.createStringArrayList();
        assertEquals(strings, extractedStrings);
    }

    @Test
    public void testReadWriteByteArray() throws Exception {
        final byte[] bytes = {1, 2};
        parcel.writeByteArray(bytes);
        parcel.setDataPosition(0);
        final byte[] bytes2 = new byte[bytes.length];
        parcel.readByteArray(bytes2);
        assertTrue(Arrays.equals(bytes, bytes2));
    }

    @Test
    public void testReadWriteBooleanArray() {
        final boolean[] booleans = {false, true, true};
        parcel.writeBooleanArray(booleans);
        parcel.setDataPosition(0);
        final boolean[] booleans2 = new boolean[booleans.length];
        parcel.readBooleanArray(booleans2);
        assertTrue(Arrays.equals(booleans, booleans2));
    }

    @Test
    public void testReadWriteCharArray() {
        final char[] chars = {'a', 'b', 'c'};
        parcel.writeCharArray(chars);
        parcel.setDataPosition(0);
        final char[] chars2 = new char[chars.length];
        parcel.readCharArray(chars2);
        assertTrue(Arrays.equals(chars, chars2));
    }

    @Test
    public void testWriteCreateBooleanArray() {
        final boolean[] booleans = {false, true, true};
        parcel.writeBooleanArray(booleans);
        parcel.setDataPosition(0);
        final boolean[] booleans2 = parcel.createBooleanArray();
        assertTrue(Arrays.equals(booleans, booleans2));
    }

    @Test
    public void testWriteCreateByteArray() {
        final byte[] bytes = {1, 2};
        parcel.writeByteArray(bytes);
        parcel.setDataPosition(0);
        final byte[] bytes2 = parcel.createByteArray();
        assertTrue(Arrays.equals(bytes, bytes2));
    }

    @Test
    public void testWriteCreateCharArray() {
        final char[] chars = {'a', 'b', 'c'};
        parcel.writeCharArray(chars);
        parcel.setDataPosition(0);
        final char[] chars2 = parcel.createCharArray();
        assertTrue(Arrays.equals(chars, chars2));
    }

    @Test
    public void testWriteCreateIntArray() {
        final int[] ints = {1, 2};
        parcel.writeIntArray(ints);
        parcel.setDataPosition(0);
        final int[] ints2 = parcel.createIntArray();
        assertTrue(Arrays.equals(ints, ints2));
    }

    @Test
    public void testWriteCreateLongArray() {
        final long[] longs = {1, 2};
        parcel.writeLongArray(longs);
        parcel.setDataPosition(0);
        final long[] longs2 = parcel.createLongArray();
        assertTrue(Arrays.equals(longs, longs2));
    }

    @Test
    public void testWriteCreateFloatArray() {
        final float[] floats = {1.5f, 2.25f};
        parcel.writeFloatArray(floats);
        parcel.setDataPosition(0);
        final float[] floats2 = parcel.createFloatArray();
        assertTrue(Arrays.equals(floats, floats2));
    }

    @Test
    public void testWriteCreateDoubleArray() {
        final double[] doubles = {1.2, 2.2};
        parcel.writeDoubleArray(doubles);
        parcel.setDataPosition(0);
        final double[] doubles2 = parcel.createDoubleArray();
        assertTrue(Arrays.equals(doubles, doubles2));
    }

    @Test
    public void testDataPositionAfterStringWrite() {
      parcel.writeString("string");
      assertEquals(10, parcel.dataPosition());
    }

    @Test
    public void testDataPositionAfterByteWrite() {
      parcel.writeByte((byte) 0);
      assertEquals(1, parcel.dataPosition());
    }

    @Test
    public void testDataPositionAfterIntWrite() {
      parcel.writeInt(1);
      assertEquals(4, parcel.dataPosition());
    }

    @Test
    public void testDataPositionAfterLongWrite() {
      parcel.writeLong(23);
      assertEquals(8, parcel.dataPosition());
    }

    @Test
    public void testDataPositionAfterFloatWrite() {
      parcel.writeFloat(0.5f);
      assertEquals(4, parcel.dataPosition());
    }

    @Test
    public void testDataPositionAfterDoubleWrite() {
      parcel.writeDouble(8.8);
      assertEquals(8, parcel.dataPosition());
    }

    @Test
    public void testResetDataPositionAfterWrite() {
      parcel.writeInt(4);
      parcel.setDataPosition(0);
      assertEquals(0, parcel.dataPosition());
    }

    @Test
    public void testOverwritePreviousValue() {
      parcel.writeInt(4);
      parcel.setDataPosition(0);
      parcel.writeInt(34);
      parcel.setDataPosition(0);
      assertEquals(34, parcel.readInt());
      assertEquals(4, parcel.dataSize());
    }
}
