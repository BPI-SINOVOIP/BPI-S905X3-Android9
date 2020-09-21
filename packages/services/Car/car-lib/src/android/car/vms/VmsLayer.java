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

import java.util.Objects;

/**
 * A VMS Layer which can be subscribed to by VMS clients.
 *
 * @hide
 */
@SystemApi
public final class VmsLayer implements Parcelable {

    // The layer Type.
    private int mType;

    // The layer Subtype.
    private int mSubtype;

    // The layer version.
    private int mVersion;

    public VmsLayer(int type, int subtype, int version) {
        mType = type;
        mSubtype = subtype;
        mVersion = version;
    }

    public int getType() {
        return mType;
    }

    public int getSubtype() {
        return mSubtype;
    }

    public int getVersion() {
        return mVersion;
    }

    /**
     * Checks the two objects for equality by comparing their IDs and Versions.
     *
     * @param o the {@link VmsLayer} to which this one is to be checked for equality
     * @return true if the underlying objects of the VmsLayer are both considered equal
     */
    @Override
    public boolean equals(Object o) {
        if (!(o instanceof VmsLayer)) {
            return false;
        }
        VmsLayer p = (VmsLayer) o;
        return Objects.equals(p.mType, mType) &&
            Objects.equals(p.mSubtype, mSubtype) &&
            Objects.equals(p.mVersion, mVersion);
    }

    /**
     * Compute a hash code similarly tp {@link android.util.Pair}
     *
     * @return a hashcode of the Pair
     */
    @Override
    public int hashCode() {
        return Objects.hash(mType, mSubtype, mVersion);
    }

    @Override
    public String toString() {
        return "VmsLayer{ Type: " + mType + ", Sub type: " + mSubtype + ", Version: " + mVersion + "}";
    }


    // Parcelable related methods.
    public static final Parcelable.Creator<VmsLayer> CREATOR = new
            Parcelable.Creator<VmsLayer>() {
                public VmsLayer createFromParcel(Parcel in) {
                    return new VmsLayer(in);
                }

                public VmsLayer[] newArray(int size) {
                    return new VmsLayer[size];
                }
            };

    @Override
    public void writeToParcel(Parcel out, int flags) {
        out.writeInt(mType);
        out.writeInt(mSubtype);
        out.writeInt(mVersion);
    }

    @Override
    public int describeContents() {
        return 0;
    }

    private VmsLayer(Parcel in) {
        readFromParcel(in);
    }

    private void readFromParcel(Parcel in) {
        mType = in.readInt();
        mSubtype = in.readInt();
        mVersion = in.readInt();
    }
}