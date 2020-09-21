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

package com.android.tradefed.result;

import com.android.tradefed.build.BuildInfo;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.StreamUtil;

import junit.framework.TestCase;

import org.easymock.EasyMock;

import java.io.BufferedInputStream;
import java.io.BufferedReader;
import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.zip.GZIPInputStream;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

/**
 * Unit tests for {@link LogFileSaver}.
 */
public class LogFileSaverTest extends TestCase {

    private File mRootDir;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mRootDir = FileUtil.createTempDir("tmpdir");
    }

    @Override
    protected void tearDown() throws Exception {
        FileUtil.recursiveDelete(mRootDir);
        super.tearDown();
    }

    /**
     * Test that a unique directory is created
     */
    public void testGetFileDir() {
        final String buildId = "88888";
        final String branch = "somebranch";
        final String testtag = "sometest";
        IBuildInfo mockBuild = EasyMock.createMock(IBuildInfo.class);
        EasyMock.expect(mockBuild.getBuildBranch()).andReturn(branch).anyTimes();
        EasyMock.expect(mockBuild.getBuildId()).andReturn(buildId).anyTimes();
        EasyMock.expect(mockBuild.getTestTag()).andReturn(testtag).anyTimes();
        EasyMock.replay(mockBuild);
        LogFileSaver saver = new LogFileSaver(mockBuild, mRootDir);
        File generatedDir = saver.getFileDir();
        File tagDir = generatedDir.getParentFile();
        // ensure a directory with name == testtag is parent of generated directory
        assertEquals(testtag, tagDir.getName());
        File buildDir = tagDir.getParentFile();
        // ensure a directory with name == build number is parent of generated directory
        assertEquals(buildId, buildDir.getName());
        // ensure a directory with name == branch is parent of generated directory
        File branchDir = buildDir.getParentFile();
        assertEquals(branch, branchDir.getName());
        // ensure parent directory is rootDir
        assertEquals(0, mRootDir.compareTo(branchDir.getParentFile()));

        // now create a new log saver,
        LogFileSaver newsaver = new LogFileSaver(mockBuild, mRootDir);
        File newgeneratedDir = newsaver.getFileDir();
        // ensure a new dir is created
        assertTrue(generatedDir.compareTo(newgeneratedDir) != 0);
        // verify tagDir is reused
        File newTagDir = newgeneratedDir.getParentFile();
        assertEquals(0, tagDir.compareTo(newTagDir));
    }

    /**
     * Test that a unique directory is created when no branch is specified
     */
    public void testGetFileDir_nobranch() {
        final String buildId = "88888";
        final String testtag = "sometest";
        IBuildInfo mockBuild = EasyMock.createMock(IBuildInfo.class);
        EasyMock.expect(mockBuild.getBuildBranch()).andReturn(null).anyTimes();
        EasyMock.expect(mockBuild.getBuildId()).andReturn(buildId).anyTimes();
        EasyMock.expect(mockBuild.getTestTag()).andReturn(testtag).anyTimes();
        EasyMock.replay(mockBuild);
        LogFileSaver saver = new LogFileSaver(mockBuild, mRootDir);
        File generatedDir = saver.getFileDir();
        File tagDir = generatedDir.getParentFile();
        // ensure a directory with name == testtag is parent of generated directory
        assertEquals(testtag, tagDir.getName());
        File buildDir = tagDir.getParentFile();
        // ensure a directory with name == build number is parent of generated directory
        assertEquals(buildId, buildDir.getName());
        // ensure parent directory is rootDir
        assertEquals(0, mRootDir.compareTo(buildDir.getParentFile()));
    }

    /**
     * Test that retention file creation
     */
    @SuppressWarnings("deprecation")
    public void testGetFileDir_retention() throws IOException, ParseException {
        final String buildId = "88888";
        final String branch = "somebranch";
        final String testtag = "sometest";
        IBuildInfo mockBuild = EasyMock.createMock(IBuildInfo.class);
        EasyMock.expect(mockBuild.getBuildBranch()).andReturn(branch).anyTimes();
        EasyMock.expect(mockBuild.getBuildId()).andReturn(buildId).anyTimes();
        EasyMock.expect(mockBuild.getTestTag()).andReturn(testtag).anyTimes();
        EasyMock.replay(mockBuild);
        LogFileSaver saver = new LogFileSaver(mockBuild, mRootDir, 1);
        File retentionFile = new File(saver.getFileDir(), RetentionFileSaver.RETENTION_FILE_NAME);
        assertTrue(retentionFile.isFile());
        String timestamp = StreamUtil.getStringFromStream(new FileInputStream(retentionFile));
        SimpleDateFormat formatter = new SimpleDateFormat(RetentionFileSaver.RETENTION_DATE_FORMAT);
        Date retentionDate = formatter.parse(timestamp);
        Date currentDate = new Date();
        int expectedDay = currentDate.getDay() == 6 ? 0 : currentDate.getDay() + 1;
        assertEquals(expectedDay, retentionDate.getDay());
    }

    /**
     * Simple normal case test for
     * {@link LogFileSaver#saveLogData(String, LogDataType, InputStream)}.
     */
    public void testSaveLogData() throws IOException {
        File logFile = null;
        BufferedReader logFileReader = null;
        try {
            // TODO: would be nice to create a mock file output to make this test not use disk I/O
            LogFileSaver saver = new LogFileSaver(new BuildInfo(), mRootDir);
            final String testData = "Here's some test data, blah";
            ByteArrayInputStream mockInput = new ByteArrayInputStream(testData.getBytes());
            logFile = saver.saveLogData("testSaveLogData", LogDataType.TEXT, mockInput);

            // Verify test data was written to file
            logFileReader = new BufferedReader(new FileReader(logFile));
            String actualLogString = logFileReader.readLine().trim();
            assertTrue(actualLogString.equals(testData));
        } finally {
            if (logFileReader != null) {
                logFileReader.close();
            }
            if (logFile != null) {
                logFile.delete();
            }
        }
    }

    /**
     * Simple normal case test for {@link LogFileSaver#saveAndGZipLogData}.
     */
    public void testSaveAndGZipLogData() throws IOException {
        File logFile = null;
        GZIPInputStream gzipStream = null;
        try {
            // TODO: would be nice to create a mock file output to make this test not use disk I/O
            LogFileSaver saver = new LogFileSaver(new BuildInfo(), mRootDir);
            final String testData = "Here's some test data, blah";
            ByteArrayInputStream mockInput = new ByteArrayInputStream(testData.getBytes());
            logFile = saver.saveAndGZipLogData("testSaveLogData", LogDataType.TEXT, mockInput);

            assertTrue(logFile.getName().endsWith(LogDataType.GZIP.getFileExt()));
            // Verify test data was written to file
            gzipStream = new GZIPInputStream(new FileInputStream(logFile));
            String actualLogString = StreamUtil.getStringFromStream(gzipStream);
            assertTrue(actualLogString.equals(testData));
        } finally {
            StreamUtil.close(gzipStream);
            FileUtil.deleteFile(logFile);
        }
    }

    /**
     * Simple normal case test for {@link LogFileSaver#saveAndZipLogData}.
     */
    public void testSaveAndZipLogData() throws IOException {
        File logFile = null;
        ZipFile zipFile = null;
        try {
            // TODO: would be nice to create a mock file output to make this test not use disk I/O
            LogFileSaver saver = new LogFileSaver(new BuildInfo(), mRootDir);
            final String testData = "Here's some test data, blah";
            ByteArrayInputStream mockInput = new ByteArrayInputStream(testData.getBytes());
            logFile = saver.saveAndZipLogData("testSaveLogData", LogDataType.TEXT, mockInput);

            assertTrue(logFile.getName().endsWith(LogDataType.ZIP.getFileExt()));
            // Verify test data was written to file
            zipFile = new ZipFile(logFile);

            String actualLogString = StreamUtil.getStringFromStream(zipFile.getInputStream(
                    new ZipEntry("testSaveLogData.txt")));
            assertTrue(actualLogString.equals(testData));
        } finally {
            if (zipFile != null) {
                zipFile.close();
            }
            FileUtil.deleteFile(logFile);
        }
    }

    /**
     * Simple normal case test for {@link LogFileSaver#createCompressedLogFile} and
     * {@link LogFileSaver#createGZipLogStream(File)}
     */
    public void testCreateAndGZipLogData() throws IOException {
        File logFile = null;
        OutputStream gzipOutStream = null;
        InputStream gzipInputStream = null;
        try {
            // TODO: would be nice to create a mock file output to make this test not use disk I/O
            LogFileSaver saver = new LogFileSaver(new BuildInfo(), mRootDir);
            logFile = saver.createCompressedLogFile("testSaveAndGZipLogData", LogDataType.TEXT);
            assertTrue(logFile.getName().endsWith(LogDataType.TEXT.getFileExt() + "." +
                    LogDataType.GZIP.getFileExt()));
            assertTrue(logFile.exists());

            // write data
            gzipOutStream = saver.createGZipLogStream(logFile);
            final String testData = "Here's some test data, blah";
            ByteArrayInputStream mockInput = new ByteArrayInputStream(testData.getBytes());
            StreamUtil.copyStreams(mockInput, gzipOutStream);
            StreamUtil.close(gzipOutStream);
            // Verify test data was written to file
            gzipInputStream = new GZIPInputStream(new BufferedInputStream(
                    new FileInputStream(logFile)));

            String actualLogString = StreamUtil.getStringFromStream(gzipInputStream);
            assertTrue(actualLogString.equals(testData));
        } finally {
            StreamUtil.close(gzipInputStream);
            FileUtil.deleteFile(logFile);
        }
    }

    public void testSaveLogDataRaw() throws Exception {
        File logFile = null;
        BufferedReader logFileReader = null;
        try {
            // TODO: would be nice to create a mock file output to make this test not use disk I/O
            LogFileSaver saver = new LogFileSaver(new BuildInfo(), mRootDir);
            final String testData = "Here's some test data, blah";
            ByteArrayInputStream mockInput = new ByteArrayInputStream(testData.getBytes());
            logFile = saver.saveLogDataRaw(
                    "testSaveLogData", LogDataType.TEXT.getFileExt(), mockInput);

            // Verify test data was written to file
            logFileReader = new BufferedReader(new FileReader(logFile));
            String actualLogString = logFileReader.readLine().trim();
            assertEquals(actualLogString, testData);
        } finally {
            StreamUtil.close(logFileReader);
            FileUtil.deleteFile(logFile);
        }
    }
}
