/*
 * Copyright (C) 2013 The Android Open Source Project
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

public enum FreeDeviceState {

        /** device is responsive, and can be returned to the available device queue */
        AVAILABLE,

        /**
         * Device is not available for testing, and should not be returned to the available device
         * queue. Typically this means device is not visible via adb, but not necessarily.
         */
        UNAVAILABLE,

        /**
         * Device is visible on adb, but is not responsive. Depending on configuration this
         * device may be returned to available queue.
         */
        UNRESPONSIVE,

        /**
         * Device should be ignored, and not returned to the available device queue.
         */
        IGNORE;

}
