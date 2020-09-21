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

package com.android.car;

import android.util.JsonReader;
import android.util.JsonWriter;
import android.util.Log;

import com.android.car.systeminterface.SystemInterface;

import com.android.car.systeminterface.TimeInterface;
import com.android.internal.annotations.VisibleForTesting;

import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.util.Objects;
import java.util.Optional;

/**
 * A class that can keep track of how long its instances are alive for.
 *
 * It can be used as a helper object to track the lifetime of system components, e.g.
 *
 * class InterestingService {
 *     private UptimeTracker mTracker;
 *
 *     public void onCreate() {
 *         mTracker = new UptimeTracker(
 *             "/storage/emulated/0/Android/data/interestingservice.uptime", 1 hour);
 *         mTracker.onCreate();
 *     }
 *
 *     public void onDestroy() {
 *         mTracker.onDestroy();
 *     }
 * }
 *
 * Now it's possible to know how long InterestingService has been alive in the system by querying
 * mTracker.getTotalUptime(). Because this data is stored to disk, the uptime is maintained across
 * process and system reboot boundaries. It is possible to configure periodic snapshot points to
 * ensure that crashes do not cause more than a certain amount of uptime to go untracked.
 */
public class UptimeTracker {
    /**
     * In order to prevent excessive wear-out of the storage, do not allow snapshots to happen
     * more frequently than this value
     */
    public static final long MINIMUM_SNAPSHOT_INTERVAL_MS = 60 * 60 * 1000;

    /**
     * The default snapshot interval if none is given
     */
    private static long DEFAULT_SNAPSHOT_INTERVAL_MS = 5 * 60 * 60 * 1000; // 5 hours

    private final Object mLock = new Object();

    /**
     * The file that uptime metrics are stored to
     */
    private File mUptimeFile;

    /**
     * The uptime value retrieved from mUptimeFile
     */
    private Optional<Long> mHistoricalUptime;

    /**
     * Last value of elapsedRealTime read from the system
     */
    private long mLastRealTimeSnapshot;

    /**
     * The source of real-time and scheduling
     */
    private TimeInterface mTimeInterface;

    public UptimeTracker(File file) {
        this(file, DEFAULT_SNAPSHOT_INTERVAL_MS);
    }

    public UptimeTracker(File file, long snapshotInterval) {
        this(file, snapshotInterval, new TimeInterface.DefaultImpl());
    }

    UptimeTracker(File file, long snapshotInterval, SystemInterface systemInterface) {
        this(file, snapshotInterval, systemInterface.getTimeInterface());
    }

    // This constructor allows one to replace the source of time-based truths with
    // a mock version. This is mostly useful for testing purposes.
    @VisibleForTesting
    UptimeTracker(File file,
            long snapshotInterval,
            TimeInterface timeInterface) {
        snapshotInterval = Math.max(snapshotInterval, MINIMUM_SNAPSHOT_INTERVAL_MS);
        mUptimeFile = Objects.requireNonNull(file);
        mTimeInterface = timeInterface;
        mLastRealTimeSnapshot = mTimeInterface.getUptime(TimeInterface.EXCLUDE_DEEP_SLEEP_TIME);
        mHistoricalUptime = Optional.empty();

        mTimeInterface.scheduleAction(this::flushSnapshot, snapshotInterval);
    }

    void onDestroy() {
        synchronized (mLock) {
            if (mTimeInterface != null) {
                mTimeInterface.cancelAllActions();
            }
            flushSnapshot();
            mTimeInterface = null;
            mUptimeFile = null;
        }
    }

    /**
     * Return the total amount of uptime that has been observed, in milliseconds.
     *
     * This is the sum of the uptime stored on disk + the uptime seen since the last snapshot.
     */
    long getTotalUptime() {
        synchronized (mLock) {
            if (mTimeInterface == null) {
                return 0;
            }
            return getHistoricalUptimeLocked() + (
                    mTimeInterface.getUptime(TimeInterface.EXCLUDE_DEEP_SLEEP_TIME)
                            - mLastRealTimeSnapshot);
        }
    }

    private long getHistoricalUptimeLocked() {
        if (!mHistoricalUptime.isPresent() && mUptimeFile != null && mUptimeFile.exists()) {
            try {
                JsonReader reader = new JsonReader(new FileReader(mUptimeFile));
                reader.beginObject();
                if (!reader.nextName().equals("uptime")) {
                    throw new IllegalArgumentException(
                        mUptimeFile + " is not in a valid format");
                } else {
                    mHistoricalUptime = Optional.of(reader.nextLong());
                }
                reader.endObject();
                reader.close();
            } catch (IllegalArgumentException | IOException e) {
                Log.w(CarLog.TAG_SERVICE, "unable to read historical uptime data", e);
                mHistoricalUptime = Optional.empty();
            }
        }
        return mHistoricalUptime.orElse(0L);
    }

    private void flushSnapshot() {
        synchronized (mLock) {
            if (mUptimeFile == null) {
                return;
            }
            try {
                long newUptime = getTotalUptime();
                mHistoricalUptime = Optional.of(newUptime);
                mLastRealTimeSnapshot = mTimeInterface.getUptime(
                        TimeInterface.EXCLUDE_DEEP_SLEEP_TIME);

                JsonWriter writer = new JsonWriter(new FileWriter(mUptimeFile));
                writer.beginObject();
                writer.name("uptime");
                writer.value(newUptime);
                writer.endObject();
                writer.close();
            } catch (IOException e) {
                Log.w(CarLog.TAG_SERVICE, "unable to write historical uptime data", e);
            }
        }
    }
}
