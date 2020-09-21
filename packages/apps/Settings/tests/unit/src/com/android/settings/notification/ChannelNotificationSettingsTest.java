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

package com.android.settings.notification;

import static android.app.NotificationManager.IMPORTANCE_MIN;
import static android.app.NotificationManager.IMPORTANCE_NONE;
import static android.support.test.espresso.Espresso.onView;
import static android.support.test.espresso.assertion.ViewAssertions.matches;
import static android.support.test.espresso.matcher.ViewMatchers.isDisplayed;
import static android.support.test.espresso.matcher.ViewMatchers.withId;
import static android.support.test.espresso.matcher.ViewMatchers.withText;

import static org.hamcrest.Matchers.allOf;
import static org.junit.Assert.fail;

import android.app.INotificationManager;
import android.app.Instrumentation;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.content.Context;
import android.content.Intent;
import android.os.Process;
import android.os.ServiceManager;
import android.provider.Settings;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;
import android.support.test.runner.AndroidJUnit4;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
@SmallTest
public class ChannelNotificationSettingsTest {

    private Context mTargetContext;
    private Instrumentation mInstrumentation;
    private NotificationChannel mNotificationChannel;
    private NotificationManager mNm;

    @Before
    public void setUp() {
        mInstrumentation = InstrumentationRegistry.getInstrumentation();
        mTargetContext = mInstrumentation.getTargetContext();

        mNm  = (NotificationManager) mTargetContext.getSystemService(Context.NOTIFICATION_SERVICE);
        mNotificationChannel = new NotificationChannel(this.getClass().getName(),
                this.getClass().getName(), IMPORTANCE_MIN);
        mNm.createNotificationChannel(mNotificationChannel);
    }

    @Test
    public void launchNotificationSetting_shouldNotCrash() {
        final Intent intent = new Intent(Settings.ACTION_CHANNEL_NOTIFICATION_SETTINGS)
                .putExtra(Settings.EXTRA_APP_PACKAGE, mTargetContext.getPackageName())
                .putExtra(Settings.EXTRA_CHANNEL_ID, mNotificationChannel.getId());
        mInstrumentation.startActivitySync(intent);

        onView(allOf(withText(mNotificationChannel.getName().toString()))).check(
                matches(isDisplayed()));
    }

    @Test
    public void launchNotificationSettings_blockedChannel() throws Exception {
        NotificationChannel blocked =
                new NotificationChannel("blocked", "blocked", IMPORTANCE_NONE);
        mNm.createNotificationChannel(blocked);

        INotificationManager sINM = INotificationManager.Stub.asInterface(
                ServiceManager.getService(Context.NOTIFICATION_SERVICE));
        blocked.setImportance(IMPORTANCE_NONE);
        sINM.updateNotificationChannelForPackage(
                mTargetContext.getPackageName(), Process.myUid(), blocked);

        final Intent intent = new Intent(Settings.ACTION_CHANNEL_NOTIFICATION_SETTINGS)
                .putExtra(Settings.EXTRA_APP_PACKAGE, mTargetContext.getPackageName())
                .putExtra(Settings.EXTRA_CHANNEL_ID, blocked.getId());
        mInstrumentation.startActivitySync(intent);

        onView(allOf(withText("Off"), isDisplayed())).check(matches(isDisplayed()));
        onView(allOf(withText("Android is blocking this category of notifications from"
                + " appearing on this device"))).check(matches(isDisplayed()));

        try {
            onView(allOf(withText("On the lock screen"))).check(matches(isDisplayed()));
            fail("settings appearing for blocked channel");
        } catch (Exception e) {
            // expected
        }
    }
}
