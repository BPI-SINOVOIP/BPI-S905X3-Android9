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
package com.android.car;

import android.test.suitebuilder.annotation.MediumTest;
import com.android.car.systeminterface.TimeInterface;
import com.android.car.test.utils.TemporaryFile;

import junit.framework.TestCase;

@MediumTest
public class UptimeTrackerTest extends TestCase {
    static final String TAG = UptimeTrackerTest.class.getSimpleName();

    static final class MockTimeInterface implements TimeInterface {
        private long mCurrentTime = 0;
        private Runnable mRunnable = null;

        MockTimeInterface incrementTime(long by) {
            mCurrentTime += by;
            return this;
        }

        MockTimeInterface tick() {
            if (mRunnable != null) {
                mRunnable.run();
            }
            return this;
        }

        @Override
        public long getUptime(boolean includeDeepSleepTime) {
            return mCurrentTime;
        }

        @Override
        public void scheduleAction(Runnable r, long delayMs) {
            if (mRunnable != null) {
                throw new IllegalStateException("task already scheduled");
            }
            mRunnable = r;
        }

        @Override
        public void cancelAllActions() {
            mRunnable = null;
        }
    }

    private static final long SNAPSHOT_INTERVAL = 0; // actual time doesn't matter for this test

    public void testUptimeTrackerFromCleanSlate() throws Exception {
        MockTimeInterface timeInterface = new MockTimeInterface();
        try (TemporaryFile uptimeFile = new TemporaryFile(TAG)) {
            UptimeTracker uptimeTracker = new UptimeTracker(uptimeFile.getFile(),
                SNAPSHOT_INTERVAL, timeInterface);

            assertEquals(0, uptimeTracker.getTotalUptime());

            timeInterface.incrementTime(5000).tick();
            assertEquals(5000, uptimeTracker.getTotalUptime());

            timeInterface.tick();
            assertEquals(5000, uptimeTracker.getTotalUptime());

            timeInterface.incrementTime(1000).tick();
            assertEquals(6000, uptimeTracker.getTotalUptime());

            timeInterface.incrementTime(400).tick();
            assertEquals(6400, uptimeTracker.getTotalUptime());
        }
    }

    public void testUptimeTrackerWithHistoricalState() throws Exception {
        MockTimeInterface timeInterface = new MockTimeInterface();
        try (TemporaryFile uptimeFile = new TemporaryFile(TAG)) {
            uptimeFile.write("{\"uptime\" : 5000}");
            UptimeTracker uptimeTracker = new UptimeTracker(uptimeFile.getFile(),
                SNAPSHOT_INTERVAL, timeInterface);

            assertEquals(5000, uptimeTracker.getTotalUptime());

            timeInterface.incrementTime(5000).tick();
            assertEquals(10000, uptimeTracker.getTotalUptime());

            timeInterface.incrementTime(1000).tick();
            assertEquals(11000, uptimeTracker.getTotalUptime());
        }
    }

    public void testUptimeTrackerAcrossHistoricalState() throws Exception {
        MockTimeInterface timeInterface = new MockTimeInterface();
        try (TemporaryFile uptimeFile = new TemporaryFile(TAG)) {
            uptimeFile.write("{\"uptime\" : 5000}");
            UptimeTracker uptimeTracker = new UptimeTracker(uptimeFile.getFile(),
                SNAPSHOT_INTERVAL, timeInterface);

            assertEquals(5000, uptimeTracker.getTotalUptime());

            timeInterface.incrementTime(5000).tick();
            assertEquals(10000, uptimeTracker.getTotalUptime());

            timeInterface.incrementTime(500).tick();
            uptimeTracker.onDestroy();
            timeInterface.cancelAllActions();

            uptimeTracker = new UptimeTracker(uptimeFile.getFile(),
                SNAPSHOT_INTERVAL, timeInterface);

            timeInterface.incrementTime(3000).tick();
            assertEquals(13500, uptimeTracker.getTotalUptime());
        }
    }

    public void testUptimeTrackerShutdown() throws Exception {
        MockTimeInterface timeInterface = new MockTimeInterface();
        try (TemporaryFile uptimeFile = new TemporaryFile(TAG)) {
            UptimeTracker uptimeTracker = new UptimeTracker(uptimeFile.getFile(),
                SNAPSHOT_INTERVAL, timeInterface);

            timeInterface.incrementTime(6000);
            uptimeTracker.onDestroy();
            timeInterface.cancelAllActions();

            uptimeTracker = new UptimeTracker(uptimeFile.getFile(),
                SNAPSHOT_INTERVAL, timeInterface);
            assertEquals(6000, uptimeTracker.getTotalUptime());
        }
    }
}
