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

package com.android.settings.applications;

import android.app.AppOpsManager;
import android.provider.Settings;
import android.support.test.filters.LargeTest;
import android.support.test.runner.AndroidJUnit4;

import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
@LargeTest
public class ExternalSourcesSettingsTest extends AppOpsSettingsTest {

    public ExternalSourcesSettingsTest() {
        super(Settings.ACTION_MANAGE_UNKNOWN_APP_SOURCES,
                AppOpsManager.OP_REQUEST_INSTALL_PACKAGES);
    }

    // Test cases are in the superclass.
}
