/*
 * Copyright (C) 2013 The Android Open Source Project
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

import com.android.tradefed.build.BuildSerializedVersion;

import java.io.Serializable;

/** Class to hold the metadata for a saved log file. */
public class LogFile implements Serializable {
    private static final long serialVersionUID = BuildSerializedVersion.VERSION;

    private final String mPath;
    private final String mUrl;
    private final boolean mIsText;
    private final boolean mIsCompressed;
    private final LogDataType mType;
    private final long mSize;

    /**
     * Construct a {@link LogFile} with filepath, URL, and {@link LogDataType} metadata.
     *
     * @param path The absolute path to the saved file.
     * @param url The URL where the saved file can be accessed.
     * @param type The {@link LogDataType} of the logged file.
     */
    public LogFile(String path, String url, LogDataType type) {
        mPath = path;
        mUrl = url;
        mIsCompressed = type.isCompressed();
        mIsText = type.isText();
        mType = type;
        mSize = 0L;
    }

    /**
     * Construct a {@link LogFile} with filepath, URL, and {@link LogDataType} metadata. Variation
     * of {@link LogFile#LogFile(String, String, LogDataType)} where the compressed state can be set
     * explicitly because in some cases, we manually compress the file but we want to keep the
     * original type for tracking purpose.
     *
     * @param path The absolute path to the saved file.
     * @param url The URL where the saved file can be accessed.
     * @param isCompressed Whether the file was compressed or not.
     * @param type The {@link LogDataType} of the logged file.
     * @param size The size of the logged file in bytes.
     */
    public LogFile(String path, String url, boolean isCompressed, LogDataType type, long size) {
        mPath = path;
        mUrl = url;
        mIsCompressed = isCompressed;
        mIsText = type.isText();
        mType = type;
        mSize = size;
    }

    /**
     * Get the absolute path.
     */
    public String getPath() {
        return mPath;
    }

    /**
     * Get the URL where the saved file can be accessed.
     */
    public String getUrl() {
        return mUrl;
    }

    /**
     * Get whether the file can be displayed as text.
     */
    public boolean isText() {
        return mIsText;
    }

    /**
     * Get whether the file is compressed.
     */
    public boolean isCompressed() {
        return mIsCompressed;
    }

    /** Get the {@link LogDataType} of the file that was logged. */
    public LogDataType getType() {
        return mType;
    }

    /** Get the size of the logged file. */
    public long getSize() {
        return mSize;
    }
}
