/*
 * Copyright (C) 2011 The Android Open Source Project
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
package com.android.framework.tests;

import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.StreamUtil;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.util.HashMap;
import java.util.Map;

public class BandwidthUtils {

    private BandwidthStats mDevStats;
    private Map<Integer, BandwidthStats> mUidStats = new HashMap<Integer, BandwidthStats>();
    private String mIface;
    ITestDevice mTestDevice = null;

    BandwidthUtils(ITestDevice device, String iface) throws DeviceNotAvailableException {
        mTestDevice = device;
        mIface = iface;
        parseUidStats();
        parseNetDevStats();
    }

    public BandwidthStats getStatsForUid(int uid) {
        return mUidStats.get(uid);
    }

    public BandwidthStats getDevStats() {
        return mDevStats;
    }

    public BandwidthStats getSumOfUidStats() {
        long rb = 0, rp = 0, tb = 0, tp = 0;
        for (BandwidthStats stats : mUidStats.values()) {
            rb += stats.getRxBytes();
            rp += stats.getRxPackets();
            tb += stats.getTxBytes();
            tp += stats.getTxPackets();
        }
        return new BandwidthStats(rb, rp, tb, tp);
    }

    /**
     * Parses the uid stats from /proc/net/xt_qtaguid/stats
     * @throws DeviceNotAvailableException
     */
    private void parseUidStats() throws DeviceNotAvailableException {
        File statsFile = mTestDevice.pullFile("/proc/net/xt_qtaguid/stats");
        FileInputStream fStream = null;
        try {
            fStream = new FileInputStream(statsFile);
            String tmp = StreamUtil.getStringFromStream(fStream);
            String[] lines = tmp.split("\n");
            for (String line : lines) {
                if (line.contains("idx")) {
                    continue;
                }
                String[] parts = line.trim().split(" ");
                String iface = parts[1];
                if (!mIface.equals(iface)) {  // skip if its not iface we need
                    continue;
                }
                String foreground = parts[4];
                if (!"0".equals(foreground)) { // test uses background data, skip foreground
                    continue;
                }
                String tag = parts[2];
                int uid = Integer.parseInt(parts[3]);
                int rb = Integer.parseInt(parts[5]);
                int rp = Integer.parseInt(parts[6]);
                int tb = Integer.parseInt(parts[7]);
                int tp = Integer.parseInt(parts[8]);
                if ("0x0".equals(tag)) {
                    CLog.v("qtag iface %s pid %d rb %d rp %d tb %d tp %d",
                            iface, uid, rb, rp, tb, tp);
                    mUidStats.put(uid, new BandwidthStats(rb, rp, tb, tp));
                }
            }
        } catch (IOException e) {
            CLog.d("Failed to read file %s: %s", statsFile.toString(), e.getMessage());
        } finally {
            StreamUtil.close(fStream);
        }
    }

    /**
     * Add stats, if the iface is currently active or it is an unknown iface found in /proc/net/dev.
     * @throws DeviceNotAvailableException
     */
    private void parseNetDevStats() throws DeviceNotAvailableException {
        File file = mTestDevice.pullFile("/proc/net/dev");
        FileInputStream fStream = null;
        try {
            fStream = new FileInputStream(file);
            String tmp = StreamUtil.getStringFromStream(fStream);
            String[] lines = tmp.split("\n");
            for (String line : lines) {
                if (line.contains("Receive") || line.contains("multicast")) {
                    continue;
                }
                String[] parts = line.trim().split("[ :]+");
                String iface = parts[0].replace(":", "").trim();
                if (mIface.equals(iface)) {
                    int rb = Integer.parseInt(parts[1]);
                    int rp = Integer.parseInt(parts[2]);
                    int tb = Integer.parseInt(parts[9]);
                    int tp = Integer.parseInt(parts[10]);
                    CLog.v("net iface %s rb %d rp %d tb %d tp %d", iface, rb, rp, tb, tp);
                    mDevStats = new BandwidthStats(rb, rp, tb, tp);
                    break;
                }
            }
        } catch (IOException e) {
            CLog.d("Failed to read file %s: %s", file.toString(), e.getMessage());
        } finally {
            StreamUtil.close(fStream);
        }
    }
}
