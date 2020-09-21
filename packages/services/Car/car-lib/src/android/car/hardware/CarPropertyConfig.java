/*
 * Copyright (C) 2016 The Android Open Source Project
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

package android.car.hardware;

import android.annotation.Nullable;
import android.annotation.SystemApi;
import android.car.VehicleAreaType;
import android.os.Parcel;
import android.os.Parcelable;
import android.util.SparseArray;

import java.lang.reflect.Array;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * Represents general information about car property such as data type and min/max ranges for car
 * areas (if applicable). This class supposed to be immutable, parcelable and could be passed over.
 *
 * <p>Use {@link CarPropertyConfig#newBuilder} to create an instance of this class.
 *
 * @param <T> refer to Parcel#writeValue(Object) to get a list of all supported types. The class
 * should be visible to framework as default class loader is being used here.
 *
 * @hide
 */
@SystemApi
public class CarPropertyConfig<T> implements Parcelable {
    private final int mAccess;
    private final int mAreaType;
    private final int mChangeMode;
    private final ArrayList<Integer> mConfigArray;
    private final String mConfigString;
    private final float mMaxSampleRate;
    private final float mMinSampleRate;
    private final int mPropertyId;
    private final SparseArray<AreaConfig<T>> mSupportedAreas;
    private final Class<T> mType;

    private CarPropertyConfig(int access, int areaType, int changeMode,
            ArrayList<Integer> configArray, String configString,
            float maxSampleRate, float minSampleRate, int propertyId,
            SparseArray<AreaConfig<T>> supportedAreas, Class<T> type) {
        mAccess = access;
        mAreaType = areaType;
        mChangeMode = changeMode;
        mConfigArray = configArray;
        mConfigString = configString;
        mMaxSampleRate = maxSampleRate;
        mMinSampleRate = minSampleRate;
        mPropertyId = propertyId;
        mSupportedAreas = supportedAreas;
        mType = type;
    }

    public int getAccess() {
        return mAccess;
    }
    public @VehicleAreaType.VehicleAreaTypeValue int getAreaType() {
        return mAreaType;
    }
    public int getChangeMode() {
        return mChangeMode;
    }
    public List<Integer> getConfigArray() {
        return Collections.unmodifiableList(mConfigArray);
    }
    public String getConfigString() {
        return mConfigString;
    }
    public float getMaxSampleRate() {
        return mMaxSampleRate;
    }
    public float getMinSampleRate() {
        return mMinSampleRate;
    }
    public int getPropertyId() {
        return mPropertyId;
    }
    public Class<T> getPropertyType() {
        return mType;
    }

    /** Returns true if this property doesn't hold car area-specific configuration */
    public boolean isGlobalProperty() {
        return mAreaType == VehicleAreaType.VEHICLE_AREA_TYPE_GLOBAL;
    }

    public int getAreaCount() {
        return mSupportedAreas.size();
    }

    public int[] getAreaIds() {
        int[] areaIds = new int[mSupportedAreas.size()];
        for (int i = 0; i < areaIds.length; i++) {
            areaIds[i] = mSupportedAreas.keyAt(i);
        }
        return areaIds;
    }

    /**
     * Returns the first areaId.
     * Throws {@link IllegalStateException} if supported area count not equals to one.
     * */
    public int getFirstAndOnlyAreaId() {
        if (mSupportedAreas.size() != 1) {
            throw new IllegalStateException("Expected one and only area in this property. Prop: 0x"
                    + Integer.toHexString(mPropertyId));
        }
        return mSupportedAreas.keyAt(0);
    }

    public boolean hasArea(int areaId) {
        return mSupportedAreas.indexOfKey(areaId) >= 0;
    }

    @Nullable
    public T getMinValue(int areaId) {
        AreaConfig<T> area = mSupportedAreas.get(areaId);
        return area == null ? null : area.getMinValue();
    }

    @Nullable
    public T getMaxValue(int areaId) {
        AreaConfig<T> area = mSupportedAreas.get(areaId);
        return area == null ? null : area.getMaxValue();
    }

    @Nullable
    public T getMinValue() {
        AreaConfig<T> area = mSupportedAreas.valueAt(0);
        return area == null ? null : area.getMinValue();
    }

    @Nullable
    public T getMaxValue() {
        AreaConfig<T> area = mSupportedAreas.valueAt(0);
        return area == null ? null : area.getMaxValue();
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeInt(mAccess);
        dest.writeInt(mAreaType);
        dest.writeInt(mChangeMode);
        dest.writeInt(mConfigArray.size());
        for (int i = 0; i < mConfigArray.size(); i++) {
            dest.writeInt(mConfigArray.get(i));
        }
        dest.writeString(mConfigString);
        dest.writeFloat(mMaxSampleRate);
        dest.writeFloat(mMinSampleRate);
        dest.writeInt(mPropertyId);
        dest.writeInt(mSupportedAreas.size());
        for (int i = 0; i < mSupportedAreas.size(); i++) {
            dest.writeInt(mSupportedAreas.keyAt(i));
            dest.writeParcelable(mSupportedAreas.valueAt(i), flags);
        }
        dest.writeString(mType.getName());
    }

    @SuppressWarnings("unchecked")
    private CarPropertyConfig(Parcel in) {
        mAccess = in.readInt();
        mAreaType = in.readInt();
        mChangeMode = in.readInt();
        int configArraySize = in.readInt();
        mConfigArray = new ArrayList<Integer>(configArraySize);
        for (int i = 0; i < configArraySize; i++) {
            mConfigArray.add(in.readInt());
        }
        mConfigString = in.readString();
        mMaxSampleRate = in.readFloat();
        mMinSampleRate = in.readFloat();
        mPropertyId = in.readInt();
        int areaSize = in.readInt();
        mSupportedAreas = new SparseArray<>(areaSize);
        for (int i = 0; i < areaSize; i++) {
            int areaId = in.readInt();
            AreaConfig<T> area = in.readParcelable(getClass().getClassLoader());
            mSupportedAreas.put(areaId, area);
        }
        String className = in.readString();
        try {
            mType = (Class<T>) Class.forName(className);
        } catch (ClassNotFoundException e) {
            throw new IllegalArgumentException("Class not found: " + className);
        }
    }

    public static final Creator<CarPropertyConfig> CREATOR = new Creator<CarPropertyConfig>() {
        @Override
        public CarPropertyConfig createFromParcel(Parcel in) {
            return new CarPropertyConfig(in);
        }

        @Override
        public CarPropertyConfig[] newArray(int size) {
            return new CarPropertyConfig[size];
        }
    };

    @Override
    public String toString() {
        return "CarPropertyConfig{"
                + "mPropertyId=" + mPropertyId
                + ", mAccess=" + mAccess
                + ", mAreaType=" + mAreaType
                + ", mChangeMode=" + mChangeMode
                + ", mConfigArray=" + mConfigArray
                + ", mConfigString=" + mConfigString
                + ", mMaxSampleRate=" + mMaxSampleRate
                + ", mMinSampleRate=" + mMinSampleRate
                + ", mSupportedAreas=" + mSupportedAreas
                + ", mType=" + mType
                + '}';
    }

    public static class AreaConfig<T> implements Parcelable {
        @Nullable private final T mMinValue;
        @Nullable private final T mMaxValue;

        private AreaConfig(T minValue, T maxValue) {
            mMinValue = minValue;
            mMaxValue = maxValue;
        }

        public static final Parcelable.Creator<AreaConfig<Object>> CREATOR
                = getCreator(Object.class);

        private static <E> Parcelable.Creator<AreaConfig<E>> getCreator(final Class<E> clazz) {
            return new Creator<AreaConfig<E>>() {
                @Override
                public AreaConfig<E> createFromParcel(Parcel source) {
                    return new AreaConfig<>(source);
                }

                @Override @SuppressWarnings("unchecked")
                public AreaConfig<E>[] newArray(int size) {
                    return (AreaConfig<E>[]) Array.newInstance(clazz, size);
                }
            };
        }

        @SuppressWarnings("unchecked")
        private AreaConfig(Parcel in) {
            mMinValue = (T) in.readValue(getClass().getClassLoader());
            mMaxValue = (T) in.readValue(getClass().getClassLoader());
        }

        @Nullable public T getMinValue() { return mMinValue; }
        @Nullable public T getMaxValue() { return mMaxValue; }

        @Override
        public int describeContents() {
            return 0;
        }

        @Override
        public void writeToParcel(Parcel dest, int flags) {
            dest.writeValue(mMinValue);
            dest.writeValue(mMaxValue);
        }

        @Override
        public String toString() {
            return "CarAreaConfig{" +
                    "mMinValue=" + mMinValue +
                    ", mMaxValue=" + mMaxValue +
                    '}';
        }
    }

    /**
     * Prepare an instance of CarPropertyConfig
     *
     * @return Builder<T>
     */
    public static <T> Builder<T> newBuilder(Class<T> type, int propertyId, int areaType,
                                            int areaCapacity) {
        return new Builder<>(areaCapacity, areaType, propertyId, type);
    }


    /**
     * Prepare an instance of CarPropertyConfig
     *
     * @return Builder<T>
     */
    public static <T> Builder<T> newBuilder(Class<T> type, int propertyId, int areaType) {
        return new Builder<>(0, areaType, propertyId, type);
    }

    public static class Builder<T> {
        private int mAccess;
        private final int mAreaType;
        private int mChangeMode;
        private final ArrayList<Integer> mConfigArray;
        private String mConfigString;
        private float mMaxSampleRate;
        private float mMinSampleRate;
        private final int mPropertyId;
        private final SparseArray<AreaConfig<T>> mSupportedAreas;
        private final Class<T> mType;

        private Builder(int areaCapacity, int areaType, int propertyId, Class<T> type) {
            mAreaType = areaType;
            mConfigArray = new ArrayList<>();
            mPropertyId = propertyId;
            if (areaCapacity != 0) {
                mSupportedAreas = new SparseArray<>(areaCapacity);
            } else {
                mSupportedAreas = new SparseArray<>();
            }
            mType = type;
        }

        /**
         * Add supported areas parameter to CarPropertyConfig
         *
         * @return Builder<T>
         */
        public Builder<T> addAreas(int[] areaIds) {
            for (int id : areaIds) {
                mSupportedAreas.put(id, null);
            }
            return this;
        }

        /**
         * Add area to CarPropertyConfig
         *
         * @return Builder<T>
         */
        public Builder<T> addArea(int areaId) {
            return addAreaConfig(areaId, null, null);
        }

        /**
         * Add areaConfig to CarPropertyConfig
         *
         * @return Builder<T>
         */
        public Builder<T> addAreaConfig(int areaId, T min, T max) {
            if (min == null && max == null) {
                mSupportedAreas.put(areaId, null);
            } else {
                mSupportedAreas.put(areaId, new AreaConfig<>(min, max));
            }
            return this;
        }

        /**
         * Set access parameter to CarPropertyConfig
         *
         * @return Builder<T>
         */
        public Builder<T> setAccess(int access) {
            mAccess = access;
            return this;
        }

        /**
         * Set changeMode parameter to CarPropertyConfig
         *
         * @return Builder<T>
         */
        public Builder<T> setChangeMode(int changeMode) {
            mChangeMode = changeMode;
            return this;
        }

        /**
         * Set configArray parameter to CarPropertyConfig
         *
         * @return Builder<T>
         */
        public Builder<T> setConfigArray(ArrayList<Integer> configArray) {
            mConfigArray.clear();
            mConfigArray.addAll(configArray);
            return this;
        }

        /**
         * Set configString parameter to CarPropertyConfig
         *
         * @return Builder<T>
         */
        public Builder<T> setConfigString(String configString) {
            mConfigString = configString;
            return this;
        }

        /**
         * Set maxSampleRate parameter to CarPropertyConfig
         *
         * @return Builder<T>
         */
        public Builder<T> setMaxSampleRate(float maxSampleRate) {
            mMaxSampleRate = maxSampleRate;
            return this;
        }

        /**
         * Set minSampleRate parameter to CarPropertyConfig
         *
         * @return Builder<T>
         */
        public Builder<T> setMinSampleRate(float minSampleRate) {
            mMinSampleRate = minSampleRate;
            return this;
        }

        public CarPropertyConfig<T> build() {
            return new CarPropertyConfig<>(mAccess, mAreaType, mChangeMode, mConfigArray,
                                           mConfigString, mMaxSampleRate, mMinSampleRate,
                                           mPropertyId, mSupportedAreas, mType);
        }
    }
}
