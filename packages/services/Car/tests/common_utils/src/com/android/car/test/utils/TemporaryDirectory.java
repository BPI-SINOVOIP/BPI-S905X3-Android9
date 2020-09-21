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
import java.io.File;
import java.io.IOException;
import java.nio.file.FileVisitResult;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.SimpleFileVisitor;
import java.nio.file.attribute.BasicFileAttributes;

/**
 * A utility class that creates a temporary directory.
 * The directory, and any files contained within, are automatically deleted when calling close().
 *
 * Example usage:
 *
 * try (TemporaryDirectory tf = new TemporaryDirectory("myTest")) {
 *     ...
 * } // directory gets deleted here
 */
public class TemporaryDirectory implements AutoCloseable {
    private Path mDirectory;

    private static final class DeletingVisitor extends SimpleFileVisitor<Path> {
        FileVisitResult consume(Path path) throws IOException {
            Files.delete(path);
            return FileVisitResult.CONTINUE;
        }

        @Override
        public FileVisitResult visitFile(Path path, BasicFileAttributes basicFileAttributes)
                throws IOException {
            return consume(path);
        }

        @Override
        public FileVisitResult postVisitDirectory(Path path, IOException e)
                throws IOException {
            return consume(path);
        }
    }

    private static final SimpleFileVisitor<Path> DELETE = new DeletingVisitor();

    TemporaryDirectory(Path directory) throws IOException {
        mDirectory = Files.createDirectory(directory);
    }

    public TemporaryDirectory(@Nullable String prefix) throws IOException {
        if (prefix == null) {
            prefix = TemporaryDirectory.class.getSimpleName();
        }
        prefix += String.valueOf(SystemClock.elapsedRealtimeNanos());

        mDirectory = Files.createTempDirectory(prefix);
    }

    @Override
    public void close() throws Exception {
        Files.walkFileTree(mDirectory, DELETE);
    }

    public File getDirectory() {
        return mDirectory.toFile();
    }

    public Path getPath() { return mDirectory; }

    public TemporaryDirectory getSubdirectory(String name) throws IOException {
        return new TemporaryDirectory(mDirectory.resolve(name));
    }
}
