/*
 * Copyright (C) 2018 The Android Open Source Project
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

package com.android.tradefed.config.gcs;

import com.android.tradefed.config.ConfigurationDef;
import com.android.tradefed.config.ConfigurationException;
import com.android.tradefed.config.IConfigurationServer;
import com.android.tradefed.config.IGlobalConfiguration;
import com.android.tradefed.config.gcs.GCSConfigurationFactory.GCSConfigLoader;
import com.android.tradefed.util.StreamUtil;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.io.ByteArrayInputStream;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.List;

/** Unit test for {@link GCSConfigurationFactory}; */
@RunWith(JUnit4.class)
public class GCSConfigurationFactoryTest {

    private static final String COMMON_CONFIG =
            "<configuration>"
                    + "<device_manager class=\"com.android.tradefed.device.DeviceManager\" />\n"
                    + "</configuration>";
    private static final String HOST_CONFIG =
            "<configuration>\n"
                    + "<include name=\"../common.xml\"/>\n"
                    + "<option name=\"dmgr:max-null-devices\" value=\"2\" />\n"
                    + "</configuration>";
    private static final String AVD_HOST_CONFIG =
            "<configuration>\n" + "<include name=\"host-config.xml\"/>\n" + "</configuration>";
    private static final String INVALID_CONFIG =
            "<configuration>\n"
                    + "<include name=\"not-exist-host-config.xml\"/>\n"
                    + "</configuration>";
    private IConfigurationServer mConfigServer;
    private GCSConfigurationFactory mGCSConfigurationFactory;
    private GCSConfigLoader mGCSConfigLoader;

    @Before
    public void setUp() {
        mConfigServer =
                new IConfigurationServer() {
                    @Override
                    public String getCurrentHostConfig() throws ConfigurationException {
                        return null;
                    }

                    @Override
                    public InputStream getConfig(String name) throws ConfigurationException {
                        String content = null;
                        if (name.equals("cluster/host-config.xml")) {
                            content = HOST_CONFIG;
                        } else if (name.equals("cluster/avd-host-config.xml")) {
                            content = AVD_HOST_CONFIG;
                        } else if (name.equals("common.xml")) {
                            content = COMMON_CONFIG;
                        } else if (name.equals("cluster/invalid-host-config.xml")) {
                            content = INVALID_CONFIG;
                        }
                        if (content != null) {
                            return new ByteArrayInputStream(content.getBytes());
                        }
                        return null;
                    }
                };
        mGCSConfigurationFactory =
                new GCSConfigurationFactory(mConfigServer) {
                    @Override
                    protected String getConfigPrefix() {
                        return "testconfigs/";
                    }
                };
        mGCSConfigLoader = mGCSConfigurationFactory.new GCSConfigLoader(true);
    }

    @After
    public void tearDown() {}

    @Test
    public void testGetConfigStream() throws Exception {
        InputStream input = mGCSConfigurationFactory.getConfigStream("cluster/host-config.xml");
        String content = StreamUtil.getStringFromStream(input);
        Assert.assertEquals(HOST_CONFIG, content);
    }

    @Test
    public void testGetConfigStream_notExist() throws Exception {
        try {
            mGCSConfigurationFactory.getConfigStream("cluster/not-exist-host-config.xml");
            Assert.fail("Should throw ConfigurationException.");
        } catch (ConfigurationException e) {
            // Expect to throw ConfigureationException.
            Assert.assertEquals(
                    "Could not find configuration 'cluster/not-exist-host-config.xml'",
                    e.getMessage());
        }
    }

    @Test
    public void testGetConfigStream_bundled() throws Exception {
        InputStream input = mGCSConfigurationFactory.getConfigStream("test-config");
        String content = StreamUtil.getStringFromStream(input);
        Assert.assertTrue(
                content.contains("test that jar plugins to Tradefed can contribute config.xml"));
    }

    @Test
    public void testGetConfigurationDef() throws Exception {
        ConfigurationDef def =
                mGCSConfigurationFactory.getConfigurationDef("common.xml", true, null);
        Assert.assertNotNull(def);
    }

    @Test
    public void testGetConfigurationDef_include() throws Exception {
        ConfigurationDef def =
                mGCSConfigurationFactory.getConfigurationDef(
                        "cluster/avd-host-config.xml", true, null);
        Assert.assertNotNull(def);
    }

    @Test
    public void testGetConfigurationDef_includeInvalid() throws Exception {
        try {
            mGCSConfigurationFactory.getConfigurationDef(
                    "cluster/invalid-avd-host-config.xml", true, null);
            Assert.fail("Should throw ConfigurationException.");
        } catch (ConfigurationException e) {
            // Expected to throw ConfigurationException.
            Assert.assertEquals(
                    "Could not find configuration 'cluster/invalid-avd-host-config.xml'",
                    e.getMessage());
        }
    }

    @Test
    public void testFindConfigName() throws Exception {
        String config = mGCSConfigLoader.findConfigName("cluster/host-config.xml", null);
        Assert.assertEquals("cluster/host-config.xml", config);
    }

    @Test
    public void testFindConfigName_bundled() throws Exception {
        String config = mGCSConfigLoader.findConfigName("test-config", null);
        Assert.assertEquals("test-config", config);
    }

    @Test
    public void testFindConfigName_bundledParent() throws Exception {
        String config =
                mGCSConfigLoader.findConfigName("cluster/avd-host-config.xml", "test-config");
        Assert.assertEquals("cluster/avd-host-config.xml", config);
    }

    @Test
    public void testFindConfigName_unbundledParent() throws Exception {
        String config =
                mGCSConfigLoader.findConfigName("host-config.xml", "cluster/avd-host-config.xml");
        Assert.assertEquals("cluster/host-config.xml", config);
    }

    @Test
    public void getPath() throws Exception {
        String path = mGCSConfigLoader.getPath("path/to/file.xml", "file1.xml");
        Assert.assertEquals("path/to/file1.xml", path);
    }

    @Test
    public void getPath_withRelative() throws Exception {
        String path = mGCSConfigLoader.getPath("path/to/file.xml", "../file1.xml");
        Assert.assertEquals("path/file1.xml", path);
    }

    @Test
    public void testCreateGlobalConfigurationFromArgs() throws Exception {
        List<String> remainingArgs = new ArrayList<String>();
        IGlobalConfiguration config =
                mGCSConfigurationFactory.createGlobalConfigurationFromArgs(
                        new String[] {"cluster/avd-host-config.xml"}, remainingArgs);
        Assert.assertNotNull(config);
        Assert.assertNotNull(config.getDeviceManager());
        Assert.assertTrue(remainingArgs.isEmpty());
    }
}
