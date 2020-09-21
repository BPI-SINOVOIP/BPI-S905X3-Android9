/*
 * Copyright (C) 2011 The Android Open Source Project
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
package com.android.tradefed.result;

import java.io.ByteArrayInputStream;
import java.io.InputStream;

public class ByteArrayInputStreamSource implements InputStreamSource {
    private byte[] mArray;
    private boolean mIsCancelled = false;

    public ByteArrayInputStreamSource(byte[] array) {
        mArray = array;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public synchronized InputStream createInputStream() {
        if (mIsCancelled) {
            return null;
        }

        return new ByteArrayInputStream(mArray);
    }

    /** {@inheritDoc} */
    @Override
    public synchronized void close() {
        if (!mIsCancelled) {
            mIsCancelled = true;
            mArray = new byte[0];
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public long size() {
        return mArray.length;
    }
}

