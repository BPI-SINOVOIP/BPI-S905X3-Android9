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
package com.android.cts.crossprofileappstest;

import static com.google.common.truth.Truth.assertThat;

import static junit.framework.Assert.assertNotNull;

import static org.junit.Assert.assertEquals;

import android.content.ComponentName;
import android.content.Context;
import android.content.pm.CrossProfileApps;
import android.os.Bundle;
import android.os.UserHandle;
import android.os.UserManager;
import android.support.test.InstrumentationRegistry;
import android.support.test.runner.AndroidJUnit4;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.UiObject2;
import android.support.test.uiautomator.Until;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.List;
import java.util.concurrent.TimeUnit;

/**
 * Test that runs {@link CrossProfileApps} APIs against valid target user.
 */
@RunWith(AndroidJUnit4.class)
public class CrossProfileAppsTargetUserTest {
    private static final String PARAM_TARGET_USER = "TARGET_USER";
    private static final String ID_USER_TEXTVIEW =
            "com.android.cts.crossprofileappstest:id/user_textview";
    private static final long TIMEOUT_WAIT_UI = TimeUnit.SECONDS.toMillis(10);

    private CrossProfileApps mCrossProfileApps;
    private UserHandle mTargetUser;
    private Context mContext;
    private UiDevice mDevice;
    private long mUserSerialNumber;

    @Before
    public void setupCrossProfileApps() {
        mContext = InstrumentationRegistry.getContext();
        mCrossProfileApps = mContext.getSystemService(CrossProfileApps.class);
    }

    @Before
    public void wakeupDeviceAndPressHome() throws Exception {
        mDevice = UiDevice.getInstance(InstrumentationRegistry.getInstrumentation());
        mDevice.wakeUp();
        mDevice.pressMenu();
        mDevice.pressHome();
    }

    @Before
    public void readTargetUser() {
        Context context = InstrumentationRegistry.getContext();
        Bundle arguments = InstrumentationRegistry.getArguments();
        UserManager userManager = context.getSystemService(UserManager.class);
        mUserSerialNumber = Long.parseLong(arguments.getString(PARAM_TARGET_USER));
        mTargetUser = userManager.getUserForSerialNumber(mUserSerialNumber);
        assertNotNull(mTargetUser);
    }

    @After
    public void pressHome() {
        mDevice.pressHome();
    }

    @Test
    public void testTargetUserIsIngetTargetUserProfiles() {
        List<UserHandle> targetProfiles = mCrossProfileApps.getTargetUserProfiles();
        assertThat(targetProfiles).contains(mTargetUser);
    }

    /**
     * Verify we succeed to start the activity in another profile by checking UI element.
     */
    @Test
    public void testCanStartMainActivity() throws Exception {
        mCrossProfileApps.startMainActivity(
                MainActivity.getComponentName(mContext), mTargetUser);

        // Look for the text view to verify that MainActivity is started.
        UiObject2 textView = mDevice.wait(
                Until.findObject(By.res(ID_USER_TEXTVIEW)),
                TIMEOUT_WAIT_UI);
        assertNotNull("Failed to start activity in target user", textView);
        // Look for the text in textview, it should be the serial number of target user.
        assertEquals("Activity is started in wrong user",
                String.valueOf(mUserSerialNumber),
                textView.getText());
    }

    @Test(expected = SecurityException.class)
    public void testCannotStartNotExportedActivity() throws Exception {
        mCrossProfileApps.startMainActivity(
                NonExportedActivity.getComponentName(mContext), mTargetUser);
    }

    @Test(expected = SecurityException.class)
    public void testCannotStartNonMainActivity() throws Exception {
        mCrossProfileApps.startMainActivity(
                NonMainActivity.getComponentName(mContext), mTargetUser);
    }

    @Test(expected = SecurityException.class)
    public void testCannotStartActivityInOtherPackage() throws Exception {
        mCrossProfileApps.startMainActivity(new ComponentName(
                "com.android.cts.launcherapps.simpleapp",
                "com.android.cts.launcherapps.simpleapp.SimpleActivity"),
                mTargetUser
        );
    }

    @Test
    public void testGetProfileSwitchingLabel() throws Exception {
        assertNotNull(mCrossProfileApps.getProfileSwitchingLabel(mTargetUser));
    }

    @Test
    public void testGetProfileSwitchingIconDrawable() throws Exception {
        assertNotNull(mCrossProfileApps.getProfileSwitchingIconDrawable(mTargetUser));
    }
}

