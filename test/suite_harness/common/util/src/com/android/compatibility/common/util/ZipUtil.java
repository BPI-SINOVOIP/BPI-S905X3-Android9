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

package com.android.compatibility.common.util;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.LinkedList;
import java.util.List;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;

public class ZipUtil {

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
            out.close();
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
            inputStream.close();
        }
    }

}
