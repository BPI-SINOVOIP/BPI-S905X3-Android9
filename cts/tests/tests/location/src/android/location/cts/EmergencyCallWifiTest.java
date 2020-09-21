/*
 * Copyright (C) 2017 Google Inc.
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

package android.location.cts;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.wifi.ScanResult;
import android.net.wifi.WifiManager;
import android.platform.test.annotations.AppModeFull;
import android.util.Log;

import java.io.IOException;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

/**
 * This class is used to test Wifi realted features.
 */
public class EmergencyCallWifiTest extends GnssTestCase {

    private final static String TAG = EmergencyCallWifiTest.class.getCanonicalName();
    private final static String LATCH_NAME = "EmergencyCallWifiTest";
    private final static String GOOGLE_URL = "www.google.com";
    private final static int WEB_PORT = 80;
    private static final int BATCH_SCAN_BSSID_LIMIT = 25;
    private static final int WIFI_SCAN_TIMEOUT_SEC = 30;
    private static final long SETTINGS_PERIOD_MS = TimeUnit.SECONDS.toMillis(4);
    private static final long INTERNET_CONNECTION_TIMEOUT = TimeUnit.SECONDS.toMillis(2);
    private static final int MIN_SCAN_COUNT = 1;

    private WifiManager mWifiManager;
    private Context mContext;
    private WifiScanReceiver mWifiScanReceiver;
    private int mScanCounter = 0;
    private CountDownLatch mLatch;

    @Override
    public void setUp() throws Exception {
        super.setUp();
        mContext = getContext();
        mWifiManager = (WifiManager) mContext.getSystemService(mContext.WIFI_SERVICE);
        mWifiScanReceiver = new WifiScanReceiver();
    }

    @AppModeFull(reason = "Requires registering a broadcast receiver")
    public void testWifiScan() throws Exception {
        mContext.registerReceiver(mWifiScanReceiver, new IntentFilter(
                WifiManager.SCAN_RESULTS_AVAILABLE_ACTION));
        mLatch = new CountDownLatch(1);
        mWifiManager.startScan();
        Log.d(TAG, "Waiting for wifiScan to complete.");
        mLatch.await(WIFI_SCAN_TIMEOUT_SEC, TimeUnit.SECONDS);
        if (mScanCounter < MIN_SCAN_COUNT) {
            fail(String.format("Expected at least %d scans but only %d scans happened",
            MIN_SCAN_COUNT, mScanCounter));
        }
    }

    public void testWifiConnection() {
        boolean isReachable =
            isReachable(GOOGLE_URL, WEB_PORT, (int)INTERNET_CONNECTION_TIMEOUT);
        assertTrue("Can not connect to google.com."
         + " Please make sure device has the internet connection", isReachable);
    }

    private static boolean isReachable(String addr, int openPort, int timeOutMillis) {
        try {
            try (Socket soc = new Socket()) {
                soc.connect(new InetSocketAddress(addr, openPort), timeOutMillis);
            }
            return true;
        } catch (IOException ex) {
            return false;
        }
    }

    private class WifiScanReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context c, Intent intent) {
            List <ScanResult> scanResults = mWifiManager.getScanResults();
            Log.d(TAG, String.format("Got scan results with size %d", scanResults.size()));
            for (ScanResult result : scanResults) {
                Log.d(TAG, result.toString());
            }
            mScanCounter++;
            mLatch.countDown();
        }
    }
}