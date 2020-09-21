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

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.Configuration;
import com.android.tradefed.config.DeviceConfigurationHolder;
import com.android.tradefed.config.IConfiguration;
import com.android.tradefed.config.IDeviceConfiguration;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.targetprep.ITargetPreparer;
import com.android.tradefed.testtype.IRemoteTest;

import org.easymock.EasyMock;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

/** Unit tests for {@link ModuleDefinition} when exercised by a multi-device config. */
@RunWith(JUnit4.class)
public class ModuleDefinitionMultiTest {

    private static final String MODULE_NAME = "fakeName";
    private static final String DEVICE_NAME_1 = "device1";
    private static final String DEVICE_NAME_2 = "device2";

    private ModuleDefinition mModule;
    private List<IRemoteTest> mTestList;
    private Map<String, List<ITargetPreparer>> mMapDeviceTargetPreparer;

    private ITestDevice mDevice1;
    private ITestDevice mDevice2;

    private IBuildInfo mBuildInfo1;
    private IBuildInfo mBuildInfo2;

    private ITestInvocationListener mListener;

    private ITargetPreparer mMockTargetPrep;

    private IConfiguration mMultiDeviceConfiguration;

    @Before
    public void setUp() {
        mMultiDeviceConfiguration = new Configuration("name", "description");
        List<IDeviceConfiguration> deviceConfigList = new ArrayList<>();
        deviceConfigList.add(new DeviceConfigurationHolder(DEVICE_NAME_1));
        deviceConfigList.add(new DeviceConfigurationHolder(DEVICE_NAME_2));
        mMultiDeviceConfiguration.setDeviceConfigList(deviceConfigList);

        mDevice1 = EasyMock.createMock(ITestDevice.class);
        mDevice2 = EasyMock.createMock(ITestDevice.class);
        mBuildInfo1 = EasyMock.createMock(IBuildInfo.class);
        mBuildInfo2 = EasyMock.createMock(IBuildInfo.class);

        mListener = EasyMock.createMock(ITestInvocationListener.class);

        mTestList = new ArrayList<>();

        mMockTargetPrep = EasyMock.createMock(ITargetPreparer.class);

        mMapDeviceTargetPreparer = new LinkedHashMap<>();
        mMapDeviceTargetPreparer.put(DEVICE_NAME_1, new ArrayList<>());
        mMapDeviceTargetPreparer.put(DEVICE_NAME_2, new ArrayList<>());
    }

    private void replayMocks() {
        EasyMock.replay(mListener, mDevice1, mDevice2, mBuildInfo1, mBuildInfo2, mMockTargetPrep);
        for (IRemoteTest test : mTestList) {
            EasyMock.replay(test);
        }
    }

    private void verifyMocks() {
        EasyMock.verify(mListener, mDevice1, mDevice2, mBuildInfo1, mBuildInfo2, mMockTargetPrep);
        for (IRemoteTest test : mTestList) {
            EasyMock.verify(test);
        }
    }

    /**
     * Create a multi device module and run it. Ensure that target preparers against each device are
     * running.
     */
    @Test
    public void testCreateAndRun() throws Exception {
        // Add a preparer to the second device
        mMapDeviceTargetPreparer.get(DEVICE_NAME_2).add(mMockTargetPrep);
        mMultiDeviceConfiguration
                .getDeviceConfigByName(DEVICE_NAME_2)
                .addSpecificConfig(mMockTargetPrep);

        mModule =
                new ModuleDefinition(
                        MODULE_NAME,
                        mTestList,
                        mMapDeviceTargetPreparer,
                        new ArrayList<>(),
                        mMultiDeviceConfiguration);
        // Simulate injection of devices from ITestSuite
        mModule.getModuleInvocationContext().addAllocatedDevice(DEVICE_NAME_1, mDevice1);
        mModule.getModuleInvocationContext().addDeviceBuildInfo(DEVICE_NAME_1, mBuildInfo1);
        mModule.getModuleInvocationContext().addAllocatedDevice(DEVICE_NAME_2, mDevice2);
        mModule.getModuleInvocationContext().addDeviceBuildInfo(DEVICE_NAME_2, mBuildInfo2);

        mListener.testRunStarted(MODULE_NAME, 0);
        mListener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());

        // Target preparation is triggered against the preparer in the second device.
        EasyMock.expect(mMockTargetPrep.isDisabled()).andReturn(false);
        mMockTargetPrep.setUp(mDevice2, mBuildInfo2);

        replayMocks();
        mModule.run(mListener);
        verifyMocks();
    }
}
