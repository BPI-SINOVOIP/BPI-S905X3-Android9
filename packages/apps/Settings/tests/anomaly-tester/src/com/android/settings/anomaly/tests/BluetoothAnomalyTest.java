/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

package com.android.settings.anomaly.tests;

import static com.google.common.truth.Truth.assertWithMessage;

import android.app.Instrumentation;
import android.content.Context;
import android.content.Intent;
import android.support.test.InstrumentationRegistry;
import android.support.test.runner.AndroidJUnit4;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.Until;
import android.text.format.DateUtils;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

/**
 * Functional test for bluetooth unoptimized scanning anomaly detector
 *
 * @see com.android.settings.fuelgauge.anomaly.checker.BluetoothScanAnomalyDetector
 */
@RunWith(AndroidJUnit4.class)
public class BluetoothAnomalyTest {
    private static final String BATTERY_INTENT = "android.intent.action.POWER_USAGE_SUMMARY";
    private static final String RES_BT_EDITTEXT =
            "com.android.settings.anomaly.tester:id/bluetooth_run_time";
    private static final String RES_BT_BUTTON =
            "com.android.settings.anomaly.tester:id/bluetooth_button";
    private static final long TIME_OUT = 3000;
    private UiDevice mDevice;

    @Before
    public void setUp() {
        final Instrumentation instrumentation = InstrumentationRegistry.getInstrumentation();
        final Context context = instrumentation.getContext();
        mDevice = UiDevice.getInstance(instrumentation);

        // setup environment
        TestUtils.setUp(instrumentation);
        // start anomaly-tester app
        TestUtils.startAnomalyApp(context, mDevice);
    }

    @After
    public void tearDown() {
        TestUtils.tearDown(InstrumentationRegistry.getInstrumentation());
    }

    @Test
    public void testBluetoothAnomaly_longScanningTime_reportAnomaly() throws InterruptedException {
        // Set running time
        final long durationMs = DateUtils.SECOND_IN_MILLIS * 15;
        TestUtils.setEditTextWithValue(mDevice, RES_BT_EDITTEXT, durationMs);

        // Click start button
        TestUtils.clickButton(mDevice, RES_BT_BUTTON);

        // Wait for its running
        mDevice.pressHome();
        TestUtils.wait(mDevice, durationMs);

        // Check it in battery main page
        Instrumentation instrumentation = InstrumentationRegistry.getInstrumentation();
        instrumentation.startActivitySync(new Intent(BATTERY_INTENT));
        assertWithMessage("Doesn't have bluetooth anomaly").that(
                mDevice.wait(Until.findObject(By.text("AnomalyTester draining battery")),
                        TIME_OUT)).isNotNull();
    }

    @Test
    public void testBluetoothAnomaly_shortScanningTime_notReport() throws InterruptedException {
        // Set running time
        final long durationMs = DateUtils.SECOND_IN_MILLIS;
        TestUtils.setEditTextWithValue(mDevice, RES_BT_EDITTEXT, durationMs);

        // Click start button
        TestUtils.clickButton(mDevice, RES_BT_BUTTON);

        // Wait for its running
        mDevice.pressHome();
        TestUtils.wait(mDevice, durationMs);

        // Check it in battery main page
        Instrumentation instrumentation = InstrumentationRegistry.getInstrumentation();
        instrumentation.startActivitySync(new Intent(BATTERY_INTENT));
        assertWithMessage("Shouldn't have bluetooth anomaly").that(
                mDevice.wait(Until.findObject(By.text("AnomalyTester draining battery")),
                        TIME_OUT)).isNull();
    }

}
