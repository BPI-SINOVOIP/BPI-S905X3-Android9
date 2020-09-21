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

import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.StreamUtil;

import java.io.File;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * A utility class that checks the device for files and sends them to
 * {@link ITestInvocationListener#testLog(String, LogDataType, InputStreamSource)} if found.
 */
public class DeviceFileReporter {
    private final Map<String, LogDataType> mFilePatterns = new LinkedHashMap<>();
    private final ITestInvocationListener mListener;
    private final ITestDevice mDevice;

    /** Whether to ignore files that have already been captured by a prior Pattern */
    private boolean mSkipRepeatFiles = true;
    /** The files which have already been reported */
    private Set<String> mReportedFiles = new HashSet<>();

    /** Whether to attempt to infer data types for patterns with {@code UNKNOWN} data type */
    private boolean mInferDataTypes = true;

    private LogDataType mDefaultFileType = LogDataType.UNKNOWN;

    private static final Map<String, LogDataType> DATA_TYPE_REVERSE_MAP = new HashMap<>();

    static {
        // Make it easy to map backward from file extension to LogDataType
        for (LogDataType type : LogDataType.values()) {
            // Extracted extension will contain a leading dot
            final String ext = "." + type.getFileExt();
            if (DATA_TYPE_REVERSE_MAP.containsKey(ext)) {
                continue;
            }

            DATA_TYPE_REVERSE_MAP.put(ext, type);
        }
    }
    /**
     * Initialize a new DeviceFileReporter with the provided {@link ITestDevice}
     */
    public DeviceFileReporter(ITestDevice device, ITestInvocationListener listener) {
        // Do a null check here, since otherwise that error would be asynchronous
        if (device == null || listener == null) {
            throw new NullPointerException();
        }
        mDevice = device;
        mListener = listener;
    }

    /**
     * Add patterns with the log data type set to the default.
     *
     * @param patterns a varargs array of {@link String} filename glob patterns. Should be absolute.
     * @see #setDefaultLogDataType
     */
    public void addPatterns(String... patterns) {
        addPatterns(Arrays.asList(patterns));
    }

    /**
     * Add patterns with the log data type set to the default.
     *
     * @param patterns a {@link List} of {@link String} filename glob patterns. Should be absolute.
     * @see #setDefaultLogDataType
     */
    public void addPatterns(List<String> patterns) {
        for (String pat : patterns) {
            mFilePatterns.put(pat, mDefaultFileType);
        }
    }

    /**
     * Add patterns with the respective log data types
     *
     * @param patterns a {@link Map} of {@link String} filename glob patterns to their respective
     *        {@link LogDataType}s.  The globs should be absolute.
     * @see #setDefaultLogDataType
     */
    public void addPatterns(Map<String, LogDataType> patterns) {
        mFilePatterns.putAll(patterns);
    }

    /**
     * Set the default log data type set for patterns that don't have an associated type.
     *
     * @param type the {@link LogDataType}
     * @see #addPatterns(List)
     */
    public void setDefaultLogDataType(LogDataType type) {
        if (type == null) {
            throw new NullPointerException();
        }
        mDefaultFileType = type;
    }

    /**
     * Whether or not to skip files which have already been reported.  This is only relevant when
     * multiple patterns are being used, and two or more of those patterns match the same file.
     * <p />
     * Note that this <emph>must only</emph> be called prior to calling {@link #run()}. Doing
     * otherwise will cause undefined behavior.
     */
    public void setSkipRepeatFiles(boolean skip) {
        mSkipRepeatFiles = skip;
    }

    /**
     * Whether to <emph>attempt to</emph> infer the data types of {@code UNKNOWN} files by checking
     * the file extensions against a list.
     * <p />
     * Note that, when enabled, these inferences will only be made for patterns with file type
     * {@code UNKNOWN} (which includes patterns added without a specific type, and without the)
     * default type having been set manually).  If the inference fails, the data type will remain
     * as {@code UNKNOWN}.
     */
    public void setInferUnknownDataTypes(boolean infer) {
        mInferDataTypes = infer;
    }

    /**
     * Actually search the filesystem for the specified patterns and send them to
     * {@link ITestInvocationListener#testLog} if found
     */
    public List<String> run() throws DeviceNotAvailableException {
        List<String> filenames = new LinkedList<>();
        CLog.d(String.format("Analyzing %d patterns.", mFilePatterns.size()));
        for (Map.Entry<String, LogDataType> pat : mFilePatterns.entrySet()) {
            final String searchCmd = String.format("ls %s", pat.getKey());
            final String fileList = mDevice.executeShellCommand(searchCmd);

            for (String filename : fileList.split("\r?\n")) {
                filename = filename.trim();
                if (filename.isEmpty() || filename.endsWith(": No such file or directory")) {
                    continue;
                }
                if (mSkipRepeatFiles && mReportedFiles.contains(filename)) {
                    CLog.v("Skipping already-reported file %s", filename);
                    continue;
                }

                File file = null;
                InputStreamSource iss = null;
                try {
                    CLog.d("Trying to pull file '%s' from device %s", filename,
                        mDevice.getSerialNumber());
                    file = mDevice.pullFile(filename);
                    iss = createIssForFile(file);
                    final LogDataType type = getDataType(filename, pat.getValue());
                    CLog.d("Local file %s has size %d and type %s", file, file.length(),
                        type.getFileExt());
                    mListener.testLog(filename, type, iss);
                    filenames.add(filename);
                    mReportedFiles.add(filename);
                } finally {
                    StreamUtil.cancel(iss);
                    iss = null;
                    FileUtil.deleteFile(file);
                }
            }
        }
        return filenames;
    }

    /**
     * Returns the data type to use for a given file.  Will attempt to infer the data type from the
     * file's extension IFF inferences are enabled, and the current data type is {@code UNKNOWN}.
     */
    LogDataType getDataType(String filename, LogDataType defaultType) {
        if (!mInferDataTypes) return defaultType;
        if (!LogDataType.UNKNOWN.equals(defaultType)) return defaultType;

        CLog.d("Running type inference for file %s with default type %s", filename, defaultType);
        String ext = FileUtil.getExtension(filename);
        CLog.v("Found raw extension \"%s\"", ext);

        // Normalize the extension
        if (ext == null) return defaultType;
        ext = ext.toLowerCase();

        if (DATA_TYPE_REVERSE_MAP.containsKey(ext)) {
            final LogDataType newType = DATA_TYPE_REVERSE_MAP.get(ext);
            CLog.d("Inferred data type %s", newType);
            return newType;
        } else {
            CLog.v("Failed to find a reverse map for extension \"%s\"", ext);
            return defaultType;
        }
    }

    /**
     * Create an {@link InputStreamSource} for a file
     *
     * <p>Exposed for unit testing
     */
    InputStreamSource createIssForFile(File file) {
        return new FileInputStreamSource(file);
    }
}
