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
import android.os.Parcel;
import android.os.Parcelable;
import android.util.JsonWriter;
import java.io.IOException;
import java.util.Objects;
import org.json.JSONException;
import org.json.JSONObject;

/**
 * uid_io stats about one user ID.
 *
 * Contains information about I/O activity that can be attributed to processes running on
 * behalf of one user of the system, as collected by the kernel.
 *
 * @hide
 */
@SystemApi
public final class IoStatsEntry implements Parcelable {

    public static final Parcelable.Creator<IoStatsEntry> CREATOR =
        new Parcelable.Creator<IoStatsEntry>() {
            public IoStatsEntry createFromParcel(Parcel in) {
                return new IoStatsEntry(in);
            }

            public IoStatsEntry[] newArray(int size) {
                return new IoStatsEntry[size];
            }
        };

    /**
     * The user id that this object contains metrics for.
     *
     * In many cases this can be converted to a list of Java app packages installed on the device.
     * In other cases, the user id can refer to either the kernel itself (uid 0), or low-level
     * system services that are running entirely natively.
     */
    public final int uid;

    /**
     * How long any process running on behalf of this user id running for, in milliseconds.
     *
     * This field is allowed to be an approximation and it does not provide any way to
     * relate uptime to specific processes.
     */
    public final long runtimeMillis;

    /**
     * Statistics for apps running in foreground.
     */
    public final IoStatsEntry.Metrics foreground;

    /**
     * Statistics for apps running in background.
     */
    public final IoStatsEntry.Metrics background;

    public IoStatsEntry(int uid,
            long runtimeMillis, IoStatsEntry.Metrics foreground, IoStatsEntry.Metrics background) {
        this.uid = uid;
        this.runtimeMillis = runtimeMillis;
        this.foreground = Objects.requireNonNull(foreground);
        this.background = Objects.requireNonNull(background);
    }

    public IoStatsEntry(Parcel in) {
        uid = in.readInt();
        runtimeMillis = in.readLong();
        foreground = in.readParcelable(IoStatsEntry.Metrics.class.getClassLoader());
        background = in.readParcelable(IoStatsEntry.Metrics.class.getClassLoader());
    }

    public IoStatsEntry(UidIoRecord record, long runtimeMillis) {
        uid = record.uid;
        this.runtimeMillis = runtimeMillis;
        foreground = new IoStatsEntry.Metrics(record.foreground_rchar,
                record.foreground_wchar,
                record.foreground_read_bytes,
                record.foreground_write_bytes,
                record.foreground_fsync);
        background = new IoStatsEntry.Metrics(record.background_rchar,
            record.background_wchar,
            record.background_read_bytes,
            record.background_write_bytes,
            record.background_fsync);
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeInt(uid);
        dest.writeLong(runtimeMillis);
        dest.writeParcelable(foreground, flags);
        dest.writeParcelable(background, flags);
    }

    public void writeToJson(JsonWriter jsonWriter) throws IOException {
        jsonWriter.beginObject();
        jsonWriter.name("uid").value(uid);
        jsonWriter.name("runtimeMillis").value(runtimeMillis);
        jsonWriter.name("foreground"); foreground.writeToJson(jsonWriter);
        jsonWriter.name("background"); background.writeToJson(jsonWriter);
        jsonWriter.endObject();
    }

    public IoStatsEntry(JSONObject in) throws JSONException {
        uid = in.getInt("uid");
        runtimeMillis = in.getLong("runtimeMillis");
        foreground = new IoStatsEntry.Metrics(in.getJSONObject("foreground"));
        background = new IoStatsEntry.Metrics(in.getJSONObject("background"));
    }

    /**
     * Returns the difference between the values stored in this object vs. those
     * stored in other.
     *
     * It is the same as doing a delta() on foreground and background, plus verifying that
     * both objects refer to the same uid.
     *
     * @hide
     */
    public IoStatsEntry delta(IoStatsEntry other) {
        if (uid != other.uid) {
            throw new IllegalArgumentException("cannot calculate delta between different user IDs");
        }
        return new IoStatsEntry(uid,
                runtimeMillis - other.runtimeMillis,
                foreground.delta(other.foreground), background.delta(other.background));
    }

    @Override
    public boolean equals(Object other) {
        if (other instanceof IoStatsEntry) {
            IoStatsEntry uidIoStatEntry = (IoStatsEntry)other;

            return uid == uidIoStatEntry.uid &&
                    runtimeMillis == uidIoStatEntry.runtimeMillis &&
                    foreground.equals(uidIoStatEntry.foreground) &&
                    background.equals(uidIoStatEntry.background);
        }

        return false;
    }

    @Override
    public int hashCode() {
        return Objects.hash(uid, runtimeMillis, foreground, background);
    }

    @Override
    public String toString() {
        return String.format("uid = %d, runtime = %d, foreground = %s, background = %s",
            uid, runtimeMillis, foreground, background);
    }

    /**
     * Validates that this object contains the same I/O metrics as a UidIoStatsRecord.
     *
     * It matches UID, and I/O activity values, but ignores runtime.
     * @hide
     */
    public boolean representsSameMetrics(UidIoRecord record) {
        return record.uid == uid &&
               record.foreground_rchar == foreground.bytesRead &&
               record.foreground_wchar == foreground.bytesWritten &&
               record.foreground_read_bytes == foreground.bytesReadFromStorage &&
               record.foreground_write_bytes == foreground.bytesWrittenToStorage &&
               record.foreground_fsync == foreground.fsyncCalls &&
               record.background_rchar == background.bytesRead &&
               record.background_wchar == background.bytesWritten &&
               record.background_read_bytes == background.bytesReadFromStorage &&
               record.background_write_bytes == background.bytesWrittenToStorage &&
               record.background_fsync == background.fsyncCalls;
    }

    /**
     * I/O activity metrics that pertain to either the foreground or the background state.
     */
    public static final class Metrics implements Parcelable {

        public static final Parcelable.Creator<IoStatsEntry.Metrics> CREATOR =
            new Parcelable.Creator<IoStatsEntry.Metrics>() {
                public IoStatsEntry.Metrics createFromParcel(Parcel in) {
                    return new IoStatsEntry.Metrics(in);
                }

                public IoStatsEntry.Metrics[] newArray(int size) {
                    return new IoStatsEntry.Metrics[size];
                }
            };

        /**
         * Total bytes that processes running on behalf of this user obtained
         * via read() system calls.
         */
        public final long bytesRead;

        /**
         * Total bytes that processes running on behalf of this user transferred
         * via write() system calls.
         */
        public final long bytesWritten;

        /**
         * Total bytes that processes running on behalf of this user obtained
         * via read() system calls that actually were served by physical storage.
         */
        public final long bytesReadFromStorage;

        /**
         * Total bytes that processes running on behalf of this user transferred
         * via write() system calls that were actually sent to physical storage.
         */
        public final long bytesWrittenToStorage;

        /**
         * Total number of fsync() system calls that processes running on behalf of this user made.
         */
        public final long fsyncCalls;

        public Metrics(long bytesRead, long bytesWritten, long bytesReadFromStorage,
            long bytesWrittenToStorage, long fsyncCalls) {
            this.bytesRead = bytesRead;
            this.bytesWritten = bytesWritten;
            this.bytesReadFromStorage = bytesReadFromStorage;
            this.bytesWrittenToStorage = bytesWrittenToStorage;
            this.fsyncCalls = fsyncCalls;
        }

        @Override
        public int describeContents() {
            return 0;
        }

        @Override
        public void writeToParcel(Parcel dest, int flags) {
            dest.writeLong(bytesRead);
            dest.writeLong(bytesWritten);
            dest.writeLong(bytesReadFromStorage);
            dest.writeLong(bytesWrittenToStorage);
            dest.writeLong(fsyncCalls);
        }

        public void writeToJson(JsonWriter jsonWriter) throws IOException {
            jsonWriter.beginObject();
            jsonWriter.name("bytesRead").value(bytesRead);
            jsonWriter.name("bytesWritten").value(bytesWritten);
            jsonWriter.name("bytesReadFromStorage").value(bytesReadFromStorage);
            jsonWriter.name("bytesWrittenToStorage").value(bytesWrittenToStorage);
            jsonWriter.name("fsyncCalls").value(fsyncCalls);
            jsonWriter.endObject();
        }

        public Metrics(Parcel in) {
            bytesRead = in.readLong();
            bytesWritten = in.readLong();
            bytesReadFromStorage = in.readLong();
            bytesWrittenToStorage = in.readLong();
            fsyncCalls = in.readLong();
        }

        public Metrics(JSONObject in) throws JSONException {
            bytesRead = in.getLong("bytesRead");
            bytesWritten = in.getLong("bytesWritten");
            bytesReadFromStorage = in.getLong("bytesReadFromStorage");
            bytesWrittenToStorage = in.getLong("bytesWrittenToStorage");
            fsyncCalls = in.getLong("fsyncCalls");
        }

        /**
         * Computes the difference between the values stored in this object
         * vs. those stored in other
         *
         * It is the same as doing
         * new PerStateMetrics(bytesRead-other.bytesRead,bytesWritten-other.bytesWritten, ...)
         *
         * @hide
         */
        public Metrics delta(Metrics other) {
            return new Metrics(bytesRead-other.bytesRead,
                bytesWritten-other.bytesWritten,
                bytesReadFromStorage-other.bytesReadFromStorage,
                bytesWrittenToStorage-other.bytesWrittenToStorage,
                fsyncCalls-other.fsyncCalls);
        }

        @Override
        public boolean equals(Object other) {
            if (other instanceof Metrics) {
                Metrics metrics = (Metrics)other;

                return (bytesRead == metrics.bytesRead) &&
                    (bytesWritten == metrics.bytesWritten) &&
                    (bytesReadFromStorage == metrics.bytesReadFromStorage) &&
                    (bytesWrittenToStorage == metrics.bytesWrittenToStorage) &&
                    (fsyncCalls == metrics.fsyncCalls);
            }
            return false;
        }

        @Override
        public int hashCode() {
            return Objects.hash(bytesRead, bytesWritten, bytesReadFromStorage,
                bytesWrittenToStorage, fsyncCalls);
        }

        @Override
        public String toString() {
            return String.format("bytesRead=%d, bytesWritten=%d, bytesReadFromStorage=%d, bytesWrittenToStorage=%d, fsyncCalls=%d",
                bytesRead, bytesWritten, bytesReadFromStorage, bytesWrittenToStorage, fsyncCalls);
        }
    }
}
