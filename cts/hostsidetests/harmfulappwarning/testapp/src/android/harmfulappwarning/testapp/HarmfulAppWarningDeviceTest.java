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

package android.harmfulappwarning.testapp;

import com.android.compatibility.common.util.BlockingBroadcastReceiver;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import android.app.Instrumentation;
import android.content.Intent;
import android.support.test.InstrumentationRegistry;
import android.support.test.runner.AndroidJUnit4;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.UiObject2;
import android.support.test.uiautomator.Until;

import java.util.concurrent.TimeUnit;

/**
 * This is the device-side part of the tests for the harmful app launch warnings.
 *
 * <p>These tests are triggered by their counterparts in the {@link HarmfulAppWarningTest}
 * host-side cts test.
 */
@RunWith(AndroidJUnit4.class)
public class HarmfulAppWarningDeviceTest {

    private static final long TIMEOUT_MILLIS = TimeUnit.SECONDS.toMillis(1);

    private static final String ACTION_ACTIVITY_STARTED =
            "android.harmfulappwarning.sampleapp.ACTIVITY_STARTED";

    private static final String TEST_APP_PACKAGE_NAME = "android.harmfulappwarning.sampleapp";
    private static final String TEST_APP_ACTIVITY_CLASS_NAME =
            "android.harmfulappwarning.sampleapp.SampleDeviceActivity";

    private BlockingBroadcastReceiver mSampleActivityStartedReceiver;

    protected static Instrumentation getInstrumentation() {
        return InstrumentationRegistry.getInstrumentation();
    }

    protected static UiDevice getUiDevice() {
        return UiDevice.getInstance(getInstrumentation());
    }

    private static void launchActivity(String packageName, String activityClassName) {
        Intent intent = new Intent(Intent.ACTION_MAIN);
        intent.setClassName(packageName, activityClassName);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        getInstrumentation().getTargetContext().startActivity(intent, null);
    }

    private void verifyWarningShown() {
        getUiDevice().wait(Until.findObject(By.res("android:id/message")), TIMEOUT_MILLIS);
        UiObject2 obj = getUiDevice().findObject(By.res("android:id/message"));
        Assert.assertEquals(obj.getText(), "This is a warning message.");
        Assert.assertNotNull(obj);
    }

    private void verifySampleActivityLaunched() {
        Assert.assertNotNull("Sample activity not launched.",
                mSampleActivityStartedReceiver.awaitForBroadcast(TIMEOUT_MILLIS));
    }

    private void verifySampleActivityNotLaunched() {
        Assert.assertNull("Sample activity was unexpectedly launched.",
                mSampleActivityStartedReceiver.awaitForBroadcast(TIMEOUT_MILLIS));
    }

    private void clickLaunchAnyway() {
        UiObject2 obj = getUiDevice().findObject(By.res("android:id/button2"));
        Assert.assertNotNull(obj);
        obj.click();
    }

    private void clickUninstall() {
        UiObject2 obj = getUiDevice().findObject(By.res("android:id/button1"));
        Assert.assertNotNull(obj);
        obj.click();
    }

    @Before
    public void setUp() {
        mSampleActivityStartedReceiver = new BlockingBroadcastReceiver(
                InstrumentationRegistry.getContext(), ACTION_ACTIVITY_STARTED);
        mSampleActivityStartedReceiver.register();
    }

    @After
    public void tearDown() {
        mSampleActivityStartedReceiver.unregisterQuietly();
    }

    @Test
    public void testNormalLaunch() throws Exception {
        launchActivity(TEST_APP_PACKAGE_NAME, TEST_APP_ACTIVITY_CLASS_NAME);
        verifySampleActivityLaunched();
    }

    @Test
    public void testLaunchAnyway() throws Exception {
        launchActivity(TEST_APP_PACKAGE_NAME, TEST_APP_ACTIVITY_CLASS_NAME);
        verifyWarningShown();
        clickLaunchAnyway();
        verifySampleActivityLaunched();
    }

    @Test
    public void testUninstall() throws Exception {
        launchActivity(TEST_APP_PACKAGE_NAME, TEST_APP_ACTIVITY_CLASS_NAME);
        verifyWarningShown();
        clickUninstall();
        verifySampleActivityNotLaunched();
    }

    @Test
    public void testDismissDialog() throws Exception {
        launchActivity(TEST_APP_PACKAGE_NAME, TEST_APP_ACTIVITY_CLASS_NAME);
        verifyWarningShown();
        getUiDevice().pressHome();
        verifySampleActivityNotLaunched();
    }
}
