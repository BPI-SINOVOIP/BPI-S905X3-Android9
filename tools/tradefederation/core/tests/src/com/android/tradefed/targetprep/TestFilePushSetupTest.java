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

import com.google.common.io.Files;

import com.android.tradefed.build.BuildInfo;
import com.android.tradefed.build.DeviceBuildInfo;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.util.FakeTestsZipFolder;
import com.android.tradefed.util.FakeTestsZipFolder.ItemType;
import com.android.tradefed.util.FileUtil;

import junit.framework.TestCase;

import org.easymock.EasyMock;
import org.easymock.IAnswer;

import java.io.File;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * A test for {@link TestFilePushSetup}.
 */
public class TestFilePushSetupTest extends TestCase {

    private Map<String, ItemType> mFiles;
    private List<String> mDeviceLocationList;
    private FakeTestsZipFolder mFakeTestsZipFolder;
    private File mAltDirFile1, mAltDirFile2;
    private static final String ALT_FILENAME1 = "foobar";
    private static final String ALT_FILENAME2 = "barfoo";

    private ITestDevice mMockDevice;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mFiles = new HashMap<String, ItemType>();
        mFiles.put("app/AndroidCommonTests.apk", ItemType.FILE);
        mFiles.put("app/GalleryTests.apk", ItemType.FILE);
        mFiles.put("testinfo", ItemType.DIRECTORY);
        mFiles.put(ALT_FILENAME1, ItemType.FILE);
        mFakeTestsZipFolder = new FakeTestsZipFolder(mFiles);
        assertTrue(mFakeTestsZipFolder.createItems());
        mDeviceLocationList = new ArrayList<String>();
        for (String file : mFiles.keySet()) {
            mDeviceLocationList.add(TestFilePushSetup.getDevicePathFromUserData(file));
        }
        File tmpBase = Files.createTempDir();
        mAltDirFile1 = new File(tmpBase, ALT_FILENAME1);
        assertTrue("failed to create temp file", mAltDirFile1.createNewFile());
        mAltDirFile2 = new File(tmpBase, ALT_FILENAME2);
        assertTrue("failed to create temp file", mAltDirFile2.createNewFile());
        mMockDevice = EasyMock.createMock(ITestDevice.class);
        EasyMock.expect(mMockDevice.getSerialNumber()).andStubReturn("SERIAL");
    }

    @Override
    protected void tearDown() throws Exception {
        mFakeTestsZipFolder.cleanUp();
        File tmpDir = mAltDirFile1.getParentFile();
        FileUtil.deleteFile(mAltDirFile1);
        FileUtil.deleteFile(mAltDirFile2);
        FileUtil.recursiveDelete(tmpDir);
        super.tearDown();
    }

    public void testSetup() throws TargetSetupError, BuildError, DeviceNotAvailableException {
        TestFilePushSetup testFilePushSetup = new TestFilePushSetup();
        DeviceBuildInfo stubBuild = new DeviceBuildInfo("0", "stub");
        stubBuild.setTestsDir(mFakeTestsZipFolder.getBasePath(), "0");
        assertFalse(mFiles.isEmpty());
        assertFalse(mDeviceLocationList.isEmpty());
        ITestDevice device = EasyMock.createMock(ITestDevice.class);
        EasyMock.expect(device.pushDir((File) EasyMock.anyObject(), (String) EasyMock.anyObject()))
                .andAnswer(new IAnswer<Boolean>() {
                    @Override
                    public Boolean answer() throws Throwable {
                        return mDeviceLocationList.remove(EasyMock.getCurrentArguments()[1]);
                    }
                });
        EasyMock.expect(device.pushFile((File) EasyMock.anyObject(),
                (String) EasyMock.anyObject())).andAnswer(new IAnswer<Boolean>() {
                    @Override
                    public Boolean answer() throws Throwable {
                        return mDeviceLocationList.remove(EasyMock.getCurrentArguments()[1]);
                    }
                }).times(3);
        EasyMock.expect(device.executeShellCommand((String) EasyMock.anyObject()))
                .andReturn("").times(4);
        EasyMock.replay(device);
        for (String file : mFiles.keySet()) {
            testFilePushSetup.addTestFileName(file);
        }
        testFilePushSetup.setUp(device, stubBuild);
        assertTrue(mDeviceLocationList.isEmpty());
        EasyMock.verify(device);
    }

    /**
     * Test that setup throws an exception if provided with something else than DeviceBuildInfo
     */
    public void testSetup_notDeviceBuildInfo() throws Exception {
        TestFilePushSetup testFilePushSetup = new TestFilePushSetup();
        BuildInfo stubBuild = new BuildInfo("stub", "stub");
        try {
            testFilePushSetup.setUp(EasyMock.createMock(ITestDevice.class), stubBuild);
            fail("should have thrown an exception");
        } catch (IllegalArgumentException expected) {
            assertEquals("Provided buildInfo is not a com.android.tradefed.build.IDeviceBuildInfo",
                    expected.getMessage());
        }
    }

    /**
     * Test that the artifact path resolution logic correctly uses alt dir as override
     */
    public void testAltDirOverride() throws Exception {
        TestFilePushSetup setup = new TestFilePushSetup();
        setup.setAltDirBehavior(AltDirBehavior.OVERRIDE);
        DeviceBuildInfo info = new DeviceBuildInfo();
        info.setTestsDir(mFakeTestsZipFolder.getBasePath(), "0");
        setup.setAltDir(mAltDirFile1.getParentFile());
        File apk = setup.getLocalPathForFilename(info, ALT_FILENAME1, mMockDevice);
        assertEquals(mAltDirFile1.getAbsolutePath(), apk.getAbsolutePath());
    }

    /**
     * Test that the artifact path resolution logic correctly throws if alt dir behavior is invalid
     */
    public void testAltDirOverride_null() throws Exception {
        TestFilePushSetup setup = new TestFilePushSetup();
        setup.setAltDirBehavior(null);
        DeviceBuildInfo info = new DeviceBuildInfo();
        try {
            setup.getLocalPathForFilename(info, ALT_FILENAME1, mMockDevice);
            fail("Should have thrown an exception");
        } catch (TargetSetupError expected) {
            assertEquals("Missing handler for alt-dir-behavior: null null", expected.getMessage());
        }
    }

    /**
     * Test that the artifact path resolution logic correctly throws if alt dir is empty.
     */
    public void testAltDirOverride_empty() throws Exception {
        TestFilePushSetup setup = new TestFilePushSetup();
        setup.setAltDirBehavior(AltDirBehavior.OVERRIDE);
        DeviceBuildInfo info = new DeviceBuildInfo();
        try {
            setup.getLocalPathForFilename(info, ALT_FILENAME1, mMockDevice);
            fail("Should have thrown an exception");
        } catch (TargetSetupError expected) {
            assertEquals("Provided buildInfo does not contain a valid tests directory and no "
                    + "alternative directories were provided null", expected.getMessage());
        }
    }

    /**
     * Test that the artifact path resolution logic correctly favors the one in test dir
     */
    public void testAltDirNoFallback() throws Exception {
        TestFilePushSetup setup = new TestFilePushSetup();
        DeviceBuildInfo info = new DeviceBuildInfo();
        info.setTestsDir(mFakeTestsZipFolder.getBasePath(), "0");
        setup.setAltDir(mAltDirFile1.getParentFile());
        File apk = setup.getLocalPathForFilename(info, ALT_FILENAME1, mMockDevice);
        File apkInTestDir = new File(
                new File(mFakeTestsZipFolder.getBasePath(), "DATA"), ALT_FILENAME1);
        assertEquals(apkInTestDir.getAbsolutePath(), apk.getAbsolutePath());
    }

    /**
     * Test that the artifact path resolution logic correctly falls back to alt dir
     */
    public void testAltDirFallback() throws Exception {
        TestFilePushSetup setup = new TestFilePushSetup();
        DeviceBuildInfo info = new DeviceBuildInfo();
        info.setTestsDir(mFakeTestsZipFolder.getBasePath(), "0");
        setup.setAltDir(mAltDirFile2.getParentFile());
        File apk = setup.getLocalPathForFilename(info, ALT_FILENAME2, mMockDevice);
        assertEquals(mAltDirFile2.getAbsolutePath(), apk.getAbsolutePath());
    }

    /**
     * Test that an exception is thrown if the file doesn't exist in extracted test dir
     */
    public void testThrowIfNotFound() throws Exception {
        TestFilePushSetup setup = new TestFilePushSetup();
        setup.setThrowIfNoFile(true);
        // Assuming that the "file-not-in-test-zip" file doesn't exist in the test zips folder.
        setup.addTestFileName("file-not-in-test-zip");
        DeviceBuildInfo stubBuild = new DeviceBuildInfo("0", "stub");
        stubBuild.setTestsDir(mFakeTestsZipFolder.getBasePath(), "0");
        try {
            setup.setUp(mMockDevice, stubBuild);
            fail("Should have thrown an exception");
        } catch (TargetSetupError expected) {
            assertEquals(
                    "Could not find test file file-not-in-test-zip "
                            + "directory in extracted tests.zip null",
                    expected.getMessage());
        }
    }

    /**
     * Test that no exception is thrown if the file doesn't exist in extracted test dir
     * given that the option "throw-if-not-found" is set to false.
     */
    public void testThrowIfNotFound_false() throws Exception {
        TestFilePushSetup setup = new TestFilePushSetup();
        setup.setThrowIfNoFile(false);
        // Assuming that the "file-not-in-test-zip" file doesn't exist in the test zips folder.
        setup.addTestFileName("file-not-in-test-zip");
        DeviceBuildInfo stubBuild = new DeviceBuildInfo("0", "stub");
        stubBuild.setTestsDir(mFakeTestsZipFolder.getBasePath(), "0");
        // test that it does not throw
        setup.setUp(mMockDevice, stubBuild);
    }
}
