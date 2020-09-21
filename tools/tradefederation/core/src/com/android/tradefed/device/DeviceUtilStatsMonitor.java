/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.tradefed.device;

import com.android.tradefed.command.remote.DeviceDescriptor;
import com.android.tradefed.config.GlobalConfiguration;
import com.android.tradefed.config.Option;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.CircularByteArray;

import java.util.HashMap;
import java.util.Hashtable;
import java.util.Map;
import java.util.Timer;
import java.util.TimerTask;

/**
 * A {@link IDeviceMonitor} that calculates device utilization stats.
 * <p/>
 * Currently measures simple moving average of allocation time % over a 24 hour window.
 */
public class DeviceUtilStatsMonitor implements IDeviceMonitor {

    private static final int INITIAL_DELAY_MS = 5000;

    /**
     * Enum for configuring treatment of stub devices when calculating average host utilization
     */
    public enum StubDeviceUtil {
        /** never include stub device data */
        IGNORE,
        /**
         * include stub device data only if any stub device of same type is allocated at least
         * once
         */
        INCLUDE_IF_USED,
        /** always include stub device data */
        ALWAYS_INCLUDE
    }

    @Option(name = "collect-null-device", description =
            "controls if null device data should be used when calculating avg host utilization")
    private StubDeviceUtil mCollectNullDevice = StubDeviceUtil.INCLUDE_IF_USED;

    @Option(name = "collect-emulator", description =
            "controls if emulator data should be used when calculating avg host utilization")
    private StubDeviceUtil mCollectEmulator = StubDeviceUtil.INCLUDE_IF_USED;

    @Option(name = "sample-window-hours", description =
            "the moving average window size to use, in hours")
    private int mSampleWindowHours = 8;

    @Option(name = "sample-interval-secs", description =
            "the time period between samples, in seconds")
    private int mSamplingIntervalSec = 60;

    private boolean mNullDeviceAllocated = false;
    private boolean mEmulatorAllocated = false;

    /**
     * Container for utilization stats.
     */
    public static class UtilizationDesc {
        final int mTotalUtil;
        final Map<String, Integer> mDeviceUtil;

        public UtilizationDesc(int totalUtil, Map<String, Integer> deviceUtil) {
            mTotalUtil = totalUtil;
            mDeviceUtil = deviceUtil;
        }

        /**
         * Return the total utilization for all devices in TF process, measured as total allocation
         * time for all devices vs total available time.
         *
         * @return percentage utilization
         */
        public int getTotalUtil() {
            return mTotalUtil;
        }

        /**
         * Helper method to return percent utilization for a device. Returns 0 if no utilization
         * data exists for device
         */
        public Integer getUtilForDevice(String serial) {
            Integer util = mDeviceUtil.get(serial);
            if (util == null) {
                return 0;
            }
            return util;
        }
    }

    private class DeviceUtilRecord {
        // store samples of device util, where 0 = avail, 1 = allocated
        // TODO: save memory by using CircularBitArray
        private CircularByteArray mData;
        private int mConsecutiveMissedSamples = 0;

        DeviceUtilRecord() {
            mData = new CircularByteArray(mMaxSamples);
        }

        public void addSample(DeviceAllocationState state) {
            if (DeviceAllocationState.Allocated.equals(state)) {
                mData.add((byte)1);
            } else {
                mData.add((byte)0);
            }
            mConsecutiveMissedSamples = 0;
        }

        public long getNumAllocations() {
            return mData.getSum();
        }

        public long getTotalSamples() {
            return mData.size();
        }

        /**
         * Record sample for missing device.
         *
         * @param serial device serial number
         * @return true if sample was added, false if device has been missing for more than max
         * samples
         */
        public boolean addMissingSample(String serial) {
            mConsecutiveMissedSamples++;
            if (mConsecutiveMissedSamples > mMaxSamples) {
                return false;
            }
            mData.add((byte)0);
            return true;
        }
    }

    private class SamplingTask extends TimerTask {
        @Override
        public void run() {
            CLog.d("Collecting utilization");
            // track devices that we have records for, but are not reported by device lister
            Map<String, DeviceUtilRecord> goneDevices = new HashMap<>(mDeviceUtilMap);

            for (DeviceDescriptor deviceDesc : mDeviceLister.listDevices()) {
                DeviceUtilRecord record = getDeviceRecord(deviceDesc.getSerial());
                record.addSample(deviceDesc.getState());
                goneDevices.remove(deviceDesc.getSerial());
            }

            // now record samples for gone devices
            for (Map.Entry<String, DeviceUtilRecord> goneSerialEntry : goneDevices.entrySet()) {
                String serial = goneSerialEntry.getKey();
                if (!goneSerialEntry.getValue().addMissingSample(serial)) {
                    CLog.d("Forgetting device %s", serial);
                    mDeviceUtilMap.remove(serial);
                }
            }
        }
    }

    private int mMaxSamples;

    /** a map of device serial to device records */
    private Map<String, DeviceUtilRecord> mDeviceUtilMap = new Hashtable<>();

    private DeviceLister mDeviceLister;

    private Timer mTimer;
    private SamplingTask mSamplingTask = new SamplingTask();

    /**
     * Get the device utilization up to the last 24 hours
     */
    public synchronized UtilizationDesc getUtilizationStats() {
        CLog.d("Calculating device util");

        long totalAllocSamples = 0;
        long totalSamples = 0;
        Map<String, Integer> deviceUtilMap = new HashMap<>();
        for (Map.Entry<String, DeviceUtilRecord> deviceRecordEntry : mDeviceUtilMap.entrySet()) {
            if (shouldTrackDevice(deviceRecordEntry.getKey())) {
                long allocSamples = deviceRecordEntry.getValue().getNumAllocations();
                long numSamples = deviceRecordEntry.getValue().getTotalSamples();
                totalAllocSamples += allocSamples;
                totalSamples += numSamples;
                deviceUtilMap.put(deviceRecordEntry.getKey(), getUtil(allocSamples, numSamples));
            }
        }
        return new UtilizationDesc(getUtil(totalAllocSamples, totalSamples), deviceUtilMap);
    }

    /**
     * Get device utilization as a percent
     */
    private static int getUtil(long allocSamples, long numSamples) {
        if (numSamples <= 0) {
            return 0;
        }
        return (int)((allocSamples * 100) / numSamples);
    }

    @Override
    public void run() {
        calculateMaxSamples();
        mTimer  = new Timer();
        mTimer.scheduleAtFixedRate(mSamplingTask, INITIAL_DELAY_MS, mSamplingIntervalSec * 1000);
    }

    @Override
    public void stop() {
        if (mTimer != null) {
            mTimer.cancel();
            mTimer.purge();
        }
    }

    @Override
    public void setDeviceLister(DeviceLister lister) {
        mDeviceLister = lister;
    }

    /**
     * Listens to device state changes and records time that device transitions from or to
     * available or allocated state.
     */
    @Override
    public synchronized void notifyDeviceStateChange(String serial, DeviceAllocationState oldState,
            DeviceAllocationState newState) {
        if (mNullDeviceAllocated && mEmulatorAllocated) {
            // optimization, don't enter calculation below unless needed
            return;
        }
        if (DeviceAllocationState.Allocated.equals(newState)) {
            IDeviceManager dvcMgr = getDeviceManager();
            if (dvcMgr.isNullDevice(serial)) {
                mNullDeviceAllocated = true;
            } else if (dvcMgr.isEmulator(serial)) {
                mEmulatorAllocated = true;
            }
        }
    }

    /**
     * Get the device util records for given serial, creating if necessary.
     */
    private DeviceUtilRecord getDeviceRecord(String serial) {
        DeviceUtilRecord r = mDeviceUtilMap.get(serial);
        if (r == null) {
            r = new DeviceUtilRecord();
            mDeviceUtilMap.put(serial, r);
        }
        return r;
    }

    private boolean shouldTrackDevice(String serial) {
        IDeviceManager dvcMgr = getDeviceManager();
        if (dvcMgr.isNullDevice(serial)) {
            switch (mCollectNullDevice) {
                case ALWAYS_INCLUDE:
                    return true;
                case IGNORE:
                    return false;
                case INCLUDE_IF_USED:
                    return mNullDeviceAllocated;
            }
        } else if (dvcMgr.isEmulator(serial)) {
            switch (mCollectEmulator) {
                case ALWAYS_INCLUDE:
                    return true;
                case IGNORE:
                    return false;
                case INCLUDE_IF_USED:
                    return mEmulatorAllocated;
            }
        }
        return true;
    }

    IDeviceManager getDeviceManager() {
        return GlobalConfiguration.getDeviceManagerInstance();
    }

    TimerTask getSamplingTask() {
        return mSamplingTask;
    }

    // @VisibleForTesting
    void calculateMaxSamples() {
        // find max samples to collect by converting sample window to seconds then divide by
        // sampling interval
        mMaxSamples = mSampleWindowHours * 60 * 60 / mSamplingIntervalSec;
        assert(mMaxSamples > 0);
    }

    // @VisibleForTesting
    void setMaxSamples(int maxSamples) {
        mMaxSamples = maxSamples;
    }

    // @VisibleForTesting
    int getMaxSamples() {
        return mMaxSamples;
    }
}
