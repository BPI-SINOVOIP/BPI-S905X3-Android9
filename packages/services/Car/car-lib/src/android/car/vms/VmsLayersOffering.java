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
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Objects;
import java.util.Set;

/**
 * The state of dependencies for a single publisher.
 *
 * @hide
 */
@SystemApi
public final class VmsLayersOffering implements Parcelable {

    private final Set<VmsLayerDependency> mDependencies;

    private final int mPublisherId;

    public VmsLayersOffering(Set<VmsLayerDependency> dependencies, int publisherId) {
        mDependencies = Collections.unmodifiableSet(dependencies);
        mPublisherId = publisherId;
    }

    /**
     * Returns the dependencies.
     */
    public Set<VmsLayerDependency> getDependencies() {
        return mDependencies;
    }

    public int getPublisherId() {
        return mPublisherId;
    }

    public static final Parcelable.Creator<VmsLayersOffering> CREATOR = new
            Parcelable.Creator<VmsLayersOffering>() {
                public VmsLayersOffering createFromParcel(Parcel in) {
                    return new VmsLayersOffering(in);
                }

                public VmsLayersOffering[] newArray(int size) {
                    return new VmsLayersOffering[size];
                }
            };

    @Override
    public String toString() {
        return "VmsLayersOffering{ Publisher: " +
                mPublisherId +
                " Dependencies: " +
                mDependencies +
                "}";
    }

    @Override
    public void writeToParcel(Parcel out, int flags) {

        out.writeParcelableList(new ArrayList(mDependencies), flags);
        out.writeInt(mPublisherId);
    }

    @Override
    public boolean equals(Object o) {
        if (!(o instanceof VmsLayersOffering)) {
            return false;
        }
        VmsLayersOffering p = (VmsLayersOffering) o;
        return Objects.equals(p.mPublisherId, mPublisherId) && p.mDependencies.equals(mDependencies);
    }

    @Override
    public int hashCode() {
        return Objects.hash(mPublisherId, mDependencies);
    }

    @Override
    public int describeContents() {
        return 0;
    }

    private VmsLayersOffering(Parcel in) {
        List<VmsLayerDependency> dependencies = new ArrayList<>();
        in.readParcelableList(dependencies, VmsLayerDependency.class.getClassLoader());
        mDependencies = Collections.unmodifiableSet(new HashSet<>(dependencies));
        mPublisherId = in.readInt();
    }
}