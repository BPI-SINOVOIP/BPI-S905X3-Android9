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
package com.android.tradefed.config;

import com.android.tradefed.config.Option.Importance;
import com.android.tradefed.util.keystore.IKeyStoreClient;
import com.android.tradefed.util.keystore.StubKeyStoreClient;

import junit.framework.TestCase;

import org.easymock.EasyMock;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Unit tests for {@link ArgsOptionParser}.
 */
@SuppressWarnings("unused")
public class ArgsOptionParserTest extends TestCase {

    /**
     * An option source with one {@link Option} specified.
     */
    private static class OneOptionSource {

        private static final String DEFAULT_VALUE = "default";
        private static final String OPTION_NAME = "my_option";
        private static final String OPTION_DESC = "option description";

        @Option(name=OPTION_NAME, shortName='o', description=OPTION_DESC)
        private String mMyOption = DEFAULT_VALUE;
    }

    /**
     * An option source with one {@link Option} specified.
     */
    private static class MapOptionSource {

        private static final String OPTION_NAME = "my_option";
        private static final String OPTION_DESC = "option description";

        @Option(name=OPTION_NAME, shortName='o', description=OPTION_DESC)
        private Map<Integer, Boolean> mMyOption = new HashMap<Integer, Boolean>();
    }

    /**
     * An option source with one {@link Option} specified.
     */
    private static class MapStringOptionSource {

        private static final String OPTION_NAME = "my_option";
        private static final String OPTION_DESC = "option description";

        @Option(name=OPTION_NAME, shortName='o', description=OPTION_DESC)
        private Map<String, String> mMyOption = new HashMap<String, String>();
    }

    /**
     * An option source with boolean {@link Option} specified.
     */
    private static class BooleanOptionSource {

        private static final boolean DEFAULT_BOOL = false;
        private static final String DEFAULT_VALUE = "default";

        @Option(name="my_boolean", shortName='b')
        private boolean mMyBool = DEFAULT_BOOL;

        @Option(name="my_option", shortName='o')
        protected String mMyOption = DEFAULT_VALUE;
    }

    /**
     * An option source with boolean {@link Option} specified with default = true.
     */
    private static class BooleanTrueOptionSource {

        private static final boolean DEFAULT_BOOL = true;

        @Option(name="my_boolean", shortName='b')
        private boolean mMyBool = DEFAULT_BOOL;
    }

    /**
     * An option source that has a superclass with options
     */
    private static class InheritedOptionSource extends OneOptionSource {

        private static final String OPTION_NAME = "my_sub_option";
        private static final String OPTION_DESC = "sub description";

        @Option(name=OPTION_NAME, description=OPTION_DESC)
        private String mMySubOption = "";
    }

    /**
     * An option source for testing the {@link Option#importance()} settings
     */
    private static class ImportantOptionSource {

        private static final String IMPORTANT_OPTION_NAME = "important_option";
        private static final String IMPORTANT_UNSET_OPTION_NAME = "unset_important_option";
        private static final String UNIMPORTANT_OPTION_NAME = "unimportant_option";

        @Option(name = IMPORTANT_OPTION_NAME, description = IMPORTANT_OPTION_NAME,
                importance = Importance.ALWAYS)
        private String mImportantOption = "foo";

        @Option(name = IMPORTANT_UNSET_OPTION_NAME, description = IMPORTANT_UNSET_OPTION_NAME,
                importance = Importance.IF_UNSET)
        private String mImportantUnsetOption = null;

        @Option(name = UNIMPORTANT_OPTION_NAME, description = UNIMPORTANT_OPTION_NAME,
                importance = Importance.NEVER)
        private String mUnimportantOption = null;

        ImportantOptionSource(String setOption) {
            mImportantUnsetOption = setOption;
        }

        ImportantOptionSource() {
        }
    }

    /**
     * Option source whose options shouldn't end up in the global namespace
     */
    @OptionClass(alias = "ngos", global_namespace = false)
    private static class NonGlobalOptionSource {
        @Option(name = "option")
        Boolean mOption = null;
    }

    /**
     * Option source with mandatory options
     */
    private static class MandatoryOptionSourceNoDefault {
        @Option(name = "no-default", mandatory = true)
        private String mNoDefaultOption;
    }

    /**
     * Option source with mandatory options
     */
    private static class MandatoryOptionSourceNull {
        @Option(name = "null", mandatory = true)
        private String mNullOption = null;
    }

    /**
     * Option source with mandatory options
     */
    private static class MandatoryOptionSourceEmptyCollection {
        @Option(name = "empty-collection", mandatory = true)
        private Collection<String> mEmptyCollection = new ArrayList<String>(0);
    }

    /**
     * Option source with mandatory options
     */
    private static class MandatoryOptionSourceEmptyMap {
        @Option(name = "empty-map", mandatory = true)
        private Map<String, String> mEmptyMap = new HashMap<String, String>();
    }

    /**
     * An option source that exercises the {@link OptionUpdateRule}s.
     */
    private static class OptionUpdateRuleSource {

        public static final String DEFAULT_VALUE = "5 default";
        public static final String BIGGER_VALUE = "9 bigger";
        public static final String SMALLER_VALUE = "0 smaller";

        @Option(name = "default")
        private String mDefaultOption = DEFAULT_VALUE;

        @Option(name = "first", updateRule = OptionUpdateRule.FIRST)
        private String mFirstOption = DEFAULT_VALUE;

        @Option(name = "last", updateRule = OptionUpdateRule.LAST)
        private String mLastOption = DEFAULT_VALUE;

        @Option(name = "greatest", updateRule = OptionUpdateRule.GREATEST)
        private String mGreatestOption = DEFAULT_VALUE;

        @Option(name = "least", updateRule = OptionUpdateRule.LEAST)
        private String mLeastOption = DEFAULT_VALUE;

        @Option(name = "immutable", updateRule = OptionUpdateRule.IMMUTABLE)
        private String mImmutableOption = DEFAULT_VALUE;

        @Option(name = "null-immutable", updateRule = OptionUpdateRule.IMMUTABLE)
        private String mNullImmutableOption = null;
    }


    // SECTION: option update rule validation
    /**
     * Verify that {@link OptionUpdateRule}s work properly when the update compares to greater-than
     * the default value.
     */
    public void testOptionUpdateRule_greater() throws Exception {
        OptionUpdateRuleSource object = new OptionUpdateRuleSource();
        ArgsOptionParser parser = new ArgsOptionParser(object);
        final String current = OptionUpdateRuleSource.DEFAULT_VALUE;
        final String big = OptionUpdateRuleSource.BIGGER_VALUE;

        parser.parse(new String[] {"--default", big, "--first", big, "--last", big,
                "--greatest", big, "--least", big});
        assertEquals(current, object.mFirstOption);
        assertEquals(big, object.mLastOption);
        assertEquals(big, object.mDefaultOption);  // default should be LAST
        assertEquals(big, object.mGreatestOption);
        assertEquals(current, object.mLeastOption);
    }

    /**
     * Verify that {@link OptionUpdateRule}s work properly when the update compares to greater-than
     * the default value.
     */
    public void testOptionUpdateRule_lesser() throws Exception {
        OptionUpdateRuleSource object = new OptionUpdateRuleSource();
        ArgsOptionParser parser = new ArgsOptionParser(object);
        final String current = OptionUpdateRuleSource.DEFAULT_VALUE;
        final String small = OptionUpdateRuleSource.SMALLER_VALUE;

        parser.parse(new String[] {"--default", small, "--first", small, "--last", small,
                "--greatest", small, "--least", small});
        assertEquals(current, object.mFirstOption);
        assertEquals(small, object.mLastOption);
        assertEquals(small, object.mDefaultOption);  // default should be LAST
        assertEquals(current, object.mGreatestOption);
        assertEquals(small, object.mLeastOption);
    }

    /**
     * Verify that {@link OptionUpdateRule}s work properly when the update compares to greater-than
     * the default value.
     */
    public void testOptionUpdateRule_immutable() throws Exception {
        OptionUpdateRuleSource object = new OptionUpdateRuleSource();
        ArgsOptionParser parser = new ArgsOptionParser(object);
        final String update = OptionUpdateRuleSource.BIGGER_VALUE;

        try {
            parser.parse(new String[] {"--immutable", update});
            fail("ConfigurationException not thrown when updating an IMMUTABLE option");
        } catch (ConfigurationException e) {
            // expected
        }

        assertNull(object.mNullImmutableOption);
        parser.parse(new String[] {"--null-immutable", update});
        assertEquals(update, object.mNullImmutableOption);

        try {
            parser.parse(new String[] {"--null-immutable", update});
            fail("ConfigurationException not thrown when updating an IMMUTABLE option");
        } catch (ConfigurationException e) {
            // expected
        }
    }

    /**
     * Setting an option with a namespace alias should work fine
     */
    public void testNonGlobalOptionSource_alias() throws Exception {
        NonGlobalOptionSource source = new NonGlobalOptionSource();
        ArgsOptionParser parser = new ArgsOptionParser(source);

        assertNull(source.mOption);
        parser.parse(new String[] {"--ngos:option"});
        assertTrue(source.mOption);
        parser.parse(new String[] {"--ngos:no-option"});
        assertFalse(source.mOption);
    }

    /**
     * Setting an option with a classname namespace should work fine
     */
    public void testNonGlobalOptionSource_className() throws Exception {
        NonGlobalOptionSource source = new NonGlobalOptionSource();
        ArgsOptionParser parser = new ArgsOptionParser(source);

        assertNull(source.mOption);
        parser.parse(new String[] {String.format("--%s:option", source.getClass().getName())});
        assertTrue(source.mOption);
        parser.parse(new String[] {String.format("--%s:no-option", source.getClass().getName())});
        assertFalse(source.mOption);
    }

    /**
     * Setting an option without a namespace should fail
     */
    public void testNonGlobalOptionSource_global() throws Exception {
        NonGlobalOptionSource source = new NonGlobalOptionSource();
        ArgsOptionParser parser = new ArgsOptionParser(source);

        assertNull(source.mOption);
        try {
            parser.parse(new String[] {"--option"});
            fail("ConfigurationException not thrown when assigning a global option to an @Option " +
                    "field in a non-global-namespace class");
        } catch (ConfigurationException e) {
            // expected
        }

        try {
            parser.parse(new String[] {"--no-option"});
            fail("ConfigurationException not thrown when assigning a global option to an @Option " +
                    "field in a non-global-namespace class");
        } catch (ConfigurationException e) {
            // expected
        }
    }


    // SECTION: tests for #parse(...)
    /**
    * Test passing an empty argument list for an object that has one option specified.
    * <p/>
    * Expected that the option field should retain its default value.
    */
    public void testParse_noArg() throws ConfigurationException {
        OneOptionSource object = new OneOptionSource();
        ArgsOptionParser parser = new ArgsOptionParser(object);
        parser.parse(new String[] {});
        assertEquals(OneOptionSource.DEFAULT_VALUE, object.mMyOption);
    }

    /**
     * Test passing an single argument for an object that has one option specified.
     */
    public void testParse_oneArg() throws ConfigurationException {
        OneOptionSource object = new OneOptionSource();
        ArgsOptionParser parser = new ArgsOptionParser(object);
        final String expectedValue = "set";
        parser.parse(new String[] {"--my_option", expectedValue});
        assertEquals(expectedValue, object.mMyOption);
    }

    /**
     * Test passing an single argument for an object that has one option specified.
     */
    public void testParse_oneMapArg() throws ConfigurationException {
        MapOptionSource object = new MapOptionSource();
        ArgsOptionParser parser = new ArgsOptionParser(object);
        final int expectedKey = 13;
        final boolean expectedValue = true;
        parser.parse(new String[] {"--my_option", Integer.toString(expectedKey),
                Boolean.toString(expectedValue)});
        assertNotNull(object.mMyOption);
        assertEquals(1, object.mMyOption.size());
        assertEquals(expectedValue, (boolean) object.mMyOption.get(expectedKey));
    }

    /**
     * Test passing an single argument for an object that has one option specified.
     */
    public void testParseMapArg_mismatchKeyType() throws ConfigurationException {
        MapOptionSource object = new MapOptionSource();
        ArgsOptionParser parser = new ArgsOptionParser(object);
        final String expectedKey = "istanbul";
        final boolean expectedValue = true;
        try {
            parser.parse(new String[] {"--my_option", expectedKey, Boolean.toString(expectedValue)});
            fail("ConfigurationException not thrown");
        } catch (ConfigurationException e) {
            // expect an exception that explicitly mentions that the "key" is incorrect
            assertTrue(String.format("Expected exception message to contain 'key': %s",
                    e.getMessage()), e.getMessage().contains("key"));
            assertTrue(String.format("Expected exception message to contain '%s': %s",
                    expectedKey, e.getMessage()), e.getMessage().contains(expectedKey));
        }
    }

    /**
     * Test passing an single argument for an object that has one option specified.
     */
    public void testParseMapArg_mismatchValueType() throws ConfigurationException {
        MapOptionSource object = new MapOptionSource();
        ArgsOptionParser parser = new ArgsOptionParser(object);
        final int expectedKey = 13;
        final String expectedValue = "notconstantinople";
        try {
            parser.parse(new String[] {"--my_option", Integer.toString(expectedKey), expectedValue});
            fail("ConfigurationException not thrown");
        } catch (ConfigurationException e) {
            // expect an exception that explicitly mentions that the "value" is incorrect
            assertTrue(String.format("Expected exception message to contain 'value': '%s'",
                    e.getMessage()), e.getMessage().contains("value"));
            assertTrue(String.format("Expected exception message to contain '%s': %s",
                    expectedValue, e.getMessage()), e.getMessage().contains(expectedValue));
        }
    }

    /**
     * Test passing an single argument for an object that has one option specified.
     */
    public void testParseMapArg_missingKey() throws ConfigurationException {
        MapOptionSource object = new MapOptionSource();
        ArgsOptionParser parser = new ArgsOptionParser(object);
        try {
            parser.parse(new String[] {"--my_option"});
            fail("ConfigurationException not thrown");
        } catch (ConfigurationException e) {
            // expect an exception that explicitly mentions that the "key" is incorrect
            assertTrue(String.format("Expected exception message to contain 'key': '%s'",
                    e.getMessage()), e.getMessage().contains("key"));
        }
    }

    /**
     * Test passing an single argument for an object that has one option specified.
     */
    public void testParseMapArg_missingValue() throws ConfigurationException {
        MapOptionSource object = new MapOptionSource();
        ArgsOptionParser parser = new ArgsOptionParser(object);
        final int expectedKey = 13;
        try {
            parser.parse(new String[] {"--my_option", Integer.toString(expectedKey)});
            fail("ConfigurationException not thrown");
        } catch (ConfigurationException e) {
            // expect an exception that explicitly mentions that the "value" is incorrect
            assertTrue(String.format("Expected exception message to contain 'value': '%s'",
                    e.getMessage()), e.getMessage().contains("value"));
        }
    }

    /**
     * Test passing an single argument for an object that has one option specified, using the
     * option=value notation.
     */
    public void testParse_oneArgEquals() throws ConfigurationException {
        OneOptionSource object = new OneOptionSource();
        ArgsOptionParser parser = new ArgsOptionParser(object);
        final String expectedValue = "set";
        parser.parse(new String[] {String.format("--my_option=%s", expectedValue)});
        assertEquals(expectedValue, object.mMyOption);
    }

    /**
     * Test passing a single argument for an object that has one option specified, using the
     * short option notation.
     */
    public void testParse_oneShortArg() throws ConfigurationException {
        OneOptionSource object = new OneOptionSource();
        ArgsOptionParser parser = new ArgsOptionParser(object);
        final String expectedValue = "set";
        parser.parse(new String[] {"-o", expectedValue});
        assertEquals(expectedValue, object.mMyOption);
    }

    /**
     * Test that "--" marks the beginning of positional arguments
     */
    public void testParse_posArgs() throws ConfigurationException {
        OneOptionSource object = new OneOptionSource();
        ArgsOptionParser parser = new ArgsOptionParser(object);
        final String expectedValue = "set";
        // have a position argument with a long option prefix, to try to confuse the parser
        final String posArg = "--unused";
        List<String> leftOver = parser.parse(new String[] {"-o", expectedValue, "--", posArg});
        assertEquals(expectedValue, object.mMyOption);
        assertTrue(leftOver.contains(posArg));
    }

    /**
     * Test passing a single boolean argument.
     */
    public void testParse_boolArg() throws ConfigurationException {
        BooleanOptionSource object = new BooleanOptionSource();
        ArgsOptionParser parser = new ArgsOptionParser(object);
        parser.parse(new String[] {"-b"});
        assertTrue(object.mMyBool);
    }

    /**
     * Test passing a boolean argument with another short argument.
     */
    public void testParse_boolTwoArg() throws ConfigurationException {
        BooleanOptionSource object = new BooleanOptionSource();
        ArgsOptionParser parser = new ArgsOptionParser(object);
        final String expectedValue = "set";
        parser.parse(new String[] {"-bo", expectedValue});
        assertTrue(object.mMyBool);
        assertEquals(expectedValue, object.mMyOption);
    }

    /**
     * Test passing a boolean argument with another short argument, with value concatenated.
     * e.g -bovalue
     */
    public void testParse_boolTwoArgValue() throws ConfigurationException {
        BooleanOptionSource object = new BooleanOptionSource();
        ArgsOptionParser parser = new ArgsOptionParser(object);
        final String expectedValue = "set";
        parser.parse(new String[] {String.format("-bo%s", expectedValue)});
        assertTrue(object.mMyBool);
        assertEquals(expectedValue, object.mMyOption);
    }

    /**
     * Test the "--no-(bool option)" syntax
     */
    public void testParse_boolFalse() throws ConfigurationException {
        BooleanTrueOptionSource object = new BooleanTrueOptionSource();
        ArgsOptionParser parser = new ArgsOptionParser(object);
        parser.parse(new String[] {"--no-my_boolean"});
        assertFalse(object.mMyBool);
    }

    /**
     * Test the boolean long option syntax
     */
    public void testParse_boolLong() throws ConfigurationException {
        BooleanOptionSource object = new BooleanOptionSource();
        ArgsOptionParser parser = new ArgsOptionParser(object);
        parser.parse(new String[] {"--my_boolean"});
        assertTrue(object.mMyBool);
    }

    /**
     * Test passing arg string where value is missing
     */
    public void testParse_missingValue() throws ConfigurationException {
        OneOptionSource object = new OneOptionSource();
        ArgsOptionParser parser = new ArgsOptionParser(object);
        try {
            parser.parse(new String[] {"--my_option"});
            fail("ConfigurationException not thrown");
        } catch (ConfigurationException e) {
            // expected
        }
    }

    /**
     * Test parsing args for an option that does not exist.
     */
    public void testParse_optionNotPresent() throws ConfigurationException {
        OneOptionSource object = new OneOptionSource();
        ArgsOptionParser parser = new ArgsOptionParser(object);
        try {
            parser.parse(new String[] {"--my_option", "set", "--not_here", "value"});
            fail("ConfigurationException not thrown");
        } catch (ConfigurationException e) {
            // expected
        }
    }


    // SECTION: tests for #parseBestEffort(...)
    /**
     * Test passing an single argument for an object that has one option specified.
     */
    public void testParseBestEffort_oneArg() throws ConfigurationException {
        OneOptionSource object = new OneOptionSource();
        ArgsOptionParser parser = new ArgsOptionParser(object);
        final String option = "--my_option";
        final String value = "set";
        final List<String> leftovers = parser.parseBestEffort(
                new String[] {option, value});
        assertEquals(value, object.mMyOption);
        assertEquals(0, leftovers.size());
    }

    /**
     * Make sure that overwriting arguments works as expected.
     */
    public void testParseBestEffort_oneArg_overwrite() throws ConfigurationException {
        OneOptionSource object = new OneOptionSource();
        ArgsOptionParser parser = new ArgsOptionParser(object);
        final String option = "--my_option";
        final String value1 = "set";
        final String value2 = "game";
        final List<String> leftovers = parser.parseBestEffort(
                new String[] {option, value1, option, value2});
        assertEquals(value2, object.mMyOption);
        assertEquals(0, leftovers.size());
    }

    /**
     * Test passing a usable argument followed by an unusable one.
     */
    public void testParseBestEffort_oneArg_oneLeftover() throws ConfigurationException {
        OneOptionSource object = new OneOptionSource();
        ArgsOptionParser parser = new ArgsOptionParser(object);
        final String expectedValue = "set";
        final String leftoverOption = "--no_exist";
        final List<String> leftovers = parser.parseBestEffort(
                new String[] {"--my_option", expectedValue, leftoverOption});
        assertEquals(expectedValue, object.mMyOption);
        assertEquals(1, leftovers.size());
        assertEquals(leftoverOption, leftovers.get(0));
    }

    /**
     * Test passing an unusable argument followed by a usable one.  Basically verifies that the
     * parse attempt stops wholesale, and doesn't merely skip the unusable arg.
     */
    public void testParseBestEffort_oneLeftover_oneArg() throws ConfigurationException {
        OneOptionSource object = new OneOptionSource();
        ArgsOptionParser parser = new ArgsOptionParser(object);
        final String goodOption = "--my_option";
        final String value = "set";
        final String badOption = "--no_exist";
        final List<String> leftovers = parser.parseBestEffort(
                new String[] {badOption, goodOption, value});
        assertEquals(OneOptionSource.DEFAULT_VALUE, object.mMyOption);
        assertEquals(3, leftovers.size());
        assertEquals(badOption, leftovers.get(0));
        assertEquals(goodOption, leftovers.get(1));
        assertEquals(value, leftovers.get(2));
    }

    /**
     * Make sure that parsing stops when a bare option prefix, "--", is encountered.  That prefix
     * should _not_ be returned as one of the leftover args.
     */
    public void testParseBestEffort_manualStop() throws ConfigurationException {
        OneOptionSource object = new OneOptionSource();
        ArgsOptionParser parser = new ArgsOptionParser(object);
        final String goodOption = "--my_option";
        final String value = "set";
        final String badOption = "--no_exist";
        final List<String> leftovers = parser.parseBestEffort(
                new String[] {"--", badOption, goodOption, value});
        assertEquals(OneOptionSource.DEFAULT_VALUE, object.mMyOption);
        assertEquals(3, leftovers.size());
        assertEquals(badOption, leftovers.get(0));
        assertEquals(goodOption, leftovers.get(1));
        assertEquals(value, leftovers.get(2));
    }

    /**
     * Make sure that parsing stops when a bare word is encountered.  Unlike a bare option prefix,
     * the bare word _should_ be returned as one of the leftover args.
     */
    public void testParseBestEffort_bareWord() throws ConfigurationException {
        OneOptionSource object = new OneOptionSource();
        ArgsOptionParser parser = new ArgsOptionParser(object);
        final String goodOption = "--my_option";
        final String value = "set";
        final String badOption = "--no_exist";
        final String bareWord = "configName";
        final List<String> leftovers = parser.parseBestEffort(
                new String[] {bareWord, badOption, goodOption, value});
        assertEquals(OneOptionSource.DEFAULT_VALUE, object.mMyOption);
        assertEquals(4, leftovers.size());
        assertEquals(bareWord, leftovers.get(0));
        assertEquals(badOption, leftovers.get(1));
        assertEquals(goodOption, leftovers.get(2));
        assertEquals(value, leftovers.get(3));
    }

    /**
     * Make sure that parsing stops when a bare option prefix, "--", is encountered.  That prefix
     * should _not_ be returned as one of the leftover args.
     */
    public void testParseBestEffort_oneArg_manualStop() throws ConfigurationException {
        OneOptionSource object = new OneOptionSource();
        ArgsOptionParser parser = new ArgsOptionParser(object);
        final String option = "--my_option";
        final String value1 = "set";
        final String value2 = "game";
        final List<String> leftovers = parser.parseBestEffort(
                new String[] {option, value1, "--", option, value2});
        assertEquals(value1, object.mMyOption);
        assertEquals(2, leftovers.size());
        assertEquals(option, leftovers.get(0));
        assertEquals(value2, leftovers.get(1));
    }

    /**
     * Make sure that parsing stops when a bare word is encountered.  Unlike a bare option prefix,
     * the bare word _should_ be returned as one of the leftover args.
     */
    public void testParseBestEffort_oneArg_bareWord() throws ConfigurationException {
        OneOptionSource object = new OneOptionSource();
        ArgsOptionParser parser = new ArgsOptionParser(object);
        final String option = "--my_option";
        final String value1 = "set";
        final String value2 = "game";
        final String bareWord = "configName";
        final List<String> leftovers = parser.parseBestEffort(
                new String[] {option, value1, bareWord, option, value2});
        assertEquals(value1, object.mMyOption);
        assertEquals(3, leftovers.size());
        assertEquals(bareWord, leftovers.get(0));
        assertEquals(option, leftovers.get(1));
        assertEquals(value2, leftovers.get(2));
    }

    /**
     * Test passing an single argument for an object that has one option specified.
     */
    public void testParseBestEffort_oneArg_twoLeftovers() throws ConfigurationException {
        OneOptionSource object = new OneOptionSource();
        ArgsOptionParser parser = new ArgsOptionParser(object);
        final String expectedValue = "set";
        final String leftover1 = "--no_exist";
        final String leftover2 = "--me_neither";
        final List<String> leftovers = parser.parseBestEffort(
                new String[] {"--my_option", expectedValue, leftover1, leftover2});
        assertEquals(expectedValue, object.mMyOption);
        assertEquals(2, leftovers.size());
        assertEquals(leftover1, leftovers.get(0));
        assertEquals(leftover2, leftovers.get(1));
    }

    /**
     * Make sure that map option parsing works as expected.
     */
    public void testParseBestEffort_mapOption() throws ConfigurationException {
        MapOptionSource object = new MapOptionSource();
        ArgsOptionParser parser = new ArgsOptionParser(object);
        final String option = "--my_option";
        final String key = "123";  // Integer is the key type
        final String value = "true";  // Boolean is the value type
        final String key2 = "345";  // Integer is the key type
        final String value2 = "false";  // Boolean is the value type
        final Integer expKey = 123;
        final Boolean expValue = Boolean.TRUE;
        final Integer expKey2 = 345;
        final Boolean expValue2 = Boolean.FALSE;

        final List<String> leftovers = parser.parseBestEffort(
                new String[] {option, key, value, option, key2, value2});

        assertEquals(0, leftovers.size());
        assertNotNull(object.mMyOption);
        assertEquals(2, object.mMyOption.size());
        assertTrue(object.mMyOption.containsKey(expKey));
        assertEquals(expValue, object.mMyOption.get(expKey));
        assertTrue(object.mMyOption.containsKey(expKey2));
        assertEquals(expValue2, object.mMyOption.get(expKey2));
    }

    /**
     * Make sure that the single value map option parsing works as expected.
     */
    public void testParseBestEffort_mapOption_singleValue() throws ConfigurationException {
        MapOptionSource object = new MapOptionSource();
        ArgsOptionParser parser = new ArgsOptionParser(object);
        final String option = "--my_option";
        final String value = "123=true";  // Integer is the key type
        final Integer expKey = 123;
        final Boolean expValue = Boolean.TRUE;

        final List<String> leftovers = parser.parseBestEffort(
                new String[] {option, value});

        assertEquals(0, leftovers.size());
        assertNotNull(object.mMyOption);
        assertEquals(1, object.mMyOption.size());
        assertTrue(object.mMyOption.containsKey(expKey));
        assertEquals(expValue, object.mMyOption.get(expKey));
    }

    /**
     * Make sure that the single map option parsing works as expected with escaped value.
     */
    public void testParseBestEffort_mapOption_escaping() throws ConfigurationException {
        MapStringOptionSource object = new MapStringOptionSource();
        ArgsOptionParser parser = new ArgsOptionParser(object);
        final String option = "--my_option";
        final String key = "hello\\=bar";
        final String value = "123\\=true";
        final String expKey = "hello=bar";
        final String expValue = "123=true";

        final List<String> leftovers = parser.parseBestEffort(
                new String[] {option, key, value});

        assertEquals(0, leftovers.size());
        assertNotNull(object.mMyOption);
        assertEquals(1, object.mMyOption.size());
        assertTrue(object.mMyOption.containsKey(expKey));
        assertEquals(expValue, object.mMyOption.get(expKey));
    }

    /**
     * Make sure that the single map option parsing works as expected with escaped value.
     */
    public void testParseBestEffort_mapOption_singleValue_escaping() throws ConfigurationException {
        MapStringOptionSource object = new MapStringOptionSource();
        ArgsOptionParser parser = new ArgsOptionParser(object);
        final String option = "--my_option";
        final String value = "hello\\=bar=123\\=true\\=more";
        // Note that the actual value we store, is escaped.
        final String expKey = "hello=bar";
        final String expValue = "123=true=more";

        final List<String> leftovers = parser.parseBestEffort(
                new String[] {option, value});

        assertEquals(0, leftovers.size());
        assertNotNull(object.mMyOption);
        assertEquals(1, object.mMyOption.size());
        assertTrue(object.mMyOption.containsKey(expKey));
        assertEquals(expValue, object.mMyOption.get(expKey));
    }

    /**
     * Make sure that the both map option parsing work together as expected.
     */
    public void testParseBestEffort_mapOption_bothFormat() throws ConfigurationException {
        MapOptionSource object = new MapOptionSource();
        ArgsOptionParser parser = new ArgsOptionParser(object);
        final String option = "--my_option";
        final String value = "123=true";  // Integer is the key type
        final String key2 = "234";
        final String value2 = "false";
        final Integer expKey = 123;
        final Boolean expValue = Boolean.TRUE;
        final Integer expKey2 = 234;
        final Boolean expValue2 = Boolean.FALSE;

        final List<String> leftovers = parser.parseBestEffort(
                new String[] {option, value, option, key2, value2});

        assertEquals(0, leftovers.size());
        assertNotNull(object.mMyOption);
        assertEquals(2, object.mMyOption.size());
        assertTrue(object.mMyOption.containsKey(expKey));
        assertEquals(expValue, object.mMyOption.get(expKey));
        assertTrue(object.mMyOption.containsKey(expKey2));
        assertEquals(expValue2, object.mMyOption.get(expKey2));
    }

    /**
     * Make sure that we backtrack the appropriate amount when a Map option parse fails in the
     * middle
     */
    public void testParseBestEffort_mapOption_missingValue() throws ConfigurationException {
        MapOptionSource object = new MapOptionSource();
        ArgsOptionParser parser = new ArgsOptionParser(object);
        final String option = "--my_option";
        final String key = "123";  // Integer is the key type
        final List<String> leftovers = parser.parseBestEffort(
                new String[] {option, key});
        assertTrue(object.mMyOption.isEmpty());
        assertEquals(2, leftovers.size());
        assertEquals(option, leftovers.get(0));
        assertEquals(key, leftovers.get(1));
    }

    /**
     * Make sure that we backtrack the appropriate amount when a Map option parse fails in the
     * middle
     */
    public void testParseBestEffort_mapOption_badValue() throws ConfigurationException {
        MapOptionSource object = new MapOptionSource();
        ArgsOptionParser parser = new ArgsOptionParser(object);
        final String option = "--my_option";
        final String key = "123";  // Integer is the key type
        final String value = "notBoolean";  // Boolean is the value type
        final List<String> leftovers = parser.parseBestEffort(
                new String[] {option, key, value});
        assertTrue(object.mMyOption.isEmpty());
        assertEquals(3, leftovers.size());
        assertEquals(option, leftovers.get(0));
        assertEquals(key, leftovers.get(1));
        assertEquals(value, leftovers.get(2));
    }

    /**
     * Make sure that we backtrack the appropriate amount when a Map option parse fails in the
     * middle
     */
    public void testParseBestEffort_mapOption_badKey() throws ConfigurationException {
        MapOptionSource object = new MapOptionSource();
        ArgsOptionParser parser = new ArgsOptionParser(object);
        final String option = "--my_option";
        final String key = "NotANumber";  // Integer is the key type
        final String value = "true";  // Boolean is the value type
        final List<String> leftovers = parser.parseBestEffort(
                new String[] {option, key, value});
        assertTrue(object.mMyOption.isEmpty());
        assertEquals(3, leftovers.size());
        assertEquals(option, leftovers.get(0));
        assertEquals(key, leftovers.get(1));
        assertEquals(value, leftovers.get(2));
    }

    /**
     * Make sure that the single key value for map works.
     */
    public void testParseBestEffort_mapOption_singleValue_badValue() throws ConfigurationException {
        MapOptionSource object = new MapOptionSource();
        ArgsOptionParser parser = new ArgsOptionParser(object);
        final String option = "--my_option";
        final String value = "too=many=equals";
        final List<String> leftovers = parser.parseBestEffort(
                new String[] {option, value});

        assertTrue(object.mMyOption.isEmpty());
        assertEquals(2, leftovers.size());
        assertEquals(option, leftovers.get(0));
        assertEquals(value, leftovers.get(1));
    }


    // SECTION: help-related tests
    /**
     * Test that help text is displayed for all fields
     */
    public void testGetOptionHelp() {
        String help = ArgsOptionParser.getOptionHelp(false, new InheritedOptionSource());
        assertTrue(help.contains(InheritedOptionSource.OPTION_NAME));
        assertTrue(help.contains(InheritedOptionSource.OPTION_DESC));
        assertTrue(help.contains(OneOptionSource.OPTION_NAME));
        assertTrue(help.contains(OneOptionSource.OPTION_DESC));
        assertTrue(help.contains(OneOptionSource.DEFAULT_VALUE));
    }

    /**
     * Test displaying important only help text
     */
    public void testGetOptionHelp_important() {
        String help = ArgsOptionParser.getOptionHelp(true, new ImportantOptionSource());
        assertTrue(help.contains(ImportantOptionSource.IMPORTANT_OPTION_NAME));
        assertTrue(help.contains(ImportantOptionSource.IMPORTANT_UNSET_OPTION_NAME));
        assertFalse(help.contains(ImportantOptionSource.UNIMPORTANT_OPTION_NAME));
    }

    /**
     * Test that {@link Importance#IF_UNSET} {@link Option}s are hidden from help if set.
     */
    public void testGetOptionHelp_importantUnset() {
        String help = ArgsOptionParser.getOptionHelp(true, new ImportantOptionSource("foo"));
        assertTrue(help.contains(ImportantOptionSource.IMPORTANT_OPTION_NAME));
        assertFalse(help.contains(ImportantOptionSource.IMPORTANT_UNSET_OPTION_NAME));
        assertFalse(help.contains(ImportantOptionSource.UNIMPORTANT_OPTION_NAME));
    }


    // SECTION: mandatory option tests
    public void testMandatoryOption_noDefault() throws Exception {
        MandatoryOptionSourceNoDefault object = new MandatoryOptionSourceNoDefault();
        ArgsOptionParser parser = new ArgsOptionParser(object);
        // expect success
        parser.parse(new String[] {});
        try {
            parser.validateMandatoryOptions();
            fail("ConfigurationException not thrown");
        } catch (ConfigurationException e) {
            // expected
        }
    }

    public void testMandatoryOption_null() throws Exception {
        MandatoryOptionSourceNull object = new MandatoryOptionSourceNull();
        ArgsOptionParser parser = new ArgsOptionParser(object);
        parser.parse(new String[] {});
        try {
            parser.validateMandatoryOptions();
            fail("ConfigurationException not thrown");
        } catch (ConfigurationException e) {
            // expected
        }
    }

    public void testMandatoryOption_emptyCollection() throws Exception {
        MandatoryOptionSourceEmptyCollection object = new MandatoryOptionSourceEmptyCollection();
        ArgsOptionParser parser = new ArgsOptionParser(object);
        parser.parse(new String[] {});
        try {
            parser.validateMandatoryOptions();
            fail("ConfigurationException not thrown");
        } catch (ConfigurationException e) {
            // expected
        }
    }

    public void testMandatoryOption_emptyMap() throws Exception {
        MandatoryOptionSourceEmptyMap object = new MandatoryOptionSourceEmptyMap();
        ArgsOptionParser parser = new ArgsOptionParser(object);
        parser.parse(new String[] {});
        try {
            parser.validateMandatoryOptions();
            fail("ConfigurationException not thrown");
        } catch (ConfigurationException e) {
            // expected
        }
    }

    // Key Store related
    public void testKeyStore_string() throws Exception {
        final String expectedValue = "set";
        // Mock key store
        IKeyStoreClient c = EasyMock.createNiceMock(IKeyStoreClient.class);
        EasyMock.expect(c.isAvailable()).andReturn(true);
        EasyMock.expect(c.containsKey("foo")).andStubReturn(true);
        EasyMock.expect(c.fetchKey("foo")).andReturn(expectedValue);
        EasyMock.replay(c);
        OneOptionSource object = new OneOptionSource();
        ArgsOptionParser parser = new ArgsOptionParser(object);
        parser.setKeyStore(c);
        parser.parse(new String[] {"--my_option", "USE_KEYSTORE@foo"});
        // Key store value is set for the option as expected.
        assertEquals(expectedValue, object.mMyOption);
        EasyMock.verify(c);
    }

    public void testKeyStore_stringWithNullKeyStore() throws Exception {
        final String expectedValue = "set";
        OneOptionSource object = new OneOptionSource();
        ArgsOptionParser parser = new ArgsOptionParser(object);
        parser.setKeyStore(null);
        // An null key store should not affect normal operations.
        parser.parse(new String[] {"--my_option", expectedValue});
        assertEquals(expectedValue, object.mMyOption);
    }

    public void testKeyStore_stringWithNullKeyStoreAndKey() throws Exception {
        final String expectedValue = "";
        OneOptionSource object = new OneOptionSource();
        ArgsOptionParser parser = new ArgsOptionParser(object);
        parser.setKeyStore(null);
        // We try to fetch a key store value with null key store this should return an empty value.
        try {
            parser.parse(new String[] {"--my_option", "USE_KEYSTORE@foo"});
        } catch (ConfigurationException e) {
            //expected
            return;
        }
        fail("ConfigurationException not thrown for attempted use of null keystore.");
    }

    public void testKeyStore_stringWithUnavalableKeyStore() throws Exception {
        final String expectedValue = "set";
        // Mock key store
        IKeyStoreClient c = EasyMock.createNiceMock(IKeyStoreClient.class);
        EasyMock.expect(c.isAvailable()).andStubReturn(false);
        EasyMock.replay(c);
        OneOptionSource object = new OneOptionSource();
        ArgsOptionParser parser = new ArgsOptionParser(object);
        parser.setKeyStore(c);
        // An unavailable key store should not affect normal operations.
        parser.parse(new String[] {"--my_option", expectedValue});
        assertEquals(expectedValue, object.mMyOption);
        EasyMock.verify(c);
    }

    public void testKeyStore_stringWithUnavalableKeyStoreWithKey() throws Exception {
        final String expectedValue = "";
        // Mock key store
        IKeyStoreClient c = EasyMock.createNiceMock(IKeyStoreClient.class);
        EasyMock.expect(c.isAvailable()).andReturn(false);
        EasyMock.replay(c);
        OneOptionSource object = new OneOptionSource();
        ArgsOptionParser parser = new ArgsOptionParser(object);
        parser.setKeyStore(c);
        // We try to fetch a key store value with unavailable key store this should throw an
        // exception
        try {
            parser.parse(new String[] {"--my_option", "USE_KEYSTORE@foo"});
        } catch (ConfigurationException e) {
            //expected
            EasyMock.verify(c);
            return;
        }
        fail("ConfiguationException not thrown for attempted use of unavailable keystore.");
    }

    public void testKeyStore_stringWithUnavalableKey() throws Exception {
        final String expectedValue = "";
        // Mock key store
        IKeyStoreClient c = EasyMock.createNiceMock(IKeyStoreClient.class);
        EasyMock.expect(c.isAvailable()).andReturn(true);
        EasyMock.expect(c.containsKey("foobar")).andStubReturn(true);
        EasyMock.replay(c);
        OneOptionSource object = new OneOptionSource();
        ArgsOptionParser parser = new ArgsOptionParser(object);
        parser.setKeyStore(c);
        // We try to fetch a key store value with unavailable key this should throw an exception
        try {
            parser.parse(new String[] {"--my_option", "USE_KEYSTORE@foo"});
        } catch (ConfigurationException e) {
            //expected
            EasyMock.verify(c);
            return;
        }
        fail("ConfiguationException not thrown for attempted use of unavailable keystore.");
    }

    public void testKeyStore_mapOptions() throws Exception {
        final String option = "--my_option";
        final String key = "hello";
        final String value = "USE_KEYSTORE@foobar";
        final String expKey = "hello";
        final String expValue = "123";

        // Mock key store
        IKeyStoreClient c = EasyMock.createNiceMock(IKeyStoreClient.class);
        EasyMock.expect(c.isAvailable()).andReturn(true);
        EasyMock.expect(c.containsKey("foobar")).andStubReturn(true);
        EasyMock.expect(c.fetchKey("foobar")).andReturn(expValue);
        EasyMock.replay(c);
        MapStringOptionSource object = new MapStringOptionSource();
        ArgsOptionParser parser = new ArgsOptionParser(object);
        parser.setKeyStore(c);
        // --option hello USE_KEYSTORE@foobar should give --option hello 123;
        // where hello is set to 123
        final List<String> leftovers = parser.parseBestEffort(
                new String[] {option, key, value});

        assertEquals(0, leftovers.size());
        assertNotNull(object.mMyOption);
        assertEquals(1, object.mMyOption.size());
        assertTrue(object.mMyOption.containsKey(expKey));
        assertEquals(expValue, object.mMyOption.get(expKey));
        EasyMock.verify(c);
    }

    public void testKeyStore_mapOptionsSingleValue() throws Exception {
        final String option = "--my_option";
        final String value = "hello=USE_KEYSTORE@foobar";
        final String expKey = "hello";
        final String expValue = "123";

        // Mock key store
        IKeyStoreClient c = EasyMock.createNiceMock(IKeyStoreClient.class);
        EasyMock.expect(c.isAvailable()).andReturn(true);
        EasyMock.expect(c.containsKey("foobar")).andStubReturn(true);
        EasyMock.expect(c.fetchKey("foobar")).andReturn(expValue);
        EasyMock.replay(c);
        MapStringOptionSource object = new MapStringOptionSource();
        ArgsOptionParser parser = new ArgsOptionParser(object);
        parser.setKeyStore(c);
        // --option hello=USE_KEYSTORE@foobar should give --option hello=123
        final List<String> leftovers = parser.parseBestEffort(
                new String[] {option, value});

        assertEquals(0, leftovers.size());
        assertNotNull(object.mMyOption);
        assertEquals(1, object.mMyOption.size());
        assertTrue(object.mMyOption.containsKey(expKey));
        assertEquals(expValue, object.mMyOption.get(expKey));
        EasyMock.verify(c);

    }

    public void testKeyStore_mapOptionsMixedValue() throws Exception {
        final String option = "--my_option";
        final String value = "hello=123";
        final String key2 = "byebye";
        final String value2 = "USE_KEYSTORE@bar";
        final String expKey = "hello";
        final String expValue = "123";
        final String expKey2 = "byebye";
        final String expValue2 = "456";

        // Mock key store
        IKeyStoreClient c = EasyMock.createNiceMock(IKeyStoreClient.class);
        EasyMock.expect(c.isAvailable()).andReturn(true);
        EasyMock.expect(c.containsKey("bar")).andStubReturn(true);
        EasyMock.expect(c.fetchKey("bar")).andReturn(expValue2);
        EasyMock.replay(c);
        MapStringOptionSource object = new MapStringOptionSource();
        ArgsOptionParser parser = new ArgsOptionParser(object);
        parser.setKeyStore(c);

        final List<String> leftovers = parser.parseBestEffort(
                new String[] {option, key2, value2, option, value});

        assertEquals(0, leftovers.size());
        assertNotNull(object.mMyOption);
        assertEquals(2, object.mMyOption.size());

        assertTrue(object.mMyOption.containsKey(expKey2));
        assertEquals(expValue2, object.mMyOption.get(expKey2));
        assertTrue(object.mMyOption.containsKey(expKey));
        assertEquals(expValue, object.mMyOption.get(expKey));
        EasyMock.verify(c);
    }

    public void testKeyStore_mapOptionsMixedValue_allKeys() throws Exception {
        final String option = "--my_option";
        final String value = "hello=USE_KEYSTORE@foobar";
        final String key2 = "byebye";
        final String value2 = "USE_KEYSTORE@bar";
        final String expKey = "hello";
        final String expValue = "123";
        final String expKey2 = "byebye";
        final String expValue2 = "456";

        // Mock key store
        IKeyStoreClient c = EasyMock.createNiceMock(IKeyStoreClient.class);
        EasyMock.expect(c.isAvailable()).andStubReturn(true);
        EasyMock.expect(c.containsKey("foobar")).andStubReturn(true);
        EasyMock.expect(c.fetchKey("foobar")).andReturn(expValue);
        EasyMock.expect(c.containsKey("bar")).andStubReturn(true);
        EasyMock.expect(c.fetchKey("bar")).andReturn(expValue2);
        EasyMock.replay(c);
        MapStringOptionSource object = new MapStringOptionSource();
        ArgsOptionParser parser = new ArgsOptionParser(object);
        parser.setKeyStore(c);

        final List<String> leftovers = parser.parseBestEffort(
                new String[] {option, key2, value2, option, value});

        assertEquals(0, leftovers.size());
        assertNotNull(object.mMyOption);
        assertEquals(2, object.mMyOption.size());

        assertTrue(object.mMyOption.containsKey(expKey2));
        assertEquals(expValue2, object.mMyOption.get(expKey2));
        assertTrue(object.mMyOption.containsKey(expKey));
        assertEquals(expValue, object.mMyOption.get(expKey));
        EasyMock.verify(c);
    }

    /**
     * Test that when the default {@link StubKeyStoreClient} is used and a keystore option is
     * requested, we throw a configuration exception and no crash.
     */
    public void testKeyStore_StubKeystore_optionRequested() throws Exception {
        final String expectedValue = "";
        OneOptionSource object = new OneOptionSource();
        ArgsOptionParser parser = new ArgsOptionParser(object);
        parser.setKeyStore(new StubKeyStoreClient());
        // We try to fetch a key store value with unavailable key this should throw an exception
        try {
            parser.parse(new String[] {"--my_option", "USE_KEYSTORE@foo"});
        } catch (ConfigurationException e) {
            //expected
            return;
        }
        fail("ConfiguationException not thrown for attempted use of unavailable keystore.");
    }
}
