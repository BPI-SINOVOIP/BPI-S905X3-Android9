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

package android.jobscheduler.cts;

import android.annotation.TargetApi;
import android.app.job.JobInfo;
import android.support.test.InstrumentationRegistry;
import android.support.test.uiautomator.UiDevice;

/**
 * Make sure the state of {@link android.app.job.JobScheduler} is correct.
 */
@TargetApi(28)
public class DeviceStatesTest extends ConstraintTest {
    /** Unique identifier for the job scheduled by this suite of tests. */
    public static final int STATE_JOB_ID = DeviceStatesTest.class.hashCode();

    private JobInfo.Builder mBuilder;
    private UiDevice mUiDevice;

    @Override
    public void setUp() throws Exception {
        super.setUp();
        mBuilder = new JobInfo.Builder(STATE_JOB_ID, kJobServiceComponent);
        mUiDevice = UiDevice.getInstance(InstrumentationRegistry.getInstrumentation());
    }

    @Override
    public void tearDown() throws Exception {
        mJobScheduler.cancel(STATE_JOB_ID);
        // Put device back in to normal operation.
        toggleScreenOn(true /* screen on */);
    }

    void assertJobReady() throws Exception {
        assertJobReady(STATE_JOB_ID);
    }

    void assertJobWaiting() throws Exception {
        assertJobWaiting(STATE_JOB_ID);
    }

    void assertJobNotReady() throws Exception {
        assertJobNotReady(STATE_JOB_ID);
    }

    /**
     * Toggle device is dock idle or dock active.
     */
    private void toggleFakeDeviceDockState(final boolean idle) throws Exception {
        mUiDevice.executeShellCommand("cmd jobscheduler trigger-dock-state "
                + (idle ? "idle" : "active"));
    }

    /**
     * Make sure the screen state.
     */
    private void toggleScreenOn(final boolean screenon) throws Exception {
        if (screenon) {
            mUiDevice.wakeUp();
        } else {
            mUiDevice.sleep();
        }
        // Since the screen on/off intent is ordered, they will not be sent right now.
        Thread.sleep(3000);
    }

    /**
     * Simulated for idle, and then perform idle maintenance now.
     */
    private void triggerIdleMaintenance() throws Exception {
        mUiDevice.executeShellCommand("cmd activity idle-maintenance");
    }

    /**
     * Schedule a job that requires the device is idle, and assert it fired to make
     * sure the device is idle.
     */
    void verifyIdleState() throws Exception {
        kTestEnvironment.setExpectedExecutions(1);
        kTestEnvironment.setExpectedWaitForRun();
        mJobScheduler.schedule(mBuilder.setRequiresDeviceIdle(true).build());
        assertJobReady();
        kTestEnvironment.readyToRun();

        assertTrue("Job with idle constraint did not fire on idle",
                kTestEnvironment.awaitExecution());
    }

    /**
     * Schedule a job that requires the device is idle, and assert it failed to make
     * sure the device is active.
     */
    void verifyActiveState() throws Exception {
        kTestEnvironment.setExpectedExecutions(0);
        kTestEnvironment.setExpectedWaitForRun();
        mJobScheduler.schedule(mBuilder.setRequiresDeviceIdle(true).build());
        assertJobWaiting();
        assertJobNotReady();
        kTestEnvironment.readyToRun();

        assertFalse("Job with idle constraint fired while not on idle.",
                kTestEnvironment.awaitExecution(250));
    }

    /**
     * Ensure that device can switch state normally.
     */
    public void testDeviceChangeIdleActiveState() throws Exception {
        toggleScreenOn(true /* screen on */);
        verifyActiveState();

        // Assert device is idle when screen is off for a while.
        toggleScreenOn(false /* screen off */);
        triggerIdleMaintenance();
        verifyIdleState();

        // Assert device is back to active when screen is on.
        toggleScreenOn(true /* screen on */);
        verifyActiveState();
    }

    /**
     * Ensure that device can switch state on dock normally.
     */
    public void testScreenOnDeviceOnDockChangeState() throws Exception {
        toggleScreenOn(true /* screen on */);
        verifyActiveState();

        // Assert device go to idle if user doesn't interact with device for a while.
        toggleFakeDeviceDockState(true /* idle */);
        triggerIdleMaintenance();
        verifyIdleState();

        // Assert device go back to active if user interacts with device.
        toggleFakeDeviceDockState(false /* active */);
        verifyActiveState();
    }

    /**
     *  Ensure that ignores this dock intent during screen off.
     */
    public void testScreenOffDeviceOnDockNoChangeState() throws Exception {
        toggleScreenOn(false /* screen off */);
        triggerIdleMaintenance();
        verifyIdleState();

        toggleFakeDeviceDockState(false /* active */);
        verifyIdleState();
    }
}
