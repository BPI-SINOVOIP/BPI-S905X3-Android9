/*
 * Copyright (C) 2010 The Android Open Source Project
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

/**
 * Represents the data type of log data.
 */
public enum LogDataType {

    TEXT("txt", "text/plain", false, true),
    XML("xml", "text/xml", false, true),
    HTML("html", "text/html", true, true),
    PNG("png", "image/png", true, false),
    MP4("mp4", "video/mp4", true, false),
    EAR("ear", "application/octet-stream", true, false),
    ZIP("zip", "application/zip", true, false),
    JPEG("jpeg", "image/jpeg", true, false),
    TAR_GZ("tar.gz", "application/gzip", true, false),
    GZIP("gz", "application/gzip", true, false),
    HPROF("hprof", "text/plain", true, false),
    COVERAGE("ec", "text/plain", false, false), // Emma coverage file
    PB("pb", "application/octet-stream", true, false), // Binary proto file
    TEXTPB("textproto", "text/plain", false, true), // Text proto file
    /* Specific text file types */
    BUGREPORT("txt", "text/plain", false, true),
    BUGREPORTZ("zip", "application/zip", true, false),
    LOGCAT("txt", "text/plain", false, true),
    KERNEL_LOG("txt", "text/plain", false, true),
    MONKEY_LOG("txt", "text/plain", false, true),
    MUGSHOT_LOG("txt", "text/plain", false, true),
    PROCRANK("txt", "text/plain", false, true),
    MEM_INFO("txt", "text/plain", false, true),
    TOP("txt", "text/plain", false, true),
    DUMPSYS("txt", "text/plain", false, true),
    COMPACT_MEMINFO("txt", "text/plain", false, true), // dumpsys meminfo -c
    SERVICES("txt", "text/plain", false, true), // dumpsys activity services
    GFX_INFO("txt", "text/plain", false, true), // dumpsys gfxinfo
    CPU_INFO("txt", "text/plain", false, true), // dumpsys cpuinfo
    JACOCO_CSV("csv", "text/csv", false, true), // JaCoCo coverage report in CSV format
    JACOCO_XML("xml", "text/xml", false, true), // JaCoCo coverage report in XML format
    ATRACE("atr", "text/plain", true, false), // atrace -z format
    KERNEL_TRACE("dat", "text/plain", false, false), // raw kernel ftrace buffer
    DIR("", "text/plain", false, false),
    /* Unknown file type */
    UNKNOWN("dat", "text/plain", false, false);

    private final String mFileExt;
    private final String mContentType;
    private final boolean mIsCompressed;
    private final boolean mIsText;

    LogDataType(String fileExt, String contentType, boolean compressed, boolean text) {
        mFileExt = fileExt;
        mIsCompressed = compressed;
        mIsText = text;
        mContentType = contentType;
    }

    public String getFileExt() {
        return mFileExt;
    }

    public String getContentType() {
        return mContentType;
    }

    /**
     * @return <code>true</code> if data type is a compressed format.
     */
    public boolean isCompressed() {
        return mIsCompressed;
    }

    /**
     * @return <code>true</code> if data type is a textual format.
     */
    public boolean isText() {
        return mIsText;
    }
}
