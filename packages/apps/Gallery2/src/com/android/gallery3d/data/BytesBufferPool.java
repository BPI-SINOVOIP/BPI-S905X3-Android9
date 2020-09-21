/*
 * Copyright (C) 2012 The Android Open Source Project
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

package com.android.gallery3d.data;

import com.android.gallery3d.util.ThreadPool.JobContext;

import java.io.FileDescriptor;
import java.io.FileInputStream;
import java.io.IOException;
import java.util.ArrayList;

public class BytesBufferPool {

    private static final int READ_STEP = 4096;

    public static class BytesBuffer {
        public byte[] data;
        public int offset;
        public int length;

        private BytesBuffer(int capacity) {
            this.data = new byte[capacity];
        }

        // an helper function to read content from FileDescriptor
        public void readFrom(JobContext jc, FileDescriptor fd) throws IOException {
            FileInputStream fis = new FileInputStream(fd);
            length = 0;
            try {
                int capacity = data.length;
                while (true) {
                    int step = Math.min(READ_STEP, capacity - length);
                    int rc = fis.read(data, length, step);
                    if (rc < 0 || jc.isCancelled()) return;
                    length += rc;

                    if (length == capacity) {
                        byte[] newData = new byte[data.length * 2];
                        System.arraycopy(data, 0, newData, 0, data.length);
                        data = newData;
                        capacity = data.length;
                    }
                }
            } finally {
                fis.close();
            }
        }
    }

    private final int mPoolSize;
    private final int mBufferSize;
    private final ArrayList<BytesBuffer> mList;

    public BytesBufferPool(int poolSize, int bufferSize) {
        mList = new ArrayList<BytesBuffer>(poolSize);
        mPoolSize = poolSize;
        mBufferSize = bufferSize;
    }

    public synchronized BytesBuffer get() {
        int n = mList.size();
        return n > 0 ? mList.remove(n - 1) : new BytesBuffer(mBufferSize);
    }

    public synchronized void recycle(BytesBuffer buffer) {
        if (buffer.data.length != mBufferSize) return;
        if (mList.size() < mPoolSize) {
            buffer.offset = 0;
            buffer.length = 0;
            mList.add(buffer);
        }
    }

    public synchronized void clear() {
        mList.clear();
    }
}
