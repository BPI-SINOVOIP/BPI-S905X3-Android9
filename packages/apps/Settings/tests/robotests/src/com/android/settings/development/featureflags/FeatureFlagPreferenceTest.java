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

package com.android.settings.development.featureflags;

import static com.google.common.truth.Truth.assertThat;

import android.content.Context;

import com.android.settings.testutils.SettingsRobolectricTestRunner;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RuntimeEnvironment;

@RunWith(SettingsRobolectricTestRunner.class)
public class FeatureFlagPreferenceTest {

    private static final String KEY = "feature_key";

    private Context mContext;
    private FeatureFlagPreference mPreference;

    @Before
    public void setUp() {
        mContext = RuntimeEnvironment.application;
        mPreference = new FeatureFlagPreference(mContext, KEY);
    }

    @Test
    public void constructor_shouldSetTitleAndSummary() {
        assertThat(mPreference.getTitle()).isEqualTo(KEY);
        assertThat(mPreference.getSummary()).isEqualTo("false");
        assertThat(mPreference.isChecked()).isFalse();
    }

    @Test
    public void toggle_shouldUpdateSummary() {
        mPreference.setChecked(true);

        assertThat(mPreference.getSummary()).isEqualTo("true");
        assertThat(mPreference.isChecked()).isTrue();
    }
}
