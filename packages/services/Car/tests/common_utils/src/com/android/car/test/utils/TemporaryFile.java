/*
 * Copyright (C) 2017 The Android Open Source Project
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

package com.android.car.test.utils;

import android.annotation.Nullable;
import android.os.SystemClock;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;

/**
 * A utility class that creates a temporary file.
 * The file is automatically deleted when calling close().
 *
 * Example usage:
 *
 * try (TemporaryFile tf = new TemporaryFile("myTest")) {
 *     ...
 * } // file gets deleted here
 */
public final class TemporaryFile implements AutoCloseable {
    private File mFile;

    public TemporaryFile(@Nullable String prefix) throws IOException {
        if (prefix == null) {
            prefix = TemporaryFile.class.getSimpleName();
        }
        mFile = File.createTempFile(prefix, String.valueOf(SystemClock.elapsedRealtimeNanos()));
    }

    public TemporaryFile(File path) {
        mFile = path;
    }

    @Override
    public void close() throws Exception {
        Files.delete(mFile.toPath());
    }

    public void write(String s) throws IOException {
        BufferedWriter writer = new BufferedWriter(new FileWriter(mFile));
        writer.write(s);
        writer.close();
    }

    public FileWriter newFileWriter() throws IOException {
        return new FileWriter(mFile);
    }

    public File getFile() {
        return mFile;
    }

    public Path getPath() { return mFile.toPath(); }
}
