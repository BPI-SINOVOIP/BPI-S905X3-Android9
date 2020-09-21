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
package com.android.tradefed.targetprep.suite;

import static org.junit.Assert.*;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.build.IDeviceBuildInfo;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.targetprep.TargetSetupError;
import com.android.tradefed.util.FileUtil;

import org.easymock.EasyMock;
import org.junit.Before;
import org.junit.Test;
import org.mockito.Mockito;

import java.io.File;
import java.io.FileNotFoundException;
import java.util.HashMap;
import java.util.Map;

/** Unit test for {@link SuiteApkInstaller} */
public class SuiteApkInstallerTest {

    private SuiteApkInstaller mPreparer;
    private IBuildInfo mMockBuildInfo;
    private ITestDevice mMockDevice;

    @Before
    public void setUp() {
        mPreparer = new SuiteApkInstaller();
        mMockBuildInfo = Mockito.mock(IBuildInfo.class);
        mMockDevice = Mockito.mock(ITestDevice.class);
    }

    /**
     * Test that when there is no $ANDROID_TARGET_OUT_TESTCASES defined and no ROOT_DIR, we throw an
     * exception because we have nowhere to look for the files.
     */
    @Test
    public void testGetTestsDir_noVar_noRootDir() {
        mPreparer =
                new SuiteApkInstaller() {
                    @Override
                    String getEnvVariable() {
                        return null;
                    }
                };
        doReturn(new HashMap<String, String>()).when(mMockBuildInfo).getBuildAttributes();
        try {
            mPreparer.getTestsDir(mMockBuildInfo);
            fail("Should have thrown an exception.");
        } catch (FileNotFoundException expected) {
            // expected
        }
    }

    /**
     * Test that when there is no $ANDROID_TARGET_OUT_TESTCASES defined but a ROOT_DIR is defined,
     * we return the ROOT_DIR location.
     */
    @Test
    public void testGetTestsDir_noVar() throws Exception {
        mPreparer =
                new SuiteApkInstaller() {
                    @Override
                    String getEnvVariable() {
                        return null;
                    }
                };
        File tmpDir = FileUtil.createTempDir("suite-apk-installer");
        try {
            Map<String, String> attributes = new HashMap<>();
            attributes.put("ROOT_DIR", tmpDir.getAbsolutePath());
            doReturn(attributes).when(mMockBuildInfo).getBuildAttributes();
            File res = mPreparer.getTestsDir(mMockBuildInfo);
            assertNotNull(res);
            assertEquals(tmpDir.getAbsolutePath(), res.getAbsolutePath());
        } finally {
            FileUtil.recursiveDelete(tmpDir);
        }
    }

    /**
     * Tests that when $ANDROID_TARGET_OUT_TESTCASES is defined it is returned, we do not check
     * ROOT_DIR.
     */
    @Test
    public void testGetLocalPathForFilename_withVariable() throws Exception {
        File varDir = FileUtil.createTempDir("suite-apk-installer-var");
        File apk = FileUtil.createTempFile("testapk", ".apk", varDir);
        try {
            mPreparer =
                    new SuiteApkInstaller() {
                        @Override
                        String getEnvVariable() {
                            return varDir.getAbsolutePath();
                        }
                    };
            File res =
                    mPreparer.getLocalPathForFilename(mMockBuildInfo, apk.getName(), mMockDevice);
            verify(mMockBuildInfo, times(0)).getBuildAttributes();
            assertNotNull(res);
            assertEquals(apk.getAbsolutePath(), res.getAbsolutePath());
        } finally {
            FileUtil.recursiveDelete(varDir);
        }
    }

    /**
     * Tests that when $ANDROID_TARGET_OUT_TESTCASES is defined but is not a directory, we check and
     * return ROOT_DIR instead.
     */
    @Test
    public void testGetTestsDir_notDir() throws Exception {
        File varDir = FileUtil.createTempFile("suite-apk-installer-var", ".txt");
        File tmpDir = FileUtil.createTempDir("suite-apk-installer");
        File apkFile = FileUtil.createTempFile("apk-test", ".apk", tmpDir);
        try {
            mPreparer =
                    new SuiteApkInstaller() {
                        @Override
                        String getEnvVariable() {
                            return varDir.getAbsolutePath();
                        }
                    };

            Map<String, String> attributes = new HashMap<>();
            attributes.put("ROOT_DIR", tmpDir.getAbsolutePath());
            doReturn(attributes).when(mMockBuildInfo).getBuildAttributes();
            File res =
                    mPreparer.getLocalPathForFilename(
                            mMockBuildInfo, apkFile.getName(), mMockDevice);
            assertNotNull(res);
            assertEquals(apkFile.getAbsolutePath(), res.getAbsolutePath());
        } finally {
            FileUtil.recursiveDelete(varDir);
            FileUtil.recursiveDelete(tmpDir);
        }
    }

    /**
     * Test that {@link SuiteApkInstaller#getLocalPathForFilename(IBuildInfo, String, ITestDevice)}
     * returns the apk file when found.
     */
    @Test
    public void testGetLocalPathForFileName() throws Exception {
        File tmpApk = FileUtil.createTempFile("suite-apk-installer", ".apk");
        mPreparer =
                new SuiteApkInstaller() {
                    @Override
                    protected File getTestsDir(IBuildInfo buildInfo) throws FileNotFoundException {
                        return tmpApk.getParentFile();
                    }
                };
        try {
            File apk =
                    mPreparer.getLocalPathForFilename(
                            mMockBuildInfo, tmpApk.getName(), mMockDevice);
            assertEquals(tmpApk.getAbsolutePath(), apk.getAbsolutePath());
        } finally {
            FileUtil.deleteFile(tmpApk);
        }
    }

    /**
     * Test that {@link SuiteApkInstaller#getLocalPathForFilename(IBuildInfo, String, ITestDevice)}
     * throws an exception when the apk file is not found.
     */
    @Test
    public void testGetLocalPathForFileName_noFound() throws Exception {
        File tmpApk = FileUtil.createTempFile("suite-apk-installer", ".apk");
        mPreparer =
                new SuiteApkInstaller() {
                    @Override
                    protected File getTestsDir(IBuildInfo buildInfo) throws FileNotFoundException {
                        return tmpApk.getParentFile();
                    }
                };
        try {
            mPreparer.getLocalPathForFilename(mMockBuildInfo, "no_exist", mMockDevice);
            fail("Should have thrown an exception.");
        } catch (TargetSetupError expected) {
            // expected
        } finally {
            FileUtil.deleteFile(tmpApk);
        }
    }

    /**
     * Test that {@link SuiteApkInstaller#getLocalPathForFilename(IBuildInfo, String, ITestDevice)}
     * returns the apk file located in IDeviceBuildInfo.getTestsDir().
     */
    @Test
    public void testGetLocalPathForFileName_testsDir() throws Exception {
        mPreparer =
                new SuiteApkInstaller() {
                    @Override
                    protected File getTestsDir(IBuildInfo buildInfo) throws FileNotFoundException {
                        return null;
                    }
                };
        IDeviceBuildInfo deviceBuildInfo = EasyMock.createMock(IDeviceBuildInfo.class);
        File tmpDir = null;
        try {
            tmpDir = FileUtil.createTempDir("test");
            File tmpApk = FileUtil.createTempFile("suite-apk-installer", ".apk", tmpDir);
            EasyMock.expect(deviceBuildInfo.getTestsDir()).andReturn(tmpDir);
            EasyMock.replay(deviceBuildInfo);
            File apk =
                    mPreparer.getLocalPathForFilename(
                            deviceBuildInfo, tmpApk.getName(), mMockDevice);
            assertEquals(tmpApk.getAbsolutePath(), apk.getAbsolutePath());
            EasyMock.verify(deviceBuildInfo);
        } finally {
            FileUtil.recursiveDelete(tmpDir);
        }
    }
}
