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
 * limitations under the License
 */
package com.android.settings.system;

import static com.google.common.truth.Truth.assertThat;

import android.content.Context;
import android.os.UserManager;
import android.provider.Settings;

import com.android.settings.testutils.SettingsRobolectricTestRunner;
import com.android.settings.testutils.shadow.ShadowUtils;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.Shadows;
import org.robolectric.shadows.ShadowUserManager;

@RunWith(SettingsRobolectricTestRunner.class)
public class FactoryResetPreferenceControllerTest {

    private static final String FACTORY_RESET_KEY = "factory_reset";

    private ShadowUserManager mShadowUserManager;

    private Context mContext;
    private FactoryResetPreferenceController mController;

    @Before
    public void setUp() {
        mContext = RuntimeEnvironment.application;
        UserManager userManager = (UserManager) mContext.getSystemService(Context.USER_SERVICE);
        mShadowUserManager = Shadows.shadowOf(userManager);

        mController = new FactoryResetPreferenceController(mContext);
    }

    @After
    public void tearDown() {
        ShadowUtils.reset();
        mShadowUserManager.setIsAdminUser(false);
        mShadowUserManager.setIsDemoUser(false);
        Settings.Global.putInt(mContext.getContentResolver(), Settings.Global.DEVICE_DEMO_MODE, 0);
    }

    @Test
    public void isAvailable_systemUser() {
        mShadowUserManager.setIsAdminUser(true);

        assertThat(mController.isAvailable()).isTrue();
    }

    @Test
    public void isAvailable_nonSystemUser() {
        mShadowUserManager.setIsAdminUser(false);
        mShadowUserManager.setIsDemoUser(false);

        assertThat(mController.isAvailable()).isFalse();
    }

    @Test
    public void isAvailable_demoUser() {
        mShadowUserManager.setIsAdminUser(false);

        // Place the device in demo mode.
        Settings.Global.putInt(mContext.getContentResolver(), Settings.Global.DEVICE_DEMO_MODE, 1);

        // Indicate the user is a demo user.
        mShadowUserManager.setIsDemoUser(true);

        assertThat(mController.isAvailable()).isTrue();
    }

    @Test
    public void getPreferenceKey() {
        assertThat(mController.getPreferenceKey()).isEqualTo(FACTORY_RESET_KEY);
    }
}
