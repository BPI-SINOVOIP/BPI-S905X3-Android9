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

package com.google.android.car.kitchensink.property;

import android.car.hardware.CarPropertyConfig;
import android.hardware.automotive.vehicle.V2_0.VehicleProperty;

class PropertyInfo implements Comparable<PropertyInfo> {
    public final CarPropertyConfig mConfig;
    public final String mName;
    public final int mPropId;

    PropertyInfo(CarPropertyConfig config) {
        mConfig = config;
        mPropId = config.getPropertyId();
        mName = VehicleProperty.toString(mPropId);
    }

    @Override
    public String toString() {
        return mName;
    }

    @Override
    public boolean equals(Object other) {
        if (other instanceof PropertyInfo) {
            return ((PropertyInfo) other).mPropId == mPropId;
        }
        return false;
    }

    @Override
    public int hashCode() {
        return mPropId;
    }

    @Override
    public int compareTo(PropertyInfo propertyInfo) {
        return mName.compareTo(propertyInfo.mName);
    }
}
