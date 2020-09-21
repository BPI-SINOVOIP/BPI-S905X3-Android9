/*
 * Copyright 2017 The Android Open Source Project
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

package androidx.recyclerview.selection.testing;

import android.os.Bundle;
import android.os.Parcel;

public final class Bundles {

    private Bundles() {
    }

    public static Bundle forceParceling(Bundle in) {
        Parcel parcel = Parcel.obtain();
        in.writeToParcel(parcel, 0);

        parcel.setDataPosition(0);
        return parcel.readBundle();
    }
}
