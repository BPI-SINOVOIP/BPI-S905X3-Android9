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
package com.android.tradefed.util;

import com.android.tradefed.result.LogDataType;

import java.io.File;
import java.io.IOException;

/**
 * An interface representing a compression algorithm that can be selected at runtime.
 */
public interface ICompressionStrategy {
    /**
     * Compresses the {@code source} file (or folder) and returns the resulting archive.
     *
     * @param source The file or directory to compress
     * @return The compressed archive
     * @throws IOException If the operation could not be completed
     */
    public File compress(File source) throws IOException;

    /** Returns the {@link LogDataType} of the archive format used by this strategy. */
    public LogDataType getLogDataType();
}
