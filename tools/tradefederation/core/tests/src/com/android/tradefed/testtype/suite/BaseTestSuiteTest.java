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
package com.android.tradefed.testtype.suite;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertThat;
import static org.junit.Assert.assertTrue;
import static org.hamcrest.CoreMatchers.*;
import static org.junit.Assert.fail;

import com.android.tradefed.build.DeviceBuildInfo;
import com.android.tradefed.build.IDeviceBuildInfo;
import com.android.tradefed.config.IConfiguration;
import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.testtype.Abi;
import com.android.tradefed.testtype.IAbi;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.util.AbiUtils;
import com.android.tradefed.util.FileUtil;


import org.easymock.EasyMock;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.io.File;
import java.util.Collection;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.Set;

/** Unit tests for {@link BaseTestSuite}. */
@RunWith(JUnit4.class)
public class BaseTestSuiteTest {
    private BaseTestSuite mRunner;
    private IDeviceBuildInfo mBuildInfo;
    private ITestDevice mMockDevice;

    private static final String TEST_MODULE = "test-module";

    @Before
    public void setUp() throws Exception {
        mMockDevice = EasyMock.createMock(ITestDevice.class);
        mBuildInfo = new DeviceBuildInfo();
        mRunner = new AbiBaseTestSuite();
        mRunner.setBuild(mBuildInfo);
        mRunner.setDevice(mMockDevice);

        EasyMock.expect(mMockDevice.getProperty(EasyMock.anyObject())).andReturn("arm64-v8a");
        EasyMock.expect(mMockDevice.getProperty(EasyMock.anyObject())).andReturn("armeabi-v7a");
        EasyMock.replay(mMockDevice);
    }
    /**
     * Test BaseTestSuite that hardcodes the abis to avoid failures related to running the tests
     * against a particular abi build of tradefed.
     */
    public static class AbiBaseTestSuite extends BaseTestSuite {
        @Override
        public Set<IAbi> getAbis(ITestDevice device) throws DeviceNotAvailableException {
            Set<IAbi> abis = new HashSet<>();
            abis.add(new Abi("arm64-v8a", AbiUtils.getBitness("arm64-v8a")));
            abis.add(new Abi("armeabi-v7a", AbiUtils.getBitness("armeabi-v7a")));
            return abis;
        }
    }

    /** Test for {@#link BaseTestSuite#setupFilters()} implementation, no modules match. */
    @Test
    public void testSetupFilters_noMatch() throws Exception {
        File tmpDir = FileUtil.createTempDir(TEST_MODULE);
        File moduleConfig = new File(tmpDir, "module_name.config");
        moduleConfig.createNewFile();
        try {
            OptionSetter setter = new OptionSetter(mRunner);
            setter.setOptionValue("module", "my_module");
            mRunner.setupFilters(tmpDir);
            fail("Should have thrown exception");
        } catch (IllegalArgumentException expected) {
            assertEquals("No modules found matching my_module", expected.getMessage());
        } finally {
            FileUtil.recursiveDelete(tmpDir);
        }
    }

    /** Test for {@#link BaseTestSuite#setupFilters()} implementation, only one module matches. */
    @Test
    public void testSetupFilters_oneMatch() throws Exception {
        File tmpDir = FileUtil.createTempDir(TEST_MODULE);
        File moduleConfig = new File(tmpDir, "module_name.config");
        File moduleConfig2 = new File(tmpDir, "module_name2.config");
        moduleConfig.createNewFile();
        moduleConfig2.createNewFile();
        try {
            OptionSetter setter = new OptionSetter(mRunner);
            setter.setOptionValue("module", "module_name2");
            mRunner.setupFilters(tmpDir);
            assertEquals(1, mRunner.getIncludeFilter().size());
            assertThat(
                    mRunner.getIncludeFilter(),
                    hasItem(
                            new SuiteTestFilter(mRunner.getRequestedAbi(), "module_name2", null)
                                    .toString()));
        } finally {
            FileUtil.recursiveDelete(tmpDir);
        }
    }

    /**
     * Test for {@#link BaseTestSuite#setupFilters()} implementation, multi modules match prefix but
     * don't exact match.
     */
    @Test
    public void testSetupFilters_multiMatchNoExactMatch() throws Exception {
        File tmpDir = FileUtil.createTempDir(TEST_MODULE);
        File moduleConfig = new File(tmpDir, "module_name1.config");
        File moduleConfig2 = new File(tmpDir, "module_name2.config");
        moduleConfig.createNewFile();
        moduleConfig2.createNewFile();
        try {
            OptionSetter setter = new OptionSetter(mRunner);
            setter.setOptionValue("module", "module_name");
            mRunner.setupFilters(tmpDir);
            fail("Should have thrown exception");
        } catch (IllegalArgumentException expected) {
            assertThat(
                    expected.getMessage(),
                    containsString("Multiple modules found matching module_name:"));
        } finally {
            FileUtil.recursiveDelete(tmpDir);
        }
    }

    /**
     * Test for {@#link BaseTestSuite#setupFilters()} implementation, multi modules match prefix and
     * one matches exactly.
     */
    @Test
    public void testSetupFilters_multiMatchOneExactMatch() throws Exception {
        File tmpDir = FileUtil.createTempDir(TEST_MODULE);
        File moduleConfig = new File(tmpDir, "module_name.config");
        File moduleConfig2 = new File(tmpDir, "module_name2.config");
        moduleConfig.createNewFile();
        moduleConfig2.createNewFile();
        try {
            OptionSetter setter = new OptionSetter(mRunner);
            setter.setOptionValue("module", "module_name");
            mRunner.setupFilters(tmpDir);
            assertEquals(1, mRunner.getIncludeFilter().size());
            assertThat(
                    mRunner.getIncludeFilter(),
                    hasItem(
                            new SuiteTestFilter(mRunner.getRequestedAbi(), "module_name", null)
                                    .toString()));
        } finally {
            FileUtil.recursiveDelete(tmpDir);
        }
    }

    /**
     * Test for {@link BaseTestSuite#loadTests()} implementation, for basic example configurations.
     */
    @Test
    public void testLoadTests() throws Exception {
        OptionSetter setter = new OptionSetter(mRunner);
        setter.setOptionValue("suite-config-prefix", "suite");
        setter.setOptionValue("run-suite-tag", "example-suite");
        LinkedHashMap<String, IConfiguration> configMap = mRunner.loadTests();
        assertEquals(4, configMap.size());
        assertTrue(configMap.containsKey("arm64-v8a suite/stub1"));
        assertTrue(configMap.containsKey("arm64-v8a suite/stub2"));
        assertTrue(configMap.containsKey("armeabi-v7a suite/stub1"));
        assertTrue(configMap.containsKey("armeabi-v7a suite/stub2"));
    }

    /**
     * Test for {@link BaseTestSuite#loadTests()} implementation, only stub1.xml is part of this
     * suite.
     */
    @Test
    public void testLoadTests_suite2() throws Exception {
        OptionSetter setter = new OptionSetter(mRunner);
        setter.setOptionValue("suite-config-prefix", "suite");
        setter.setOptionValue("run-suite-tag", "example-suite2");
        LinkedHashMap<String, IConfiguration> configMap = mRunner.loadTests();
        assertEquals(2, configMap.size());
        assertTrue(configMap.containsKey("arm64-v8a suite/stub1"));
        assertTrue(configMap.containsKey("armeabi-v7a suite/stub1"));
    }

    /** Test that when splitting, the instance of the implementation is used. */
    @Test
    public void testSplit() throws Exception {
        OptionSetter setter = new OptionSetter(mRunner);
        setter.setOptionValue("suite-config-prefix", "suite");
        setter.setOptionValue("run-suite-tag", "example-suite");
        Collection<IRemoteTest> tests = mRunner.split(2);
        assertEquals(4, tests.size());
        for (IRemoteTest test : tests) {
            assertTrue(test instanceof BaseTestSuite);
        }
    }

    /**
     * Test that when {@link BaseTestSuite} run-suite-tag is not set we cannot shard since there is
     * no configuration.
     */
    @Test
    public void testSplit_nothingToLoad() throws Exception {
        OptionSetter setter = new OptionSetter(mRunner);
        setter.setOptionValue("suite-config-prefix", "doesnotexists");
        setter.setOptionValue("run-suite-tag", "doesnotexists");
        assertNull(mRunner.split(2));
    }

    /**
     * Test for {@link BaseTestSuite#loadTests()} that when a test config supports IAbiReceiver,
     * multiple instances of the config are queued up.
     */
    @Test
    public void testLoadTestsForMultiAbi() throws Exception {
        ITestDevice mockDevice = EasyMock.createMock(ITestDevice.class);
        mRunner.setDevice(mockDevice);
        OptionSetter setter = new OptionSetter(mRunner);
        setter.setOptionValue("suite-config-prefix", "suite");
        setter.setOptionValue("run-suite-tag", "example-suite-abi");
        EasyMock.replay(mockDevice);
        LinkedHashMap<String, IConfiguration> configMap = mRunner.loadTests();
        assertEquals(2, configMap.size());
        assertTrue(configMap.containsKey("arm64-v8a suite/stubAbi"));
        assertTrue(configMap.containsKey("armeabi-v7a suite/stubAbi"));
        EasyMock.verify(mockDevice);
    }
}
