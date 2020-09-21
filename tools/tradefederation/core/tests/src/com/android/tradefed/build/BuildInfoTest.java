/*
 * Copyright (C) 2011 The Android Open Source Project
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
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.SerializationUtil;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.io.File;
import java.util.Arrays;

/** Unit tests for {@link BuildInfo}. */
@RunWith(JUnit4.class)
public class BuildInfoTest {
    private static final String VERSION = "2";
    private static final String ATTRIBUTE_KEY = "attribute";
    private static final String FILE_KEY = "file";

    private BuildInfo mBuildInfo;
    private File mFile;

    @Before
    public void setUp() throws Exception {
        mBuildInfo = new BuildInfo("1", "target");
        mBuildInfo.addBuildAttribute(ATTRIBUTE_KEY, "value");
        mFile = FileUtil.createTempFile("image", "tmp");
        FileUtil.writeToFile("filedata", mFile);
        mBuildInfo.setFile(FILE_KEY, mFile, VERSION);
    }

    @After
    public void tearDown() throws Exception {
        FileUtil.deleteFile(mFile);
    }

    /** Test method for {@link BuildInfo#clone()}. */
    @Test
    public void testClone() throws Exception {
        BuildInfo copy = (BuildInfo) mBuildInfo.clone();
        assertEquals(mBuildInfo.getBuildAttributes().get(ATTRIBUTE_KEY),
                copy.getBuildAttributes().get(ATTRIBUTE_KEY));
        try {
            // ensure a copy of mImageFile was created
            assertEquals(VERSION, copy.getVersion(FILE_KEY));
            assertTrue(!mFile.getAbsolutePath().equals(copy.getFile(FILE_KEY).getAbsolutePath()));
            assertTrue(FileUtil.compareFileContents(mFile, copy.getFile(FILE_KEY)));
        } finally {
            FileUtil.deleteFile(copy.getFile(FILE_KEY));
        }
    }

    /** Test method for {@link BuildInfo#cleanUp()}. */
    @Test
    public void testCleanUp() {
        assertTrue(mBuildInfo.getFile(FILE_KEY).exists());
        mBuildInfo.cleanUp();
        assertNull(mBuildInfo.getFile(FILE_KEY));
        assertFalse(mFile.exists());
    }

    /** Test for {@link BuildInfo#toString()}. */
    @Test
    public void testToString() {
        mBuildInfo.setBuildFlavor("testFlavor");
        mBuildInfo.setBuildBranch("testBranch");
        mBuildInfo.setDeviceSerial("fakeSerial");
        String expected = "BuildInfo{bid=1, target=target, build_flavor=testFlavor, "
                + "branch=testBranch, serial=fakeSerial}";
        assertEquals(expected, mBuildInfo.toString());
    }

    /** Test for {@link BuildInfo#toString()} when a build alias is present. */
    @Test
    public void testToString_withBuildAlias() {
        mBuildInfo.addBuildAttribute("build_alias", "NMR12");
        mBuildInfo.setBuildFlavor("testFlavor");
        mBuildInfo.setBuildBranch("testBranch");
        mBuildInfo.setDeviceSerial("fakeSerial");
        String expected = "BuildInfo{build_alias=NMR12, bid=1, target=target, "
                + "build_flavor=testFlavor, branch=testBranch, serial=fakeSerial}";
        assertEquals(expected, mBuildInfo.toString());
    }

    /**
     * Test that all the components of {@link BuildInfo} can be serialized via the default java
     * object serialization.
     */
    @Test
    public void testSerialization() throws Exception {
        File tmpSerialized = SerializationUtil.serialize(mBuildInfo);
        Object o = SerializationUtil.deserialize(tmpSerialized, true);
        assertTrue(o instanceof BuildInfo);
        BuildInfo test = (BuildInfo) o;
        // use the custom build info equals to check similar properties
        assertTrue(mBuildInfo.equals(test));
    }

    /**
     * Test {@link BuildInfo#cleanUp(java.util.List)} to ensure it removes non-existing files and
     * lives others.
     */
    @Test
    public void testCleanUpWithExemption() throws Exception {
        File testFile = FileUtil.createTempFile("fake-versioned-file", ".txt");
        File testFile2 = FileUtil.createTempFile("fake-versioned-file2", ".txt");
        try {
            mBuildInfo.setFile("name", testFile, "version");
            mBuildInfo.setFile("name2", testFile2, "version2");
            assertNotNull(mBuildInfo.getFile("name"));
            assertNotNull(mBuildInfo.getFile("name2"));
            assertNotNull(mBuildInfo.getFile("name"));
            assertNotNull(mBuildInfo.getFile("name2"));
            // Clean up with an exception on one of the file
            mBuildInfo.cleanUp(Arrays.asList(testFile2));
            assertNull(mBuildInfo.getFile("name"));
            // The second file still exists and is left untouched.
            assertNotNull(mBuildInfo.getFile("name2"));
        } finally {
            FileUtil.deleteFile(testFile);
            FileUtil.deleteFile(testFile2);
        }
    }
}
