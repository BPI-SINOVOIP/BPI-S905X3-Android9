/*
 * Copyright (C) 2018 The Android Open Source Project
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

package com.android.settings.widget;

import static com.google.common.truth.Truth.assertThat;

import android.content.Context;

import com.android.settings.R;
import com.android.settings.testutils.SettingsRobolectricTestRunner;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RuntimeEnvironment;

@RunWith(SettingsRobolectricTestRunner.class)
public class AppCheckBoxPreferenceTest {

    private Context mContext;
    private AppCheckBoxPreference mPreference;
    private AppCheckBoxPreference mAttrPreference;

    @Before
    public void setUp() {
        mContext = RuntimeEnvironment.application;
        mPreference = new AppCheckBoxPreference(mContext);
        mAttrPreference = new AppCheckBoxPreference(mContext, null /* attrs */);
    }

    @Test
    public void testGetLayoutResource() {
        assertThat(mPreference.getLayoutResource()).isEqualTo(R.layout.preference_app);
        assertThat(mAttrPreference.getLayoutResource()).isEqualTo(R.layout.preference_app);
    }
}
