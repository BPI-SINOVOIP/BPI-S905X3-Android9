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
package android.car.apitest;

import android.car.VehicleAreaMirror;
import android.test.AndroidTestCase;
import android.test.suitebuilder.annotation.SmallTest;

@SmallTest
public class VehicleAreaMirrorTest extends AndroidTestCase{

    public void testMatchWithVehicleHal() {
        assertEquals(android.hardware.automotive.vehicle.V2_0.VehicleAreaMirror.DRIVER_CENTER,
                VehicleAreaMirror.MIRROR_DRIVER_CENTER);
        assertEquals(android.hardware.automotive.vehicle.V2_0.VehicleAreaMirror.DRIVER_LEFT,
                VehicleAreaMirror.MIRROR_DRIVER_LEFT);
        assertEquals(android.hardware.automotive.vehicle.V2_0.VehicleAreaMirror.DRIVER_RIGHT,
                VehicleAreaMirror.MIRROR_DRIVER_RIGHT);
    }
}
