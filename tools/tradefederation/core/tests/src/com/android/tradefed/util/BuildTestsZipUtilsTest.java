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
package com.android.tradefed.util;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;

import com.android.tradefed.build.DeviceBuildInfo;
import com.android.tradefed.targetprep.AltDirBehavior;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.io.File;
import java.util.ArrayList;

/** Unit tests for {@link BuildTestsZipUtils}. */
@RunWith(JUnit4.class)
public class BuildTestsZipUtilsTest {

    /**
     * Tests that when the search is unsuccessful across tests dir, we return null, and no exception
     * is thrown.
     */
    @Test
    public void testGetApkFile_notFound() throws Exception {
        DeviceBuildInfo buildInfo = new DeviceBuildInfo();
        File testDir = FileUtil.createTempDir("test-dir-build-tests");
        try {
            buildInfo.setTestsDir(testDir, "1");
            File apkFile =
                    BuildTestsZipUtils.getApkFile(
                            buildInfo,
                            "TestApk.apk",
                            new ArrayList<>(),
                            AltDirBehavior.FALLBACK,
                            true,
                            null);
            assertNull(apkFile);
        } finally {
            FileUtil.recursiveDelete(testDir);
        }
    }

    /** Tests that when the search finds the file in the tests dir, the file found is returned. */
    @Test
    public void testGetApkFile_fromTestDir() throws Exception {
        DeviceBuildInfo buildInfo = new DeviceBuildInfo();
        File testDir = FileUtil.createTempDir("test-dir-build-tests");
        try {
            buildInfo.setTestsDir(testDir, "1");
            File apkDir = new File(testDir, "TestApk");
            apkDir.mkdir();
            File apk = new File(apkDir, "TestApk.apk");
            apk.createNewFile();
            File apkFile =
                    BuildTestsZipUtils.getApkFile(
                            buildInfo,
                            "TestApk.apk",
                            new ArrayList<>(),
                            AltDirBehavior.FALLBACK,
                            true,
                            null);
            assertNotNull(apkFile);
            assertEquals(apk, apkFile);
        } finally {
            FileUtil.recursiveDelete(testDir);
        }
    }

    /**
     * Tests that when the search finds the file in the sub testcases dir we still properly find it
     * and return it.
     */
    @Test
    public void testGetApkFile_fromTestDir_testCase() throws Exception {
        DeviceBuildInfo buildInfo = new DeviceBuildInfo();
        File testDir = FileUtil.createTempDir("test-dir-build-tests");
        try {
            buildInfo.setTestsDir(testDir, "1");
            File testcasedir = new File(testDir, "testcases");
            testcasedir.mkdir();
            File apkDir = new File(testcasedir, "TestApk");
            apkDir.mkdir();
            File apk = new File(apkDir, "TestApk.apk");
            apk.createNewFile();
            File apkFile =
                    BuildTestsZipUtils.getApkFile(
                            buildInfo,
                            "TestApk.apk",
                            new ArrayList<>(),
                            AltDirBehavior.FALLBACK,
                            true,
                            null);
            assertNotNull(apkFile);
            assertEquals(apk, apkFile);
        } finally {
            FileUtil.recursiveDelete(testDir);
        }
    }
}
