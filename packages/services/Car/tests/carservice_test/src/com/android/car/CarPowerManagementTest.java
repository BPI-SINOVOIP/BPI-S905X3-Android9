/*
 * Copyright (C) 2015 The Android Open Source Project
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
package com.android.car;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.fail;

import android.hardware.automotive.vehicle.V2_0.VehicleApPowerBootupReason;
import android.hardware.automotive.vehicle.V2_0.VehicleApPowerStateConfigFlag;
import android.hardware.automotive.vehicle.V2_0.VehicleApPowerStateReport;
import android.hardware.automotive.vehicle.V2_0.VehicleApPowerStateReq;
import android.hardware.automotive.vehicle.V2_0.VehicleApPowerStateReqIndex;
import android.hardware.automotive.vehicle.V2_0.VehicleApPowerStateShutdownParam;
import android.hardware.automotive.vehicle.V2_0.VehiclePropValue;
import android.hardware.automotive.vehicle.V2_0.VehicleProperty;
import android.os.SystemClock;
import android.support.test.annotation.UiThreadTest;
import android.support.test.filters.MediumTest;
import android.support.test.runner.AndroidJUnit4;

import com.android.car.systeminterface.DisplayInterface;
import com.android.car.systeminterface.SystemInterface;
import com.android.car.vehiclehal.VehiclePropValueBuilder;
import com.android.car.vehiclehal.test.MockedVehicleHal.VehicleHalPropertyHandler;

import com.google.android.collect.Lists;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.ArrayList;
import java.util.LinkedList;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

@RunWith(AndroidJUnit4.class)
@MediumTest
public class CarPowerManagementTest extends MockedCarTestBase {

    private final PowerStatePropertyHandler mPowerStateHandler = new PowerStatePropertyHandler();
    private final MockDisplayInterface mMockDisplayInterface = new MockDisplayInterface();

    @Override
    protected synchronized SystemInterface.Builder getSystemInterfaceBuilder() {
        SystemInterface.Builder builder = super.getSystemInterfaceBuilder();
        return builder.withDisplayInterface(mMockDisplayInterface);
    }

    private void setupPowerPropertyAndStart(boolean allowSleep) throws Exception {
        addProperty(VehicleProperty.AP_POWER_STATE_REQ, mPowerStateHandler)
                .setConfigArray(Lists.newArrayList(
                        allowSleep ? VehicleApPowerStateConfigFlag.ENABLE_DEEP_SLEEP_FLAG : 0));
        addProperty(VehicleProperty.AP_POWER_STATE_REPORT, mPowerStateHandler);

        addStaticProperty(VehicleProperty.AP_POWER_BOOTUP_REASON,
                VehiclePropValueBuilder.newBuilder(VehicleProperty.AP_POWER_BOOTUP_REASON)
                        .addIntValue(VehicleApPowerBootupReason.USER_POWER_ON)
                        .build());

        reinitializeMockedHal();
    }

    @Test
    @UiThreadTest
    public void testImmediateShutdown() throws Exception {
        setupPowerPropertyAndStart(true);
        assertBootComplete();
        mPowerStateHandler.sendPowerState(
                VehicleApPowerStateReq.SHUTDOWN_PREPARE,
                VehicleApPowerStateShutdownParam.SHUTDOWN_IMMEDIATELY);
        mPowerStateHandler.waitForStateSetAndGetAll(DEFAULT_WAIT_TIMEOUT_MS,
                VehicleApPowerStateReport.SHUTDOWN_START);
        mPowerStateHandler.sendPowerState(VehicleApPowerStateReq.ON_FULL, 0);
    }

    @Test
    @UiThreadTest
    public void testDisplayOnOff() throws Exception {
        setupPowerPropertyAndStart(true);
        assertBootComplete();
        for (int i = 0; i < 2; i++) {
            mPowerStateHandler.sendPowerState(VehicleApPowerStateReq.ON_DISP_OFF, 0);
            mMockDisplayInterface.waitForDisplayState(false);
            mPowerStateHandler.sendPowerState(VehicleApPowerStateReq.ON_FULL, 0);
            mMockDisplayInterface.waitForDisplayState(true);
        }
    }

    /* TODO make deep sleep work to test this
    @Test public void testSleepEntry() throws Exception {
        assertBootComplete();
        mPowerStateHandler.sendPowerState(
                VehicleApPowerState.SHUTDOWN_PREPARE,
                VehicleApPowerStateShutdownParam.CAN_SLEEP);
        assertResponse(VehicleApPowerSetState.DEEP_SLEEP_ENTRY, 0);
        assertResponse(VehicleApPowerSetState.DEEP_SLEEP_EXIT, 0);
        mPowerStateHandler.sendPowerState(
                VehicleApPowerState.ON_FULL,
                0);
    }*/

    private void assertResponse(int expectedResponseState, int expectedResponseParam)
            throws Exception {
        LinkedList<int[]> setEvents = mPowerStateHandler.waitForStateSetAndGetAll(
                DEFAULT_WAIT_TIMEOUT_MS, expectedResponseState);
        int[] last = setEvents.getLast();
        assertEquals(expectedResponseState, last[0]);
        assertEquals(expectedResponseParam, last[1]);
    }

    private void assertBootComplete() throws Exception {
        mPowerStateHandler.waitForSubscription(DEFAULT_WAIT_TIMEOUT_MS);
        LinkedList<int[]> setEvents = mPowerStateHandler.waitForStateSetAndGetAll(
                DEFAULT_WAIT_TIMEOUT_MS, VehicleApPowerStateReport.BOOT_COMPLETE);
        int[] first = setEvents.getFirst();
        assertEquals(VehicleApPowerStateReport.BOOT_COMPLETE, first[0]);
        assertEquals(0, first[1]);
    }

    private final class MockDisplayInterface implements DisplayInterface {
        private boolean mDisplayOn = true;
        private final Semaphore mDisplayStateWait = new Semaphore(0);

        @Override
        public void setDisplayBrightness(int brightness) {}

        @Override
        public synchronized void setDisplayState(boolean on) {
            mDisplayOn = on;
            mDisplayStateWait.release();
        }

        boolean waitForDisplayState(boolean expectedState)
            throws Exception {
            if (expectedState == mDisplayOn) {
                return true;
            }
            mDisplayStateWait.tryAcquire(MockedCarTestBase.SHORT_WAIT_TIMEOUT_MS,
                    TimeUnit.MILLISECONDS);
            return expectedState == mDisplayOn;
        }

        @Override
        public void startDisplayStateMonitoring(CarPowerManagementService service) {}

        @Override
        public void stopDisplayStateMonitoring() {}
    }

    private class PowerStatePropertyHandler implements VehicleHalPropertyHandler {

        private int mPowerState = VehicleApPowerStateReq.ON_FULL;
        private int mPowerParam = 0;

        private final Semaphore mSubscriptionWaitSemaphore = new Semaphore(0);
        private final Semaphore mSetWaitSemaphore = new Semaphore(0);
        private LinkedList<int[]> mSetStates = new LinkedList<>();

        @Override
        public void onPropertySet(VehiclePropValue value) {
            ArrayList<Integer> v = value.value.int32Values;
            synchronized (this) {
                mSetStates.add(new int[] {
                        v.get(VehicleApPowerStateReqIndex.STATE),
                        v.get(VehicleApPowerStateReqIndex.ADDITIONAL)
                });
            }
            mSetWaitSemaphore.release();
        }

        @Override
        public synchronized VehiclePropValue onPropertyGet(VehiclePropValue value) {
            return VehiclePropValueBuilder.newBuilder(VehicleProperty.AP_POWER_STATE_REQ)
                    .setTimestamp(SystemClock.elapsedRealtimeNanos())
                    .addIntValue(mPowerState, mPowerParam)
                    .build();
        }

        @Override
        public void onPropertySubscribe(int property, float sampleRate) {
            mSubscriptionWaitSemaphore.release();
        }

        @Override
        public void onPropertyUnsubscribe(int property) {
            //ignore
        }

        private synchronized void setCurrentState(int state, int param) {
            mPowerState = state;
            mPowerParam = param;
        }

        private void waitForSubscription(long timeoutMs) throws Exception {
            if (!mSubscriptionWaitSemaphore.tryAcquire(timeoutMs, TimeUnit.MILLISECONDS)) {
                fail("waitForSubscription timeout");
            }
        }

        private LinkedList<int[]> waitForStateSetAndGetAll(long timeoutMs, int expectedSet)
                throws Exception {
            while (true) {
                if (!mSetWaitSemaphore.tryAcquire(timeoutMs, TimeUnit.MILLISECONDS)) {
                    fail("waitForStateSetAndGetAll timeout");
                }
                synchronized (this) {
                    boolean found = false;
                    for (int[] state : mSetStates) {
                        if (state[0] == expectedSet) {
                            found = true;
                        }
                    }
                    if (found) {
                        LinkedList<int[]> res = mSetStates;
                        mSetStates = new LinkedList<>();
                        return res;
                    }
                }
            }
        }

        private void sendPowerState(int state, int param) {
            getMockedVehicleHal().injectEvent(
                    VehiclePropValueBuilder.newBuilder(VehicleProperty.AP_POWER_STATE_REQ)
                            .setTimestamp(SystemClock.elapsedRealtimeNanos())
                            .addIntValue(state, param)
                            .build());
        }
    }
}
