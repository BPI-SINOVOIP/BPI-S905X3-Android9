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
package com.android.settings.core;

import static com.google.common.truth.Truth.assertThat;

import android.content.Context;
import android.support.v14.preference.SwitchPreference;

import com.android.settings.slices.SliceData;
import com.android.settings.testutils.SettingsRobolectricTestRunner;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RuntimeEnvironment;

@RunWith(SettingsRobolectricTestRunner.class)
public class TogglePreferenceControllerTest {

    FakeToggle mToggleController;

    Context mContext;
    SwitchPreference mPreference;

    @Before
    public void setUp() {
        mContext = RuntimeEnvironment.application;
        mPreference = new SwitchPreference(mContext);
        mToggleController = new FakeToggle(mContext, "key");
    }

    @Test
    public void testSetsPreferenceValue_setsChecked() {
        mToggleController.setChecked(true);
        mPreference.setChecked(false);

        mToggleController.updateState(mPreference);

        assertThat(mPreference.isChecked()).isTrue();
    }

    @Test
    public void testSetsPreferenceValue_setsNotChecked() {
        mToggleController.setChecked(false);
        mPreference.setChecked(true);

        mToggleController.updateState(mPreference);

        assertThat(mPreference.isChecked()).isFalse();
    }

    @Test
    public void testUpdatesPreferenceOnChange_turnsOn() {
        boolean newValue = true;
        mToggleController.setChecked(!newValue);

        mToggleController.onPreferenceChange(mPreference, newValue);

        assertThat(mToggleController.isChecked()).isEqualTo(newValue);
    }

    @Test
    public void testUpdatesPreferenceOnChange_turnsOff() {
        boolean newValue = false;
        mToggleController.setChecked(!newValue);

        mToggleController.onPreferenceChange(mPreference, newValue);

        assertThat(mToggleController.isChecked()).isEqualTo(newValue);
    }

    @Test
    public void testSliceType_returnsSliceType() {
        assertThat(mToggleController.getSliceType()).isEqualTo(
                SliceData.SliceType.SWITCH);
    }

    private static class FakeToggle extends TogglePreferenceController {

        private boolean checkedFlag;

        public FakeToggle(Context context, String preferenceKey) {
            super(context, preferenceKey);
        }

        @Override
        public boolean isChecked() {
            return checkedFlag;
        }

        @Override
        public boolean setChecked(boolean isChecked) {
            checkedFlag = isChecked;
            return true;
        }

        @Override
        public int getAvailabilityStatus() {
            return AVAILABLE;
        }
    }
}
