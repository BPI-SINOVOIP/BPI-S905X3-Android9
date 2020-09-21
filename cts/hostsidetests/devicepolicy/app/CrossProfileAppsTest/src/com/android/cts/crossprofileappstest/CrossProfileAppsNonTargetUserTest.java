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

import android.content.Context;
import android.content.pm.CrossProfileApps;
import android.os.Bundle;
import android.os.UserHandle;
import android.os.UserManager;
import android.support.test.InstrumentationRegistry;
import android.support.test.runner.AndroidJUnit4;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.List;

/**
 * Test that runs {@link CrossProfileApps} APIs against non-valid target user.
 */
@RunWith(AndroidJUnit4.class)
public class CrossProfileAppsNonTargetUserTest {
    private static final String PARAM_TARGET_USER = "TARGET_USER";

    private CrossProfileApps mCrossProfileApps;
    private UserHandle mTargetUser;
    private Context mContext;

    @Before
    public void setupCrossProfileApps() throws Exception {
        mContext = InstrumentationRegistry.getContext();
        mCrossProfileApps = mContext.getSystemService(CrossProfileApps.class);
    }

    @Before
    public void readTargetUser() {
        Context context = InstrumentationRegistry.getContext();
        Bundle arguments = InstrumentationRegistry.getArguments();
        UserManager userManager = context.getSystemService(UserManager.class);
        final int userSn = Integer.parseInt(arguments.getString(PARAM_TARGET_USER));
        mTargetUser = userManager.getUserForSerialNumber(userSn);
    }

    @Test
    public void testTargetUserIsNotInGetTargetProfiles() {
        List<UserHandle> targetProfiles = mCrossProfileApps.getTargetUserProfiles();
        assertThat(targetProfiles).doesNotContain(mTargetUser);
    }

    @Test(expected = SecurityException.class)
    public void testCannotStartActivity() {
        mCrossProfileApps.startMainActivity(
                MainActivity.getComponentName(mContext), mTargetUser);
    }

    @Test(expected = SecurityException.class)
    public void testCannotGetProfileSwitchingLabel() throws Exception {
        mCrossProfileApps.getProfileSwitchingLabel(mTargetUser);
    }

    @Test(expected = SecurityException.class)
    public void testCannotGetProfileSwitchingIconDrawable() throws Exception {
        mCrossProfileApps.getProfileSwitchingIconDrawable(mTargetUser);
    }
}

