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

import android.annotation.NonNull;
import android.car.storagemonitoring.WearEstimate;
import android.car.storagemonitoring.WearEstimateChange;
import android.util.JsonWriter;
import java.io.IOException;
import java.time.Instant;
import java.util.Objects;
import org.json.JSONException;
import org.json.JSONObject;

/**
 * This class represents a wear estimate record as stored by CarStorageMonitoringService.
 *
 * Because it is meant to map 1:1 to on-disk records, it is not directly convertible to a
 * WearEstimateChange because it does not include information about "acceptable degradation".
 */
public class WearEstimateRecord {

    private final WearEstimate mOldWearEstimate;
    private final WearEstimate mNewWearEstimate;
    private final long mTotalCarServiceUptime;
    private final Instant mUnixTimestamp;

    public WearEstimateRecord(@NonNull WearEstimate oldWearEstimate,
        @NonNull WearEstimate newWearEstimate,
        long totalCarServiceUptime,
        @NonNull Instant unixTimestamp) {
        mOldWearEstimate = Objects.requireNonNull(oldWearEstimate);
        mNewWearEstimate = Objects.requireNonNull(newWearEstimate);
        mTotalCarServiceUptime = totalCarServiceUptime;
        mUnixTimestamp = Objects.requireNonNull(unixTimestamp);
    }

    WearEstimateRecord(@NonNull JSONObject json) throws JSONException {
        mOldWearEstimate = new WearEstimate(json.getJSONObject("oldWearEstimate"));
        mNewWearEstimate = new WearEstimate(json.getJSONObject("newWearEstimate"));
        mTotalCarServiceUptime = json.getLong("totalCarServiceUptime");
        mUnixTimestamp = Instant.ofEpochMilli(json.getLong("unixTimestamp"));

    }

    void writeToJson(@NonNull JsonWriter jsonWriter) throws IOException {
        jsonWriter.beginObject();
        jsonWriter.name("oldWearEstimate"); mOldWearEstimate.writeToJson(jsonWriter);
        jsonWriter.name("newWearEstimate"); mNewWearEstimate.writeToJson(jsonWriter);
        jsonWriter.name("totalCarServiceUptime").value(mTotalCarServiceUptime);
        jsonWriter.name("unixTimestamp").value(mUnixTimestamp.toEpochMilli());
        jsonWriter.endObject();
    }

    public WearEstimate getOldWearEstimate() {
        return mOldWearEstimate;
    }

    public WearEstimate getNewWearEstimate() {
        return mNewWearEstimate;
    }

    public long getTotalCarServiceUptime() {
        return mTotalCarServiceUptime;
    }

    public Instant getUnixTimestamp() {
        return mUnixTimestamp;
    }

    WearEstimateChange toWearEstimateChange(boolean isAcceptableDegradation) {
        return new WearEstimateChange(mOldWearEstimate,
                mNewWearEstimate, mTotalCarServiceUptime, mUnixTimestamp, isAcceptableDegradation);
    }

    @Override
    public boolean equals(Object other) {
        if (other instanceof WearEstimateRecord) {
            WearEstimateRecord wer = (WearEstimateRecord)other;
            if (!wer.mOldWearEstimate.equals(mOldWearEstimate)) return false;
            if (!wer.mNewWearEstimate.equals(mNewWearEstimate)) return false;
            if (wer.mTotalCarServiceUptime != mTotalCarServiceUptime) return false;
            if (!wer.mUnixTimestamp.equals(mUnixTimestamp)) return false;
            return true;
        }
        return false;
    }

    /**
     * Checks whether this record tracks the same change as the provided estimate.
     * That means the two objects have the same values for:
     * <ul>
     *  <li>old wear indicators</li>
     *  <li>new wear indicators</li>
     *  <li>uptime at event</li>
     * </ul>
     */
    public boolean isSameAs(@NonNull WearEstimateChange wearEstimateChange) {
        if (!mOldWearEstimate.equals(wearEstimateChange.oldEstimate)) return false;
        if (!mNewWearEstimate.equals(wearEstimateChange.newEstimate)) return false;
        return (mTotalCarServiceUptime == wearEstimateChange.uptimeAtChange);
    }

    @Override
    public int hashCode() {
        return Objects.hash(mOldWearEstimate,
                mNewWearEstimate, mTotalCarServiceUptime, mUnixTimestamp);
    }

    @Override
    public String toString() {
        return String.format("WearEstimateRecord {" +
            "mOldWearEstimate = %s, " +
            "mNewWearEstimate = %s, " +
            "mTotalCarServiceUptime = %d, " +
            "mUnixTimestamp = %s}",
            mOldWearEstimate, mNewWearEstimate, mTotalCarServiceUptime, mUnixTimestamp);
    }

    public static final class Builder {
        private WearEstimate mOldWearEstimate = null;
        private WearEstimate mNewWearEstimate = null;
        private long mTotalCarServiceUptime = -1;
        private Instant mUnixTimestamp = null;

        private Builder() {}

        public static Builder newBuilder() {
            return new Builder();
        }

        public Builder fromWearEstimate(@NonNull WearEstimate wearEstimate) {
            mOldWearEstimate = Objects.requireNonNull(wearEstimate);
            return this;
        }

        public Builder toWearEstimate(@NonNull WearEstimate wearEstimate) {
            mNewWearEstimate = Objects.requireNonNull(wearEstimate);
            return this;
        }

        public Builder atUptime(long uptime) {
            if (uptime < 0) {
                throw new IllegalArgumentException("uptime must be >= 0");
            }
            mTotalCarServiceUptime = uptime;
            return this;
        }

        public Builder atTimestamp(@NonNull Instant now) {
            mUnixTimestamp = Objects.requireNonNull(now);
            return this;
        }

        public WearEstimateRecord build() {
            if (mOldWearEstimate == null || mNewWearEstimate == null ||
                    mTotalCarServiceUptime < 0 || mUnixTimestamp == null) {
                throw new IllegalStateException("malformed builder state");
            }
            return new WearEstimateRecord(
                    mOldWearEstimate, mNewWearEstimate, mTotalCarServiceUptime, mUnixTimestamp);
        }
    }
}
