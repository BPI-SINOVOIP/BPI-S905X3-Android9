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
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import com.android.ddmlib.IDevice;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.Configuration;
import com.android.tradefed.config.ConfigurationDef;
import com.android.tradefed.config.ConfigurationDescriptor;
import com.android.tradefed.config.ConfigurationException;
import com.android.tradefed.config.ConfigurationFactory;
import com.android.tradefed.config.IConfiguration;
import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.DeviceUnresponsiveException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.device.metric.BaseDeviceMetricCollector;
import com.android.tradefed.device.metric.DeviceMetricData;
import com.android.tradefed.device.metric.IMetricCollector;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.invoker.InvocationContext;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.metrics.proto.MetricMeasurement.Measurements;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ByteArrayInputStreamSource;
import com.android.tradefed.result.ILogSaver;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.suite.checker.ISystemStatusChecker;
import com.android.tradefed.suite.checker.KeyguardStatusChecker;
import com.android.tradefed.suite.checker.StatusCheckerResult;
import com.android.tradefed.suite.checker.StatusCheckerResult.CheckStatus;
import com.android.tradefed.targetprep.StubTargetPreparer;
import com.android.tradefed.testtype.FakeTest;
import com.android.tradefed.testtype.IAbi;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.testtype.StubTest;
import com.android.tradefed.util.AbiUtils;
import com.android.tradefed.util.MultiMap;

import org.easymock.Capture;
import org.easymock.EasyMock;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Set;

/** Unit tests for {@link ITestSuite}. */
@RunWith(JUnit4.class)
public class ITestSuiteTest {

    private static final MultiMap<String, String> METADATA_INCLUDES = new MultiMap<>();
    private static final MultiMap<String, String> METADATA_EXCLUDES = new MultiMap<>();
    private static final String EMPTY_CONFIG = "empty";
    private static final String TEST_CONFIG_NAME = "test";
    private static final String FAKE_HOST_ARCH = "arm";

    private TestSuiteImpl mTestSuite;
    private ITestInvocationListener mMockListener;
    private ITestDevice mMockDevice;
    private IBuildInfo mMockBuildInfo;
    private ISystemStatusChecker mMockSysChecker;
    private IInvocationContext mContext;
    private List<IMetricCollector> mListCollectors;
    private IConfiguration mStubMainConfiguration;
    private ILogSaver mMockLogSaver;

    /**
     * Very basic implementation of {@link ITestSuite} to test it.
     */
    static class TestSuiteImpl extends ITestSuite {
        private int mNumTests = 1;

        public TestSuiteImpl() {}

        public TestSuiteImpl(int numTests) {
            mNumTests = numTests;
        }

        @Override
        public LinkedHashMap<String, IConfiguration> loadTests() {
            LinkedHashMap<String, IConfiguration> testConfig = new LinkedHashMap<>();
            try {
                IConfiguration config =
                        ConfigurationFactory.getInstance()
                                .createConfigurationFromArgs(new String[] {EMPTY_CONFIG});
                config.setTest(new StubCollectingTest());
                testConfig.put(TEST_CONFIG_NAME, config);

                for (int i = 1; i < mNumTests; i++) {
                    IConfiguration extraConfig =
                            ConfigurationFactory.getInstance()
                                    .createConfigurationFromArgs(new String[] {EMPTY_CONFIG});
                    extraConfig.setTest(new StubTest());
                    testConfig.put(TEST_CONFIG_NAME + i, extraConfig);
                }
            } catch (ConfigurationException e) {
                CLog.e(e);
                throw new RuntimeException(e);
            }
            return testConfig;
        }

        @Override
        protected Set<String> getAbisForBuildTargetArch() {
            return AbiUtils.getAbisForArch(FAKE_HOST_ARCH);
        }
    }

    public static class StubCollectingTest implements IRemoteTest {
        private DeviceNotAvailableException mException;
        private RuntimeException mRunException;

        public StubCollectingTest() {}

        public StubCollectingTest(DeviceNotAvailableException e) {
            mException = e;
        }

        public StubCollectingTest(RuntimeException e) {
            mRunException = e;
        }

        @Override
        public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
            listener.testRunStarted(TEST_CONFIG_NAME, 1);
            try {
            if (mException != null) {
                throw mException;
            }
                if (mRunException != null) {
                    throw mRunException;
                }
                TestDescription test = new TestDescription(EMPTY_CONFIG, EMPTY_CONFIG);
                listener.testStarted(test, 0);
                listener.testEnded(test, 5, new HashMap<String, Metric>());
            } finally {
                listener.testRunEnded(0, new HashMap<String, Metric>());
            }
        }
    }

    @Before
    public void setUp() {
        mTestSuite = new TestSuiteImpl();
        mMockListener = EasyMock.createMock(ITestInvocationListener.class);
        mMockDevice = EasyMock.createMock(ITestDevice.class);
        EasyMock.expect(mMockDevice.getSerialNumber()).andStubReturn("SERIAL");
        mMockBuildInfo = EasyMock.createMock(IBuildInfo.class);
        mMockSysChecker = EasyMock.createMock(ISystemStatusChecker.class);
        mMockLogSaver = EasyMock.createMock(ILogSaver.class);
        mStubMainConfiguration = new Configuration("stub", "stub");
        mStubMainConfiguration.setLogSaver(mMockLogSaver);

        mTestSuite.setDevice(mMockDevice);
        mTestSuite.setBuild(mMockBuildInfo);
        mTestSuite.setConfiguration(mStubMainConfiguration);
        mContext = new InvocationContext();
        mTestSuite.setInvocationContext(mContext);
        mListCollectors = new ArrayList<>();
        mListCollectors.add(
                new BaseDeviceMetricCollector() {
                    @Override
                    public void onTestRunStart(DeviceMetricData runData) {
                        runData.addMetric(
                                "metric1",
                                Metric.newBuilder()
                                        .setMeasurements(
                                                Measurements.newBuilder()
                                                        .setSingleString("value1")));
                    }
                });
        mListCollectors.add(
                new BaseDeviceMetricCollector() {
                    @Override
                    public void onTestRunStart(DeviceMetricData runData) {
                        runData.addMetric(
                                "metric2",
                                Metric.newBuilder()
                                        .setMeasurements(
                                                Measurements.newBuilder()
                                                        .setSingleString("value2")));
                    }
                });
    }

    /**
     * Helper for replaying mocks.
     */
    private void replayMocks() {
        EasyMock.replay(mMockListener, mMockDevice, mMockBuildInfo, mMockSysChecker);
    }

    /**
     * Helper for verifying mocks.
     */
    private void verifyMocks() {
        EasyMock.verify(mMockListener, mMockDevice, mMockBuildInfo, mMockSysChecker);
    }

    /** Helper to expect the test run callback. */
    private void expectTestRun(ITestInvocationListener listener) {
        listener.testModuleStarted(EasyMock.anyObject());
        listener.testRunStarted(TEST_CONFIG_NAME, 1);
        TestDescription test = new TestDescription(EMPTY_CONFIG, EMPTY_CONFIG);
        listener.testStarted(test, 0);
        listener.testEnded(test, 5, new HashMap<String, Metric>());
        listener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        listener.testModuleEnded();
    }

    /** Test for {@link ITestSuite#run(ITestInvocationListener)}. */
    @Test
    public void testRun() throws Exception {
        OptionSetter setter = new OptionSetter(mTestSuite);
        setter.setOptionValue("reboot-before-test", "true");
        mContext.addAllocatedDevice(ConfigurationDef.DEFAULT_DEVICE_NAME, mMockDevice);
        // Since we set the option, expect a reboot to occur.
        EasyMock.expect(mMockDevice.getIDevice()).andReturn(EasyMock.createMock(IDevice.class));
        mMockDevice.reboot();

        List<ISystemStatusChecker> sysChecker = new ArrayList<ISystemStatusChecker>();
        sysChecker.add(mMockSysChecker);
        mTestSuite.setSystemStatusChecker(sysChecker);
        EasyMock.expect(mMockSysChecker.preExecutionCheck(EasyMock.eq(mMockDevice)))
                .andReturn(new StatusCheckerResult(CheckStatus.SUCCESS));
        EasyMock.expect(mMockSysChecker.postExecutionCheck(EasyMock.eq(mMockDevice)))
                .andReturn(new StatusCheckerResult(CheckStatus.SUCCESS));
        expectTestRun(mMockListener);
        replayMocks();
        mTestSuite.run(mMockListener);
        verifyMocks();
    }

    /**
     * Test for {@link ITestSuite#run(ITestInvocationListener)} when the System status checker is
     * failing.
     */
    @Test
    public void testRun_failedSystemChecker() throws Exception {
        final byte[] fakeData = "fakeData".getBytes();
        InputStreamSource fakeSource = new ByteArrayInputStreamSource(fakeData);
        List<ISystemStatusChecker> sysChecker = new ArrayList<ISystemStatusChecker>();
        sysChecker.add(mMockSysChecker);
        mTestSuite.setSystemStatusChecker(sysChecker);
        StatusCheckerResult result = new StatusCheckerResult(CheckStatus.FAILED);
        result.setErrorMessage("some failures.");
        EasyMock.expect(mMockSysChecker.preExecutionCheck(EasyMock.eq(mMockDevice)))
                .andReturn(result);
        EasyMock.expect(mMockDevice.getBugreport()).andReturn(fakeSource).times(2);
        mMockListener.testLog((String)EasyMock.anyObject(), EasyMock.eq(LogDataType.BUGREPORT),
                EasyMock.eq(fakeSource));
        EasyMock.expectLastCall().times(2);
        EasyMock.expect(mMockSysChecker.postExecutionCheck(EasyMock.eq(mMockDevice)))
                .andReturn(result);
        expectTestRun(mMockListener);
        replayMocks();
        mTestSuite.run(mMockListener);
        verifyMocks();
    }

    /**
     * Test for {@link ITestSuite#run(ITestInvocationListener)} when the System status checker is
     * passing pre-check but failing post-check and we enable reporting a failure for it.
     */
    @Test
    public void testRun_failedSystemChecker_reportFailure() throws Exception {
        OptionSetter setter = new OptionSetter(mTestSuite);
        setter.setOptionValue("report-system-checkers", "true");
        final byte[] fakeData = "fakeData".getBytes();
        InputStreamSource fakeSource = new ByteArrayInputStreamSource(fakeData);
        List<ISystemStatusChecker> sysChecker = new ArrayList<ISystemStatusChecker>();
        sysChecker.add(mMockSysChecker);
        mTestSuite.setSystemStatusChecker(sysChecker);
        EasyMock.expect(mMockSysChecker.preExecutionCheck(EasyMock.eq(mMockDevice)))
                .andReturn(new StatusCheckerResult(CheckStatus.SUCCESS));
        EasyMock.expect(mMockDevice.getBugreport()).andReturn(fakeSource).times(1);
        mMockListener.testLog(
                (String) EasyMock.anyObject(),
                EasyMock.eq(LogDataType.BUGREPORT),
                EasyMock.eq(fakeSource));
        EasyMock.expectLastCall().times(1);
        StatusCheckerResult result = new StatusCheckerResult(CheckStatus.FAILED);
        result.setErrorMessage("some failures.");
        EasyMock.expect(mMockSysChecker.postExecutionCheck(EasyMock.eq(mMockDevice)))
                .andReturn(result);
        expectTestRun(mMockListener);

        mMockListener.testRunStarted(ITestSuite.MODULE_CHECKER_PRE + "_test", 0);
        mMockListener.testRunEnded(
                EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());

        mMockListener.testRunStarted(ITestSuite.MODULE_CHECKER_POST + "_test", 0);
        mMockListener.testRunFailed(EasyMock.contains("some failures."));
        mMockListener.testRunEnded(
                EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());

        replayMocks();
        mTestSuite.run(mMockListener);
        verifyMocks();
    }

    /**
     * Test for {@link ITestSuite#run(ITestInvocationListener)} when the System status checker is
     * disabled and we request reboot before module run.
     */
    @Test
    public void testRun_rebootBeforeModule() throws Exception {
        List<ISystemStatusChecker> sysChecker = new ArrayList<ISystemStatusChecker>();
        sysChecker.add(mMockSysChecker);
        mTestSuite.setSystemStatusChecker(sysChecker);
        OptionSetter setter = new OptionSetter(mTestSuite);
        setter.setOptionValue("skip-all-system-status-check", "true");
        setter.setOptionValue("reboot-per-module", "true");
        EasyMock.expect(mMockDevice.getProperty("ro.build.type")).andReturn("userdebug");
        mMockDevice.reboot();
        expectTestRun(mMockListener);
        replayMocks();
        mTestSuite.run(mMockListener);
        verifyMocks();
    }

    /**
     * Test for {@link ITestSuite#run(ITestInvocationListener)} when the test throw an
     * unresponsive device exception. The run can continue since device has been recovered in this
     * case.
     */
    @Test
    public void testRun_unresponsiveDevice() throws Exception {
        List<ISystemStatusChecker> sysChecker = new ArrayList<ISystemStatusChecker>();
        sysChecker.add(mMockSysChecker);
        mTestSuite =
                new TestSuiteImpl() {
                    @Override
                    public LinkedHashMap<String, IConfiguration> loadTests() {
                        LinkedHashMap<String, IConfiguration> testConfig = new LinkedHashMap<>();
                        try {
                            IConfiguration fake =
                                    ConfigurationFactory.getInstance()
                                            .createConfigurationFromArgs(
                                                    new String[] {EMPTY_CONFIG});
                            fake.setTest(new StubCollectingTest(new DeviceUnresponsiveException()));
                            testConfig.put(TEST_CONFIG_NAME, fake);
                        } catch (ConfigurationException e) {
                            CLog.e(e);
                            throw new RuntimeException(e);
                        }
                        return testConfig;
                    }
                };
        mTestSuite.setDevice(mMockDevice);
        mTestSuite.setBuild(mMockBuildInfo);
        mTestSuite.setInvocationContext(mContext);
        mTestSuite.setSystemStatusChecker(sysChecker);
        mTestSuite.setConfiguration(mStubMainConfiguration);
        OptionSetter setter = new OptionSetter(mTestSuite);
        setter.setOptionValue("skip-all-system-status-check", "true");
        setter.setOptionValue("reboot-per-module", "true");
        EasyMock.expect(mMockDevice.getProperty("ro.build.type")).andReturn("user");
        mMockListener.testModuleStarted(EasyMock.anyObject());
        mMockListener.testRunStarted(TEST_CONFIG_NAME, 1);
        EasyMock.expectLastCall().times(1);
        mMockListener.testRunFailed("Module test only ran 0 out of 1 expected tests.");
        mMockListener.testRunEnded(
                EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.expectLastCall().times(1);
        mMockListener.testModuleEnded();
        replayMocks();
        mTestSuite.run(mMockListener);
        verifyMocks();
    }

    /**
     * Test for {@link ITestSuite#run(ITestInvocationListener)} when the test throw a runtime
     * exception. The run can continue in this case.
     */
    @Test
    public void testRun_runtimeException() throws Exception {
        List<ISystemStatusChecker> sysChecker = new ArrayList<ISystemStatusChecker>();
        sysChecker.add(mMockSysChecker);
        mTestSuite =
                new TestSuiteImpl() {
                    @Override
                    public LinkedHashMap<String, IConfiguration> loadTests() {
                        LinkedHashMap<String, IConfiguration> testConfig = new LinkedHashMap<>();
                        try {
                            IConfiguration fake =
                                    ConfigurationFactory.getInstance()
                                            .createConfigurationFromArgs(
                                                    new String[] {EMPTY_CONFIG});
                            fake.setTest(new StubCollectingTest(new RuntimeException()));
                            testConfig.put(TEST_CONFIG_NAME, fake);
                        } catch (ConfigurationException e) {
                            CLog.e(e);
                            throw new RuntimeException(e);
                        }
                        return testConfig;
                    }
                };
        mTestSuite.setSystemStatusChecker(sysChecker);
        mTestSuite.setDevice(mMockDevice);
        mTestSuite.setBuild(mMockBuildInfo);
        mTestSuite.setInvocationContext(mContext);
        mTestSuite.setConfiguration(mStubMainConfiguration);
        OptionSetter setter = new OptionSetter(mTestSuite);
        setter.setOptionValue("skip-all-system-status-check", "true");
        setter.setOptionValue("reboot-per-module", "true");
        EasyMock.expect(mMockDevice.getProperty("ro.build.type")).andReturn("user");
        mMockListener.testModuleStarted(EasyMock.anyObject());
        mMockListener.testRunStarted(TEST_CONFIG_NAME, 1);
        EasyMock.expectLastCall().times(1);
        mMockListener.testRunFailed("Module test only ran 0 out of 1 expected tests.");
        mMockListener.testRunEnded(
                EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.expectLastCall().times(1);
        mMockListener.testModuleEnded();
        replayMocks();
        mTestSuite.run(mMockListener);
        verifyMocks();
    }

    /**
     * Test for {@link ITestSuite#split(int)} for modules that are not shardable. We end up with a
     * list of all tests. Note that the shardCountHint of 3 in this case does not drive the final
     * number of tests.
     */
    @Test
    public void testShardModules_notShardable() {
        mTestSuite = new TestSuiteImpl(5);
        Collection<IRemoteTest> tests = mTestSuite.split(3);
        assertEquals(5, tests.size());
        for (IRemoteTest test : tests) {
            assertTrue(test instanceof TestSuiteImpl);
        }
    }

    /** Test that when splitting a single non-splitable test we end up with only one IRemoteTest. */
    @Test
    public void testGetTestShard_onlyOneTest() {
        Collection<IRemoteTest> tests = mTestSuite.split(2);
        assertEquals(1, tests.size());
        for (IRemoteTest test : tests) {
            assertTrue(test instanceof TestSuiteImpl);
        }
    }

    /** Test that after being sharded, ITestSuite shows the module runtime that it holds. */
    @Test
    public void testGetRuntimeHint() {
        // default runtime hint is 0, it is only meant to be used for sharding.
        assertEquals(0l, mTestSuite.getRuntimeHint());
        mTestSuite = new TestSuiteImpl(5);
        Collection<IRemoteTest> tests = mTestSuite.split(3);
        for (IRemoteTest test : tests) {
            assertTrue(test instanceof TestSuiteImpl);
            // once sharded modules from the shard start reporting their runtime.
            assertEquals(60000l, ((TestSuiteImpl) test).getRuntimeHint());
        }
    }

    /**
     * Test that {@link ITestSuite#getAbis(ITestDevice)} is returning a proper intersection of CTS
     * supported architectures and Device supported architectures.
     */
    @Test
    public void testGetAbis() throws DeviceNotAvailableException {
        EasyMock.expect(mMockDevice.getProperty(EasyMock.eq("ro.product.cpu.abilist")))
                .andReturn("arm64-v8a,armeabi-v7a,armeabi");
        Set<String> expectedAbis = new HashSet<>();
        expectedAbis.add("arm64-v8a");
        expectedAbis.add("armeabi-v7a");
        EasyMock.replay(mMockDevice);
        Set<IAbi> res = mTestSuite.getAbis(mMockDevice);
        assertEquals(2, res.size());
        for (IAbi abi : res) {
            assertTrue(expectedAbis.contains(abi.getName()));
        }
        EasyMock.verify(mMockDevice);
    }

    /**
     * Test that {@link ITestSuite#getAbis(ITestDevice)} is throwing an exception when none of the
     * CTS build supported abi match the device abi.
     */
    @Test
    public void testGetAbis_notSupported() throws DeviceNotAvailableException {
        EasyMock.expect(mMockDevice.getProperty(EasyMock.eq("ro.product.cpu.abilist")))
                .andReturn("armeabi");
        EasyMock.replay(mMockDevice);
        try {
            mTestSuite.getAbis(mMockDevice);
            fail("Should have thrown an exception");
        } catch (IllegalArgumentException e) {
            assertEquals(
                    "None of the abi supported by this tests suite build "
                            + "('[armeabi-v7a, arm64-v8a]')"
                            + " are supported by the device ('[armeabi]').",
                    e.getMessage());
        }
        EasyMock.verify(mMockDevice);
    }

    /**
     * Test that {@link ITestSuite#getAbis(ITestDevice)} is returning only the device primary abi.
     */
    @Test
    public void testGetAbis_primaryAbiOnly() throws Exception {
        OptionSetter setter = new OptionSetter(mTestSuite);
        setter.setOptionValue(ITestSuite.PRIMARY_ABI_RUN, "true");
        EasyMock.expect(mMockDevice.getProperty(EasyMock.eq("ro.product.cpu.abi")))
                .andReturn("arm64-v8a");
        Set<String> expectedAbis = new HashSet<>();
        expectedAbis.add("arm64-v8a");
        EasyMock.replay(mMockDevice);
        Set<IAbi> res = mTestSuite.getAbis(mMockDevice);
        assertEquals(1, res.size());
        for (IAbi abi : res) {
            assertTrue(expectedAbis.contains(abi.getName()));
        }
        EasyMock.verify(mMockDevice);
    }

    /**
     * Test that {@link ITestSuite#getAbis(ITestDevice)} is throwing an exception if the primary abi
     * is not supported.
     */
    @Test
    public void testGetAbis_primaryAbiOnly_NotSupported() throws Exception {
        OptionSetter setter = new OptionSetter(mTestSuite);
        setter.setOptionValue(ITestSuite.PRIMARY_ABI_RUN, "true");
        EasyMock.expect(mMockDevice.getProperty(EasyMock.eq("ro.product.cpu.abi")))
                .andReturn("armeabi");
        EasyMock.replay(mMockDevice);
        try {
            mTestSuite.getAbis(mMockDevice);
            fail("Should have thrown an exception");
        } catch (IllegalArgumentException e) {
            assertEquals(
                    "Your tests suite hasn't been built with abi 'armeabi' support, "
                            + "this suite currently supports '[armeabi-v7a, arm64-v8a]'.",
                    e.getMessage());
        }
        EasyMock.verify(mMockDevice);
    }

    /**
     * Test that {@link ITestSuite#getAbis(ITestDevice)} is returning the list of abi supported by
     * Compatibility and the device, and not the particular CTS build.
     */
    @Test
    public void testGetAbis_skipCtsArchCheck() throws Exception {
        OptionSetter setter = new OptionSetter(mTestSuite);
        setter.setOptionValue(ITestSuite.SKIP_HOST_ARCH_CHECK, "true");
        EasyMock.expect(mMockDevice.getProperty(EasyMock.eq("ro.product.cpu.abilist")))
                .andReturn("x86_64,x86,armeabi");
        Set<String> expectedAbis = new HashSet<>();
        expectedAbis.add("x86_64");
        expectedAbis.add("x86");
        EasyMock.replay(mMockDevice);
        Set<IAbi> res = mTestSuite.getAbis(mMockDevice);
        assertEquals(2, res.size());
        for (IAbi abi : res) {
            assertTrue(expectedAbis.contains(abi.getName()));
        }
        EasyMock.verify(mMockDevice);
    }

    /**
     * Test {@link ITestSuite#getAbis(ITestDevice)} when we skip the Cts side architecture check and
     * want to run x86 abi.
     */
    @Test
    public void testGetAbis_skipCtsArchCheck_abiSpecified() throws Exception {
        OptionSetter setter = new OptionSetter(mTestSuite);
        setter.setOptionValue(ITestSuite.SKIP_HOST_ARCH_CHECK, "true");
        setter.setOptionValue(ITestSuite.ABI_OPTION, "x86");
        Set<String> expectedAbis = new HashSet<>();
        expectedAbis.add("x86");
        EasyMock.replay(mMockDevice);
        Set<IAbi> res = mTestSuite.getAbis(mMockDevice);
        assertEquals(1, res.size());
        for (IAbi abi : res) {
            assertTrue(expectedAbis.contains(abi.getName()));
        }
        EasyMock.verify(mMockDevice);
    }

    /** When there are no metadata based filters specified, config should be included. */
    @Test
    public void testMetadataFilter_emptyFilters() throws Exception {
        IConfiguration config = new Configuration("foo", "bar");
        assertTrue(
                "config not included when metadata filters are empty",
                mTestSuite.filterByConfigMetadata(config, METADATA_INCLUDES, METADATA_EXCLUDES));
    }

    /** When inclusion filter is specified, config matching the filter is included. */
    @Test
    public void testMetadataFilter_matchInclude() throws Exception {
        IConfiguration config = new Configuration("foo", "bar");
        ConfigurationDescriptor desc = config.getConfigurationDescription();
        MultiMap<String, String> metadata = new MultiMap<>();
        metadata.put("component", "foo");
        desc.setMetaData(metadata);
        MultiMap<String, String> includeFilter = new MultiMap<>();
        includeFilter.put("component", "foo");
        assertTrue(
                "config not included with matching inclusion filter",
                mTestSuite.filterByConfigMetadata(config, includeFilter, METADATA_EXCLUDES));
    }

    /** When inclusion filter is specified, config not matching the filter is excluded */
    @Test
    public void testMetadataFilter_noMatchInclude_mismatchValue() throws Exception {
        IConfiguration config = new Configuration("foo", "bar");
        ConfigurationDescriptor desc = config.getConfigurationDescription();
        MultiMap<String, String> metadata = new MultiMap<>();
        metadata.put("component", "foo");
        desc.setMetaData(metadata);
        MultiMap<String, String> includeFilter = new MultiMap<>();
        includeFilter.put("component", "bar");
        assertFalse(
                "config not excluded with mismatching inclusion filter",
                mTestSuite.filterByConfigMetadata(config, includeFilter, METADATA_EXCLUDES));
    }

    /** When inclusion filter is specified, config not matching the filter is excluded. */
    @Test
    public void testMetadataFilter_noMatchInclude_mismatchKey() throws Exception {
        IConfiguration config = new Configuration("foo", "bar");
        ConfigurationDescriptor desc = config.getConfigurationDescription();
        MultiMap<String, String> metadata = new MultiMap<>();
        metadata.put("component", "foo");
        desc.setMetaData(metadata);
        MultiMap<String, String> includeFilter = new MultiMap<>();
        includeFilter.put("group", "bar");
        assertFalse(
                "config not excluded with mismatching inclusion filter",
                mTestSuite.filterByConfigMetadata(config, includeFilter, METADATA_EXCLUDES));
    }

    /** When exclusion filter is specified, config matching the filter is excluded. */
    @Test
    public void testMetadataFilter_matchExclude() throws Exception {
        IConfiguration config = new Configuration("foo", "bar");
        ConfigurationDescriptor desc = config.getConfigurationDescription();
        MultiMap<String, String> metadata = new MultiMap<>();
        metadata.put("component", "foo");
        desc.setMetaData(metadata);
        MultiMap<String, String> excludeFilter = new MultiMap<>();
        excludeFilter.put("component", "foo");
        assertFalse(
                "config not excluded with matching exclusion filter",
                mTestSuite.filterByConfigMetadata(config, METADATA_INCLUDES, excludeFilter));
    }

    /** When exclusion filter is specified, config not matching the filter is included. */
    @Test
    public void testMetadataFilter_noMatchExclude_mismatchKey() throws Exception {
        IConfiguration config = new Configuration("foo", "bar");
        ConfigurationDescriptor desc = config.getConfigurationDescription();
        MultiMap<String, String> metadata = new MultiMap<>();
        metadata.put("component", "foo");
        desc.setMetaData(metadata);
        MultiMap<String, String> excludeFilter = new MultiMap<>();
        excludeFilter.put("component", "bar");
        assertTrue(
                "config not included with mismatching exclusion filter",
                mTestSuite.filterByConfigMetadata(config, METADATA_INCLUDES, excludeFilter));
    }

    /** When exclusion filter is specified, config not matching the filter is included. */
    @Test
    public void testMetadataFilter_noMatchExclude_mismatchValue() throws Exception {
        IConfiguration config = new Configuration("foo", "bar");
        ConfigurationDescriptor desc = config.getConfigurationDescription();
        MultiMap<String, String> metadata = new MultiMap<>();
        metadata.put("component", "foo");
        desc.setMetaData(metadata);
        MultiMap<String, String> excludeFilter = new MultiMap<>();
        excludeFilter.put("group", "bar");
        assertTrue(
                "config not included with mismatching exclusion filter",
                mTestSuite.filterByConfigMetadata(config, METADATA_INCLUDES, excludeFilter));
    }

    /**
     * When inclusion filter is specified, config with one of the metadata field matching the filter
     * is included.
     */
    @Test
    public void testMetadataFilter_matchInclude_multipleMetadataField() throws Exception {
        IConfiguration config = new Configuration("foo", "bar");
        ConfigurationDescriptor desc = config.getConfigurationDescription();
        MultiMap<String, String> metadata = new MultiMap<>();
        metadata.put("component", "foo");
        metadata.put("component", "bar");
        desc.setMetaData(metadata);
        MultiMap<String, String> includeFilter = new MultiMap<>();
        includeFilter.put("component", "foo");
        assertTrue(
                "config not included with matching inclusion filter",
                mTestSuite.filterByConfigMetadata(config, includeFilter, METADATA_EXCLUDES));
    }

    /**
     * When exclusion filter is specified, config with one of the metadata field matching the filter
     * is excluded.
     */
    @Test
    public void testMetadataFilter_matchExclude_multipleMetadataField() throws Exception {
        IConfiguration config = new Configuration("foo", "bar");
        ConfigurationDescriptor desc = config.getConfigurationDescription();
        MultiMap<String, String> metadata = new MultiMap<>();
        metadata.put("component", "foo");
        metadata.put("component", "bar");
        desc.setMetaData(metadata);
        MultiMap<String, String> excludeFilter = new MultiMap<>();
        excludeFilter.put("component", "foo");
        assertFalse(
                "config not excluded with matching exclusion filter",
                mTestSuite.filterByConfigMetadata(config, METADATA_INCLUDES, excludeFilter));
    }

    /**
     * When inclusion filters are specified, config with metadata field matching one of the filter
     * is included.
     */
    @Test
    public void testMetadataFilter_matchInclude_multipleFilters() throws Exception {
        IConfiguration config = new Configuration("foo", "bar");
        ConfigurationDescriptor desc = config.getConfigurationDescription();
        MultiMap<String, String> metadata = new MultiMap<>();
        metadata.put("component", "foo");
        desc.setMetaData(metadata);
        MultiMap<String, String> includeFilter = new MultiMap<>();
        includeFilter.put("component", "foo");
        includeFilter.put("component", "bar");
        assertTrue(
                "config not included with matching inclusion filter",
                mTestSuite.filterByConfigMetadata(config, includeFilter, METADATA_EXCLUDES));
    }

    /**
     * When exclusion filters are specified, config with metadata field matching one of the filter
     * is excluded.
     */
    @Test
    public void testMetadataFilter_matchExclude_multipleFilters() throws Exception {
        IConfiguration config = new Configuration("foo", "bar");
        ConfigurationDescriptor desc = config.getConfigurationDescription();
        MultiMap<String, String> metadata = new MultiMap<>();
        metadata.put("component", "foo");
        desc.setMetaData(metadata);
        MultiMap<String, String> excludeFilter = new MultiMap<>();
        excludeFilter.put("component", "foo");
        excludeFilter.put("component", "bar");
        assertFalse(
                "config not excluded with matching exclusion filter",
                mTestSuite.filterByConfigMetadata(config, METADATA_INCLUDES, excludeFilter));
    }

    /**
     * When inclusion filters are specified, config with metadata field matching one of the filter
     * is included.
     */
    @Test
    public void testMetadataFilter_matchInclude_multipleMetadataAndFilters() throws Exception {
        IConfiguration config = new Configuration("foo", "bar");
        ConfigurationDescriptor desc = config.getConfigurationDescription();
        MultiMap<String, String> metadata = new MultiMap<>();
        metadata.put("component", "foo1");
        metadata.put("group", "bar1");
        desc.setMetaData(metadata);
        MultiMap<String, String> includeFilter = new MultiMap<>();
        includeFilter.put("component", "foo1");
        includeFilter.put("group", "bar2");
        assertTrue(
                "config not included with matching inclusion filter",
                mTestSuite.filterByConfigMetadata(config, includeFilter, METADATA_EXCLUDES));
    }

    /**
     * When exclusion filters are specified, config with metadata field matching one of the filter
     * is excluded.
     */
    @Test
    public void testMetadataFilter_matchExclude_multipleMetadataAndFilters() throws Exception {
        IConfiguration config = new Configuration("foo", "bar");
        ConfigurationDescriptor desc = config.getConfigurationDescription();
        MultiMap<String, String> metadata = new MultiMap<>();
        metadata.put("component", "foo1");
        metadata.put("group", "bar1");
        desc.setMetaData(metadata);
        MultiMap<String, String> excludeFilter = new MultiMap<>();
        excludeFilter.put("component", "foo1");
        excludeFilter.put("group", "bar2");
        assertFalse(
                "config not excluded with matching exclusion filter",
                mTestSuite.filterByConfigMetadata(config, METADATA_INCLUDES, excludeFilter));
    }

    /**
     * When inclusion and exclusion filters are both specified, config can pass through the filters
     * as expected.
     */
    @Test
    public void testMetadataFilter_includeAndExclude() throws Exception {
        IConfiguration config = new Configuration("foo", "bar");
        ConfigurationDescriptor desc = config.getConfigurationDescription();
        MultiMap<String, String> metadata = new MultiMap<>();
        metadata.put("component", "foo");
        metadata.put("group", "bar1");
        desc.setMetaData(metadata);
        MultiMap<String, String> includeFilter = new MultiMap<>();
        includeFilter.put("component", "foo");
        MultiMap<String, String> excludeFilter = new MultiMap<>();
        excludeFilter.put("group", "bar2");
        assertTrue(
                "config not included with matching inclusion and mismatching exclusion filters",
                mTestSuite.filterByConfigMetadata(config, includeFilter, excludeFilter));
    }

    /** When inclusion and exclusion filters are both specified, config be excluded as specified */
    @Test
    public void testMetadataFilter_includeThenExclude() throws Exception {
        IConfiguration config = new Configuration("foo", "bar");
        ConfigurationDescriptor desc = config.getConfigurationDescription();
        MultiMap<String, String> metadata = new MultiMap<>();
        metadata.put("component", "foo");
        metadata.put("group", "bar");
        desc.setMetaData(metadata);
        MultiMap<String, String> includeFilter = new MultiMap<>();
        includeFilter.put("component", "foo");
        MultiMap<String, String> excludeFilter = new MultiMap<>();
        excludeFilter.put("group", "bar");
        assertFalse(
                "config not excluded with matching inclusion and exclusion filters",
                mTestSuite.filterByConfigMetadata(config, includeFilter, excludeFilter));
    }

    /** Test that running a module with {@link IMetricCollector}s properly reports the metrics. */
    @Test
    public void testRun_withCollectors() throws Exception {
        List<ISystemStatusChecker> sysChecker = new ArrayList<ISystemStatusChecker>();
        sysChecker.add(mMockSysChecker);
        mTestSuite.setSystemStatusChecker(sysChecker);
        mTestSuite.setMetricCollectors(mListCollectors);
        EasyMock.expect(mMockSysChecker.preExecutionCheck(EasyMock.eq(mMockDevice)))
                .andReturn(new StatusCheckerResult(CheckStatus.SUCCESS));
        EasyMock.expect(mMockSysChecker.postExecutionCheck(EasyMock.eq(mMockDevice)))
                .andReturn(new StatusCheckerResult(CheckStatus.SUCCESS));

        Capture<HashMap<String, Metric>> c = new Capture<>();
        mMockListener.testModuleStarted(EasyMock.anyObject());
        mMockListener.testRunStarted(TEST_CONFIG_NAME, 1);
        TestDescription test = new TestDescription(EMPTY_CONFIG, EMPTY_CONFIG);
        mMockListener.testStarted(test, 0);
        mMockListener.testEnded(test, 5, new HashMap<String, Metric>());
        mMockListener.testRunEnded(EasyMock.anyLong(), EasyMock.capture(c));
        mMockListener.testModuleEnded();

        replayMocks();
        mTestSuite.run(mMockListener);
        verifyMocks();
        assertEquals("value1", c.getValue().get("metric1").getMeasurements().getSingleString());
        assertEquals("value2", c.getValue().get("metric2").getMeasurements().getSingleString());
    }

    /**
     * If a non-existing {@link ISystemStatusChecker} is specified to be skipped. Throw from the
     * very beginning as it's not going to work.
     */
    @Test
    public void testStatusChecker_doesNotExist() throws Exception {
        OptionSetter setter = new OptionSetter(mTestSuite);
        setter.setOptionValue(ITestSuite.SKIP_SYSTEM_STATUS_CHECKER, "com.i.dont.exist.Checker");
        try {
            mTestSuite.run(mMockListener);
            fail("Should have thrown an exception.");
        } catch (RuntimeException expected) {
            assertTrue(expected.getCause() instanceof ConfigurationException);
        }
    }

    /**
     * Test for {@link ITestSuite#run(ITestInvocationListener)} when only one of the module checkers
     * is skipped.
     */
    @Test
    public void testRun_SkipOneModuleChecker() throws Exception {
        OptionSetter setter = new OptionSetter(mTestSuite);
        setter.setOptionValue(
                ITestSuite.SKIP_SYSTEM_STATUS_CHECKER, KeyguardStatusChecker.class.getName());
        List<ISystemStatusChecker> sysChecker = new ArrayList<ISystemStatusChecker>();
        sysChecker.add(mMockSysChecker);
        // getKeyguardState is never called because it is skipped.
        sysChecker.add(new KeyguardStatusChecker());
        mTestSuite.setSystemStatusChecker(sysChecker);
        EasyMock.expect(mMockSysChecker.preExecutionCheck(EasyMock.eq(mMockDevice)))
                .andReturn(new StatusCheckerResult(CheckStatus.SUCCESS));
        EasyMock.expect(mMockSysChecker.postExecutionCheck(EasyMock.eq(mMockDevice)))
                .andReturn(new StatusCheckerResult(CheckStatus.SUCCESS));
        expectTestRun(mMockListener);
        replayMocks();
        mTestSuite.run(mMockListener);
        verifyMocks();
    }

    /** If a non-existing runner name is provided for the whitelist, throw an exception. */
    @Test
    public void testWhitelistRunner_notFound() throws Exception {
        OptionSetter setter = new OptionSetter(mTestSuite);
        setter.setOptionValue(ITestSuite.RUNNER_WHITELIST, "com.I.dont.exist.runner");
        try {
            mTestSuite.run(mMockListener);
            fail("Should have thrown an exception.");
        } catch (RuntimeException expected) {
            assertTrue(expected.getCause() instanceof ConfigurationException);
        }
    }

    /**
     * Test that if the preparer is whitelisted it is left untouched by {@link
     * ITestSuite#filterPreparers(IConfiguration, Set)}.
     */
    @Test
    public void testPreparerWhitelist() throws Exception {
        IConfiguration config = new Configuration("name", "description");
        config.setTargetPreparer(new StubTargetPreparer());
        Set<String> allowedPreparers = new HashSet<>();
        allowedPreparers.add(StubTargetPreparer.class.getName());
        assertEquals(1, config.getTargetPreparers().size());
        mTestSuite.filterPreparers(config, allowedPreparers);
        assertEquals(1, config.getTargetPreparers().size());
    }

    /**
     * Test that if the preparer whitelist is empty then nothing is done, we do not filter the
     * preparer.
     */
    @Test
    public void testPreparerWhitelist_empty() throws Exception {
        IConfiguration config = new Configuration("name", "description");
        config.setTargetPreparer(new StubTargetPreparer());
        // Empty whitelist should allow everything to run.
        Set<String> allowedPreparers = new HashSet<>();
        assertEquals(1, config.getTargetPreparers().size());
        mTestSuite.filterPreparers(config, allowedPreparers);
        assertEquals(1, config.getTargetPreparers().size());
    }

    /**
     * Test that if the preparer is not whitelisted it is left filtered out by {@link
     * ITestSuite#filterPreparers(IConfiguration, Set)}.
     */
    @Test
    public void testPreparerWhitelist_filtered() throws Exception {
        IConfiguration config = new Configuration("name", "description");
        config.setTargetPreparer(new StubTargetPreparer());
        Set<String> allowedPreparers = new HashSet<>();
        allowedPreparers.add("some.other.preparer");
        assertEquals(1, config.getTargetPreparers().size());
        mTestSuite.filterPreparers(config, allowedPreparers);
        assertEquals(0, config.getTargetPreparers().size());
    }

    /**
     * Test that if the runner is allowed to run, the config is untouched and can run through {@link
     * ITestSuite#filterByRunnerType(IConfiguration, Set)}.
     */
    @Test
    public void testWhiteListFiltering() throws Exception {
        IConfiguration config = new Configuration("name", "description");
        List<IRemoteTest> tests = new ArrayList<>();
        tests.add(new StubTest());
        config.setTests(tests);
        Set<String> allowedRunners = new HashSet<>();
        allowedRunners.add(StubTest.class.getName());
        assertTrue(mTestSuite.filterByRunnerType(config, allowedRunners));
    }

    /** Test that if no runner whitelist is provided, we skip the runner check. */
    @Test
    public void testWhiteListFiltering_empty() throws Exception {
        IConfiguration config = new Configuration("name", "description");
        List<IRemoteTest> tests = new ArrayList<>();
        tests.add(new StubTest());
        config.setTests(tests);
        Set<String> allowedRunners = new HashSet<>();
        assertTrue(mTestSuite.filterByRunnerType(config, allowedRunners));
    }

    /**
     * Test that if the runner is not allowed to run, the config is rejected if all its runner are
     * not allowed through {@link ITestSuite#filterByRunnerType(IConfiguration, Set)}.
     */
    @Test
    public void testWhiteListFiltering_disallow() throws Exception {
        IConfiguration config = new Configuration("name", "description");
        List<IRemoteTest> tests = new ArrayList<>();
        tests.add(new StubTest());
        config.setTests(tests);
        Set<String> allowedRunners = new HashSet<>();
        // Our runner is not allowed.
        allowedRunners.add(FakeTest.class.getName());
        assertFalse(mTestSuite.filterByRunnerType(config, allowedRunners));
    }

    /**
     * Test that if only one of the runner is allowed to run. The config is modified to only keep
     * the allowed runner and then is allowed to run.
     */
    @Test
    public void testWhiteListFiltering_partialModule() throws Exception {
        IConfiguration config = new Configuration("name", "description");
        List<IRemoteTest> tests = new ArrayList<>();
        tests.add(new StubTest());
        tests.add(new FakeTest());
        config.setTests(tests);
        Set<String> allowedRunners = new HashSet<>();
        // Only one of the runner is allowed
        allowedRunners.add(FakeTest.class.getName());
        // The config is allowed to run but was modified to only include the accepted runner.
        assertTrue(mTestSuite.filterByRunnerType(config, allowedRunners));
        assertEquals(1, config.getTests().size());
        assertTrue(config.getTests().get(0) instanceof FakeTest);
    }

    /** Test for {@link ITestSuite#run(ITestInvocationListener)} when a module listener is used. */
    @Test
    public void testRun_withModuleListener() throws Exception {
        ITestInvocationListener moduleListener = EasyMock.createMock(ITestInvocationListener.class);
        mTestSuite =
                new TestSuiteImpl() {
                    @Override
                    protected List<ITestInvocationListener> createModuleListeners() {
                        List<ITestInvocationListener> list = super.createModuleListeners();
                        list.add(moduleListener);
                        return list;
                    }
                };
        mTestSuite.setDevice(mMockDevice);
        mTestSuite.setBuild(mMockBuildInfo);
        mTestSuite.setConfiguration(mStubMainConfiguration);
        mContext = new InvocationContext();
        mTestSuite.setInvocationContext(mContext);
        mContext.addAllocatedDevice(ConfigurationDef.DEFAULT_DEVICE_NAME, mMockDevice);

        List<ISystemStatusChecker> sysChecker = new ArrayList<ISystemStatusChecker>();
        sysChecker.add(mMockSysChecker);
        mTestSuite.setSystemStatusChecker(sysChecker);
        EasyMock.expect(mMockSysChecker.preExecutionCheck(EasyMock.eq(mMockDevice)))
                .andReturn(new StatusCheckerResult(CheckStatus.SUCCESS));
        EasyMock.expect(mMockSysChecker.postExecutionCheck(EasyMock.eq(mMockDevice)))
                .andReturn(new StatusCheckerResult(CheckStatus.SUCCESS));
        expectTestRun(mMockListener);
        // We expect the full test run on the module listener too
        expectTestRun(moduleListener);
        replayMocks();
        EasyMock.replay(moduleListener);
        mTestSuite.run(mMockListener);
        verifyMocks();
        EasyMock.verify(moduleListener);
    }
}
