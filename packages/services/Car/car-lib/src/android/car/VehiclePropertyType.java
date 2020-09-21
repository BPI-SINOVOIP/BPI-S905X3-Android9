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

package android.car;

import android.annotation.IntDef;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;


/**
 * Value type of VehicleProperty
 * @hide
 */
public class VehiclePropertyType {
    public static final int STRING          = 0x00100000;
    public static final int BOOLEAN         = 0x00200000;
    public static final int INT32           = 0x00400000;
    public static final int INT32_VEC       = 0x00410000;
    public static final int INT64           = 0x00500000;
    public static final int INT64_VEC       = 0x00510000;
    public static final int FLOAT           = 0x00600000;
    public static final int FLOAT_VEC       = 0x00610000;
    public static final int BYTES           = 0x00700000;
    public static final int MIXED           = 0x00e00000;
    public static final int MASK            = 0x00ff0000;
    /** @hide */
    @Retention(RetentionPolicy.SOURCE)
    @IntDef({
            STRING,
            BOOLEAN,
            INT32,
            INT32_VEC,
            INT64,
            INT64_VEC,
            FLOAT,
            FLOAT_VEC,
            BYTES,
            /**
             * Any combination of scalar or vector types. The exact format must be
             * provided in the description of the property.
            */
            MIXED,
            MASK
    })
    public @interface Enum {}
    private VehiclePropertyType() {}
}
