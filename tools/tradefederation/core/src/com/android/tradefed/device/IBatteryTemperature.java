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

package com.android.tradefed.device;

import com.android.ddmlib.IDevice;

public interface IBatteryTemperature {

    /**
     * Get the device's battery temperature, in degrees celsius.
     *
     * <p>TODO: Implement as ddmlib {@code IDevice#getBatteryTemperature()}
     *
     * @return The device's temperature, in degrees celsius. If unavailable, the return value is 0.
     */
    Integer getBatteryTemperature(IDevice device);
}
