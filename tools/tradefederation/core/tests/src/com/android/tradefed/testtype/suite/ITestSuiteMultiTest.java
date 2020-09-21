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
package com.android.tradefed.testtype.suite;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.Configuration;
import com.android.tradefed.config.ConfigurationException;
import com.android.tradefed.config.ConfigurationFactory;
import com.android.tradefed.config.DeviceConfigurationHolder;
import com.android.tradefed.config.IConfiguration;
import com.android.tradefed.config.IDeviceConfiguration;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.invoker.InvocationContext;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ILogSaver;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.targetprep.ITargetPreparer;

import org.easymock.EasyMock;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.List;

/** Unit tests for {@link ITestSuite} when used with multiple devices. */
@RunWith(JUnit4.class)
public class ITestSuiteMultiTest {

    private static final String EMPTY_CONFIG = "empty";
    private static final String TEST_CONFIG_NAME = "test";
    private static final String DEVICE_NAME_1 = "device1";
    private static final String DEVICE_NAME_2 = "device2";

    private ITestSuite mTestSuite;
    private ITestInvocationListener mMockListener;
    private IInvocationContext mContext;
    private ITestDevice mMockDevice1;
    private IBuildInfo mMockBuildInfo1;
    private ITestDevice mMockDevice2;
    private IBuildInfo mMockBuildInfo2;

    private ITargetPreparer mMockTargetPrep;
    private IConfiguration mStubMainConfiguration;
    private ILogSaver mMockLogSaver;

    static class TestSuiteMultiDeviceImpl extends ITestSuite {
        private int mNumTests = 1;
        private ITargetPreparer mPreparer;

        public TestSuiteMultiDeviceImpl() {}

        public TestSuiteMultiDeviceImpl(int numTests, ITargetPreparer targetPrep) {
            mNumTests = numTests;
            mPreparer = targetPrep;
        }

        @Override
        public LinkedHashMap<String, IConfiguration> loadTests() {
            LinkedHashMap<String, IConfiguration> testConfig = new LinkedHashMap<>();
            try {
                for (int i = 1; i < mNumTests; i++) {
                    IConfiguration extraConfig =
                            ConfigurationFactory.getInstance()
                                    .createConfigurationFromArgs(new String[] {EMPTY_CONFIG});
                    List<IDeviceConfiguration> deviceConfigs = new ArrayList<>();
                    deviceConfigs.add(new DeviceConfigurationHolder(DEVICE_NAME_1));

                    IDeviceConfiguration holder2 = new DeviceConfigurationHolder(DEVICE_NAME_2);
                    deviceConfigs.add(holder2);
                    holder2.addSpecificConfig(mPreparer);

                    extraConfig.setDeviceConfigList(deviceConfigs);

                    MultiDeviceStubTest test = new MultiDeviceStubTest();
                    test.setExceptedDevice(2);
                    extraConfig.setTest(test);
                    testConfig.put(TEST_CONFIG_NAME + i, extraConfig);
                }
            } catch (ConfigurationException e) {
                CLog.e(e);
                throw new RuntimeException(e);
            }
            return testConfig;
        }
    }

    @Before
    public void setUp() {
        mMockTargetPrep = EasyMock.createMock(ITargetPreparer.class);

        mTestSuite = new TestSuiteMultiDeviceImpl(2, mMockTargetPrep);

        mMockListener = EasyMock.createMock(ITestInvocationListener.class);
        // 2 devices and 2 builds
        mMockDevice1 = EasyMock.createMock(ITestDevice.class);
        EasyMock.expect(mMockDevice1.getSerialNumber()).andStubReturn("SERIAL1");
        mMockBuildInfo1 = EasyMock.createMock(IBuildInfo.class);
        mMockDevice2 = EasyMock.createMock(ITestDevice.class);
        EasyMock.expect(mMockDevice2.getSerialNumber()).andStubReturn("SERIAL2");
        mMockBuildInfo2 = EasyMock.createMock(IBuildInfo.class);
        mMockLogSaver = EasyMock.createMock(ILogSaver.class);
        mStubMainConfiguration = new Configuration("stub", "stub");
        mStubMainConfiguration.setLogSaver(mMockLogSaver);

        mTestSuite.setConfiguration(mStubMainConfiguration);
    }

    /**
     * Test that a multi-devices test will execute through without hitting issues since all
     * structures are properly injected.
     */
    @Test
    public void testMultiDeviceITestSuite() throws Exception {
        mTestSuite.setDevice(mMockDevice1);
        mTestSuite.setBuild(mMockBuildInfo1);

        mContext = new InvocationContext();
        mContext.addAllocatedDevice(DEVICE_NAME_1, mMockDevice1);
        mContext.addDeviceBuildInfo(DEVICE_NAME_1, mMockBuildInfo1);
        mContext.addAllocatedDevice(DEVICE_NAME_2, mMockDevice2);
        mContext.addDeviceBuildInfo(DEVICE_NAME_2, mMockBuildInfo2);
        mTestSuite.setInvocationContext(mContext);
        mTestSuite.setDeviceInfos(mContext.getDeviceBuildMap());

        mTestSuite.setSystemStatusChecker(new ArrayList<>());
        mMockListener.testModuleStarted(EasyMock.anyObject());
        mMockListener.testRunStarted("test1", 2);
        TestDescription test1 =
                new TestDescription(MultiDeviceStubTest.class.getSimpleName(), "test0");
        mMockListener.testStarted(test1, 0l);
        mMockListener.testEnded(test1, 5l, new HashMap<String, Metric>());
        TestDescription test2 =
                new TestDescription(MultiDeviceStubTest.class.getSimpleName(), "test1");
        mMockListener.testStarted(test2, 0l);
        mMockListener.testEnded(test2, 5l, new HashMap<String, Metric>());
        mMockListener.testRunEnded(
                EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        mMockListener.testModuleEnded();

        // Target preparation is triggered against the preparer in the second device.
        EasyMock.expect(mMockTargetPrep.isDisabled()).andReturn(false);
        mMockTargetPrep.setUp(mMockDevice2, mMockBuildInfo2);

        EasyMock.replay(
                mMockListener,
                mMockBuildInfo1,
                mMockBuildInfo2,
                mMockDevice1,
                mMockDevice2,
                mMockTargetPrep);
        mTestSuite.run(mMockListener);
        EasyMock.verify(
                mMockListener,
                mMockBuildInfo1,
                mMockBuildInfo2,
                mMockDevice1,
                mMockDevice2,
                mMockTargetPrep);
    }
}
