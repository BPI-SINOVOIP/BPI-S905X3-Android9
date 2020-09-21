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

package android.app;

import android.os.Parcel;
import android.os.Parcelable;

import java.io.PrintWriter;

/**
 * Display properties to be used by VR mode when creating a virtual display.
 *
 * @hide
 */
public final class Vr2dDisplayProperties implements Parcelable {

    public static final int FLAG_VIRTUAL_DISPLAY_ENABLED = 1;

    /**
     * The actual width, height and dpi.
     */
    private final int mWidth;
    private final int mHeight;
    private final int mDpi;

    // Flags describing the virtual display behavior.
    private final int mAddedFlags;
    private final int mRemovedFlags;

    public Vr2dDisplayProperties(int width, int height, int dpi) {
        this(width, height, dpi, 0, 0);
    }

    private Vr2dDisplayProperties(int width, int height, int dpi, int flags, int removedFlags) {
        mWidth = width;
        mHeight = height;
        mDpi = dpi;
        mAddedFlags = flags;
        mRemovedFlags = removedFlags;
    }

    @Override
    public int hashCode() {
        int result = getWidth();
        result = 31 * result + getHeight();
        result = 31 * result + getDpi();
        return result;
    }

    @Override
    public String toString() {
        return "Vr2dDisplayProperties{"
                + "mWidth=" + mWidth
                + ", mHeight=" + mHeight
                + ", mDpi=" + mDpi
                + ", flags=" + toReadableFlags(mAddedFlags)
                + ", removed_flags=" + toReadableFlags(mRemovedFlags)
                + "}";
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;

        Vr2dDisplayProperties that = (Vr2dDisplayProperties) o;

        if (getFlags() != that.getFlags()) return false;
        if (getRemovedFlags() != that.getRemovedFlags()) return false;
        if (getWidth() != that.getWidth()) return false;
        if (getHeight() != that.getHeight()) return false;
        return getDpi() == that.getDpi();
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeInt(mWidth);
        dest.writeInt(mHeight);
        dest.writeInt(mDpi);
        dest.writeInt(mAddedFlags);
        dest.writeInt(mRemovedFlags);
    }

    public static final Parcelable.Creator<Vr2dDisplayProperties> CREATOR
            = new Parcelable.Creator<Vr2dDisplayProperties>() {
        @Override
        public Vr2dDisplayProperties createFromParcel(Parcel source) {
            return new Vr2dDisplayProperties(source);
        }

        @Override
        public Vr2dDisplayProperties[] newArray(int size) {
            return new Vr2dDisplayProperties[size];
        }
    };

    private Vr2dDisplayProperties(Parcel source) {
        mWidth = source.readInt();
        mHeight = source.readInt();
        mDpi = source.readInt();
        mAddedFlags = source.readInt();
        mRemovedFlags = source.readInt();
    }

    public void dump(PrintWriter pw, String prefix) {
        pw.println(prefix + toString());
    }

    public int getWidth() {
        return mWidth;
    }

    public int getHeight() {
        return mHeight;
    }

    public int getDpi() {
        return mDpi;
    }

    public int getFlags() {
        return mAddedFlags;
    }

    public int getRemovedFlags() {
        return mRemovedFlags;
    }

    private static String toReadableFlags(int flags) {
        String retval = "{";
        if ((flags & FLAG_VIRTUAL_DISPLAY_ENABLED) == FLAG_VIRTUAL_DISPLAY_ENABLED) {
            retval += "enabled";
        }
        return retval + "}";
    }

    /**
     * Convenience class for creating Vr2dDisplayProperties.
     */
    public static class Builder {
        private int mAddedFlags = 0;
        private int mRemovedFlags = 0;

        // Negative values are translated as an "ignore" to VrManagerService.
        private int mWidth = -1;
        private int mHeight = -1;
        private int mDpi = -1;

        public Builder() {
        }

        /**
         * Sets the dimensions to use for the virtual display.
         */
        public Builder setDimensions(int width, int height, int dpi) {
            mWidth = width;
            mHeight = height;
            mDpi = dpi;
            return this;
        }

        /**
         * Toggles the virtual display functionality for 2D activities in VR.
         */
        public Builder setEnabled(boolean enabled) {
            if (enabled) {
                addFlags(FLAG_VIRTUAL_DISPLAY_ENABLED);
            } else {
                removeFlags(FLAG_VIRTUAL_DISPLAY_ENABLED);
            }
            return this;
        }

        /**
         * Adds property flags.
         */
        public Builder addFlags(int flags) {
            mAddedFlags |= flags;
            mRemovedFlags &= ~flags;
            return this;
        }

        /**
         * Removes property flags.
         */
        public Builder removeFlags(int flags) {
            mRemovedFlags |= flags;
            mAddedFlags &= ~flags;
            return this;
        }

        /**
         * Builds the Vr2dDisplayProperty instance.
         */
        public Vr2dDisplayProperties build() {
            return new Vr2dDisplayProperties(mWidth, mHeight, mDpi, mAddedFlags, mRemovedFlags);
        }
    }
}
