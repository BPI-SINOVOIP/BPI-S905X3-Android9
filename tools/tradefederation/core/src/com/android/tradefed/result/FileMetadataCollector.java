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
package com.android.tradefed.result;

import com.android.test.metrics.proto.FileMetadataProto.FileMetadata;
import com.android.test.metrics.proto.FileMetadataProto.LogFile;
import com.android.test.metrics.proto.FileMetadataProto.LogType;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.StreamUtil;

import com.google.common.annotations.VisibleForTesting;

import java.io.IOException;
import java.io.InputStream;

/** A listener that collects and uploads metadata about saved log files. */
@OptionClass(alias = "metadata")
public class FileMetadataCollector implements ILogSaverListener, ITestInvocationListener {
    @Option(name = "disable", description = "Disable metadata collecting")
    protected boolean mDisable = false;

    private ILogSaver mLogSaver = null;
    private FileMetadata.Builder mMetadataBuilder;

    /** Create a {@link FileMetadataCollector}. */
    public FileMetadataCollector() {
        super();
        mMetadataBuilder = FileMetadata.newBuilder();
    }

    /**
     * Query proto contents for testing.
     * @hide
     */
    @VisibleForTesting
    public FileMetadata getMetadataContents() {
        return mMetadataBuilder.build();
    }

    @Override
    public void testLogSaved(String dataName, LogDataType dataType, InputStreamSource source,
            com.android.tradefed.result.LogFile file) {
        if (mDisable) {
            return;
        }

        LogFile log =
                LogFile.newBuilder().setLogType(getLogType(dataType)).setName(dataName).build();
        mMetadataBuilder.addLogFiles(log);
    }

    @Override
    public void invocationEnded(long elapsedTime) {
        if (mDisable) {
            return;
        }
        // Log(save) the file contents to the result directory
        InputStreamSource source =
                new ByteArrayInputStreamSource(getMetadataContents().toByteArray());
        InputStream stream = null;
        try {
            stream = source.createInputStream();
            mLogSaver.saveLogDataRaw("metadata", LogDataType.TEXTPB, stream);
        } catch (IOException e) {
            CLog.e(e);
            CLog.e("Failed to save metadata.");
        } finally {
            StreamUtil.cancel(source);
            StreamUtil.close(stream);
        }
    }

    @Override
    public void setLogSaver(ILogSaver logSaver) {
        mLogSaver = logSaver;
    }

    private LogType getLogType(LogDataType type) {
        switch (type) {
            case BUGREPORT:
                return LogType.BUGREPORT;
            case BUGREPORTZ:
                return LogType.BUGREPORTZ;
            case LOGCAT:
                return LogType.LOGCAT;
            case KERNEL_LOG:
                return LogType.KERNEL;
            case MEM_INFO:
                return LogType.MEMINFO;
            case TOP:
                return LogType.TOP;
            case DUMPSYS:
                return LogType.DUMPSYS;
            case COMPACT_MEMINFO:
                return LogType.COMPACT_MEMINFO;
            case SERVICES:
                return LogType.SERVICES;
            case MUGSHOT_LOG:
                return LogType.MUGSHOT;
            default: // All others
                return LogType.UNKNOWN;
        }
    }
}
