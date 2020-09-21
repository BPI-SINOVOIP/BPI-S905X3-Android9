/*
 * Copyright (C) 2014 The Android Open Source Project
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

package android.car.media;

import android.media.AudioDevicePort;
import android.media.AudioPatch;
import android.os.Parcel;
import android.os.Parcelable;

import com.android.internal.util.Preconditions;

/**
 * A class to encapsulate the handle for a system level audio patch. This is used
 * to provide a "safe" way for permitted applications to route automotive audio sources
 * outside of android.
 */
public final class CarAudioPatchHandle implements Parcelable {

    // This is enough information to uniquely identify a patch to the system
    private final int mHandleId;
    private final String mSourceAddress;
    private final String mSinkAddress;

    /**
     * Construct a audio patch handle container given the system level handle
     * NOTE: Assumes (as it true today), that there is exactly one device port in the source
     * and sink arrays.
     */
    public CarAudioPatchHandle(AudioPatch patch) {
        Preconditions.checkArgument(patch.sources().length == 1
                && patch.sources()[0].port() instanceof AudioDevicePort,
                "Accepts exactly one device port as source");
        Preconditions.checkArgument(patch.sinks().length == 1
                && patch.sinks()[0].port() instanceof AudioDevicePort,
                "Accepts exactly one device port as sink");

        mHandleId = patch.id();
        mSourceAddress = ((AudioDevicePort) patch.sources()[0].port()).address();
        mSinkAddress = ((AudioDevicePort) patch.sinks()[0].port()).address();
    }

    /**
     * Returns true if this instance matches the provided AudioPatch object.
     * This is intended only for use by the CarAudioManager implementation when
     * communicating with the AudioManager API.
     *
     * Effectively only the {@link #mHandleId} is used for comparison,
     * {@link android.media.AudioSystem#listAudioPatches(java.util.ArrayList, int[])}
     * does not populate the device type and address properly.
     *
     * @hide
     */
    public boolean represents(AudioPatch patch) {
        return patch.sources().length == 1
                && patch.sinks().length == 1
                && patch.id() == mHandleId;
    }

    @Override
    public String toString() {
        return "Patch (mHandleId=" + mHandleId + "): "
                + mSourceAddress + " => " + mSinkAddress;
    }

    /**
     * Given a parcel, populate our data members
     */
    private CarAudioPatchHandle(Parcel in) {
        mHandleId = in.readInt();
        mSourceAddress = in.readString();
        mSinkAddress = in.readString();
    }

    /**
     * Serialize our internal data to a parcel
     */
    @Override
    public void writeToParcel(Parcel out, int flags) {
        out.writeInt(mHandleId);
        out.writeString(mSourceAddress);
        out.writeString(mSinkAddress);
    }

    public static final Parcelable.Creator<CarAudioPatchHandle> CREATOR
            = new Parcelable.Creator<CarAudioPatchHandle>() {
        public CarAudioPatchHandle createFromParcel(Parcel in) {
            return new CarAudioPatchHandle(in);
        }

        public CarAudioPatchHandle[] newArray(int size) {
            return new CarAudioPatchHandle[size];
        }
    };

    @Override
    public int describeContents() {
        return 0;
    }
}
