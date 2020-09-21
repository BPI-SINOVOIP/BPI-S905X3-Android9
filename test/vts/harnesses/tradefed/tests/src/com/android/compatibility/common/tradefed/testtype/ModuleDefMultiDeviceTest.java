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

package com.android.compatibility.common.tradefed.testtype;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.targetprep.BuildError;
import com.android.tradefed.targetprep.TargetSetupError;
import com.android.tradefed.targetprep.multi.IMultiTargetPreparer;
import com.android.tradefed.testtype.IAbi;
import com.android.tradefed.testtype.VtsMultiDeviceTest;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.easymock.EasyMock;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import static org.junit.Assert.assertEquals;

/**
 * Unit tests for {@link ModuleDefMultiDeviceTest}.
 */
@RunWith(JUnit4.class)
public class ModuleDefMultiDeviceTest {
    private ITestDevice mMockDevice;
    private IBuildInfo mBuildInfo;
    private ModuleDefMultiDevice mTest;
    private Map<ITestDevice, IBuildInfo> mDeviceInfos;
    private IInvocationContext mInvocationContext;
    private IMultiTargetPreparer mMultiTargetPreparer;
    private VtsMultiDeviceTest mVtsMultiDeviceTest;

    @Before
    public void setUp() throws Exception {
        mMockDevice = EasyMock.createMock(ITestDevice.class);
        mBuildInfo = EasyMock.createMock(IBuildInfo.class);
        IAbi mAbi = EasyMock.createMock(IAbi.class);
        EasyMock.expect(mAbi.getName()).andReturn("FAKE_ABI");
        mMultiTargetPreparer = EasyMock.createMock(IMultiTargetPreparer.class);

        List<IMultiTargetPreparer> preparers = new ArrayList<>();
        preparers.add(mMultiTargetPreparer);
        mVtsMultiDeviceTest = new VtsMultiDeviceTest();
        mTest = new ModuleDefMultiDevice(
                "FAKE_MODULE", mAbi, mVtsMultiDeviceTest, new ArrayList<>(), preparers, null);
        mDeviceInfos = new HashMap<>();
        mInvocationContext = EasyMock.createMock(IInvocationContext.class);
        ArrayList<ITestDevice> devices = new ArrayList<>();
        devices.add(mMockDevice);
        ArrayList<IBuildInfo> buildInfos = new ArrayList<>();
        buildInfos.add(mBuildInfo);
        EasyMock.expect(mInvocationContext.getDevices()).andReturn(devices);
        EasyMock.expect(mInvocationContext.getBuildInfos()).andReturn(buildInfos);
        EasyMock.replay(mInvocationContext);

        mTest.setDeviceInfos(mDeviceInfos);
        mTest.setInvocationContext(mInvocationContext);
    }

    /**
     * Test the testRunPreparerSetUp method.
     * @throws BuildError
     * @throws TargetSetupError
     * @throws DeviceNotAvailableException
     */
    @Test
    public void testRunPreparerSetups()
            throws TargetSetupError, BuildError, DeviceNotAvailableException {
        mMultiTargetPreparer.setUp(mInvocationContext);
        EasyMock.expectLastCall();
        EasyMock.replay(mMultiTargetPreparer);
        mTest.runPreparerSetups();
        EasyMock.verify(mMultiTargetPreparer);
    }

    /**
     * Test the prepareTestClass method.
     */
    @Test
    public void testPrepareTestClass() {
        mTest.prepareTestClass();
        assertEquals(mVtsMultiDeviceTest.getInvocationContext(), mInvocationContext);
    }
}
