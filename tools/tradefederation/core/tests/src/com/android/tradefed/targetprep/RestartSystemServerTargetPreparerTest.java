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
package com.android.tradefed.targetprep;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;

import org.easymock.EasyMock;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Unit Tests for {@link RestartSystemServerTargetPreparer}. */
@RunWith(JUnit4.class)
public class RestartSystemServerTargetPreparerTest {

    private RestartSystemServerTargetPreparer mRestartSystemServerTargetPreparer;
    private ITestDevice mMockDevice;
    private IBuildInfo mMockBuildInfo;

    @Before
    public void setUp() {
        mMockDevice = EasyMock.createMock(ITestDevice.class);
        mMockBuildInfo = EasyMock.createMock(IBuildInfo.class);
        mRestartSystemServerTargetPreparer = new RestartSystemServerTargetPreparer();
    }

    @Test
    public void testSetUp_bootComplete() throws Exception {
        EasyMock.expect(mMockDevice.enableAdbRoot()).andReturn(true).once();
        EasyMock.expect(mMockDevice.executeShellCommand("setprop dev.bootcomplete 0"))
                .andReturn(null);
        EasyMock.expect(
                        mMockDevice.executeShellCommand(
                                RestartSystemServerTargetPreparer.KILL_SERVER_COMMAND))
                .andReturn(null)
                .once();
        mMockDevice.waitForDeviceAvailable();
        EasyMock.expectLastCall().once();
        EasyMock.replay(mMockDevice, mMockBuildInfo);

        mRestartSystemServerTargetPreparer.setUp(mMockDevice, mMockBuildInfo);
        EasyMock.verify(mMockDevice, mMockBuildInfo);
    }

    @Test(expected = DeviceNotAvailableException.class)
    public void testSetUp_bootNotComplete() throws Exception {
        EasyMock.expect(mMockDevice.enableAdbRoot()).andReturn(true).once();
        EasyMock.expect(mMockDevice.executeShellCommand("setprop dev.bootcomplete 0"))
                .andReturn(null);
        EasyMock.expect(
                        mMockDevice.executeShellCommand(
                                RestartSystemServerTargetPreparer.KILL_SERVER_COMMAND))
                .andReturn(null)
                .once();
        mMockDevice.waitForDeviceAvailable();
        EasyMock.expectLastCall().andThrow(new DeviceNotAvailableException());
        EasyMock.replay(mMockDevice, mMockBuildInfo);

        mRestartSystemServerTargetPreparer.setUp(mMockDevice, mMockBuildInfo);
    }

    @Test
    public void testTearDown_restart() throws Exception {
        OptionSetter optionSetter = new OptionSetter(mRestartSystemServerTargetPreparer);
        optionSetter.setOptionValue("restart-setup", "false");
        optionSetter.setOptionValue("restart-teardown", "true");

        EasyMock.expect(mMockDevice.enableAdbRoot()).andReturn(true).once();
        EasyMock.expect(mMockDevice.executeShellCommand("setprop dev.bootcomplete 0"))
                .andReturn(null);
        EasyMock.expect(
                        mMockDevice.executeShellCommand(
                                RestartSystemServerTargetPreparer.KILL_SERVER_COMMAND))
                .andReturn(null)
                .once();
        mMockDevice.waitForDeviceAvailable();
        EasyMock.expectLastCall().once();
        EasyMock.replay(mMockDevice, mMockBuildInfo);

        mRestartSystemServerTargetPreparer.tearDown(mMockDevice, mMockBuildInfo, null);
        EasyMock.verify(mMockDevice, mMockBuildInfo);
    }

    @Test
    public void testNone() throws Exception {
        OptionSetter optionSetter = new OptionSetter(mRestartSystemServerTargetPreparer);
        optionSetter.setOptionValue("restart-setup", "false");
        optionSetter.setOptionValue("restart-teardown", "false");
        EasyMock.replay(mMockDevice, mMockBuildInfo);

        mRestartSystemServerTargetPreparer.setUp(mMockDevice, mMockBuildInfo);
        EasyMock.verify(mMockDevice, mMockBuildInfo);
    }
}
