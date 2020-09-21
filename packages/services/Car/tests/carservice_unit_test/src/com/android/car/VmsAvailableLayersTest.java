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
import android.car.vms.VmsAvailableLayers;
import android.car.vms.VmsLayer;
import android.test.AndroidTestCase;
import android.test.suitebuilder.annotation.SmallTest;

import android.os.Parcel;

import java.util.Arrays;
import java.util.HashSet;


/*
 * A class to test the VmsAvailableLayers parcelability.
 */
@SmallTest
public class VmsAvailableLayersTest extends AndroidTestCase {
    private static final int LAYER_ID = 12;
    private static final int LAYER_VERSION = 34;
    private static final int LAYER_SUBTYPE = 56;
    private static final int PUBLISHER_ID = 999;
    private static final int SEQUENCE_NUMBER = 17;

    private static final int DIFFERENT_SEQUENCE_NUMBER = 71;
    private static final int DIFFERENT_LAYER_ID = 21;

    private static final VmsLayer VMS_LAYER = new VmsLayer(
            LAYER_ID,
            LAYER_SUBTYPE,
            LAYER_VERSION);

    private static final VmsLayer ANOTHER_VMS_LAYER = new VmsLayer(
            DIFFERENT_LAYER_ID,
            LAYER_SUBTYPE,
            LAYER_VERSION);

    private static final VmsAssociatedLayer VMS_ASSOCIATED_LAYER = new VmsAssociatedLayer(
            VMS_LAYER,
            new HashSet<>(Arrays.asList(PUBLISHER_ID)));

    private static final VmsAssociatedLayer ANOTHER_VMS_ASSOCIATED_LAYER = new VmsAssociatedLayer(
            ANOTHER_VMS_LAYER,
            new HashSet<>(Arrays.asList(PUBLISHER_ID)));

    public void testNoAvailableLayersParcel() throws Exception {
        VmsAvailableLayers vmsAvailableLayers =
                new VmsAvailableLayers(new HashSet<>(Arrays.asList()), SEQUENCE_NUMBER);

        Parcel parcel = Parcel.obtain();

        vmsAvailableLayers.writeToParcel(parcel, vmsAvailableLayers.describeContents());
        parcel.setDataPosition(0);
        VmsAvailableLayers createdFromParcel = VmsAvailableLayers.CREATOR.createFromParcel(parcel);
        assertEquals(vmsAvailableLayers, createdFromParcel);
    }

    public void testMultipleAvailableLayersParcel() throws Exception {
        VmsAvailableLayers vmsAvailableLayers =
                new VmsAvailableLayers(
                        new HashSet<>(Arrays.asList(
                                VMS_ASSOCIATED_LAYER,
                                ANOTHER_VMS_ASSOCIATED_LAYER)),
                        SEQUENCE_NUMBER);

        Parcel parcel = Parcel.obtain();

        vmsAvailableLayers.writeToParcel(parcel, vmsAvailableLayers.describeContents());
        parcel.setDataPosition(0);
        VmsAvailableLayers createdFromParcel = VmsAvailableLayers.CREATOR.createFromParcel(parcel);
        assertEquals(vmsAvailableLayers, createdFromParcel);
    }

    public void testNoAvailableLayerEquality() throws Exception {
        VmsAvailableLayers original =
                new VmsAvailableLayers(
                        new HashSet<>(Arrays.asList()),
                        SEQUENCE_NUMBER);

        VmsAvailableLayers similar =
                new VmsAvailableLayers(
                        new HashSet<>(Arrays.asList()),
                        SEQUENCE_NUMBER);

        assertEquals(original.getAssociatedLayers(), similar.getAssociatedLayers());
        assertEquals(original.getSequence(), similar.getSequence());
        assertEquals(original, similar);
    }

    public void testMultipleAvailableLayerEquality() throws Exception {
        VmsAvailableLayers original =
                new VmsAvailableLayers(
                        new HashSet<>(Arrays.asList(
                                VMS_ASSOCIATED_LAYER,
                                ANOTHER_VMS_ASSOCIATED_LAYER)),
                        SEQUENCE_NUMBER);

        VmsAvailableLayers similar =
                new VmsAvailableLayers(
                        new HashSet<>(Arrays.asList(
                                VMS_ASSOCIATED_LAYER,
                                ANOTHER_VMS_ASSOCIATED_LAYER)),
                        SEQUENCE_NUMBER);

        assertEquals(original.getAssociatedLayers(), similar.getAssociatedLayers());
        assertEquals(original.getSequence(), similar.getSequence());
        assertEquals(original, similar);
    }

    public void testVerifyNonEqualOnAssociatedLayer() throws Exception {
        VmsAvailableLayers original =
                new VmsAvailableLayers(
                        new HashSet<>(Arrays.asList(VMS_ASSOCIATED_LAYER)),
                        SEQUENCE_NUMBER);

        VmsAvailableLayers similar =
                new VmsAvailableLayers(
                        new HashSet<>(Arrays.asList(ANOTHER_VMS_ASSOCIATED_LAYER)),
                        SEQUENCE_NUMBER);

        assertNotEquals(original, similar);
    }

    public void testVerifyNonEqualOnSequence() throws Exception {
        VmsAvailableLayers original =
                new VmsAvailableLayers(
                        new HashSet<>(Arrays.asList(VMS_ASSOCIATED_LAYER)),
                        SEQUENCE_NUMBER);

        VmsAvailableLayers similar =
                new VmsAvailableLayers(
                        new HashSet<>(Arrays.asList(VMS_ASSOCIATED_LAYER)),
                        DIFFERENT_SEQUENCE_NUMBER);

        assertNotEquals(original, similar);
    }
}