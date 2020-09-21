package com.xtremelabs.robolectric.shadows;

import static com.xtremelabs.robolectric.Robolectric.shadowOf;
import static junit.framework.Assert.assertEquals;
import static junit.framework.Assert.assertTrue;
import static org.hamcrest.CoreMatchers.equalTo;
import static org.hamcrest.CoreMatchers.not;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotSame;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertThat;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Parcel;
import android.os.Parcelable;

import com.xtremelabs.robolectric.Robolectric;
import com.xtremelabs.robolectric.WithTestDefaultsRunner;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.Serializable;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Set;

@RunWith(WithTestDefaultsRunner.class)
public class IntentTest {
    @Test
    public void testStringExtra() throws Exception {
        Intent intent = new Intent();
        assertSame(intent, intent.putExtra("foo", "bar"));
        assertEquals("bar", intent.getExtras().get("foo"));
    }

    @Test
    public void testCharSequenceExtra() throws Exception {
        Intent intent = new Intent();
        CharSequence cs = new TestCharSequence("bar");
        assertSame(intent, intent.putExtra("foo", cs));
        assertSame(cs, intent.getExtras().get("foo"));
    }

    @Test
    public void testIntExtra() throws Exception {
        Intent intent = new Intent();
        assertSame(intent, intent.putExtra("foo", 2));
        assertEquals(2, intent.getExtras().get("foo"));
        assertEquals(2, intent.getIntExtra("foo", -1));
    }

    @Test
    public void testDoubleExtra() throws Exception {
        Intent intent = new Intent();
        assertSame(intent, intent.putExtra("foo", 2d));
        assertEquals(2d, intent.getExtras().get("foo"));
        assertEquals(2d, intent.getDoubleExtra("foo", -1));
    }

    @Test
    public void testFloatExtra() throws Exception {
        Intent intent = new Intent();
        assertSame(intent, intent.putExtra("foo", 2f));
        assertEquals(2f, intent.getExtras().get("foo"));
        assertEquals(2f, intent.getFloatExtra("foo", -1));
    }

    @Test
    public void testIntArrayExtra() throws Exception {
        Intent intent = new Intent();
        int[] array = new int[2];
        array[0] = 1;
        array[1] = 2;
        assertSame(intent, intent.putExtra("foo", array));
        assertEquals(1, intent.getIntArrayExtra("foo")[0]);
        assertEquals(2, intent.getIntArrayExtra("foo")[1]);
    }

    @Test
    public void testLongArrayExtra() throws Exception {
        Intent intent = new Intent();
        long[] array = new long[2];
        array[0] = 1L;
        array[1] = 2L;
        assertSame(intent, intent.putExtra("foo", array));
        assertEquals(1L, intent.getLongArrayExtra("foo")[0]);
        assertEquals(2L, intent.getLongArrayExtra("foo")[1]);
    }

    @Test
    public void testSerializableExtra() throws Exception {
        Intent intent = new Intent();
        TestSerializable serializable = new TestSerializable("some string");
        assertSame(intent, intent.putExtra("foo", serializable));
        assertEquals(serializable, intent.getExtras().get("foo"));
        assertNotSame(serializable, intent.getExtras().get("foo"));
        assertEquals(serializable, intent.getSerializableExtra("foo"));
        assertNotSame(serializable, intent.getSerializableExtra("foo"));
    }

    @Test
    public void testParcelableExtra() throws Exception {
        Intent intent = new Intent();
        Parcelable parcelable = new TestParcelable(44);
        assertSame(intent, intent.putExtra("foo", parcelable));
        assertSame(parcelable, intent.getExtras().get("foo"));
        assertSame(parcelable, intent.getParcelableExtra("foo"));
    }

    @Test
    public void testParcelableArrayExtra() throws Exception {
        Intent intent = new Intent();
        Parcelable parcelable = new TestParcelable(11);
        intent.putExtra("foo", parcelable);
        assertSame(null, intent.getParcelableArrayExtra("foo"));
        Parcelable[] parcelables = {new TestParcelable(12), new TestParcelable(13)};
        assertSame(intent, intent.putExtra("bar", parcelables));
        assertSame(parcelables, intent.getParcelableArrayExtra("bar"));
    }

    @Test
    public void testParcelableArrayListExtra() {
        Intent intent = new Intent();
        Parcelable parcel1 = new TestParcelable(22);
        Parcelable parcel2 = new TestParcelable(23);
        ArrayList<Parcelable> parcels = new ArrayList<Parcelable>();
        parcels.add(parcel1);
        parcels.add(parcel2);

        assertSame(intent, intent.putParcelableArrayListExtra("foo", parcels));
        assertSame(parcels, intent.getParcelableArrayListExtra("foo"));
        assertSame(parcel1, intent.getParcelableArrayListExtra("foo").get(0));
        assertSame(parcel2, intent.getParcelableArrayListExtra("foo").get(1));
        assertSame(parcels, intent.getExtras().getParcelableArrayList("foo"));
    }

    @Test
    public void testLongExtra() throws Exception {
        Intent intent = new Intent();
        assertSame(intent, intent.putExtra("foo", 2L));
        assertEquals(2L, shadowOf(intent).getExtras().get("foo"));
        assertEquals(2L, intent.getLongExtra("foo", -1));
        assertEquals(-1L, intent.getLongExtra("bar", -1));
    }

    @Test
    public void testBundleExtra() throws Exception {
        Intent intent = new Intent();
        Bundle bundle = new Bundle();
        bundle.putInt("bar", 5);
        assertSame(intent, intent.putExtra("foo", bundle));
        assertEquals(5, intent.getBundleExtra("foo").getInt("bar"));
    }

    @Test
    public void testHasExtra() throws Exception {
        Intent intent = new Intent();
        assertSame(intent, intent.putExtra("foo", ""));
        assertTrue(intent.hasExtra("foo"));
        assertFalse(intent.hasExtra("bar"));
    }

    @Test
    public void testGetActionReturnsWhatWasSet() throws Exception {
        Intent intent = new Intent();
        assertSame(intent, intent.setAction("foo"));
        assertEquals("foo", intent.getAction());
    }

    @Test
    public void testSetData() throws Exception {
        Intent intent = new Intent();
        Uri uri = Uri.parse("content://this/and/that");
        intent.setType("abc");
        assertSame(intent, intent.setData(uri));
        assertSame(uri, intent.getData());
        assertNull(intent.getType());
    }

    @Test
    public void testSetType() throws Exception {
        Intent intent = new Intent();
        intent.setData(Uri.parse("content://this/and/that"));
        assertSame(intent, intent.setType("def"));
        assertNull(intent.getData());
        assertEquals("def", intent.getType());
    }

    @Test
    public void testSetDataAndType() throws Exception {
        Intent intent = new Intent();
        Uri uri = Uri.parse("content://this/and/that");
        assertSame(intent, intent.setDataAndType(uri, "ghi"));
        assertSame(uri, intent.getData());
        assertEquals("ghi", intent.getType());
    }

    @Test
    public void testSetClass() throws Exception {
        Intent intent = new Intent();
        Class<? extends IntentTest> thisClass = getClass();
        Intent output = intent.setClass(new Activity(), thisClass);

        assertSame(output, intent);
        ShadowIntent si = shadowOf(intent);
        assertSame(si.getIntentClass(), thisClass);
    }

    @Test
    public void testSetClassName() throws Exception {
        Intent intent = new Intent();
        Class<? extends IntentTest> thisClass = getClass();
        intent.setClassName("package.name", thisClass.getName());
        assertSame(thisClass.getName(), intent.getComponent().getClassName());
        assertEquals("package.name", intent.getComponent().getPackageName());
        ShadowIntent si = shadowOf(intent);
        assertSame(si.getIntentClass(), thisClass);
    }

    @Test
    public void testSetClassThroughConstructor() throws Exception {
        Intent intent = new Intent(new Activity(), getClass());
        assertEquals(shadowOf(intent).getIntentClass(), getClass());
    }

    @Test
    public void shouldSetFlags() throws Exception {
        Intent intent = new Intent();
        Intent self = intent.setFlags(1234);
        assertEquals(1234, intent.getFlags());
        assertSame(self, intent);
    }

    @Test
    public void shouldAddFlags() throws Exception {
        Intent intent = new Intent();
        Intent self = intent.addFlags(4);
        self.addFlags(8);
        assertEquals(12, intent.getFlags());
        assertSame(self, intent);
    }

    @Test
    public void shouldSupportCategories() throws Exception {
        Intent intent = new Intent();
        Intent self = intent.addCategory("category.name.1");
        intent.addCategory("category.name.2");

        assertTrue(intent.hasCategory("category.name.1"));
        assertTrue(intent.hasCategory("category.name.2"));

        Set<String> categories = intent.getCategories();
        assertTrue(categories.contains("category.name.1"));
        assertTrue(categories.contains("category.name.2"));

        intent.removeCategory("category.name.1");
        assertFalse(intent.hasCategory("category.name.1"));
        assertTrue(intent.hasCategory("category.name.2"));

        intent.removeCategory("category.name.2");
        assertFalse(intent.hasCategory("category.name.2"));

        assertEquals(0, intent.getCategories().size());

        assertSame(self, intent);
    }

    @Test
    public void shouldAddCategories() throws Exception {
        Intent intent = new Intent();
        Intent self = intent.addCategory("foo");
        assertTrue(intent.getCategories().contains("foo"));
        assertSame(self, intent);
    }

    @Test
    public void shouldFillIn() throws Exception {
        Intent intentA = new Intent();
        Intent intentB = new Intent();

        intentB.setAction("foo");
        Uri uri = Uri.parse("http://www.foo.com");
        intentB.setDataAndType(uri, "text/html");
        String category = "category";
        intentB.addCategory(category);
        intentB.setPackage("com.foobar.app");
        ComponentName cn = new ComponentName("com.foobar.app", "activity");
        intentB.setComponent(cn);
        intentB.putExtra("FOO", 23);

        int flags = Intent.FILL_IN_ACTION |
                Intent.FILL_IN_DATA |
                Intent.FILL_IN_CATEGORIES |
                Intent.FILL_IN_PACKAGE |
                Intent.FILL_IN_COMPONENT;

        int result = intentA.fillIn(intentB, flags);
        assertEquals("foo", intentA.getAction());
        assertSame(uri, intentA.getData());
        assertEquals("text/html", intentA.getType());
        assertTrue(intentA.getCategories().contains(category));
        assertEquals("com.foobar.app", intentA.getPackage());
        assertSame(cn, intentA.getComponent());
        assertEquals(23, intentA.getIntExtra("FOO", -1));
        assertEquals(result, flags);
    }

    @Test
    public void equals_shouldTestActionComponentNameDataAndExtras() throws Exception {
        Intent intentA = new Intent()
                .setAction("action")
                .setData(Uri.parse("content:1"))
                .setComponent(new ComponentName("pkg", "cls"))
                .putExtra("extra", "blah")
                .setType("image/*")
                .addCategory("category.name");

        Intent intentB = new Intent()
                .setAction("action")
                .setData(Uri.parse("content:1"))
                .setComponent(new ComponentName("pkg", "cls"))
                .putExtra("extra", "blah")
                .setType("image/*")
                .addCategory("category.name");

        assertThat(intentA, equalTo(intentB));

        intentB.setAction("other action");
        assertThat(intentA, not(equalTo(intentB)));

        intentB.setAction("action");
        intentB.setData(Uri.parse("content:other"));
        assertThat(intentA, not(equalTo(intentB)));

        intentB.setData(Uri.parse("content:1"));
        intentB.setComponent(new ComponentName("other-pkg", "other-cls"));
        assertThat(intentA, not(equalTo(intentB)));

        intentB.setComponent(new ComponentName("pkg", "cls"));
        intentB.putExtra("extra", "foo");
        assertThat(intentA, not(equalTo(intentB)));

        intentB.putExtra("extra", "blah");
        intentB.setType("other/*");
        assertThat(intentA, not(equalTo(intentB)));

        intentB.setType("image/*");
        assertThat(intentA, equalTo(intentB));

        intentB.removeCategory("category.name");
        assertThat(intentA, not(equalTo(intentB)));
    }

    @Test
    public void equals_whenOtherObjectIsNotAnIntent_shouldReturnFalse() throws Exception {
        assertThat(new Intent(), not(equalTo(new Object())));
    }

    @Test
    public void createChooser_shouldWrapIntent() throws Exception {
        Intent originalIntent = new Intent(Intent.ACTION_BATTERY_CHANGED, Uri.parse("foo://blah"));
        Intent chooserIntent = Intent.createChooser(originalIntent, "The title");
        Intent expectedIntent = new Intent(Intent.ACTION_CHOOSER);
        expectedIntent.putExtra(Intent.EXTRA_INTENT, originalIntent);
        expectedIntent.putExtra(Intent.EXTRA_TITLE, "The title");
        assertEquals(expectedIntent, chooserIntent);
    }

    @Test
    public void putStringArrayListExtra_addsListToExtras() {
        Intent intent = new Intent();
        final ArrayList<String> strings = new ArrayList<String>(Arrays.asList("hi", "there"));

        intent.putStringArrayListExtra("KEY", strings);
        assertThat(intent.getStringArrayListExtra("KEY"), equalTo(strings));
        assertThat(Robolectric.shadowOf(intent.getExtras()).getStringArrayList("KEY"), equalTo(strings));
    }

    @Test
    public void putIntegerArrayListExtra_addsListToExtras() {
        Intent intent = new Intent();
        final ArrayList<Integer> integers = new ArrayList<Integer>(Arrays.asList(100, 200, 300));

        intent.putIntegerArrayListExtra("KEY", integers);
        assertThat(intent.getIntegerArrayListExtra("KEY"), equalTo(integers));
        assertThat(Robolectric.shadowOf(intent.getExtras()).getIntegerArrayList("KEY"), equalTo(integers));
    }

    @Test
    public void testParcelIo_explicitIntent() {
        Intent intent = new Intent(new Activity(), getClass());
        intent.putExtra("boolean", true);
        intent.putExtra("string", "string value");
        Bundle bundle = new Bundle();
        bundle.putDouble("bundle double", 3.14);
        intent.putExtra("bundle", bundle);
        intent.putExtra("long", 893);

        verifyIntentReadIsWhatWasWrittenToParcel(intent);
    }

    @Test
    public void testParcelIo_actionTypeCategory() {
        Intent intent = new Intent("action");
        intent.setType("type");
        intent.addCategory("category");
        verifyIntentReadIsWhatWasWrittenToParcel(intent);
    }

    @Test
    public void shouldHaveCreator() throws Exception {
        Intent expected = new Intent("action");
        expected.setType("type");
        expected.addCategory("category");
        Parcel p = Parcel.obtain();
        expected.writeToParcel(p, 0);

        p.setDataPosition(0);

        Intent actual = Intent.CREATOR.createFromParcel(p);
        assertThat(expected, equalTo(actual));
    }

    private void verifyIntentReadIsWhatWasWrittenToParcel(Intent expected) {
        Parcel parcel = Parcel.obtain();
        expected.writeToParcel(parcel, 0);
        parcel.setDataPosition(0);
        Intent actual = new Intent();
        actual.readFromParcel(parcel);

        assertThat(expected, equalTo(actual));
    }

    private static class TestSerializable implements Serializable {
        private final String someValue;

        public TestSerializable(String someValue) {
            this.someValue = someValue;
        }

        @Override
        public boolean equals(Object o) {
            if (this == o) return true;
            if (o == null || getClass() != o.getClass()) return false;

            TestSerializable that = (TestSerializable) o;

            if (someValue != null ? !someValue.equals(that.someValue) : that.someValue != null) return false;

            return true;
        }

        @Override
        public int hashCode() {
            return someValue != null ? someValue.hashCode() : 0;
        }
    }

    private class TestCharSequence implements CharSequence {
        String s;

        public TestCharSequence(String s) {
            this.s = s;
        }

        @Override
        public char charAt(int index) {
            return s.charAt(index);
        }

        @Override
        public int length() {
            return s.length();
        }

        @Override
        public CharSequence subSequence(int start, int end) {
            return s.subSequence(start, end);
        }

    }
}
