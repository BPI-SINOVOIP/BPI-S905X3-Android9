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

import android.car.vms.VmsAssociatedLayer;
import android.car.vms.VmsLayer;
import android.test.AndroidTestCase;
import android.test.suitebuilder.annotation.SmallTest;

import android.os.Parcel;

import java.util.Arrays;
import java.util.HashSet;


/*
 * A class to test the VmsAssociatedLayer parcelability.
 */
@SmallTest
public class VmsAssociatedLayerTest extends AndroidTestCase {
    private static final int LAYER_ID = 12;
    private static final int LAYER_VERSION = 34;
    private static final int LAYER_SUBTYPE = 56;

    private static final int PUBLISHER_ID_1 = 111;
    private static final int PUBLISHER_ID_2 = 222;

    private static final int DIFFERENT_LAYER_ID = 99;

    private static final VmsLayer VMS_LAYER = new VmsLayer(
            LAYER_ID,
            LAYER_SUBTYPE,
            LAYER_VERSION);

    private static final VmsLayer ANOTHER_VMS_LAYER = new VmsLayer(
            DIFFERENT_LAYER_ID,
            LAYER_SUBTYPE,
            LAYER_VERSION);


    public void testNoPublishersAssociatedLayerParcel() throws Exception {
        VmsAssociatedLayer vmsAssociatedLayer =
                new VmsAssociatedLayer(
                        VMS_LAYER,
                        new HashSet<>(Arrays.asList()));
        Parcel parcel = Parcel.obtain();
        vmsAssociatedLayer.writeToParcel(parcel, vmsAssociatedLayer.describeContents());
        parcel.setDataPosition(0);
        VmsAssociatedLayer createdFromParcel = VmsAssociatedLayer.CREATOR.createFromParcel(parcel);
        assertEquals(vmsAssociatedLayer, createdFromParcel);
    }

    public void testLayerWithMultiplePublishersParcel() throws Exception {
        VmsAssociatedLayer vmsAssociatedLayer =
                new VmsAssociatedLayer(
                        VMS_LAYER,
                        new HashSet<>(Arrays.asList(PUBLISHER_ID_1, PUBLISHER_ID_2)));
        Parcel parcel = Parcel.obtain();
        vmsAssociatedLayer.writeToParcel(parcel, vmsAssociatedLayer.describeContents());
        parcel.setDataPosition(0);
        VmsAssociatedLayer createdFromParcel = VmsAssociatedLayer.CREATOR.createFromParcel(parcel);
        assertEquals(vmsAssociatedLayer, createdFromParcel);
    }

    public void testNoPublishersAssociatedLayerEquality() throws Exception {
        VmsAssociatedLayer original =
                new VmsAssociatedLayer(
                        VMS_LAYER,
                        new HashSet<>(Arrays.asList()));

        VmsAssociatedLayer similar =
                new VmsAssociatedLayer(
                        VMS_LAYER,
                        new HashSet<>(Arrays.asList()));

        assertEquals(original.getVmsLayer(), similar.getVmsLayer());
        assertEquals(original.getPublisherIds(), similar.getPublisherIds());
        assertEquals(original, similar);
    }

    public void testLayerWithMultiplePublishersEquality() throws Exception {
        VmsAssociatedLayer original =
                new VmsAssociatedLayer(
                        VMS_LAYER,
                        new HashSet<>(Arrays.asList(PUBLISHER_ID_1, PUBLISHER_ID_2)));

        VmsAssociatedLayer similar =
                new VmsAssociatedLayer(
                        VMS_LAYER,
                        new HashSet<>(Arrays.asList(PUBLISHER_ID_1, PUBLISHER_ID_2)));

        assertEquals(original.getVmsLayer(), similar.getVmsLayer());
        assertEquals(original.getPublisherIds(), similar.getPublisherIds());
        assertEquals(original, similar);
    }

    public void testVerifyNonEqualOnLayer() throws Exception {
        VmsAssociatedLayer original =
                new VmsAssociatedLayer(
                        ANOTHER_VMS_LAYER,
                        new HashSet<>(Arrays.asList()));

        VmsAssociatedLayer similar =
                new VmsAssociatedLayer(
                        VMS_LAYER,
                        new HashSet<>(Arrays.asList()));

        assertNotEquals(original, similar);
    }

    public void testVerifyNonEqualOnPublisherIds() throws Exception {
        VmsAssociatedLayer original =
                new VmsAssociatedLayer(
                        VMS_LAYER,
                        new HashSet<>(Arrays.asList(PUBLISHER_ID_1)));

        VmsAssociatedLayer similar =
                new VmsAssociatedLayer(
                        VMS_LAYER,
                        new HashSet<>(Arrays.asList(PUBLISHER_ID_2)));

        assertNotEquals(original, similar);
    }
}