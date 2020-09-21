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

package com.android.car.storagemonitoring;

import android.car.storagemonitoring.IoStatsEntry;
import android.car.storagemonitoring.UidIoRecord;
import android.test.suitebuilder.annotation.MediumTest;
import android.util.SparseArray;
import com.android.car.procfsinspector.ProcessInfo;
import com.android.car.systeminterface.SystemStateInterface;
import java.time.Duration;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.stream.Collectors;
import junit.framework.TestCase;

/**
 * Tests IoStatsTracker functionality.
 */
@MediumTest
public class IoStatsTrackerTest extends TestCase {
    private static final int SAMPLE_WINDOW_MS = 1000;
    private static final List<IoStatsEntry> EMPTY = Collections.emptyList();
    private static final String TAG = IoStatsTrackerTest.class.getSimpleName();

    public void testNewUsersAppear() throws Exception {
        final MockSystemStateInterface mockSystemStateInterface = new MockSystemStateInterface();
        IoStatsTracker ioStatsTracker = new IoStatsTracker(EMPTY,
            SAMPLE_WINDOW_MS, mockSystemStateInterface);

        assertEquals(0, ioStatsTracker.getCurrentSample().size());
        assertEquals(0, ioStatsTracker.getTotal().size());

        UserActivity user0 = new UserActivity(0);
        user0.foreground_rchar = 50;
        user0.background_wchar = 10;

        UserActivity user1 = new UserActivity(1);
        user1.foreground_rchar = 30;
        user1.background_wchar = 50;

        UidIoRecord process0 = user0.updateSystemState(mockSystemStateInterface);
        UidIoRecord process1 = user1.updateSystemState(mockSystemStateInterface);

        ioStatsTracker.update(mockSystemStateInterface.mIoRecords);

        assertEquals(2, ioStatsTracker.getCurrentSample().size());
        assertEquals(2, ioStatsTracker.getTotal().size());

        assertTrue(ioStatsTracker.getCurrentSample().get(0).representsSameMetrics(process0));
        assertTrue(ioStatsTracker.getCurrentSample().get(1).representsSameMetrics(process1));

        assertTrue(ioStatsTracker.getTotal().get(0).representsSameMetrics(process0));
        assertTrue(ioStatsTracker.getTotal().get(1).representsSameMetrics(process1));
    }

    public void testUserMetricsChange() throws Exception {
        final MockSystemStateInterface mockSystemStateInterface = new MockSystemStateInterface();
        IoStatsTracker ioStatsTracker = new IoStatsTracker(EMPTY,
            SAMPLE_WINDOW_MS, mockSystemStateInterface);

        UserActivity user0 = new UserActivity(0);
        user0.foreground_rchar = 50;
        user0.background_wchar = 10;

        user0.updateSystemState(mockSystemStateInterface);
        ioStatsTracker.update(mockSystemStateInterface.mIoRecords);

        user0.foreground_rchar = 60;
        user0.foreground_wchar = 10;
        UidIoRecord process0 = user0.updateSystemState(mockSystemStateInterface);
        ioStatsTracker.update(mockSystemStateInterface.mIoRecords);

        assertEquals(1, ioStatsTracker.getCurrentSample().size());
        assertEquals(1, ioStatsTracker.getTotal().size());

        IoStatsEntry sample0 = ioStatsTracker.getCurrentSample().get(0);
        IoStatsEntry total0 = ioStatsTracker.getTotal().get(0);

        assertNotNull(sample0);
        assertNotNull(total0);

        assertTrue(total0.representsSameMetrics(process0));

        assertEquals(10, sample0.foreground.bytesRead);
        assertEquals(10, sample0.foreground.bytesWritten);
        assertEquals(0, sample0.background.bytesWritten);
    }

    public void testUpdateNoIoProcessActive() throws Exception {
        final MockSystemStateInterface mockSystemStateInterface = new MockSystemStateInterface();
        IoStatsTracker ioStatsTracker = new IoStatsTracker(EMPTY,
            SAMPLE_WINDOW_MS, mockSystemStateInterface);

        UserActivity user0 = new UserActivity(0);
        user0.foreground_rchar = 50;
        user0.background_wchar = 10;

        user0.updateSystemState(mockSystemStateInterface);
        ioStatsTracker.update(mockSystemStateInterface.mIoRecords);

        user0.spawnProcess();
        user0.updateSystemState(mockSystemStateInterface);
        ioStatsTracker.update(mockSystemStateInterface.mIoRecords);

        assertEquals(1, ioStatsTracker.getCurrentSample().size());
        assertEquals(1, ioStatsTracker.getTotal().size());

        IoStatsEntry sample0 = ioStatsTracker.getCurrentSample().get(0);
        IoStatsEntry total0 = ioStatsTracker.getTotal().get(0);

        assertEquals(2 * SAMPLE_WINDOW_MS, sample0.runtimeMillis);
        assertEquals(2 * SAMPLE_WINDOW_MS, total0.runtimeMillis);

        assertEquals(0, sample0.foreground.bytesRead);
        assertEquals(0, sample0.background.bytesWritten);
    }

    public void testUpdateNoIoProcessInactive() throws Exception {
        final MockSystemStateInterface mockSystemStateInterface = new MockSystemStateInterface();
        IoStatsTracker ioStatsTracker = new IoStatsTracker(EMPTY,
            SAMPLE_WINDOW_MS, mockSystemStateInterface);

        UserActivity user0 = new UserActivity(0);
        user0.foreground_rchar = 50;
        user0.background_wchar = 10;

        user0.updateSystemState(mockSystemStateInterface);
        ioStatsTracker.update(mockSystemStateInterface.mIoRecords);

        user0.killProcess();
        UidIoRecord record0 = user0.updateSystemState(mockSystemStateInterface);
        ioStatsTracker.update(mockSystemStateInterface.mIoRecords);

        assertEquals(0, ioStatsTracker.getCurrentSample().size());
        assertEquals(1, ioStatsTracker.getTotal().size());

        IoStatsEntry total0 = ioStatsTracker.getTotal().get(0);
        assertEquals(SAMPLE_WINDOW_MS, total0.runtimeMillis);
        assertTrue(total0.representsSameMetrics(record0));
    }

    public void testUpdateIoHappens() throws Exception {
        final MockSystemStateInterface mockSystemStateInterface = new MockSystemStateInterface();
        IoStatsTracker ioStatsTracker = new IoStatsTracker(EMPTY,
            SAMPLE_WINDOW_MS, mockSystemStateInterface);

        UserActivity user0 = new UserActivity(0);
        user0.foreground_rchar = 50;
        user0.background_wchar = 10;

        user0.updateSystemState(mockSystemStateInterface);
        ioStatsTracker.update(mockSystemStateInterface.mIoRecords);

        user0.foreground_rchar = 60;
        UidIoRecord record0 = user0.updateSystemState(mockSystemStateInterface);
        ioStatsTracker.update(mockSystemStateInterface.mIoRecords);

        assertEquals(1, ioStatsTracker.getCurrentSample().size());
        assertEquals(1, ioStatsTracker.getTotal().size());

        IoStatsEntry sample0 = ioStatsTracker.getCurrentSample().get(0);
        IoStatsEntry total0 = ioStatsTracker.getTotal().get(0);

        assertTrue(total0.representsSameMetrics(record0));
        assertEquals(2 * SAMPLE_WINDOW_MS, total0.runtimeMillis);
        assertEquals(2 * SAMPLE_WINDOW_MS, sample0.runtimeMillis);
        assertEquals(10, sample0.foreground.bytesRead);
        assertEquals(0, sample0.background.bytesWritten);
    }

    public void testUpdateGoAwayComeBackProcess() throws Exception {
        final MockSystemStateInterface mockSystemStateInterface = new MockSystemStateInterface();
        IoStatsTracker ioStatsTracker = new IoStatsTracker(EMPTY,
            SAMPLE_WINDOW_MS, mockSystemStateInterface);

        UserActivity user0 = new UserActivity(0);
        user0.foreground_rchar = 50;
        user0.background_wchar = 10;

        user0.updateSystemState(mockSystemStateInterface);
        ioStatsTracker.update(mockSystemStateInterface.mIoRecords);

        ioStatsTracker.update(mockSystemStateInterface.mIoRecords);

        assertEquals(0, ioStatsTracker.getCurrentSample().size());
        assertEquals(1, ioStatsTracker.getTotal().size());

        user0.spawnProcess();
        user0.updateSystemState(mockSystemStateInterface);
        ioStatsTracker.update(mockSystemStateInterface.mIoRecords);

        assertEquals(1, ioStatsTracker.getCurrentSample().size());
        IoStatsEntry sample0 = ioStatsTracker.getCurrentSample().get(0);
        assertEquals(2 * SAMPLE_WINDOW_MS, sample0.runtimeMillis);
    }

    public void testUpdateGoAwayComeBackIo() throws Exception {
        final MockSystemStateInterface mockSystemStateInterface = new MockSystemStateInterface();
        IoStatsTracker ioStatsTracker = new IoStatsTracker(EMPTY,
            SAMPLE_WINDOW_MS, mockSystemStateInterface);

        UserActivity user0 = new UserActivity(0);
        user0.foreground_rchar = 50;
        user0.background_wchar = 10;

        user0.updateSystemState(mockSystemStateInterface);
        ioStatsTracker.update(mockSystemStateInterface.mIoRecords);

        ioStatsTracker.update(mockSystemStateInterface.mIoRecords);

        assertEquals(0, ioStatsTracker.getCurrentSample().size());
        assertEquals(1, ioStatsTracker.getTotal().size());

        user0.foreground_fsync = 1;

        user0.updateSystemState(mockSystemStateInterface);
        ioStatsTracker.update(mockSystemStateInterface.mIoRecords);

        assertEquals(1, ioStatsTracker.getCurrentSample().size());

        IoStatsEntry sample0 = ioStatsTracker.getCurrentSample().get(0);
        assertEquals(2 * SAMPLE_WINDOW_MS, sample0.runtimeMillis);
        assertEquals(1, sample0.foreground.fsyncCalls);
    }

    private static final class UserActivity {
        private final int mUid;
        private boolean mHasProcess;

        private long foreground_rchar;
        private long foreground_wchar;
        private long foreground_read_bytes;
        private long foreground_write_bytes;
        private long foreground_fsync;

        private long background_rchar;
        private long background_wchar;
        private long background_read_bytes;
        private long background_write_bytes;
        private long background_fsync;

        UserActivity(int uid) {
            mUid = uid;
        }

        void spawnProcess() {
            mHasProcess = true;
        }
        void killProcess() {
            mHasProcess = false;
        }

        UidIoRecord updateSystemState(MockSystemStateInterface systemState) {
            UidIoRecord uidIoRecord = new UidIoRecord(mUid,
                foreground_rchar,
                foreground_wchar,
                foreground_read_bytes,
                foreground_write_bytes,
                foreground_fsync,
                background_rchar,
                background_wchar,
                background_read_bytes,
                background_write_bytes,
                background_fsync);

            systemState.addIoRecord(uidIoRecord);
            if (mHasProcess) {
                systemState.addProcess(new ProcessInfo(1, mUid));
            } else {
                systemState.removeUserProcesses(mUid);
            }

            return uidIoRecord;
        }
    }

    private final class MockSystemStateInterface implements SystemStateInterface {
        private final List<ProcessInfo> mProcesses = new ArrayList<>();
        private final SparseArray<UidIoRecord> mIoRecords = new SparseArray<>();

        @Override
        public void shutdown() {
        }

        @Override
        public boolean enterDeepSleep(int wakeupTimeSec) {
            return true;
        }

        @Override
        public void scheduleActionForBootCompleted(Runnable action, Duration delay) {
        }

        @Override
        public boolean isWakeupCausedByTimer() {
            return false;
        }

        @Override
        public boolean isSystemSupportingDeepSleep() {
            return false;
        }

        @Override
        public synchronized List<ProcessInfo> getRunningProcesses() {
            return mProcesses;
        }

        synchronized void addProcess(ProcessInfo processInfo) {
            mProcesses.add(processInfo);
        }

        synchronized void removeUserProcesses(int uid) {
            mProcesses.removeAll(
                    mProcesses.stream().filter(pi -> pi.uid == uid).collect(Collectors.toList()));
        }

        synchronized void addIoRecord(UidIoRecord record) {
            mIoRecords.put(record.uid, record);
        }

        synchronized void clear() {
            mProcesses.clear();
            mIoRecords.clear();
        }
    }
}
