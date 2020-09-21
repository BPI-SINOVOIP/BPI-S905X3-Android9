/*
 * Copyright (C) 2016 The Android Open Source Project
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
package com.android.bluetooth.gatt;

import android.bluetooth.le.ScanSettings;
import android.os.Binder;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.SystemClock;
import android.os.WorkSource;
import android.util.StatsLog;

import com.android.bluetooth.BluetoothMetricsProto;
import com.android.internal.app.IBatteryStats;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;

/**
 * ScanStats class helps keep track of information about scans
 * on a per application basis.
 * @hide
 */
/*package*/ class AppScanStats {
    private static final String TAG = AppScanStats.class.getSimpleName();

    static final DateFormat DATE_FORMAT = new SimpleDateFormat("MM-dd HH:mm:ss");

    /* ContextMap here is needed to grab Apps and Connections */ ContextMap mContextMap;

    /* GattService is needed to add scan event protos to be dumped later */ GattService
            mGattService;

    /* Battery stats is used to keep track of scans and result stats */ IBatteryStats mBatteryStats;

    class LastScan {
        public long duration;
        public long suspendDuration;
        public long suspendStartTime;
        public boolean isSuspended;
        public long timestamp;
        public boolean opportunistic;
        public boolean timeout;
        public boolean background;
        public boolean filtered;
        public int results;
        public int scannerId;

        LastScan(long timestamp, long duration, boolean opportunistic, boolean background,
                boolean filtered, int scannerId) {
            this.duration = duration;
            this.timestamp = timestamp;
            this.opportunistic = opportunistic;
            this.background = background;
            this.filtered = filtered;
            this.results = 0;
            this.scannerId = scannerId;
            this.suspendDuration = 0;
            this.suspendStartTime = 0;
            this.isSuspended = false;
        }
    }

    static final int NUM_SCAN_DURATIONS_KEPT = 5;

    // This constant defines the time window an app can scan multiple times.
    // Any single app can scan up to |NUM_SCAN_DURATIONS_KEPT| times during
    // this window. Once they reach this limit, they must wait until their
    // earliest recorded scan exits this window.
    static final long EXCESSIVE_SCANNING_PERIOD_MS = 30 * 1000;

    // Maximum msec before scan gets downgraded to opportunistic
    static final int SCAN_TIMEOUT_MS = 30 * 60 * 1000;

    public String appName;
    public WorkSource mWorkSource; // Used for BatteryStats and StatsLog
    private int mScansStarted = 0;
    private int mScansStopped = 0;
    public boolean isRegistered = false;
    private long mMinScanTime = Long.MAX_VALUE;
    private long mMaxScanTime = 0;
    private long mScanStartTime = 0;
    private long mTotalScanTime = 0;
    private long mTotalSuspendTime = 0;
    private List<LastScan> mLastScans = new ArrayList<LastScan>(NUM_SCAN_DURATIONS_KEPT);
    private HashMap<Integer, LastScan> mOngoingScans = new HashMap<Integer, LastScan>();
    public long startTime = 0;
    public long stopTime = 0;
    public int results = 0;

    AppScanStats(String name, WorkSource source, ContextMap map, GattService service) {
        appName = name;
        mContextMap = map;
        mGattService = service;
        mBatteryStats = IBatteryStats.Stub.asInterface(ServiceManager.getService("batterystats"));

        if (source == null) {
            // Bill the caller if the work source isn't passed through
            source = new WorkSource(Binder.getCallingUid(), appName);
        }
        mWorkSource = source;
    }

    synchronized void addResult(int scannerId) {
        LastScan scan = getScanFromScannerId(scannerId);
        if (scan != null) {
            int batteryStatsResults = ++scan.results;

            // Only update battery stats after receiving 100 new results in order
            // to lower the cost of the binder transaction
            if (batteryStatsResults % 100 == 0) {
                try {
                    mBatteryStats.noteBleScanResults(mWorkSource, 100);
                } catch (RemoteException e) {
                    /* ignore */
                }
            }
        }

        results++;
    }

    boolean isScanning() {
        return !mOngoingScans.isEmpty();
    }

    LastScan getScanFromScannerId(int scannerId) {
        return mOngoingScans.get(scannerId);
    }

    synchronized void recordScanStart(ScanSettings settings, boolean filtered, int scannerId) {
        LastScan existingScan = getScanFromScannerId(scannerId);
        if (existingScan != null) {
            return;
        }
        this.mScansStarted++;
        startTime = SystemClock.elapsedRealtime();

        LastScan scan = new LastScan(startTime, 0, false, false, filtered, scannerId);
        if (settings != null) {
            scan.opportunistic = settings.getScanMode() == ScanSettings.SCAN_MODE_OPPORTUNISTIC;
            scan.background =
                    (settings.getCallbackType() & ScanSettings.CALLBACK_TYPE_FIRST_MATCH) != 0;
        }

        BluetoothMetricsProto.ScanEvent scanEvent = BluetoothMetricsProto.ScanEvent.newBuilder()
                .setScanEventType(BluetoothMetricsProto.ScanEvent.ScanEventType.SCAN_EVENT_START)
                .setScanTechnologyType(
                        BluetoothMetricsProto.ScanEvent.ScanTechnologyType.SCAN_TECH_TYPE_LE)
                .setEventTimeMillis(System.currentTimeMillis())
                .setInitiator(truncateAppName(appName)).build();
        mGattService.addScanEvent(scanEvent);

        if (!isScanning()) {
            mScanStartTime = startTime;
        }
        try {
            boolean isUnoptimized = !(scan.filtered || scan.background || scan.opportunistic);
            mBatteryStats.noteBleScanStarted(mWorkSource, isUnoptimized);
        } catch (RemoteException e) {
            /* ignore */
        }
        writeToStatsLog(scan, StatsLog.BLE_SCAN_STATE_CHANGED__STATE__ON);

        mOngoingScans.put(scannerId, scan);
    }

    synchronized void recordScanStop(int scannerId) {
        LastScan scan = getScanFromScannerId(scannerId);
        if (scan == null) {
            return;
        }
        this.mScansStopped++;
        stopTime = SystemClock.elapsedRealtime();
        long scanDuration = stopTime - scan.timestamp;
        scan.duration = scanDuration;
        if (scan.isSuspended) {
            long suspendDuration = stopTime - scan.suspendStartTime;
            scan.suspendDuration += suspendDuration;
            mTotalSuspendTime += suspendDuration;
        }
        mOngoingScans.remove(scannerId);
        if (mLastScans.size() >= NUM_SCAN_DURATIONS_KEPT) {
            mLastScans.remove(0);
        }
        mLastScans.add(scan);

        BluetoothMetricsProto.ScanEvent scanEvent = BluetoothMetricsProto.ScanEvent.newBuilder()
                .setScanEventType(BluetoothMetricsProto.ScanEvent.ScanEventType.SCAN_EVENT_STOP)
                .setScanTechnologyType(
                        BluetoothMetricsProto.ScanEvent.ScanTechnologyType.SCAN_TECH_TYPE_LE)
                .setEventTimeMillis(System.currentTimeMillis())
                .setInitiator(truncateAppName(appName))
                .setNumberResults(scan.results)
                .build();
        mGattService.addScanEvent(scanEvent);

        if (!isScanning()) {
            long totalDuration = stopTime - mScanStartTime;
            mTotalScanTime += totalDuration;
            mMinScanTime = Math.min(totalDuration, mMinScanTime);
            mMaxScanTime = Math.max(totalDuration, mMaxScanTime);
        }

        try {
            // Inform battery stats of any results it might be missing on
            // scan stop
            boolean isUnoptimized = !(scan.filtered || scan.background || scan.opportunistic);
            mBatteryStats.noteBleScanResults(mWorkSource, scan.results % 100);
            mBatteryStats.noteBleScanStopped(mWorkSource, isUnoptimized);
        } catch (RemoteException e) {
            /* ignore */
        }
        writeToStatsLog(scan, StatsLog.BLE_SCAN_STATE_CHANGED__STATE__OFF);
    }

    synchronized void recordScanSuspend(int scannerId) {
        LastScan scan = getScanFromScannerId(scannerId);
        if (scan == null || scan.isSuspended) {
            return;
        }
        scan.suspendStartTime = SystemClock.elapsedRealtime();
        scan.isSuspended = true;
    }

    synchronized void recordScanResume(int scannerId) {
        LastScan scan = getScanFromScannerId(scannerId);
        long suspendDuration = 0;
        if (scan == null || !scan.isSuspended) {
            return;
        }
        scan.isSuspended = false;
        stopTime = SystemClock.elapsedRealtime();
        suspendDuration = stopTime - scan.suspendStartTime;
        scan.suspendDuration += suspendDuration;
        mTotalSuspendTime += suspendDuration;
    }

    private void writeToStatsLog(LastScan scan, int statsLogState) {
        for (int i = 0; i < mWorkSource.size(); i++) {
            StatsLog.write_non_chained(StatsLog.BLE_SCAN_STATE_CHANGED,
                    mWorkSource.get(i), null,
                    statsLogState, scan.filtered, scan.background, scan.opportunistic);
        }

        final List<WorkSource.WorkChain> workChains = mWorkSource.getWorkChains();
        if (workChains != null) {
            for (int i = 0; i < workChains.size(); ++i) {
                WorkSource.WorkChain workChain = workChains.get(i);
                StatsLog.write(StatsLog.BLE_SCAN_STATE_CHANGED,
                        workChain.getUids(), workChain.getTags(),
                        statsLogState, scan.filtered, scan.background, scan.opportunistic);
            }
        }
    }

    synchronized void setScanTimeout(int scannerId) {
        if (!isScanning()) {
            return;
        }

        LastScan scan = getScanFromScannerId(scannerId);
        if (scan != null) {
            scan.timeout = true;
        }
    }

    synchronized boolean isScanningTooFrequently() {
        if (mLastScans.size() < NUM_SCAN_DURATIONS_KEPT) {
            return false;
        }

        return (SystemClock.elapsedRealtime() - mLastScans.get(0).timestamp)
                < EXCESSIVE_SCANNING_PERIOD_MS;
    }

    synchronized boolean isScanningTooLong() {
        if (!isScanning()) {
            return false;
        }
        return (SystemClock.elapsedRealtime() - mScanStartTime) > SCAN_TIMEOUT_MS;
    }

    // This function truncates the app name for privacy reasons. Apps with
    // four part package names or more get truncated to three parts, and apps
    // with three part package names names get truncated to two. Apps with two
    // or less package names names are untouched.
    // Examples: one.two.three.four => one.two.three
    //           one.two.three => one.two
    private String truncateAppName(String name) {
        String initiator = name;
        String[] nameSplit = initiator.split("\\.");
        if (nameSplit.length > 3) {
            initiator = nameSplit[0] + "." + nameSplit[1] + "." + nameSplit[2];
        } else if (nameSplit.length == 3) {
            initiator = nameSplit[0] + "." + nameSplit[1];
        }

        return initiator;
    }

    synchronized void dumpToString(StringBuilder sb) {
        long currTime = SystemClock.elapsedRealtime();
        long maxScan = mMaxScanTime;
        long minScan = mMinScanTime;
        long scanDuration = 0;

        if (isScanning()) {
            scanDuration = currTime - mScanStartTime;
        }
        minScan = Math.min(scanDuration, minScan);
        maxScan = Math.max(scanDuration, maxScan);

        if (minScan == Long.MAX_VALUE) {
            minScan = 0;
        }

        /*TODO: Average scan time can be skewed for
         * multiple scan clients. It will show less than
         * actual value.
         * */
        long avgScan = 0;
        long totalScanTime = mTotalScanTime + scanDuration;
        if (mScansStarted > 0) {
            avgScan = totalScanTime / mScansStarted;
        }

        sb.append("  " + appName);
        if (isRegistered) {
            sb.append(" (Registered)");
        }

        if (!mLastScans.isEmpty()) {
            LastScan lastScan = mLastScans.get(mLastScans.size() - 1);
            if (lastScan.opportunistic) {
                sb.append(" (Opportunistic)");
            }
            if (lastScan.background) {
                sb.append(" (Background)");
            }
            if (lastScan.timeout) {
                sb.append(" (Forced-Opportunistic)");
            }
            if (lastScan.filtered) {
                sb.append(" (Filtered)");
            }
        }
        sb.append("\n");

        sb.append("  LE scans (started/stopped)         : " + mScansStarted + " / " + mScansStopped
                + "\n");
        sb.append("  Scan time in ms (min/max/avg/total): " + minScan + " / " + maxScan + " / "
                + avgScan + " / " + totalScanTime + "\n");
        if (mTotalSuspendTime != 0) {
            sb.append("  Total time suspended               : " + mTotalSuspendTime + "ms\n");
        }
        sb.append("  Total number of results            : " + results + "\n");

        long currentTime = System.currentTimeMillis();
        long elapsedRt = SystemClock.elapsedRealtime();
        if (!mLastScans.isEmpty()) {
            sb.append("  Last " + mLastScans.size() + " scans                       :\n");

            for (int i = 0; i < mLastScans.size(); i++) {
                LastScan scan = mLastScans.get(i);
                Date timestamp = new Date(currentTime - elapsedRt + scan.timestamp);
                sb.append("    " + DATE_FORMAT.format(timestamp) + " - ");
                sb.append(scan.duration + "ms ");
                if (scan.opportunistic) {
                    sb.append("Opp ");
                }
                if (scan.background) {
                    sb.append("Back ");
                }
                if (scan.timeout) {
                    sb.append("Forced ");
                }
                if (scan.filtered) {
                    sb.append("Filter ");
                }
                sb.append(scan.results + " results");
                sb.append(" (" + scan.scannerId + ")");
                sb.append("\n");
                if (scan.suspendDuration != 0) {
                    sb.append("      └" + " Suspended Time: " + scan.suspendDuration + "ms\n");
                }
            }
        }

        if (!mOngoingScans.isEmpty()) {
            sb.append("  Ongoing scans                      :\n");
            for (Integer key : mOngoingScans.keySet()) {
                LastScan scan = mOngoingScans.get(key);
                Date timestamp = new Date(currentTime - elapsedRt + scan.timestamp);
                sb.append("    " + DATE_FORMAT.format(timestamp) + " - ");
                sb.append((elapsedRt - scan.timestamp) + "ms ");
                if (scan.opportunistic) {
                    sb.append("Opp ");
                }
                if (scan.background) {
                    sb.append("Back ");
                }
                if (scan.timeout) {
                    sb.append("Forced ");
                }
                if (scan.filtered) {
                    sb.append("Filter ");
                }
                if (scan.isSuspended) {
                    sb.append("Suspended ");
                }
                sb.append(scan.results + " results");
                sb.append(" (" + scan.scannerId + ")");
                sb.append("\n");
                if (scan.suspendStartTime != 0) {
                    long duration = scan.suspendDuration + (scan.isSuspended ? (elapsedRt
                            - scan.suspendStartTime) : 0);
                    sb.append("      └" + " Suspended Time: " + duration + "ms\n");
                }
            }
        }

        ContextMap.App appEntry = mContextMap.getByName(appName);
        if (appEntry != null && isRegistered) {
            sb.append("  Application ID                     : " + appEntry.id + "\n");
            sb.append("  UUID                               : " + appEntry.uuid + "\n");

            List<ContextMap.Connection> connections = mContextMap.getConnectionByApp(appEntry.id);

            sb.append("  Connections: " + connections.size() + "\n");

            Iterator<ContextMap.Connection> ii = connections.iterator();
            while (ii.hasNext()) {
                ContextMap.Connection connection = ii.next();
                long connectionTime = elapsedRt - connection.startTime;
                Date timestamp = new Date(currentTime - elapsedRt + connection.startTime);
                sb.append("    " + DATE_FORMAT.format(timestamp) + " - ");
                sb.append((connectionTime) + "ms ");
                sb.append(": " + connection.address + " (" + connection.connId + ")\n");
            }
        }
        sb.append("\n");
    }
}
