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

import android.car.VehicleAreaDoor;
import android.test.AndroidTestCase;
import android.test.suitebuilder.annotation.SmallTest;

@SmallTest
public class VehicleDoorTest extends AndroidTestCase {

    public void testMatchWithVehicleHal() {
        assertEquals(android.hardware.automotive.vehicle.V2_0.VehicleAreaDoor.HOOD,
                VehicleAreaDoor.DOOR_HOOD);
        assertEquals(android.hardware.automotive.vehicle.V2_0.VehicleAreaDoor.REAR,
                VehicleAreaDoor.DOOR_REAR);
        assertEquals(android.hardware.automotive.vehicle.V2_0.VehicleAreaDoor.ROW_1_LEFT,
                VehicleAreaDoor.DOOR_ROW_1_LEFT);
        assertEquals(android.hardware.automotive.vehicle.V2_0.VehicleAreaDoor.ROW_1_RIGHT,
                VehicleAreaDoor.DOOR_ROW_1_RIGHT);
        assertEquals(android.hardware.automotive.vehicle.V2_0.VehicleAreaDoor.ROW_2_LEFT,
                VehicleAreaDoor.DOOR_ROW_2_LEFT);
        assertEquals(android.hardware.automotive.vehicle.V2_0.VehicleAreaDoor.ROW_2_RIGHT,
                VehicleAreaDoor.DOOR_ROW_2_RIGHT);
        assertEquals(android.hardware.automotive.vehicle.V2_0.VehicleAreaDoor.ROW_3_LEFT,
                VehicleAreaDoor.DOOR_ROW_3_LEFT);
        assertEquals(android.hardware.automotive.vehicle.V2_0.VehicleAreaDoor.ROW_3_RIGHT,
                VehicleAreaDoor.DOOR_ROW_3_RIGHT);
    }
}
