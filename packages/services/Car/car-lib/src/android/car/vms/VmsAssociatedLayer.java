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

import java.util.*;

/**
 * A VMS Layer with a list of publisher IDs it is associated with.
 *
 * @hide
 */
public final class VmsAssociatedLayer implements Parcelable {

    // The VmsLayer.
    private final VmsLayer mLayer;

    // The IDs of the publishers that can publish this VmsLayer.
    private final Set<Integer> mPublisherIds;

    public VmsAssociatedLayer(VmsLayer layer, Set<Integer> publisherIds) {
        mLayer = layer;
        mPublisherIds = Collections.unmodifiableSet(publisherIds);
    }

    public VmsLayer getVmsLayer() {
        return mLayer;
    }

    public Set<Integer> getPublisherIds() {
        return mPublisherIds;
    }

    @Override
    public String toString() {
        return "VmsAssociatedLayer{ VmsLayer: " + mLayer + ", Publishers: " + mPublisherIds + "}";
    }

    // Parcelable related methods.
    public static final Parcelable.Creator<VmsAssociatedLayer> CREATOR =
            new Parcelable.Creator<VmsAssociatedLayer>() {
                public VmsAssociatedLayer createFromParcel(Parcel in) {
                    return new VmsAssociatedLayer(in);
                }

                public VmsAssociatedLayer[] newArray(int size) {
                    return new VmsAssociatedLayer[size];
                }
            };

    @Override
    public void writeToParcel(Parcel out, int flags) {
        out.writeParcelable(mLayer, flags);
        out.writeArray(mPublisherIds.toArray());
    }

    @Override
    public boolean equals(Object o) {
        if (!(o instanceof VmsAssociatedLayer)) {
            return false;
        }
        VmsAssociatedLayer p = (VmsAssociatedLayer) o;
        return Objects.equals(p.mLayer, mLayer) && p.mPublisherIds.equals(mPublisherIds);
    }

    @Override
    public int hashCode() {
        return Objects.hash(mLayer, mPublisherIds);
    }

    @Override
    public int describeContents() {
        return 0;
    }

    private VmsAssociatedLayer(Parcel in) {
        mLayer = in.readParcelable(VmsLayer.class.getClassLoader());

        Object[] objects = in.readArray(Integer.class.getClassLoader());
        Integer[] integers = Arrays.copyOf(objects, objects.length, Integer[].class);

        mPublisherIds = Collections.unmodifiableSet(
                new HashSet<>(Arrays.asList(integers)));
    }
}
