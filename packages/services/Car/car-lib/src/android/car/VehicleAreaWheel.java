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
package android.car;

import android.annotation.SystemApi;

/**
 * VehicleAreaWheel is an abstraction for the wheels on a car.  It exists to isolate the java APIs
 * from the VHAL definitions.
 * @hide
 */
@SystemApi
public final class VehicleAreaWheel {
    public static final int WHEEL_UNKNOWN = 0x00;
    public static final int WHEEL_LEFT_FRONT = 0x01;
    public static final int WHEEL_RIGHT_FRONT = 0x02;
    public static final int WHEEL_LEFT_REAR = 0x04;
    public static final int WHEEL_RIGHT_REAR = 0x08;

    private VehicleAreaWheel() {}
}

