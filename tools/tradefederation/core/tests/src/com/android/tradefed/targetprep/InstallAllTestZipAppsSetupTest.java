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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.fail;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.build.IDeviceBuildInfo;
import com.android.tradefed.config.OptionSetter;
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
import java.io.IOException;

/** Unit tests for {@link InstallAllTestZipAppsSetupTest} */
@RunWith(JUnit4.class)
public class InstallAllTestZipAppsSetupTest {

    private static final String SERIAL = "SERIAL";
    private InstallAllTestZipAppsSetup mPrep;
    private IBuildInfo mMockBuildInfo;
    private ITestDevice mMockTestDevice;
    private File mMockUnzipDir;
    private boolean mFailUnzip;
    private boolean mFailAapt;

    @Before
    public void setUp() throws Exception {
        mPrep =
                new InstallAllTestZipAppsSetup() {
                    @Override
                    File extractZip(File testsZip) throws IOException {
                        if (mFailUnzip) {
                            throw new IOException();
                        }
                        return mMockUnzipDir;
                    }

                    @Override
                    String getAppPackageName(File appFile) {
                        if (mFailAapt) {
                            return null;
                        }
                        return "";
                    }
                };
        mFailAapt = false;
        mFailUnzip = false;
        mMockUnzipDir = null;
        mMockBuildInfo = EasyMock.createMock(IDeviceBuildInfo.class);
        mMockTestDevice = EasyMock.createMock(ITestDevice.class);
        EasyMock.expect(mMockTestDevice.getSerialNumber()).andStubReturn(SERIAL);
        EasyMock.expect(mMockTestDevice.getDeviceDescriptor()).andStubReturn(null);
    }

    @After
    public void tearDown() throws Exception {
        if (mMockUnzipDir != null) {
            FileUtil.recursiveDelete(mMockUnzipDir);
        }
    }

    private void setMockUnzipDir() throws IOException {
        File testDir = FileUtil.createTempDir("TestAppSetupTest");
        // fake hierarchy of directory and files
        FileUtil.createTempFile("fakeApk", ".apk", testDir);
        FileUtil.createTempFile("fakeApk2", ".apk", testDir);
        FileUtil.createTempFile("notAnApk", ".txt", testDir);
        File subTestDir = FileUtil.createTempDir("SubTestAppSetupTest", testDir);
        FileUtil.createTempFile("subfakeApk", ".apk", subTestDir);
        mMockUnzipDir = testDir;
    }

    @Test
    public void testGetZipFile() throws TargetSetupError {
        String zip = "zip";
        mPrep.setTestZipName(zip);
        File file = new File(zip);
        EasyMock.expect(mMockBuildInfo.getFile(zip)).andReturn(file);
        EasyMock.replay(mMockBuildInfo, mMockTestDevice);
        File ret = mPrep.getZipFile(mMockTestDevice, mMockBuildInfo);
        assertEquals(file, ret);
        EasyMock.verify(mMockBuildInfo);
    }

    @Test
    public void testGetZipFileDoesntExist() throws TargetSetupError {
        String zip = "zip";
        mPrep.setTestZipName(zip);
        EasyMock.expect(mMockBuildInfo.getFile(zip)).andReturn(null);
        EasyMock.replay(mMockBuildInfo, mMockTestDevice);
        File ret = mPrep.getZipFile(mMockTestDevice, mMockBuildInfo);
        assertNull(ret);
        EasyMock.verify(mMockBuildInfo);
    }

    @Test
    public void testNullTestZipName() throws DeviceNotAvailableException {
        EasyMock.replay(mMockBuildInfo, mMockTestDevice);
        try {
            mPrep.setUp(mMockTestDevice, mMockBuildInfo);
            fail("Should have thrown a TargetSetupError");
        } catch (TargetSetupError e) {
            // expected
        }
        EasyMock.verify(mMockBuildInfo, mMockTestDevice);
    }

    @Test
    public void testSuccess() throws Exception {
        mPrep.setTestZipName("zip");

        mMockBuildInfo.getFile((String) EasyMock.anyObject());
        EasyMock.expectLastCall().andReturn(new File("zip"));

        setMockUnzipDir();

        mMockTestDevice.installPackage((File) EasyMock.anyObject(), EasyMock.anyBoolean());
        EasyMock.expectLastCall().andReturn(null).times(3);
        mMockTestDevice.uninstallPackage((String) EasyMock.anyObject());
        EasyMock.expectLastCall().andReturn(null).times(3);
        EasyMock.replay(mMockBuildInfo, mMockTestDevice);

        mPrep.setUp(mMockTestDevice, mMockBuildInfo);
        mPrep.tearDown(mMockTestDevice, mMockBuildInfo, null);

        EasyMock.verify(mMockBuildInfo, mMockTestDevice);
    }

    @Test
    public void testSuccessNoTearDown() throws Exception {
        mPrep.setTestZipName("zip");
        mPrep.setCleanup(false);

        mMockBuildInfo.getFile((String) EasyMock.anyObject());
        EasyMock.expectLastCall().andReturn(new File("zip"));

        setMockUnzipDir();

        mMockTestDevice.installPackage((File) EasyMock.anyObject(), EasyMock.anyBoolean());
        EasyMock.expectLastCall().andReturn(null).times(3);
        EasyMock.replay(mMockBuildInfo, mMockTestDevice);

        mPrep.setUp(mMockTestDevice, mMockBuildInfo);
        mPrep.tearDown(mMockTestDevice, mMockBuildInfo, null);

        EasyMock.verify(mMockBuildInfo, mMockTestDevice);
    }

    @Test
    public void testInstallFailure() throws DeviceNotAvailableException {
        final String failure = "INSTALL_PARSE_FAILED_MANIFEST_MALFORMED";
        final String file = "TEST";
        EasyMock.expect(
                        mMockTestDevice.installPackage(
                                (File) EasyMock.anyObject(), EasyMock.eq(true)))
                .andReturn(failure);
        EasyMock.replay(mMockBuildInfo, mMockTestDevice);
        try {
            mPrep.installApk(new File(file), mMockTestDevice);
            fail("Should have thrown an exception");
        } catch (TargetSetupError e) {
            String expected =
                    String.format(
                            "Failed to install %s on %s. Reason: '%s' " + "null",
                            file, SERIAL, failure);
            assertEquals(expected, e.getMessage());
        }
        EasyMock.verify(mMockBuildInfo, mMockTestDevice);
    }

    @Test
    public void testInstallFailureNoStop() throws DeviceNotAvailableException, TargetSetupError {
        final String failure = "INSTALL_PARSE_FAILED_MANIFEST_MALFORMED";
        final String file = "TEST";
        mPrep.setStopInstallOnFailure(false);
        EasyMock.expect(
                        mMockTestDevice.installPackage(
                                (File) EasyMock.anyObject(), EasyMock.eq(true)))
                .andReturn(failure);
        EasyMock.replay(mMockBuildInfo, mMockTestDevice);
        // should not throw exception
        mPrep.installApk(new File(file), mMockTestDevice);
        EasyMock.verify(mMockBuildInfo, mMockTestDevice);
    }

    @Test
    public void testDisable() throws Exception {
        OptionSetter setter = new OptionSetter(mPrep);
        setter.setOptionValue("disable", "true");
        EasyMock.replay(mMockBuildInfo, mMockTestDevice);
        mPrep.setUp(mMockTestDevice, mMockBuildInfo);
        mPrep.tearDown(mMockTestDevice, mMockBuildInfo, null);
        EasyMock.verify(mMockBuildInfo, mMockTestDevice);
    }

    @Test
    public void testUnzipFail() throws Exception {
        mFailUnzip = true;
        mPrep.setTestZipName("zip");

        mMockBuildInfo.getFile((String) EasyMock.anyObject());
        EasyMock.expectLastCall().andReturn(new File("zip"));

        EasyMock.replay(mMockBuildInfo, mMockTestDevice);

        try {
            mPrep.setUp(mMockTestDevice, mMockBuildInfo);
            fail("Should have thrown an exception");
        } catch (TargetSetupError e) {
            TargetSetupError error =
                    new TargetSetupError(
                            "Failed to extract test zip.",
                            e,
                            mMockTestDevice.getDeviceDescriptor());
            assertEquals(error.getMessage(), e.getMessage());
        }
        EasyMock.verify(mMockBuildInfo, mMockTestDevice);
    }

    @Test
    public void testAaptFail() throws Exception {
        mFailAapt = true;
        mPrep.setTestZipName("zip");
        setMockUnzipDir();

        mMockBuildInfo.getFile((String) EasyMock.anyObject());
        EasyMock.expectLastCall().andReturn(new File("zip"));
        mMockTestDevice.installPackage((File) EasyMock.anyObject(), EasyMock.anyBoolean());
        EasyMock.expectLastCall().andReturn(null);

        EasyMock.replay(mMockBuildInfo, mMockTestDevice);

        try {
            mPrep.setUp(mMockTestDevice, mMockBuildInfo);
            fail("Should have thrown an exception");
        } catch (TargetSetupError e) {
            TargetSetupError error =
                    new TargetSetupError(
                            "apk installed but AaptParser failed",
                            e,
                            mMockTestDevice.getDeviceDescriptor());
            assertEquals(error.getMessage(), e.getMessage());
        }
        EasyMock.verify(mMockBuildInfo, mMockTestDevice);
    }
}
