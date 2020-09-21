/*
 * Copyright (C) 2016 The Android Open Source Project
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

import com.android.tradefed.command.CommandScheduler;
import com.android.tradefed.config.Option.Importance;
import com.android.tradefed.util.ArrayUtil;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.keystore.StubKeyStoreFactory;

import junit.framework.TestCase;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.InputStream;
import java.io.PrintStream;
import java.util.ArrayList;
import java.util.List;

/**
 * Unit Tests for {@link GlobalConfiguration}
 * It is difficult to test since GlobalConfiguration is a singleton and we cannot use reflection to
 * unset the instance since it might be in use in the currently running Trade Federation instance.
 */
public class GlobalConfigurationTest extends TestCase {

    GlobalConfiguration mGlobalConfig;
    private final static String OPTION_DESCRIPTION = "mandatory option should be set";
    private final static String OPTION_NAME = "manda";
    private final static String ALIAS_NAME = "aliasssss";
    private final static String EMPTY_CONFIG = "empty";
    private static final String GLOBAL_TEST_CONFIG = "global-config";
    private static final String GLOBAL_CONFIG_SERVER_TEST_CONFIG = "global-config-server-config";

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mGlobalConfig = new GlobalConfiguration(EMPTY_CONFIG, "description");
        assertNotNull(mGlobalConfig);
    }

    @OptionClass(alias = ALIAS_NAME)
    private class FakeCommandScheduler extends CommandScheduler {
        @Option(name = OPTION_NAME, description = OPTION_DESCRIPTION, mandatory = true,
                importance = Importance.IF_UNSET)
        private String mandatoryTest = null;
    }

    /**
     * Test for {@link GlobalConfiguration#validateOptions()}
     */
    public void testValidateOptions() throws Exception {
        mGlobalConfig.validateOptions();
        mGlobalConfig.setCommandScheduler(new FakeCommandScheduler());
        try {
            mGlobalConfig.validateOptions();
            fail("Should have thrown a configuration exception");
        } catch (ConfigurationException ce) {
            // expected
        }
    }

    /**
     * Test that the creation of Global configuration with basic default parameter is working
     */
    public void testCreateGlobalConfiguration_empty() throws Exception {
        String[] args = {};
        List<String> nonGlobalArgs = new ArrayList<String>(args.length);
        IConfigurationFactory configFactory = ConfigurationFactory.getInstance();
        // use the known "empty" config as a global config to avoid issues.
        String globalConfigPath = EMPTY_CONFIG;
        IGlobalConfiguration mGlobalConfig = configFactory.createGlobalConfigurationFromArgs(
                ArrayUtil.buildArray(new String[] {globalConfigPath}, args), nonGlobalArgs);
        assertTrue(nonGlobalArgs.size() == 0);
        assertNotNull(mGlobalConfig);
        mGlobalConfig.validateOptions();
    }

    /**
     * Printing is properly reading the properties
     */
    public void testPrintCommandUsage() throws Exception {
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        PrintStream ps = new PrintStream(baos, true);
        mGlobalConfig.printCommandUsage(true, ps);
        String expected = "'empty' configuration: description\n\n" +
                "Printing help for only the important options. " +
                "To see help for all options, use the --help-all flag\n\n";
        assertEquals(expected, baos.toString());

        mGlobalConfig.setCommandScheduler(new FakeCommandScheduler());
        baos = new ByteArrayOutputStream();
        ps = new PrintStream(baos, true);
        mGlobalConfig.printCommandUsage(true, ps);
        expected = String.format("'%s' command_scheduler options:\n" +
                "    --%s              %s", ALIAS_NAME, OPTION_NAME, OPTION_DESCRIPTION);
        assertTrue(baos.toString().contains(expected));
    }

    /**
     * Test that the creation of Global configuration with basic global configuration that is not
     * empty.
     */
    public void testCreateGlobalConfiguration_nonEmpty() throws Exception {
        // one global arg, one non-global arg
        String[] args = {"test-tag", "test"};
        List<String> nonGlobalArgs = new ArrayList<String>(args.length);
        // In order to find our test config, we provide our testconfigs/ folder. Otherwise only
        // the config/ folder is searched for configuration by default.
        IConfigurationFactory configFactory =
                new ConfigurationFactory() {
                    @Override
                    protected String getConfigPrefix() {
                        return "testconfigs/";
                    }
                };
        String globalConfigPath = GLOBAL_TEST_CONFIG;
        IGlobalConfiguration mGlobalConfig = configFactory.createGlobalConfigurationFromArgs(
                ArrayUtil.buildArray(new String[] {globalConfigPath}, args), nonGlobalArgs);
        assertNotNull(mGlobalConfig);
        assertNotNull(mGlobalConfig.getDeviceMonitors());
        assertNotNull(mGlobalConfig.getWtfHandler());
        assertNotNull(mGlobalConfig.getKeyStoreFactory());
        mGlobalConfig.validateOptions();
        // Only --test-tag test remains, the global config name has been removed.
        assertTrue(nonGlobalArgs.size() == 2);
    }

    /**
     * Test that a subset of Global configuration can be created based on the default white list
     * filtering defined in GlobalConfiguration.
     */
    public void testCreateGlobalConfiguration_cloneConfigWithFilterByDefault() throws Exception {
        String[] args = {};
        List<String> nonGlobalArgs = new ArrayList<String>();
        // In order to find our test config, we provide our testconfigs/ folder. Otherwise only
        // the config/ folder is searched for configuration by default.
        IConfigurationFactory configFactory =
                new ConfigurationFactory() {
                    @Override
                    protected String getConfigPrefix() {
                        return "testconfigs/";
                    }
                };
        String globalConfigPath = GLOBAL_TEST_CONFIG;
        IGlobalConfiguration globalConfig =
                configFactory.createGlobalConfigurationFromArgs(
                        ArrayUtil.buildArray(new String[] {globalConfigPath}, args), nonGlobalArgs);

        File tmpXml = FileUtil.createTempFile("filtered_global_config", ".xml");
        try {
            globalConfig.cloneConfigWithFilter(tmpXml, null);

            // Load the filtered XML and confirm it has desired content.
            IGlobalConfiguration filteredGlobalConfig =
                    configFactory.createGlobalConfigurationFromArgs(
                            ArrayUtil.buildArray(new String[] {tmpXml.toString()}, args),
                            nonGlobalArgs);
            assertNotNull(filteredGlobalConfig);
            assertNotNull(filteredGlobalConfig.getKeyStoreFactory());
            filteredGlobalConfig.validateOptions();
            // Fail if any configuration not in the white list presents.
            assertNull(filteredGlobalConfig.getDeviceMonitors());
            assertNull(filteredGlobalConfig.getWtfHandler());
        } finally {
            FileUtil.deleteFile(tmpXml);
        }
    }

    /** Test that a subset of Global configuration can be created based on a given white list. */
    public void testCreateGlobalConfiguration_cloneConfigWithFilter() throws Exception {
        String[] args = {};
        List<String> nonGlobalArgs = new ArrayList<String>();
        // In order to find our test config, we provide our testconfigs/ folder. Otherwise only
        // the config/ folder is searched for configuration by default.
        IConfigurationFactory configFactory =
                new ConfigurationFactory() {
                    @Override
                    protected String getConfigPrefix() {
                        return "testconfigs/";
                    }
                };
        String globalConfigPath = GLOBAL_TEST_CONFIG;
        IGlobalConfiguration globalConfig =
                configFactory.createGlobalConfigurationFromArgs(
                        ArrayUtil.buildArray(new String[] {globalConfigPath}, args), nonGlobalArgs);

        File tmpXml = FileUtil.createTempFile("filtered_global_config", ".xml");
        try {
            globalConfig.cloneConfigWithFilter(tmpXml, new String[] {"wtf_handler"});

            // Load the filtered XML and confirm it has desired content.
            IGlobalConfiguration filteredGlobalConfig =
                    configFactory.createGlobalConfigurationFromArgs(
                            ArrayUtil.buildArray(new String[] {tmpXml.toString()}, args),
                            nonGlobalArgs);
            assertNotNull(filteredGlobalConfig);
            assertNotNull(filteredGlobalConfig.getWtfHandler());
            filteredGlobalConfig.validateOptions();
            // Fail if any configuration not in the white list presents.
            assertNull(filteredGlobalConfig.getDeviceMonitors());
            assertTrue(filteredGlobalConfig.getKeyStoreFactory() instanceof StubKeyStoreFactory);
        } finally {
            FileUtil.deleteFile(tmpXml);
        }
    }

    public static class StubConfigServer implements IConfigurationServer {

        @Option(name = "server-name", description = "Config server's name.")
        private String mServerName;

        @Override
        public InputStream getConfig(String name) throws ConfigurationException {
            return null;
        }

        @Override
        public String getCurrentHostConfig() throws ConfigurationException {
            return "current-host-config.xml";
        }

        public String getServerName() {
            return mServerName;
        }
    }

    /** Test global configuration load from config server. */
    public void testCreateGlobalConfiguration_configServer() throws Exception {
        IConfigurationFactory configFactory =
                new ConfigurationFactory() {
                    @Override
                    protected String getConfigPrefix() {
                        return "testconfigs/";
                    }
                };
        String[] args = {"--server-name", "stub", "test-tag", "test"};
        String globalConfigServerConfigPath = GLOBAL_CONFIG_SERVER_TEST_CONFIG;
        List<String> nonConfigServerArgs = new ArrayList<String>(args.length);
        IGlobalConfiguration configServerConfig =
                configFactory.createGlobalConfigurationFromArgs(
                        ArrayUtil.buildArray(new String[] {globalConfigServerConfigPath}, args),
                        nonConfigServerArgs);
        StubConfigServer globalConfigServer =
                (StubConfigServer) configServerConfig.getGlobalConfigServer();
        assertNotNull(globalConfigServer);
        assertEquals("stub", globalConfigServer.getServerName());
        assertEquals("current-host-config.xml", globalConfigServer.getCurrentHostConfig());
        assertTrue(nonConfigServerArgs.size() == 2);
    }
}
