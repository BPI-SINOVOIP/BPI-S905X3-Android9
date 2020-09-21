/*
 * Copyright (C) 2015 The Android Open Source Project
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
 * limitations under the License
 */

package com.android.tv;

import static com.google.common.truth.Truth.assertThat;

import android.support.test.filters.SmallTest;
import android.support.test.runner.AndroidJUnit4;
import org.junit.Test;
import org.junit.runner.RunWith;

/** Test for features. */
@SmallTest
@RunWith(AndroidJUnit4.class)
public class FeaturesTest {
    @Test
    public void testPropertyFeatureKeyLength() {
        // This forces the class to be loaded and verifies all PropertyFeature key lengths.
        // If any keys are too long the test will fail to load.
        assertThat(TvFeatures.TEST_FEATURE.isEnabled(null)).isFalse();
    }
}
