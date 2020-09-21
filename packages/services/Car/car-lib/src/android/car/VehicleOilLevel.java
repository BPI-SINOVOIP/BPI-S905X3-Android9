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
import android.annotation.SystemApi;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * OilLevel in the engine
 * @hide
 */
public final class VehicleOilLevel {
    /**
     * List of Oil Levels from VHAL
     */
    public static final int CRITICALLY_LOW = 0;
    public static final int LOW = 1;
    public static final int NORMAL = 2;
    public static final int HIGH = 3;
    public static final int ERROR = 4;

    /** @hide */
    @Retention(RetentionPolicy.SOURCE)
    @IntDef({
        CRITICALLY_LOW,
        LOW,
        NORMAL,
        HIGH,
        ERROR,
    })
    public @interface Enum {}

    private VehicleOilLevel() {}
}
