/*
 * Copyright (C) 2010 The Android Open Source Project
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
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.LogDataType;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.nio.file.attribute.PosixFilePermission;
import java.util.Set;

/** Unit tests for {@link FileUtil} */
@RunWith(JUnit4.class)
public class FileUtilTest {

    @Before
    public void setUp() throws Exception {
        FileUtil.setChmodBinary("chmod");
    }

    /** test {@link FileUtil#getExtension(String)} */
    @Test
    public void testGetExtension() {
        assertEquals("", FileUtil.getExtension("filewithoutext"));
        assertEquals(".txt", FileUtil.getExtension("file.txt"));
        assertEquals(".txt", FileUtil.getExtension("foo.file.txt"));
    }

    /** test {@link FileUtil#chmodGroupRW(File)} on a system that supports 'chmod' */
    @Test
    public void testChmodGroupRW() throws IOException {
        File testFile = null;
        try {
            if (!FileUtil.chmodExists()) {
                CLog.d("Chmod not available, skipping the test");
                return;
            }
            testFile = FileUtil.createTempFile("testChmodRW", ".txt");
            assertTrue(FileUtil.chmodGroupRW(testFile));
            assertTrue(testFile.canRead());
            assertTrue(testFile.canWrite());
            assertFalse(testFile.canExecute());
        } finally {
            FileUtil.deleteFile(testFile);
        }
    }

    /**
     * test {@link FileUtil#chmodGroupRW(File)} on a system that does not supports 'chmod'. File
     * permission should still be set with the fallback.
     */
    @Test
    public void testChmodGroupRW_noChmod() throws IOException {
        File testFile = null;
        FileUtil.setChmodBinary("fake_not_existing_chmod");
        try {
            testFile = FileUtil.createTempFile("testChmodRW", ".txt");
            assertTrue(FileUtil.chmodGroupRW(testFile));
            assertTrue(testFile.canRead());
            assertTrue(testFile.canWrite());
            assertFalse(testFile.canExecute());
        } finally {
            FileUtil.deleteFile(testFile);
        }
    }

    /** test {@link FileUtil#chmodGroupRWX(File)} on a system that supports 'chmod' */
    @Test
    public void testChmodGroupRWX() throws IOException {
        File testFile = null;
        try {
            if (!FileUtil.chmodExists()) {
                CLog.d("Chmod not available, skipping the test");
                return;
            }
            testFile = FileUtil.createTempFile("testChmodRWX", ".txt");
            assertTrue(FileUtil.chmodGroupRWX(testFile));
            assertTrue(testFile.canRead());
            assertTrue(testFile.canWrite());
            assertTrue(testFile.canExecute());
        } finally {
            FileUtil.deleteFile(testFile);
        }
    }

    /**
     * test {@link FileUtil#chmodGroupRWX(File)} on a system that does not supports 'chmod'. File
     * permission should still be set with the fallback.
     */
    @Test
    public void testChmodGroupRWX_noChmod() throws IOException {
        File testFile = null;
        FileUtil.setChmodBinary("fake_not_existing_chmod");
        try {
            testFile = FileUtil.createTempFile("testChmodRWX", ".txt");
            assertTrue(FileUtil.chmodGroupRWX(testFile));
            assertTrue(testFile.canRead());
            assertTrue(testFile.canWrite());
            assertTrue(testFile.canExecute());
        } finally {
            FileUtil.deleteFile(testFile);
        }
    }

    /**
     * test {@link FileUtil#createTempFile(String, String)} with a very long file name. FileSystem
     * should not throw any exception.
     */
    @Test
    public void testCreateTempFile_filenameTooLong() throws IOException {
        File testFile = null;
        try {
            final String prefix = "logcat-android.support.v7.widget.GridLayoutManagerWrapContent"
                    + "WithAspectRatioTest_testAllChildrenWrapContentInOtherDirection_"
                    + "WrapContentConfig_unlimitedWidth=true_ unlimitedHeight=true_padding="
                    + "Rect(0_0-0_0)_Config_mSpanCount=3_ mOrientation=v_mItemCount=1000_"
                    + "mReverseLayout=false_ 8__";
            testFile = FileUtil.createTempFile(prefix, ".gz");
            assertTrue(testFile.getName().length() <= FileUtil.FILESYSTEM_FILENAME_MAX_LENGTH);
        } finally {
            FileUtil.deleteFile(testFile);
        }
    }

    /**
     * test {@link FileUtil#createTempFile(String, String)} with a very long file name. FileSystem
     * should not throw any exception. If both suffix is smaller than overflow length, it will be
     * completely truncated, and prefix will truncate the remaining.
     */
    @Test
    public void testCreateTempFile_filenameTooLongEdge() throws IOException {
        File testFile = null;
        try {
            final String prefix = "logcat-android.support.v7.widget.GridLayoutManagerWrapContent"
                    + "WithAspectRatioTest_testAllChildrenWrapContentInOtherDirection_"
                    + "WrapContentConfig_unlimitedWidth=true_ unlimitedHeight=true_padding="
                    + "Rect(0_0-0_0)_Config_mSpanCount=3_ mOrientation";
            final String suffix = "logcat-android.support.v7.widget.GridLayoutManagerWrapContent"
                    + "WithAspectRatioTest_testAllChildrenWrapContentInOtherDirection_"
                    + "WrapContentConfig_unlimitedWidth=true_ unlimitedHeight=true_padding="
                    + "Rect(0_0-0_0)_Config_mSpanCount=3_ mOrientat";
            testFile = FileUtil.createTempFile(prefix, suffix);
            assertTrue(testFile.getName().length() <= FileUtil.FILESYSTEM_FILENAME_MAX_LENGTH);
        } finally {
            FileUtil.deleteFile(testFile);
        }
    }

    /**
     * Test {@link FileUtil#writeToFile(InputStream, File, boolean)} succeeds overwriting an
     * existent file.
     */
    @Test
    public void testWriteToFile_overwrites_exists() throws IOException {
        File testFile = null;
        try {
            testFile = File.createTempFile("doesnotmatter", ".txt");
            FileUtil.writeToFile(new ByteArrayInputStream("write1".getBytes()), testFile, false);
            assertEquals(FileUtil.readStringFromFile(testFile), "write1");
            FileUtil.writeToFile(new ByteArrayInputStream("write2".getBytes()), testFile, false);
            assertEquals(FileUtil.readStringFromFile(testFile), "write2");
        } finally {
            FileUtil.deleteFile(testFile);
        }
    }

    /**
     * Test {@link FileUtil#writeToFile(InputStream, File, boolean)} succeeds appending to an
     * existent file.
     */
    @Test
    public void testWriteToFile_appends_exists() throws IOException {
        File testFile = null;
        try {
            testFile = File.createTempFile("doesnotmatter", ".txt");
            FileUtil.writeToFile(new ByteArrayInputStream("write1".getBytes()), testFile, true);
            FileUtil.writeToFile(new ByteArrayInputStream("write2".getBytes()), testFile, true);
            assertEquals(FileUtil.readStringFromFile(testFile), "write1write2");
        } finally {
            FileUtil.deleteFile(testFile);
        }
    }

    /**
     * Test {@link FileUtil#writeToFile(InputStream, File, boolean)} succeeds writing to an
     * uncreated file.
     */
    @Test
    public void testWriteToFile_overwrites_doesNotExist() throws IOException {
        File testFile = null;
        try {
            testFile = FileUtil.createTempFile("fileutiltest", ".test");
            FileUtil.deleteFile(testFile);
            FileUtil.writeToFile(new ByteArrayInputStream("write1".getBytes()), testFile, false);
            assertEquals("write1", FileUtil.readStringFromFile(testFile));
        } finally {
            FileUtil.deleteFile(testFile);
        }
    }

    /**
     * Test {@link FileUtil#writeToFile(InputStream, File, boolean)} succeeds appending to an
     * uncreated file.
     */
    @Test
    public void testWriteToFile_appends_doesNotExist() throws IOException {
        File testFile = null;
        try {
            testFile = FileUtil.createTempFile("fileutiltest", ".test");
            FileUtil.deleteFile(testFile);
            FileUtil.writeToFile(new ByteArrayInputStream("write1".getBytes()), testFile, true);
            assertEquals("write1", FileUtil.readStringFromFile(testFile));
        } finally {
            FileUtil.deleteFile(testFile);
        }
    }

    /** Test {@link FileUtil#writeToFile(InputStream, File)} succeeds overwriting to a file. */
    @Test
    public void testWriteToFile_stream_overwrites() throws IOException {
        File testFile = null;
        try {
            testFile = File.createTempFile("doesnotmatter", ".txt");
            FileUtil.writeToFile(new ByteArrayInputStream("write1".getBytes()), testFile);
            assertEquals(FileUtil.readStringFromFile(testFile), "write1");
            FileUtil.writeToFile(new ByteArrayInputStream("write2".getBytes()), testFile);
            assertEquals(FileUtil.readStringFromFile(testFile), "write2");
        } finally {
            FileUtil.deleteFile(testFile);
        }
    }

    /** Test {@link FileUtil#writeToFile(String, File, boolean)} succeeds overwriting to a file. */
    @Test
    public void testWriteToFile_string_overwrites() throws IOException {
        File testFile = null;
        try {
            testFile = File.createTempFile("doesnotmatter", ".txt");
            FileUtil.writeToFile("write1", testFile, false);
            assertEquals(FileUtil.readStringFromFile(testFile), "write1");
            FileUtil.writeToFile("write2", testFile, false);
            assertEquals(FileUtil.readStringFromFile(testFile), "write2");
        } finally {
            FileUtil.deleteFile(testFile);
        }
    }

    /** Test {@link FileUtil#writeToFile(String, File, boolean)} succeeds appending to a file. */
    @Test
    public void testWriteToFile_string_appends() throws IOException {
        File testFile = null;
        try {
            testFile = File.createTempFile("doesnotmatter", ".txt");
            FileUtil.writeToFile("write1", testFile, true);
            FileUtil.writeToFile("write2", testFile, true);
            assertEquals(FileUtil.readStringFromFile(testFile), "write1write2");
        } finally {
            FileUtil.deleteFile(testFile);
        }
    }

    /** Test {@link FileUtil#writeToFile(String, File)} succeeds overwriting to a file. */
    @Test
    public void testWriteToFile_string_defaultOverwrites() throws IOException {
        File testFile = null;
        try {
            testFile = File.createTempFile("doesnotmatter", ".txt");
            FileUtil.writeToFile("write1", testFile);
            assertEquals(FileUtil.readStringFromFile(testFile), "write1");
            FileUtil.writeToFile("write2", testFile);
            assertEquals(FileUtil.readStringFromFile(testFile), "write2");
        } finally {
            FileUtil.deleteFile(testFile);
        }
    }

    /** Test {@link FileUtil#unixModeToPosix(int)} returns expected results; */
    @Test
    public void testUnixModeToPosix() {
        Set<PosixFilePermission> perms = null;
        // can't test all 8 * 8 * 8, so just a select few
        perms = FileUtil.unixModeToPosix(0777);
        assertTrue("failed unix mode conversion: 0777",
                perms.remove(PosixFilePermission.OWNER_READ) &&
                perms.remove(PosixFilePermission.OWNER_WRITE) &&
                perms.remove(PosixFilePermission.OWNER_EXECUTE) &&
                perms.remove(PosixFilePermission.GROUP_READ) &&
                perms.remove(PosixFilePermission.GROUP_WRITE) &&
                perms.remove(PosixFilePermission.GROUP_EXECUTE) &&
                perms.remove(PosixFilePermission.OTHERS_READ) &&
                perms.remove(PosixFilePermission.OTHERS_WRITE) &&
                perms.remove(PosixFilePermission.OTHERS_EXECUTE) &&
                perms.isEmpty());
        perms = FileUtil.unixModeToPosix(0644);
        assertTrue("failed unix mode conversion: 0644",
                perms.remove(PosixFilePermission.OWNER_READ) &&
                perms.remove(PosixFilePermission.OWNER_WRITE) &&
                perms.remove(PosixFilePermission.GROUP_READ) &&
                perms.remove(PosixFilePermission.OTHERS_READ) &&
                perms.isEmpty());
        perms = FileUtil.unixModeToPosix(0755);
        assertTrue("failed unix mode conversion: 0755",
                perms.remove(PosixFilePermission.OWNER_READ) &&
                perms.remove(PosixFilePermission.OWNER_WRITE) &&
                perms.remove(PosixFilePermission.OWNER_EXECUTE) &&
                perms.remove(PosixFilePermission.GROUP_READ) &&
                perms.remove(PosixFilePermission.GROUP_EXECUTE) &&
                perms.remove(PosixFilePermission.OTHERS_READ) &&
                perms.remove(PosixFilePermission.OTHERS_EXECUTE) &&
                perms.isEmpty());
    }

    /** Test {@link FileUtil#findFiles(File, String)} can find files successfully. */
    @Test
    public void testFindFilesSuccess() throws IOException {
        File tmpDir = FileUtil.createTempDir("find_files_test");
        try {
            File matchFile1 = FileUtil.createTempFile("test", ".config", tmpDir);
            File subDir = new File(tmpDir, "subfolder");
            subDir.mkdirs();
            File matchFile2 = FileUtil.createTempFile("test", ".config", subDir);
            FileUtil.createTempFile("test", ".xml", subDir);
            Set<String> matchFiles = FileUtil.findFiles(tmpDir, ".*.config");
            assertTrue(matchFiles.contains(matchFile1.getAbsolutePath()));
            assertTrue(matchFiles.contains(matchFile2.getAbsolutePath()));
            assertEquals(matchFiles.size(), 2);
        } finally {
            FileUtil.recursiveDelete(tmpDir);
        }
    }

    /**
     * Test that using {@link FileUtil#findFiles(File, String)} works when one of the directory is a
     * symlink.
     */
    @Test
    public void testFindFilesSuccess_symlink() throws IOException {
        File tmpDir = FileUtil.createTempDir("find_files_test");
        File subDir = FileUtil.createTempDir("subfolder");
        try {
            File matchFile1 = FileUtil.createTempFile("test", ".config", tmpDir);
            File matchFile2 = FileUtil.createTempFile("test", ".config", subDir);
            File destLink = new File(tmpDir, "subFolder");
            FileUtil.symlinkFile(subDir, destLink);
            FileUtil.createTempFile("test", ".xml", subDir);
            Set<String> matchFiles = FileUtil.findFiles(tmpDir, ".*.config");
            assertTrue(matchFiles.contains(matchFile1.getAbsolutePath()));
            // The file is returned under the directory of the symlink not the original location.
            assertTrue(
                    matchFiles.contains(
                            new File(destLink, matchFile2.getName()).getAbsolutePath()));
            assertEquals(matchFiles.size(), 2);
        } finally {
            FileUtil.recursiveDelete(tmpDir);
            FileUtil.recursiveDelete(subDir);
        }
    }

    /**
     * Test {@link FileUtil#findFiles(File, String)} returns empty set if no file matches filter.
     */
    @Test
    public void testFindFilesFail() throws IOException {
        File tmpDir = FileUtil.createTempDir("find_files_test");
        try {
            FileUtil.createTempFile("test", ".config", tmpDir);
            File subDir = new File(tmpDir, "subfolder");
            subDir.mkdirs();
            FileUtil.createTempFile("test", ".config", subDir);
            Set<String> matchFiles = FileUtil.findFiles(tmpDir, ".*.config_x");
            assertEquals(matchFiles.size(), 0);
        } finally {
            FileUtil.recursiveDelete(tmpDir);
        }
    }

    @Test
    public void testGetContentType_text() throws Exception {
        assertEquals(LogDataType.TEXT.getContentType(),
                FileUtil.getContentType("path/to/file.txt"));
    }

    @Test
    public void testGetContentType_html() throws Exception {
        assertEquals(LogDataType.HTML.getContentType(),
                FileUtil.getContentType("path/to/file.html"));
    }

    @Test
    public void testGetContentType_png() throws Exception {
        assertEquals(LogDataType.PNG.getContentType(),
                FileUtil.getContentType("path/to/file.png"));
    }

    /** Test {@link FileUtil#findFile(File, String)} when finding a file. */
    @Test
    public void testFindFile() throws IOException {
        File tmpDir = FileUtil.createTempDir("find_file_test");
        try {
            File subDir = FileUtil.createTempDir("sub_find_file_test", tmpDir);
            File subFile = FileUtil.createTempFile("find_file_file", ".txt", subDir);
            File res = FileUtil.findFile(tmpDir, subFile.getName());
            assertEquals(subFile.getAbsolutePath(), res.getAbsolutePath());
        } finally {
            FileUtil.recursiveDelete(tmpDir);
        }
    }

    /**
     * Test {@link FileUtil#findFile(File, String)} when finding a file that has the same name as
     * its directory, the File should be be returned in that case.
     */
    @Test
    public void testFindFile_sameDirName() throws IOException {
        File tmpDir = FileUtil.createTempDir("find_file_test");
        try {
            File subDir = FileUtil.createTempDir("find_file_file", tmpDir);
            File subFile = FileUtil.createTempFile("find_file_file", "", subDir);
            File res = FileUtil.findFile(tmpDir, subFile.getName());
            assertEquals(subFile.getAbsolutePath(), res.getAbsolutePath());
        } finally {
            FileUtil.recursiveDelete(tmpDir);
        }
    }

    /**
     * Test {@link FileUtil#findFile(File, String)} when searching a File, if a directory match the
     * name, and not child file does, then return the directory matching the file.
     */
    @Test
    public void testFindFile_directory() throws IOException {
        File tmpDir = FileUtil.createTempDir("find_file_test");
        try {
            File subDir = FileUtil.createTempDir("find_file_file", tmpDir);
            FileUtil.createTempFile("sub_file_file", ".txt", subDir);
            File res = FileUtil.findFile(tmpDir, subDir.getName());
            assertEquals(subDir.getAbsolutePath(), res.getAbsolutePath());
        } finally {
            FileUtil.recursiveDelete(tmpDir);
        }
    }

    /**
     * Test {@link FileUtil#findDirsUnder(File, File)} when root dir is not a directory, it should
     * throw an exception.
     */
    @Test
    public void testFindDirsUnder_exception() throws IOException {
        File illegalRoot = FileUtil.createTempFile("find_under", ".txt");
        try {
            FileUtil.findDirsUnder(illegalRoot, null);
            fail("Should have thrown an exception.");
        } catch (IllegalArgumentException expected) {
            // expected
        } finally {
            FileUtil.recursiveDelete(illegalRoot);
        }
    }
}
