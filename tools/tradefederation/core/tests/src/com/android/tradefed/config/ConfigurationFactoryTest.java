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

import com.android.ddmlib.Log.LogLevel;
import com.android.tradefed.build.LocalDeviceBuildProvider;
import com.android.tradefed.config.ConfigurationFactory.ConfigId;
import com.android.tradefed.log.ILeveledLogOutput;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.targetprep.DeviceWiper;
import com.android.tradefed.targetprep.StubTargetPreparer;
import com.android.tradefed.targetprep.multi.StubMultiTargetPreparer;
import com.android.tradefed.util.FileUtil;

import junit.framework.TestCase;

import org.mockito.Mockito;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.PrintStream;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * Unit tests for {@link ConfigurationFactory}
 */
public class ConfigurationFactoryTest extends TestCase {

    private ConfigurationFactory mFactory;
    // Create a real instance for tests that checks the content of our configs. making it static to
    // reduce the runtime of reloading the config thanks to the caching of configurations.
    private static ConfigurationFactory mRealFactory = new ConfigurationFactory();

    /** the test config name that is built into this jar */
    private static final String TEST_CONFIG = "test-config";
    private static final String GLOBAL_TEST_CONFIG = "global-config";
    private static final String INCLUDE_CONFIG = "include-config";

    /**
     * {@inheritDoc}
     */
    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mFactory =
                new ConfigurationFactory() {
                    @Override
                    protected String getConfigPrefix() {
                        return "testconfigs/";
                    }
                };
    }

    /**
     * Sanity test to ensure all config names on classpath are loadable
     */
    public void testLoadAllConfigs() throws ConfigurationException {
        ConfigurationFactory spyFactory = Mockito.spy(mRealFactory);
        Mockito.doReturn(new HashSet<String>()).when(spyFactory).getConfigNamesFromTestCases(null);

        // we dry-run the templates otherwise it will always fail.
        spyFactory.loadAllConfigs(false);
        assertTrue(spyFactory.getMapConfig().size() > 0);
        Mockito.verify(spyFactory, Mockito.times(1)).getConfigNamesFromTestCases(null);
    }

    /**
     * Sanity test to ensure all configs on classpath can be fully loaded and parsed
     */
    public void testLoadAndPrintAllConfigs() throws ConfigurationException {
        ConfigurationFactory spyFactory = Mockito.spy(mRealFactory);
        Mockito.doReturn(new HashSet<String>()).when(spyFactory).getConfigNamesFromTestCases(null);

        // Printing the help involves more checks since it tries to resolve the config objects.
        spyFactory.loadAndPrintAllConfigs();
        assertTrue(spyFactory.getMapConfig().size() > 0);
        Mockito.verify(spyFactory, Mockito.times(1)).getConfigNamesFromTestCases(null);
    }

    /**
     * Test that a config xml defined in this test jar can be read as a built-in
     */
    public void testGetConfiguration_extension() throws ConfigurationException {
        assertConfigValid(TEST_CONFIG);
    }

    private Map<String, String> buildMap(String... args) {
        if ((args.length % 2) != 0) {
            throw new IllegalArgumentException(String.format(
                "Expected an even number of args; got %d", args.length));
        }

        final Map<String, String> map = new HashMap<String, String>(args.length / 2);
        for (int i = 0; i < args.length; i += 2) {
            map.put(args[i], args[i + 1]);
        }

        return map;
    }

    /**
     * Make sure that ConfigId behaves in the right way to serve as a hash key
     */
    public void testConfigId_equals() {
        final ConfigId config1a = new ConfigId("one");
        final ConfigId config1b = new ConfigId("one");
        final ConfigId config2 = new ConfigId("two");
        final ConfigId config3a = new ConfigId("one", buildMap("target", "foo"));
        final ConfigId config3b = new ConfigId("one", buildMap("target", "foo"));
        final ConfigId config4 = new ConfigId("two", buildMap("target", "bar"));

        assertEquals(config1a, config1b);
        assertEquals(config3a, config3b);

        // Check for false equivalences, and don't depend on #equals being commutative
        assertFalse(config1a.equals(config2));
        assertFalse(config1a.equals(config3a));
        assertFalse(config1a.equals(config4));

        assertFalse(config2.equals(config1a));
        assertFalse(config2.equals(config3a));
        assertFalse(config2.equals(config4));

        assertFalse(config3a.equals(config1a));
        assertFalse(config3a.equals(config2));
        assertFalse(config3a.equals(config4));

        assertFalse(config4.equals(config1a));
        assertFalse(config4.equals(config2));
        assertFalse(config4.equals(config3a));
    }

    /**
     * Make sure that ConfigId behaves in the right way to serve as a hash key
     */
    public void testConfigId_hashKey() {
        final Map<ConfigId, String> map = new HashMap<>();
        final ConfigId config1a = new ConfigId("one");
        final ConfigId config1b = new ConfigId("one");
        final ConfigId config2 = new ConfigId("two");
        final ConfigId config3a = new ConfigId("one", buildMap("target", "foo"));
        final ConfigId config3b = new ConfigId("one", buildMap("target", "foo"));
        final ConfigId config4 = new ConfigId("two", buildMap("target", "bar"));

        // Make sure that keys config1a and config1b behave identically
        map.put(config1a, "1a");
        assertEquals("1a", map.get(config1a));
        assertEquals("1a", map.get(config1b));

        map.put(config1b, "1b");
        assertEquals("1b", map.get(config1a));
        assertEquals("1b", map.get(config1b));

        assertFalse(map.containsKey(config2));
        assertFalse(map.containsKey(config3a));
        assertFalse(map.containsKey(config4));

        // Make sure that keys config3a and config3b behave identically
        map.put(config3a, "3a");
        assertEquals("3a", map.get(config3a));
        assertEquals("3a", map.get(config3b));

        map.put(config3b, "3b");
        assertEquals("3b", map.get(config3a));
        assertEquals("3b", map.get(config3b));

        assertEquals(2, map.size());
        assertFalse(map.containsKey(config2));
        assertFalse(map.containsKey(config4));

        // It's unlikely for these to fail if the above tests all passed, but just fill everything
        // out for completeness
        map.put(config2, "2");
        map.put(config4, "4");

        assertEquals(4, map.size());
        assertEquals("1b", map.get(config1a));
        assertEquals("1b", map.get(config1b));
        assertEquals("2", map.get(config2));
        assertEquals("3b", map.get(config3a));
        assertEquals("3b", map.get(config3b));
        assertEquals("4", map.get(config4));
    }

    /**
     * Test specifying a config xml by file path
     */
    public void testGetConfiguration_xmlpath() throws ConfigurationException, IOException {
        // extract the test-config.xml into a tmp file
        InputStream configStream = getClass().getResourceAsStream(
                String.format("/testconfigs/%s.xml", TEST_CONFIG));
        File tmpFile = FileUtil.createTempFile(TEST_CONFIG, ".xml");
        try {
            FileUtil.writeToFile(configStream, tmpFile);
            assertConfigValid(tmpFile.getAbsolutePath());
            // check reading it again - should grab the cached version
            assertConfigValid(tmpFile.getAbsolutePath());
        } finally {
            FileUtil.deleteFile(tmpFile);
        }
    }

    /**
     * Test that a config xml defined in this test jar can be read as a built-in
     */
    public void testGetGlobalConfiguration_extension() throws ConfigurationException {
        assertGlobalConfigValid(GLOBAL_TEST_CONFIG);
    }

    /**
     * Test specifying a config xml by file path
     */
    public void testGetGlobalConfiguration_xmlpath() throws ConfigurationException, IOException {
        // extract the test-config.xml into a tmp file
        InputStream configStream = getClass().getResourceAsStream(
                String.format("/testconfigs/%s.xml", GLOBAL_TEST_CONFIG));
        File tmpFile = FileUtil.createTempFile(GLOBAL_TEST_CONFIG, ".xml");
        try {
            FileUtil.writeToFile(configStream, tmpFile);
            assertGlobalConfigValid(tmpFile.getAbsolutePath());
            // check reading it again - should grab the cached version
            assertGlobalConfigValid(tmpFile.getAbsolutePath());
        } finally {
            FileUtil.deleteFile(tmpFile);
        }
    }

    /**
     * Checks all config attributes are non-null
     */
    private void assertConfigValid(String name) throws ConfigurationException {
        IConfiguration config = mFactory.createConfigurationFromArgs(new String[] {name});
        assertNotNull(config);
    }

    /**
     * Checks all config attributes are non-null
     */
    private void assertGlobalConfigValid(String name) throws ConfigurationException {
        List<String> unprocessed = new ArrayList<String>();
        IGlobalConfiguration config =
                mFactory.createGlobalConfigurationFromArgs(new String[] {name}, unprocessed);
        assertNotNull(config);
        assertNotNull(config.getDeviceMonitors());
        assertNotNull(config.getWtfHandler());
        assertTrue(unprocessed.isEmpty());
    }

    /**
     * Test calling {@link ConfigurationFactory#createConfigurationFromArgs(String[])}
     * with a name that does not exist.
     */
    public void testCreateConfigurationFromArgs_missing()  {
        try {
            mFactory.createConfigurationFromArgs(new String[] {"non existent"});
            fail("did not throw ConfigurationException");
        } catch (ConfigurationException e) {
            // expected
        }
    }

    /**
     * Test calling {@link ConfigurationFactory#createConfigurationFromArgs(String[])}
     * with config that has unset mandatory options.
     * <p/>
     * Expect this to succeed, since mandatory option validation no longer happens at configuration
     * instantiation time.
     */
    public void testCreateConfigurationFromArgs_mandatory() throws ConfigurationException {
        assertNotNull(mFactory.createConfigurationFromArgs(new String[] {"mandatory-config"}));
    }

    /**
     * Test passing empty arg list to
     * {@link ConfigurationFactory#createConfigurationFromArgs(String[])}.
     */
    public void testCreateConfigurationFromArgs_empty() {
        try {
            mFactory.createConfigurationFromArgs(new String[] {});
            fail("did not throw ConfigurationException");
        } catch (ConfigurationException e) {
            // expected
        }
    }

    /**
     * Test {@link ConfigurationFactory#createConfigurationFromArgs(String[])} using TEST_CONFIG
     */
    public void testCreateConfigurationFromArgs() throws ConfigurationException {
        // pick an arbitrary option to test to ensure it gets populated
        IConfiguration config = mFactory.createConfigurationFromArgs(new String[] {TEST_CONFIG,
                "--log-level", LogLevel.VERBOSE.getStringValue()});
        ILeveledLogOutput logger = config.getLogOutput();
        assertEquals(LogLevel.VERBOSE, logger.getLogLevel());
    }

    /**
     * Test {@link ConfigurationFactory#createConfigurationFromArgs(String[])} when extra positional
     * arguments are supplied
     */
    public void testCreateConfigurationFromArgs_unprocessedArgs() {
        try {
            mFactory.createConfigurationFromArgs(new String[] {TEST_CONFIG, "--log-level",
                    LogLevel.VERBOSE.getStringValue(), "blah"});
            fail("ConfigurationException not thrown");
        } catch (ConfigurationException e) {
            // expected
        }
    }

    /**
     * Test {@link ConfigurationFactory#printHelp(PrintStream)}
     */
    public void testPrintHelp() {
        ByteArrayOutputStream outputStream = new ByteArrayOutputStream();
        PrintStream mockPrintStream = new PrintStream(outputStream);
        mRealFactory.printHelp(mockPrintStream);
        // verify all the instrument config names are present
        final String usageString = outputStream.toString();
        assertTrue(usageString.contains("instrument"));
    }

    /**
     * Test {@link ConfigurationFactory#printHelpForConfig(String[], boolean, PrintStream)} when
     * config referenced by args exists
     */
    public void testPrintHelpForConfig_configExists() {
        String[] args = new String[] {TEST_CONFIG};
        ByteArrayOutputStream outputStream = new ByteArrayOutputStream();
        PrintStream mockPrintStream = new PrintStream(outputStream);
        mFactory.printHelpForConfig(args, true, mockPrintStream);

        // verify the default configs name used is present
        final String usageString = outputStream.toString();
        assertTrue(usageString.contains(TEST_CONFIG));
        // TODO: add stricter verification
    }

    /**
     * Test {@link ConfigurationFactory#getConfigList()}
     */
    public void testListAllConfigs() {
        List<String> listConfigs = mRealFactory.getConfigList();
        assertTrue(listConfigs.size() != 0);
        // Check that our basic configs are always here
        assertTrue(listConfigs.contains("empty"));
        assertTrue(listConfigs.contains("host"));
        assertTrue(listConfigs.contains("instrument"));
    }

    /**
     * Test {@link ConfigurationFactory#getConfigList(String)} where we list the config in a sub
     * path only
     */
    public void testListSubConfig() {
        final String subDir = "suite/";
        List<String> listConfigs = mRealFactory.getConfigList(subDir);
        assertTrue(listConfigs.size() != 0);
        // Check that our basic configs are always here
        assertTrue(listConfigs.contains("suite/stub1"));
        assertTrue(listConfigs.contains("suite/stub2"));
        // Validate that all listed config are indeed from the subdir.
        for (String config : listConfigs) {
            assertTrue(config.startsWith(subDir));
        }
    }

    /**
     * Test loading a config that includes another config.
     */
    public void testCreateConfigurationFromArgs_includeConfig() throws Exception {
        IConfiguration config = mFactory.createConfigurationFromArgs(
                new String[]{INCLUDE_CONFIG});
        assertTrue(config.getTests().get(0) instanceof StubOptionTest);
        assertTrue(config.getTests().get(1) instanceof StubOptionTest);
        StubOptionTest fromTestConfig = (StubOptionTest) config.getTests().get(0);
        StubOptionTest fromIncludeConfig = (StubOptionTest) config.getTests().get(1);
        assertEquals("valueFromTestConfig", fromTestConfig.mOption);
        assertEquals("valueFromIncludeConfig", fromIncludeConfig.mOption);
    }

    /**
     * Test loading a config that uses the "default" attribute of a template-include tag to include
     * another config.
     */
    public void testCreateConfigurationFromArgs_defaultTemplateInclude_default() throws Exception {
        // The default behavior is to include test-config directly.  Nesting is such that innermost
        // elements come first.
        IConfiguration config = mFactory.createConfigurationFromArgs(
                new String[]{"template-include-config-with-default"});
        assertEquals(2, config.getTests().size());
        assertTrue(config.getTests().get(0) instanceof StubOptionTest);
        assertTrue(config.getTests().get(1) instanceof StubOptionTest);
        StubOptionTest innerConfig = (StubOptionTest) config.getTests().get(0);
        StubOptionTest outerConfig = (StubOptionTest) config.getTests().get(1);
        assertEquals("valueFromTestConfig", innerConfig.mOption);
        assertEquals("valueFromTemplateIncludeWithDefaultConfig", outerConfig.mOption);
    }

    /**
     * Test using {@code <include>} to load a config that uses the "default" attribute of a
     * template-include tag to include a third config.
     */
    public void testCreateConfigurationFromArgs_includeTemplateIncludeWithDefault() throws Exception {
        // The default behavior is to include test-config directly.  Nesting is such that innermost
        // elements come first.
        IConfiguration config = mFactory.createConfigurationFromArgs(
                new String[]{"include-template-config-with-default"});
        assertEquals(3, config.getTests().size());
        assertTrue(config.getTests().get(0) instanceof StubOptionTest);
        assertTrue(config.getTests().get(1) instanceof StubOptionTest);
        assertTrue(config.getTests().get(2) instanceof StubOptionTest);
        StubOptionTest innerConfig = (StubOptionTest) config.getTests().get(0);
        StubOptionTest middleConfig = (StubOptionTest) config.getTests().get(1);
        StubOptionTest outerConfig = (StubOptionTest) config.getTests().get(2);
        assertEquals("valueFromTestConfig", innerConfig.mOption);
        assertEquals("valueFromTemplateIncludeWithDefaultConfig", middleConfig.mOption);
        assertEquals("valueFromIncludeTemplateConfigWithDefault", outerConfig.mOption);
    }

    /**
     * Test loading a config that uses the "default" attribute of a template-include tag to include
     * another config.  In this case, we override the default attribute on the commandline.
     */
    public void testCreateConfigurationFromArgs_defaultTemplateInclude_alternate() throws Exception {
        IConfiguration config = mFactory.createConfigurationFromArgs(
                new String[]{"template-include-config-with-default", "--template:map", "target",
                INCLUDE_CONFIG});
        assertEquals(3, config.getTests().size());
        assertTrue(config.getTests().get(0) instanceof StubOptionTest);
        assertTrue(config.getTests().get(1) instanceof StubOptionTest);
        assertTrue(config.getTests().get(2) instanceof StubOptionTest);

        StubOptionTest innerConfig = (StubOptionTest) config.getTests().get(0);
        StubOptionTest middleConfig = (StubOptionTest) config.getTests().get(1);
        StubOptionTest outerConfig = (StubOptionTest) config.getTests().get(2);

        assertEquals("valueFromTestConfig", innerConfig.mOption);
        assertEquals("valueFromIncludeConfig", middleConfig.mOption);
        assertEquals("valueFromTemplateIncludeWithDefaultConfig", outerConfig.mOption);
    }

    /**
     * Test loading a config that uses template-include to include another config.
     */
    public void testCreateConfigurationFromArgs_templateInclude() throws Exception {
        IConfiguration config = mFactory.createConfigurationFromArgs(
                new String[]{"template-include-config", "--template:map", "target",
                "test-config"});
        assertTrue(config.getTests().get(0) instanceof StubOptionTest);
        assertTrue(config.getTests().get(1) instanceof StubOptionTest);
        StubOptionTest fromTestConfig = (StubOptionTest) config.getTests().get(0);
        StubOptionTest fromTemplateIncludeConfig = (StubOptionTest) config.getTests().get(1);
        assertEquals("valueFromTestConfig", fromTestConfig.mOption);
        assertEquals("valueFromTemplateIncludeConfig", fromTemplateIncludeConfig.mOption);
    }

    /** Test loading a config that uses template-include to include another config. */
    public void testCreateConfigurationFromArgs_templateInclude_multiKey() throws Exception {
        try {
            mFactory.createConfigurationFromArgs(
                    new String[] {
                        "template-include-config",
                        "--template:map",
                        "target",
                        "test-config",
                        "--template:map",
                        "target",
                        "test-config"
                    });
            fail("Should have thrown an exception.");
        } catch (ConfigurationException expected) {
            // Expected
            assertEquals(
                    "More than one template specified for key 'target'", expected.getMessage());
        }
    }

    /**
     * Make sure that we throw a useful error when template-include usage is underspecified.
     */
    public void testCreateConfigurationFromArgs_templateInclude_unspecified() throws Exception {
        final String configName = "template-include-config";
        try {
            mFactory.createConfigurationFromArgs(new String[]{configName});
            fail ("ConfigurationException not thrown");
        } catch (ConfigurationException e) {
            // Make sure that we get the expected error message
            final String msg = e.getMessage();
            assertNotNull(msg);

            assertTrue(String.format("Error message does not mention the name of the broken " +
                    "config.  msg was: %s", msg), msg.contains(configName));

            // Error message should help people to resolve the problem
            assertTrue(String.format("Error message should help user to resolve the " +
                    "template-include.  msg was: %s", msg),
                    msg.contains(String.format("--template:map %s", "target")));
            CLog.e(msg);
            assertTrue(
                    String.format(
                            "Error message should mention the ability to specify a "
                                    + "default resolution.  msg was: %s",
                            msg),
                    msg.contains("'default'"));
        }
    }

    /**
     * Make sure that we throw a useful error when template-include mentions a target configuration
     * that doesn't exist.
     */
    public void testCreateConfigurationFromArgs_templateInclude_missing() throws Exception {
        final String configName = "template-include-config";
        final String includeName = "no-exist";

        try {
            mFactory.createConfigurationFromArgs(
                    new String[]{configName, "--template:map", "target", includeName});
            fail ("ConfigurationException not thrown");
        } catch (ConfigurationException e) {
            // Make sure that we get the expected error message
            final String msg = e.getMessage();
            assertNotNull(msg);

            assertTrue(String.format("Error message does not mention the name of the broken " +
                    "config.  msg was: %s", msg), msg.contains(configName));
            assertTrue(String.format("Error message does not mention the name of the missing " +
                    "include target.  msg was: %s", msg), msg.contains(includeName));
        }
    }

    /**
     * Test loading a config that includes a local config.
     */
    public void testCreateConfigurationFromArgs_templateInclude_local() throws Exception {
        final String configName = "template-include-config";
        InputStream configStream = getClass().getResourceAsStream(
                String.format("/testconfigs/%s.xml", INCLUDE_CONFIG));
        File tmpConfig = FileUtil.createTempFile(INCLUDE_CONFIG, ".xml");
        try {
            FileUtil.writeToFile(configStream, tmpConfig);
            final String includeName = tmpConfig.getAbsolutePath();
            IConfiguration config = mFactory.createConfigurationFromArgs(
                    new String[]{configName, "--template:map", "target", includeName});
            assertTrue(config.getTests().get(0) instanceof StubOptionTest);
            assertTrue(config.getTests().get(1) instanceof StubOptionTest);
            StubOptionTest fromTestConfig = (StubOptionTest) config.getTests().get(0);
            StubOptionTest fromIncludeConfig = (StubOptionTest) config.getTests().get(1);
            assertEquals("valueFromTestConfig", fromTestConfig.mOption);
            assertEquals("valueFromIncludeConfig", fromIncludeConfig.mOption);
        } finally {
            FileUtil.deleteFile(tmpConfig);
        }
    }

    /**
     * This unit test codifies the expectation that an inner {@code <template-include>} tag
     * is properly found and replaced by a config containing another template that is resolved.
     * MAIN CONFIG -> Template 1 -> Template 2 = Works!
     */
    public void testCreateConfigurationFromArgs_templateInclude_dependent() throws Exception {
        final String configName = "depend-template-include-config";
        final String depTargetName = "template-include-config";
        final String targetName = "test-config";

        IConfiguration config =
                mFactory.createConfigurationFromArgs(
                        new String[] {
                            configName,
                            "--template:map",
                            "dep-target",
                            depTargetName,
                            "--template:map",
                            "target",
                            targetName
                        });
        assertTrue(config.getTests().get(0) instanceof StubOptionTest);
        assertTrue(config.getTests().get(1) instanceof StubOptionTest);
    }

    /**
     * This unit test codifies the expectation that an inner {@code <template-include>} tag
     * replaced by a missing config raise a failure.
     * MAIN CONFIG -> Template 1 -> Template 2(missing) = error
     */
    public void testCreateConfigurationFromArgs_templateInclude_dependent_missing()
            throws Exception {
        final String configName = "depend-template-include-config";
        final String depTargetName = "template-include-config";
        final String targetName = "test-config-missing";
        final String expError = String.format(
                "Bundled config '%s' is including a config '%s' that's neither local nor bundled.",
                depTargetName, targetName);

        try {
            mFactory.createConfigurationFromArgs(new String[]{configName,
                    "--template:map", "dep-target", depTargetName,
                    "--template:map", "target", targetName});
            fail ("ConfigurationException not thrown");
        } catch (ConfigurationException e) {
            // Make sure that we get the expected error message
            assertEquals(expError, e.getMessage());
        }
    }

    /**
     * This unit test codifies the expectation that an inner {@code <template-include>} tag
     * that doesn't have a default attribute will fail because cannot be resolved.
     * MAIN CONFIG -> Template 1 -> Template 2(no Default, no args override) = error
     */
    public void testCreateConfigurationFromArgs_templateInclude_dependent_nodefault()
            throws Exception {
        final String configName = "depend-template-include-config";
        final String depTargetName = "template-include-config";
        final String expError = String.format(
                "Failed to parse config xml '%s'. Reason: " +
                ConfigurationXmlParser.ConfigHandler.INNER_TEMPLATE_INCLUDE_ERROR,
                configName, configName, depTargetName);
        try {
            mFactory.createConfigurationFromArgs(new String[]{configName,
                    "--template:map", "dep-target", depTargetName});
            fail ("ConfigurationException not thrown");
        } catch (ConfigurationException e) {
            assertEquals(expError, e.getMessage());
        }
    }

    /**
     * This unit test codifies the expectation that an inner {@code <template-include>} tag
     * that is inside an include tag will be correctly replaced by arguments.
     * Main Config -> Include 1 -> Template 1 -> test-config.xml
     */
    public void testCreateConfigurationFromArgs_include_dependent() throws Exception {
        final String configName = "include-template-config";
        final String targetName = "test-config";
        try {
            IConfiguration config = mFactory.createConfigurationFromArgs(new String[]{configName,
                    "--template:map", "target", targetName});
            assertTrue(config.getTests().get(0) instanceof StubOptionTest);
            assertTrue(config.getTests().get(1) instanceof StubOptionTest);
        } catch (ConfigurationException e) {
            fail(e.getMessage());
        }
    }

    /**
     * This unit test ensure that when a configuration use included configuration the top
     * configuration description is kept.
     */
    public void testTopDescriptionIsPreserved() throws ConfigurationException {
        final String configName = "top-config";
        Map<String, String> fakeTemplate = new HashMap<>();
        fakeTemplate.put("target", "included-config");
        ConfigurationDef test = mFactory.new ConfigLoader(false)
                .getConfigurationDef(configName, fakeTemplate);
        assertEquals("top config description", test.getDescription());
    }

    /**
     * This unit test codifies the expectation that an inner {@code <template-include>} tag
     * that is inside an include tag will be correctly rejected if no arguments can match it and
     * no default value is present.
     * Main Config -> Include 1 -> Template 1 (No default, no args override) = error
     */
    public void testCreateConfigurationFromArgs_include_dependent_nodefault() throws Exception {
        final String configName = "include-template-config";
        final String includeTargetName = "template-include-config";
        final String expError = String.format(
                "Failed to parse config xml '%s'. Reason: " +
                ConfigurationXmlParser.ConfigHandler.INNER_TEMPLATE_INCLUDE_ERROR,
                configName, configName, includeTargetName);
        try {
            mFactory.createConfigurationFromArgs(new String[]{configName});
            fail ("ConfigurationException not thrown");
        } catch (ConfigurationException e) {
            assertEquals(expError, e.getMessage());
        }
    }

    /**
     * Test loading a config that tries to include itself
     */
    public void testCreateConfigurationFromArgs_recursiveInclude() throws Exception {
        try {
            mFactory.createConfigurationFromArgs(new String[] {"recursive-config"});
            fail("ConfigurationException not thrown");
        } catch (ConfigurationException e) {
            // expected
        }
    }

    /**
     * Test loading a config that tries to replace a template with itself will fail because it
     * creates a cycle of configuration.
     */
    public void testCreateConfigurationFromArgs_recursiveTemplate() throws Exception {
        final String configName = "depend-template-include-config";
        final String depTargetName = "depend-template-include-config";
        final String expError = String.format(
                "Circular configuration include: config '%s' is already included", depTargetName);
        try {
            mFactory.createConfigurationFromArgs(new String[]{configName,
                    "--template:map", "dep-target", depTargetName});
            fail ("ConfigurationException not thrown");
        } catch (ConfigurationException e) {
            assertEquals(expError, e.getMessage());
        }
    }

    /**
     * Re-apply a template on a lower config level. Should result in a fail to parse because each
     * template:map can only be applied once. So missing the default will throw exception.
     */
    public void testCreateConfigurationFromArgs_template_multilevel() throws Exception {
        final String configName = "depend-template-include-config";
        final String depTargetName = "template-include-config";
        final String depTargetName2 = "template-collision-include-config";
        final String expError = String.format(
                "Failed to parse config xml '%s'. Reason: " +
                ConfigurationXmlParser.ConfigHandler.INNER_TEMPLATE_INCLUDE_ERROR,
                configName, configName, depTargetName2);
        try {
            mFactory.createConfigurationFromArgs(new String[]{configName,
                    "--template:map", "dep-target", depTargetName,
                    "--template:map", "target", depTargetName2});
            fail ("ConfigurationException not thrown");
        } catch (ConfigurationException e) {
            assertEquals(expError, e.getMessage());
        }
    }

    /**
     * Re-apply a template twice. Should result in an error only because the configs included have
     * several build_provider
     */
    public void testCreateConfigurationFromArgs_templateCollision() throws Exception {
        final String configName = "template-collision-include-config";
        final String depTargetName = "template-include-config-with-default";
        final String expError =
                "Only one config object allowed for logger, but multiple were specified.";
        try {
            mFactory.createConfigurationFromArgs(new String[]{configName,
                    "--template:map", "target-col", depTargetName,
                    "--template:map", "target-col2", depTargetName});
            fail ("ConfigurationException not thrown");
        } catch (ConfigurationException e) {
            assertEquals(expError, e.getMessage());
        }
    }

    /**
    * Test loading a config that tries to include a non-bundled config.
    */
    public void testCreateConfigurationFromArgs_nonBundledInclude() throws Exception {
       try {
           mFactory.createConfigurationFromArgs(new String[] {"non-bundled-config"});
           fail("ConfigurationException not thrown");
       } catch (ConfigurationException e) {
           // expected
       }
    }

    /**
     * Test reloading a config after it has been updated.
     */
    public void testCreateConfigurationFromArgs_localConfigReload()
            throws ConfigurationException, IOException {

        File localConfigFile = FileUtil.createTempFile("local-config", ".xml");
        try {
            // Copy the config to the local filesystem
            InputStream source = getClass().getResourceAsStream("/testconfigs/local-config.xml");
            FileUtil.writeToFile(source, localConfigFile);

            // Depending on the system, file modification times might not have greater than 1 second
            // resolution. Backdate the original contents so that when we write to the file later,
            // it shows up as a new change.
            localConfigFile.setLastModified(System.currentTimeMillis() - 5000);

            // Load the configuration from the local file
            IConfiguration config = mFactory.createConfigurationFromArgs(
                    new String[] { localConfigFile.getAbsolutePath() });
            if (!(config.getTests().get(0) instanceof StubOptionTest)) {
                fail(String.format("Expected a StubOptionTest, but got %s",
                        config.getTests().get(0).getClass().getName()));
                return;
            }
            StubOptionTest test = (StubOptionTest)config.getTests().get(0);
            assertEquals("valueFromOriginalConfig", test.mOption);

            // Change the contents of the local file
            source = getClass().getResourceAsStream("/testconfigs/local-config-update.xml");
            FileUtil.writeToFile(source, localConfigFile);

            // Get the configuration again and verify that it picked up the update
            config = mFactory.createConfigurationFromArgs(
                    new String[] { localConfigFile.getAbsolutePath() });
            if (!(config.getTests().get(0) instanceof StubOptionTest)) {
                fail(String.format("Expected a StubOptionTest, but got %s",
                        config.getTests().get(0).getClass().getName()));
                return;
            }
            test = (StubOptionTest)config.getTests().get(0);
            assertEquals("valueFromUpdatedConfig", test.mOption);
        } finally {
            FileUtil.deleteFile(localConfigFile);
        }
    }

    /**
     * Test loading a config that has a circular include
     */
    public void testCreateConfigurationFromArgs_circularInclude() throws Exception {
        try {
            mFactory.createConfigurationFromArgs(new String[] {"circular-config"});
            fail("ConfigurationException not thrown");
        } catch (ConfigurationException e) {
            // expected
        }
    }

    /**
     * If a template:map argument is passed but doesn't match any {@code <template-include>} tag
     * a configuration exception will be thrown for unmatched arguments.
     */
    public void testCreateConfigurationFromArgs_templateName_notExist() throws Exception {
        final String configName = "include-template-config-with-default";
        final String targetName = "test-config";
        final String missingNameTemplate = "NOTEXISTINGNAME";
        Map<String, String> expected = new HashMap<String,String>();
        expected.put(missingNameTemplate, targetName);
        final String expError = String.format(
                "Unused template:map parameters: %s", expected);

        try {
            mFactory.createConfigurationFromArgs(new String[]{configName,
                    "--template:map", missingNameTemplate, targetName});
            fail ("ConfigurationException not thrown");
        } catch (ConfigurationException e) {
            // Make sure that we get the expected error message
            assertEquals(expError, e.getMessage());
        }
    }

    /**
     * If a configuration is called a second time, ensure that the cached config is also properly
     * returned, and that template:map did not cause issues.
     */
    public void testCreateConfigurationFromArgs_templateName_notExistTest() throws Exception {
        final String configName = "template-include-config-with-default";
        final String targetName = "local-config";
        final String nameTemplate = "target";
        Map<String, String> expected = new HashMap<String,String>();
        expected.put(nameTemplate, targetName);
        IConfiguration tmp = null;
        try {
            tmp = mFactory.createConfigurationFromArgs(new String[]{configName,
                    "--template:map", nameTemplate, targetName});
        } catch (ConfigurationException e) {
            fail("ConfigurationException thrown: " + e.getMessage());
        }
        assertTrue(tmp.getTests().size() == 2);

        // Call the same config a second time to make sure the cached version works.
        try {
            tmp = mFactory.createConfigurationFromArgs(new String[]{configName,
                    "--template:map", nameTemplate, targetName});
        } catch (ConfigurationException e) {
            fail("ConfigurationException thrown: " + e.getMessage());
        }
        assertTrue(tmp.getTests().size() == 2);
    }

    /**
     * If a configuration is called a second time with bad template name, it should still throw
     * the unused config template:map
     */
    public void testCreateConfigurationFromArgs_templateName_stillThrow() throws Exception {
        final String configName = "template-include-config-with-default";
        final String targetName = "local-config";
        final String nameTemplate = "target_not_exist";
        try {
            mFactory.createConfigurationFromArgs(new String[]{configName,
                    "--template:map", nameTemplate, targetName});
            fail("ConfigurationException should have been thrown");
        } catch (ConfigurationException e) {
            // expected
        }

        // Call the same config a second time to make sure it is also rejected.
        try {
            mFactory.createConfigurationFromArgs(new String[]{configName,
                    "--template:map", nameTemplate, targetName});
            fail("ConfigurationException should have been thrown");
        } catch (ConfigurationException e) {
            // expected
        }
    }

    /**
     * Parse a config with 3 different device configuration specified.
     */
    public void testCreateConfigurationFromArgs_multidevice() throws Exception {
        IConfiguration config = mFactory.createConfigurationFromArgs(
                new String[]{"multi-device"});
        assertEquals(1, config.getTests().size());
        assertTrue(config.getTests().get(0) instanceof StubOptionTest);
        // Verify that all attributes are in the right place:
        assertNotNull(config.getDeviceConfigByName("device1"));
        assertEquals("10", config.getDeviceConfigByName("device1")
                .getBuildProvider().getBuild().getBuildId());
        assertEquals("stub", config.getDeviceConfigByName("device1")
                .getBuildProvider().getBuild().getTestTag());
        assertEquals(0, config.getDeviceConfigByName("device1")
                .getTargetPreparers().size());

        assertNotNull(config.getDeviceConfigByName("device2"));
        assertEquals("0", config.getDeviceConfigByName("device2")
                .getBuildProvider().getBuild().getBuildId());
        assertEquals("stub", config.getDeviceConfigByName("device2")
                .getBuildProvider().getBuild().getTestTag());
        assertEquals(1, config.getDeviceConfigByName("device2")
                .getTargetPreparers().size());
        assertTrue(config.getDeviceConfigByName("device2")
                .getTargetPreparers().get(0) instanceof StubTargetPreparer);

        assertNotNull(config.getDeviceConfigByName("device3"));
        assertEquals("0", config.getDeviceConfigByName("device3")
                .getBuildProvider().getBuild().getBuildId());
        assertEquals("build-flavor3", config.getDeviceConfigByName("device3")
                .getBuildProvider().getBuild().getBuildFlavor());
        assertEquals(2, config.getDeviceConfigByName("device3")
                .getTargetPreparers().size());
        assertTrue(config.getDeviceConfigByName("device3")
                .getTargetPreparers().get(0) instanceof StubTargetPreparer);
    }

    /**
     * Test that if an object inside a <device> tag is implementing {@link IConfigurationReceiver}
     * it will receives the config properly like non-device object.
     */
    public void testCreateConfigurationFromArgs_injectConfiguration() throws Exception {
        IConfiguration config = mFactory.createConfigurationFromArgs(new String[] {"multi-device"});
        assertEquals(1, config.getTests().size());

        assertNotNull(config.getDeviceConfigByName("device2"));
        assertEquals(1, config.getDeviceConfigByName("device2").getTargetPreparers().size());
        assertTrue(
                config.getDeviceConfigByName("device2").getTargetPreparers().get(0)
                        instanceof StubTargetPreparer);
        StubTargetPreparer stubDevice2 =
                (StubTargetPreparer)
                        config.getDeviceConfigByName("device2").getTargetPreparers().get(0);
        assertEquals(config, stubDevice2.getConfiguration());

        assertNotNull(config.getDeviceConfigByName("device3"));
        assertEquals(2, config.getDeviceConfigByName("device3").getTargetPreparers().size());
        assertTrue(
                config.getDeviceConfigByName("device3").getTargetPreparers().get(0)
                        instanceof StubTargetPreparer);
        StubTargetPreparer stubDevice3 =
                (StubTargetPreparer)
                        config.getDeviceConfigByName("device3").getTargetPreparers().get(0);
        assertEquals(config, stubDevice3.getConfiguration());
    }

    /**
     * Parse a config with 3 different device configuration specified. And apply a command line to
     * override some attributes.
     */
    public void testCreateConfigurationFromArgs_multidevice_applyCommandLine() throws Exception {
        IConfiguration config = mFactory.createConfigurationFromArgs(
                new String[]{"multi-device", "--{device2}build-id","20", "--{device1}null-device",
                        "--{device3}com.android.tradefed.build.StubBuildProvider:build-id","30"});
        assertEquals(1, config.getTests().size());
        assertTrue(config.getTests().get(0) instanceof StubOptionTest);
        // Verify that all attributes are in the right place:
        assertNotNull(config.getDeviceConfigByName("device1"));
        assertEquals("10", config.getDeviceConfigByName("device1")
                .getBuildProvider().getBuild().getBuildId());
        assertEquals("stub", config.getDeviceConfigByName("device1")
                .getBuildProvider().getBuild().getTestTag());
        assertEquals(0, config.getDeviceConfigByName("device1")
                .getTargetPreparers().size());
        assertTrue(config.getDeviceConfigByName("device1")
                .getDeviceRequirements().nullDeviceRequested());

        assertNotNull(config.getDeviceConfigByName("device2"));
        // Device2 build provider is modified independently
        assertEquals("20", config.getDeviceConfigByName("device2")
                .getBuildProvider().getBuild().getBuildId());
        assertEquals("stub", config.getDeviceConfigByName("device2")
                .getBuildProvider().getBuild().getTestTag());
        assertEquals(1, config.getDeviceConfigByName("device2")
                .getTargetPreparers().size());
        assertTrue(config.getDeviceConfigByName("device2")
                .getTargetPreparers().get(0) instanceof StubTargetPreparer);
        assertFalse(config.getDeviceConfigByName("device2")
                .getDeviceRequirements().nullDeviceRequested());

        // Device3 build provider is modified independently
        assertNotNull(config.getDeviceConfigByName("device3"));
        assertEquals("30", config.getDeviceConfigByName("device3")
                .getBuildProvider().getBuild().getBuildId());
        assertEquals("build-flavor3", config.getDeviceConfigByName("device3")
                .getBuildProvider().getBuild().getBuildFlavor());
        assertEquals(2, config.getDeviceConfigByName("device3")
                .getTargetPreparers().size());
        assertTrue(config.getDeviceConfigByName("device3")
                .getTargetPreparers().get(0) instanceof StubTargetPreparer);
        assertFalse(config.getDeviceConfigByName("device3")
                .getDeviceRequirements().nullDeviceRequested());
    }

    /**
     * Parse a config with 3 different device configuration specified. And apply a command line to
     * override some attributes.
     */
    public void testCreateConfigurationFromArgs_multidevice_singletag() throws Exception {
        IConfiguration config =
                mFactory.createConfigurationFromArgs(
                        new String[] {
                            "multi-device-empty",
                            "--{device2}build-id",
                            "20",
                            "--{device1}null-device",
                            "--{device3}com.android.tradefed.build.StubBuildProvider:build-id",
                            "30"
                        });
        assertEquals(1, config.getTests().size());
        assertTrue(config.getTests().get(0) instanceof StubOptionTest);
        // Verify that all attributes are in the right place:
        assertNotNull(config.getDeviceConfigByName("device1"));
        assertEquals(
                "0",
                config.getDeviceConfigByName("device1").getBuildProvider().getBuild().getBuildId());
        assertEquals(
                "stub",
                config.getDeviceConfigByName("device1").getBuildProvider().getBuild().getTestTag());
        assertEquals(0, config.getDeviceConfigByName("device1").getTargetPreparers().size());
        assertTrue(
                config.getDeviceConfigByName("device1")
                        .getDeviceRequirements()
                        .nullDeviceRequested());

        assertNotNull(config.getDeviceConfigByName("device2"));
        // Device2 build provider is modified independently
        assertEquals(
                "20",
                config.getDeviceConfigByName("device2").getBuildProvider().getBuild().getBuildId());
        assertEquals(
                "stub",
                config.getDeviceConfigByName("device2").getBuildProvider().getBuild().getTestTag());
        assertEquals(0, config.getDeviceConfigByName("device2").getTargetPreparers().size());
        assertFalse(
                config.getDeviceConfigByName("device2")
                        .getDeviceRequirements()
                        .nullDeviceRequested());

        // Device3 build provider is modified independently
        assertNotNull(config.getDeviceConfigByName("device3"));
        assertEquals(
                "30",
                config.getDeviceConfigByName("device3").getBuildProvider().getBuild().getBuildId());
        assertEquals(0, config.getDeviceConfigByName("device3").getTargetPreparers().size());
        assertFalse(
                config.getDeviceConfigByName("device3")
                        .getDeviceRequirements()
                        .nullDeviceRequested());
    }

    /**
     * Parse a config with 3 different device configuration specified. And apply a command line to
     * override all attributes.
     */
    public void testCreateConfigurationFromArgs_multidevice_applyCommandLineGlobal()
            throws Exception {
        IConfiguration config = mFactory.createConfigurationFromArgs(
                new String[]{"multi-device", "--build-id","20"});
        assertEquals(1, config.getTests().size());
        assertTrue(config.getTests().get(0) instanceof StubOptionTest);
        // Verify that all attributes are in the right place:
        // All build id are now modified since option had a global scope
        assertNotNull(config.getDeviceConfigByName("device1"));
        assertEquals("20", config.getDeviceConfigByName("device1")
                .getBuildProvider().getBuild().getBuildId());
        assertEquals("stub", config.getDeviceConfigByName("device1")
                .getBuildProvider().getBuild().getTestTag());
        assertEquals(0, config.getDeviceConfigByName("device1")
                .getTargetPreparers().size());

        assertNotNull(config.getDeviceConfigByName("device2"));
        assertEquals("20", config.getDeviceConfigByName("device2")
                .getBuildProvider().getBuild().getBuildId());
        assertEquals("stub", config.getDeviceConfigByName("device2")
                .getBuildProvider().getBuild().getTestTag());
        assertEquals(1, config.getDeviceConfigByName("device2")
                .getTargetPreparers().size());
        assertTrue(config.getDeviceConfigByName("device2")
                .getTargetPreparers().get(0) instanceof StubTargetPreparer);

        assertNotNull(config.getDeviceConfigByName("device3"));
        assertEquals("20", config.getDeviceConfigByName("device3")
                .getBuildProvider().getBuild().getBuildId());
        assertEquals("build-flavor3", config.getDeviceConfigByName("device3")
                .getBuildProvider().getBuild().getBuildFlavor());
        assertEquals(2, config.getDeviceConfigByName("device3")
                .getTargetPreparers().size());
        assertTrue(config.getDeviceConfigByName("device3")
                .getTargetPreparers().get(0) instanceof StubTargetPreparer);
    }

    /**
     * Test that when <device> tags are out of order (device 1 - device 2 - device 1) and an option
     * is specified in the last device 1 with an increased frequency (a same class object from the
     * first device 1 or 2), the option is properly found and assigned.
     */
    public void testCreateConfigurationFromArgs_frequency() throws Exception {
        IConfiguration config =
                mFactory.createConfigurationFromArgs(new String[] {"multi-device-mix"});
        assertNotNull(config.getDeviceConfigByName("device1"));
        assertEquals(3, config.getDeviceConfigByName("device1").getTargetPreparers().size());
        assertTrue(
                config.getDeviceConfigByName("device1").getTargetPreparers().get(0)
                        instanceof DeviceWiper);
        DeviceWiper prep1 =
                (DeviceWiper) config.getDeviceConfigByName("device1").getTargetPreparers().get(0);
        assertTrue(prep1.isDisabled());
        assertTrue(
                config.getDeviceConfigByName("device1").getTargetPreparers().get(2)
                        instanceof DeviceWiper);
        DeviceWiper prep3 =
                (DeviceWiper) config.getDeviceConfigByName("device1").getTargetPreparers().get(2);
        assertFalse(prep3.isDisabled());

        assertNotNull(config.getDeviceConfigByName("device2"));
        assertEquals(1, config.getDeviceConfigByName("device2").getTargetPreparers().size());
        assertTrue(
                config.getDeviceConfigByName("device2").getTargetPreparers().get(0)
                        instanceof DeviceWiper);
        DeviceWiper prep2 =
                (DeviceWiper) config.getDeviceConfigByName("device2").getTargetPreparers().get(0);
        // Only device 1 preparer has been targeted.
        assertTrue(prep2.isDisabled());
    }

    /**
     * Tests a different usage of options for a multi device interleaved config where options are
     * specified.
     */
    public void testCreateConfigurationFromArgs_frequency_withOptionOpen() throws Exception {
        IConfiguration config =
                mFactory.createConfigurationFromArgs(new String[] {"multi-device-mix-options"});
        assertNotNull(config.getDeviceConfigByName("device1"));
        assertEquals(3, config.getDeviceConfigByName("device1").getTargetPreparers().size());
        assertTrue(
                config.getDeviceConfigByName("device1").getTargetPreparers().get(0)
                        instanceof DeviceWiper);
        DeviceWiper prep1 =
                (DeviceWiper) config.getDeviceConfigByName("device1").getTargetPreparers().get(0);
        assertTrue(prep1.isDisabled());
        assertTrue(
                config.getDeviceConfigByName("device1").getTargetPreparers().get(2)
                        instanceof DeviceWiper);
        DeviceWiper prep3 =
                (DeviceWiper) config.getDeviceConfigByName("device1").getTargetPreparers().get(2);
        assertFalse(prep3.isDisabled());

        assertNotNull(config.getDeviceConfigByName("device2"));
        assertEquals(1, config.getDeviceConfigByName("device2").getTargetPreparers().size());
        assertTrue(
                config.getDeviceConfigByName("device2").getTargetPreparers().get(0)
                        instanceof DeviceWiper);
        DeviceWiper prep2 =
                (DeviceWiper) config.getDeviceConfigByName("device2").getTargetPreparers().get(0);
        // Only device 1 preparer has been targeted.
        assertTrue(prep2.isDisabled());
    }

    /**
     * Configuration for multi device is wrong since it contains a build_provider tag outside the
     * devices tags.
     */
    public void testCreateConfigurationFromArgs_multidevice_exception() throws Exception {
        String expectedException =
                "You seem to want a multi-devices configuration but you have "
                        + "[build_provider] tags outside the <device> tags";
        try {
            mFactory.createConfigurationFromArgs(new String[]{"multi-device-outside-tag"});
            fail("Should have thrown a Configuration Exception");
        } catch(ConfigurationException e) {
            assertEquals(expectedException, e.getMessage());
        }
    }

    /**
     * Parse a config with no multi device config, and expect the new device holder to still be
     * there and adding a default device.
     */
    public void testCreateConfigurationFromArgs_old_config_with_deviceHolder() throws Exception {
        IConfiguration config = mFactory.createConfigurationFromArgs(
                new String[]{"test-config", "--build-id","20", "--serial", "test"});
        assertEquals(1, config.getTests().size());
        assertTrue(config.getTests().get(0) instanceof StubOptionTest);
        // Verify that all attributes are in the right place:
        // All build id are now modified since option had a global scope
        assertNotNull(config.getDeviceConfigByName(ConfigurationDef.DEFAULT_DEVICE_NAME));
        assertEquals("20", config.getDeviceConfigByName(ConfigurationDef.DEFAULT_DEVICE_NAME)
                .getBuildProvider().getBuild().getBuildId());
        assertEquals("stub", config.getDeviceConfigByName(ConfigurationDef.DEFAULT_DEVICE_NAME)
                .getBuildProvider().getBuild().getTestTag());
        assertEquals(1, config.getDeviceConfigByName(ConfigurationDef.DEFAULT_DEVICE_NAME)
                .getTargetPreparers().size());
        List<String> serials = new ArrayList<String>();
        serials.add("test");
        assertEquals(serials, config.getDeviceRequirements().getSerials(null));
        assertEquals(
                serials,
                config.getDeviceConfigByName(ConfigurationDef.DEFAULT_DEVICE_NAME)
                        .getDeviceRequirements()
                        .getSerials(null));
    }

    /**
     * Test that when parsing command line options, boolean options with Device tag and namespace
     * are correctly assigned.
     */
    public void testCreateConfiguration_injectDeviceBooleanOption() throws Exception {
        IConfiguration config =
                mFactory.createConfigurationFromArgs(
                        new String[] {
                            "test-config-multi",
                            "--{device1}no-test-boolean-option",
                            "--{device1}test-boolean-option-false",
                            // testing with namespace too
                            "--{device2}stub-preparer:no-test-boolean-option",
                            "--{device2}stub-preparer:test-boolean-option-false"
                        });
        assertEquals(2, config.getDeviceConfig().size());
        IDeviceConfiguration device1 = config.getDeviceConfigByName("device1");
        StubTargetPreparer deviceSetup1 = (StubTargetPreparer) device1.getTargetPreparers().get(0);
        // default value of test-boolean-option is true, we set it to false
        assertFalse(deviceSetup1.getTestBooleanOption());
        // default value of test-boolean-option-false is false, we set it to true.
        assertTrue(deviceSetup1.getTestBooleanOptionFalse());

        IDeviceConfiguration device2 = config.getDeviceConfigByName("device2");
        StubTargetPreparer deviceSetup2 = (StubTargetPreparer) device2.getTargetPreparers().get(0);
        assertFalse(deviceSetup2.getTestBooleanOption());
        assertTrue(deviceSetup2.getTestBooleanOptionFalse());
    }

    /** Test that when an <include> tag is used inside a <device> tag we correctly resolve it. */
    public void testCreateConfiguration_includeInDevice() throws Exception {
        IConfiguration config =
                mFactory.createConfigurationFromArgs(
                        new String[] {"test-config-multi-include", "--test-dir", "faketestdir"});
        assertEquals(2, config.getDeviceConfig().size());
        IDeviceConfiguration device1 = config.getDeviceConfigByName("device1");
        assertTrue(device1.getTargetPreparers().get(0) instanceof StubTargetPreparer);
        // The included config in device2 loads a different build_provider
        IDeviceConfiguration device2 = config.getDeviceConfigByName("device2");
        assertTrue(device2.getBuildProvider() instanceof LocalDeviceBuildProvider);
        LocalDeviceBuildProvider provider = (LocalDeviceBuildProvider) device2.getBuildProvider();
        // command line options are properly propagated to the included object in device tag.
        assertEquals("faketestdir", provider.getTestDir().getName());
    }

    /**
     * Test when an <include> tag tries to load a <device> tag inside another <device> tag. This
     * should throw an exception.
     */
    public void testCreateConfiguration_includeInDevice_inDevice() throws Exception {
        try {
            mFactory.createConfigurationFromArgs(
                    new String[] {
                        "multi-device-incorrect-include",
                    });
            fail("Should have thrown an exception.");
        } catch (ConfigurationException expected) {
            assertEquals(
                    "<device> tag cannot be included inside another device", expected.getMessage());
        }
    }

    /** Test that {@link ConfigurationFactory#reorderArgs(String[])} is properly reordering args. */
    public void testReorderArgs_check_ordering() throws Throwable {
        String[] args =
                new String[] {
                    "config",
                    "--option1",
                    "o1",
                    "--template:map",
                    "tm=tm1",
                    "--option2",
                    "--option3",
                    "o3",
                    "--template:map",
                    "tm",
                    "tm2"
                };
        String[] wantArgs =
                new String[] {
                    "config",
                    "--template:map",
                    "tm=tm1",
                    "--template:map",
                    "tm",
                    "tm2",
                    "--option1",
                    "o1",
                    "--option2",
                    "--option3",
                    "o3"
                };

        assertEquals(Arrays.toString(wantArgs), Arrays.toString(mFactory.reorderArgs(args)));
    }

    /**
     * Test that {@link ConfigurationFactory#reorderArgs(String[])} properly handles a short arg
     * after a template arg.
     */
    public void testReorderArgs_template_with_short_arg() throws Throwable {
        String[] args =
                new String[] {
                    "config",
                    "--option1",
                    "o1",
                    "--template:map",
                    "tm=tm1",
                    "-option2",
                    "--option3",
                    "o3",
                    "--template:map",
                    "tm",
                    "tm2"
                };
        String[] wantArgs =
                new String[] {
                    "config",
                    "--template:map",
                    "tm=tm1",
                    "--template:map",
                    "tm",
                    "tm2",
                    "--option1",
                    "o1",
                    "-option2",
                    "--option3",
                    "o3"
                };

        assertEquals(Arrays.toString(wantArgs), Arrays.toString(mFactory.reorderArgs(args)));
    }

    /**
     * Test that {@link ConfigurationFactory#reorderArgs(String[])} properly handles a incomplete
     * template arg.
     */
    public void testReorderArgs_incomplete_template_arg() throws Throwable {
        String[] args =
                new String[] {
                    "config",
                    "--option1",
                    "o1",
                    "--template:map",
                    "tm=tm1",
                    "-option2",
                    "--option3",
                    "o3",
                    "--template:map",
                };
        String[] wantArgs =
                new String[] {
                    "config",
                    "--template:map",
                    "tm=tm1",
                    "--template:map",
                    "--option1",
                    "o1",
                    "-option2",
                    "--option3",
                    "o3"
                };

        assertEquals(Arrays.toString(wantArgs), Arrays.toString(mFactory.reorderArgs(args)));
    }

    /**
     * Test that when doing a dry-run with keystore arguments, we skip the keystore validation. We
     * accept the argument, as long as the key exists.
     */
    public void testCreateConfigurationFromArgs_dryRun_keystore() throws Exception {
        IConfiguration res =
                mFactory.createConfigurationFromArgs(
                        new String[] {
                            "test-config",
                            "--build-id",
                            "USE_KEYSTORE@test_string",
                            "--dry-run",
                            "--online-wait-time=USE_KEYSTORE@test_long",
                            "--min-battery-after-recovery",
                            "USE_KEYSTORE@test_int",
                            "--disable-unresponsive-reboot=USE_KEYSTORE@test_boolean",
                        });
        res.validateOptions();
        // we still throw exception if the option itself doesn't exists.
        try {
            mFactory.createConfigurationFromArgs(
                    new String[] {
                        "test-config", "--does-not-exists", "USE_KEYSTORE@test_string", "--dry-run"
                    });
            fail("Should have thrown an exception.");
        } catch (ConfigurationException expected) {
            // expected
        }
    }

    /**
     * Test that when mandatory option are set with a keystore during a dry-run, they can still be
     * validated.
     */
    public void testCreateConfigurationFromArgs_dryRun_keystore_required_arg() throws Exception {
        IConfiguration res =
                mFactory.createConfigurationFromArgs(
                        new String[] {
                            "mandatory-config",
                            "--build-dir",
                            "USE_KEYSTORE@test_string",
                            "--dry-run",
                        });
        // Check that mandatory option was properly set, otherwise it will throw.
        res.validateOptions();
    }

    /**
     * This unit test ensures that the code will search for missing test configs in directories
     * specified in certain environment variables.
     */
    public void testSearchConfigFromEnvVar() throws IOException {
        File externalConfig = FileUtil.createTempFile("external-config", ".config");
        String configName = FileUtil.getBaseName(externalConfig.getName());
        File tmpDir = externalConfig.getParentFile();

        ConfigurationFactory spyFactory = Mockito.spy(mFactory);
        Mockito.doReturn(Arrays.asList(tmpDir)).when(spyFactory).getExternalTestCasesDirs();

        try {
            File config = spyFactory.getTestCaseConfigPath(configName);
            assertEquals(config.getAbsolutePath(), externalConfig.getAbsolutePath());
        } finally {
            FileUtil.deleteFile(externalConfig);
        }
    }

    /**
     * This unit test ensures that the code will search for missing test configs in directories
     * specified in certain environment variables, and fail as the test config still can't be found.
     */
    public void testSearchConfigFromEnvVarFailed() throws Exception {
        File tmpDir = FileUtil.createTempDir("config-check-var");
        try {
            ConfigurationFactory spyFactory = Mockito.spy(mFactory);
            Mockito.doReturn(Arrays.asList(tmpDir)).when(spyFactory).getExternalTestCasesDirs();
            File config = spyFactory.getTestCaseConfigPath("non-exist");
            assertNull(config);
            Mockito.verify(spyFactory, Mockito.times(1)).getExternalTestCasesDirs();
        } finally {
            FileUtil.recursiveDelete(tmpDir);
        }
    }

    /**
     * Tests that {@link ConfigurationFactory#getConfigNamesFromTestCases(String)} returns the
     * proper files of the subpath only.
     */
    public void testGetConfigNamesFromTestCases_subpath() throws Exception {
        File tmpDir = FileUtil.createTempDir("test-config-dir");
        try {
            FileUtil.createTempFile("testconfig1", ".config", tmpDir);
            File subDir = FileUtil.createTempDir("subdir", tmpDir);
            FileUtil.createTempFile("testconfig2", ".xml", subDir);
            ConfigurationFactory spyFactory = Mockito.spy(mFactory);
            Mockito.doReturn(Arrays.asList(tmpDir)).when(spyFactory).getExternalTestCasesDirs();
            // looking at full path we get both configs
            Set<String> res = spyFactory.getConfigNamesFromTestCases(null);
            assertEquals(2, res.size());
            res = spyFactory.getConfigNamesFromTestCases(subDir.getName());
            assertEquals(1, res.size());
            assertTrue(res.iterator().next().contains("testconfig2"));
        } finally {
            FileUtil.recursiveDelete(tmpDir);
        }
    }

    /**
     * Test that if the base configuration is single device and a template attempt to make it
     * multi-devices, it will fail and provide a clear message about which tags are not in the right
     * place.
     */
    public void testCreateConfigurationFromArgs_singleDeviceTemplate_includeMulti()
            throws Exception {
        try {
            mFactory.createConfigurationFromArgs(
                    new String[] {
                        "local-preparer-base", "--template:map", "config", "test-config-multi"
                    });
            fail("Should have thrown an exception");
        } catch (ConfigurationException expected) {
            assertEquals(
                    "You seem to want a multi-devices configuration but you have "
                            + "[target_preparer] tags outside the <device> tags",
                    expected.getMessage());
        }
    }

    /**
     * Opposite scenario of {@link
     * #testCreateConfigurationFromArgs_singleDeviceTemplate_includeMulti()} where the base template
     * is multi-devices and the template included has a single device tag.
     */
    public void testCreateConfigurationFromArgs_multiDeviceTemplate_includeSingle()
            throws Exception {
        try {
            mFactory.createConfigurationFromArgs(
                    new String[] {
                        "multi-device", "--template:map", "preparers", "local-preparer-base"
                    });
            fail("Should have thrown an exception");
        } catch (ConfigurationException expected) {
            assertEquals(
                    "You seem to want a multi-devices configuration but you have "
                            + "[target_preparer] tags outside the <device> tags",
                    expected.getMessage());
        }
    }

    /**
     * Test that if we load a configuration with some unfound class, we throw an exception and
     * report the objects that did not load.
     */
    public void testCreateConfigurationFromArgs_failedToLoadClass() throws Exception {
        try {
            mFactory.createConfigurationFromArgs(new String[] {"multi-device-incorrect"});
            fail("Should have thrown an exception");
        } catch (ClassNotFoundConfigurationException expected) {
            assertTrue(
                    expected.getMessage()
                            .contains("Failed to load some objects in the configuration:"));
            assertEquals(
                    "device1:build_provider",
                    expected.getRejectedObjects().get("com.android.tradefed.build.doesnotexists"));
            assertEquals(
                    "device3:target_preparer",
                    expected.getRejectedObjects()
                            .get("com.android.tradefed.targetprep.doesnotexistseither"));
        }
    }

    /**
     * Test that a configuration with one real device and one fake device (isFake=true) will be
     * loaded like a single device config: The objects outside the <device> tags will be part of the
     * real device.
     */
    public void testCreateConfiguration_multiDevice_fake() throws Exception {
        IConfiguration config =
                mFactory.createConfigurationFromArgs(
                        new String[] {
                            "test-config-multi-fake",
                            "--{device1}no-test-boolean-option",
                            "--{device1}test-boolean-option-false",
                            // testing with namespace too
                            "--{device1}stub-preparer:no-test-boolean-option",
                            "--{device1}stub-preparer:test-boolean-option-false"
                        });
        assertEquals(2, config.getDeviceConfig().size());
        IDeviceConfiguration device1 = config.getDeviceConfigByName("device1");
        // One target preparer from inside the device tag, one from outside.
        assertEquals(2, device1.getTargetPreparers().size());
        StubTargetPreparer deviceSetup1 = (StubTargetPreparer) device1.getTargetPreparers().get(0);
        // default value of test-boolean-option is true, we set it to false
        assertFalse(deviceSetup1.getTestBooleanOption());
        // default value of test-boolean-option-false is false, we set it to true.
        assertTrue(deviceSetup1.getTestBooleanOptionFalse());

        // Check that the second preparer, outside device1 can still receive option as {device1}.
        StubTargetPreparer deviceSetup2 = (StubTargetPreparer) device1.getTargetPreparers().get(1);
        // default value of test-boolean-option is true, we set it to false
        assertFalse(deviceSetup2.getTestBooleanOption());
        // default value of test-boolean-option-false is false, we set it to true.
        assertTrue(deviceSetup2.getTestBooleanOptionFalse());

        assertFalse(config.isDeviceConfiguredFake("device1"));
        assertTrue(config.isDeviceConfiguredFake("device2"));
    }

    /** Test that a configuration with all the device marked as isReal=false will be rejected. */
    public void testCreateConfiguration_multiDevice_real_notReal() throws Exception {
        try {
            mFactory.createConfigurationFromArgs(new String[] {"test-config-real-not-real"});
            fail("Should have thrown an exception");
        } catch (ConfigurationException expected) {
            assertEquals(
                    "Failed to parse config xml 'test-config-real-not-real'. Reason: Mismatch for "
                            + "device 'device1'. It was defined once as isFake=false, once as "
                            + "isFake=true",
                    expected.getMessage());
        }
    }

    /**
     * Test that a configuration with two real devices and one fake one (isFake=false) will reject
     * object that are at the root: We cannot decide where to put these objects.
     */
    public void testCreateConfiguration_multiDevice_twoReal_oneFake() throws Exception {
        try {
            mFactory.createConfigurationFromArgs(new String[] {"test-config-multi-3-fake"});
            fail("Should have thrown an exception");
        } catch (ConfigurationException expected) {
            assertEquals(
                    "You seem to want a multi-devices configuration but you have [target_preparer] "
                            + "tags outside the <device> tags",
                    expected.getMessage());
        }
    }

    /**
     * Test that even if multi_pre_target_prep and multi_target_prep share the same type, we do not
     * mix the objects internally.
     */
    public void testParse_multiTargetPrep() throws Exception {
        String normalConfig =
                "<configuration description=\"desc\" >\n"
                        + "  <multi_pre_target_preparer class=\""
                        + StubMultiTargetPreparer.class.getName()
                        + "\" />\n"
                        + "  <multi_target_preparer class=\""
                        + StubMultiTargetPreparer.class.getName()
                        + "\" />\n"
                        + "</configuration>";
        File tmpConfig = FileUtil.createTempFile("tmp-config-tests", ".xml");
        try {
            FileUtil.writeToFile(normalConfig, tmpConfig);
            IConfiguration config =
                    mFactory.createConfigurationFromArgs(
                            new String[] {tmpConfig.getAbsolutePath()});
            assertEquals(1, config.getMultiPreTargetPreparers().size());
            assertEquals(1, config.getMultiTargetPreparers().size());
            // Different objects have been created for each.
            assertNotSame(
                    config.getMultiPreTargetPreparers().get(0),
                    config.getMultiTargetPreparers().get(0));
        } finally {
            FileUtil.deleteFile(tmpConfig);
        }
    }
}
