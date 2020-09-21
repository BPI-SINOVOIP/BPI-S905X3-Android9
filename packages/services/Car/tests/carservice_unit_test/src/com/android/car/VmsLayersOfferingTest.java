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
import android.car.vms.VmsLayerDependency;
import android.car.vms.VmsLayersOffering;
import android.test.AndroidTestCase;
import android.test.suitebuilder.annotation.SmallTest;
import android.os.Parcel;

import java.util.Arrays;
import java.util.HashSet;

import android.util.Log;

/*
 * A class to test the VmsLayersOffering parcelability.
 */
@SmallTest
public class VmsLayersOfferingTest extends AndroidTestCase {
    private static final int PUBLISHER_ID = 1;

    private static final int LAYER_ID = 112;
    private static final int LAYER_VERSION = 134;
    private static final int LAYER_SUBTYPE = 156;

    private static final int DEPENDENT_LAYER_ID = 212;
    private static final int DEPENDENT_LAYER_VERSION = 234;
    private static final int DEPENDENT_LAYER_SUBTYPE = 256;

    private static final int DIFFERENT_LAYER_ID = 99;
    private static final int DIFFERENT_PUBLISHER_ID = 2;

    private static final VmsLayer VMS_LAYER = new VmsLayer(
            LAYER_ID,
            LAYER_SUBTYPE,
            LAYER_VERSION);

    private static final VmsLayer DEPENDENT_VMS_LAYER = new VmsLayer(
            DEPENDENT_LAYER_ID,
            DEPENDENT_LAYER_SUBTYPE,
            DEPENDENT_LAYER_VERSION);

    private static final VmsLayer ANOTHER_DEPENDENT_VMS_LAYER = new VmsLayer(
            DIFFERENT_LAYER_ID,
            DEPENDENT_LAYER_SUBTYPE,
            DEPENDENT_LAYER_VERSION);

    private static final VmsLayerDependency VMS_LAYER_DEPENDENCY =
            new VmsLayerDependency(
                    VMS_LAYER,
                    new HashSet<>(Arrays.asList(DEPENDENT_VMS_LAYER)));

    private static final VmsLayerDependency ANOTHER_VMS_LAYER_DEPENDENCY =
            new VmsLayerDependency(
                    VMS_LAYER,
                    new HashSet<>(Arrays.asList(ANOTHER_DEPENDENT_VMS_LAYER)));

    public void testNoOfferingParcel() throws Exception {
        VmsLayersOffering vmsLayersOffering =
                new VmsLayersOffering(new HashSet<>(Arrays.asList()), PUBLISHER_ID);

        Parcel parcel = Parcel.obtain();
        vmsLayersOffering.writeToParcel(parcel, vmsLayersOffering.describeContents());
        parcel.setDataPosition(0);
        VmsLayersOffering createdFromParcel = VmsLayersOffering.CREATOR.createFromParcel(parcel);
        assertEquals(vmsLayersOffering, createdFromParcel);
    }

    public void testMultipleOfferingParcel() throws Exception {
        VmsLayersOffering vmsLayersOffering =
                new VmsLayersOffering(
                        new HashSet<>(Arrays.asList(
                                VMS_LAYER_DEPENDENCY,
                                ANOTHER_VMS_LAYER_DEPENDENCY)),
                        PUBLISHER_ID);

        Parcel parcel = Parcel.obtain();
        vmsLayersOffering.writeToParcel(parcel, vmsLayersOffering.describeContents());
        parcel.setDataPosition(0);
        VmsLayersOffering createdFromParcel = VmsLayersOffering.CREATOR.createFromParcel(parcel);

        assertEquals(vmsLayersOffering, createdFromParcel);
    }

    public void testNoOfferingEquality() throws Exception {
        VmsLayersOffering original =
                new VmsLayersOffering(
                        new HashSet<>(Arrays.asList()), PUBLISHER_ID);

        VmsLayersOffering similar =
                new VmsLayersOffering(
                        new HashSet<>(Arrays.asList()), PUBLISHER_ID);

        assertEquals(original.getDependencies(), similar.getDependencies());
        assertEquals(original.getPublisherId(), similar.getPublisherId());
        assertEquals(original, similar);
    }

    public void testMultipleOfferingEquality() throws Exception {
        VmsLayersOffering original =
                new VmsLayersOffering(
                        new HashSet<>(Arrays.asList(VMS_LAYER_DEPENDENCY, ANOTHER_VMS_LAYER_DEPENDENCY)), PUBLISHER_ID);

        VmsLayersOffering similar =
                new VmsLayersOffering(
                        new HashSet<>(Arrays.asList(VMS_LAYER_DEPENDENCY, ANOTHER_VMS_LAYER_DEPENDENCY)), PUBLISHER_ID);

        assertEquals(original.getDependencies(), similar.getDependencies());
        assertEquals(original.getPublisherId(), similar.getPublisherId());
        assertEquals(original, similar);
    }

    public void testVerifyNonEqualOnPublisherId() throws Exception {
        VmsLayersOffering original =
                new VmsLayersOffering(
                        new HashSet<>(Arrays.asList(VMS_LAYER_DEPENDENCY)), PUBLISHER_ID);

        VmsLayersOffering similar =
                new VmsLayersOffering(
                        new HashSet<>(Arrays.asList(VMS_LAYER_DEPENDENCY)), DIFFERENT_PUBLISHER_ID);

        assertNotEquals(original, similar);
    }

    public void testVerifyNonEqualOnDependency() throws Exception {
        VmsLayersOffering original =
                new VmsLayersOffering(
                        new HashSet<>(Arrays.asList(VMS_LAYER_DEPENDENCY)), PUBLISHER_ID);

        VmsLayersOffering similar =
                new VmsLayersOffering(
                        new HashSet<>(Arrays.asList(ANOTHER_VMS_LAYER_DEPENDENCY)), PUBLISHER_ID);

        assertNotEquals(original, similar);
    }
}
