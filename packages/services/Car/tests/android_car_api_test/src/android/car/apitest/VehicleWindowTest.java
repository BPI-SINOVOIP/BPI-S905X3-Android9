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

import android.car.VehicleAreaWindow;
import android.test.AndroidTestCase;
import android.test.suitebuilder.annotation.SmallTest;

@SmallTest
public class VehicleWindowTest extends AndroidTestCase {

    public void testMatchWithVehicleHal() {
        assertEquals(android.hardware.automotive.vehicle.V2_0.VehicleAreaWindow.FRONT_WINDSHIELD,
                VehicleAreaWindow.WINDOW_FRONT_WINDSHIELD);
        assertEquals(android.hardware.automotive.vehicle.V2_0.VehicleAreaWindow.REAR_WINDSHIELD,
                VehicleAreaWindow.WINDOW_REAR_WINDSHIELD);
        assertEquals(android.hardware.automotive.vehicle.V2_0.VehicleAreaWindow.ROW_1_LEFT,
                VehicleAreaWindow.WINDOW_ROW_1_LEFT);
        assertEquals(android.hardware.automotive.vehicle.V2_0.VehicleAreaWindow.ROW_1_RIGHT,
                VehicleAreaWindow.WINDOW_ROW_1_RIGHT);
        assertEquals(android.hardware.automotive.vehicle.V2_0.VehicleAreaWindow.ROW_2_LEFT,
                VehicleAreaWindow.WINDOW_ROW_2_LEFT);
        assertEquals(android.hardware.automotive.vehicle.V2_0.VehicleAreaWindow.ROW_2_RIGHT,
                VehicleAreaWindow.WINDOW_ROW_2_RIGHT);
        assertEquals(android.hardware.automotive.vehicle.V2_0.VehicleAreaWindow.ROW_3_LEFT,
                VehicleAreaWindow.WINDOW_ROW_3_LEFT);
        assertEquals(android.hardware.automotive.vehicle.V2_0.VehicleAreaWindow.ROW_3_RIGHT,
                VehicleAreaWindow.WINDOW_ROW_3_RIGHT);
        assertEquals(android.hardware.automotive.vehicle.V2_0.VehicleAreaWindow.ROOF_TOP_1,
                VehicleAreaWindow.WINDOW_ROOF_TOP_1);
        assertEquals(android.hardware.automotive.vehicle.V2_0.VehicleAreaWindow.ROOF_TOP_2,
                VehicleAreaWindow.WINDOW_ROOF_TOP_2);
    }
}
