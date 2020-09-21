/*
 * Copyright (C) 2018 The Android Open Source Project
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

package android.car.settings;

import android.os.Parcel;
import android.os.Parcelable;

import java.util.Objects;

/**
 * A configuration struct that holds information for tweaking SpeedBump settings.
 *
 * @see androidx.car.moderator.SpeedBumpView
 */
public final class SpeedBumpConfiguration implements Parcelable {
    private final double mAcquiredPermitsPerSecond;
    private final double mMaxPermitPool;
    private final long mPermitFillDelay;

    public SpeedBumpConfiguration(double permitsPerSecond, double maxPermitPool,
            long permitFillDelay) {
        mAcquiredPermitsPerSecond = permitsPerSecond;
        mMaxPermitPool = maxPermitPool;
        mPermitFillDelay = permitFillDelay;
    }

    /**
     * Returns the number of permitted actions that are acquired each second that the user has not
     * interacted with the {@code SpeedBumpView}.
     */
    public double getAcquiredPermitsPerSecond() {
        return mAcquiredPermitsPerSecond;
    }

    /**
     * Returns the maximum number of permits that can be acquired when the user is idling.
     */
    public double getMaxPermitPool() {
        return mMaxPermitPool;
    }

    /**
     * Returns the delay time before when the permit pool has been depleted and when it begins to
     * refill.
     */
    public long getPermitFillDelay() {
        return mPermitFillDelay;
    }

    @Override
    public String toString() {
        return String.format(
                "[acquired_permits_per_second: %f, max_permit_pool: %f, permit_fill_delay: %d]",
                mAcquiredPermitsPerSecond,
                mMaxPermitPool,
                mPermitFillDelay);
    }

    @Override
    public boolean equals(Object object) {
        if (object == this) {
            return true;
        }

        if (!(object instanceof SpeedBumpConfiguration)) {
            return false;
        }

        SpeedBumpConfiguration other = (SpeedBumpConfiguration) object;
        return mAcquiredPermitsPerSecond == other.getAcquiredPermitsPerSecond()
                && mMaxPermitPool == other.getMaxPermitPool()
                && mPermitFillDelay == other.getPermitFillDelay();
    }

    @Override
    public int hashCode() {
        return Objects.hash(mAcquiredPermitsPerSecond, mMaxPermitPool, mPermitFillDelay);
    }

    @Override
    public void writeToParcel(Parcel desk, int flags) {
        desk.writeDouble(mAcquiredPermitsPerSecond);
        desk.writeDouble(mMaxPermitPool);
        desk.writeLong(mPermitFillDelay);
    }

    @Override
    public int describeContents() {
        return 0;
    }

    private SpeedBumpConfiguration(Parcel in) {
        mAcquiredPermitsPerSecond = in.readDouble();
        mMaxPermitPool = in.readDouble();
        mPermitFillDelay = in.readLong();
    }

    public static final Parcelable.Creator<SpeedBumpConfiguration> CREATOR =
            new Parcelable.Creator<SpeedBumpConfiguration>() {
        public SpeedBumpConfiguration createFromParcel(Parcel in) {
            return new SpeedBumpConfiguration(in);
        }

        public SpeedBumpConfiguration[] newArray(int size) {
            return new SpeedBumpConfiguration[size];
        }
    };
}
