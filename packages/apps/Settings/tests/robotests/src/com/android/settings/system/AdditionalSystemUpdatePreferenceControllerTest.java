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
package com.android.settings.system;

import static com.google.common.truth.Truth.assertThat;

import android.content.Context;

import com.android.settings.testutils.SettingsRobolectricTestRunner;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.MockitoAnnotations;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;

@RunWith(SettingsRobolectricTestRunner.class)
public class AdditionalSystemUpdatePreferenceControllerTest {

    private Context mContext;
    private AdditionalSystemUpdatePreferenceController mController;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);

        mContext = RuntimeEnvironment.application;
        mController = new AdditionalSystemUpdatePreferenceController(mContext);
    }

    @Test
    public void displayPrefs_ifNotAvailable_shouldNotDisplay() {
        assertThat(mController.isAvailable()).isFalse();
    }

    @Test
    @Config(qualifiers = "mcc999")
    public void displayPrefs_ifAvailable_shouldDisplay() {
        assertThat(mController.isAvailable()).isTrue();
    }
}
