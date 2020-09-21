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
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.FileInputStreamSource;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.result.LogDataType;

import org.apache.commons.compress.archivers.ArchiveException;
import org.apache.commons.compress.archivers.ArchiveStreamFactory;
import org.apache.commons.compress.archivers.tar.TarArchiveEntry;
import org.apache.commons.compress.archivers.tar.TarArchiveInputStream;
import org.apache.commons.compress.utils.IOUtils;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.LinkedList;
import java.util.List;
import java.util.zip.GZIPInputStream;
import java.util.zip.GZIPOutputStream;

/**
 * Utility to manipulate a tar file. It wraps the commons-compress in order to provide tar support.
 */
public class TarUtil {

    /**
     * Untar a tar file into a directory.
     * tar.gz file need to up {@link #unGzip(File, File)} first.
     *
     * @param inputFile The tar file to extract
     * @param outputDir the directory where to put the extracted files.
     * @return The list of {@link File} untarred.
     * @throws FileNotFoundException
     * @throws IOException
     */
    public static List<File> unTar(final File inputFile, final File outputDir)
            throws FileNotFoundException, IOException {
        CLog.i(String.format("Untaring %s to dir %s.", inputFile.getAbsolutePath(),
                outputDir.getAbsolutePath()));
        final List<File> untaredFiles = new LinkedList<File>();
        final InputStream is = new FileInputStream(inputFile);
        TarArchiveInputStream debInputStream = null;
        try {
            debInputStream = (TarArchiveInputStream)
                    new ArchiveStreamFactory().createArchiveInputStream("tar", is);
            TarArchiveEntry entry = null;
            while ((entry = (TarArchiveEntry)debInputStream.getNextEntry()) != null) {
                final File outputFile = new File(outputDir, entry.getName());
                if (entry.isDirectory()) {
                    CLog.i(String.format("Attempting to write output directory %s.",
                            outputFile.getAbsolutePath()));
                    if (!outputFile.exists()) {
                        CLog.i(String.format("Attempting to create output directory %s.",
                                outputFile.getAbsolutePath()));
                        if (!outputFile.mkdirs()) {
                            throw new IllegalStateException(
                                    String.format("Couldn't create directory %s.",
                                    outputFile.getAbsolutePath()));
                        }
                    }
                } else {
                    CLog.i(String.format("Creating output file %s.", outputFile.getAbsolutePath()));
                    final OutputStream outputFileStream = new FileOutputStream(outputFile);
                    IOUtils.copy(debInputStream, outputFileStream);
                    StreamUtil.close(outputFileStream);
                }
                untaredFiles.add(outputFile);
            }
        } catch (ArchiveException ae) {
            // We rethrow the ArchiveException through a more generic one.
            throw new IOException(ae);
        } finally {
            StreamUtil.close(debInputStream);
            StreamUtil.close(is);
        }
        return untaredFiles;
    }

    /**
     * UnGZip a file: a tar.gz file will become a tar file.
     *
     * @param inputFile The {@link File} to ungzip
     * @param outputDir The directory where to put the ungzipped file.
     * @return a {@link File} pointing to the ungzipped file.
     * @throws FileNotFoundException
     * @throws IOException
     */
    public static File unGzip(final File inputFile, final File outputDir)
            throws FileNotFoundException, IOException {
        CLog.i(String.format("Ungzipping %s to dir %s.", inputFile.getAbsolutePath(),
                outputDir.getAbsolutePath()));
        // rename '-3' to remove the '.gz' extension.
        final File outputFile = new File(outputDir, inputFile.getName().substring(0,
                inputFile.getName().length() - 3));
        GZIPInputStream in = null;
        FileOutputStream out = null;
        try {
            in = new GZIPInputStream(new FileInputStream(inputFile));
            out = new FileOutputStream(outputFile);
            IOUtils.copy(in, out);
        } finally {
            StreamUtil.close(in);
            StreamUtil.close(out);
        }
        return outputFile;
    }

    /**
     * Utility function to gzip (.gz) a file. the .gz extension will be added to base file name.
     *
     * @param inputFile the {@link File} to be gzipped.
     * @return the gzipped file.
     * @throws IOException
     */
    public static File gzip(final File inputFile) throws IOException {
        File outputFile = FileUtil.createTempFile(inputFile.getName(), ".gz");
        GZIPOutputStream out = null;
        FileInputStream in = null;
        try {
            out = new GZIPOutputStream(new FileOutputStream(outputFile));
            in = new FileInputStream(inputFile);
            IOUtils.copy(in, out);
        } catch (IOException e) {
            // delete the tmp file if we failed to gzip.
            FileUtil.deleteFile(outputFile);
            throw e;
        } finally {
            StreamUtil.close(in);
            StreamUtil.close(out);
        }
        return outputFile;
    }

    /**
     * Helper to extract and log to the reporters a tar gz file and its content
     *
     * @param listener the {@link ITestLogger} where to log the files.
     * @param targzFile the tar.gz {@link File} that needs its content log.
     * @param baseName the base name under which the files will be found.
     */
    public static void extractAndLog(ITestLogger listener, File targzFile, String baseName)
            throws FileNotFoundException, IOException {
        // First upload the tar.gz file
        InputStreamSource inputStream = null;
        try {
            inputStream = new FileInputStreamSource(targzFile);
            listener.testLog(baseName, LogDataType.TAR_GZ, inputStream);
        } finally {
            StreamUtil.cancel(inputStream);
        }

        // extract and upload internal files.
        File dir = FileUtil.createTempDir("tmp_tar_dir");
        File ungzipLog = null;
        try {
            ungzipLog = TarUtil.unGzip(targzFile, dir);
            List<File> logs = TarUtil.unTar(ungzipLog, dir);
            for (File f : logs) {
                InputStreamSource s = null;
                try {
                    s = new FileInputStreamSource(f);
                    listener.testLog(String.format("%s_%s", baseName, f.getName()),
                            LogDataType.TEXT, s);
                } finally {
                    StreamUtil.cancel(s);
                }
            }
        } finally {
            FileUtil.deleteFile(ungzipLog);
            FileUtil.recursiveDelete(dir);
        }
    }
}
