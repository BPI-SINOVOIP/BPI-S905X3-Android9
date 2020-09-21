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
package com.android.car.storagemonitoring;

import android.annotation.Nullable;
import android.car.storagemonitoring.UidIoRecord;
import android.util.Log;
import android.util.SparseArray;
import com.android.car.CarLog;
import com.android.internal.annotations.VisibleForTesting;
import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;
import java.util.Objects;
import java.util.StringTokenizer;

/**
 * Loads I/O stats from procfs
 *
 * The Android kernel can be configured to provide uid I/O stats in /proc/uid_io/stats.
 */
public class ProcfsUidIoStatsProvider implements UidIoStatsProvider {
    private static Path DEFAULT_STATS_FILE = new File("/proc/uid_io/stats").toPath();

    private final Path mStatsFile;

    public ProcfsUidIoStatsProvider() {
        this(DEFAULT_STATS_FILE);
    }

    @VisibleForTesting
    ProcfsUidIoStatsProvider(Path statsFile) {
        mStatsFile = Objects.requireNonNull(statsFile);
    }

    @Nullable
    @Override
    public SparseArray<UidIoRecord> load() {
        List<String> lines;
        SparseArray<UidIoRecord> result = new SparseArray<>();
        try {
            lines = Files.readAllLines(mStatsFile);
        } catch (IOException e) {
            Log.w(CarLog.TAG_STORAGE, "can't read I/O stats from " + mStatsFile, e);
            return null;
        }

        for (String line : lines) {
            StringTokenizer tokenizer = new StringTokenizer(line);
            if (tokenizer.countTokens() != 11) {
                Log.w(CarLog.TAG_STORAGE, "malformed I/O stats entry: " + line);
                return null;
            }

            try {
                int uid = Integer.valueOf(tokenizer.nextToken());
                long foreground_rchar = Long.valueOf(tokenizer.nextToken());
                long foreground_wchar = Long.valueOf(tokenizer.nextToken());
                long foreground_read_bytes = Long.valueOf(tokenizer.nextToken());
                long foreground_write_bytes = Long.valueOf(tokenizer.nextToken());
                long background_rchar = Long.valueOf(tokenizer.nextToken());
                long background_wchar = Long.valueOf(tokenizer.nextToken());
                long background_read_bytes = Long.valueOf(tokenizer.nextToken());
                long background_write_bytes = Long.valueOf(tokenizer.nextToken());
                long foreground_fsync = Long.valueOf(tokenizer.nextToken());
                long background_fsync = Long.valueOf(tokenizer.nextToken());

                result.append(uid, new UidIoRecord(uid,
                            foreground_rchar,
                            foreground_wchar,
                            foreground_read_bytes,
                            foreground_write_bytes,
                            foreground_fsync,
                            background_rchar,
                            background_wchar,
                            background_read_bytes,
                            background_write_bytes,
                            background_fsync));

            } catch (NumberFormatException e) {
                Log.w(CarLog.TAG_STORAGE, "malformed I/O stats entry: " + line, e);
                return null;
            }
        }

        return result;
    }
}


