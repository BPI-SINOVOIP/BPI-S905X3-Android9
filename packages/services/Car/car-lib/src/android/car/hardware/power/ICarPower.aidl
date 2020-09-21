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

package android.car.hardware.power;

import android.car.hardware.power.ICarPowerStateListener;

/** @hide */
interface ICarPower {
    void registerListener(in ICarPowerStateListener listener) = 0;

    void unregisterListener(in ICarPowerStateListener listener) = 1;

    void requestShutdownOnNextSuspend() = 2;

    int getBootReason() = 3;

    void finished(in ICarPowerStateListener listener, int token) = 4;
}
