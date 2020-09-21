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
package com.android.tradefed.targetprep;

import com.android.tradefed.build.IDeviceBuildInfo;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.util.FileUtil;

import org.easymock.EasyMock;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.io.File;
import java.util.ArrayList;
import java.util.Collection;

import static org.junit.Assert.*;

/**
 * Unit tests for {@link InstallApkSetup}
 */
@RunWith(JUnit4.class)
public class InstallApkSetupTest {

    private static final String SERIAL = "SERIAL";
    private InstallApkSetup mInstallApkSetup;
    private IDeviceBuildInfo mMockBuildInfo;
    private ITestDevice mMockTestDevice;

    private File testDir = null;
    private File testFile = null;
    private Collection<File> testCollectionFiles = new ArrayList<File>();

    @Before
    public void setUp() throws Exception {
        mInstallApkSetup = new InstallApkSetup();
        mMockBuildInfo = EasyMock.createMock(IDeviceBuildInfo.class);
        mMockTestDevice = EasyMock.createMock(ITestDevice.class);
        EasyMock.expect(mMockTestDevice.getSerialNumber()).andStubReturn(SERIAL);
        EasyMock.expect(mMockTestDevice.getDeviceDescriptor()).andStubReturn(null);

        testDir = FileUtil.createTempDir("TestApkDir");
        testFile = FileUtil.createTempFile("File", ".apk", testDir);
    }

    @After
    public void tearDown() throws Exception {
        testCollectionFiles.clear();
        FileUtil.recursiveDelete(testDir);
    }

    /**
     * Test {@link InstallApkSetupTest#setUp()} by successfully installing 2 Apk files
     */
    @Test
    public void testSetup() throws DeviceNotAvailableException, BuildError, TargetSetupError {
        testCollectionFiles.add(testFile);
        testCollectionFiles.add(testFile);
        mInstallApkSetup.setApkPaths(testCollectionFiles);
        EasyMock.expect(mMockTestDevice.installPackage((File) EasyMock.anyObject(),
                EasyMock.eq(true))).andReturn(null).times(2);
        EasyMock.replay(mMockBuildInfo, mMockTestDevice);
        mInstallApkSetup.setUp(mMockTestDevice, mMockBuildInfo);
        EasyMock.verify(mMockBuildInfo, mMockTestDevice);
    }

    /**
     * Test {@link InstallApkSetupTest#setUp()} by installing a non-existing Apk
     */
    @Test
    public void testNonExistingApk() throws DeviceNotAvailableException, BuildError {
        testCollectionFiles.add(testFile);
        FileUtil.recursiveDelete(testFile);
        mInstallApkSetup.setApkPaths(testCollectionFiles);
        EasyMock.replay(mMockBuildInfo, mMockTestDevice);
        try {
            mInstallApkSetup.setUp(mMockTestDevice, mMockBuildInfo);
            fail("should have failed due to missing APK file");
        } catch (TargetSetupError expected) {
            String refMessage = String.format("%s does not exist null", testFile.getAbsolutePath());
            assertEquals(refMessage, expected.getMessage());
        }
        EasyMock.verify(mMockBuildInfo, mMockTestDevice);
    }

    /**
     * Test {@link InstallApkSetupTest#setUp()} by having an installation failure
     * but not throwing any exception
     */
    @Test
    public void testInstallFailureNoThrow() throws DeviceNotAvailableException, BuildError,
            TargetSetupError {
        testCollectionFiles.add(testFile);
        mInstallApkSetup.setApkPaths(testCollectionFiles);

        EasyMock.expect(mMockTestDevice.installPackage((File) EasyMock.anyObject(),
                EasyMock.eq(true))).andReturn(String.format("%s (Permission denied)",
                testFile.getAbsolutePath())).times(1);
        EasyMock.replay(mMockBuildInfo, mMockTestDevice);
        mInstallApkSetup.setUp(mMockTestDevice, mMockBuildInfo);
        EasyMock.verify(mMockBuildInfo, mMockTestDevice);
    }

    /**
     * Test {@link InstallApkSetupTest#setUp()} by having an installation failure
     * and throwing an exception
     */
    @Test
    public void testInstallFailureThrow() throws DeviceNotAvailableException, BuildError {
        testCollectionFiles.add(testFile);
        mInstallApkSetup.setApkPaths(testCollectionFiles);
        mInstallApkSetup.setThrowIfInstallFail(true);

        EasyMock.expect(mMockTestDevice.installPackage((File) EasyMock.anyObject(),
                EasyMock.eq(true))).andReturn(String.format("%s (Permission denied)",
                testFile.getAbsolutePath())).times(1);
        EasyMock.replay(mMockBuildInfo, mMockTestDevice);
        try {
            mInstallApkSetup.setUp(mMockTestDevice, mMockBuildInfo);
            fail("should have failed due to installation failure");
        } catch (TargetSetupError expected) {
            String refMessage = String.format("Stopping test: failed to install %s on device %s. " +
                    "Reason: %s (Permission denied) null", testFile.getAbsolutePath(),
                    SERIAL, testFile.getAbsolutePath());
            assertEquals(refMessage, expected.getMessage());
        }
        EasyMock.verify(mMockBuildInfo, mMockTestDevice);
    }
}
