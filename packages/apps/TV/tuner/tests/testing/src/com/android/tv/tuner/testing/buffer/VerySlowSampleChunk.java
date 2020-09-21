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
package com.android.tv.tuner.testing.buffer;

import android.os.SystemClock;
import com.android.tv.tuner.exoplayer.buffer.SampleChunk;
import com.android.tv.tuner.exoplayer.buffer.SamplePool;
import com.google.android.exoplayer.SampleHolder;
import java.io.File;
import java.io.IOException;

/** A Sample chunk that is slow for testing */
public class VerySlowSampleChunk extends SampleChunk {

    /** Creates a {@link VerySlowSampleChunk}. */
    public static class VerySlowSampleChunkCreator extends SampleChunkCreator {
        @Override
        public SampleChunk createSampleChunk(
                SamplePool samplePool,
                File file,
                long startPositionUs,
                ChunkCallback chunkCallback) {
            return new VerySlowSampleChunk(
                    samplePool, file, startPositionUs, System.currentTimeMillis(), chunkCallback);
        }
    }

    private VerySlowSampleChunk(
            SamplePool samplePool,
            File file,
            long startPositionUs,
            long createdTimeMs,
            ChunkCallback chunkCallback) {
        super(samplePool, file, startPositionUs, createdTimeMs, chunkCallback);
    }

    @Override
    protected void write(SampleHolder sample, IoState state) throws IOException {
        SystemClock.sleep(10);
        super.write(sample, state);
    }
}
