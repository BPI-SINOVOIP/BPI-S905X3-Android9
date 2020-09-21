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

import com.android.tradefed.log.ITestLogger;
import com.android.tradefed.result.LogDataType;

import org.easymock.EasyMock;
import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;

import java.io.BufferedOutputStream;
import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.LinkedList;
import java.util.List;
import java.util.zip.ZipOutputStream;

/**
 * Unit test suite for {@link Bugreport}
 */
public class BugreportTest {

    private static final String BUGREPORT_PREFIX = "bugreport_DEVICE_";
    private static final String BUGREPORT_CONTENT = "BUGREPORT CONTENT";

    private Bugreport mBugreport;
    private File mZipFile;
    private File mRegularFile;

    @Before
    public void setUp() throws Exception {
        mRegularFile = FileUtil.createTempFile("bugreport-test", "txt");
        File tempDir = null;
        try {
            tempDir = FileUtil.createTempDir("bugreportz");
            File mainEntry = FileUtil.createTempFile("main_entry", ".txt", tempDir);
            File bugreport = FileUtil.createTempFile(BUGREPORT_PREFIX, ".txt", tempDir);
            InputStream name = new ByteArrayInputStream(bugreport.getName().getBytes());
            FileUtil.writeToFile(name, mainEntry);
            InputStream content = new ByteArrayInputStream(BUGREPORT_CONTENT.getBytes());
            FileUtil.writeToFile(content, bugreport);
            mainEntry.renameTo(new File(tempDir, "main_entry.txt"));
            // We create a fake bugreport zip with main_entry.txt and the bugreport.
            mZipFile = ZipUtil.createZip(new File(""));
            FileOutputStream fileStream = new FileOutputStream(mZipFile);
            ZipOutputStream out = new ZipOutputStream(new BufferedOutputStream(fileStream));
            ZipUtil.addToZip(out, bugreport, new LinkedList<String>());
            ZipUtil.addToZip(out, new File(tempDir, "main_entry.txt"), new LinkedList<String>());
            StreamUtil.close(out);
        } finally {
            FileUtil.recursiveDelete(tempDir);
        }
    }

    @After
    public void tearDown() throws Exception {
        FileUtil.deleteFile(mRegularFile);
        FileUtil.deleteFile(mZipFile);
    }

    /**
     * Test {@link Bugreport#getMainFile()} for a flat bugreport.
     */
    @Test
    public void testBugreport_flat() throws IOException {
        mBugreport = new Bugreport(mRegularFile, false);
        try {
            Assert.assertEquals(mRegularFile, mBugreport.getMainFile());
            Assert.assertNull(mBugreport.getListOfFiles());
        } finally {
            mBugreport.close();
        }
    }

    /**
     * Test {@link Bugreport#getMainFile()} for a zipped bugreport.
     */
    @Test
    public void testBugreport_zipped() throws IOException {
        mBugreport = new Bugreport(mZipFile, true);
        File mainFile = null;
        try {
            Assert.assertEquals(2, mBugreport.getListOfFiles().size());
            mainFile = mBugreport.getMainFile();
            Assert.assertNotSame(mZipFile, mainFile);
            Assert.assertEquals(BUGREPORT_CONTENT, FileUtil.readStringFromFile(mainFile));
        } finally {
            FileUtil.deleteFile(mainFile);
            mBugreport.close();
        }
    }

    /**
     * Test {@link Bugreport#getFileByName(String)} for a zipped bugreport.
     */
    @Test
    public void testBugreport_getFileByName() throws IOException {
        mBugreport = new Bugreport(mZipFile, true);
        File mainFile = null;
        try {
            List<String> listFile = mBugreport.getListOfFiles();
            Assert.assertEquals(2, listFile.size());
            String toFetch = null;
            for (String name : listFile) {
                if (name.startsWith(BUGREPORT_PREFIX)) {
                    toFetch = name;
                }
            }
            Assert.assertNotNull(toFetch);
            mainFile = mBugreport.getFileByName(toFetch);
            Assert.assertEquals(BUGREPORT_CONTENT, FileUtil.readStringFromFile(mainFile));
        } finally {
            FileUtil.deleteFile(mainFile);
            mBugreport.close();
        }
    }

    /**
     * Test when created with a null file for zipped and unzipped.
     */
    @Test
    public void testBugreport_nullFile() {
        mBugreport = new Bugreport(null, true);
        Assert.assertNull(mBugreport.getMainFile());
        Assert.assertNull(mBugreport.getListOfFiles());
        Assert.assertNull(mBugreport.getFileByName(""));

        mBugreport = new Bugreport(null, false);
        Assert.assertNull(mBugreport.getMainFile());
        Assert.assertNull(mBugreport.getListOfFiles());
        Assert.assertNull(mBugreport.getFileByName(""));
    }

    /**
     * Test when the zipped bugreport does not contain a main_entry.txt
     */
    @Test
    public void testBugreportz_noMainFile() throws Exception {
        File tempDir = null;
        File zipFile = null;
        try {
            tempDir = FileUtil.createTempDir("bugreportz");
            File bugreport = FileUtil.createTempFile(BUGREPORT_PREFIX, ".txt", tempDir);
            InputStream content = new ByteArrayInputStream(BUGREPORT_CONTENT.getBytes());
            FileUtil.writeToFile(content, bugreport);
            // We create a fake bugreport zip no main_entry.txt and the bugreport.
            zipFile = ZipUtil.createZip(new File(""));
            FileOutputStream fileStream = new FileOutputStream(zipFile);
            ZipOutputStream out = new ZipOutputStream(new BufferedOutputStream(fileStream));
            ZipUtil.addToZip(out, bugreport, new LinkedList<String>());
            StreamUtil.close(out);

            mBugreport = new Bugreport(zipFile, true);
            Assert.assertNull(mBugreport.getMainFile());
        } finally {
            FileUtil.recursiveDelete(tempDir);
            FileUtil.deleteFile(zipFile);
            mBugreport.close();
        }
    }

    /**
     * Test that logging a zip bugreport use the proper type.
     */
    @Test
    public void testLogBugreport() throws Exception {
        final String dataName = "TEST";
        try {
            mBugreport = new Bugreport(mZipFile, true);
            ITestLogger logger = EasyMock.createMock(ITestLogger.class);
            logger.testLog(EasyMock.eq(dataName), EasyMock.eq(LogDataType.BUGREPORTZ),
                    EasyMock.anyObject());
            EasyMock.replay(logger);
            mBugreport.log(dataName, logger);
            EasyMock.verify(logger);
        } finally {
            mBugreport.close();
        }
    }

    /**
     * Test that logging a flat bugreport use the proper type.
     */
    @Test
    public void testLogBugreportFlat() throws Exception {
        final String dataName = "TEST";
        try {
            mBugreport = new Bugreport(mRegularFile, false);
            ITestLogger logger = EasyMock.createMock(ITestLogger.class);
            logger.testLog(EasyMock.eq(dataName), EasyMock.eq(LogDataType.BUGREPORT),
                    EasyMock.anyObject());
            EasyMock.replay(logger);
            mBugreport.log(dataName, logger);
            EasyMock.verify(logger);
        } finally {
            mBugreport.close();
        }
    }
}
