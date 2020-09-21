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

package com.android.cts.verifier.sensors;

import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.hardware.TriggerEvent;
import android.hardware.TriggerEventListener;
import android.hardware.cts.helpers.SensorNotSupportedException;
import com.android.cts.verifier.R;
import com.android.cts.verifier.sensors.base.SensorCtsVerifierTestActivity;
import junit.framework.Assert;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

/**
 * Tests about event policy when the observer's UID is idle.
 */
public class EventSanitizationTestActivity extends SensorCtsVerifierTestActivity {
    public EventSanitizationTestActivity() {
        super(EventSanitizationTestActivity.class);
    }

    // time for the test to wait for an event
    private static final int EVENT_TIMEOUT = 30;

    @Override
    protected void activitySetUp() throws Exception {
        getTestLogger().logInstructions(R.string.snsr_event_sanitization_test_setup);
        waitForUserToBegin();
    }

    @Override
    protected void activityCleanUp() throws Exception {
        getTestLogger().logInstructions(R.string.snsr_event_sanitization_test_cleanup);
        waitForUserToContinue();
    }

    /**
     * Test that no trigger events are triggered while the UID is idle.
     */
    public String testNoTriggerEventsWhileUidIdle() throws Exception {
        // Not significant motion sensor, nothing to do.
        final SensorManager sensorManager = getApplicationContext()
                .getSystemService(SensorManager.class);
        final Sensor sensor = sensorManager.getDefaultSensor(
                Sensor.TYPE_SIGNIFICANT_MOTION);
        if (sensor == null) {
            throw new SensorNotSupportedException(Sensor.TYPE_SIGNIFICANT_MOTION);
        }

        // Let us begin.
        final SensorTestLogger logger = getTestLogger();
        logger.logInstructions(R.string.snsr_significant_motion_test_uid_idle);
        waitForUserToBegin();

        // Watch for the trigger event.
        final CountDownLatch latch = new CountDownLatch(1);
        final TriggerEventListener listener = new TriggerEventListener() {
            @Override
            public void onTrigger(TriggerEvent event) {
                latch.countDown();
            }
        };
        sensorManager.requestTriggerSensor(listener, sensor);

        // Tell the user now when the test completes.
        logger.logWaitForSound();

        // We shouldn't be getting an event.
        try {
            Assert.assertFalse(getString(R.string
                    .snsr_significant_motion_test_uid_idle_expectation),
                    latch.await(EVENT_TIMEOUT, TimeUnit.SECONDS));
        } finally {
            sensorManager.cancelTriggerSensor(listener, sensor);
            playSound();
        }

        return null;
    }

    /**
     * Test that no on-change events are triggered while the UID is idle.
     */
    public String testNoOnChangeEventsWhileUidIdle() throws Exception {
        // Not significant motion sensor, nothing to do.
        final SensorManager sensorManager = getApplicationContext()
                .getSystemService(SensorManager.class);
        final Sensor sensor = sensorManager.getDefaultSensor(
                Sensor.TYPE_PROXIMITY);
        if (sensor == null) {
            throw new SensorNotSupportedException(Sensor.TYPE_PROXIMITY);
        }

        // Let us begin.
        final SensorTestLogger logger = getTestLogger();
        logger.logInstructions(R.string.snsr_proximity_test_uid_idle);
        waitForUserToBegin();

        // Watch for the change event.
        final CountDownLatch latch = new CountDownLatch(1);
        final SensorEventListener listener = new SensorEventListener() {
            @Override
            public void onSensorChanged(SensorEvent event) {
                latch.countDown();
            }

            @Override
            public void onAccuracyChanged(Sensor sensor, int accuracy) {
                /* do nothing */
            }
        };
        sensorManager.registerListener(listener, sensor,
                sensor.getMinDelay(), sensor.getMaxDelay());

        // Tell the user now when the test completes.
        logger.logWaitForSound();

        // We shouldn't be getting an event.
        try {
            Assert.assertFalse(getString(R.string
                    .snsr_proximity_test_uid_idle_expectation),
                    latch.await(EVENT_TIMEOUT, TimeUnit.SECONDS));
        } finally {
            sensorManager.unregisterListener(listener, sensor);
            playSound();
        }

        return null;
    }
}
