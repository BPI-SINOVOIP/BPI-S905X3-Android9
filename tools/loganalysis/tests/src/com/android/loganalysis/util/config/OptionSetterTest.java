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

package com.android.loganalysis.util.config;

import junit.framework.TestCase;

import java.io.File;
import java.io.IOException;
import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.Map;

/**
 * Unit tests for {@link OptionSetter}.
 */
public class OptionSetterTest extends TestCase {

    /** Option source with generic type. */
    private static class GenericTypeOptionSource {
        @Option(name = "my_option", shortName = 'o')
        private Collection<?> mMyOption;
    }

    /** Option source with unparameterized type. */
    @SuppressWarnings("rawtypes")
    private static class CollectionTypeOptionSource {
        @Option(name = "my_option", shortName = 'o')
        private Collection mMyOption;
    }

    private static class MyGeneric<T> {
    }

    /** Option source with unparameterized type. */
    private static class NonCollectionGenericTypeOptionSource {
        @Option(name = "my_option", shortName = 'o')
        private MyGeneric<String> mMyOption;
    }

    /** Option source with options with same name. */
    private static class DuplicateOptionSource {
        @Option(name = "string", shortName = 's')
        private String mMyOption;

        @Option(name = "string", shortName = 's')
        private String mMyDuplicateOption;
    }

    /** Option source with an option with same name as AllTypesOptionSource. */
    @OptionClass(alias = "shared")
    private static class SharedOptionSource {
        @Option(name = "string", shortName = 's')
        private String mMyOption;

        @Option(name = "enum")
        private DefaultEnumClass mEnum = null;

        @Option(name = "string_collection")
        private Collection<String> mStringCollection = new ArrayList<String>();

        @Option(name = "enumMap")
        private Map<DefaultEnumClass, CustomEnumClass> mEnumMap =
                new HashMap<DefaultEnumClass, CustomEnumClass>();

        @Option(name = "enumCollection")
        private Collection<DefaultEnumClass> mEnumCollection =
                new ArrayList<DefaultEnumClass>();
    }

    /**
     * Option source with an option with same name as AllTypesOptionSource, but a different type.
     */
    private static class SharedOptionWrongTypeSource {
        @Option(name = "string", shortName = 's')
        private int mMyOption;
    }

    /** option source with all supported types. */
    @OptionClass(alias = "all")
    private static class AllTypesOptionSource {
        @Option(name = "string_collection")
        private final Collection<String> mStringCollection = new ArrayList<String>();

        @Option(name = "string_string_map")
        private Map<String, String> mStringMap = new HashMap<String, String>();

        @Option(name = "string")
        private String mString = null;

        @Option(name = "boolean")
        private boolean mBool = false;

        @Option(name = "booleanObj")
        private Boolean mBooleanObj = false;

        @Option(name = "byte")
        private byte mByte = 0;

        @Option(name = "byteObj")
        private Byte mByteObj = 0;

        @Option(name = "short")
        private short mShort = 0;

        @Option(name = "shortObj")
        private Short mShortObj = null;

        @Option(name = "int")
        private int mInt = 0;

        @Option(name = "intObj")
        private Integer mIntObj = 0;

        @Option(name = "long")
        private long mLong = 0;

        @Option(name = "longObj")
        private Long mLongObj = null;

        @Option(name = "float")
        private float mFloat = 0;

        @Option(name = "floatObj")
        private Float mFloatObj = null;

        @Option(name = "double")
        private double mDouble = 0;

        @Option(name = "doubleObj")
        private Double mDoubleObj = null;

        @Option(name = "file")
        private File mFile = null;

        @Option(name = "enum")
        private DefaultEnumClass mEnum = null;

        @Option(name = "customEnum")
        private CustomEnumClass mCustomEnum = null;

        @Option(name = "enumMap")
        private Map<DefaultEnumClass, CustomEnumClass> mEnumMap =
                new HashMap<DefaultEnumClass, CustomEnumClass>();

        @Option(name = "enumCollection")
        private Collection<DefaultEnumClass> mEnumCollection =
                new ArrayList<DefaultEnumClass>();
    }

    private static class ParentOptionSource {
        @Option(name = "string")
        private String mString = null;

        protected String getParentString() {
            return mString;
        }
    }

    private static class ChildOptionSource extends ParentOptionSource {
        @Option(name = "child-string")
        private String mChildString = null;
    }

    /**
     * Option source with invalid option name.
     */
    private static class BadOptionNameSource {
        @Option(name = "bad:string", shortName = 's')
        private int mMyOption;
    }

    private static enum DefaultEnumClass {
        VAL1, VAL3, VAL2;
    }

    private static enum CustomEnumClass {
        VAL1(42);

        private int mVal;

        CustomEnumClass(int val) {
            mVal = val;
        }

        public int getVal() {
            return mVal;
        }
    }

    private static class FinalOption {
        @Option(name = "final-string", description="final field, not allowed")
        private final String mFinal= "foo";
    }

    /**
     * Test creating an {@link OptionSetter} for a source with invalid option type.
     */
    public void testOptionSetter_noType() {
        try {
            new OptionSetter(new GenericTypeOptionSource());
            fail("ConfigurationException not thrown");
        } catch (ConfigurationException e) {
            // expected
        }
    }

    /**
     * Test creating an {@link OptionSetter} for a source with duplicate option names.
     */
    public void testOptionSetter_duplicateOptions() {
        try {
            new OptionSetter(new DuplicateOptionSource());
            fail("ConfigurationException not thrown");
        } catch (ConfigurationException e) {
            // expected
        }
    }

    /**
     * Test option with same name can be used in multiple option sources.
     */
    public void testOptionSetter_sharedOptions() throws ConfigurationException {
        AllTypesOptionSource object1 = new AllTypesOptionSource();
        SharedOptionSource object2 = new SharedOptionSource();
        OptionSetter setter = new OptionSetter(object1, object2);
        setter.setOptionValue("string", "test");
        assertEquals("test", object1.mString);
        assertEquals("test", object2.mMyOption);
    }

    /**
     * Test {@link OptionSetter#setOptionValue(String, String)} for Enums used as the key and value
     * of a {@link Map}.
     */
    public void testOptionSetter_sharedEnumMap() throws ConfigurationException {
        AllTypesOptionSource object1 = new AllTypesOptionSource();
        SharedOptionSource object2 = new SharedOptionSource();

        final String key = "VAL1";
        final String value = "VAL1";
        final DefaultEnumClass expectedKey = DefaultEnumClass.VAL1;
        final CustomEnumClass expectedValue = CustomEnumClass.VAL1;

        // Actually set the key/value pair
        OptionSetter parser = new OptionSetter(object1, object2);
        parser.setOptionMapValue("enumMap", key, value);

        // verify object1
        assertEquals(1, object1.mEnumMap.size());
        assertNotNull(object1.mEnumMap.get(expectedKey));
        assertEquals(expectedValue, object1.mEnumMap.get(expectedKey));

        // verify object2
        assertEquals(1, object2.mEnumMap.size());
        assertNotNull(object2.mEnumMap.get(expectedKey));
        assertEquals(expectedValue, object2.mEnumMap.get(expectedKey));
    }


    /**
     * Test {@link OptionSetter#setOptionValue(String, String)} for Enums used as the key and value
     * of a {@link Map}.
     */
    public void testOptionSetter_sharedEnumCollection() throws ConfigurationException {
        AllTypesOptionSource object1 = new AllTypesOptionSource();
        SharedOptionSource object2 = new SharedOptionSource();

        final String value = "VAL1";
        final DefaultEnumClass expectedValue = DefaultEnumClass.VAL1;

        // Actually add the element
        OptionSetter parser = new OptionSetter(object1, object2);
        parser.setOptionValue("enumCollection", value);

        // verify object1
        assertEquals(1, object1.mEnumCollection.size());
        assertTrue(object1.mEnumCollection.contains(expectedValue));

        // verify object2
        assertEquals(1, object2.mEnumCollection.size());
        assertTrue(object2.mEnumCollection.contains(expectedValue));
    }


    /**
     * Test that multiple options with same name must have the same type.
     */
    public void testOptionSetter_sharedOptionsDiffType() {
        try {
            new OptionSetter(new AllTypesOptionSource(), new SharedOptionWrongTypeSource());
            fail("ConfigurationException not thrown");
        } catch (ConfigurationException e) {
            // expected
        }
    }

    /**
     * Test namespaced options using class names.
     */
    public void testOptionSetter_namespacedClassName() throws ConfigurationException {
        AllTypesOptionSource object1 = new AllTypesOptionSource();
        SharedOptionSource object2 = new SharedOptionSource();
        OptionSetter setter = new OptionSetter(object1, object2);
        setter.setOptionValue(AllTypesOptionSource.class.getName() + ":string", "alltest");
        setter.setOptionValue(SharedOptionSource.class.getName() + ":string", "sharedtest");
        assertEquals("alltest", object1.mString);
        assertEquals("sharedtest", object2.mMyOption);
    }

    /**
     * Test namespaced options using OptionClass aliases
     */
    public void testOptionSetter_namespacedAlias() throws ConfigurationException {
        AllTypesOptionSource object1 = new AllTypesOptionSource();
        SharedOptionSource object2 = new SharedOptionSource();
        OptionSetter setter = new OptionSetter(object1, object2);
        setter.setOptionValue("all:string", "alltest");
        setter.setOptionValue("shared:string", "sharedtest");
        assertEquals("alltest", object1.mString);
        assertEquals("sharedtest", object2.mMyOption);
    }

    /**
     * Test creating an {@link OptionSetter} for a Collection with no type.
     */
    public void testOptionSetter_unparamType() {
        try {
            new OptionSetter(new CollectionTypeOptionSource());
            fail("ConfigurationException not thrown");
        } catch (ConfigurationException e) {
            // expected
        }
    }

    /**
     * Test creating an {@link OptionSetter} for a non collection option with generic type
     */
    public void testOptionSetter_genericType() {
        try {
            new OptionSetter(new NonCollectionGenericTypeOptionSource());
            fail("ConfigurationException not thrown");
        } catch (ConfigurationException e) {
            // expected
        }
    }

    /**
     * Test creating an {@link OptionSetter} for class with inherited options
     */
    public void testOptionSetter_inheritedOptions() throws ConfigurationException {
        ChildOptionSource source = new ChildOptionSource();
        OptionSetter setter = new OptionSetter(source);
        setter.setOptionValue("string", "parent");
        setter.setOptionValue("child-string", "child");
        assertEquals("parent", source.getParentString());
        assertEquals("child", source.mChildString);
    }

    /**
     * Test that options with {@link OptionSetter#NAMESPACE_SEPARATOR} are rejected
     */
    public void testOptionSetter_badOptionName() {
        try {
            new OptionSetter(new BadOptionNameSource());
            fail("ConfigurationException not thrown");
        } catch (ConfigurationException e) {
            // expected
        }
    }

    /**
     * Test {@link OptionSetter#isBooleanOption(String)} when passed an unknown option name
     */
    public void testIsBooleanOption_unknown() throws ConfigurationException {
        OptionSetter parser = new OptionSetter(new AllTypesOptionSource());
        try {
            parser.isBooleanOption("unknown");
            fail("ConfigurationException not thrown");
        } catch (ConfigurationException e) {
            // expected
        }
    }

    /**
     * Test {@link OptionSetter#isBooleanOption(String)} when passed boolean option name
     */
    public void testIsBooleanOption_true() throws ConfigurationException {
        OptionSetter parser = new OptionSetter(new AllTypesOptionSource());
        assertTrue(parser.isBooleanOption("boolean"));
    }

    /**
     * Test {@link OptionSetter#isBooleanOption(String)} when passed boolean option name for a
     * Boolean object
     */
    public void testIsBooleanOption_objTrue() throws ConfigurationException {
        OptionSetter parser = new OptionSetter(new AllTypesOptionSource());
        assertTrue(parser.isBooleanOption("booleanObj"));
    }

    /**
     * Test {@link OptionSetter#isBooleanOption(String)} when passed non-boolean option
     */
    public void testIsBooleanOption_false() throws ConfigurationException {
        OptionSetter parser = new OptionSetter(new AllTypesOptionSource());
        assertFalse(parser.isBooleanOption("string"));
    }

    /**
     * Test {@link OptionSetter#setOptionValue(String, String)} when passed an unknown option name
     */
    public void testSetOptionValue_unknown() throws ConfigurationException {
        OptionSetter parser = new OptionSetter(new AllTypesOptionSource());
        try {
            parser.setOptionValue("unknown", "foo");
            fail("ConfigurationException not thrown");
        } catch (ConfigurationException e) {
            // expected
        }
    }

    /**
     * Test setting a value for a option with an unknown generic type.
     */
    public void testSetOptionValue_unknownType() throws ConfigurationException {
        OptionSetter parser = new OptionSetter(new AllTypesOptionSource());
        try {
            parser.setOptionValue("my_option", "foo");
            fail("ConfigurationException not thrown");
        } catch (ConfigurationException e) {
            // expected
        }
    }

    /**
     * Test setting a value for a non-parameterized Collection
     */
    public void testSetOptionValue_unparameterizedType() throws ConfigurationException {
        OptionSetter parser = new OptionSetter(new AllTypesOptionSource());
        try {
            parser.setOptionValue("my_option", "foo");
            fail("ConfigurationException not thrown");
        } catch (ConfigurationException e) {
            // expected
        }
    }

    /**
     * Test {@link OptionSetter#setOptionValue(String, String)} for a String.
     */
    public void testSetOptionValue_string() throws ConfigurationException {
        AllTypesOptionSource optionSource = new AllTypesOptionSource();
        final String expectedValue = "stringvalue";
        assertSetOptionValue(optionSource, "string", expectedValue);
        assertEquals(expectedValue, optionSource.mString);
    }

    /**
     * Test {@link OptionSetter#setOptionValue(String, String)} for a Collection.
     */
    public void testSetOptionValue_collection() throws ConfigurationException {
        AllTypesOptionSource optionSource = new AllTypesOptionSource();
        final String expectedValue = "stringvalue";
        assertSetOptionValue(optionSource, "string_collection", expectedValue);
        assertEquals(1, optionSource.mStringCollection.size());
        assertTrue(optionSource.mStringCollection.contains(expectedValue));
    }

    /**
     * Test {@link OptionSetter#setOptionValue(String, String)} for a Map.
     */
    public void testSetOptionValue_map() throws ConfigurationException {
        AllTypesOptionSource optionSource = new AllTypesOptionSource();
        final String expectedKey = "stringkey";
        final String expectedValue = "stringvalue";

        // Actually set the key/value pair
        OptionSetter parser = new OptionSetter(optionSource);
        parser.setOptionMapValue("string_string_map", expectedKey, expectedValue);

        assertEquals(1, optionSource.mStringMap.size());
        assertNotNull(optionSource.mStringMap.get(expectedKey));
        assertEquals(expectedValue, optionSource.mStringMap.get(expectedKey));
    }

    /**
     * Test {@link OptionSetter#setOptionValue(String, String)} for a boolean.
     */
    public void testSetOptionValue_boolean() throws ConfigurationException {
        AllTypesOptionSource optionSource = new AllTypesOptionSource();
        assertSetOptionValue(optionSource, "boolean", "true");
        assertEquals(true, optionSource.mBool);
    }

    /**
     * Test {@link OptionSetter#setOptionValue(String, String)} for a boolean for a non-boolean
     * value.
     */
    public void testSetOptionValue_booleanInvalid() {
        AllTypesOptionSource optionSource = new AllTypesOptionSource();
        assertSetOptionValueInvalid(optionSource, "boolean", "blah");
    }

    /**
     * Test {@link OptionSetter#setOptionValue(String, String)} for a Boolean.
     */
    public void testSetOptionValue_booleanObj() throws ConfigurationException {
        AllTypesOptionSource optionSource = new AllTypesOptionSource();
        assertSetOptionValue(optionSource, "booleanObj", "true");
        assertTrue(optionSource.mBooleanObj);
    }

    /**
     * Test {@link OptionSetter#setOptionValue(String, String)} for a byte.
     */
    public void testSetOptionValue_byte() throws ConfigurationException {
        AllTypesOptionSource optionSource = new AllTypesOptionSource();
        assertSetOptionValue(optionSource, "byte", "2");
        assertEquals(2, optionSource.mByte);
    }

    /**
     * Test {@link OptionSetter#setOptionValue(String, String)} for a byte for an invalid value.
     */
    public void testSetOptionValue_byteInvalid() {
        AllTypesOptionSource optionSource = new AllTypesOptionSource();
        assertSetOptionValueInvalid(optionSource, "byte", "blah");
    }

    /**
     * Test {@link OptionSetter#setOptionValue(String, String)} for a Byte.
     */
    public void testSetOptionValue_byteObj() throws ConfigurationException {
        AllTypesOptionSource optionSource = new AllTypesOptionSource();
        assertSetOptionValue(optionSource, "byteObj", "2");
        assertTrue(2 == optionSource.mByteObj);
    }

    /**
     * Test {@link OptionSetter#setOptionValue(String, String)} for a short.
     */
    public void testSetOptionValue_short() throws ConfigurationException {
        AllTypesOptionSource optionSource = new AllTypesOptionSource();
        assertSetOptionValue(optionSource, "short", "2");
        assertTrue(2 == optionSource.mShort);
    }

    /**
     * Test {@link OptionSetter#setOptionValue(String, String)} for a Short.
     */
    public void testSetOptionValue_shortObj() throws ConfigurationException {
        AllTypesOptionSource optionSource = new AllTypesOptionSource();
        assertSetOptionValue(optionSource, "shortObj", "2");
        assertTrue(2 == optionSource.mShortObj);
    }

    /**
     * Test {@link OptionSetter#setOptionValue(String, String)} for a short for an invalid value.
     */
    public void testSetOptionValue_shortInvalid() {
        AllTypesOptionSource optionSource = new AllTypesOptionSource();
        assertSetOptionValueInvalid(optionSource, "short", "blah");
    }

    /**
     * Test {@link OptionSetter#setOptionValue(String, String)} for a int.
     */
    public void testSetOptionValue_int() throws ConfigurationException {
        AllTypesOptionSource optionSource = new AllTypesOptionSource();
        assertSetOptionValue(optionSource, "int", "2");
        assertTrue(2 == optionSource.mInt);
    }

    /**
     * Test {@link OptionSetter#setOptionValue(String, String)} for a Integer.
     */
    public void testSetOptionValue_intObj() throws ConfigurationException {
        AllTypesOptionSource optionSource = new AllTypesOptionSource();
        assertSetOptionValue(optionSource, "intObj", "2");
        assertTrue(2 == optionSource.mIntObj);
    }

    /**
     * Test {@link OptionSetter#setOptionValue(String, String)} for a int for an invalid value.
     */
    public void testSetOptionValue_intInvalid() {
        AllTypesOptionSource optionSource = new AllTypesOptionSource();
        assertSetOptionValueInvalid(optionSource, "int", "blah");
    }

    /**
     * Test {@link OptionSetter#setOptionValue(String, String)} for a long.
     */
    public void testSetOptionValue_long() throws ConfigurationException {
        AllTypesOptionSource optionSource = new AllTypesOptionSource();
        assertSetOptionValue(optionSource, "long", "2");
        assertTrue(2 == optionSource.mLong);
    }

    /**
     * Test {@link OptionSetter#setOptionValue(String, String)} for a Long.
     */
    public void testSetOptionValue_longObj() throws ConfigurationException {
        AllTypesOptionSource optionSource = new AllTypesOptionSource();
        assertSetOptionValue(optionSource, "longObj", "2");
        assertTrue(2 == optionSource.mLongObj);
    }

    /**
     * Test {@link OptionSetter#setOptionValue(String, String)} for a long for an invalid value.
     */
    public void testSetOptionValue_longInvalid() {
        AllTypesOptionSource optionSource = new AllTypesOptionSource();
        assertSetOptionValueInvalid(optionSource, "long", "blah");
    }

    /**
     * Test {@link OptionSetter#setOptionValue(String, String)} for a float.
     */
    public void testSetOptionValue_float() throws ConfigurationException {
        AllTypesOptionSource optionSource = new AllTypesOptionSource();
        assertSetOptionValue(optionSource, "float", "2.1");
        assertEquals(2.1, optionSource.mFloat, 0.01);
    }

    /**
     * Test {@link OptionSetter#setOptionValue(String, String)} for a Float.
     */
    public void testSetOptionValue_floatObj() throws ConfigurationException {
        AllTypesOptionSource optionSource = new AllTypesOptionSource();
        assertSetOptionValue(optionSource, "floatObj", "2.1");
        assertEquals(2.1, optionSource.mFloatObj, 0.01);
    }

    /**
     * Test {@link OptionSetter#setOptionValue(String, String)} for a float for an invalid value.
     */
    public void testSetOptionValue_floatInvalid() {
        AllTypesOptionSource optionSource = new AllTypesOptionSource();
        assertSetOptionValueInvalid(optionSource, "float", "blah");
    }

    /**
     * Test {@link OptionSetter#setOptionValue(String, String)} for a float.
     */
    public void testSetOptionValue_double() throws ConfigurationException {
        AllTypesOptionSource optionSource = new AllTypesOptionSource();
        assertSetOptionValue(optionSource, "double", "2.1");
        assertEquals(2.1, optionSource.mDouble, 0.01);
    }

    /**
     * Test {@link OptionSetter#setOptionValue(String, String)} for a Float.
     */
    public void testSetOptionValue_doubleObj() throws ConfigurationException {
        AllTypesOptionSource optionSource = new AllTypesOptionSource();
        assertSetOptionValue(optionSource, "doubleObj", "2.1");
        assertEquals(2.1, optionSource.mDoubleObj, 0.01);
    }

    /**
     * Test {@link OptionSetter#setOptionValue(String, String)} for a double for an invalid value.
     */
    public void testSetOptionValue_doubleInvalid() {
        AllTypesOptionSource optionSource = new AllTypesOptionSource();
        assertSetOptionValueInvalid(optionSource, "double", "blah");
    }

    /**
     * Test {@link OptionSetter#setOptionValue(String, String)} for a File.
     */
    public void testSetOptionValue_file() throws ConfigurationException, IOException {
        AllTypesOptionSource optionSource = new AllTypesOptionSource();
        File tmpFile = File.createTempFile("testSetOptionValue_file", "txt");
        try {
            assertSetOptionValue(optionSource, "file", tmpFile.getAbsolutePath());
            assertEquals(tmpFile.getAbsolutePath(), optionSource.mFile.getAbsolutePath());
        } finally {
            tmpFile.delete();
        }
    }

    /**
     * Test {@link OptionSetter#setOptionValue(String, String)} for an Enum.
     */
    public void testSetOptionValue_enum() throws ConfigurationException {
        AllTypesOptionSource optionSource = new AllTypesOptionSource();
        assertSetOptionValue(optionSource, "enum", "VAL1");
        assertEquals(DefaultEnumClass.VAL1, optionSource.mEnum);
    }

    /**
     * Test {@link OptionSetter#setOptionValue(String, String)} for an Enum.  Specifically make sure
     * that we fall back properly, so that a mixed-case value will be silently mapped to an
     * uppercase version, since Enum constants tend to be uppercase by convention.
     */
    public void testSetOptionValue_enumMixedCase() throws ConfigurationException {
        AllTypesOptionSource optionSource = new AllTypesOptionSource();
        assertSetOptionValue(optionSource, "enum", "Val1");
        assertEquals(DefaultEnumClass.VAL1, optionSource.mEnum);
    }

    /**
     * Test {@link OptionSetter#setOptionValue(String, String)} for an Enum with custom values.
     */
    public void testSetOptionValue_customEnum() throws ConfigurationException {
        AllTypesOptionSource optionSource = new AllTypesOptionSource();
        assertSetOptionValue(optionSource, "customEnum", "VAL1");
        assertEquals(CustomEnumClass.VAL1, optionSource.mCustomEnum);
        assertEquals(42, optionSource.mCustomEnum.getVal());
    }

    /**
     * Test {@link OptionSetter#setOptionValue(String, String)} for Enums used as the key and value
     * of a {@link Map}.
     */
    public void testSetOptionValue_enumMap() throws ConfigurationException {
        AllTypesOptionSource optionSource = new AllTypesOptionSource();

        final String key = "VAL1";
        final String value = "VAL1";
        final DefaultEnumClass expectedKey = DefaultEnumClass.VAL1;
        final CustomEnumClass expectedValue = CustomEnumClass.VAL1;

        // Actually set the key/value pair
        OptionSetter parser = new OptionSetter(optionSource);
        parser.setOptionMapValue("enumMap", key, value);

        assertEquals(1, optionSource.mEnumMap.size());
        assertNotNull(optionSource.mEnumMap.get(expectedKey));
        assertEquals(expectedValue, optionSource.mEnumMap.get(expectedKey));
    }


    /**
     * Test {@link OptionSetter#setOptionValue(String, String)} for Enums used as the key and value
     * of a {@link Map}.
     */
    public void testSetOptionValue_enumCollection() throws ConfigurationException {
        AllTypesOptionSource optionSource = new AllTypesOptionSource();

        final String value = "VAL1";
        final DefaultEnumClass expectedValue = DefaultEnumClass.VAL1;

        assertSetOptionValue(optionSource, "enumCollection", value);

        assertEquals(1, optionSource.mEnumCollection.size());
        assertTrue(optionSource.mEnumCollection.contains(expectedValue));
    }


    /**
     * Test {@link OptionSetter#setOptionValue(String, String)} for an Enum.
     */
    public void testSetOptionValue_enumBadValue() {
        AllTypesOptionSource optionSource = new AllTypesOptionSource();
        try {
            assertSetOptionValue(optionSource, "enum", "noexist");
            fail("ConfigurationException not thrown");
        } catch (ConfigurationException e) {
            // expected
        }
    }

    /**
     * Make sure that Enum documentation shows the defaults properly
     */
    public void testEnumDocs() throws Exception {
        // We assume here that the fields are returned in declaration order, as documented in the
        // {@link Enum} javadoc.
        String expectedValues = " Valid values: [VAL1, VAL3, VAL2]";
        Field field = AllTypesOptionSource.class.getDeclaredField("mEnum");
        String actualValues = OptionSetter.getEnumFieldValuesAsString(field);
        assertEquals(expectedValues, actualValues);
    }

    /**
     * Test {@link OptionSetter} for a final field
     */
    public void testOptionSetter_finalField() {
        FinalOption optionSource = new FinalOption();
        try {
            new OptionSetter(optionSource);
            fail("ConfigurationException not thrown");
        } catch (ConfigurationException e) {
            // expected
        }
    }

    /**
     * Perform {@link OptionSetter#setOptionValue(String, String)} for a given option.
     */
    private void assertSetOptionValue(AllTypesOptionSource optionSource, final String optionName,
            final String expectedValue) throws ConfigurationException {
        OptionSetter parser = new OptionSetter(optionSource);
        parser.setOptionValue(optionName, expectedValue);
    }

    /**
     * Perform {@link OptionSetter#setOptionValue(String, String)} for a given option, with an
     * invalid value for the option type.
     */
    private void assertSetOptionValueInvalid(AllTypesOptionSource optionSource,
            final String optionName, final String expectedValue) {
        try {
            assertSetOptionValue(optionSource, optionName, expectedValue);
            fail("ConfigurationException not thrown");
        } catch (ConfigurationException e) {
            // expected
        }
    }
}
