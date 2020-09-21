/*
 * Copyright (C) 2010 The Android Open Source Project
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

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import com.android.tradefed.build.DeviceBuildInfo;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.build.IDeviceBuildInfo;
import com.android.tradefed.command.remote.DeviceDescriptor;
import com.android.tradefed.device.DeviceAllocationState;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.DeviceUnresponsiveException;
import com.android.tradefed.device.IDeviceManager;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.device.ITestDevice.RecoveryMode;
import com.android.tradefed.device.TestDeviceOptions;
import com.android.tradefed.host.IHostOptions;
import com.android.tradefed.targetprep.IDeviceFlasher.UserDataFlashOption;
import com.android.tradefed.util.CommandStatus;
import com.android.tradefed.util.FileUtil;

import org.easymock.EasyMock;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.io.File;
import java.util.Arrays;

/** Unit tests for {@link DeviceFlashPreparer}. */
@RunWith(JUnit4.class)
public class DeviceFlashPreparerTest {

    private IDeviceManager mMockDeviceManager;
    private IDeviceFlasher mMockFlasher;
    private DeviceFlashPreparer mDeviceFlashPreparer;
    private ITestDevice mMockDevice;
    private IDeviceBuildInfo mMockBuildInfo;
    private IHostOptions mMockHostOptions;
    private File mTmpDir;
    private boolean mFlashingMetricsReported;

    @Before
    public void setUp() throws Exception {
        mMockDeviceManager = EasyMock.createMock(IDeviceManager.class);
        mMockFlasher = EasyMock.createMock(IDeviceFlasher.class);
        mMockDevice = EasyMock.createMock(ITestDevice.class);
        EasyMock.expect(mMockDevice.getSerialNumber()).andReturn("foo").anyTimes();
        EasyMock.expect(mMockDevice.getOptions()).andReturn(new TestDeviceOptions()).anyTimes();
        mMockBuildInfo = new DeviceBuildInfo("0", "");
        mMockBuildInfo.setDeviceImageFile(new File("foo"), "0");
        mMockBuildInfo.setBuildFlavor("flavor");
        mMockHostOptions = EasyMock.createMock(IHostOptions.class);
        mFlashingMetricsReported = false;
        mDeviceFlashPreparer = new DeviceFlashPreparer() {
            @Override
            protected IDeviceFlasher createFlasher(ITestDevice device) {
                return mMockFlasher;
            }

            @Override
            int getDeviceBootPollTimeMs() {
                return 100;
            }

            @Override
            IDeviceManager getDeviceManager() {
                return mMockDeviceManager;
            }

            @Override
            protected IHostOptions getHostOptions() {
                return mMockHostOptions;
            }

            @Override
            protected void reportFlashMetrics(String branch, String buildFlavor, String buildId,
                    String serial, long queueTime, long flashingTime,
                    CommandStatus flashingStatus) {
                mFlashingMetricsReported = true;
            }
        };
        // Reset default settings
        mDeviceFlashPreparer.setDeviceBootTime(100);
        // expect this call
        mMockFlasher.setUserDataFlashOption(UserDataFlashOption.FLASH);
        mTmpDir = FileUtil.createTempDir("tmp");
    }

    @After
    public void tearDown() throws Exception {
        FileUtil.recursiveDelete(mTmpDir);
    }

    /** Simple normal case test for {@link DeviceFlashPreparer#setUp(ITestDevice, IBuildInfo)}. */
    @Test
    public void testSetup() throws Exception {
        doSetupExpectations();
        // report flashing success in normal case
        EasyMock.expect(mMockFlasher.getSystemFlashingStatus())
            .andReturn(CommandStatus.SUCCESS).anyTimes();
        EasyMock.replay(mMockFlasher, mMockDevice);
        mDeviceFlashPreparer.setUp(mMockDevice, mMockBuildInfo);
        EasyMock.verify(mMockFlasher, mMockDevice);
        assertTrue("should report flashing metrics in normal case", mFlashingMetricsReported);
    }

    /**
     * Set EasyMock expectations for a normal setup call
     */
    private void doSetupExpectations() throws TargetSetupError, DeviceNotAvailableException {
        mMockDevice.setRecoveryMode(RecoveryMode.ONLINE);
        mMockFlasher.overrideDeviceOptions(mMockDevice);
        mMockFlasher.setForceSystemFlash(false);
        mMockFlasher.setDataWipeSkipList(Arrays.asList(new String[]{}));
        mMockFlasher.flash(mMockDevice, mMockBuildInfo);
        mMockFlasher.setWipeTimeout(EasyMock.anyLong());
        mMockDevice.waitForDeviceOnline();
        EasyMock.expect(mMockDevice.enableAdbRoot()).andStubReturn(Boolean.TRUE);
        mMockDevice.setDate(null);
        EasyMock.expect(mMockDevice.getBuildId()).andReturn(mMockBuildInfo.getBuildId());
        EasyMock.expect(mMockDevice.getBuildFlavor()).andReturn(mMockBuildInfo.getBuildFlavor());
        EasyMock.expect(mMockDevice.isEncryptionSupported()).andStubReturn(Boolean.TRUE);
        EasyMock.expect(mMockDevice.isDeviceEncrypted()).andStubReturn(Boolean.FALSE);
        mMockDevice.clearLogcat();
        mMockDevice.waitForDeviceAvailable(EasyMock.anyLong());
        mMockDevice.setRecoveryMode(RecoveryMode.AVAILABLE);
        mMockDevice.postBootSetup();
    }

    /**
     * Test {@link DeviceFlashPreparer#setUp(ITestDevice, IBuildInfo)} when a non IDeviceBuildInfo
     * type is provided.
     */
    @Test
    public void testSetUp_nonDevice() throws Exception {
        try {
            mDeviceFlashPreparer.setUp(mMockDevice, EasyMock.createMock(IBuildInfo.class));
            fail("IllegalArgumentException not thrown");
        } catch (IllegalArgumentException e) {
            // expected
        }
    }

    /** Test {@link DeviceFlashPreparer#setUp(ITestDevice, IBuildInfo)} when build does not boot. */
    @Test
    public void testSetup_buildError() throws Exception {
        mMockDevice.setRecoveryMode(RecoveryMode.ONLINE);
        mMockFlasher.overrideDeviceOptions(mMockDevice);
        mMockFlasher.setForceSystemFlash(false);
        mMockFlasher.setDataWipeSkipList(Arrays.asList(new String[]{}));
        mMockFlasher.flash(mMockDevice, mMockBuildInfo);
        mMockFlasher.setWipeTimeout(EasyMock.anyLong());
        mMockDevice.waitForDeviceOnline();
        EasyMock.expect(mMockDevice.enableAdbRoot()).andStubReturn(Boolean.TRUE);
        mMockDevice.setDate(null);
        EasyMock.expect(mMockDevice.getBuildId()).andReturn(mMockBuildInfo.getBuildId());
        EasyMock.expect(mMockDevice.getBuildFlavor()).andReturn(mMockBuildInfo.getBuildFlavor());
        EasyMock.expect(mMockDevice.isEncryptionSupported()).andStubReturn(Boolean.TRUE);
        EasyMock.expect(mMockDevice.isDeviceEncrypted()).andStubReturn(Boolean.FALSE);
        mMockDevice.clearLogcat();
        mMockDevice.waitForDeviceAvailable(EasyMock.anyLong());
        EasyMock.expectLastCall().andThrow(new DeviceUnresponsiveException("foo", "fakeserial"));
        mMockDevice.setRecoveryMode(RecoveryMode.AVAILABLE);
        EasyMock.expect(mMockDevice.getDeviceDescriptor()).andReturn(
                new DeviceDescriptor("SERIAL", false, DeviceAllocationState.Available, "unknown",
                        "unknown", "unknown", "unknown", "unknown"));
        // report SUCCESS since device was flashed successfully (but didn't boot up)
        EasyMock.expect(mMockFlasher.getSystemFlashingStatus())
            .andReturn(CommandStatus.SUCCESS).anyTimes();
        EasyMock.replay(mMockFlasher, mMockDevice);
        try {
            mDeviceFlashPreparer.setUp(mMockDevice, mMockBuildInfo);
            fail("DeviceFlashPreparerTest not thrown");
        } catch (BuildError e) {
            // expected; use the general version to make absolutely sure that
            // DeviceFailedToBootError properly masquerades as a BuildError.
            assertTrue(e instanceof DeviceFailedToBootError);
        }
        EasyMock.verify(mMockFlasher, mMockDevice);
        assertTrue("should report flashing metrics with device boot failure",
                mFlashingMetricsReported);
    }

    /**
     * Test {@link DeviceFlashPreparer#setUp(ITestDevice, IBuildInfo)} when flashing step hits
     * device failure.
     **/
    @Test
    public void testSetup_flashException() throws Exception {
        mMockDevice.setRecoveryMode(RecoveryMode.ONLINE);
        mMockFlasher.overrideDeviceOptions(mMockDevice);
        mMockFlasher.setForceSystemFlash(false);
        mMockFlasher.setDataWipeSkipList(Arrays.asList(new String[]{}));
        mMockFlasher.flash(mMockDevice, mMockBuildInfo);
        EasyMock.expectLastCall().andThrow(new DeviceNotAvailableException());
        mMockFlasher.setWipeTimeout(EasyMock.anyLong());
        // report exception
        EasyMock.expect(mMockFlasher.getSystemFlashingStatus())
            .andReturn(CommandStatus.EXCEPTION).anyTimes();
        EasyMock.replay(mMockFlasher, mMockDevice);
        try {
            mDeviceFlashPreparer.setUp(mMockDevice, mMockBuildInfo);
            fail("DeviceNotAvailableException not thrown");
        } catch (DeviceNotAvailableException e) {
            // expected
        }
        EasyMock.verify(mMockFlasher, mMockDevice);
        assertTrue("should report flashing metrics with device flash failure",
                mFlashingMetricsReported);
    }

    /**
     * Test {@link DeviceFlashPreparer#setUp(ITestDevice, IBuildInfo)} when flashing of system
     * partitions are skipped.
     **/
    @Test
    public void testSetup_flashSkipped() throws Exception {
        doSetupExpectations();
        // report flashing status as null (for not flashing system partitions)
        EasyMock.expect(mMockFlasher.getSystemFlashingStatus()).andReturn(null).anyTimes();
        EasyMock.replay(mMockFlasher, mMockDevice);
        mDeviceFlashPreparer.setUp(mMockDevice, mMockBuildInfo);
        EasyMock.verify(mMockFlasher, mMockDevice);
        assertFalse("should not report flashing metrics in normal case", mFlashingMetricsReported);
    }
}
