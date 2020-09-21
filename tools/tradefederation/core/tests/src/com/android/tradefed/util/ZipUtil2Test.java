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

import static org.junit.Assert.fail;

import org.apache.commons.compress.archivers.zip.ZipFile;
import org.junit.After;
import org.junit.Assert;
import org.junit.Test;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Files;
import java.nio.file.attribute.PosixFilePermission;
import java.nio.file.attribute.PosixFilePermissions;
import java.util.HashSet;
import java.util.Set;

/**
 * Unit tests for {@link ZipUtil2}
 */
public class ZipUtil2Test {

    private Set<File> mTempFiles = new HashSet<File>();

    /**
     * Cleans up temp test files
     * @throws Exception
     */
    @After
    public void tempFileCleanUp() throws Exception {
        for (File file : mTempFiles) {
            FileUtil.recursiveDelete(file);
        }
    }

    /**
     * Utility method to assert that provided file has expected permission per standard Unix
     * notation, e.g. file should have permission "rwxrwxrwx"
     * @param file
     */
    private void verifyFilePermission(File file, String permString) throws IOException {
        Set<PosixFilePermission> actual = Files.getPosixFilePermissions(file.toPath());
        Set<PosixFilePermission> expected = PosixFilePermissions.fromString(permString);
        Assert.assertEquals(expected, actual);
    }

    /**
     * Utility method to assert that provided file has expected permission per its file name, i.e.
     * a file (literally) named "rwxrwxrwx" should have the expected permission bits
     * @param file
     */
    private void verifyFilePermission(File file) throws IOException {
        verifyFilePermission(file, file.getName());
    }

    @Test
    public void testExtractFileFromZip() throws Exception {
        final File zip = getTestDataFile("permission-test");
        final String fileName = "rwxr-x--x";
        ZipFile zipFile = null;
        try {
            zipFile = new ZipFile(zip);
            File extracted = ZipUtil2.extractFileFromZip(zipFile, fileName);
            mTempFiles.add(extracted);
            verifyFilePermission(extracted, fileName);
        } finally {
            ZipFile.closeQuietly(zipFile);
        }
    }

    @Test
    public void testExtractZip() throws Exception {
        final File zip = getTestDataFile("permission-test");
        final File destDir = createTempDir("ZipUtil2Test");
        ZipFile zipFile = null;
        try {
            zipFile = new ZipFile(zip);
            ZipUtil2.extractZip(zipFile, destDir);
            // now loop over files to verify
            for (File file : destDir.listFiles()) {
                // the pmierssion-test.zip file has no hierarchy inside
                verifyFilePermission(file);
            }
        } finally {
            ZipFile.closeQuietly(zipFile);
        }
    }

    /**
     * Test that {@link ZipUtil2#extractZipToTemp(File, String)} properly throws when an incorrect
     * zip is presented.
     */
    @Test
    public void testExtractZipToTemp() throws Exception {
        File tmpFile = FileUtil.createTempFile("ziputiltest", ".zip");
        try {
            ZipUtil2.extractZipToTemp(tmpFile, "testExtractZipToTemp");
            fail("Should have thrown an exception");
        } catch (IOException expected) {
            // expected
        } finally {
            FileUtil.deleteFile(tmpFile);
        }
    }

    private File getTestDataFile(String name) throws IOException {
        final InputStream inputStream =
                getClass().getResourceAsStream(String.format("/util/%s.zip", name));
        final File zipFile = createTempFile(name, ".zip");
        FileUtil.writeToFile(inputStream, zipFile);
        return zipFile;
    }

    // Helpers
    private File createTempDir(String prefix) throws IOException {
        return createTempDir(prefix, null);
    }

    private File createTempDir(String prefix, File parentDir) throws IOException {
        File tempDir = FileUtil.createTempDir(prefix, parentDir);
        mTempFiles.add(tempDir);
        return tempDir;
    }

    private File createTempFile(String prefix, String suffix) throws IOException {
        File tempFile = FileUtil.createTempFile(prefix, suffix);
        mTempFiles.add(tempFile);
        return tempFile;
    }
}
