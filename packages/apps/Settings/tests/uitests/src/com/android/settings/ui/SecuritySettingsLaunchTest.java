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

package com.android.settings.ui;

import android.os.RemoteException;
import android.provider.Settings;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.MediumTest;
import android.support.test.runner.AndroidJUnit4;
import android.support.test.uiautomator.UiDevice;
import android.system.helpers.SettingsHelper;

import com.android.settings.ui.testutils.SettingsTestUtils;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@MediumTest
@RunWith(AndroidJUnit4.class)
public class SecuritySettingsLaunchTest {

    // Items we really want to always show
    private static final String[] CATEGORIES = new String[]{
            "Security status",
            "Device security",
            "Privacy",
    };

    private UiDevice mDevice;
    private SettingsHelper mHelper;

    @Before
    public void setUp() throws Exception {
        mDevice = UiDevice.getInstance(InstrumentationRegistry.getInstrumentation());
        mHelper = SettingsHelper.getInstance();
        try {
            mDevice.setOrientationNatural();
        } catch (RemoteException e) {
            throw new RuntimeException("failed to freeze device orientaion", e);
        }
    }

    @After
    public void tearDown() throws Exception {
        // Go back to home for next test.
        mDevice.pressHome();
    }

    @Test
    public void launchSecuritySettings() throws Exception {
        // Launch Settings
        SettingsHelper.launchSettingsPage(
                InstrumentationRegistry.getTargetContext(), Settings.ACTION_SECURITY_SETTINGS);
        mHelper.scrollVert(false);
        for (String category : CATEGORIES) {
            SettingsTestUtils.assertTitleMatch(mDevice, category);
        }
    }
}
