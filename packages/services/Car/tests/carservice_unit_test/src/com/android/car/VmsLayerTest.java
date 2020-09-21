/*
 * Copyright (C) 2017 The Android Open Source Project
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

package com.android.car;

import static org.junit.Assert.assertNotEquals;

import android.car.vms.VmsLayer;
import android.test.AndroidTestCase;
import android.test.suitebuilder.annotation.SmallTest;

import android.os.Parcel;

import java.util.Arrays;
import java.util.HashSet;

/*
 * A class to test the VmsLayer parcelability.
 */
@SmallTest
public class VmsLayerTest extends AndroidTestCase {
    private static final int LAYER_ID = 12;
    private static final int LAYER_VERSION = 34;
    private static final int LAYER_SUBTYPE = 56;

    private static final int DIFFERENT_LAYER_ID = 99;

    public void testLayerParcel() throws Exception {
        VmsLayer vmsLayer =
                new VmsLayer(
                        LAYER_ID,
                        LAYER_SUBTYPE,
                        LAYER_VERSION);

        Parcel parcel = Parcel.obtain();
        vmsLayer.writeToParcel(parcel, vmsLayer.describeContents());
        parcel.setDataPosition(0);
        VmsLayer createdFromParcel = VmsLayer.CREATOR.createFromParcel(parcel);
        assertEquals(vmsLayer, createdFromParcel);
    }

    public void testLayerEquality() throws Exception {
        VmsLayer original =
                new VmsLayer(
                        LAYER_ID,
                        LAYER_SUBTYPE,
                        LAYER_VERSION);

        VmsLayer similar =
                new VmsLayer(
                        LAYER_ID,
                        LAYER_SUBTYPE,
                        LAYER_VERSION);

        assertEquals(original.getType(), similar.getType());
        assertEquals(original.getSubtype(), similar.getSubtype());
        assertEquals(original.getVersion(), similar.getVersion());
        assertEquals(original, similar);
    }

    public void testVerifyNonEqual() throws Exception {
        VmsLayer original =
                new VmsLayer(
                        LAYER_ID,
                        LAYER_SUBTYPE,
                        LAYER_VERSION);

        VmsLayer similar =
                new VmsLayer(
                        DIFFERENT_LAYER_ID,
                        LAYER_SUBTYPE,
                        LAYER_VERSION);

        assertNotEquals(original, similar);
    }
}