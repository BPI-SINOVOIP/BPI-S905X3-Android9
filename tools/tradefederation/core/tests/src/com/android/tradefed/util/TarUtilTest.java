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
package com.android.tradefed.util;

import static org.junit.Assert.*;

import com.android.tradefed.log.ITestLogger;
import com.android.tradefed.result.LogDataType;

import org.easymock.EasyMock;
import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.InputStream;
import java.util.List;

/**
 * Test class for {@link TarUtil}.
 */
public class TarUtilTest {

    private static final String EMMA_METADATA_RESOURCE_PATH = "/testdata/LOG.tar.gz";
    private File mWorkDir;

    @Before
    public void setUp() throws Exception {
        mWorkDir = FileUtil.createTempDir("tarutil_unit_test_dir");
    }

    @After
    public void tearDown() {
        FileUtil.recursiveDelete(mWorkDir);
    }

    /**
     * Test that {TarUtil#unGzip(File, File)} can ungzip properly a tar.gz file.
     */
    @Test
    public void testUnGzip() throws Exception {
        InputStream logTarGz = getClass().getResourceAsStream(EMMA_METADATA_RESOURCE_PATH);
        File logTarGzFile = FileUtil.createTempFile("log_tarutil_test", ".tar.gz");
        try {
            FileUtil.writeToFile(logTarGz, logTarGzFile);
            File testFile = TarUtil.unGzip(logTarGzFile, mWorkDir);
            Assert.assertTrue(testFile.exists());
            // Expect same name without the .gz extension.
            String expectedName = logTarGzFile.getName().substring(0,
                    logTarGzFile.getName().length() - 3);
            Assert.assertEquals(expectedName, testFile.getName());
        } finally {
            FileUtil.deleteFile(logTarGzFile);
        }
    }

    /**
     * Test that {TarUtil#unTar(File, File)} can untar properly a tar file.
     */
    @Test
    public void testUntar() throws Exception {
        InputStream logTarGz = getClass().getResourceAsStream(EMMA_METADATA_RESOURCE_PATH);
        File logTarGzFile = FileUtil.createTempFile("log_tarutil_test", ".tar.gz");
        try {
            FileUtil.writeToFile(logTarGz, logTarGzFile);
            File testFile = TarUtil.unGzip(logTarGzFile, mWorkDir);
            Assert.assertTrue(testFile.exists());
            List<File> untaredList = TarUtil.unTar(testFile, mWorkDir);
            Assert.assertEquals(2, untaredList.size());
        } finally {
            FileUtil.deleteFile(logTarGzFile);
        }
    }

    /**
     * Test that {TarUtil#extractAndLog(ITestLogger, File, String)} can untar properly a tar file
     * and export its content.
     */
    @Test
    public void testExtractAndLog() throws Exception {
        final String baseName = "BASE_NAME";
        InputStream logTarGz = getClass().getResourceAsStream(EMMA_METADATA_RESOURCE_PATH);
        File logTarGzFile = FileUtil.createTempFile("log_tarutil_test", ".tar.gz");
        try {
            FileUtil.writeToFile(logTarGz, logTarGzFile);
            ITestLogger listener = EasyMock.createMock(ITestLogger.class);
            // Main tar file is logged under the base name directly
            listener.testLog(EasyMock.eq(baseName), EasyMock.eq(LogDataType.TAR_GZ),
                    EasyMock.anyObject());
            // Contents is log under baseName_filename
            listener.testLog(EasyMock.eq(baseName + "_TEST.log"), EasyMock.eq(LogDataType.TEXT),
                    EasyMock.anyObject());
            listener.testLog(EasyMock.eq(baseName + "_TEST2.log"), EasyMock.eq(LogDataType.TEXT),
                    EasyMock.anyObject());
            EasyMock.replay(listener);
            TarUtil.extractAndLog(listener, logTarGzFile, baseName);
            EasyMock.verify(listener);
        } finally {
            FileUtil.deleteFile(logTarGzFile);
        }
    }

    /**
     * Test that {@link TarUtil#gzip(File)} is properly zipping the file and can be unzipped to
     * recover the original file.
     */
    @Test
    public void testGzipDir_unGzip() throws Exception {
        final String content = "I'LL BE ZIPPED";
        File tmpFile = FileUtil.createTempFile("base_file", ".txt", mWorkDir);
        File zipped = null;
        File unzipped = FileUtil.createTempDir("unzip-test-dir", mWorkDir);
        try {
            FileUtil.writeToFile(content, tmpFile);
            zipped = TarUtil.gzip(tmpFile);
            assertTrue(zipped.exists());
            assertTrue(zipped.getName().endsWith(".gz"));
            // unzip the file to ensure our utility can go both way
            TarUtil.unGzip(zipped, unzipped);
            assertEquals(1, unzipped.list().length);
            // the original file is found
            assertEquals(content, FileUtil.readStringFromFile(unzipped.listFiles()[0]));
        } finally {
            FileUtil.recursiveDelete(zipped);
            FileUtil.recursiveDelete(unzipped);
        }
    }

    /** Test to ensure that {@link TarUtil#gzip(File)} properly throws if the file is not valid. */
    @Test
    public void testGzip_invalidFile() throws Exception {
        try {
            TarUtil.gzip(new File("i_do_not_exist"));
            fail("Should have thrown an exception.");
        } catch (FileNotFoundException expected) {
            // expected
        }
    }
}
