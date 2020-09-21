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
package com.android.tradefed.build;

import com.android.tradefed.util.FileUtil;

import junit.framework.TestCase;

import java.io.File;

/**
 * Unit tests for {@link KernelBuildInfo}.
 */
public class KernelDeviceBuildInfoTest extends TestCase {
    private static final String DEVICE_VERSION = "device";
    private static final String KERNEL_VERSION = "kernel";

    private KernelDeviceBuildInfo mBuildInfo;
    private File mDeviceFile;
    private File mKernelFile;

    @Override
    public void setUp() throws Exception {
        super.setUp();

        mBuildInfo = new KernelDeviceBuildInfo("kernel_device", "build");
        mBuildInfo.setDeviceBuild(new DeviceBuildInfo("device", "target"));
        mBuildInfo.setKernelBuild(new KernelBuildInfo("kernel", "ker", 0, "build"));

        mDeviceFile = FileUtil.createTempFile("device", "tmp");
        FileUtil.writeToFile("device", mDeviceFile);
        mBuildInfo.setDeviceImageFile(mDeviceFile, DEVICE_VERSION);

        mKernelFile = FileUtil.createTempFile("kernel", "tmp");
        FileUtil.writeToFile("kernel", mKernelFile);
        mBuildInfo.setKernelFile(mKernelFile, KERNEL_VERSION);
    }

    @Override
    public void tearDown() throws Exception {
        mDeviceFile.delete();
        mKernelFile.delete();

        super.tearDown();
    }

    /**
     * Test method for {@link KernelDeviceBuildInfo#clone()}.
     */
    public void testClone() throws Exception {
        KernelDeviceBuildInfo copy = (KernelDeviceBuildInfo) mBuildInfo.clone();

        try {
            assertEquals(mBuildInfo.getBuildBranch(), copy.getBuildBranch());
            assertEquals(mBuildInfo.getBuildFlavor(), copy.getBuildFlavor());
            assertEquals(mBuildInfo.getBuildId(), copy.getBuildId());
            assertEquals(mBuildInfo.getBuildTargetName(), copy.getBuildTargetName());
            assertEquals(mBuildInfo.getCommitTime(), copy.getCommitTime());
            assertEquals(mBuildInfo.getShortSha1(), copy.getShortSha1());
            assertEquals(mBuildInfo.getTestTag(), copy.getTestTag());

            assertFalse(mDeviceFile.getAbsolutePath().equals(copy.getDeviceImageFile()));
            assertTrue(FileUtil.compareFileContents(mDeviceFile, copy.getDeviceImageFile()));

            assertFalse(mKernelFile.getAbsolutePath().equals(copy.getKernelFile()));
            assertTrue(FileUtil.compareFileContents(mKernelFile, copy.getKernelFile()));
        } finally {
            copy.cleanUp();
        }
    }

    /**
     * Test method for {@link KernelDeviceBuildInfo#cleanUp()}.
     */
    public void testCleanUp() {
        assertTrue(mBuildInfo.getDeviceImageFile().exists());
        assertTrue(mBuildInfo.getKernelFile().exists());
        mBuildInfo.cleanUp();
        assertNull(mBuildInfo.getDeviceImageFile());
        assertNull(mBuildInfo.getKernelFile());
        assertFalse(mDeviceFile.exists());
        assertFalse(mKernelFile.exists());
    }
}
