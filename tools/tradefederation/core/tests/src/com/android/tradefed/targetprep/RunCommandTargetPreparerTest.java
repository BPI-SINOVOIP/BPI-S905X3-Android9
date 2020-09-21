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

import com.android.ddmlib.AdbCommandRejectedException;
import com.android.ddmlib.IDevice;
import com.android.ddmlib.IShellOutputReceiver;
import com.android.ddmlib.ShellCommandUnresponsiveException;
import com.android.ddmlib.TimeoutException;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.ConfigurationException;
import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.device.TestDeviceState;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;
import com.android.tradefed.util.RunUtil;

import org.easymock.EasyMock;
import org.junit.Before;
import org.junit.Test;

import java.io.IOException;
import java.util.concurrent.TimeUnit;


/** Unit Tests for {@link RunCommandTargetPreparer} */
public class RunCommandTargetPreparerTest {

    private static final int LONG_WAIT_TIME_MS = 200;
    private static final TestDeviceState ONLINE_STATE = TestDeviceState.ONLINE;

    private RunCommandTargetPreparer mPreparer = null;
    private ITestDevice mMockDevice = null;
    private IBuildInfo mMockBuildInfo = null;

    @Before
    public void setUp() throws Exception {
        mPreparer = new RunCommandTargetPreparer();
        OptionSetter setter = new OptionSetter(mPreparer);
        setter.setOptionValue("use-shell-v2", "true");
        mMockDevice = EasyMock.createMock(ITestDevice.class);
        mMockBuildInfo = EasyMock.createMock(IBuildInfo.class);
    }

    /**
     * Test that {@link RunCommandTargetPreparer#setUp(ITestDevice, IBuildInfo)} is properly going
     * through without exception when running a command.
     */
    @Test
    public void testSetUp() throws Exception {
        final String command = "mkdir test";
        OptionSetter setter = new OptionSetter(mPreparer);
        setter.setOptionValue("run-command", command);
        EasyMock.expect(mMockDevice.getSerialNumber()).andReturn("SERIAL").times(2);
        CommandResult res = new CommandResult();
        res.setStatus(CommandStatus.SUCCESS);
        res.setStdout("");
        EasyMock.expect(mMockDevice.executeShellV2Command(EasyMock.eq(command))).andReturn(res);
        EasyMock.replay(mMockDevice, mMockBuildInfo);
        mPreparer.setUp(mMockDevice, mMockBuildInfo);
        EasyMock.verify(mMockDevice, mMockBuildInfo);
    }

    /**
     * Test that {@link RunCommandTargetPreparer#setUp(ITestDevice, IBuildInfo)} is properly going
     * through without exception when running a command with timeout.
     */
    @Test
    public void testSetUp_withTimeout() throws Exception {
        final String command = "mkdir test";
        OptionSetter setter = new OptionSetter(mPreparer);
        setter.setOptionValue("run-command", command);
        setter.setOptionValue("run-command-timeout", "100");
        EasyMock.expect(mMockDevice.getSerialNumber()).andReturn("SERIAL").times(2);
        CommandResult res = new CommandResult();
        res.setStatus(CommandStatus.SUCCESS);
        res.setStdout("");
        EasyMock.expect(
                        mMockDevice.executeShellV2Command(
                                EasyMock.eq(command),
                                EasyMock.eq(100l),
                                EasyMock.eq(TimeUnit.MILLISECONDS),
                                EasyMock.eq(0)))
                .andReturn(res);
        EasyMock.replay(mMockDevice, mMockBuildInfo);
        mPreparer.setUp(mMockDevice, mMockBuildInfo);
        EasyMock.verify(mMockDevice, mMockBuildInfo);
    }

    /**
     * Test that {@link RunCommandTargetPreparer#setUp(ITestDevice, IBuildInfo)} and {@link
     * RunCommandTargetPreparer#tearDown(ITestDevice, IBuildInfo, Throwable)} are properly skipped
     * when disabled and no command is ran.
     */
    @Test
    public void testDisabled() throws Exception {
        final String command = "mkdir test";
        OptionSetter setter = new OptionSetter(mPreparer);
        setter.setOptionValue("run-command", command);
        setter.setOptionValue("disable", "true");
        EasyMock.replay(mMockDevice, mMockBuildInfo);
        mPreparer.setUp(mMockDevice, mMockBuildInfo);
        mPreparer.tearDown(mMockDevice, mMockBuildInfo, null);
        EasyMock.verify(mMockDevice, mMockBuildInfo);
    }

    /**
     * Test that {@link RunCommandTargetPreparer#tearDown(ITestDevice, IBuildInfo, Throwable)} is
     * properly going through without exception when running a command.
     */
    @Test
    public void testTearDown() throws Exception {
        final String command = "mkdir test";
        OptionSetter setter = new OptionSetter(mPreparer);
        setter.setOptionValue("teardown-command", command);
        EasyMock.expect(mMockDevice.getSerialNumber()).andReturn("SERIAL").times(1);
        EasyMock.expect(mMockDevice.executeShellCommand(EasyMock.eq(command))).andReturn("");
        EasyMock.replay(mMockDevice, mMockBuildInfo);
        mPreparer.tearDown(mMockDevice, mMockBuildInfo, null);
        EasyMock.verify(mMockDevice, mMockBuildInfo);
    }

    /**
     * Test that {@link RunCommandTargetPreparer#setUp(ITestDevice, IBuildInfo)} and
     * {@link RunCommandTargetPreparer#tearDown(ITestDevice, IBuildInfo, Throwable)} is properly
     * going through without exception when running a background command.
     */
    @Test
    public void testBgCmd() throws AdbCommandRejectedException, ConfigurationException,
        DeviceNotAvailableException, IOException, ShellCommandUnresponsiveException,
        TargetSetupError, TimeoutException {
        final String command = "mkdir test";
        OptionSetter setter = new OptionSetter(mPreparer);
        setter.setOptionValue("run-bg-command", command);
        IDevice mMockIDevice = EasyMock.createMock(IDevice.class);
        EasyMock.expect(mMockIDevice.getSerialNumber()).andReturn("SERIAL").anyTimes();
        IShellOutputReceiver mMockReceiver = EasyMock.createMock(IShellOutputReceiver.class);
        mMockReceiver.addOutput((byte[]) EasyMock.anyObject(), EasyMock.anyInt(),
                EasyMock.anyInt());
        EasyMock.expect(mMockDevice.getIDevice()).andReturn(mMockIDevice).atLeastOnce();
        EasyMock.expect(mMockDevice.getSerialNumber()).andReturn("SERIAL").atLeastOnce();
        mMockIDevice.executeShellCommand(EasyMock.eq(command), EasyMock.same(mMockReceiver),
                EasyMock.anyLong(), EasyMock.eq(TimeUnit.MILLISECONDS));
        EasyMock.expect(mMockDevice.getDeviceState()).andReturn(ONLINE_STATE).atLeastOnce();
        EasyMock.replay(mMockDevice, mMockBuildInfo);
        mPreparer.setUp(mMockDevice, mMockBuildInfo);
        RunUtil.getDefault().sleep(LONG_WAIT_TIME_MS);
        mPreparer.tearDown(mMockDevice, mMockBuildInfo, null);
        EasyMock.verify(mMockDevice, mMockBuildInfo);
    }
}
