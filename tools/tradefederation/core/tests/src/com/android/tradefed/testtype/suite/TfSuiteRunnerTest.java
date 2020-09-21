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
package com.android.tradefed.testtype.suite;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.Mockito.mock;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.build.IDeviceBuildInfo;
import com.android.tradefed.config.Configuration;
import com.android.tradefed.config.ConfigurationException;
import com.android.tradefed.config.IConfiguration;
import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.invoker.InvocationContext;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ILogSaver;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.testtype.Abi;
import com.android.tradefed.testtype.IAbi;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.testtype.StubTest;
import com.android.tradefed.util.AbiUtils;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.ZipUtil;

import org.easymock.EasyMock;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.io.File;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Set;

/**
 * Unit tests for {@link TfSuiteRunner}.
 */
@RunWith(JUnit4.class)
public class TfSuiteRunnerTest {

    private static final String TEST_CONFIG =
            "<configuration description=\"Runs a stub tests part of some suite\">\n"
                    + "    <option name=\"test-suite-tag\" value=\"example-suite\" />\n"
                    + "    <test class=\"com.android.tradefed.testtype.StubTest\" />\n"
                    + "</configuration>";

    private TfSuiteRunner mRunner;
    private IConfiguration mStubMainConfiguration;
    private ILogSaver mMockLogSaver;

    @Before
    public void setUp() {
        mRunner = new TestTfSuiteRunner();
        mMockLogSaver = EasyMock.createMock(ILogSaver.class);
        mStubMainConfiguration = new Configuration("stub", "stub");
        mStubMainConfiguration.setLogSaver(mMockLogSaver);
        mRunner.setConfiguration(mStubMainConfiguration);
    }

    /**
     * Test TfSuiteRunner that hardcodes the abis to avoid failures related to running the tests
     * against a particular abi build of tradefed.
     */
    public static class TestTfSuiteRunner extends TfSuiteRunner {
        @Override
        public Set<IAbi> getAbis(ITestDevice device) throws DeviceNotAvailableException {
            Set<IAbi> abis = new HashSet<>();
            abis.add(new Abi("arm64-v8a", AbiUtils.getBitness("arm64-v8a")));
            abis.add(new Abi("armeabi-v7a", AbiUtils.getBitness("armeabi-v7a")));
            return abis;
        }
    }

    /**
     * Test for {@link TfSuiteRunner#loadTests()} implementation, for basic example configurations.
     */
    @Test
    public void testLoadTests() throws Exception {
        OptionSetter setter = new OptionSetter(mRunner);
        setter.setOptionValue("suite-config-prefix", "suite");
        setter.setOptionValue("run-suite-tag", "example-suite");
        LinkedHashMap <String, IConfiguration> configMap = mRunner.loadTests();
        assertEquals(2, configMap.size());
        assertTrue(configMap.containsKey("suite/stub1"));
        assertTrue(configMap.containsKey("suite/stub2"));
    }

    /**
     * Test for {@link TfSuiteRunner#loadTests()} implementation, only stub1.xml is part of this
     * suite.
     */
    @Test
    public void testLoadTests_suite2() throws Exception {
        OptionSetter setter = new OptionSetter(mRunner);
        setter.setOptionValue("suite-config-prefix", "suite");
        setter.setOptionValue("run-suite-tag", "example-suite2");
        LinkedHashMap <String, IConfiguration> configMap = mRunner.loadTests();
        assertEquals(1, configMap.size());
        assertTrue(configMap.containsKey("suite/stub1"));
    }

    /** Test that when splitting, the instance of the implementation is used. */
    @Test
    public void testSplit() throws Exception {
        OptionSetter setter = new OptionSetter(mRunner);
        setter.setOptionValue("suite-config-prefix", "suite");
        setter.setOptionValue("run-suite-tag", "example-suite");
        Collection<IRemoteTest> tests = mRunner.split(2);
        assertEquals(2, tests.size());
        for (IRemoteTest test : tests) {
            assertTrue(test instanceof TfSuiteRunner);
        }
    }

    /**
     * Test that when {@link TfSuiteRunner} run-suite-tag is not set we cannot shard since there is
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
     * Attempt to load a suite from a suite, but the sub-suite does not have a default run-suite-tag
     * so it cannot run anything.
     */
    @Test
    public void testLoadSuite_noSubConfigs() throws ConfigurationException {
        OptionSetter setter = new OptionSetter(mRunner);
        setter.setOptionValue("suite-config-prefix", "suite");
        setter.setOptionValue("run-suite-tag", "test-empty");
        LinkedHashMap<String, IConfiguration> configMap = mRunner.loadTests();
        assertEquals(0, configMap.size());
    }

    /**
     * Attempt to load a suite from a suite, the sub-suite has a default run-suite-tag that will be
     * loaded.
     */
    @Test
    public void testLoadSuite() throws ConfigurationException {
        OptionSetter setter = new OptionSetter(mRunner);
        setter.setOptionValue("suite-config-prefix", "suite");
        setter.setOptionValue("run-suite-tag", "test-sub-suite");
        LinkedHashMap<String, IConfiguration> configMap = mRunner.loadTests();
        assertEquals(3, configMap.size());
        // 2 test configs loaded from the sub-suite
        assertTrue(configMap.containsKey("suite/stub1"));
        assertTrue(configMap.containsKey("suite/stub2"));
        // 1 config from the left over <test> that was not a suite.
        assertTrue(configMap.containsKey("suite/sub-suite"));
        IConfiguration config = configMap.get("suite/sub-suite");
        // assert that the TfSuiteRunner was removed from the config, only the stubTest remains
        assertTrue(config.getTests().size() == 1);
        assertTrue(config.getTests().get(0) instanceof StubTest);
    }

    /**
     * In case of cycle include of sub-suite configuration. We throw an exception to prevent any
     * weird runs.
     */
    @Test
    public void testLoadSuite_cycle() throws ConfigurationException {
        OptionSetter setter = new OptionSetter(mRunner);
        setter.setOptionValue("suite-config-prefix", "suite");
        setter.setOptionValue("run-suite-tag", "test-cycle-a");
        try {
            mRunner.loadTests();
            fail("Should have thrown an exception.");
        } catch (RuntimeException expected) {
            // expected
        }
    }

    /** Test for {@link TfSuiteRunner#run(ITestInvocationListener)} when loading another suite. */
    @Test
    public void testLoadTests_suite() throws Exception {
        OptionSetter setter = new OptionSetter(mRunner);
        setter.setOptionValue("suite-config-prefix", "suite");
        setter.setOptionValue("run-suite-tag", "example-suite3");
        ITestInvocationListener listener = EasyMock.createMock(ITestInvocationListener.class);
        mRunner.setDevice(mock(ITestDevice.class));
        mRunner.setBuild(mock(IBuildInfo.class));
        mRunner.setSystemStatusChecker(new ArrayList<>());
        mRunner.setInvocationContext(new InvocationContext());
        // runs the expanded suite
        listener.testModuleStarted(EasyMock.anyObject());
        listener.testRunStarted("suite/stub1", 0);
        listener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        listener.testModuleEnded();
        EasyMock.replay(listener);
        mRunner.run(listener);
        EasyMock.verify(listener);
    }

    /**
     * Test for {@link TfSuiteRunner#run(ITestInvocationListener)} when loading test configs from
     * additional-tests-zip.
     */
    @Test
    public void testLoadTests_additionalTestsZip() throws Exception {
        File tmpDir = null;
        File deviceTestDir = null;
        File additionalTestsZipFile = null;
        try {
            tmpDir = FileUtil.createTempDir("test");
            // tests directory for the build.
            deviceTestDir = FileUtil.createTempDir("build-info-test-dir");

            File zipDir = FileUtil.getFileForPath(tmpDir, "suite");
            FileUtil.mkdirsRWX(zipDir);

            // Create 2 test configs inside a zip.
            File testConfig = new File(zipDir, "test1.config");
            FileUtil.writeToFile(TEST_CONFIG, testConfig);
            File testConfig2 = new File(zipDir, "test2.config");
            FileUtil.writeToFile(TEST_CONFIG, testConfig2);
            additionalTestsZipFile = ZipUtil.createZip(zipDir);

            OptionSetter setter = new OptionSetter(mRunner);
            setter.setOptionValue("suite-config-prefix", "suite");
            setter.setOptionValue("run-suite-tag", "example-suite");
            setter.setOptionValue("additional-tests-zip", additionalTestsZipFile.getAbsolutePath());

            IDeviceBuildInfo deviceBuildInfo = EasyMock.createMock(IDeviceBuildInfo.class);
            EasyMock.expect(deviceBuildInfo.getTestsDir()).andReturn(deviceTestDir);
            mRunner.setBuild(deviceBuildInfo);

            EasyMock.replay(deviceBuildInfo);
            LinkedHashMap<String, IConfiguration> configMap = mRunner.loadTests();
            assertEquals(4, configMap.size());
            // The keySet should be stable and always ensure the same order of files.
            List<String> keyList = new ArrayList<>(configMap.keySet());
            // test1 and test2 name was sanitized to look like the included configs.
            assertEquals("suite/test1", keyList.get(0));
            assertEquals("suite/test2", keyList.get(1));
            assertEquals("suite/stub1", keyList.get(2));
            assertEquals("suite/stub2", keyList.get(3));
            EasyMock.verify(deviceBuildInfo);
        } finally {
            FileUtil.recursiveDelete(deviceTestDir);
            FileUtil.recursiveDelete(tmpDir);
            FileUtil.recursiveDelete(additionalTestsZipFile);
        }
    }

    /**
     * Test for {@link TfSuiteRunner#loadTests()} that when a test config supports IAbiReceiver,
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
