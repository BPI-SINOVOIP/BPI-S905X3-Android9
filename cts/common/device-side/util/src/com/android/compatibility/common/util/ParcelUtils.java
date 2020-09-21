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
package com.android.compatibility.common.util;

import static org.junit.Assert.assertNotNull;

import android.os.Parcel;
import android.os.Parcelable;

public class ParcelUtils {
    private ParcelUtils() {
    }

    /** Convert a Parcelable into a byte[]. */
    public static byte[] toBytes(Parcelable p) {
        assertNotNull(p);

        final Parcel parcel = Parcel.obtain();
        parcel.writeParcelable(p, 0);
        byte[] data = parcel.marshall();
        parcel.recycle();

        return data;
    }

    /** Decode a byte[] into a Parcelable. */
    public static <T extends Parcelable> T fromBytes(byte[] data) {
        assertNotNull(data);

        final Parcel parcel = Parcel.obtain();
        parcel.unmarshall(data, 0, data.length);
        parcel.setDataPosition(0);
        T ret = parcel.readParcelable(ParcelUtils.class.getClassLoader());
        parcel.recycle();

        return ret;
    }
}
