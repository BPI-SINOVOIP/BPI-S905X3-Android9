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

package android.car.drivingstate;

import android.car.drivingstate.CarUxRestrictions;
import android.car.drivingstate.ICarUxRestrictionsChangeListener;

/**
 * Binder interface for {@link android.car.drivingstate.CarUxRestrictionsManager}.
 * Check {@link android.car.drivingstate.CarUxRestrictionsManager} APIs for expected behavior of
 * each call.
 *
 * @hide
 */
interface ICarUxRestrictionsManager {
    void registerUxRestrictionsChangeListener(in ICarUxRestrictionsChangeListener listener) = 0;
    void unregisterUxRestrictionsChangeListener(in ICarUxRestrictionsChangeListener listener) = 1;
    CarUxRestrictions getCurrentUxRestrictions() = 2;
}
