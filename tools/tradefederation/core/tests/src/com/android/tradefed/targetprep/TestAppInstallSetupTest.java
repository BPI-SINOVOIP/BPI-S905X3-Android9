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
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.build.IDeviceBuildInfo;
import com.android.tradefed.command.remote.DeviceDescriptor;
import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.util.FileUtil;

import org.easymock.EasyMock;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.io.File;

/** Unit tests for {@link TestAppInstallSetup} */
@RunWith(JUnit4.class)
public class TestAppInstallSetupTest {

    private static final String SERIAL = "SERIAL";
    private static final String PACKAGE_NAME = "PACKAGE_NAME";
    private static final String APK_NAME = "fakeApk.apk";
    private File fakeApk;
    private File mFakeBuildApk;
    private TestAppInstallSetup mPrep;
    private IDeviceBuildInfo mMockBuildInfo;
    private ITestDevice mMockTestDevice;
    private File mTestDir;
    private File mBuildTestDir;
    private OptionSetter mSetter;

    @Before
    public void setUp() throws Exception {
        mTestDir = FileUtil.createTempDir("TestAppSetupTest");
        mBuildTestDir = FileUtil.createTempDir("TestAppBuildTestDir");
        // fake hierarchy of directory and files
        fakeApk = FileUtil.createTempFile("fakeApk", ".apk", mTestDir);
        FileUtil.copyFile(fakeApk, new File(mTestDir, APK_NAME));
        fakeApk = new File(mTestDir, APK_NAME);

        mFakeBuildApk = FileUtil.createTempFile("fakeApk", ".apk", mBuildTestDir);
        new File(mBuildTestDir, "DATA/app").mkdirs();
        FileUtil.copyFile(mFakeBuildApk, new File(mBuildTestDir, "DATA/app/" + APK_NAME));
        mFakeBuildApk = new File(mBuildTestDir, "/DATA/app/" + APK_NAME);

        mPrep =
                new TestAppInstallSetup() {
                    @Override
                    protected String parsePackageName(
                            File testAppFile, DeviceDescriptor deviceDescriptor) {
                        return PACKAGE_NAME;
                    }

                    @Override
                    protected File getLocalPathForFilename(
                            IBuildInfo buildInfo, String apkFileName, ITestDevice device)
                            throws TargetSetupError {
                        return fakeApk;
                    }
                };
        mPrep.addTestFileName(APK_NAME);

        mSetter = new OptionSetter(mPrep);
        mSetter.setOptionValue("cleanup-apks", "true");
        mMockBuildInfo = EasyMock.createMock(IDeviceBuildInfo.class);
        mMockTestDevice = EasyMock.createMock(ITestDevice.class);
        EasyMock.expect(mMockTestDevice.getSerialNumber()).andStubReturn(SERIAL);
        EasyMock.expect(mMockTestDevice.getDeviceDescriptor()).andStubReturn(null);
    }

    @After
    public void tearDown() throws Exception {
        FileUtil.recursiveDelete(mTestDir);
        FileUtil.recursiveDelete(mBuildTestDir);
    }

    @Test
    public void testSetupAndTeardown() throws Exception {
        EasyMock.expect(
                        mMockTestDevice.installPackage(
                                (File) EasyMock.anyObject(), EasyMock.eq(true)))
                .andReturn(null);
        EasyMock.replay(mMockBuildInfo, mMockTestDevice);
        mPrep.setUp(mMockTestDevice, mMockBuildInfo);
        EasyMock.verify(mMockBuildInfo, mMockTestDevice);
    }

    @Test
    public void testSetup_instantMode() throws Exception {
        OptionSetter setter = new OptionSetter(mPrep);
        setter.setOptionValue("instant-mode", "true");
        EasyMock.expect(
                        mMockTestDevice.installPackage(
                                (File) EasyMock.anyObject(),
                                EasyMock.eq(true),
                                EasyMock.eq("--instant")))
                .andReturn(null);
        EasyMock.replay(mMockBuildInfo, mMockTestDevice);
        mPrep.setUp(mMockTestDevice, mMockBuildInfo);
        EasyMock.verify(mMockBuildInfo, mMockTestDevice);
    }

    /**
     * If force-install-mode is set, we ignore "instant-mode". This allow some preparer to receive
     * options as part of the same Tf config but keep their behavior.
     */
    @Test
    public void testSetup_forceMode() throws Exception {
        OptionSetter setter = new OptionSetter(mPrep);
        setter.setOptionValue("instant-mode", "true");
        setter.setOptionValue("force-install-mode", "FULL");
        EasyMock.expect(
                        mMockTestDevice.installPackage(
                                (File) EasyMock.anyObject(), EasyMock.eq(true)))
                .andReturn(null);
        EasyMock.replay(mMockBuildInfo, mMockTestDevice);
        mPrep.setUp(mMockTestDevice, mMockBuildInfo);
        EasyMock.verify(mMockBuildInfo, mMockTestDevice);
    }

    @Test
    public void testInstallFailure() throws Exception {
        final String failure = "INSTALL_PARSE_FAILED_MANIFEST_MALFORMED";
        EasyMock.expect(
                        mMockTestDevice.installPackage(
                                (File) EasyMock.anyObject(), EasyMock.eq(true)))
                .andReturn(failure);
        EasyMock.replay(mMockBuildInfo, mMockTestDevice);
        try {
            mPrep.setUp(mMockTestDevice, mMockBuildInfo);
            fail("Expected TargetSetupError");
        } catch (TargetSetupError e) {
            String expected =
                    String.format(
                            "Failed to install %s on %s. Reason: '%s' " + "null",
                            "fakeApk.apk", SERIAL, failure);
            assertEquals(expected, e.getMessage());
        }
        EasyMock.verify(mMockBuildInfo, mMockTestDevice);
    }

    @Test
    public void testInstallFailedUpdateIncompatible() throws Exception {
        final String failure = "INSTALL_FAILED_UPDATE_INCOMPATIBLE";
        EasyMock.expect(
                        mMockTestDevice.installPackage(
                                (File) EasyMock.anyObject(), EasyMock.eq(true)))
                .andReturn(failure);
        EasyMock.expect(mMockTestDevice.uninstallPackage(PACKAGE_NAME)).andReturn(null);
        EasyMock.expect(
                        mMockTestDevice.installPackage(
                                (File) EasyMock.anyObject(), EasyMock.eq(true)))
                .andReturn(null);
        EasyMock.replay(mMockBuildInfo, mMockTestDevice);
        mPrep.setUp(mMockTestDevice, mMockBuildInfo);
        EasyMock.verify(mMockBuildInfo, mMockTestDevice);
    }

    /**
     * Test {@link TestAppInstallSetup#setUp(ITestDevice, IBuildInfo)} with a missing apk.
     * TargetSetupError expected.
     */
    @Test
    public void testMissingApk() throws Exception {
        fakeApk = null; // Apk doesn't exist

        try {
            mPrep.setUp(mMockTestDevice, mMockBuildInfo);
            fail("TestAppInstallSetup#setUp() did not raise TargetSetupError with missing apk.");
        } catch (TargetSetupError e) {
            assertTrue(e.getMessage().contains("not found"));
        }
    }

    /**
     * Test {@link TestAppInstallSetup#setUp(ITestDevice, IBuildInfo)} with an unreadable apk.
     * TargetSetupError expected.
     */
    @Test
    public void testUnreadableApk() throws Exception {
        fakeApk = new File("/not/a/real/path"); // Apk cannot be read

        try {
            mPrep.setUp(mMockTestDevice, mMockBuildInfo);
            fail("TestAppInstallSetup#setUp() did not raise TargetSetupError with unreadable apk.");
        } catch (TargetSetupError e) {
            assertTrue(e.getMessage().contains("not read"));
        }
    }

    /**
     * Test {@link TestAppInstallSetup#setUp(ITestDevice, IBuildInfo)} with a missing apk and
     * ThrowIfNoFile=False. Silent skip expected.
     */
    @Test
    public void testMissingApk_silent() throws Exception {
        fakeApk = null; // Apk doesn't exist
        mSetter.setOptionValue("throw-if-not-found", "false");

        mPrep.setUp(mMockTestDevice, mMockBuildInfo);
    }

    /**
     * Test {@link TestAppInstallSetup#setUp(ITestDevice, IBuildInfo)} with an unreadable apk and
     * ThrowIfNoFile=False. Silent skip expected.
     */
    @Test
    public void testUnreadableApk_silent() throws Exception {
        fakeApk = new File("/not/a/real/path"); // Apk cannot be read
        mSetter.setOptionValue("throw-if-not-found", "false");

        mPrep.setUp(mMockTestDevice, mMockBuildInfo);
    }

    /**
     * Tests that when in OVERRIDE mode we install first from alt-dirs, then from BuildInfo if not
     * found.
     */
    @Test
    public void testFindApk_override() throws Exception {
        mPrep =
                new TestAppInstallSetup() {
                    @Override
                    protected String parsePackageName(
                            File testAppFile, DeviceDescriptor deviceDescriptor) {
                        return PACKAGE_NAME;
                    }
                };
        mPrep.addTestFileName("fakeApk.apk");
        OptionSetter setter = new OptionSetter(mPrep);
        setter.setOptionValue("alt-dir-behavior", "OVERRIDE");
        setter.setOptionValue("alt-dir", mTestDir.getAbsolutePath());
        setter.setOptionValue("install-arg", "-d");

        EasyMock.expect(mMockTestDevice.getDeviceDescriptor()).andStubReturn(null);
        EasyMock.expect(mMockBuildInfo.getTestsDir()).andStubReturn(mBuildTestDir);

        EasyMock.expect(
                        mMockTestDevice.installPackage(
                                EasyMock.eq(fakeApk), EasyMock.anyBoolean(), EasyMock.eq("-d")))
                .andReturn(null);

        EasyMock.replay(mMockTestDevice, mMockBuildInfo);
        mPrep.setUp(mMockTestDevice, mMockBuildInfo);
        EasyMock.verify(mMockTestDevice, mMockBuildInfo);
    }

    /**
     * Test when OVERRIDE is set but there is not alt-dir, in this case we still use the BuildInfo.
     */
    @Test
    public void testFindApk_override_onlyInBuild() throws Exception {
        mPrep =
                new TestAppInstallSetup() {
                    @Override
                    protected String parsePackageName(
                            File testAppFile, DeviceDescriptor deviceDescriptor) {
                        return PACKAGE_NAME;
                    }
                };
        mPrep.addTestFileName("fakeApk.apk");
        OptionSetter setter = new OptionSetter(mPrep);
        setter.setOptionValue("alt-dir-behavior", "OVERRIDE");
        setter.setOptionValue("install-arg", "-d");

        EasyMock.expect(mMockTestDevice.getDeviceDescriptor()).andStubReturn(null);
        EasyMock.expect(mMockBuildInfo.getTestsDir()).andStubReturn(mBuildTestDir);

        EasyMock.expect(
                        mMockTestDevice.installPackage(
                                EasyMock.eq(mFakeBuildApk),
                                EasyMock.anyBoolean(),
                                EasyMock.eq("-d")))
                .andReturn(null);

        EasyMock.replay(mMockTestDevice, mMockBuildInfo);
        mPrep.setUp(mMockTestDevice, mMockBuildInfo);
        EasyMock.verify(mMockTestDevice, mMockBuildInfo);
    }
}
