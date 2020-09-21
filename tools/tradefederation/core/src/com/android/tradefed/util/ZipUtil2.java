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

import com.android.tradefed.log.LogUtil.CLog;

import org.apache.commons.compress.archivers.zip.ZipArchiveEntry;
import org.apache.commons.compress.archivers.zip.ZipFile;

import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.util.Enumeration;

/**
 * A helper class for zip extraction that takes POSIX file permissions into account
 */
public class ZipUtil2 {

    /**
     * A util method to apply unix mode from {@link ZipArchiveEntry} to the created local file
     * system entry if necessary
     * @param entry the entry inside zipfile (potentially contains mode info)
     * @param localFile the extracted local file entry
     * @throws IOException
     */
    private static void applyUnixModeIfNecessary(ZipArchiveEntry entry, File localFile)
            throws IOException {
        if (entry.getPlatform() == ZipArchiveEntry.PLATFORM_UNIX) {
            Files.setPosixFilePermissions(localFile.toPath(),
                    FileUtil.unixModeToPosix(entry.getUnixMode()));
        } else {
            CLog.d(
                    "Entry '%s' exists but does not contain Unix mode permission info. File will "
                            + "have default permission.",
                    entry.getName());
        }
    }

    /**
     * Utility method to extract entire contents of zip file into given directory
     *
     * @param zipFile the {@link ZipFile} to extract
     * @param destDir the local dir to extract file to
     * @throws IOException if failed to extract file
     */
    public static void extractZip(ZipFile zipFile, File destDir) throws IOException {
        Enumeration<? extends ZipArchiveEntry> entries = zipFile.getEntries();
        while (entries.hasMoreElements()) {
            ZipArchiveEntry entry = entries.nextElement();
            File childFile = new File(destDir, entry.getName());
            childFile.getParentFile().mkdirs();
            if (entry.isDirectory()) {
                childFile.mkdirs();
                applyUnixModeIfNecessary(entry, childFile);
                continue;
            } else {
                FileUtil.writeToFile(zipFile.getInputStream(entry), childFile);
                applyUnixModeIfNecessary(entry, childFile);
            }
        }
    }

    /**
     * Utility method to extract a zip file into a given directory. The zip file being presented as
     * a {@link File}.
     *
     * @param zipFile a {@link File} pointing to a zip file.
     * @param destDir the local dir to extract file to
     * @throws IOException if failed to extract file
     */
    public static void extractZip(File zipFile, File destDir) throws IOException {
        try (ZipFile zip = new ZipFile(zipFile)) {
            extractZip(zip, destDir);
        }
    }

    /**
     * Utility method to extract one specific file from zip file into a tmp file
     *
     * @param zipFile the {@link ZipFile} to extract
     * @param filePath the filePath of to extract
     * @throws IOException if failed to extract file
     * @return the {@link File} or null if not found
     */
    public static File extractFileFromZip(ZipFile zipFile, String filePath) throws IOException {
        ZipArchiveEntry entry = zipFile.getEntry(filePath);
        if (entry == null) {
            return null;
        }
        File createdFile = FileUtil.createTempFile("extracted",
                FileUtil.getExtension(filePath));
        FileUtil.writeToFile(zipFile.getInputStream(entry), createdFile);
        applyUnixModeIfNecessary(entry, createdFile);
        return createdFile;
    }

    /**
     * Extract a zip file to a temp directory prepended with a string
     *
     * @param zipFile the zip file to extract
     * @param nameHint a prefix for the temp directory
     * @return a {@link File} pointing to the temp directory
     */
    public static File extractZipToTemp(File zipFile, String nameHint) throws IOException {
        File localRootDir = FileUtil.createTempDir(nameHint);
        try (ZipFile zip = new ZipFile(zipFile)) {
            extractZip(zip, localRootDir);
            return localRootDir;
        } catch (IOException e) {
            // clean tmp file since we couldn't extract.
            FileUtil.recursiveDelete(localRootDir);
            throw e;
        }
    }

    /**
     * Close an open {@link ZipFile}, ignoring any exceptions.
     *
     * @param zipFile the file to close
     */
    public static void closeZip(ZipFile zipFile) {
        if (zipFile != null) {
            try {
                zipFile.close();
            } catch (IOException e) {
                // ignore
            }
        }
    }
}
