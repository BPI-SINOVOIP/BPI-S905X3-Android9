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

package com.android.settings.system;

import static com.google.common.truth.Truth.assertThat;

import android.content.Context;

import com.android.settings.testutils.SettingsRobolectricTestRunner;
import com.android.settings.testutils.XmlTestUtils;
import com.android.settings.testutils.shadow.SettingsShadowResources;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;

import java.util.List;

@RunWith(SettingsRobolectricTestRunner.class)
@Config(shadows = SettingsShadowResources.class)
public class SystemDashboardFragmentTest {

    @Before
    public void setup() {
        SettingsShadowResources.overrideResource(
                com.android.internal.R.bool.config_supportSystemNavigationKeys, true);
    }

    @After
    public void tearDown() {
        SettingsShadowResources.reset();
    }

    @Test
    public void testNonIndexableKeys_existInXmlLayout() {
        final Context context = RuntimeEnvironment.application;
        final List<String> niks = SystemDashboardFragment.SEARCH_INDEX_DATA_PROVIDER
                .getNonIndexableKeys(context);
        final int xmlId = (new SystemDashboardFragment()).getPreferenceScreenResId();

        final List<String> keys = XmlTestUtils.getKeysFromPreferenceXml(context, xmlId);

        assertThat(keys).containsAllIn(niks);
    }
}
