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
import android.test.AndroidTestCase;
import android.test.suitebuilder.annotation.SmallTest;

import android.os.Parcel;

import java.util.Arrays;
import java.util.HashSet;


/*
 * A class to test the VmsLayerDependency parcelability.
 */
@SmallTest
public class VmsLayerDependencyTest extends AndroidTestCase {
    private static final int LAYER_ID = 112;
    private static final int LAYER_VERSION = 134;
    private static final int LAYER_SUBTYPE = 156;

    private static final int DEPENDENT_LAYER_ID = 212;
    private static final int DEPENDENT_LAYER_VERSION = 234;
    private static final int DEPENDENT_LAYER_SUBTYPE = 256;

    private static final int DIFFERENT_LAYER_ID = 99;

    private static final VmsLayer VMS_LAYER = new VmsLayer(
            LAYER_ID,
            LAYER_SUBTYPE,
            LAYER_VERSION);

    private static final VmsLayer ANOTHER_VMS_LAYER = new VmsLayer(
            DIFFERENT_LAYER_ID,
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

    public void testNoDependendtLayerParcel() throws Exception {
        VmsLayerDependency vmsLayerDependency =
                new VmsLayerDependency(
                        VMS_LAYER,
                        new HashSet<>(Arrays.asList()));

        Parcel parcel = Parcel.obtain();
        vmsLayerDependency.writeToParcel(parcel, vmsLayerDependency.describeContents());
        parcel.setDataPosition(0);
        VmsLayerDependency createdFromParcel = VmsLayerDependency.CREATOR.createFromParcel(parcel);
        assertEquals(vmsLayerDependency, createdFromParcel);
    }

    public void testMultipleDependendtLayerParcel() throws Exception {
        VmsLayerDependency vmsLayerDependency =
                new VmsLayerDependency(
                        VMS_LAYER,
                        new HashSet<>(Arrays.asList(DEPENDENT_VMS_LAYER, ANOTHER_DEPENDENT_VMS_LAYER)));

        Parcel parcel = Parcel.obtain();
        vmsLayerDependency.writeToParcel(parcel, vmsLayerDependency.describeContents());
        parcel.setDataPosition(0);
        VmsLayerDependency createdFromParcel = VmsLayerDependency.CREATOR.createFromParcel(parcel);
        assertEquals(vmsLayerDependency, createdFromParcel);
    }

    public void testNoDependendtLayerEquality() throws Exception {
        VmsLayerDependency original =
                new VmsLayerDependency(
                        VMS_LAYER,
                        new HashSet<>(Arrays.asList()));

        VmsLayerDependency similar =
                new VmsLayerDependency(VMS_LAYER);

        assertEquals(original.getLayer(), similar.getLayer());
        assertEquals(original.getDependencies(), similar.getDependencies());
        assertEquals(original, similar);
    }

    public void testMultipleDependendtLayerEquality() throws Exception {
        VmsLayerDependency original =
                new VmsLayerDependency(
                        VMS_LAYER,
                        new HashSet<>(Arrays.asList(DEPENDENT_VMS_LAYER, ANOTHER_DEPENDENT_VMS_LAYER)));

        VmsLayerDependency similar =
                new VmsLayerDependency(
                        VMS_LAYER,
                        new HashSet<>(Arrays.asList(DEPENDENT_VMS_LAYER, ANOTHER_DEPENDENT_VMS_LAYER)));
        assertEquals(original.getLayer(), similar.getLayer());
        assertEquals(original.getDependencies(), similar.getDependencies());
        assertEquals(original, similar);
    }

    public void testVerifyNonEqualOnLayer() throws Exception {
        VmsLayerDependency original =
                new VmsLayerDependency(
                        VMS_LAYER,
                        new HashSet<>(Arrays.asList(DEPENDENT_VMS_LAYER)));

        VmsLayerDependency similar =
                new VmsLayerDependency(
                        ANOTHER_DEPENDENT_VMS_LAYER,
                        new HashSet<>(Arrays.asList(DEPENDENT_VMS_LAYER)));

        assertNotEquals(original, similar);
    }

    public void testVerifyNonEqualOnDependentLayer() throws Exception {
        VmsLayerDependency original =
                new VmsLayerDependency(
                        VMS_LAYER,
                        new HashSet<>(Arrays.asList(DEPENDENT_VMS_LAYER)));

        VmsLayerDependency similar =
                new VmsLayerDependency(
                        VMS_LAYER,
                        new HashSet<>(Arrays.asList(ANOTHER_DEPENDENT_VMS_LAYER)));

        assertNotEquals(original, similar);
    }
}
