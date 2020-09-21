/**
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

package com.android.car.broadcastradio.support;

import android.annotation.NonNull;
import android.annotation.Nullable;
import android.graphics.Bitmap;
import android.hardware.radio.ProgramSelector;
import android.hardware.radio.RadioManager.ProgramInfo;
import android.os.Parcel;
import android.os.Parcelable;

import com.android.car.broadcastradio.support.platform.ProgramInfoExt;

import java.util.Objects;

/**
 * Holds storable information about a Program.
 *
 * Contrary to {@link android.hardware.radio.RadioManager.ProgramInfo}, it doesn't hold runtime
 * information, like artist or signal quality.
 */
public final class Program implements Parcelable {
    private final @NonNull ProgramSelector mSelector;
    private final @NonNull String mName;

    public Program(@NonNull ProgramSelector selector, @NonNull String name) {
        mSelector = Objects.requireNonNull(selector);
        mName = Objects.requireNonNull(name);
    }

    public @NonNull ProgramSelector getSelector() {
        return mSelector;
    }

    public @NonNull String getName() {
        return mName;
    }

    /** @hide */
    public @Nullable Bitmap getIcon() {
        // TODO(b/73950974): implement saving icons
        return null;
    }

    @Override
    public String toString() {
        return "Program(\"" + mName + "\", " + mSelector + ")";
    }

    @Override
    public int hashCode() {
        return mSelector.hashCode();
    }

    /**
     * Two programs are considered equal if their selectors are equal.
     */
    @Override
    public boolean equals(Object obj) {
        if (this == obj) return true;
        if (!(obj instanceof Program)) return false;
        Program other = (Program) obj;
        return mSelector.equals(other.mSelector);
    }

    /**
     * Builds a new {@link Program} object from {@link ProgramInfo}.
     */
    public static @NonNull Program fromProgramInfo(@NonNull ProgramInfo info) {
        return new Program(info.getSelector(), ProgramInfoExt.getProgramName(info, 0));
    }

    private Program(Parcel in) {
        mSelector = Objects.requireNonNull(in.readTypedObject(ProgramSelector.CREATOR));
        mName = Objects.requireNonNull(in.readString());
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeTypedObject(mSelector, 0);
        dest.writeString(mName);
    }

    @Override
    public int describeContents() {
        return 0;
    }

    public static final Parcelable.Creator<Program> CREATOR = new Parcelable.Creator<Program>() {
        public Program createFromParcel(Parcel in) {
            return new Program(in);
        }

        public Program[] newArray(int size) {
            return new Program[size];
        }
    };
}
