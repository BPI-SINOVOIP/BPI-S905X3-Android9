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
package android.car.storagemonitoring;

import android.annotation.IntRange;
import android.annotation.SystemApi;
import android.os.Parcel;
import android.os.Parcelable;
import android.util.JsonReader;
import android.util.JsonWriter;
import java.io.IOException;
import java.util.Objects;
import org.json.JSONException;
import org.json.JSONObject;

/**
 * Wear-out information for flash storage.
 *
 * Contains a lower-bound estimate of the wear of "type A" (SLC) and "type B" (MLC) storage.
 *
 * Current technology offers wear data in increments of 10% (i.e. from 0 == new device up to
 * 100 == worn-out device). It is possible for a storage device to only support one type of memory
 * cell, in which case it is expected that the storage type not supported will have UNKNOWN wear.
 *
 * @hide
 */
@SystemApi
public class WearEstimate implements Parcelable {
    public static final int UNKNOWN = -1;

    /** @hide */
    public static final WearEstimate UNKNOWN_ESTIMATE = new WearEstimate(UNKNOWN, UNKNOWN);

    public static final Parcelable.Creator<WearEstimate> CREATOR =
        new Parcelable.Creator<WearEstimate>() {
            public WearEstimate createFromParcel(Parcel in) {
                return new WearEstimate(in);
            }

            public WearEstimate[] newArray(int size) {
                return new WearEstimate[size];
            }
        };

    /**
     * Wear estimate data for "type A" storage.
     */
    @IntRange(from=-1, to=100)
    public final int typeA;

    /**
     * Wear estimate data for "type B" storage.
     */
    @IntRange(from=-1, to=100)
    public final int typeB;

    private static final int validateWearValue(int value) {
        if (value == UNKNOWN) return value;
        if ((value >= 0) && (value <= 100)) return value;
        throw new IllegalArgumentException(value + " is not a valid wear estimate");
    }

    public WearEstimate(int typeA, int typeB) {
        this.typeA = validateWearValue(typeA);
        this.typeB = validateWearValue(typeB);
    }

    public WearEstimate(Parcel in) {
        typeA = validateWearValue(in.readInt());
        typeB = validateWearValue(in.readInt());
    }

    public WearEstimate(JsonReader in) throws IOException {
        int typeA = UNKNOWN;
        int typeB = UNKNOWN;
        in.beginObject();
        while (in.hasNext()) {
            switch (in.nextName()) {
                case "wearEstimateTypeA":
                    typeA = validateWearValue(in.nextInt());
                    break;
                case "wearEstimateTypeB":
                    typeB = validateWearValue(in.nextInt());
                    break;
            }
        }
        in.endObject();
        this.typeA = typeA;
        this.typeB = typeB;
    }

    /** @hide */
    public WearEstimate(JSONObject in) throws JSONException {
        typeA = in.getInt("wearEstimateTypeA");
        typeB = in.getInt("wearEstimateTypeB");
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeInt(typeA);
        dest.writeInt(typeB);
    }

    public void writeToJson(JsonWriter jsonWriter) throws IOException {
        jsonWriter.beginObject();
        jsonWriter.name("wearEstimateTypeA").value(typeA);
        jsonWriter.name("wearEstimateTypeB").value(typeB);
        jsonWriter.endObject();
    }

    @Override
    public boolean equals(Object other) {
        if (other instanceof WearEstimate) {
            WearEstimate wo = (WearEstimate)other;
            return wo.typeA == typeA && wo.typeB == typeB;
        }
        return false;
    }

    @Override
    public int hashCode() {
        return Objects.hash(typeA, typeB);
    }

    private static final String wearValueToString(int value) {
        if (value == UNKNOWN) return "unknown";
        return value + "%";
    }

    @Override
    public String toString() {
        return "type A: " + wearValueToString(typeA) + ", type B: " + wearValueToString(typeB);
    }
}
