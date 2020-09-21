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

package android.car.hardware.property;

import android.car.hardware.property.CarPropertyEvent;

/**
 * Binder callback for CarPropertyEventListener.
 * This is generated per each CarClient.
 * @hide
 */
oneway interface ICarPropertyEventListener {
    /**
     * Called when an event is triggered in response to one of the calls (such as on tune) or
     * asynchronously (such as on announcement).
     */
    void onEvent(in List<CarPropertyEvent> events) = 0;
}

