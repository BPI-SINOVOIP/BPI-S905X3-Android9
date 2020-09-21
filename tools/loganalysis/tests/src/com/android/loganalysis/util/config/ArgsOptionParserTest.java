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

import com.android.loganalysis.util.config.Option.Importance;

import junit.framework.TestCase;

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
     * Test the "--no-<bool option>" syntax
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
}
