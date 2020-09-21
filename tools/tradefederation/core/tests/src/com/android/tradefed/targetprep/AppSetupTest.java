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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.fail;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.times;

import com.android.tradefed.build.BuildInfo;
import com.android.tradefed.build.IAppBuildInfo;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.build.VersionedFile;
import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.util.AaptParser;
import com.android.tradefed.util.FileUtil;

import java.io.File;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.concurrent.TimeUnit;

import org.easymock.EasyMock;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.mockito.Mockito;

/**
 * Unit Tests for {@link AppSetup}.
 */
@RunWith(JUnit4.class)
public class AppSetupTest {

    private static final String SERIAL = "serial";
    private AppSetup mAppSetup;
    private ITestDevice mMockDevice;
    private IAppBuildInfo mMockBuildInfo;
    private AaptParser mMockAaptParser;

    @Before
    public void setUp() {
        mAppSetup = new AppSetup();
        mMockDevice = EasyMock.createMock(ITestDevice.class);
        EasyMock.expect(mMockDevice.getSerialNumber()).andStubReturn(SERIAL);
        EasyMock.expect(mMockDevice.getDeviceDescriptor()).andStubReturn(null);
        mMockBuildInfo = EasyMock.createMock(IAppBuildInfo.class);
        mMockAaptParser = Mockito.mock(AaptParser.class);
    }

    private void replayMocks() {
        EasyMock.replay(mMockDevice, mMockBuildInfo);
    }

    private void verifyMocks() {
        EasyMock.verify(mMockDevice, mMockBuildInfo);
    }

    /**
     * Test for {@link AppSetup#setUp(ITestDevice, IBuildInfo)} when the IBuildInfo is not an
     * instance of {@link IAppBuildInfo}.
     */
    @Test
    public void testSetup_notIAppBuildInfo() throws Exception {
        replayMocks();
        try {
            mAppSetup.setUp(mMockDevice, new BuildInfo());
            fail("Should have thrown an exception.");
        } catch (IllegalArgumentException expected) {
            assertEquals("Provided buildInfo is not a AppBuildInfo", expected.getMessage());
        }
        verifyMocks();
    }

    /**
     * Test for {@link AppSetup#setUp(ITestDevice, IBuildInfo)} when the apk installation fails
     * with some error.
     */
    @Test
    public void testSetup_failToInstall() throws Exception {
        List<VersionedFile> files = new ArrayList<>();
        File tmpFile = FileUtil.createTempFile("versioned", ".test");
        try {
            files.add(new VersionedFile(tmpFile, "1"));
            EasyMock.expect(mMockBuildInfo.getAppPackageFiles()).andReturn(files);
            EasyMock.expect(mMockDevice.installPackage(EasyMock.eq(tmpFile), EasyMock.eq(true)))
                    .andReturn("Error");
            replayMocks();
            mAppSetup.setUp(mMockDevice, mMockBuildInfo);
            fail("Should have thrown an exception.");
        } catch (BuildError expected) {
            assertEquals(String.format("Failed to install %s on %s. Reason: Error null",
                    tmpFile.getName(), SERIAL), expected.getMessage());
        } finally {
            FileUtil.deleteFile(tmpFile);
            verifyMocks();
        }
    }

    /**
     * Test for {@link AppSetup#setUp(ITestDevice, IBuildInfo)} when installation succeed but we
     * cannot get any aapt info from the apk.
     */
    @Test
    public void testSetup_aaptCannotParse() throws Exception {
        List<VersionedFile> files = new ArrayList<>();
        File tmpFile = FileUtil.createTempFile("versioned", ".test");
        try {
            files.add(new VersionedFile(tmpFile, "1"));
            EasyMock.expect(mMockBuildInfo.getAppPackageFiles()).andReturn(files);
            EasyMock.expect(mMockDevice.installPackage(EasyMock.eq(tmpFile), EasyMock.eq(true)))
                    .andReturn(null);
            replayMocks();
            mAppSetup.setUp(mMockDevice, mMockBuildInfo);
            fail("Should have thrown an exception.");
        } catch (TargetSetupError expected) {
            assertEquals(String.format("Failed to extract info from '%s' using aapt null",
                    tmpFile.getAbsolutePath()), expected.getMessage());
        } finally {
            FileUtil.deleteFile(tmpFile);
            verifyMocks();
        }
    }

    /**
     * Test for {@link AppSetup#setUp(ITestDevice, IBuildInfo)} when the install succeed but we
     * cannot find any package name on the apk.
     */
    @Test
    public void testSetup_noPackageName() throws Exception {
        mAppSetup = new AppSetup() {
            @Override
            AaptParser doAaptParse(File apkFile) {
                return mMockAaptParser;
            }
        };
        List<VersionedFile> files = new ArrayList<>();
        File tmpFile = FileUtil.createTempFile("versioned", ".test");
        try {
            files.add(new VersionedFile(tmpFile, "1"));
            EasyMock.expect(mMockBuildInfo.getAppPackageFiles()).andReturn(files);
            EasyMock.expect(mMockDevice.installPackage(EasyMock.eq(tmpFile), EasyMock.eq(true)))
                    .andReturn(null);
            doReturn(null).when(mMockAaptParser).getPackageName();
            replayMocks();
            mAppSetup.setUp(mMockDevice, mMockBuildInfo);
            fail("Should have thrown an exception.");
        } catch (TargetSetupError expected) {
            assertEquals(String.format("Failed to find package name for '%s' using aapt null",
                    tmpFile.getAbsolutePath()), expected.getMessage());
        } finally {
            FileUtil.deleteFile(tmpFile);
            verifyMocks();
            Mockito.verify(mMockAaptParser, times(1)).getPackageName();
        }
    }

    /**
     * Test for {@link AppSetup#setUp(ITestDevice, IBuildInfo)} when checking min sdk is enabled
     * and we fail to parse the file.
     */
    @Test
    public void testSetup_checkMinSdk_failParsing() throws Exception {
        mAppSetup = new AppSetup() {
            @Override
            AaptParser doAaptParse(File apkFile) {
                return null;
            }
        };
        OptionSetter setter = new OptionSetter(mAppSetup);
        setter.setOptionValue("check-min-sdk", "true");
        List<VersionedFile> files = new ArrayList<>();
        File tmpFile = FileUtil.createTempFile("versioned", ".test");
        try {
            files.add(new VersionedFile(tmpFile, "1"));
            EasyMock.expect(mMockBuildInfo.getAppPackageFiles()).andReturn(files);
            replayMocks();
            mAppSetup.setUp(mMockDevice, mMockBuildInfo);
            fail("Should have thrown an exception.");
        } catch (TargetSetupError expected) {
            assertEquals(String.format("Failed to extract info from '%s' using aapt null",
                    tmpFile.getName()), expected.getMessage());
        } finally {
            FileUtil.deleteFile(tmpFile);
            verifyMocks();
        }
    }

    /**
     * Test for {@link AppSetup#setUp(ITestDevice, IBuildInfo)} when we check the min sdk level
     * and the device API level is too low. Install should be skipped.
     */
    @Test
    public void testSetup_checkMinSdk_apiLow() throws Exception {
        mAppSetup = new AppSetup() {
            @Override
            AaptParser doAaptParse(File apkFile) {
                return mMockAaptParser;
            }
        };
        OptionSetter setter = new OptionSetter(mAppSetup);
        setter.setOptionValue("check-min-sdk", "true");
        List<VersionedFile> files = new ArrayList<>();
        File tmpFile = FileUtil.createTempFile("versioned", ".test");
        try {
            files.add(new VersionedFile(tmpFile, "1"));
            EasyMock.expect(mMockBuildInfo.getAppPackageFiles()).andReturn(files);
            EasyMock.expect(mMockDevice.getApiLevel()).andReturn(21).times(2);
            doReturn(22).when(mMockAaptParser).getSdkVersion();
            replayMocks();
            mAppSetup.setUp(mMockDevice, mMockBuildInfo);
        } finally {
            FileUtil.deleteFile(tmpFile);
            verifyMocks();
            Mockito.verify(mMockAaptParser, times(2)).getSdkVersion();
        }
    }

    /**
     * Test for {@link AppSetup#setUp(ITestDevice, IBuildInfo)} when api level of device is high
     * enough to install the apk.
     */
    @Test
    public void testSetup_checkMinSdk_apiOk() throws Exception {
        mAppSetup = new AppSetup() {
            @Override
            AaptParser doAaptParse(File apkFile) {
                return mMockAaptParser;
            }
        };
        OptionSetter setter = new OptionSetter(mAppSetup);
        setter.setOptionValue("check-min-sdk", "true");
        List<VersionedFile> files = new ArrayList<>();
        File tmpFile = FileUtil.createTempFile("versioned", ".test");
        try {
            files.add(new VersionedFile(tmpFile, "1"));
            EasyMock.expect(mMockBuildInfo.getAppPackageFiles()).andReturn(files);
            EasyMock.expect(mMockDevice.getApiLevel()).andReturn(23);
            doReturn(22).when(mMockAaptParser).getSdkVersion();
            EasyMock.expect(mMockDevice.installPackage(EasyMock.eq(tmpFile), EasyMock.eq(true)))
                    .andReturn(null);
            doReturn("com.fake.package").when(mMockAaptParser).getPackageName();
            replayMocks();
            mAppSetup.setUp(mMockDevice, mMockBuildInfo);
        } finally {
            FileUtil.deleteFile(tmpFile);
            verifyMocks();
            Mockito.verify(mMockAaptParser, times(2)).getPackageName();
            Mockito.verify(mMockAaptParser, times(1)).getSdkVersion();
        }
    }

    /**
     * Test for {@link AppSetup#setUp(ITestDevice, IBuildInfo)} when running a post install
     * command.
     */
    @Test
    public void testSetup_executePostInstall() throws Exception {
        final String fakeCmd = "fake command";
        OptionSetter setter = new OptionSetter(mAppSetup);
        setter.setOptionValue("install", "false");
        setter.setOptionValue("post-install-cmd", fakeCmd);
        mMockDevice.executeShellCommand(EasyMock.eq(fakeCmd), EasyMock.anyObject(),
                EasyMock.anyLong(), EasyMock.eq(TimeUnit.MILLISECONDS), EasyMock.eq(1));
        EasyMock.expectLastCall();
        replayMocks();
        mAppSetup.setUp(mMockDevice, mMockBuildInfo);
        verifyMocks();
    }

    /**
     * Test for {@link AppSetup#setUp(ITestDevice, IBuildInfo)} when uninstall all package is
     * requested but there is no package to uninstall.
     */
    @Test
    public void testSetup_uninstallAll_noPackage() throws Exception {
        OptionSetter setter = new OptionSetter(mAppSetup);
        setter.setOptionValue("install", "false");
        setter.setOptionValue("uninstall-all", "true");
        Set<String> res = new HashSet<>();
        EasyMock.expect(mMockDevice.getUninstallablePackageNames()).andReturn(res);
        replayMocks();
        mAppSetup.setUp(mMockDevice, mMockBuildInfo);
        verifyMocks();
    }

    /**
     * Test for {@link AppSetup#setUp(ITestDevice, IBuildInfo)} when uninstall is working and all
     * package to be uninstalled are uninstalled.
     */
    @Test
    public void testSetup_uninstallAll() throws Exception {
        OptionSetter setter = new OptionSetter(mAppSetup);
        setter.setOptionValue("install", "false");
        setter.setOptionValue("uninstall-all", "true");
        Set<String> res = new HashSet<>();
        res.add("com.fake1");
        EasyMock.expect(mMockDevice.getUninstallablePackageNames()).andReturn(res).times(2);
        EasyMock.expect(mMockDevice.uninstallPackage("com.fake1")).andReturn(null).times(2);
        EasyMock.expect(mMockDevice.getUninstallablePackageNames()).andReturn(new HashSet<>());
        replayMocks();
        mAppSetup.setUp(mMockDevice, mMockBuildInfo);
        verifyMocks();
    }

    /**
     * Test for {@link AppSetup#setUp(ITestDevice, IBuildInfo)} when uninstall is failing.
     */
    @Test
    public void testSetup_uninstallAll_fails() throws Exception {
        OptionSetter setter = new OptionSetter(mAppSetup);
        setter.setOptionValue("install", "false");
        setter.setOptionValue("uninstall-all", "true");
        Set<String> res = new HashSet<>();
        res.add("com.fake1");
        EasyMock.expect(mMockDevice.getUninstallablePackageNames()).andReturn(res).times(4);
        EasyMock.expect(mMockDevice.uninstallPackage("com.fake1")).andReturn("error").times(3);
        replayMocks();
        try {
            mAppSetup.setUp(mMockDevice, mMockBuildInfo);
            fail("Should have thrown an exception.");
        } catch (DeviceNotAvailableException expected) {
            assertEquals("Failed to uninstall apps on " + SERIAL, expected.getMessage());
        }
        verifyMocks();
    }

    /**
     * Test for {@link AppSetup#tearDown(ITestDevice, IBuildInfo, Throwable)} when throwable is
     * a DNAE instance, it should just return.
     */
    @Test
    public void testTearDown_DNAE() throws Exception {
        replayMocks();
        mAppSetup.tearDown(mMockDevice, mMockBuildInfo, new DeviceNotAvailableException());
        verifyMocks();
    }

    /**
     * Test for {@link AppSetup#tearDown(ITestDevice, IBuildInfo, Throwable)}.
     */
    @Test
    public void testTearDown() throws Exception {
        mMockDevice.reboot();
        EasyMock.expectLastCall();
        replayMocks();
        mAppSetup.tearDown(mMockDevice, mMockBuildInfo, null);
        verifyMocks();
    }
}
