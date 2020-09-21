/*
 * Copyright (C) 2008 The Android Open Source Project
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

package com.android.email;

import java.io.IOException;
import java.io.InputStream;

/**
 * A filtering InputStream that allows single byte "peeks" without consuming the byte. The
 * client of this stream can call peek() to see the next available byte in the stream
 * and a subsequent read will still return the peeked byte. 
 */
public class PeekableInputStream extends InputStream {
    private final InputStream mIn;
    private boolean mPeeked;
    private int mPeekedByte;

    public PeekableInputStream(InputStream in) {
        this.mIn = in;
    }

    @Override
    public int read() throws IOException {
        if (!mPeeked) {
            return mIn.read();
        } else {
            mPeeked = false;
            return mPeekedByte;
        }
    }

    public int peek() throws IOException {
        if (!mPeeked) {
            mPeekedByte = read();
            mPeeked = true;
        }
        return mPeekedByte;
    }

    @Override
    public int read(byte[] b, int offset, int length) throws IOException {
        if (!mPeeked) {
            return mIn.read(b, offset, length);
        } else {
            b[0] = (byte)mPeekedByte;
            mPeeked = false;
            int r = mIn.read(b, offset + 1, length - 1);
            if (r == -1) {
                return 1;
            } else {
                return r + 1;
            }
        }
    }

    @Override
    public int read(byte[] b) throws IOException {
        return read(b, 0, b.length);
    }

    @Override
    public String toString() {
        return String.format("PeekableInputStream(in=%s, peeked=%b, peekedByte=%d)",
                mIn.toString(), mPeeked, mPeekedByte);
    }
}
