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
package com.android.documentsui.inspector;

import static junit.framework.Assert.assertEquals;

import android.content.pm.PackageManager;
import android.os.LocaleList;
import android.support.test.InstrumentationRegistry;

import android.content.Context;
import android.support.test.runner.AndroidJUnit4;
import android.test.suitebuilder.annotation.SmallTest;
import android.view.textclassifier.TextClassification;
import android.view.textclassifier.TextClassificationManager;
import android.view.textclassifier.TextClassifier;
import com.android.documentsui.testing.TestPackageManager;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

/**
 * Test class for testing recognizing geo coordinates.
 */
@RunWith(AndroidJUnit4.class)
@SmallTest
public class GpsCoordinatesTextClassifierTest {

    private GpsCoordinatesTextClassifier mClassifier;

    @Before
    public void setUp() throws Exception {
        PackageManager pm = TestPackageManager.create();
        Context context = InstrumentationRegistry.getTargetContext();

        TextClassifier defaultClassifier =
                context.getSystemService(TextClassificationManager.class).getTextClassifier();
        mClassifier = new GpsCoordinatesTextClassifier(pm, defaultClassifier);
    }

    @Test
    public void testBasicPatterns() throws Exception {
        assertClassifiedGeo("0,0", true);
        assertClassifiedGeo("0, 0", true);
        assertClassifiedGeo("0.0,0.0", true);
        assertClassifiedGeo("0.0, 0.0", true);
        assertClassifiedGeo("90,180", true);
        assertClassifiedGeo("90, 180", true);
        assertClassifiedGeo("90.0000,180.0000", true);
        assertClassifiedGeo("90.000, 180.000000000000000", true);
        assertClassifiedGeo("-77.5646564,133.656554654", true);
        assertClassifiedGeo("33, -179.324234242423", true);
        assertClassifiedGeo("44.4545454,70.0", true);
        assertClassifiedGeo("60.0, 60.0", true);
        assertClassifiedGeo("-33.33,-180", true);
        assertClassifiedGeo("-88.888888, -33.3333", true);
        assertClassifiedGeo("90.0, 180.000000", true);
        assertClassifiedGeo("-90.00000, -180.0", true);
    }

    @Test
    public void testInvalidPatterns() throws Exception {
        assertClassifiedGeo("0", false);
        assertClassifiedGeo("Geo Intent", false);
        assertClassifiedGeo("GeoIntent", false);
        assertClassifiedGeo("A.B, C.D", false);
        assertClassifiedGeo("90.165464, 180.1", false);
        // TODO: Failing tests below
        // assertClassifiedGeo("-90.1, -180.156754", false);
        // assertClassifiedGeo("5000, 5000", false);
        // assertClassifiedGeo("500, 500", false);
    }

    private void assertClassifiedGeo(CharSequence text, boolean expectClassified) {
        boolean wasClassified;
        TextClassification test = mClassifier.classifyText(
                text,
                0,
                text.length(),
                new LocaleList());
        try {
            wasClassified = "geo".equals(test.getIntent().getData().getScheme());
        } catch (NullPointerException intentNotSet) {
            wasClassified = false;
        }
        assertEquals(wasClassified, expectClassified);
    }
}
