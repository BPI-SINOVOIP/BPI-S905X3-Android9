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

package com.android.server.cts.device.statsd;

import android.accounts.Account;
import android.accounts.AccountManager;
import android.app.AlarmManager;
import android.app.PendingIntent;
import android.app.Activity;
import android.app.job.JobInfo;
import android.app.job.JobScheduler;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanFilter;
import android.bluetooth.le.ScanResult;
import android.bluetooth.le.ScanSettings;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraManager;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.media.MediaPlayer;
import android.net.wifi.WifiManager;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.PowerManager;
import android.os.SystemClock;
import android.support.test.InstrumentationRegistry;
import android.util.Log;
import android.util.StatsLog;

import static com.android.compatibility.common.util.SystemUtil.runShellCommand;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import org.junit.Test;

import java.util.Arrays;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

public class AtomTests {
    private static final String TAG = AtomTests.class.getSimpleName();

    @Test
    public void testAudioState() {
        // TODO: This should surely be getTargetContext(), here and everywhere, but test first.
        Context context = InstrumentationRegistry.getContext();
        MediaPlayer mediaPlayer = MediaPlayer.create(context, R.raw.good);
        mediaPlayer.start();
        sleep(2_000);
        mediaPlayer.stop();
    }

    @Test
    public void testBleScanOpportunistic() {
        ScanSettings scanSettings = new ScanSettings.Builder()
                .setScanMode(ScanSettings.SCAN_MODE_OPPORTUNISTIC).build();
        performBleScan(scanSettings, null,false);
    }

    @Test
    public void testBleScanUnoptimized() {
        ScanSettings scanSettings = new ScanSettings.Builder()
                .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY).build();
        performBleScan(scanSettings, null, false);
    }

    @Test
    public void testBleScanResult() {
        ScanSettings scanSettings = new ScanSettings.Builder()
                .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY).build();
        ScanFilter.Builder scanFilter = new ScanFilter.Builder();
        performBleScan(scanSettings, Arrays.asList(scanFilter.build()), true);
    }

    private static void performBleScan(ScanSettings scanSettings, List<ScanFilter> scanFilters, boolean waitForResult) {
        BluetoothAdapter bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        if (bluetoothAdapter == null) {
            Log.e(TAG, "Device does not support Bluetooth");
            return;
        }
        boolean bluetoothEnabledByTest = false;
        if (!bluetoothAdapter.isEnabled()) {
            if (!bluetoothAdapter.enable()) {
                Log.e(TAG, "Bluetooth is not enabled");
                return;
            }
            sleep(8_000);
            bluetoothEnabledByTest = true;
        }
        BluetoothLeScanner bleScanner = bluetoothAdapter.getBluetoothLeScanner();
        if (bleScanner == null) {
            Log.e(TAG, "Cannot access BLE scanner");
            return;
        }

        CountDownLatch resultsLatch = new CountDownLatch(1);
        ScanCallback scanCallback = new ScanCallback() {
            @Override
            public void onScanResult(int callbackType, ScanResult result) {
                Log.v(TAG, "called onScanResult");
                resultsLatch.countDown();
            }
            @Override
            public void onScanFailed(int errorCode) {
                Log.v(TAG, "called onScanFailed");
            }
            @Override
            public void onBatchScanResults(List<ScanResult> results) {
                Log.v(TAG, "called onBatchScanResults");
                resultsLatch.countDown();
            }
        };

        bleScanner.startScan(scanFilters, scanSettings, scanCallback);
        if (waitForResult) {
            waitForReceiver(InstrumentationRegistry.getContext(), 59_000, resultsLatch, null);
        } else {
            sleep(2_000);
        }
        bleScanner.stopScan(scanCallback);

        // Restore adapter state at end of test
        if (bluetoothEnabledByTest) {
            bluetoothAdapter.disable();
        }
    }

    @Test
    public void testCameraState() throws Exception {
        Context context = InstrumentationRegistry.getContext();
        CameraManager cam = context.getSystemService(CameraManager.class);
        String[] cameraIds = cam.getCameraIdList();
        if (cameraIds.length == 0) {
            Log.e(TAG, "No camera found on device");
            return;
        }

        CountDownLatch latch = new CountDownLatch(1);
        final CameraDevice.StateCallback cb = new CameraDevice.StateCallback() {
            @Override
            public void onOpened(CameraDevice cd) {
                Log.i(TAG, "CameraDevice " + cd.getId() + " opened");
                sleep(2_000);
                cd.close();
            }
            @Override
            public void onClosed(CameraDevice cd) {
                latch.countDown();
                Log.i(TAG, "CameraDevice " + cd.getId() + " closed");
            }
            @Override
            public void onDisconnected(CameraDevice cd) {
                Log.w(TAG, "CameraDevice  " + cd.getId() + " disconnected");
            }
            @Override
            public void onError(CameraDevice cd, int error) {
                Log.e(TAG, "CameraDevice " + cd.getId() + "had error " + error);
            }
        };

        HandlerThread handlerThread = new HandlerThread("br_handler_thread");
        handlerThread.start();
        Looper looper = handlerThread.getLooper();
        Handler handler = new Handler(looper);

        cam.openCamera(cameraIds[0], cb, handler);
        waitForReceiver(context, 10_000, latch, null);
    }

    @Test
    public void testFlashlight() throws Exception {
        Context context = InstrumentationRegistry.getContext();
        CameraManager cam = context.getSystemService(CameraManager.class);
        String[] cameraIds = cam.getCameraIdList();
        boolean foundFlash = false;
        for (int i = 0; i < cameraIds.length; i++) {
            String id = cameraIds[i];
            if(cam.getCameraCharacteristics(id).get(CameraCharacteristics.FLASH_INFO_AVAILABLE)) {
                cam.setTorchMode(id, true);
                sleep(500);
                cam.setTorchMode(id, false);
                foundFlash = true;
                break;
            }
        }
        if(!foundFlash) {
            Log.e(TAG, "No flashlight found on device");
        }
    }

    @Test
    public void testForegroundService() throws Exception {
        Context context = InstrumentationRegistry.getContext();
        // The service goes into foreground and exits shortly
        Intent intent = new Intent(context, StatsdCtsForegroundService.class);
        context.startService(intent);
        sleep(500);
        context.stopService(intent);
    }

    @Test
    public void testGpsScan() {
        Context context = InstrumentationRegistry.getContext();
        final LocationManager locManager = context.getSystemService(LocationManager.class);
        if (!locManager.isProviderEnabled(LocationManager.GPS_PROVIDER)) {
            Log.e(TAG, "GPS provider is not enabled");
            return;
        }
        CountDownLatch latch = new CountDownLatch(1);

        final LocationListener locListener = new LocationListener() {
            public void onLocationChanged(Location location) {
                Log.v(TAG, "onLocationChanged: location has been obtained");
            }
            public void onProviderDisabled(String provider) {
                Log.w(TAG, "onProviderDisabled " + provider);
            }
            public void onProviderEnabled(String provider) {
                Log.w(TAG, "onProviderEnabled " + provider);
            }
            public void onStatusChanged(String provider, int status, Bundle extras) {
                Log.w(TAG, "onStatusChanged " + provider + " " + status);
            }
        };

        new AsyncTask<Void, Void, Void>() {
            @Override
            protected Void doInBackground(Void... params) {
                Looper.prepare();
                locManager.requestLocationUpdates(LocationManager.GPS_PROVIDER, 990, 0,
                        locListener);
                sleep(1_000);
                locManager.removeUpdates(locListener);
                latch.countDown();
                return null;
            }
        }.execute();

        waitForReceiver(context, 59_000, latch, null);
    }

    @Test
    public void testScreenBrightness() {
        Context context = InstrumentationRegistry.getContext();
        PowerManager pm = context.getSystemService(PowerManager.class);
        PowerManager.WakeLock wl = pm.newWakeLock(PowerManager.SCREEN_BRIGHT_WAKE_LOCK |
                PowerManager.ACQUIRE_CAUSES_WAKEUP, "StatsdBrightnessTest");
        wl.acquire();
        sleep(500);

        setScreenBrightness(47);
        sleep(500);
        setScreenBrightness(100);
        sleep(500);
        setScreenBrightness(198);
        sleep(500);


        wl.release();
    }

    @Test
    public void testSyncState() throws Exception {

        Context context = InstrumentationRegistry.getContext();
        StatsdAuthenticator.removeAllAccounts(context);
        AccountManager am = context.getSystemService(AccountManager.class);
        CountDownLatch latch = StatsdSyncAdapter.resetCountDownLatch();

        Account account = StatsdAuthenticator.getTestAccount();
        StatsdAuthenticator.ensureTestAccount(context);
        sleep(500);

        // Just force set is syncable.
        ContentResolver.setMasterSyncAutomatically(true);
        sleep(500);
        ContentResolver.setIsSyncable(account, StatsdProvider.AUTHORITY, 1);
        // Wait for the first (automatic) sync to finish
        waitForReceiver(context, 120_000, latch, null);

        //Sleep for 500ms, since we assert each start/stop to be ~500ms apart.
        sleep(500);

        // Request and wait for the second sync to finish
        latch = StatsdSyncAdapter.resetCountDownLatch();
        StatsdSyncAdapter.requestSync(account);
        waitForReceiver(context, 120_000, latch, null);
        StatsdAuthenticator.removeAllAccounts(context);
    }

    @Test
    public void testScheduledJob() throws Exception {
        final ComponentName name = new ComponentName("com.android.server.cts.device.statsd",
                StatsdJobService.class.getName());

        Context context = InstrumentationRegistry.getContext();
        JobScheduler js = context.getSystemService(JobScheduler.class);
        assertTrue("JobScheduler service not available", js != null);

        JobInfo.Builder builder = new JobInfo.Builder(1, name);
        builder.setOverrideDeadline(0);
        JobInfo job = builder.build();

        long startTime = System.currentTimeMillis();
        CountDownLatch latch = StatsdJobService.resetCountDownLatch();
        js.schedule(job);
        waitForReceiver(context, 2_500, latch, null);
    }

    @Test
    public void testWakelockState() {
        Context context = InstrumentationRegistry.getContext();
        PowerManager pm = context.getSystemService(PowerManager.class);
        PowerManager.WakeLock wl = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK,
                "StatsdPartialWakelock");
        wl.acquire();
        sleep(500);
        wl.release();
    }

    @Test
    public void testWakelockLoad() {
        final int NUM_THREADS = 16;
        CountDownLatch latch = new CountDownLatch(NUM_THREADS);
        for (int i = 0; i < NUM_THREADS; i++) {
            Thread t = new Thread(new WakelockLoadTestRunnable("StatsdPartialWakelock" + i, latch));
            t.start();
        }
        waitForReceiver(null, 120_000, latch, null);
    }

    @Test
    public void testWakeupAlarm() {
        Context context = InstrumentationRegistry.getContext();
        String name = "android.cts.statsd.testWakeupAlarm";
        CountDownLatch onReceiveLatch = new CountDownLatch(1);
        BroadcastReceiver receiver =
                registerReceiver(context, onReceiveLatch, new IntentFilter(name));
        AlarmManager manager = (AlarmManager) (context.getSystemService(AlarmManager.class));
        PendingIntent pintent = PendingIntent.getBroadcast(context, 0, new Intent(name), 0);
        manager.setExact(AlarmManager.ELAPSED_REALTIME_WAKEUP,
            SystemClock.elapsedRealtime() + 2_000, pintent);
        waitForReceiver(context, 10_000, onReceiveLatch, receiver);
    }

    @Test
    public void testWifiLock() {
        Context context = InstrumentationRegistry.getContext();
        WifiManager wm = context.getSystemService(WifiManager.class);
        WifiManager.WifiLock lock = wm.createWifiLock("StatsdCTSWifiLock");
        lock.acquire();
        sleep(500);
        lock.release();
    }

    @Test
    public void testWifiMulticastLock() {
        Context context = InstrumentationRegistry.getContext();
        WifiManager wm = context.getSystemService(WifiManager.class);
        WifiManager.MulticastLock lock = wm.createMulticastLock("StatsdCTSMulticastLock");
        lock.acquire();
        sleep(500);
        lock.release();
    }

    @Test
    /** Does two wifi scans. */
    // TODO: Copied this from BatterystatsValidation but we probably don't need to wait for results.
    public void testWifiScan() {
        Context context = InstrumentationRegistry.getContext();
        IntentFilter intentFilter = new IntentFilter(WifiManager.SCAN_RESULTS_AVAILABLE_ACTION);
        // Sometimes a scan was already running (from a different uid), so the first scan doesn't
        // start when requested. Therefore, additionally wait for whatever scan is currently running
        // to finish, then request a scan again - at least one of these two scans should be
        // attributed to this app.
        for (int i = 0; i < 2; i++) {
            CountDownLatch onReceiveLatch = new CountDownLatch(1);
            BroadcastReceiver receiver = registerReceiver(context, onReceiveLatch, intentFilter);
            context.getSystemService(WifiManager.class).startScan();
            waitForReceiver(context, 60_000, onReceiveLatch, receiver);
        }
    }

    @Test
    public void testSimpleCpu() {
        long timestamp = System.currentTimeMillis();
        for (int i = 0; i < 10000; i ++) {
            timestamp += i;
        }
        Log.i(TAG, "The answer is " + timestamp);
    }

    // ------- Helper methods

    /** Puts the current thread to sleep. */
    static void sleep(int millis) {
        try {
            Thread.sleep(millis);
        } catch (InterruptedException e) {
            Log.e(TAG, "Interrupted exception while sleeping", e);
        }
    }

    /** Register receiver to determine when given action is complete. */
    private static BroadcastReceiver registerReceiver(
            Context ctx, CountDownLatch onReceiveLatch, IntentFilter intentFilter) {
        BroadcastReceiver receiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                Log.d(TAG, "Received broadcast.");
                onReceiveLatch.countDown();
            }
        };
        // Run Broadcast receiver in a different thread since the main thread will wait.
        HandlerThread handlerThread = new HandlerThread("br_handler_thread");
        handlerThread.start();
        Looper looper = handlerThread.getLooper();
        Handler handler = new Handler(looper);
        ctx.registerReceiver(receiver, intentFilter, null, handler);
        return receiver;
    }

    /**
     * Uses the receiver to wait until the action is complete. ctx and receiver may be null if no
     * receiver is needed to be unregistered.
     */
    private static void waitForReceiver(Context ctx,
            int maxWaitTimeMs, CountDownLatch latch, BroadcastReceiver receiver) {
        try {
            boolean didFinish = latch.await(maxWaitTimeMs, TimeUnit.MILLISECONDS);
            if (didFinish) {
                Log.v(TAG, "Finished performing action");
            } else {
                // This is not necessarily a problem. If we just want to make sure a count was
                // recorded for the request, it doesn't matter if the action actually finished.
                Log.w(TAG, "Did not finish in specified time.");
            }
        } catch (InterruptedException e) {
            Log.e(TAG, "Interrupted exception while awaiting action to finish", e);
        }
        if (ctx != null && receiver != null) {
            ctx.unregisterReceiver(receiver);
        }
    }

    private static void setScreenBrightness(int brightness) {
        runShellCommand("settings put system screen_brightness " + brightness);
    }
}
