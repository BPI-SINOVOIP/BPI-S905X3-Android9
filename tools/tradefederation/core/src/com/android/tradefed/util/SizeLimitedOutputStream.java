/*
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache
 * License, Version 2.0 (the "License");
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

import com.google.common.io.CountingOutputStream;

import java.io.BufferedOutputStream;
import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.SequenceInputStream;

/**
 * A thread safe file backed {@link OutputStream} that limits the maximum amount of data that can be
 * written.
 * <p/>
 * This is implemented by keeping a circular list of Files of fixed size. Once a File has reached a
 * certain size, the class jumps to use the next File in the list. If the next File is non empty, it
 * is deleted, and a new file created.
 */
public class SizeLimitedOutputStream extends OutputStream {

    private static final int DEFAULT_NUM_TMP_FILES = 5;

    /** The max number of bytes to store in the buffer */
    private static final int BUFF_SIZE = 32 * 1024;

    // circular array of backing files
    private final File[] mFiles;
    private final long mMaxFileSize;
    private CountingOutputStream mCurrentOutputStream;
    private int mCurrentFilePos = 0;
    private final String mTempFilePrefix;
    private final String mTempFileSuffix;

    /**
     * Creates a {@link SizeLimitedOutputStream}.
     *
     * @param maxDataSize the approximate max size in bytes to keep in the output stream
     * @param numFiles the max number of backing files to use to store data. Higher values will mean
     *            max data kept will be close to maxDataSize, but with a possible performance
     *            penalty.
     * @param tempFilePrefix prefix to use for temporary files
     * @param tempFileSuffix suffix to use for temporary files
     */
    public SizeLimitedOutputStream(long maxDataSize, int numFiles, String tempFilePrefix,
            String tempFileSuffix) {
        mMaxFileSize = maxDataSize / numFiles;
        mFiles = new File[numFiles];
        mCurrentFilePos = numFiles;
        mTempFilePrefix = tempFilePrefix;
        mTempFileSuffix = tempFileSuffix;
    }

    /**
     * Creates a {@link SizeLimitedOutputStream} with default number of backing files.
     *
     * @param maxDataSize the approximate max size to keep in the output stream
     * @param tempFilePrefix prefix to use for temporary files
     * @param tempFileSuffix suffix to use for temporary files
     */
    public SizeLimitedOutputStream(long maxDataSize, String tempFilePrefix, String tempFileSuffix) {
        this(maxDataSize, DEFAULT_NUM_TMP_FILES, tempFilePrefix, tempFileSuffix);
    }

    /**
     * Gets the collected output as a {@link InputStream}.
     * <p/>
     * It is recommended to  buffer returned stream before using.
     *
     * @return The collected output as a {@link InputStream}.
     */
    public synchronized InputStream getData() throws IOException {
        flush();
        InputStream combinedStream = null;
        for (int i = 0; i < mFiles.length; i++) {
            // oldest/starting file is always the next one up from current
            int currentPos = (mCurrentFilePos + i + 1) % mFiles.length;
            if (mFiles[currentPos] != null) {
                @SuppressWarnings("resource")
                FileInputStream fStream = new FileInputStream(mFiles[currentPos]);
                if (combinedStream == null) {
                    combinedStream = fStream;
                } else {
                    combinedStream = new SequenceInputStream(combinedStream, fStream);
                }
            }
        }
        if (combinedStream == null) {
            combinedStream = new ByteArrayInputStream(new byte[0]);
        }
        return combinedStream;

    }

    /**
     * {@inheritDoc}
     */
    @Override
    public synchronized void flush() {
        if (mCurrentOutputStream == null) {
            return;
        }
        try {
            mCurrentOutputStream.flush();
        } catch (IOException e) {
            // don't use CLog in this class, because its the underlying stream for the logger.
            // leads to bad things
           System.out.printf("failed to flush data: %s\n", e);
        }
    }

    /**
     * Delete all accumulated data.
     */
    public void delete() {
        close();
        for (int i = 0; i < mFiles.length; i++) {
            FileUtil.deleteFile(mFiles[i]);
            mFiles[i] = null;
        }
    }

    /**
     * Closes the write stream
     */
    @Override
    public synchronized void close() {
        StreamUtil.flushAndCloseStream(mCurrentOutputStream);
        mCurrentOutputStream = null;
    }

    /**
     * Creates a new tmp file, closing the old one as necessary
     * <p>
     * Exposed for unit testing.
     * </p>
     *
     * @throws IOException
     * @throws FileNotFoundException
     */
    synchronized void generateNextFile() throws IOException, FileNotFoundException {
        // close current stream
        close();
        mCurrentFilePos = getNextIndex(mCurrentFilePos);
        FileUtil.deleteFile(mFiles[mCurrentFilePos]);
        mFiles[mCurrentFilePos] = FileUtil.createTempFile(mTempFilePrefix, mTempFileSuffix);
        mCurrentOutputStream = new CountingOutputStream(new BufferedOutputStream(
                new FileOutputStream(mFiles[mCurrentFilePos]), BUFF_SIZE));
    }

    /**
     * Gets the next index to use for <var>mFiles</var>, treating it as a circular list.
     */
    private int getNextIndex(int i) {
        return (i + 1) % mFiles.length;
    }

    @Override
    public synchronized void write(int data) throws IOException {
        if (mCurrentOutputStream == null) {
            generateNextFile();
        }
        mCurrentOutputStream.write(data);
        if (mCurrentOutputStream.getCount() >= mMaxFileSize) {
            generateNextFile();
        }
    }

    @Override
    public synchronized void write(byte[] b, int off, int len) throws IOException {
        if (mCurrentOutputStream == null) {
            generateNextFile();
        }
        // keep writing to output stream as long as we have something to write
        while (len > 0) {
            // get current output stream size
            long currentSize = mCurrentOutputStream.getCount();
            // get how many more we can write into current
            long freeSpace = mMaxFileSize - currentSize;
            // decide how much we should write: either fill up free space, or write entire content
            long sizeToWrite = freeSpace > len ? len : freeSpace;
            mCurrentOutputStream.write(b, off, (int)sizeToWrite);
            // accounting of space left, where to write next
            freeSpace -= sizeToWrite;
            off += sizeToWrite;
            len -= sizeToWrite;
            if (freeSpace <= 0) {
                generateNextFile();
            }
        }
    }
}
