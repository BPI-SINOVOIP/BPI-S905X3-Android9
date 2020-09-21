/*
 * Copyright (C) 2012 The Android Open Source Project
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

import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.FileUtil;

import java.io.File;
import java.io.IOException;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Date;

/**
 * Helper class for creating a .retention file in a directory.
 * Intended to be used by external tools to determine when a directory can be deleted.
 */
public class RetentionFileSaver {

    public static final String RETENTION_DATE_FORMAT = "yyyy-MM-dd HH:mm:ss zzz";
    public static final String RETENTION_FILE_NAME = ".retention";

    /**
     * Creates a .retention file in given dir with timestamp == current + logRetentionDays
     */
    public void writeRetentionFile(File dir, int logRetentionDays) {
        try {
            long deleteTimeEpoch = System.currentTimeMillis() + (long)logRetentionDays * 24 * 60 *
                    60 * 1000;
            Date date = new Date(deleteTimeEpoch);
            File retentionFile = new File(dir, RETENTION_FILE_NAME);
            FileUtil.writeToFile(new SimpleDateFormat(RETENTION_DATE_FORMAT).format(date),
                    retentionFile);
        } catch (IOException e) {
            CLog.e("Unable to create retention file in directory in %s", dir.getAbsolutePath());
            CLog.e(e);
        }
    }

    public boolean shouldDelete(File retentionFile) {
        if (!retentionFile.isFile() || !retentionFile.getName().equals(RETENTION_FILE_NAME)) {
            CLog.w("%s is not a retention file", retentionFile.getAbsolutePath());
            return false;
        }
        String timestamp;
        try {
            timestamp = FileUtil.readStringFromFile(retentionFile);
            Date retentionDate = new SimpleDateFormat(RETENTION_DATE_FORMAT).parse(timestamp);
            return new Date().after(retentionDate);
        } catch (IOException e) {
            CLog.e("Unable to read retention file %s", retentionFile.getAbsolutePath());
            CLog.e(e);
        } catch (ParseException e) {
            CLog.e("Unable to read timestamp in retention file %s", retentionFile.getAbsolutePath());
            CLog.e(e);
        }
        return false;
    }
}
