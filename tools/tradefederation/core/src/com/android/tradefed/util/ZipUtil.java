/*
 * Copyright (C) 2013 The Android Open Source Project
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

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Enumeration;
import java.util.LinkedList;
import java.util.List;
import java.util.zip.GZIPOutputStream;
import java.util.zip.ZipEntry;
import java.util.zip.ZipException;
import java.util.zip.ZipFile;
import java.util.zip.ZipOutputStream;

/**
 * A helper class for compression-related operations
 */
public class ZipUtil {

    private static final String DEFAULT_DIRNAME = "dir";
    private static final String DEFAULT_FILENAME = "files";
    private static final String ZIP_EXTENSION = ".zip";

    /**
     * Utility method to verify that a zip file is not corrupt.
     *
     * @param zipFile the {@link File} to check
     * @param thorough Whether to attempt to fully extract the archive.  If {@code false}, this
     *        method will fail to detect CRC errors in a well-formed archive.
     * @throws IOException if the file could not be opened or read
     * @return {@code false} if the file appears to be corrupt; {@code true} otherwise
     */
    public static boolean isZipFileValid(File zipFile, boolean thorough) throws IOException {
        if (zipFile != null && !zipFile.exists()) {
            CLog.d("Zip file does not exist: %s", zipFile.getAbsolutePath());
            return false;
        }

        try (ZipFile z = new ZipFile(zipFile)) {
            if (thorough) {
                // Reading the entire file is the only way to detect CRC errors within the archive
                final File extractDir = FileUtil.createTempDir("extract-" + zipFile.getName());
                try {
                    extractZip(z, extractDir);
                } finally {
                    FileUtil.recursiveDelete(extractDir);
                }
            }
        } catch (ZipException e) {
            // File is likely corrupted
            CLog.d("Detected corrupt zip file %s:", zipFile.getCanonicalPath());
            CLog.e(e);
            return false;
        }

        return true;
    }

    /**
     * Utility method to extract entire contents of zip file into given directory
     *
     * @param zipFile the {@link ZipFile} to extract
     * @param destDir the local dir to extract file to
     * @throws IOException if failed to extract file
     */
    public static void extractZip(ZipFile zipFile, File destDir) throws IOException {
        Enumeration<? extends ZipEntry> entries = zipFile.entries();
        while (entries.hasMoreElements()) {

            ZipEntry entry = entries.nextElement();
            File childFile = new File(destDir, entry.getName());
            childFile.getParentFile().mkdirs();
            if (entry.isDirectory()) {
                continue;
            } else {
                FileUtil.writeToFile(zipFile.getInputStream(entry), childFile);
            }
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
        ZipEntry entry = zipFile.getEntry(filePath);
        if (entry == null) {
            return null;
        }
        File createdFile = FileUtil.createTempFile("extracted",
                FileUtil.getExtension(filePath));
        FileUtil.writeToFile(zipFile.getInputStream(entry), createdFile);
        return createdFile;
    }

    /**
     * Utility method to create a temporary zip file containing the given directory and
     * all its contents.
     *
     * @param dir the directory to zip
     * @return a temporary zip {@link File} containing directory contents
     * @throws IOException if failed to create zip file
     */
    public static File createZip(File dir) throws IOException {
        return createZip(dir, DEFAULT_DIRNAME);
    }

    /**
     * Utility method to create a temporary zip file containing the given directory and
     * all its contents.
     *
     * @param dir the directory to zip
     * @param name the base name of the zip file created without the extension.
     * @return a temporary zip {@link File} containing directory contents
     * @throws IOException if failed to create zip file
     */
    public static File createZip(File dir, String name) throws IOException {
        File zipFile = FileUtil.createTempFile(name, ZIP_EXTENSION);
        createZip(dir, zipFile);
        return zipFile;
    }

    /**
     * Utility method to create a zip file containing the given directory and
     * all its contents.
     *
     * @param dir the directory to zip
     * @param zipFile the zip file to create - it should not already exist
     * @throws IOException if failed to create zip file
     */
    public static void createZip(File dir, File zipFile) throws IOException {
        ZipOutputStream out = null;
        try {
            FileOutputStream fileStream = new FileOutputStream(zipFile);
            out = new ZipOutputStream(new BufferedOutputStream(fileStream));
            addToZip(out, dir, new LinkedList<String>());
        } catch (IOException e) {
            zipFile.delete();
            throw e;
        } catch (RuntimeException e) {
            zipFile.delete();
            throw e;
        } finally {
            StreamUtil.close(out);
        }
    }

    /**
     * Utility method to create a temporary zip file containing the given files
     *
     * @param files list of files to zip
     * @return a temporary zip {@link File} containing directory contents
     * @throws IOException if failed to create zip file
     */
    public static File createZip(List<File> files) throws IOException {
        return createZip(files, DEFAULT_FILENAME);
    }

    /**
     * Utility method to create a temporary zip file containing the given files.
     *
     * @param files list of files to zip
     * @param name the base name of the zip file created without the extension.
     * @return a temporary zip {@link File} containing directory contents
     * @throws IOException if failed to create zip file
     */
    public static File createZip(List<File> files, String name) throws IOException {
        File zipFile = FileUtil.createTempFile(name, ZIP_EXTENSION);
        createZip(files, zipFile);
        return zipFile;
    }

    /**
     * Utility method to create a zip file containing the given files
     *
     * @param files list of files to zip
     * @param zipFile the zip file to create - it should not already exist
     * @throws IOException if failed to create zip file
     */
    public static void createZip(List<File> files, File zipFile) throws IOException {
        ZipOutputStream out = null;
        try {
            FileOutputStream fileStream = new FileOutputStream(zipFile);
            out = new ZipOutputStream(new BufferedOutputStream(fileStream));
            for (File file : files) {
                addToZip(out, file, new LinkedList<String>());
            }
        } catch (IOException|RuntimeException e) {
            zipFile.delete();
            throw e;
        } finally {
            StreamUtil.close(out);
        }
    }

    /**
     * Recursively adds given file and its contents to ZipOutputStream
     *
     * @param out the {@link ZipOutputStream}
     * @param file the {@link File} to add to the stream
     * @param relativePathSegs the relative path of file, including separators
     * @throws IOException if failed to add file to zip
     */
    public static void addToZip(ZipOutputStream out, File file, List<String> relativePathSegs)
            throws IOException {
        relativePathSegs.add(file.getName());
        if (file.isDirectory()) {
            // note: it appears even on windows, ZipEntry expects '/' as a path separator
            relativePathSegs.add("/");
        }
        ZipEntry zipEntry = new ZipEntry(buildPath(relativePathSegs));
        out.putNextEntry(zipEntry);
        if (file.isFile()) {
            writeToStream(file, out);
        }
        out.closeEntry();
        if (file.isDirectory()) {
            // recursively add contents
            File[] subFiles = file.listFiles();
            if (subFiles == null) {
                throw new IOException(String.format("Could not read directory %s",
                        file.getAbsolutePath()));
            }
            for (File subFile : subFiles) {
                addToZip(out, subFile, relativePathSegs);
            }
            // remove the path separator
            relativePathSegs.remove(relativePathSegs.size()-1);
        }
        // remove the last segment, added at beginning of method
        relativePathSegs.remove(relativePathSegs.size()-1);
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

    /**
     * Helper method to create a gzipped version of a single file.
     *
     * @param file the original file
     * @param gzipFile the file to place compressed contents in
     * @throws IOException
     */
    public static void gzipFile(File file, File gzipFile) throws IOException {
        GZIPOutputStream out = null;
        try {
            FileOutputStream fileStream = new FileOutputStream(gzipFile);
            out = new GZIPOutputStream(new BufferedOutputStream(fileStream, 64 * 1024));
            writeToStream(file, out);
        } catch (IOException e) {
            gzipFile.delete();
            throw e;
        } catch (RuntimeException e) {
            gzipFile.delete();
            throw e;
        } finally {
            StreamUtil.close(out);
        }
    }

    /**
     * Helper method to write input file contents to output stream.
     *
     * @param file the input {@link File}
     * @param out the {@link OutputStream}
     *
     * @throws IOException
     */
    private static void writeToStream(File file, OutputStream out) throws IOException {
        InputStream inputStream = null;
        try {
            inputStream = new BufferedInputStream(new FileInputStream(file));
            StreamUtil.copyStreams(inputStream, out);
        } finally {
            StreamUtil.close(inputStream);
        }
    }

    /**
     * Builds a file system path from a stack of relative path segments
     *
     * @param relativePathSegs the list of relative paths
     * @return a {@link String} containing all relativePathSegs
     */
    private static String buildPath(List<String> relativePathSegs) {
        StringBuilder pathBuilder = new StringBuilder();
        for (String segment : relativePathSegs) {
            pathBuilder.append(segment);
        }
        return pathBuilder.toString();
    }

    /**
     * Extract a zip file to a temp directory prepended with a string
     *
     * @param zipFile the zip file to extract
     * @param nameHint a prefix for the temp directory
     * @return a {@link File} pointing to the temp directory
     */
    public static File extractZipToTemp(File zipFile, String nameHint)
            throws IOException, ZipException {
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
}
