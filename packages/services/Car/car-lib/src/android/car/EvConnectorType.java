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
 * EvConnectorType denotes the different connectors a EV may use.
 * @hide
 */
public final class EvConnectorType {
    /**
     * List of EV Connector Types from VHAL
     */
    public static final int UNKNOWN = 0;
    public static final int J1772 = 1;
    public static final int MENNEKES = 2;
    public static final int CHADEMO = 3;
    public static final int COMBO_1 = 4;
    public static final int COMBO_2 = 5;
    public static final int TESLA_ROADSTER = 6;
    public static final int TESLA_HPWC = 7;
    public static final int TESLA_SUPERCHARGER = 8;
    public static final int GBT = 9;
    /**
     * Connector type to use when no other types apply. Before using this
     * value, work with the AOSP community to see if the EvConnectorType enum can be
     * extended with an appropriate value.
     */
    public static final int OTHER = 101;

    /** @hide */
    @Retention(RetentionPolicy.SOURCE)
    @IntDef({
        UNKNOWN,
        J1772,
        MENNEKES,
        CHADEMO,
        COMBO_1,
        COMBO_2,
        TESLA_ROADSTER,
        TESLA_HPWC,
        TESLA_SUPERCHARGER,
        GBT,
        OTHER
    })
    public @interface Enum {}


    private EvConnectorType() {}
}
