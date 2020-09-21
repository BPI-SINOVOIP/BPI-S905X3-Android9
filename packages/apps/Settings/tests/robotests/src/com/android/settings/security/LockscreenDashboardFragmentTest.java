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

package com.android.settings.security;

import static com.google.common.truth.Truth.assertThat;

import com.android.settings.testutils.SettingsRobolectricTestRunner;
import com.android.settings.testutils.XmlTestUtils;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RuntimeEnvironment;

import java.util.List;

@RunWith(SettingsRobolectricTestRunner.class)
public class LockscreenDashboardFragmentTest {

    private LockscreenDashboardFragment mFragment;

    @Test
    public void containsNotificationSettingsForPrimaryUserAndWorkProfile() {
        mFragment = new LockscreenDashboardFragment();

        List<String> keys = XmlTestUtils.getKeysFromPreferenceXml(RuntimeEnvironment.application,
                mFragment.getPreferenceScreenResId());

        assertThat(keys).containsAllOf(LockscreenDashboardFragment.KEY_LOCK_SCREEN_NOTIFICATON,
                LockscreenDashboardFragment.KEY_LOCK_SCREEN_NOTIFICATON_WORK_PROFILE,
                LockscreenDashboardFragment.KEY_LOCK_SCREEN_NOTIFICATON_WORK_PROFILE_HEADER);
    }
}
