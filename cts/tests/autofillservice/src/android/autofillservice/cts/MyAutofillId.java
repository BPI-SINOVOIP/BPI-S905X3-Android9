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

package android.autofillservice.cts;

import android.os.Parcel;
import android.os.Parcelable;
import android.view.autofill.AutofillId;

public final class MyAutofillId implements Parcelable {

    private final AutofillId mId;

    public MyAutofillId(AutofillId id) {
        mId = id;
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = 1;
        result = prime * result + ((mId == null) ? 0 : mId.hashCode());
        return result;
    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj) return true;
        if (obj == null) return false;
        if (getClass() != obj.getClass()) return false;
        MyAutofillId other = (MyAutofillId) obj;
        if (mId == null) {
            if (other.mId != null) return false;
        } else if (!mId.equals(other.mId)) {
            return false;
        }
        return true;
    }

    @Override
    public String toString() {
        return mId.toString();
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeParcelable(mId, flags);
    }

    public static final Creator<MyAutofillId> CREATOR = new Creator<MyAutofillId>() {

        @Override
        public MyAutofillId createFromParcel(Parcel source) {
            return new MyAutofillId(source.readParcelable(null));
        }

        @Override
        public MyAutofillId[] newArray(int size) {
            return new MyAutofillId[size];
        }
    };
}
