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

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.SequenceInputStream;

/**
 * An in-memory {@link OutputStream} that only keeps a maximum amount of data.
 * <p/>
 * This is implemented by keeping a circular byte array of fixed size.
 * <p/>
 * Not thread safe.
 */
public class FixedByteArrayOutputStream extends OutputStream {

    private final byte[] mBuffer;
    private int mWritePos = 0;
    /**
     * flag used to indicate if mWritePos has been wrapped to beginning of buffer. eg amount of
     * data written to stream is greater than buffer size.
     */
    private boolean mHasWrapped = false;

    /**
     * Creates a {@link FixedByteArrayOutputStream}.
     *
     * @param maxDataSize the approximate max size in bytes to keep in the output stream
     */
    public FixedByteArrayOutputStream(int maxDataSize) {
        mBuffer = new byte[maxDataSize];
    }

    /**
     * Gets a InputStream for reading collected output.
     * <p/>
     * Not thread safe. Assumes no data will be written while being read
     */
    public InputStream getData() {
        if (mHasWrapped) {
          InputStream s1 =  new ByteArrayInputStream(mBuffer, mWritePos,
                  mBuffer.length - mWritePos);
          InputStream s2 =  new ByteArrayInputStream(mBuffer, 0, mWritePos);
          return new SequenceInputStream(s1, s2);
        } else {
            return new ByteArrayInputStream(mBuffer, 0, mWritePos);
        }
    }

    @Override
    public void write(int data) throws IOException {
        mBuffer[mWritePos] = (byte)data;
        mWritePos++;
        if (mWritePos >= mBuffer.length) {
            mHasWrapped = true;
            mWritePos = 0;
        }
    }

    @Override
    public void write(byte[] b, int off, int len) throws IOException {
        if (b == null) {
            throw new NullPointerException();
        }
        if (len >= mBuffer.length) {
            // incoming is larger than buffer just take the tail end of the incoming
            System.arraycopy(b, off + len - mBuffer.length, mBuffer, 0, mBuffer.length);
            mHasWrapped = true;
            mWritePos = 0;
        } else {
            // incoming is smaller, fill it into free space of buffer, wrap if needed
            int freeSpace = mBuffer.length - mWritePos;
            if (freeSpace >= len) {
                // current write position to end of buffer is large enough for "len"
                System.arraycopy(b, off, mBuffer, mWritePos, len);
            } else {
                // fill up free space of the buffer first
                System.arraycopy(b, off, mBuffer, mWritePos, freeSpace);
                // wrap around the circular buffer
                System.arraycopy(b, off + freeSpace, mBuffer, 0, len - freeSpace);
            }
            // update write position
            mWritePos += len;
            if (mWritePos > mBuffer.length) {
                mHasWrapped = true;
                mWritePos = mWritePos % mBuffer.length;
            }
        }
    }

    /**
     * @return the number of bytes currently stored.
     */
    public long size() {
        if (mHasWrapped) {
            return mBuffer.length;
        } else {
            return mWritePos;
        }
    }
}
