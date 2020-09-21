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

import static org.junit.Assert.*;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.invoker.InvocationContext;
import com.android.tradefed.invoker.ShardMasterResultForwarder;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.StreamUtil;

import org.easymock.EasyMock;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.io.BufferedReader;
import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

/**
 * Unit tests for {@link FileSystemLogSaver}.
 *
 * <p>Depends on filesyste I/O.
 */
@RunWith(JUnit4.class)
public class FileSystemLogSaverTest {
    private static final String BUILD_ID = "88888";
    private static final String BRANCH = "somebranch";
    private static final String TEST_TAG = "sometest";

    private File mReportDir;
    private IBuildInfo mMockBuild;
    private IInvocationContext mContext;

    @Before
    public void setUp() throws Exception {
        mReportDir = FileUtil.createTempDir("tmpdir");

        mMockBuild = EasyMock.createMock(IBuildInfo.class);
        EasyMock.expect(mMockBuild.getBuildBranch()).andReturn(BRANCH).anyTimes();
        EasyMock.expect(mMockBuild.getBuildId()).andReturn(BUILD_ID).anyTimes();
        EasyMock.expect(mMockBuild.getTestTag()).andReturn(TEST_TAG).anyTimes();
        EasyMock.replay(mMockBuild);
        mContext = new InvocationContext();
        mContext.addDeviceBuildInfo("fakeDevice", mMockBuild);
        mContext.setTestTag(TEST_TAG);
    }

    @After
    public void tearDown() throws Exception {
        FileUtil.recursiveDelete(mReportDir);
    }

    /** Test that a unique directory is created */
    @Test
    public void testGetFileDir() {
        FileSystemLogSaver saver = new FileSystemLogSaver();
        saver.setReportDir(mReportDir);
        saver.invocationStarted(mContext);

        File generatedDir = new File(saver.getLogReportDir().getPath());
        File tagDir = generatedDir.getParentFile();
        // ensure a directory with name == testtag is parent of generated directory
        assertEquals(TEST_TAG, tagDir.getName());
        File buildDir = tagDir.getParentFile();
        // ensure a directory with name == build number is parent of generated directory
        assertEquals(BUILD_ID, buildDir.getName());
        // ensure a directory with name == branch is parent of generated directory
        File branchDir = buildDir.getParentFile();
        assertEquals(BRANCH, branchDir.getName());
        // ensure parent directory is rootDir
        assertEquals(0, mReportDir.compareTo(branchDir.getParentFile()));

        // now create a new log saver,
        FileSystemLogSaver newSaver = new FileSystemLogSaver();
        newSaver.setReportDir(mReportDir);
        newSaver.invocationStarted(mContext);

        File newGeneratedDir = new File(newSaver.getLogReportDir().getPath());
        // ensure a new dir is created
        assertTrue(generatedDir.compareTo(newGeneratedDir) != 0);
        // verify tagDir is reused
        File newTagDir = newGeneratedDir.getParentFile();
        assertEquals(0, tagDir.compareTo(newTagDir));
    }

    /** Test that a unique directory is created when no branch is specified */
    @Test
    public void testGetFileDir_nobranch() {
        IBuildInfo mockBuild = EasyMock.createMock(IBuildInfo.class);
        EasyMock.expect(mockBuild.getBuildBranch()).andReturn(null).anyTimes();
        EasyMock.expect(mockBuild.getBuildId()).andReturn(BUILD_ID).anyTimes();
        EasyMock.expect(mockBuild.getTestTag()).andReturn(TEST_TAG).anyTimes();
        EasyMock.replay(mockBuild);
        IInvocationContext context = new InvocationContext();
        context.addDeviceBuildInfo("fakeDevice", mockBuild);
        FileSystemLogSaver saver = new FileSystemLogSaver();
        saver.setReportDir(mReportDir);
        saver.invocationStarted(context);

        File generatedDir = new File(saver.getLogReportDir().getPath());
        File tagDir = generatedDir.getParentFile();
        // ensure a directory with name == testtag is parent of generated directory
        assertEquals(TEST_TAG, tagDir.getName());
        File buildDir = tagDir.getParentFile();
        // ensure a directory with name == build number is parent of generated directory
        assertEquals(BUILD_ID, buildDir.getName());
        // ensure parent directory is rootDir
        assertEquals(0, mReportDir.compareTo(buildDir.getParentFile()));
    }

    /** Test that retention file creation */
    @Test
    @SuppressWarnings("deprecation")
    public void testGetFileDir_retention() throws IOException, ParseException {
        FileSystemLogSaver saver = new FileSystemLogSaver();
        saver.setReportDir(mReportDir);
        saver.setLogRetentionDays(1);
        saver.invocationStarted(mContext);

        File retentionFile = new File(new File(saver.getLogReportDir().getPath()),
                RetentionFileSaver.RETENTION_FILE_NAME);
        assertTrue(retentionFile.isFile());
        String timestamp = StreamUtil.getStringFromStream(new FileInputStream(retentionFile));
        SimpleDateFormat formatter = new SimpleDateFormat(RetentionFileSaver.RETENTION_DATE_FORMAT);
        Date retentionDate = formatter.parse(timestamp);
        Date currentDate = new Date();
        int expectedDay = (currentDate.getDay() + 1) % 7;
        assertEquals(expectedDay, retentionDate.getDay());
    }

    /**
     * Test saving uncompressed data for {@link FileSystemLogSaver#saveLogData(String, LogDataType,
     * InputStream)}.
     */
    @Test
    public void testSaveLogData_uncompressed() throws IOException {
        LogFile logFile = null;
        ZipFile zipFile = null;
        BufferedReader logFileReader = null;
        try {
            // TODO: would be nice to create a mock file output to make this test not use disk I/O
            FileSystemLogSaver saver = new FileSystemLogSaver();
            saver.setReportDir(mReportDir);
            saver.invocationStarted(mContext);

            final String testData = "Here's some test data, blah";
            ByteArrayInputStream mockInput = new ByteArrayInputStream(testData.getBytes());
            logFile = saver.saveLogData("testSaveLogData", LogDataType.TEXT, mockInput);

            assertTrue(logFile.getPath().endsWith(LogDataType.ZIP.getFileExt()));
            // Verify test data was written to file
            zipFile = new ZipFile(new File(logFile.getPath()));

            String actualLogString = StreamUtil.getStringFromStream(zipFile.getInputStream(
                    new ZipEntry("testSaveLogData.txt")));
            assertTrue(actualLogString.equals(testData));
        } finally {
            StreamUtil.close(logFileReader);
            if (zipFile != null) {
                zipFile.close();
            }
            FileUtil.deleteFile(new File(logFile.getPath()));
        }
    }

    /**
     * Test saving compressed data for {@link FileSystemLogSaver#saveLogDataRaw(String, LogDataType,
     * InputStream)}.
     */
    @Test
    public void testSaveLogData_compressed() throws IOException {
        LogFile logFile = null;
        BufferedReader logFileReader = null;
        try {
            // TODO: would be nice to create a mock file output to make this test not use disk I/O
            FileSystemLogSaver saver = new FileSystemLogSaver();
            saver.setReportDir(mReportDir);
            saver.invocationStarted(mContext);

            final String testData = "Here's some test data, blah";
            ByteArrayInputStream mockInput = new ByteArrayInputStream(testData.getBytes());
            logFile = saver.saveLogData("testSaveLogData", LogDataType.ZIP, mockInput);

            // Verify test data was written to file
            logFileReader = new BufferedReader(new FileReader(new File(logFile.getPath())));
            String actualLogString = logFileReader.readLine().trim();
            assertTrue(actualLogString.equals(testData));
        } finally {
            StreamUtil.close(logFileReader);
            FileUtil.deleteFile(new File(logFile.getPath()));
        }
    }

    /**
     * Simple normal case test for {@link FileSystemLogSaver#saveLogDataRaw(String, LogDataType,
     * InputStream)}.
     */
    @Test
    public void testSaveLogDataRaw() throws IOException {
        LogFile logFile = null;
        BufferedReader logFileReader = null;
        try {
            // TODO: would be nice to create a mock file output to make this test not use disk I/O
            FileSystemLogSaver saver = new FileSystemLogSaver();
            saver.setReportDir(mReportDir);
            saver.invocationStarted(mContext);

            final String testData = "Here's some test data, blah";
            ByteArrayInputStream mockInput = new ByteArrayInputStream(testData.getBytes());
            logFile = saver.saveLogDataRaw("testSaveLogData", LogDataType.TEXT, mockInput);

            // Verify test data was written to file
            logFileReader = new BufferedReader(new FileReader(new File(logFile.getPath())));
            String actualLogString = logFileReader.readLine().trim();
            assertTrue(actualLogString.equals(testData));
        } finally {
            StreamUtil.close(logFileReader);
            FileUtil.deleteFile(new File(logFile.getPath()));
        }
    }

    /**
     * Test running the log saver in sharded environment, only one reporting folder should be
     * created.
     */
    @Test
    public void testCreateReportDirectory_sharded() throws Exception {
        final int shardCount = 5;
        FileSystemLogSaver saver = new FileSystemLogSaver();
        OptionSetter setter = new OptionSetter(saver);
        setter.setOptionValue("log-file-path", mReportDir.getAbsolutePath());
        saver.invocationStarted(mContext);
        ShardMasterResultForwarder master =
                new ShardMasterResultForwarder(
                        new ArrayList<ITestInvocationListener>(), shardCount);
        for (int i = 0; i < shardCount; i++) {
            master.invocationStarted(mContext);
        }
        for (int i = 0; i < shardCount; i++) {
            master.invocationEnded(5);
        }
        // only one folder is created under the hierarchy: branch/buildid/testtag/<inv_ folders>
        assertEquals(1, mReportDir.list().length);
        assertEquals(BRANCH, mReportDir.list()[0]);
        assertEquals(1, mReportDir.listFiles()[0].list().length);
        assertEquals(BUILD_ID, mReportDir.listFiles()[0].list()[0]);
        assertEquals(1, mReportDir.listFiles()[0].listFiles()[0].list().length);
        assertEquals(TEST_TAG, mReportDir.listFiles()[0].listFiles()[0].list()[0]);
        // Only one inv_ folder
        assertEquals(1, mReportDir.listFiles()[0].listFiles()[0].listFiles()[0].list().length);
        assertTrue(
                mReportDir.listFiles()[0].listFiles()[0].listFiles()[0].list()[0].startsWith(
                        "inv_"));
    }
}
