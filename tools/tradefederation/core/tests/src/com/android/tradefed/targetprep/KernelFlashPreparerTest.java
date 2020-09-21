/*
 * Copyright (C) 2012 The Android Open Source Project
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

import com.android.tradefed.build.BuildInfo;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.build.IDeviceBuildInfo;
import com.android.tradefed.build.IKernelBuildInfo;
import com.android.tradefed.build.KernelDeviceBuildInfo;
import com.android.tradefed.command.remote.DeviceDescriptor;
import com.android.tradefed.device.DeviceAllocationState;
import com.android.tradefed.device.DeviceUnresponsiveException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.IRunUtil;

import junit.framework.TestCase;

import org.easymock.Capture;
import org.easymock.EasyMock;

import java.io.File;
import java.io.IOException;

/**
 * Unit tests for {@link KernelFlashPreparer}.
 */
public class KernelFlashPreparerTest extends TestCase {

    private ITestDevice mMockDevice;
    private IDeviceBuildInfo mMockDeviceBuildInfo;
    private IKernelBuildInfo mMockKernelBuildInfo;
    private IRunUtil mMockRunUtil;
    private KernelDeviceBuildInfo mBuildInfo;
    private KernelFlashPreparer mFlashPreparer;
    private File mKernel = null;
    private File mMkbootimg = null;
    private File mRamdisk = null;
    private File mBootImg = null;

    /**
     * {@inheritDoc}
     */
    @Override
    protected void setUp() throws Exception {
        super.setUp();

        mMkbootimg = File.createTempFile("mkbootimg", "");
        mRamdisk = File.createTempFile("ramdisk", ".img");
        mKernel = File.createTempFile("kernel", "");
        mBootImg = File.createTempFile("boot", ".img");

        mMockDevice = EasyMock.createMock(ITestDevice.class);
        EasyMock.expect(mMockDevice.getSerialNumber()).andStubReturn("serial");
        EasyMock.expect(mMockDevice.getBuildId()).andStubReturn("0");
        EasyMock.expect(mMockDevice.getDeviceDescriptor()).andStubReturn(null);

        mMockDeviceBuildInfo = EasyMock.createMock(IDeviceBuildInfo.class);
        EasyMock.expect(mMockDeviceBuildInfo.getRamdiskFile()).andStubReturn(mRamdisk);
        EasyMock.expect(mMockDeviceBuildInfo.getMkbootimgFile()).andStubReturn(mMkbootimg);

        mMockKernelBuildInfo = EasyMock.createMock(IKernelBuildInfo.class);
        EasyMock.expect(mMockKernelBuildInfo.getKernelFile()).andStubReturn(mKernel);
        EasyMock.expect(mMockKernelBuildInfo.getSha1()).andStubReturn("sha1");

        mBuildInfo = new KernelDeviceBuildInfo("0", "build");
        mBuildInfo.setDeviceBuild(mMockDeviceBuildInfo);
        mBuildInfo.setKernelBuild(mMockKernelBuildInfo);

        mMockRunUtil = EasyMock.createMock(IRunUtil.class);

        mFlashPreparer = new KernelFlashPreparer() {
            @Override
            IRunUtil getRunUtil() {
                return mMockRunUtil;
            }

            @Override
            String getBootImgPath() {
                return mBootImg.getAbsolutePath();
            }
        };
    }

    /**
     * {@inheritDoc}
     */
    @Override
    protected void tearDown() throws Exception {
        super.tearDown();

        FileUtil.deleteFile(mKernel);
        FileUtil.deleteFile(mMkbootimg);
        FileUtil.deleteFile(mRamdisk);
        FileUtil.deleteFile(mBootImg);
    }

    /**
     * Test {@link KernelFlashPreparer#setUp(ITestDevice, com.android.tradefed.build.IBuildInfo)} in
     * the normal execution path.
     * <p>
     * Expect for the boot image to be created, the device to be booted into the bootloader,
     * flashed, rebooted, and the boot image to be deleted.
     * </p>
     */
    public void testSetUp() throws Exception {
        mFlashPreparer = getStubBootPreparer();

        mMockDevice.rebootIntoBootloader();
        EasyMock.expect(mMockDevice.executeFastbootCommand("flash", "boot",
                mBootImg.getAbsolutePath())).andReturn(new CommandResult());
        mMockDevice.reboot();
        mMockDevice.postBootSetup();

        EasyMock.replay(mMockDevice, mMockDeviceBuildInfo, mMockKernelBuildInfo);
        mFlashPreparer.setUp(mMockDevice, mBuildInfo);
        EasyMock.verify(mMockDevice, mMockDeviceBuildInfo, mMockKernelBuildInfo);

        assertFalse(mBootImg.exists());
    }

    /**
     * Test {@link KernelFlashPreparer#setUp(ITestDevice, com.android.tradefed.build.IBuildInfo)} if
     * the boot image cannot be created
     * <p>
     * Expect a {@link TargetSetupError} to be thrown.
     * </p>
     */
    public void testSetup_targetsetuperror() throws Exception {
        mFlashPreparer = new KernelFlashPreparer() {
            @Override
            File createBootImage(File mkbootimg, File kernel, File ramdisk) throws IOException {
                throw new IOException();
            }
        };

        EasyMock.replay(mMockDevice, mMockDeviceBuildInfo, mMockKernelBuildInfo);
        try {
            mFlashPreparer.setUp(mMockDevice, mBuildInfo);
            fail("Expected TargetSetupError");
        } catch (TargetSetupError e) {
            // Expected
        }
        EasyMock.verify(mMockDevice, mMockDeviceBuildInfo, mMockKernelBuildInfo);
    }

    /**
     * Test {@link KernelFlashPreparer#setUp(ITestDevice, com.android.tradefed.build.IBuildInfo)} if
     * the device is unresponsive in fastboot.
     * <p>
     * Expect for the boot image to be created, a {@link DeviceUnresponsiveException} to be thrown,
     * and the boot image to be deleted.
     * </p>
     */
    public void testSetup_fastbooterror() throws Exception {
        mFlashPreparer = getStubBootPreparer();

        mMockDevice.rebootIntoBootloader();
        EasyMock.expect(mMockDevice.executeFastbootCommand("flash", "boot",
                mBootImg.getAbsolutePath())).andThrow(new DeviceUnresponsiveException());

        EasyMock.replay(mMockDevice, mMockDeviceBuildInfo, mMockKernelBuildInfo);
        try {
            mFlashPreparer.setUp(mMockDevice, mBuildInfo);
            fail("Expected DeviceUnresponsiveException");
        } catch (DeviceUnresponsiveException e) {
            // Expected
        }
        EasyMock.verify(mMockDevice, mMockDeviceBuildInfo, mMockKernelBuildInfo);

        assertFalse(mBootImg.exists());
    }

    /**
     * Test {@link KernelFlashPreparer#setUp(ITestDevice, com.android.tradefed.build.IBuildInfo)} if
     * the device is unresponsive at boot.
     * <p>
     * Expect for the boot image to be created, the device to be booted into the bootloader,
     * flashed, a {@link BuildError} to be thrown, and the boot image to be deleted.
     * </p>
     */
    public void testSetup_builderror() throws Exception {
        mFlashPreparer = getStubBootPreparer();

        mMockDevice.rebootIntoBootloader();
        EasyMock.expect(mMockDevice.executeFastbootCommand("flash", "boot",
                mBootImg.getAbsolutePath())).andReturn(new CommandResult());
        mMockDevice.reboot();
        EasyMock.expectLastCall().andThrow(new DeviceUnresponsiveException());
        EasyMock.expect(mMockDevice.getDeviceDescriptor()).andReturn(new DeviceDescriptor("SERIAL",
                false, DeviceAllocationState.Available, "unknown", "unknown", "unknown", "unknown",
                "unknown"));
        EasyMock.replay(mMockDevice, mMockDeviceBuildInfo, mMockKernelBuildInfo);
        try {
            mFlashPreparer.setUp(mMockDevice, mBuildInfo);
            fail("Expected BuildError");
        } catch (BuildError e) {
            // Expected
        }
        EasyMock.verify(mMockDevice, mMockDeviceBuildInfo, mMockKernelBuildInfo);

        assertFalse(mBootImg.exists());
    }

    /**
     * Test {@link KernelFlashPreparer#setUp(ITestDevice, com.android.tradefed.build.IBuildInfo)} if
     * the device boots successfully but is unresponsive post boot.
     * <p>
     * Expect for the boot image to be created, the device to be booted into the bootloader,
     * flashed, and rebooted, a {@link DeviceUnresponsiveException} to be thrown, and the boot image
     * to be deleted.
     * </p>
     */
    public void testSetup_unresponsive() throws Exception {
        mFlashPreparer = getStubBootPreparer();

        mMockDevice.rebootIntoBootloader();
        EasyMock.expect(mMockDevice.executeFastbootCommand("flash", "boot",
                mBootImg.getAbsolutePath())).andReturn(new CommandResult());
        mMockDevice.reboot();
        mMockDevice.postBootSetup();
        EasyMock.expectLastCall().andThrow(new DeviceUnresponsiveException());

        EasyMock.replay(mMockDevice, mMockDeviceBuildInfo, mMockKernelBuildInfo);
        try {
            mFlashPreparer.setUp(mMockDevice, mBuildInfo);
            fail("Expected DeviceUnresponsiveException");
        } catch (DeviceUnresponsiveException e) {
            // Expected
        }
        EasyMock.verify(mMockDevice, mMockDeviceBuildInfo, mMockKernelBuildInfo);

        assertFalse(mBootImg.exists());
    }

    /**
     * Test {@link KernelFlashPreparer#setUp(ITestDevice, com.android.tradefed.build.IBuildInfo)} if
     * passed an {@link IBuildInfo} object that is not a {@link KernelDeviceBuildInfo}.
     */
    public void testSetup_invalidbuildinfo() throws Exception {
        mFlashPreparer = new KernelFlashPreparer();
        try {
            mFlashPreparer.setUp(null, new BuildInfo());
            fail("Expected IllegalArgumentException");
        } catch (IllegalArgumentException e) {
            // Expected
        }
    }

    /**
     * Get a {@link KernelFlashPreparer} which returns {@code mBootImg} when
     * {@code createBootImage()} is called.
     */
    private KernelFlashPreparer getStubBootPreparer() {
        return new KernelFlashPreparer() {
            @Override
            File createBootImage(File mkbootimg, File kernel, File ramdisk) {
                return mBootImg;
            }
        };
    }

    /**
     * Test {@link KernelFlashPreparer#createBootImage(File, File, File)} in the normal execution
     * path.
     */
    public void testCreateBootImage() throws IOException {
        Capture<String> bootimgPath = new Capture<String>();
        EasyMock.expect(mMockRunUtil.runTimedCmd(EasyMock.anyInt(),
                EasyMock.eq(mMkbootimg.getAbsolutePath()), EasyMock.eq("--kernel"),
                EasyMock.eq(mKernel.getAbsolutePath()), EasyMock.eq("--ramdisk"),
                EasyMock.eq(mRamdisk.getAbsolutePath()), EasyMock.eq("-o"),
                EasyMock.capture(bootimgPath))).andReturn(new CommandResult(CommandStatus.SUCCESS));

        EasyMock.replay(mMockRunUtil);

        mBootImg = mFlashPreparer.createBootImage(mMkbootimg, mKernel, mRamdisk);
        assertEquals(mBootImg.getAbsolutePath(), bootimgPath.getValue());
        assertTrue(mBootImg.exists());
        assertTrue(mBootImg.isFile());
        EasyMock.verify(mMockRunUtil);
    }

    /**
     * Test {@link KernelFlashPreparer#createBootImage(File, File, File)} if mkbootimg fails.
     */
    public void testCreateBootImage_failed() {
        Capture<String> bootimgPath = new Capture<String>();
        EasyMock.expect(mMockRunUtil.runTimedCmd(EasyMock.anyInt(),
                EasyMock.eq(mMkbootimg.getAbsolutePath()), EasyMock.eq("--kernel"),
                EasyMock.eq(mKernel.getAbsolutePath()), EasyMock.eq("--ramdisk"),
                EasyMock.eq(mRamdisk.getAbsolutePath()), EasyMock.eq("-o"),
                EasyMock.capture(bootimgPath))).andReturn(new CommandResult(CommandStatus.FAILED));


        EasyMock.replay(mMockRunUtil);

        try {
            mBootImg = mFlashPreparer.createBootImage(mMkbootimg, mKernel, mRamdisk);
            fail("Expected IOException");
        } catch (IOException e) {
            // Expected.
        }
        assertEquals(mBootImg.getAbsolutePath(), bootimgPath.getValue());
        assertFalse(mBootImg.exists());

        EasyMock.verify(mMockRunUtil);
    }

    /**
     * Test {@link KernelFlashPreparer#createBootImage(File, File, File)} if there was a
     * Runtime.
     * <p>
     * Expect that the boot image is deleted.
     * </p>
     */
    public void testCreateBootImage_runtimeexception() throws IOException {
        Capture<String> bootimgPath = new Capture<String>();
        EasyMock.expect(mMockRunUtil.runTimedCmd(EasyMock.anyInt(),
                EasyMock.eq(mMkbootimg.getAbsolutePath()), EasyMock.eq("--kernel"),
                EasyMock.eq(mKernel.getAbsolutePath()), EasyMock.eq("--ramdisk"),
                EasyMock.eq(mRamdisk.getAbsolutePath()), EasyMock.eq("-o"),
                EasyMock.capture(bootimgPath))).andThrow(new RuntimeException());

        EasyMock.replay(mMockRunUtil);

        try {
            mBootImg = mFlashPreparer.createBootImage(mMkbootimg, mKernel, mRamdisk);
            fail("Expected RuntimeException");
        } catch (RuntimeException e) {
            // Expected.
        }
        assertEquals(mBootImg.getAbsolutePath(), bootimgPath.getValue());
        assertFalse(mBootImg.exists());

        EasyMock.verify(mMockRunUtil);
    }
}
