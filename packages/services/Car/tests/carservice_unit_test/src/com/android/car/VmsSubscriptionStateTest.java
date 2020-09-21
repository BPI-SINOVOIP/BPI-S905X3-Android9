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
import android.car.vms.VmsSubscriptionState;
import android.test.AndroidTestCase;
import android.test.suitebuilder.annotation.SmallTest;

import android.os.Parcel;

import java.util.Arrays;
import java.util.HashSet;

/*
 * A class to test the VmsSubscriptionState parcelability.
 */
@SmallTest
public class VmsSubscriptionStateTest extends AndroidTestCase {
    private static final int LAYER_ID = 12;
    private static final int LAYER_VERSION = 34;
    private static final int LAYER_SUBTYPE = 56;

    private static final int PUBLISHER_ID_1= 111;
    private static final int PUBLISHER_ID_2= 222;

    private static final int DIFFERENT_LAYER_ID = 99;

    private static final int SEQUENCE_NUMBER = 1;
    private static final int DIFFERENT_SEQUENCE_NUMBER = 2;

    private static final VmsLayer VMS_LAYER = new VmsLayer(
            LAYER_ID,
            LAYER_SUBTYPE,
            LAYER_VERSION);

    private static final VmsLayer ANOTHER_VMS_LAYER = new VmsLayer(
            DIFFERENT_LAYER_ID,
            LAYER_SUBTYPE,
            LAYER_VERSION);

    private static final VmsAssociatedLayer VMS_ASSOCIATED_LAYER =
            new VmsAssociatedLayer(
                    VMS_LAYER,
                    new HashSet<>(Arrays.asList(PUBLISHER_ID_1)));

    public void testNoSubscriptionsParcel() throws Exception {
        VmsSubscriptionState vmsSubscriptionState =
                new VmsSubscriptionState(
                        SEQUENCE_NUMBER,
                        new HashSet<>(Arrays.asList()),
                        new HashSet<>(Arrays.asList()));

        Parcel parcel = Parcel.obtain();
        vmsSubscriptionState.writeToParcel(parcel, vmsSubscriptionState.describeContents());
        parcel.setDataPosition(0);
        VmsSubscriptionState createdFromParcel = VmsSubscriptionState.CREATOR.createFromParcel(parcel);
        assertEquals(vmsSubscriptionState, createdFromParcel);
    }

    public void testMultipleSubscriptionsParcel() throws Exception {
        VmsSubscriptionState vmsSubscriptionState =
                new VmsSubscriptionState(
                        SEQUENCE_NUMBER,
                        new HashSet<>(Arrays.asList(VMS_LAYER)),
                        new HashSet<>(Arrays.asList(VMS_ASSOCIATED_LAYER)));

        Parcel parcel = Parcel.obtain();
        vmsSubscriptionState.writeToParcel(parcel, vmsSubscriptionState.describeContents());
        parcel.setDataPosition(0);
        VmsSubscriptionState createdFromParcel = VmsSubscriptionState.CREATOR.createFromParcel(parcel);
        assertEquals(vmsSubscriptionState, createdFromParcel);
    }

    public void testNoSubscriptionsEquality() throws Exception {
        VmsSubscriptionState original =
                new VmsSubscriptionState(
                        SEQUENCE_NUMBER,
                        new HashSet<>(Arrays.asList()),
                        new HashSet<>(Arrays.asList()));

        VmsSubscriptionState similar =
                new VmsSubscriptionState(
                        SEQUENCE_NUMBER,
                        new HashSet<>(Arrays.asList()),
                        new HashSet<>(Arrays.asList()));

        assertEquals(original.getSequenceNumber(), similar.getSequenceNumber());
        assertEquals(original.getLayers(), similar.getLayers());
        assertEquals(original.getAssociatedLayers(), similar.getAssociatedLayers());
        assertEquals(original, similar);
    }

    public void testMultipleSubscriptionsEquality() throws Exception {
        VmsSubscriptionState original =
                new VmsSubscriptionState(
                        SEQUENCE_NUMBER,
                        new HashSet<>(Arrays.asList(VMS_LAYER)),
                        new HashSet<>(Arrays.asList(VMS_ASSOCIATED_LAYER)));

        VmsSubscriptionState similar =
                new VmsSubscriptionState(
                        SEQUENCE_NUMBER,
                        new HashSet<>(Arrays.asList(VMS_LAYER)),
                        new HashSet<>(Arrays.asList(VMS_ASSOCIATED_LAYER)));

        assertEquals(original.getSequenceNumber(), similar.getSequenceNumber());
        assertEquals(original.getLayers(), similar.getLayers());
        assertEquals(original.getAssociatedLayers(), similar.getAssociatedLayers());
        assertEquals(original, similar);
    }

    public void testVerifyNonEqualOnSequenceNumber() throws Exception {
        VmsSubscriptionState original =
                new VmsSubscriptionState(
                        SEQUENCE_NUMBER,
                        new HashSet<>(Arrays.asList(VMS_LAYER)),
                        new HashSet<>(Arrays.asList(VMS_ASSOCIATED_LAYER)));

        VmsSubscriptionState similar =
                new VmsSubscriptionState(
                        DIFFERENT_SEQUENCE_NUMBER,
                        new HashSet<>(Arrays.asList(VMS_LAYER)),
                        new HashSet<>(Arrays.asList(VMS_ASSOCIATED_LAYER)));

        if (!original.equals(similar)) {
            return;
        }
        fail("VmsSubscriptionState with different sequence numbers appear to be equal. original: " +
                original +
                ", similar: " +
                similar);
    }

    public void testVerifyNonEqualOnLayers() throws Exception {
        VmsSubscriptionState original =
                new VmsSubscriptionState(
                        SEQUENCE_NUMBER,
                        new HashSet<>(Arrays.asList(VMS_LAYER)),
                        new HashSet<>(Arrays.asList(VMS_ASSOCIATED_LAYER)));

        VmsSubscriptionState similar =
                new VmsSubscriptionState(
                        SEQUENCE_NUMBER,
                        new HashSet<>(Arrays.asList()),
                        new HashSet<>(Arrays.asList(VMS_ASSOCIATED_LAYER)));

        assertNotEquals(original, similar);
    }

    public void testVerifyNonEqualOnAssociatedLayers() throws Exception {
        VmsSubscriptionState original =
                new VmsSubscriptionState(
                        SEQUENCE_NUMBER,
                        new HashSet<>(Arrays.asList(VMS_LAYER)),
                        new HashSet<>(Arrays.asList(VMS_ASSOCIATED_LAYER)));

        VmsSubscriptionState similar =
                new VmsSubscriptionState(
                        SEQUENCE_NUMBER,
                        new HashSet<>(Arrays.asList(VMS_LAYER)),
                        new HashSet<>(Arrays.asList()));

        assertNotEquals(original, similar);
    }
}