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

package android.car.vms;

import android.annotation.SystemApi;
import android.os.Parcel;
import android.os.Parcelable;


import java.util.Set;
import java.util.ArrayList;
import java.util.List;
import java.util.HashSet;
import java.util.Collections;

import java.util.Objects;

/**
 * VMS Layers that can be subscribed to by VMS clients.
 *
 * @hide
 */
@SystemApi
public final class VmsAvailableLayers implements Parcelable {

    // A sequence number.
    private final int mSeq;

    // The list of AssociatedLayers
    private final Set<VmsAssociatedLayer> mAssociatedLayers;

    public VmsAvailableLayers(Set<VmsAssociatedLayer> associatedLayers, int sequence) {
        mSeq = sequence;

        mAssociatedLayers = Collections.unmodifiableSet(associatedLayers);
    }

    public int getSequence() {
        return mSeq;
    }

    public Set<VmsAssociatedLayer> getAssociatedLayers() {
        return mAssociatedLayers;
    }

    @Override
    public String toString() {
        return "VmsAvailableLayers{ seq: " +
                mSeq +
                ", AssociatedLayers: " +
                mAssociatedLayers +
                "}";
    }


    // Parcelable related methods.
    public static final Parcelable.Creator<VmsAvailableLayers> CREATOR = new
            Parcelable.Creator<VmsAvailableLayers>() {
                public VmsAvailableLayers createFromParcel(Parcel in) {
                    return new VmsAvailableLayers(in);
                }

                public VmsAvailableLayers[] newArray(int size) {
                    return new VmsAvailableLayers[size];
                }
            };

    @Override
    public void writeToParcel(Parcel out, int flags) {
        out.writeInt(mSeq);
        out.writeParcelableList(new ArrayList(mAssociatedLayers), flags);
    }

    @Override
    public boolean equals(Object o) {
        if (!(o instanceof VmsAvailableLayers)) {
            return false;
        }
        VmsAvailableLayers p = (VmsAvailableLayers) o;
        return Objects.equals(p.mAssociatedLayers, mAssociatedLayers) &&
                p.mSeq == mSeq;
    }

    @Override
    public int describeContents() {
        return 0;
    }

    private VmsAvailableLayers(Parcel in) {
        mSeq = in.readInt();

        List<VmsAssociatedLayer> associatedLayers = new ArrayList<>();
        in.readParcelableList(associatedLayers, VmsAssociatedLayer.class.getClassLoader());
        mAssociatedLayers = Collections.unmodifiableSet(new HashSet(associatedLayers));
    }
}