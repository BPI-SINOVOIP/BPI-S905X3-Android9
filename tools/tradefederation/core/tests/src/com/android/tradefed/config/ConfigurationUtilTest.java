/*
 * Copyright (C) 2017 The Android Open Source Project
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
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import com.android.tradefed.command.CommandScheduler;
import com.android.tradefed.device.DeviceManager;
import com.android.tradefed.util.FileUtil;

import org.junit.Test;
import org.kxml2.io.KXmlSerializer;

import java.io.File;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Set;

/** Unit tests for {@link ConfigurationUtil} */
public class ConfigurationUtilTest {

    private static final String DEVICE_MANAGER_TYPE_NAME = "device_manager";

    /**
     * Test {@link ConfigurationUtil#dumpClassToXml(KXmlSerializer, String, Object, List)} to create
     * a dump of a configuration.
     */
    @Test
    public void testDumpClassToXml() throws Throwable {
        File tmpXml = FileUtil.createTempFile("global_config", ".xml");
        try {
            PrintWriter output = new PrintWriter(tmpXml);
            KXmlSerializer serializer = new KXmlSerializer();
            serializer.setOutput(output);
            serializer.setFeature("http://xmlpull.org/v1/doc/features.html#indent-output", true);
            serializer.startDocument("UTF-8", null);
            serializer.startTag(null, ConfigurationUtil.CONFIGURATION_NAME);

            DeviceManager deviceManager = new DeviceManager();
            ConfigurationUtil.dumpClassToXml(
                    serializer, DEVICE_MANAGER_TYPE_NAME, deviceManager, new ArrayList<String>());

            serializer.endTag(null, ConfigurationUtil.CONFIGURATION_NAME);
            serializer.endDocument();

            // Read the dump XML file, make sure configurations can be loaded.
            String content = FileUtil.readStringFromFile(tmpXml);
            assertTrue(content.length() > 100);
            assertTrue(content.contains("<configuration>"));
            assertTrue(content.contains("<option name=\"adb-path\" value=\"adb\" />"));
            assertTrue(
                    content.contains(
                            "<device_manager class=\"com.android.tradefed.device."
                                    + "DeviceManager\">"));
        } finally {
            FileUtil.deleteFile(tmpXml);
        }
    }

    /**
     * Test {@link ConfigurationUtil#dumpClassToXml(KXmlSerializer, String, Object, List)} to create
     * a dump of a configuration with filters
     */
    @Test
    public void testDumpClassToXml_filtered() throws Throwable {
        File tmpXml = FileUtil.createTempFile("global_config", ".xml");
        try {
            PrintWriter output = new PrintWriter(tmpXml);
            KXmlSerializer serializer = new KXmlSerializer();
            serializer.setOutput(output);
            serializer.setFeature("http://xmlpull.org/v1/doc/features.html#indent-output", true);
            serializer.startDocument("UTF-8", null);
            serializer.startTag(null, ConfigurationUtil.CONFIGURATION_NAME);

            DeviceManager deviceManager = new DeviceManager();
            ConfigurationUtil.dumpClassToXml(
                    serializer,
                    GlobalConfiguration.DEVICE_MANAGER_TYPE_NAME,
                    deviceManager,
                    Arrays.asList("com.android.tradefed.device.DeviceManager"));
            ConfigurationUtil.dumpClassToXml(
                    serializer,
                    GlobalConfiguration.SCHEDULER_TYPE_NAME,
                    new CommandScheduler(),
                    Arrays.asList("com.android.tradefed.device.DeviceManager"));

            serializer.endTag(null, ConfigurationUtil.CONFIGURATION_NAME);
            serializer.endDocument();
            // Read the dump XML file, make sure configurations can be loaded.
            String content = FileUtil.readStringFromFile(tmpXml);
            assertTrue(content.contains("<configuration>"));
            assertFalse(
                    content.contains(
                            "<device_manager class=\"com.android.tradefed.device."
                                    + "DeviceManager\">"));
            assertTrue(
                    content.contains(
                            "<command_scheduler class=\"com.android.tradefed.command."
                                    + "CommandScheduler\">"));
        } finally {
            FileUtil.deleteFile(tmpXml);
        }
    }

    /**
     * Test {@link ConfigurationUtil#getConfigNamesFromDirs(String, List)} is able to retrieve the
     * test configs from given directories.
     */
    @Test
    public void testGetConfigNamesFromDirs() throws Exception {
        File tmpDir = null;
        try {
            tmpDir = FileUtil.createTempDir("test_configs_dir");
            // Test config (.config) located in the root directory
            File config1 = FileUtil.createTempFile("config", ".config", tmpDir);
            // Test config (.xml) located in a sub directory
            File subDir = FileUtil.getFileForPath(tmpDir, "sub");
            FileUtil.mkdirsRWX(subDir);
            File config2 = FileUtil.createTempFile("config", ".xml", subDir);

            // Test getConfigNamesFromDirs only locate configs under subPath.
            Set<String> configs =
                    ConfigurationUtil.getConfigNamesFromDirs("sub", Arrays.asList(tmpDir));
            assertEquals(1, configs.size());
            assertTrue(configs.contains(config2.getAbsolutePath()));

            // Test getConfigNamesFromDirs locate all configs.
            configs = ConfigurationUtil.getConfigNamesFromDirs("", Arrays.asList(tmpDir));
            assertEquals(2, configs.size());
            assertTrue(configs.contains(config1.getAbsolutePath()));
            assertTrue(configs.contains(config2.getAbsolutePath()));
        } finally {
            FileUtil.recursiveDelete(tmpDir);
        }
    }

    /**
     * Test {@link ConfigurationUtil#getConfigNamesFileFromDirs(String, List, List)} can search for
     * file in directories based on patterns.
     */
    @Test
    public void testGetConfigNamesFromDirs_patterns() throws Exception {
        File tmpDir = null;
        try {
            tmpDir = FileUtil.createTempDir("test_configs_dir");
            // Random file located in the root directory
            FileUtil.createTempFile("whatevername", ".whateverextension", tmpDir);
            // Other file located in a sub directory
            File subDir = FileUtil.getFileForPath(tmpDir, "sub");
            FileUtil.mkdirsRWX(subDir);
            File config2 = FileUtil.createTempFile("config", ".otherext", subDir);

            List<String> patterns = new ArrayList<>();
            patterns.add(".*.other.*");
            // Test getConfigNamesFileFromDirs only locate configs under subPath.
            Set<File> configs =
                    ConfigurationUtil.getConfigNamesFileFromDirs(
                            "sub", Arrays.asList(tmpDir), patterns);
            assertEquals(1, configs.size());
            assertTrue(configs.contains(config2));
        } finally {
            FileUtil.recursiveDelete(tmpDir);
        }
    }
}
