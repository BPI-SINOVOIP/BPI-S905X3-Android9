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
package android.car.storagemonitoring;

import android.annotation.SystemApi;

/**
 * Record of data as extracted from /proc/uid_io/stats
 *
 * @hide
 */
@SystemApi
public final class UidIoRecord {

    public final int uid;

    public final long foreground_rchar;
    public final long foreground_wchar;
    public final long foreground_read_bytes;
    public final long foreground_write_bytes;
    public final long foreground_fsync;

    public final long background_rchar;
    public final long background_wchar;
    public final long background_read_bytes;
    public final long background_write_bytes;
    public final long background_fsync;

    public UidIoRecord(int uid,
            long foreground_rchar,
            long foreground_wchar,
            long foreground_read_bytes,
            long foreground_write_bytes,
            long foreground_fsync,
            long background_rchar,
            long background_wchar,
            long background_read_bytes,
            long background_write_bytes,
            long background_fsync) {
        this.uid = uid;

        this.foreground_rchar = foreground_rchar;
        this.foreground_wchar = foreground_wchar;
        this.foreground_read_bytes = foreground_read_bytes;
        this.foreground_write_bytes = foreground_write_bytes;
        this.foreground_fsync = foreground_fsync;

        this.background_rchar = background_rchar;
        this.background_wchar = background_wchar;
        this.background_read_bytes = background_read_bytes;
        this.background_write_bytes = background_write_bytes;
        this.background_fsync = background_fsync;
    }

    /** @hide */
    public UidIoRecord delta(IoStatsEntry other) {
        if (uid != other.uid) {
            throw new IllegalArgumentException("cannot calculate delta between different user IDs");
        }

        return new UidIoRecord(uid,
            foreground_rchar - other.foreground.bytesRead,
            foreground_wchar - other.foreground.bytesWritten,
            foreground_read_bytes - other.foreground.bytesReadFromStorage,
            foreground_write_bytes - other.foreground.bytesWrittenToStorage,
            foreground_fsync - other.foreground.fsyncCalls,
            background_rchar - other.background.bytesRead,
            background_wchar - other.background.bytesWritten,
            background_read_bytes - other.background.bytesReadFromStorage,
            background_write_bytes - other.background.bytesWrittenToStorage,
            background_fsync - other.background.fsyncCalls);
    }

    /** @hide */
    public UidIoRecord delta(UidIoRecord other) {
        if (uid != other.uid) {
            throw new IllegalArgumentException("cannot calculate delta between different user IDs");
        }

        return new UidIoRecord(uid,
            foreground_rchar - other.foreground_rchar,
            foreground_wchar - other.foreground_wchar,
            foreground_read_bytes - other.foreground_read_bytes,
            foreground_write_bytes - other.foreground_write_bytes,
            foreground_fsync - other.foreground_fsync,
            background_rchar - other.background_rchar,
            background_wchar - other.background_wchar,
            background_read_bytes - other.background_read_bytes,
            background_write_bytes - other.background_write_bytes,
            background_fsync - other.background_fsync);
    }
}
