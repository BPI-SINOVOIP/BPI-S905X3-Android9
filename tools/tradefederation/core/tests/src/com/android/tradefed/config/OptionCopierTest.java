/*
 * Copyright (C) 2012 The Android Open Source Project
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

package com.android.tradefed.config;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.Map;

/** Unit tests for {@link OptionCopier}. */
@RunWith(JUnit4.class)
public class OptionCopierTest {

    private static enum DefaultEnumClass {
        VAL1, VAL3, VAL2;
    }

    private static class OptionSource {
        @Option(name = "string", shortName = 's')
        private String mMyOption = "value";

        @Option(name = "int")
        private int mMyIntOption = 42;

        @SuppressWarnings("unused")
        @Option(name = "notInDest")
        private String mMyNotInDestOption = "foo";

        @Option(name = "string_collection")
        private Collection<String> mStringCollection = new ArrayList<String>();

        @SuppressWarnings("unused")
        @Option(name = "enum")
        private DefaultEnumClass mEnum = DefaultEnumClass.VAL1;

        @Option(name = "enum_map")
        private Map<DefaultEnumClass, DefaultEnumClass> mEnumMap =
                new HashMap<DefaultEnumClass, DefaultEnumClass>();

        @Option(name = "enum_collection")
        private Collection<DefaultEnumClass> mEnumCollection =
                new ArrayList<DefaultEnumClass>();
    }

    /**
     * Option source with an option with same name as OptionSource, but a different type.
     */
    private static class OptionWrongTypeDest {
        @Option(name = "string", shortName = 's')
        private int mMyOption;
    }

    /**
     * Option source with an option with same name as OptionSource, but only the primitive type of
     * the collection.
     */
    private static class OptionCollectionWrongTypeDest {
        @Option(name = "string_collection", shortName = 's')
        private String mString = null;
    }

    /**
     * Option source with an option with same name as OptionSource, and with a different primitive
     * type of the collection.
     */
    private static class OptionCollectionWrongPrimitiveTypeDest {
        @Option(name = "string_collection", shortName = 's')
        private int mmyOption;
    }

    /** Option source with a non-initialized collection. */
    private static class OptionCollectionNull {
        @Option(name = "string_collection", shortName = 's')
        private Collection<String> mStringCollection = null;
    }

    /**
     * Option source with an option with same name as OptionSource, but a different type.
     */
    @SuppressWarnings("unused")
    private static class OptionWrongGenericTypeDest {

        @Option(name = "string_collection")
        private Collection<Integer> mIntCollection = new ArrayList<Integer>();

    }

    private static class OptionDest {

        @Option(name = "string", shortName = 's')
        private String mDestOption;

        @Option(name = "int")
        private int mDestIntOption;

        @Option(name = "string_collection")
        private Collection<String> mStringDestCollection = new ArrayList<String>();

        @Option(name = "enum")
        private DefaultEnumClass mEnum = null;

        @Option(name = "enum_map")
        private Map<DefaultEnumClass, DefaultEnumClass> mEnumMap =
                new HashMap<DefaultEnumClass, DefaultEnumClass>();

        @Option(name = "enum_collection")
        private Collection<DefaultEnumClass> mEnumCollection =
                new ArrayList<DefaultEnumClass>();
    }

    /** Test success case for {@link OptionCopier} using String fields */
    @Test
    public void testCopyOptions_string() throws ConfigurationException {
        OptionSource source = new OptionSource();
        OptionDest dest = new OptionDest();
        OptionCopier.copyOptions(source, dest);
        assertEquals(source.mMyOption, dest.mDestOption);
    }

    /** Test success case for {@link OptionCopier} for an int field */
    @Test
    public void testCopyOptions_int() throws ConfigurationException {
        OptionSource source = new OptionSource();
        OptionDest dest = new OptionDest();
        OptionCopier.copyOptions(source, dest);
        assertEquals(source.mMyIntOption, dest.mDestIntOption);
    }

    /** Test success case for {@link OptionCopier} for a {@link Collection}. */
    @Test
    public void testCopyOptions_collection() throws ConfigurationException {
        OptionSource source = new OptionSource();
        source.mStringCollection.add("foo");
        OptionDest dest = new OptionDest();
        dest.mStringDestCollection.add("bar");
        OptionCopier.copyOptions(source, dest);
        assertEquals(2, dest.mStringDestCollection.size());
        assertTrue(dest.mStringDestCollection.contains("foo"));
        assertTrue(dest.mStringDestCollection.contains("bar"));
    }

    /** Test success case for {@link OptionCopier} for an enum field */
    @Test
    public void testCopyOptions_enum() throws ConfigurationException {
        OptionSource source = new OptionSource();
        OptionDest dest = new OptionDest();
        OptionCopier.copyOptions(source, dest);
        assertEquals(DefaultEnumClass.VAL1, dest.mEnum);
    }

    /** Test success case for {@link OptionCopier} for an enum {@link Collection}. */
    @Test
    public void testCopyOptions_enumCollection() throws ConfigurationException {
        OptionSource source = new OptionSource();
        source.mEnumCollection.add(DefaultEnumClass.VAL2);
        OptionDest dest = new OptionDest();
        source.mEnumCollection.add(DefaultEnumClass.VAL3);
        OptionCopier.copyOptions(source, dest);
        assertEquals(2, dest.mEnumCollection.size());
        assertTrue(dest.mEnumCollection.contains(DefaultEnumClass.VAL2));
        assertTrue(dest.mEnumCollection.contains(DefaultEnumClass.VAL3));
    }

    /** Test success case for {@link OptionCopier} for an enum {@link Map}. */
    @Test
    public void testCopyOptions_enumMap() throws ConfigurationException {
        OptionSource source = new OptionSource();
        source.mEnumMap.put(DefaultEnumClass.VAL1, DefaultEnumClass.VAL2);
        OptionDest dest = new OptionDest();
        dest.mEnumMap.put(DefaultEnumClass.VAL2, DefaultEnumClass.VAL3);
        OptionCopier.copyOptions(source, dest);
        assertEquals(2, dest.mEnumMap.size());
        assertEquals(DefaultEnumClass.VAL2, dest.mEnumMap.get(DefaultEnumClass.VAL1));
    }

    /** Test {@link OptionCopier} when field's to be copied have different types */
    @Test
    public void testCopyOptions_wrongType() {
        OptionSource source = new OptionSource();
        OptionWrongTypeDest dest = new OptionWrongTypeDest();
        try {
            OptionCopier.copyOptions(source, dest);
            fail("ConfigurationException not thrown");
        } catch (ConfigurationException e) {
            // expected
        }
    }

    /** Copying a collection to primitive type will fail. */
    @Test
    public void testCopyOptions_collectionToPrimitive() {
        OptionSource source = new OptionSource();
        OptionCollectionWrongTypeDest dest = new OptionCollectionWrongTypeDest();
        try {
            OptionCopier.copyOptions(source, dest);
            fail("ConfigurationException not thrown");
        } catch (ConfigurationException e) {
            // expected
            assertEquals("internal error when setting option 'string_collection'", e.getMessage());
        }
    }

    /** Copying a null initialized primitive value to collection will fail. */
    @Test
    public void testCopyOptions_primitiveToCollection_null() {
        OptionCollectionWrongTypeDest source = new OptionCollectionWrongTypeDest();
        OptionSource dest = new OptionSource();
        try {
            OptionCopier.copyOptions(source, dest);
            fail("ConfigurationException not thrown");
        } catch (ConfigurationException e) {
            // expected
            assertEquals(
                    "Value 'null' is not of type 'class java.lang.String' like the Collection.",
                    e.getMessage());
        }
    }

    /**
     * Copying an initialized primitive value to collection will succeed since the type is
     * compatible.
     */
    @Test
    public void testCopyOptions_primitiveToCollection() throws Exception {
        OptionCollectionWrongTypeDest source = new OptionCollectionWrongTypeDest();
        OptionSetter setter = new OptionSetter(source);
        setter.setOptionValue("string_collection", "not_null");
        OptionSource dest = new OptionSource();
        OptionCopier.copyOptions(source, dest);
        assertEquals(1, dest.mStringCollection.size());
        assertEquals("not_null", dest.mStringCollection.iterator().next());
    }

    /**
     * Copying a primitive value different from the collection type will fail since we do not want
     * to mix types in a collection.
     */
    @Test
    public void testCopyOptions_primitiveToCollection_differentPrimitive() throws Exception {
        OptionCollectionWrongPrimitiveTypeDest source =
                new OptionCollectionWrongPrimitiveTypeDest();
        OptionSetter setter = new OptionSetter(source);
        setter.setOptionValue("string_collection", "5");
        OptionSource dest = new OptionSource();
        try {
            OptionCopier.copyOptions(source, dest);
            fail("ConfigurationException not thrown");
        } catch (ConfigurationException e) {
            // expected
            assertEquals(
                    "Value '5' is not of type 'class java.lang.String' like the Collection.",
                    e.getMessage());
        }
    }

    /** Copying a non-initialized collection (null) to a collection results in an exception. */
    @Test
    public void testCopyOptions_nullCollection() throws Exception {
        OptionCollectionNull source = new OptionCollectionNull();
        OptionSource dest = new OptionSource();
        try {
            OptionCopier.copyOptions(source, dest);
            fail("ConfigurationException not thrown");
        } catch (ConfigurationException e) {
            // expected
            assertEquals(
                    "Value 'null' is not of type 'class java.lang.String' like the Collection.",
                    e.getMessage());
        }
    }
}
