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
package android.car;

import android.annotation.IntDef;
import android.annotation.SystemApi;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * FuelType denotes the different fuels a vehicle may use.
 * @hide
 */
public final class FuelType {
    /**
     * List of Fuel Types from VHAL
     */
    public static final int UNKNOWN = 0;
    /** Unleaded gasoline */
    public static final int UNLEADED = 1;
    /** Leaded gasoline */
    public static final int LEADED = 2;
    /** Diesel #1 */
    public static final int DIESEL_1 = 3;
    /** Diesel #2 */
    public static final int DIESEL_2 = 4;
    /** Biodiesel */
    public static final int BIODIESEL = 5;
    /** 85% ethanol/gasoline blend */
    public static final int E85 = 6;
    /** Liquified petroleum gas */
    public static final int LPG = 7;
    /** Compressed natural gas */
    public static final int CNG = 8;
    /** Liquified natural gas */
    public static final int LNG = 9;
    /** Electric */
    public static final int ELECTRIC = 10;
    /** Hydrogen fuel cell */
    public static final int HYDROGEN = 11;
    /**
     * Fuel type to use when no other types apply. Before using this value, work with
     * Google to see if the FuelType enum can be extended with an appropriate value.
     */
    public static final int OTHER = 12;

    /** @hide */
    @Retention(RetentionPolicy.SOURCE)
    @IntDef({
        UNKNOWN,
        UNLEADED,
        LEADED,
        DIESEL_1,
        DIESEL_2,
        BIODIESEL,
        E85,
        LPG,
        CNG,
        LNG,
        ELECTRIC,
        HYDROGEN,
        OTHER
    })
    public @interface Enum {}


    private FuelType() {}
}
