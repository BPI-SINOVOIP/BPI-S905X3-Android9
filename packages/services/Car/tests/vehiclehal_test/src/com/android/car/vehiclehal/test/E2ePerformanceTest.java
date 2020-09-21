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
package com.android.car.vehiclehal.test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import static java.lang.Integer.toHexString;

import android.annotation.Nullable;
import android.car.Car;
import android.car.CarNotConnectedException;
import android.car.hardware.CarPropertyConfig;
import android.car.hardware.CarSensorManager;
import android.car.hardware.CarSensorManager.OnSensorChangedListener;
import android.car.hardware.hvac.CarHvacManager;
import android.hardware.automotive.vehicle.V2_0.VehicleProperty;
import android.os.SystemClock;
import android.support.test.filters.MediumTest;
import android.support.test.runner.AndroidJUnit4;
import android.util.Log;
import android.util.SparseArray;
import android.util.SparseIntArray;

import com.google.android.collect.Lists;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

/**
 * This test suite will make e2e test and measure some performance characteristics. The main idea
 * is to send command to Vehicle HAL to generate some events with certain time interval and capture
 * these events through car public API, e.g. CarSensorManager.
 *
 * TODO(pavelm): benchmark tests might be flaky, need a way to run them multiple times / use avg
 *               metrics.
 */
@MediumTest
@RunWith(AndroidJUnit4.class)
public class E2ePerformanceTest extends E2eCarTestBase {
    private static String TAG = Utils.concatTag(E2ePerformanceTest.class);

    @Test
    public void singleOnChangeProperty() throws Exception {
        verifyEventsFromSingleProperty(
                CarSensorManager.SENSOR_TYPE_ODOMETER, VehicleProperty.PERF_ODOMETER);
    }

    @Test
    public void singleContinuousProperty() throws Exception {
        verifyEventsFromSingleProperty(
                CarSensorManager.SENSOR_TYPE_CAR_SPEED, VehicleProperty.PERF_VEHICLE_SPEED);
    }

    @Test
    public void benchmarkEventBandwidthThroughCarService() throws Exception {
        int[] mgrProperties = new int[] {
                CarSensorManager.SENSOR_TYPE_ODOMETER,
                CarSensorManager.SENSOR_TYPE_CAR_SPEED
        };
        // Expecting to receive at least 10 events within 150ms.
        final int EVENT_INTERVAL_MS = 1; //
        final int EXPECTED_EVENTS_PER_PROPERTY = 1000;
        final int EXPECTED_EVENTS = EXPECTED_EVENTS_PER_PROPERTY * mgrProperties.length;

        CarSensorManager mgr = (CarSensorManager) mCar.getCarManager(Car.SENSOR_SERVICE);
        assertNotNull(mgr);
        for (int mgrPropId: mgrProperties) {
            assertTrue("PropId: 0x" + toHexString(mgrPropId) + " is not supported",
                    mgr.isSensorSupported(mgrPropId));
        }

        VhalEventGenerator odometerGenerator = new LinearVhalEventGenerator(mVehicle)
                .setProp(VehicleProperty.PERF_ODOMETER)
                .setIntervalMs(EVENT_INTERVAL_MS)
                .setInitialValue(1000)
                .setIncrement(1.0f)
                .setDispersion(100);


        VhalEventGenerator speedGenerator = new LinearVhalEventGenerator(mVehicle)
                .setProp(VehicleProperty.PERF_VEHICLE_SPEED)
                .setIntervalMs(EVENT_INTERVAL_MS)
                .setInitialValue(20.0f)
                .setIncrement(0.1f)
                .setDispersion(10);

        odometerGenerator.start();
        speedGenerator.start();

        SparseArray<CountDownLatch> eventsCounters = new SparseArray<>();
        for (int i = 0; i < mgrProperties.length; i++) {
            eventsCounters.put(mgrProperties[i], new CountDownLatch(EXPECTED_EVENTS_PER_PROPERTY));
        }
        OnSensorChangedListener listener = e -> eventsCounters.get(e.sensorType).countDown();
        for (int propId: mgrProperties) {
            mgr.registerListener(listener, propId, CarSensorManager.SENSOR_RATE_FASTEST);
        }

        final long WAIT_TIME = (long) ((EVENT_INTERVAL_MS * EXPECTED_EVENTS_PER_PROPERTY) * 1.6);
        CountDownLatch[] latches = new CountDownLatch[eventsCounters.size()];
        for (int i = 0; i < eventsCounters.size(); i++) {
            latches[i] = eventsCounters.valueAt(i);
        }
        boolean allEventsReceived = awaitCountDownLatches(latches, WAIT_TIME);
        mgr.unregisterListener(listener);

        odometerGenerator.stop();
        speedGenerator.stop();

        if (!allEventsReceived) {
            SparseIntArray missingEventsPerProperty = new SparseIntArray();
            for (int i = 0; i < eventsCounters.size(); i++) {
                int missingEvents = (int) eventsCounters.valueAt(i).getCount();
                if (missingEvents > 0) {
                    missingEventsPerProperty.put(eventsCounters.keyAt(i), missingEvents);
                }
            }

            assertTrue("Too slow. Expected to receive: " + EXPECTED_EVENTS
                            + " within " + WAIT_TIME + " ms, "
                            + " missing events per property: " + missingEventsPerProperty,
                    missingEventsPerProperty.size() == 0);
        }
    }

    @Test
    public void benchmarkSetGetFromSingleClient() throws Exception {
        final int PROP = CarHvacManager.ID_WINDOW_DEFROSTER_ON;
        CarHvacManager mgr = (CarHvacManager) mCar.getCarManager(Car.HVAC_SERVICE);
        assertNotNull(mgr);
        long start = SystemClock.elapsedRealtimeNanos();

        final long TEST_DURATION_NANO = 1_000_000_000;  // 1 second.
        final long EXPECTED_ITERATIONS = 100; // We expect to have at least 100 get/set calls.

        boolean value = false;
        long actualIterations = 0;
        while (SystemClock.elapsedRealtimeNanos() < start + TEST_DURATION_NANO) {
            mgr.setBooleanProperty(PROP, 1, value);
            boolean actualValue = mgr.getBooleanProperty(PROP, 1);
            assertEquals(value, actualValue);
            value = !value;
            actualIterations++;
        }
        assertTrue("Too slow. Expected iterations: " + EXPECTED_ITERATIONS
                + ", actual: " + actualIterations
                + ", test duration: " + TEST_DURATION_NANO / 1000_1000 +  " ms.",
                actualIterations >= EXPECTED_ITERATIONS);
    }

    @Test
    public void benchmarkSetGetFromSingleClientMultipleThreads() throws Exception {
        final int PROP = CarHvacManager.ID_ZONED_TEMP_SETPOINT;
        CarHvacManager mgr = (CarHvacManager) mCar.getCarManager(Car.HVAC_SERVICE);
        assertNotNull(mgr);

        CarPropertyConfig<Float> cfg = findHvacPropConfig(Float.class, PROP, mgr);
        assertNotNull(cfg);
        assertTrue("Expected at least 2 zones for 0x" + Integer.toHexString(PROP)
                        + ", got: " + cfg.getAreaCount(), cfg.getAreaCount() >= 2);


        final int EXPECTED_INVOCATIONS = 1000;  // How many time get/set will be called.
        final int EXPECTED_DURATION_MS = 2500;
        // This is a stress test and it can be flaky because it shares resources with all currently
        // running process. Let's have this number of attempt before giving up.
        final int ATTEMPTS = 3;

        for (int curAttempt = 0; curAttempt < ATTEMPTS; curAttempt++) {
            long missingInvocations = stressTestHvacProperties(mgr, cfg,
                    EXPECTED_INVOCATIONS, EXPECTED_DURATION_MS);
            if (missingInvocations == 0) return;  // All done.

            Log.w(TAG, "Failed to invoke get/set " + EXPECTED_INVOCATIONS
                            + " within " + EXPECTED_DURATION_MS + "ms"
                            + ", actually invoked: "
                            + (EXPECTED_INVOCATIONS - missingInvocations));
        }
        fail("Failed to invoke get/set " + EXPECTED_INVOCATIONS + " within "
                + EXPECTED_DURATION_MS + "ms. Number of attempts: " + ATTEMPTS
                + ". See logs for details.");
    }

    private long stressTestHvacProperties(CarHvacManager mgr, CarPropertyConfig<Float> cfg,
            int EXPECTED_INVOCATIONS, int EXPECTED_DURATION_MS) throws InterruptedException {
        CountDownLatch counter = new CountDownLatch(EXPECTED_INVOCATIONS);

        List<Thread> threads = new ArrayList<>(Lists.newArrayList(
            new Thread(() -> invokeSetAndGetForHvacFloat(mgr, cfg, cfg.getAreaIds()[0], counter)),
            new Thread(() -> invokeSetAndGetForHvacFloat(mgr, cfg, cfg.getAreaIds()[1], counter))));

        for (Thread t : threads) {
            t.start();
        }

        counter.await(EXPECTED_DURATION_MS, TimeUnit.MILLISECONDS);
        long missingInvocations = counter.getCount();

        for (Thread t : threads) {
            t.join(10000);  // Let thread join to not interfere with other test.
            assertFalse(t.isAlive());
        }
        return missingInvocations;
    }

    private void invokeSetAndGetForHvacFloat(CarHvacManager mgr,
            CarPropertyConfig<Float> cfg, int areaId, CountDownLatch counter) {
        float minValue = cfg.getMinValue(areaId);
        float maxValue = cfg.getMaxValue(areaId);
        float curValue = minValue;

        while (counter.getCount() > 0) {
            float actualValue;
            try {
                mgr.setFloatProperty(cfg.getPropertyId(), areaId, curValue);
                actualValue = mgr.getFloatProperty(cfg.getPropertyId(), areaId);
            } catch (CarNotConnectedException e) {
                throw new RuntimeException(e);
            }
            assertEquals(curValue, actualValue, 0.001);
            curValue += 0.5;
            if (curValue > maxValue) {
                curValue = minValue;
            }

            counter.countDown();
        }
    }

    @Nullable
    private <T> CarPropertyConfig<T> findHvacPropConfig(Class<T> clazz, int hvacPropId,
                CarHvacManager mgr)
            throws CarNotConnectedException {
        for (CarPropertyConfig<?> cfg : mgr.getPropertyList()) {
            if (cfg.getPropertyId() == hvacPropId) {
                return (CarPropertyConfig<T>) cfg;
            }
        }
        return null;
    }

    private void verifyEventsFromSingleProperty(int mgrPropId, int halPropId) throws Exception {
        // Expecting to receive at least 10 events within 150ms.
        final int EXPECTED_EVENTS = 10;
        final int EXPECTED_TIME_DURATION_MS = 150;
        final float INITIAL_VALUE = 1000;
        final float INCREMENT = 1.0f;

        CarSensorManager mgr = (CarSensorManager) mCar.getCarManager(Car.SENSOR_SERVICE);
        assertNotNull(mgr);
        assertTrue(mgr.isSensorSupported(mgrPropId));

        VhalEventGenerator generator = new LinearVhalEventGenerator(mVehicle)
                .setProp(halPropId)
                .setIntervalMs(10)
                .setInitialValue(INITIAL_VALUE)
                .setIncrement(INCREMENT)
                .setDispersion(100);

        generator.start();

        CountDownLatch latch = new CountDownLatch(EXPECTED_EVENTS);
        OnSensorChangedListener listener = event -> latch.countDown();

        mgr.registerListener(listener, mgrPropId, CarSensorManager.SENSOR_RATE_FASTEST);
        try {
            assertTrue(latch.await(EXPECTED_TIME_DURATION_MS, TimeUnit.MILLISECONDS));
        } finally {
            generator.stop();
            mgr.unregisterListener(listener);
        }
    }


    private static boolean awaitCountDownLatches(CountDownLatch[] latches, long timeoutMs)
            throws InterruptedException {
        long start = SystemClock.elapsedRealtime();
        for (int i = 0; i < latches.length; i++) {
            long timeLeft = timeoutMs - (SystemClock.elapsedRealtime() - start);
            if (!latches[i].await(timeLeft, TimeUnit.MILLISECONDS)) {
                return false;
            }
        }

        return true;
    }
}
