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

import android.os.Parcel;
import android.os.Parcelable;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Objects;
import java.util.Set;

/**
 * The list of layers with subscribers.
 *
 * @hide
 */
public final class VmsSubscriptionState implements Parcelable {
    private final int mSequenceNumber;
    private final Set<VmsLayer> mLayers;
    private final Set<VmsAssociatedLayer> mSubscribedLayersFromPublishers;

    /**
     * Construcs a summary of the state of the current subscriptions for publishers to consume
     * and adjust which layers that the are publishing.
     */
    public VmsSubscriptionState(int sequenceNumber,
                                Set<VmsLayer> subscribedLayers,
                                Set<VmsAssociatedLayer> layersFromPublishers) {
        mSequenceNumber = sequenceNumber;
        mLayers = Collections.unmodifiableSet(subscribedLayers);
        mSubscribedLayersFromPublishers = Collections.unmodifiableSet(layersFromPublishers);
    }

    /**
     * Returns the sequence number assigned by the VMS service. Sequence numbers are
     * monotonically increasing and help clients ignore potential out-of-order states.
     */
    public int getSequenceNumber() {
        return mSequenceNumber;
    }

    public Set<VmsLayer> getLayers() {
        return mLayers;
    }

    public Set<VmsAssociatedLayer> getAssociatedLayers() {
        return mSubscribedLayersFromPublishers;
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append("sequence number=").append(mSequenceNumber);
        sb.append("; layers={");
        for (VmsLayer layer : mLayers) {
            sb.append(layer).append(",");
        }
        sb.append("}");
        sb.append("; associatedLayers={");
        for (VmsAssociatedLayer layer : mSubscribedLayersFromPublishers) {
            sb.append(layer).append(",");
        }
        sb.append("}");
        return sb.toString();
    }

    public static final Parcelable.Creator<VmsSubscriptionState> CREATOR = new
            Parcelable.Creator<VmsSubscriptionState>() {
                public VmsSubscriptionState createFromParcel(Parcel in) {
                    return new VmsSubscriptionState(in);
                }

                public VmsSubscriptionState[] newArray(int size) {
                    return new VmsSubscriptionState[size];
                }
            };

    @Override
    public void writeToParcel(Parcel out, int flags) {
        out.writeInt(mSequenceNumber);
        out.writeParcelableList(new ArrayList(mLayers), flags);
        out.writeParcelableList(new ArrayList(mSubscribedLayersFromPublishers), flags);
    }

    @Override
    public boolean equals(Object o) {
        if (!(o instanceof VmsSubscriptionState)) {
            return false;
        }
        VmsSubscriptionState p = (VmsSubscriptionState) o;
        return Objects.equals(p.mSequenceNumber, mSequenceNumber) &&
                p.mLayers.equals(mLayers) &&
                p.mSubscribedLayersFromPublishers.equals(mSubscribedLayersFromPublishers);
    }

    @Override
    public int hashCode() {
        return Objects.hash(mSequenceNumber, mLayers, mSubscribedLayersFromPublishers);
    }

    @Override
    public int describeContents() {
        return 0;
    }

    private VmsSubscriptionState(Parcel in) {
        mSequenceNumber = in.readInt();

        List<VmsLayer> layers = new ArrayList<>();
        in.readParcelableList(layers, VmsLayer.class.getClassLoader());
        mLayers = Collections.unmodifiableSet(new HashSet(layers));

        List<VmsAssociatedLayer> associatedLayers = new ArrayList<>();
        in.readParcelableList(associatedLayers, VmsAssociatedLayer.class.getClassLoader());
        mSubscribedLayersFromPublishers =
                Collections.unmodifiableSet(new HashSet(associatedLayers));
    }
}