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
import static org.junit.Assert.assertTrue;

import com.android.tradefed.build.IDeviceBuildInfo;
import com.android.tradefed.config.IConfiguration;
import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.testtype.Abi;
import com.android.tradefed.testtype.IAbi;
import com.android.tradefed.testtype.InstrumentationTest;
import com.android.tradefed.util.AbiUtils;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.ZipUtil;

import org.easymock.EasyMock;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.io.File;
import java.io.InputStream;
import java.nio.file.Paths;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.Set;

/** Unit tests for {@link TestMappingSuiteRunner}. */
@RunWith(JUnit4.class)
public class TestMappingSuiteRunnerTest {

    private static final String ABI_1 = "arm64-v8a";
    private static final String ABI_2 = "armeabi-v7a";
    private static final String NON_EXISTING_DIR = "non-existing-dir";
    private static final String TEST_DATA_DIR = "testdata";
    private static final String TEST_MAPPING = "TEST_MAPPING";
    private static final String TEST_MAPPINGS_ZIP = "test_mappings.zip";

    private TestMappingSuiteRunner mRunner;
    private IDeviceBuildInfo mBuildInfo;
    private ITestDevice mMockDevice;

    @Before
    public void setUp() throws Exception {
        mMockDevice = EasyMock.createMock(ITestDevice.class);
        mBuildInfo = EasyMock.createMock(IDeviceBuildInfo.class);
        mRunner =
                new TestMappingSuiteRunner() {
                    @Override
                    public Set<IAbi> getAbis(ITestDevice device)
                            throws DeviceNotAvailableException {
                        Set<IAbi> abis = new HashSet<>();
                        abis.add(new Abi(ABI_1, AbiUtils.getBitness(ABI_1)));
                        abis.add(new Abi(ABI_2, AbiUtils.getBitness(ABI_2)));
                        return abis;
                    }
                };
        mRunner.setBuild(mBuildInfo);
        mRunner.setDevice(mMockDevice);

        EasyMock.expect(mBuildInfo.getTestsDir()).andReturn(new File(NON_EXISTING_DIR));
        EasyMock.expect(mMockDevice.getProperty(EasyMock.anyObject())).andReturn(ABI_1);
        EasyMock.expect(mMockDevice.getProperty(EasyMock.anyObject())).andReturn(ABI_2);
        EasyMock.replay(mBuildInfo, mMockDevice);
    }

    /**
     * Test for {@link TestMappingSuiteRunner#loadTests()} to fail when both options include-filter
     * and test-mapping-test-group are set.
     */
    @Test(expected = RuntimeException.class)
    public void testLoadTests_conflictTestGroup() throws Exception {
        OptionSetter setter = new OptionSetter(mRunner);
        setter.setOptionValue("include-filter", "test1");
        setter.setOptionValue("test-mapping-test-group", "group");
        mRunner.loadTests();
    }

    /** Test for {@link TestMappingSuiteRunner#loadTests()} to fail when no test option is set. */
    @Test(expected = RuntimeException.class)
    public void testLoadTests_noOption() throws Exception {
        mRunner.loadTests();
    }

    /**
     * Test for {@link TestMappingSuiteRunner#loadTests()} for loading tests from test_mappings.zip.
     */
    @Test
    public void testLoadTests_testMappingsZip() throws Exception {
        File tempDir = null;
        try {
            OptionSetter setter = new OptionSetter(mRunner);
            setter.setOptionValue("test-mapping-test-group", "postsubmit");

            tempDir = FileUtil.createTempDir("test_mapping");

            File srcDir = FileUtil.createTempDir("src", tempDir);
            String srcFile = File.separator + TEST_DATA_DIR + File.separator + "test_mapping_1";
            InputStream resourceStream = this.getClass().getResourceAsStream(srcFile);
            FileUtil.saveResourceFile(resourceStream, srcDir, TEST_MAPPING);
            File subDir = FileUtil.createTempDir("sub_dir", srcDir);
            srcFile = File.separator + TEST_DATA_DIR + File.separator + "test_mapping_2";
            resourceStream = this.getClass().getResourceAsStream(srcFile);
            FileUtil.saveResourceFile(resourceStream, subDir, TEST_MAPPING);

            File zipFile = Paths.get(tempDir.getAbsolutePath(), TEST_MAPPINGS_ZIP).toFile();
            ZipUtil.createZip(srcDir, zipFile);

            IDeviceBuildInfo mockBuildInfo = EasyMock.createMock(IDeviceBuildInfo.class);
            EasyMock.expect(mockBuildInfo.getTestsDir()).andReturn(new File("non-existing-dir"));
            EasyMock.expect(mockBuildInfo.getFile(TEST_MAPPINGS_ZIP)).andReturn(zipFile);

            mRunner.setBuild(mockBuildInfo);
            EasyMock.replay(mockBuildInfo);

            LinkedHashMap<String, IConfiguration> configMap = mRunner.loadTests();

            // Test configs in test_mapping_1 doesn't exist, but should be listed in
            // include-filters.
            assertTrue(mRunner.getIncludeFilter().contains("test1"));
            assertTrue(mRunner.getIncludeFilter().contains("test2"));
            assertTrue(mRunner.getIncludeFilter().contains("instrument"));
            assertTrue(mRunner.getIncludeFilter().contains("suite/stub1"));
            assertTrue(mRunner.getIncludeFilter().contains("suite/stub2"));

            // Check module-arg work as expected.
            InstrumentationTest test =
                    (InstrumentationTest) configMap.get("arm64-v8a instrument").getTests().get(0);
            assertEquals("some-name", test.getRunName());

            assertEquals(6, configMap.size());
            assertTrue(configMap.containsKey(ABI_1 + " instrument"));
            assertTrue(configMap.containsKey(ABI_1 + " suite/stub1"));
            assertTrue(configMap.containsKey(ABI_1 + " suite/stub2"));
            assertTrue(configMap.containsKey(ABI_2 + " instrument"));
            assertTrue(configMap.containsKey(ABI_2 + " suite/stub1"));
            assertTrue(configMap.containsKey(ABI_2 + " suite/stub2"));

            EasyMock.verify(mockBuildInfo);
        } finally {
            FileUtil.recursiveDelete(tempDir);
        }
    }

    /** Test for {@link TestMappingSuiteRunner#loadTests()} to fail when no test is found. */
    @Test(expected = RuntimeException.class)
    public void testLoadTests_noTest() throws Exception {
        File tempDir = null;
        try {
            OptionSetter setter = new OptionSetter(mRunner);
            setter.setOptionValue("test-mapping-test-group", "none-exist");

            tempDir = FileUtil.createTempDir("test_mapping");

            File srcDir = FileUtil.createTempDir("src", tempDir);

            File zipFile = Paths.get(tempDir.getAbsolutePath(), TEST_MAPPINGS_ZIP).toFile();
            ZipUtil.createZip(srcDir, zipFile);

            IDeviceBuildInfo mockBuildInfo = EasyMock.createMock(IDeviceBuildInfo.class);
            EasyMock.expect(mockBuildInfo.getTestsDir()).andReturn(new File("non-existing-dir"));
            EasyMock.expect(mockBuildInfo.getFile(TEST_MAPPINGS_ZIP)).andReturn(zipFile);

            mRunner.setBuild(mockBuildInfo);
            EasyMock.replay(mockBuildInfo);

            mRunner.loadTests();
        } finally {
            FileUtil.recursiveDelete(tempDir);
        }
    }

    /**
     * Test for {@link TestMappingSuiteRunner#loadTests()} that when a test config supports
     * IAbiReceiver, multiple instances of the config are queued up.
     */
    @Test
    public void testLoadTestsForMultiAbi() throws Exception {
        OptionSetter setter = new OptionSetter(mRunner);
        setter.setOptionValue("include-filter", "suite/stubAbi");

        ITestDevice mockDevice = EasyMock.createMock(ITestDevice.class);
        mRunner.setDevice(mockDevice);
        EasyMock.replay(mockDevice);

        LinkedHashMap<String, IConfiguration> configMap = mRunner.loadTests();

        assertEquals(2, configMap.size());
        assertTrue(configMap.containsKey(ABI_1 + " suite/stubAbi"));
        assertTrue(configMap.containsKey(ABI_2 + " suite/stubAbi"));
        EasyMock.verify(mockDevice);
    }
}
