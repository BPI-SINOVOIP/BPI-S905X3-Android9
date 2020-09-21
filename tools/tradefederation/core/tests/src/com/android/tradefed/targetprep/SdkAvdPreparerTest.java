/*
 * Copyright (C) 2011 The Android Open Source Project
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

import com.android.ddmlib.IDevice;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.build.ISdkBuildInfo;
import com.android.tradefed.command.remote.DeviceDescriptor;
import com.android.tradefed.device.DeviceAllocationState;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.IDeviceManager;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.device.TestDeviceState;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.IRunUtil;

import junit.framework.TestCase;

import org.easymock.EasyMock;

import java.io.File;
import java.util.List;

/**
 * Unit tests for {@link SdkAvdPreparer}.
 */
public class SdkAvdPreparerTest extends TestCase {

    private static final String ANDROID_TOOL = "android";
    private static final String EMULATOR_TOOL = "emulator";
    private SdkAvdPreparer mPreparer;
    private IRunUtil mMockRunUtil;
    private IDeviceManager mMockDeviceManager;
    private ISdkBuildInfo mMockBuildInfo;
    private ITestDevice mMockDevice;
    private IDevice mMockIDevice;
    private File mParentFolder;

    /**
     * {@inheritDoc}
     */
    @Override
    protected void setUp() throws Exception {
        mMockRunUtil = EasyMock.createMock(IRunUtil.class);
        mMockDeviceManager = EasyMock.createMock(IDeviceManager.class);
        mParentFolder = FileUtil.createTempDir("sdk-avd-preparer");
        mPreparer = new SdkAvdPreparer(mMockRunUtil, mMockDeviceManager) {
            @Override
            File createParentSdkHome() throws java.io.IOException {
                return mParentFolder;
            }
        };
        mMockBuildInfo = EasyMock.createMock(ISdkBuildInfo.class);
        mMockDevice = EasyMock.createMock(ITestDevice.class);
        mMockIDevice = EasyMock.createMock(IDevice.class);

        mMockRunUtil.setEnvVariable(EasyMock.eq("ANDROID_SWT"), (String)EasyMock.anyObject());
        EasyMock.expect(mMockBuildInfo.getAndroidToolPath()).andStubReturn(ANDROID_TOOL);
        EasyMock.expect(mMockBuildInfo.getEmulatorToolPath()).andStubReturn(EMULATOR_TOOL);
        EasyMock.expect(mMockBuildInfo.getSdkDir()).andStubReturn(new File("sdk"));
        EasyMock.expect(mMockDevice.getIDevice()).andStubReturn(mMockIDevice);
        EasyMock.expect(mMockDevice.getSerialNumber()).andStubReturn("serial");
        EasyMock.expect(mMockDevice.getDeviceDescriptor()).andStubReturn(null);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    protected void tearDown() throws Exception {
        FileUtil.recursiveDelete(mParentFolder);
        super.tearDown();
    }

    /**
     * Test for {@link SdkAvdPreparer#setUp(ITestDevice, IBuildInfo)} when no SDK targets are found.
     */
    public void testSetUp_noTargets() throws Exception {
        setGetTargetsResponse("");
        replayMocks();
        try {
            mPreparer.setUp(mMockDevice, mMockBuildInfo);
            fail("TargetSetupError not thrown");
        } catch (TargetSetupError e) {
            // expected
        }
        verifyMocks();
    }

    /**
     * Test normal success case for {@link SdkAvdPreparer#setUp(ITestDevice,IBuildInfo)}
     */
    @SuppressWarnings("unchecked")
    public void testSetUp() throws Exception {
        mMockRunUtil.setEnvVariable(EasyMock.eq("ANDROID_SDK_HOME"), (String)EasyMock.anyObject());
        mMockRunUtil.setEnvVariable(EasyMock.eq("ANDROID_SDK_ROOT"), (String)EasyMock.anyObject());
        setGetTargetsResponse("target");
        setCreateAvdResponse("target");
        EasyMock.expect(mMockDevice.getDeviceState()).andReturn(TestDeviceState.NOT_AVAILABLE);
        EasyMock.expect(mMockDevice.getSerialNumber()).andReturn("emulator-5554").anyTimes();
        EasyMock.expect(mMockIDevice.isEmulator()).andReturn(true);
        mMockDeviceManager.launchEmulator(EasyMock.eq(mMockDevice), EasyMock.anyLong(),
                EasyMock.eq(mMockRunUtil), (List<String>)EasyMock.anyObject());
        // expect the commands to test emulator-adb connectivity
        EasyMock.expect(mMockDevice.executeShellCommand((String) EasyMock.anyObject()))
                .andReturn("").times(3);
        mMockRunUtil.sleep(EasyMock.anyLong());
        EasyMock.expectLastCall().times(3);
        // expect an avd name == target name
        EasyMock.expect(mMockIDevice.getAvdName()).andReturn("target");

        replayMocks();
        mPreparer.setUp(mMockDevice, mMockBuildInfo);
        verifyMocks();
    }

    /**
     * Test {@link SdkAvdPreparer#setUp(ITestDevice, IBuildInfo)} when emulator launches with unknown avd name
     */
    @SuppressWarnings("unchecked")
    public void testSetUp_noAvdName() throws Exception {
        mMockRunUtil.setEnvVariable(EasyMock.eq("ANDROID_SDK_HOME"), (String)EasyMock.anyObject());
        mMockRunUtil.setEnvVariable(EasyMock.eq("ANDROID_SDK_ROOT"), (String)EasyMock.anyObject());
        setGetTargetsResponse("target");
        setCreateAvdResponse("target");
        EasyMock.expect(mMockDevice.getDeviceState()).andReturn(TestDeviceState.NOT_AVAILABLE);
        EasyMock.expect(mMockDevice.getSerialNumber()).andReturn("emulator-5554").anyTimes();
        EasyMock.expect(mMockIDevice.isEmulator()).andReturn(true);
        mMockDeviceManager.launchEmulator(EasyMock.eq(mMockDevice), EasyMock.anyLong(),
                EasyMock.eq(mMockRunUtil), (List<String>)EasyMock.anyObject());

        // expect the commands to test emulator-adb connectivity
        EasyMock.expect(mMockDevice.executeShellCommand((String) EasyMock.anyObject()))
                .andReturn("").times(3);
        mMockRunUtil.sleep(EasyMock.anyLong());
        EasyMock.expectLastCall().times(3);
        // expect an avd name == target name
        EasyMock.expect(mMockIDevice.getAvdName()).andReturn("").times(2);
        EasyMock.expect(mMockDevice.getDeviceDescriptor()).andReturn(new DeviceDescriptor("SERIAL",
                false, DeviceAllocationState.Available, "unknown", "unknown", "unknown", "unknown",
                "unknown"));
        replayMocks();
        try {
            mPreparer.setUp(mMockDevice, mMockBuildInfo);
            fail("BuildError not thrown");
        } catch (BuildError e) {
            // expected
        }
        verifyMocks();
    }

    /**
     * Test {@link SdkAvdPreparer#setUp(ITestDevice, IBuildInfo)} when emulator fails to boot
     */
    @SuppressWarnings("unchecked")
    public void testSetUp_failedBoot() throws Exception {
        mMockRunUtil.setEnvVariable(EasyMock.eq("ANDROID_SDK_HOME"), (String)EasyMock.anyObject());
        mMockRunUtil.setEnvVariable(EasyMock.eq("ANDROID_SDK_ROOT"), (String)EasyMock.anyObject());
        setGetTargetsResponse("target");
        setCreateAvdResponse("target");
        EasyMock.expect(mMockDevice.getDeviceState()).andReturn(TestDeviceState.NOT_AVAILABLE);
        EasyMock.expect(mMockDevice.getSerialNumber()).andReturn("emulator-5554").anyTimes();
        EasyMock.expect(mMockIDevice.isEmulator()).andReturn(true);
        mMockDeviceManager.launchEmulator(EasyMock.eq(mMockDevice), EasyMock.anyLong(),
                EasyMock.eq(mMockRunUtil), (List<String>)EasyMock.anyObject());
        EasyMock.expectLastCall().andThrow(new DeviceNotAvailableException());
        mMockDeviceManager.killEmulator(EasyMock.eq(mMockDevice));
        EasyMock.expect(mMockDevice.getDeviceDescriptor()).andReturn(new DeviceDescriptor("SERIAL",
                false, DeviceAllocationState.Available, "unknown", "unknown", "unknown", "unknown",
                "unknown"));
        replayMocks();
        try {
            mPreparer.setUp(mMockDevice, mMockBuildInfo);
            fail("DeviceFailedToBootError not thrown");
        } catch (BuildError e) {
            // expected; use the general version to make absolutely sure that
            // DeviceFailedToBootError properly masquerades as a BuildError.
            assertTrue(e instanceof DeviceFailedToBootError);
        }
        verifyMocks();
    }

    /**
     * Test {@link SdkAvdPreparer#setUp(ITestDevice, IBuildInfo)} when avd creation fails
     */
    public void testSetUp_failedCreateAvd() throws Exception {
        mMockRunUtil.setEnvVariable(EasyMock.eq("ANDROID_SDK_HOME"), (String)EasyMock.anyObject());
        setGetTargetsResponse("target");
        // simulate a bad error message
        CommandResult result = new CommandResult(CommandStatus.SUCCESS);
        result.setStdout("");
        result.setStderr("Error: avd cannot be created due to a mysterious error");
        setCreateAvdResponse(result);
        EasyMock.expect(mMockDevice.getDeviceDescriptor()).andReturn(new DeviceDescriptor("SERIAL",
                false, DeviceAllocationState.Available, "unknown", "unknown", "unknown", "unknown",
                "unknown"));
        replayMocks();
        try {
            mPreparer.setUp(mMockDevice, mMockBuildInfo);
            fail("BuildError not thrown");
        } catch (BuildError e) {
            // expected
        }
        verifyMocks();
    }

    /**
     * Configure the mock objects to deliver given response for the 'android list targets --compact'
     * call
     */
    private void setGetTargetsResponse(String stdout) {
        CommandResult result = new CommandResult(CommandStatus.SUCCESS);
        result.setStdout(stdout);
        EasyMock.expect(mMockRunUtil.runTimedCmd(EasyMock.anyLong(), EasyMock.eq(ANDROID_TOOL),
                (String)EasyMock.anyObject(), (String)EasyMock.anyObject(),
                (String)EasyMock.anyObject())).andReturn(result);
    }

    /**
     * Configure the mock objects to deliver success response for the 'android create avd ...'
     * call.
     */
    private void setCreateAvdResponse(String avdName) {
        CommandResult result = new CommandResult(CommandStatus.SUCCESS);
        result.setStdout(String.format("Created AVD '%s'", avdName));
        setCreateAvdResponse(result);
    }

    /**
     * Configure the mock objects to deliver specific response for the 'android create avd ...'
     * call.
     */
    @SuppressWarnings("unchecked")
    private void setCreateAvdResponse(CommandResult result) {
        EasyMock.expect(mMockRunUtil.runTimedCmdWithInput(EasyMock.anyLong(),
             (String)EasyMock.anyObject(), (List<String>)EasyMock.anyObject())).andReturn(result);
    }

    private void replayMocks() {
        EasyMock.replay(mMockRunUtil, mMockDeviceManager, mMockBuildInfo, mMockDevice,
                mMockIDevice);
    }

    private void verifyMocks() {
        EasyMock.verify(mMockRunUtil, mMockDeviceManager, mMockBuildInfo, mMockDevice,
                mMockIDevice);
    }
}
