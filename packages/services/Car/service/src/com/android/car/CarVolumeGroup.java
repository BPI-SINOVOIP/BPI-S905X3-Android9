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

package com.android.car;

import android.annotation.NonNull;
import android.annotation.Nullable;
import android.car.media.CarAudioManager;
import android.content.ContentResolver;
import android.content.Context;
import android.hardware.automotive.audiocontrol.V1_0.ContextNumber;
import android.media.AudioDevicePort;
import android.provider.Settings;
import android.util.SparseArray;
import android.util.SparseIntArray;

import com.android.internal.util.Preconditions;

import java.io.PrintWriter;
import java.util.Arrays;

/**
 * A class encapsulates a volume group in car.
 *
 * Volume in a car is controlled by group. A group holds one or more car audio contexts.
 * Call {@link CarAudioManager#getVolumeGroupCount()} to get the count of {@link CarVolumeGroup}
 * supported in a car.
 */
/* package */ final class CarVolumeGroup {

    private final ContentResolver mContentResolver;
    private final int mId;
    private final int[] mContexts;
    private final SparseIntArray mContextToBus = new SparseIntArray();
    private final SparseArray<CarAudioDeviceInfo> mBusToCarAudioDeviceInfos = new SparseArray<>();

    private int mDefaultGain = Integer.MIN_VALUE;
    private int mMaxGain = Integer.MIN_VALUE;
    private int mMinGain = Integer.MAX_VALUE;
    private int mStepSize = 0;
    private int mStoredGainIndex;
    private int mCurrentGainIndex = -1;

    CarVolumeGroup(Context context, int id, @NonNull int[] contexts) {
        mContentResolver = context.getContentResolver();
        mId = id;
        mContexts = contexts;

        mStoredGainIndex = Settings.Global.getInt(mContentResolver,
                CarAudioManager.getVolumeSettingsKeyForGroup(mId), -1);;
    }

    int getId() {
        return mId;
    }

    int[] getContexts() {
        return mContexts;
    }

    int[] getBusNumbers() {
        final int[] busNumbers = new int[mBusToCarAudioDeviceInfos.size()];
        for (int i = 0; i < busNumbers.length; i++) {
            busNumbers[i] = mBusToCarAudioDeviceInfos.keyAt(i);
        }
        return busNumbers;
    }

    /**
     * Binds the context number to physical bus number and audio device port information.
     * Because this may change the groups min/max values, thus invalidating an index computed from
     * a gain before this call, all calls to this function must happen at startup before any
     * set/getGainIndex calls.
     *
     * @param contextNumber Context number as defined in audio control HAL
     * @param busNumber Physical bus number for the audio device port
     * @param info {@link CarAudioDeviceInfo} instance relates to the physical bus
     */
    void bind(int contextNumber, int busNumber, CarAudioDeviceInfo info) {
        if (mBusToCarAudioDeviceInfos.size() == 0) {
            mStepSize = info.getAudioGain().stepValue();
        } else {
            Preconditions.checkArgument(
                    info.getAudioGain().stepValue() == mStepSize,
                    "Gain controls within one group must have same step value");
        }

        mContextToBus.put(contextNumber, busNumber);
        mBusToCarAudioDeviceInfos.put(busNumber, info);

        if (info.getDefaultGain() > mDefaultGain) {
            // We're arbitrarily selecting the highest bus default gain as the group's default.
            mDefaultGain = info.getDefaultGain();
        }
        if (info.getMaxGain() > mMaxGain) {
            mMaxGain = info.getMaxGain();
        }
        if (info.getMinGain() < mMinGain) {
            mMinGain = info.getMinGain();
        }
        if (mStoredGainIndex < getMinGainIndex() || mStoredGainIndex > getMaxGainIndex()) {
            // We expected to load a value from last boot, but if we didn't (perhaps this is the
            // first boot ever?), then use the highest "default" we've seen to initialize
            // ourselves.
            mCurrentGainIndex = getIndexForGain(mDefaultGain);
        } else {
            // Just use the gain index we stored last time the gain was set (presumably during our
            // last boot cycle).
            mCurrentGainIndex = mStoredGainIndex;
        }
    }

    int getDefaultGainIndex() {
        return getIndexForGain(mDefaultGain);
    }

    int getMaxGainIndex() {
        return getIndexForGain(mMaxGain);
    }

    int getMinGainIndex() {
        return getIndexForGain(mMinGain);
    }

    int getCurrentGainIndex() {
        return mCurrentGainIndex;
    }

    void setCurrentGainIndex(int gainIndex) {
        int gainInMillibels = getGainForIndex(gainIndex);

        Preconditions.checkArgument(
                gainInMillibels >= mMinGain && gainInMillibels <= mMaxGain,
                "Gain out of range (" +
                        mMinGain + ":" +
                        mMaxGain +") " +
                        gainInMillibels + "index " +
                        gainIndex);

        for (int i = 0; i < mBusToCarAudioDeviceInfos.size(); i++) {
            CarAudioDeviceInfo info = mBusToCarAudioDeviceInfos.valueAt(i);
            info.setCurrentGain(gainInMillibels);
        }

        mCurrentGainIndex = gainIndex;
        Settings.Global.putInt(mContentResolver,
                CarAudioManager.getVolumeSettingsKeyForGroup(mId), gainIndex);
    }

    // Given a group level gain index, return the computed gain in millibells
    // TODO (randolphs) If we ever want to add index to gain curves other than lock-stepped
    // linear, this would be the place to do it.
    private int getGainForIndex(int gainIndex) {
        return mMinGain + gainIndex * mStepSize;
    }

    // TODO (randolphs) if we ever went to a non-linear index to gain curve mapping, we'd need to
    // revisit this as it assumes (at the least) that getGainForIndex is reversible.  Luckily,
    // this is an internal implementation details we could factor out if/when necessary.
    private int getIndexForGain(int gainInMillibel) {
        return (gainInMillibel - mMinGain) / mStepSize;
    }

    @Nullable
    AudioDevicePort getAudioDevicePortForContext(int contextNumber) {
        final int busNumber = mContextToBus.get(contextNumber, -1);
        if (busNumber < 0 || mBusToCarAudioDeviceInfos.get(busNumber) == null) {
            return null;
        }
        return mBusToCarAudioDeviceInfos.get(busNumber).getAudioDevicePort();
    }

    @Override
    public String toString() {
        return "CarVolumeGroup id: " + mId
                + " currentGainIndex: " + mCurrentGainIndex
                + " contexts: " + Arrays.toString(mContexts)
                + " buses: " + Arrays.toString(getBusNumbers());
    }

    void dump(PrintWriter writer) {
        writer.println("CarVolumeGroup " + mId);
        writer.printf("\tGain in millibel (min / max / default/ current): %d %d %d %d\n",
                mMinGain, mMaxGain, mDefaultGain, getGainForIndex(mCurrentGainIndex));
        writer.printf("\tGain in index (min / max / default / current): %d %d %d %d\n",
                getMinGainIndex(), getMaxGainIndex(), getDefaultGainIndex(), mCurrentGainIndex);
        for (int i = 0; i < mContextToBus.size(); i++) {
            writer.printf("\tContext: %s -> Bus: %d\n",
                    ContextNumber.toString(mContextToBus.keyAt(i)), mContextToBus.valueAt(i));
        }
        for (int i = 0; i < mBusToCarAudioDeviceInfos.size(); i++) {
            mBusToCarAudioDeviceInfos.valueAt(i).dump(writer);
        }
        // Empty line for comfortable reading
        writer.println();
    }
}
