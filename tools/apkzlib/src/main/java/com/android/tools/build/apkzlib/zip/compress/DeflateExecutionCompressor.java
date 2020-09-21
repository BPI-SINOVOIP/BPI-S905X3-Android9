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

package com.android.tools.build.apkzlib.zip.compress;

import com.android.tools.build.apkzlib.zip.CompressionMethod;
import com.android.tools.build.apkzlib.zip.CompressionResult;
import com.android.tools.build.apkzlib.zip.utils.ByteTracker;
import com.android.tools.build.apkzlib.zip.utils.CloseableByteSource;
import java.io.ByteArrayOutputStream;
import java.util.concurrent.Executor;
import java.util.zip.Deflater;
import java.util.zip.DeflaterOutputStream;
import javax.annotation.Nonnull;

/**
 * Compressor that uses deflate with an executor.
 */
public class DeflateExecutionCompressor extends ExecutorCompressor {


    /**
     * Deflate compression level.
     */
    private final int level;

    /**
     * Byte tracker to use to create byte sources.
     */
    @Nonnull
    private final ByteTracker tracker;

    /**
     * Creates a new compressor.
     *
     * @param executor the executor to run deflation tasks
     * @param tracker the byte tracker to use to keep track of memory usage
     * @param level the compression level
     */
    public DeflateExecutionCompressor(
            @Nonnull Executor executor,
            @Nonnull ByteTracker tracker,
            int level) {
        super(executor);

        this.level = level;
        this.tracker = tracker;
    }

    @Nonnull
    @Override
    protected CompressionResult immediateCompress(@Nonnull CloseableByteSource source)
            throws Exception {
        ByteArrayOutputStream output = new ByteArrayOutputStream();
        Deflater deflater = new Deflater(level, true);

        try (DeflaterOutputStream dos = new DeflaterOutputStream(output, deflater)) {
            dos.write(source.read());
        }

        CloseableByteSource result = tracker.fromStream(output);
        if (result.size() >= source.size()) {
            return new CompressionResult(source, CompressionMethod.STORE, source.size());
        } else {
            return new CompressionResult(result, CompressionMethod.DEFLATE, result.size());
        }
    }
}
