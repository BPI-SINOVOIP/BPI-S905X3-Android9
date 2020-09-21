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

package com.android.tools.build.apkzlib.sign;

import com.android.apksig.util.DataSink;
import com.android.apksig.util.DataSource;
import com.android.tools.build.apkzlib.zip.ZFile;
import com.google.common.base.Preconditions;
import java.io.EOFException;
import java.io.IOException;
import java.nio.ByteBuffer;
import javax.annotation.Nonnull;

/**
 * {@link DataSource} backed by contents of {@link ZFile}.
 */
class ZFileDataSource implements DataSource {

    private static final int MAX_READ_CHUNK_SIZE = 65536;

    @Nonnull
    private final ZFile file;

    /**
     * Offset (in bytes) relative to the start of file where the region visible in this data source
     * starts.
     */
    private final long offset;

    /**
     * Size (in bytes) of the file region visible in this data source or {@code -1} if the whole
     * file is visible in this data source and thus its size may change if the file's size changes.
     */
    private final long size;

    /**
     * Constructs a new {@code ZFileDataSource} based on the data contained in the file. Changes to
     * the contents of the file, including the size of the file, will be visible in this data
     * source.
     */
    public ZFileDataSource(@Nonnull ZFile file) {
        this.file = file;
        offset = 0;
        size = -1;
    }

    /**
     * Constructs a new {@code ZFileDataSource} based on the data contained in the specified region
     * of the provided file. Changes to the contents of this region of the file will be visible in
     * this data source.
     */
    public ZFileDataSource(@Nonnull ZFile file, long offset, long size) {
        Preconditions.checkArgument(offset >= 0, "offset < 0");
        Preconditions.checkArgument(size >= 0, "size < 0");
        this.file = file;
        this.offset = offset;
        this.size = size;
    }

    @Override
    public long size() {
        if (size == -1) {
            // Data source size is the current size of the file
            try {
                return file.directSize();
            } catch (IOException e) {
                return 0;
            }
        } else {
            // Data source size is fixed
            return size;
        }
    }

    @Override
    public DataSource slice(long offset, long size) {
        long sourceSize = size();
        checkChunkValid(offset, size, sourceSize);
        if ((offset == 0) && (size == sourceSize)) {
            return this;
        }

        return new ZFileDataSource(file, this.offset + offset, size);
    }

    @Override
    public void feed(long offset, long size, @Nonnull DataSink sink) throws IOException {
        long sourceSize = size();
        checkChunkValid(offset, size, sourceSize);
        if (size == 0) {
            return;
        }

        long chunkOffsetInFile = this.offset + offset;
        long remaining = size;
        byte[] buf = new byte[(int) Math.min(remaining, MAX_READ_CHUNK_SIZE)];
        while (remaining > 0) {
            int chunkSize = (int) Math.min(remaining, buf.length);
            int readSize = file.directRead(chunkOffsetInFile, buf, 0, chunkSize);
            if (readSize == -1) {
                throw new EOFException("Premature EOF");
            }
            if (readSize > 0) {
                sink.consume(buf, 0, readSize);
                chunkOffsetInFile += readSize;
                remaining -= readSize;
            }
        }
    }

    @Override
    public void copyTo(long offset, int size, @Nonnull ByteBuffer dest) throws IOException {
        long sourceSize = size();
        checkChunkValid(offset, size, sourceSize);
        if (size == 0) {
            return;
        }

        int prevLimit = dest.limit();
        try {
            file.directFullyRead(this.offset + offset, dest);
        } finally {
            dest.limit(prevLimit);
        }
    }

    @Override
    public ByteBuffer getByteBuffer(long offset, int size) throws IOException {
        ByteBuffer result = ByteBuffer.allocate(size);
        copyTo(offset, size, result);
        result.flip();
        return result;
    }

    private static void checkChunkValid(long offset, long size, long sourceSize) {
        Preconditions.checkArgument(offset >= 0, "offset < 0");
        Preconditions.checkArgument(size >= 0, "size < 0");
        Preconditions.checkArgument(offset <= sourceSize, "offset > sourceSize");
        long endOffset = offset + size;
        Preconditions.checkArgument(offset <= endOffset, "offset > endOffset");
        Preconditions.checkArgument(endOffset <= sourceSize, "endOffset > sourceSize");
    }
}
