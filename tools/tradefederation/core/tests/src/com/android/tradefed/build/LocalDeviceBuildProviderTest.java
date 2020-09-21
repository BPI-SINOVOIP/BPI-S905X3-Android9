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
package com.android.tradefed.build;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.ZipUtil2;

import org.apache.commons.compress.archivers.zip.ZipFile;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import java.io.File;


/**
 * Unit tests for {@link LocalDeviceBuildProvider}
 */
public class LocalDeviceBuildProviderTest {

    private static final String ANDROID_INFO =
            "require board=board\n"
                    + "require version-bootloader=BHZ11h\n"
                    + "require version-baseband=M8994F-2.6.36.2.20";
    private static final String BOOT_IMG = "boot_img";
    private static final String RADIO_IMG = "radio_img";
    private static final String BOOTLOADER_IMG = "bootloader_img";
    private static final String SYSTEM_IMG = "system_img";

    private LocalDeviceBuildProvider mLocalDeviceBuildProvider = null;
    private File mTmpDir = null;
    private File mBuildDir = null;
    private File mTestDir = null;
    private File mAndroidInfo = null;
    private File mBootImg = null;
    private File mRadioImg = null;
    private File mBootloaderImg = null;
    private File mSystemImg = null;

    @Before
    public void setUp() throws Exception {
        mTmpDir = FileUtil.createTempDir("tmp");
        mBuildDir = FileUtil.createTempDir("buildOutput");
        mTestDir = FileUtil.createTempDir("tests");
        mAndroidInfo = new File(mBuildDir, "android-info.txt");
        FileUtil.writeToFile(ANDROID_INFO, mAndroidInfo);
        mBootImg = new File(mBuildDir, "boot.img");
        FileUtil.writeToFile(BOOT_IMG, mBootImg);
        mRadioImg = new File(mBuildDir, "radio.img");
        FileUtil.writeToFile(RADIO_IMG, mRadioImg);
        mBootloaderImg = new File(mBuildDir, "bootloader.img");
        FileUtil.writeToFile(BOOTLOADER_IMG, mBootloaderImg);
        mSystemImg = new File(mBuildDir, "system.img");
        FileUtil.writeToFile(SYSTEM_IMG, mSystemImg);
        mLocalDeviceBuildProvider = new LocalDeviceBuildProvider();
        mLocalDeviceBuildProvider.setBuildDir(mBuildDir);
    }

    @After
    public void tearDown() throws Exception {
        FileUtil.recursiveDelete(mTmpDir);
        FileUtil.recursiveDelete(mBuildDir);
        FileUtil.recursiveDelete(mTestDir);
    }

    @Test
    public void testGetBuild() throws Exception {
        File imgFile = new File(mBuildDir, "img.zip");
        mLocalDeviceBuildProvider = new LocalDeviceBuildProvider() {
            @Override
            void setDeviceImageFile(DeviceBuildInfo buildInfo) {
                buildInfo.setDeviceImageFile(imgFile, buildInfo.getBuildId());
            }
            @Override
            void parseBootloaderAndRadioVersions(DeviceBuildInfo buildInfo) {
                // do nothing
            }
        };
        mLocalDeviceBuildProvider.setBuildDir(mBuildDir);
        DeviceBuildInfo info = (DeviceBuildInfo)mLocalDeviceBuildProvider.getBuild();
        assertEquals(imgFile, info.getDeviceImageFile());
    }

    @Test
    public void testGetBuild_noImage() throws Exception {
        File buildImageZip = new File(mTmpDir, "image.zip");
        mLocalDeviceBuildProvider = new LocalDeviceBuildProvider() {
            @Override
            File createBuildImageZip() throws BuildRetrievalError {
                return buildImageZip;
            }
            @Override
            void parseBootloaderAndRadioVersions(DeviceBuildInfo buildInfo) {
                // do nothing
            }
        };
        mLocalDeviceBuildProvider.setBuildDir(mBuildDir);
        DeviceBuildInfo info = (DeviceBuildInfo)mLocalDeviceBuildProvider.getBuild();
        assertEquals(buildImageZip, info.getDeviceImageFile());
    }

    @Test
    public void testCreateBuildImageZip() throws Exception {
        File buildImageZip = mLocalDeviceBuildProvider.createBuildImageZip();
        try (ZipFile zip = new ZipFile(buildImageZip)) {
            assertNotNull(buildImageZip);
            ZipUtil2.extractZip(zip, mTmpDir);
            File extractedFile1 = new File(mTmpDir, mAndroidInfo.getName());
            File extractedFile2 = new File(mTmpDir, mBootImg.getName());
            File extractedFile3 = new File(mTmpDir, mSystemImg.getName());
            assertTrue(extractedFile1.exists());
            assertTrue(extractedFile2.exists());
            assertTrue(extractedFile3.exists());
            assertTrue(FileUtil.compareFileContents(mAndroidInfo, extractedFile1));
            assertTrue(FileUtil.compareFileContents(mBootImg, extractedFile2));
            assertTrue(FileUtil.compareFileContents(mSystemImg, extractedFile3));
        } finally {
            FileUtil.deleteFile(buildImageZip);
        }
    }

    @Test
    public void testSetRadioImage() throws Exception {
        DeviceBuildInfo buildInfo = new DeviceBuildInfo();
        mLocalDeviceBuildProvider.setRadioVersion("radio_version");
        mLocalDeviceBuildProvider.setRadioImage(buildInfo);
        assertEquals(mRadioImg, buildInfo.getBasebandImageFile());
        assertEquals("radio_version", buildInfo.getBasebandVersion());
    }

    @Test
    public void testSetBootloaderImage() throws Exception {
        DeviceBuildInfo buildInfo = new DeviceBuildInfo();
        mLocalDeviceBuildProvider.setBootloaderVersion("bootloader_version");
        mLocalDeviceBuildProvider.setBootloaderImage(buildInfo);
        assertEquals(mBootloaderImg, buildInfo.getBootloaderImageFile());
        assertEquals("bootloader_version", buildInfo.getBootloaderVersion());
    }

    @Test
    public void testSetDeviceImageFile() throws Exception {
        DeviceBuildInfo buildInfo = new DeviceBuildInfo();
        File buildImageZip = new File(mBuildDir, "board-img-12345.zip");
        FileUtil.writeToFile("build_image", buildImageZip);
        mLocalDeviceBuildProvider.setDeviceImageFile(buildInfo);
        assertEquals(buildImageZip, buildInfo.getDeviceImageFile());
    }

    @Test
    public void testSetTestsDir() throws Exception {
        DeviceBuildInfo buildInfo = new DeviceBuildInfo();
        mLocalDeviceBuildProvider.setTestsDir(buildInfo);
        // No test dir specified, should be null
        assertNull(buildInfo.getTestsDir());
    }

    @Test
    public void testSetTestsDir_givenTestDir() throws Exception {
        DeviceBuildInfo buildInfo = new DeviceBuildInfo();
        File testFile = new File(mTestDir, "some-test");
        FileUtil.writeToFile("tests", testFile);
        mLocalDeviceBuildProvider.setTestDir(mTestDir);
        mLocalDeviceBuildProvider.setTestsDir(buildInfo);
        File testDir = buildInfo.getTestsDir();
        assertEquals(mTestDir, testDir);
        File extractedTestFile = new File(testDir, testFile.getName());
        assertTrue(extractedTestFile.exists());
        assertTrue(FileUtil.compareFileContents(testFile, extractedTestFile));
    }

    @Test
    public void testSetTestsDir_givenTestDirPattern() throws Exception {
        DeviceBuildInfo buildInfo = new DeviceBuildInfo();
        File testDir = FileUtil.createTempDir("test_dir", mBuildDir);
        mLocalDeviceBuildProvider.setTestDirPattern(testDir.getName());
        mLocalDeviceBuildProvider.setTestsDir(buildInfo);
        assertEquals(testDir, buildInfo.getTestsDir());
    }

    @Test
    public void testFindFileInDir() throws Exception {
        File fileFound = mLocalDeviceBuildProvider.findFileInDir(mAndroidInfo.getName());
        assertEquals(mAndroidInfo, fileFound);
    }

    @Test(expected = BuildRetrievalError.class)
    public void testFindFileInDir_multipleFiles() throws Exception {
        mLocalDeviceBuildProvider.findFileInDir(".*\\.img");
    }

    @Test
    public void testFindFileInDir_noneFound() throws Exception {
        File fileFound = mLocalDeviceBuildProvider.findFileInDir("this_should_not_exist");
        assertNull(fileFound);
    }
}
