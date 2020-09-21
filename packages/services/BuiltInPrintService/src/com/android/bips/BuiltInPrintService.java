/*
 * Copyright (C) 2016 The Android Open Source Project
 * Copyright (C) 2016 Mopria Alliance, Inc.
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

package com.android.bips;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.net.nsd.NsdManager;
import android.net.wifi.WifiManager;
import android.os.Handler;
import android.printservice.PrintJob;
import android.printservice.PrintService;
import android.printservice.PrinterDiscoverySession;
import android.text.TextUtils;
import android.util.Log;

import com.android.bips.discovery.DelayedDiscovery;
import com.android.bips.discovery.DiscoveredPrinter;
import com.android.bips.discovery.Discovery;
import com.android.bips.discovery.ManualDiscovery;
import com.android.bips.discovery.MdnsDiscovery;
import com.android.bips.discovery.MultiDiscovery;
import com.android.bips.discovery.NsdResolveQueue;
import com.android.bips.discovery.P2pDiscovery;
import com.android.bips.ipp.Backend;
import com.android.bips.ipp.CapabilitiesCache;
import com.android.bips.p2p.P2pMonitor;
import com.android.bips.p2p.P2pUtils;
import com.android.bips.util.BroadcastMonitor;

import java.lang.ref.WeakReference;

public class BuiltInPrintService extends PrintService {
    private static final String TAG = BuiltInPrintService.class.getSimpleName();
    private static final boolean DEBUG = false;
    private static final int IPPS_PRINTER_DELAY = 150;
    private static final int P2P_DISCOVERY_DELAY = 1000;

    // Present because local activities can bind, but cannot access this object directly
    private static WeakReference<BuiltInPrintService> sInstance;

    private MultiDiscovery mAllDiscovery;
    private P2pDiscovery mP2pDiscovery;
    private Discovery mMdnsDiscovery;
    private ManualDiscovery mManualDiscovery;
    private CapabilitiesCache mCapabilitiesCache;
    private JobQueue mJobQueue;
    private Handler mMainHandler;
    private Backend mBackend;
    private WifiManager.WifiLock mWifiLock;
    private P2pMonitor mP2pMonitor;
    private NsdResolveQueue mNsdResolveQueue;

    /**
     * Return the current print service instance, if running
     */
    public static BuiltInPrintService getInstance() {
        return sInstance == null ? null : sInstance.get();
    }

    @Override
    public void onCreate() {
        if (DEBUG) {
            try {
                PackageInfo pInfo = getPackageManager().getPackageInfo(getPackageName(), 0);
                String version = pInfo.versionName;
                Log.d(TAG, "onCreate() " + version);
            } catch (PackageManager.NameNotFoundException ignored) {
            }
        }
        super.onCreate();

        sInstance = new WeakReference<>(this);
        mBackend = new Backend(this);
        mCapabilitiesCache = new CapabilitiesCache(this, mBackend,
                CapabilitiesCache.DEFAULT_MAX_CONCURRENT);
        mP2pMonitor = new P2pMonitor(this);

        NsdManager nsdManager = (NsdManager) getSystemService(Context.NSD_SERVICE);
        mNsdResolveQueue = new NsdResolveQueue(this, nsdManager);

        // Delay IPP results so that IPP is preferred
        Discovery ippDiscovery = new MdnsDiscovery(this, MdnsDiscovery.SCHEME_IPP);
        Discovery ippsDiscovery = new MdnsDiscovery(this, MdnsDiscovery.SCHEME_IPPS);
        mMdnsDiscovery = new MultiDiscovery(ippDiscovery, new DelayedDiscovery(ippsDiscovery, 0,
                IPPS_PRINTER_DELAY));
        mP2pDiscovery = new P2pDiscovery(this);
        mManualDiscovery = new ManualDiscovery(this);

        // Delay P2P discovery so that all others are found first
        mAllDiscovery = new MultiDiscovery(mMdnsDiscovery, mManualDiscovery, new DelayedDiscovery(
                mP2pDiscovery, P2P_DISCOVERY_DELAY, 0));

        mJobQueue = new JobQueue();
        mMainHandler = new Handler(getMainLooper());
        WifiManager wifiManager = (WifiManager) getSystemService(Context.WIFI_SERVICE);
        mWifiLock = wifiManager.createWifiLock(WifiManager.WIFI_MODE_FULL, TAG);
    }

    @Override
    public void onDestroy() {
        if (DEBUG) Log.d(TAG, "onDestroy()");
        mCapabilitiesCache.close();
        mP2pMonitor.stopAll();
        mBackend.close();
        unlockWifi();
        sInstance = null;
        mMainHandler.removeCallbacksAndMessages(null);
        super.onDestroy();
    }

    @Override
    protected PrinterDiscoverySession onCreatePrinterDiscoverySession() {
        if (DEBUG) Log.d(TAG, "onCreatePrinterDiscoverySession");
        return new LocalDiscoverySession(this);
    }

    @Override
    protected void onPrintJobQueued(PrintJob printJob) {
        if (DEBUG) Log.d(TAG, "onPrintJobQueued");
        mJobQueue.print(new LocalPrintJob(this, mBackend, printJob));
    }

    @Override
    protected void onRequestCancelPrintJob(PrintJob printJob) {
        if (DEBUG) Log.d(TAG, "onRequestCancelPrintJob");
        mJobQueue.cancel(printJob.getId());
    }

    /**
     * Return the global discovery object
     */
    public Discovery getDiscovery() {
        return mAllDiscovery;
    }

    /**
     * Return the global object for MDNS discoveries
     */
    public Discovery getMdnsDiscovery() {
        return mMdnsDiscovery;
    }

    /**
     * Return the global object for manual discoveries
     */
    public ManualDiscovery getManualDiscovery() {
        return mManualDiscovery;
    }

    /**
     * Return the global object for Wi-Fi Direct printer discoveries
     */
    public P2pDiscovery getP2pDiscovery() {
        return mP2pDiscovery;
    }

    /**
     * Return the global object for general Wi-Fi Direct management
     */
    public P2pMonitor getP2pMonitor() {
        return mP2pMonitor;
    }

    /**
     * Return the global {@link NsdResolveQueue}
     */
    public NsdResolveQueue getNsdResolveQueue() {
        return mNsdResolveQueue;
    }

    /**
     * Listen for a set of broadcast messages until stopped
     */
    public BroadcastMonitor receiveBroadcasts(BroadcastReceiver receiver, String... actions) {
        return new BroadcastMonitor(this, receiver, actions);
    }

    /**
     * Return the global Printer Capabilities cache
     */
    public CapabilitiesCache getCapabilitiesCache() {
        return mCapabilitiesCache;
    }

    /**
     * Return the main handler for posting {@link Runnable} objects to the main UI
     */
    public Handler getMainHandler() {
        return mMainHandler;
    }

    /** Run something on the main thread, returning an object that can cancel the request */
    public DelayedAction delay(int delay, Runnable toRun) {
        mMainHandler.postDelayed(toRun, delay);
        return () -> mMainHandler.removeCallbacks(toRun);
    }

    /**
     * Return a friendly description string including host and (if present) location
     */
    public String getDescription(DiscoveredPrinter printer) {
        if (P2pUtils.isP2p(printer) || P2pUtils.isOnConnectedInterface(this, printer)) {
            return getString(R.string.wifi_direct);
        }

        String host = printer.getHost();
        if (!TextUtils.isEmpty(printer.location)) {
            return getString(R.string.printer_description, host, printer.location);
        } else {
            return host;
        }
    }

    /** Prevent Wi-Fi from going to sleep until {@link #unlockWifi} is called */
    public void lockWifi() {
        if (!mWifiLock.isHeld()) {
            mWifiLock.acquire();
        }
    }

    /** Allow Wi-Fi to be disabled during sleep modes. */
    public void unlockWifi() {
        if (mWifiLock.isHeld()) {
            mWifiLock.release();
        }
    }
}
