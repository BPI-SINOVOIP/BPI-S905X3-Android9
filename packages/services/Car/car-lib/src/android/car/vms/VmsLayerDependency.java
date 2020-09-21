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

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Objects;
import java.util.Set;

/**
 * A dependency for a VMS layer on other VMS layers.
 *
 * @hide
 */
@SystemApi
public final class VmsLayerDependency implements Parcelable {
    private final VmsLayer mLayer;
    private final Set<VmsLayer> mDependency;

    /**
     * Construct a dependency for layer on other layers.
     */
    public VmsLayerDependency(VmsLayer layer, Set<VmsLayer> dependencies) {
        mLayer = layer;
        mDependency = Collections.unmodifiableSet(dependencies);
    }

    /**
     * Constructs a layer without a dependency.
     */
    public VmsLayerDependency(VmsLayer layer) {
        mLayer = layer;
        mDependency = Collections.emptySet();
    }

    public VmsLayer getLayer() {
        return mLayer;
    }

    /**
     * Returns the dependencies.
     */
    public Set<VmsLayer> getDependencies() {
        return mDependency;
    }

    public static final Parcelable.Creator<VmsLayerDependency> CREATOR = new
            Parcelable.Creator<VmsLayerDependency>() {
                public VmsLayerDependency createFromParcel(Parcel in) {
                    return new VmsLayerDependency(in);
                }

                public VmsLayerDependency[] newArray(int size) {
                    return new VmsLayerDependency[size];
                }
            };

    public String toString() {
        return "VmsLayerDependency{ Layer: " + mLayer + " Dependency: " + mDependency + "}";
    }

    @Override
    public boolean equals(Object o) {
        if (!(o instanceof VmsLayerDependency)) {
            return false;
        }
        VmsLayerDependency p = (VmsLayerDependency) o;
        return Objects.equals(p.mLayer, mLayer) && p.mDependency.equals(mDependency);
    }

    @Override
    public int hashCode() {
        return Objects.hash(mLayer, mDependency);
    }

    @Override
    public void writeToParcel(Parcel out, int flags) {
        out.writeParcelable(mLayer, flags);
        out.writeParcelableList(new ArrayList<VmsLayer>(mDependency), flags);
    }

    @Override
    public int describeContents() {
        return 0;
    }

    private VmsLayerDependency(Parcel in) {
        mLayer = in.readParcelable(VmsLayer.class.getClassLoader());
        List<VmsLayer> dependency = new ArrayList<>();
        in.readParcelableList(dependency, VmsLayer.class.getClassLoader());
        mDependency = Collections.unmodifiableSet(new HashSet<VmsLayer>(dependency));
    }
}
