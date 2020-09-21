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

package com.android.bips.discovery;

import android.net.Uri;
import android.net.nsd.NsdManager;
import android.net.nsd.NsdServiceInfo;
import android.net.wifi.WifiManager;
import android.text.TextUtils;
import android.util.Log;

import com.android.bips.BuiltInPrintService;

import java.net.Inet4Address;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.Map;

/**
 * Search the local network for devices advertising IPP print services
 */
public class MdnsDiscovery extends Discovery {
    public static final String SCHEME_IPP = "ipp";
    public static final String SCHEME_IPPS = "ipps";

    private static final String TAG = MdnsDiscovery.class.getSimpleName();
    private static final boolean DEBUG = false;

    // Prepend this to a UUID to create a proper URN
    private static final String PREFIX_URN_UUID = "urn:uuid:";

    // Keys for expected txtRecord attributes
    private static final String ATTRIBUTE_RP = "rp";
    private static final String ATTRIBUTE_UUID = "UUID";
    private static final String ATTRIBUTE_NOTE = "note";
    private static final String ATTRIBUTE_PRINT_WFDS = "print_wfds";
    private static final String VALUE_PRINT_WFDS_OPT_OUT = "F";

    // Service names of interest
    private static final String SERVICE_IPP = "_ipp._tcp";
    private static final String SERVICE_IPPS = "_ipps._tcp";

    private final String mServiceName;
    private final List<NsdServiceListener> mServiceListeners = new ArrayList<>();
    private final List<Resolver> mResolvers = new ArrayList<>();
    private final NsdResolveQueue mNsdResolveQueue;

    /** Lock to keep multi-cast enabled */
    private WifiManager.MulticastLock mMulticastLock;

    public MdnsDiscovery(BuiltInPrintService printService, String scheme) {
        super(printService);

        switch (scheme) {
            case SCHEME_IPP:
                mServiceName = SERVICE_IPP;
                break;
            case SCHEME_IPPS:
                mServiceName = SERVICE_IPPS;
                break;
            default:
                throw new IllegalArgumentException("unrecognized scheme " + scheme);
        }
        mNsdResolveQueue = printService.getNsdResolveQueue();
    }

    /** Return a valid {@link DiscoveredPrinter} from {@link NsdServiceInfo}, or null if invalid */
    private static DiscoveredPrinter toNetworkPrinter(NsdServiceInfo info) {
        // Honor printers that deliberately opt-out
        if (VALUE_PRINT_WFDS_OPT_OUT.equals(getStringAttribute(info, ATTRIBUTE_PRINT_WFDS))) {
            if (DEBUG) Log.d(TAG, "Opted out: " + info);
            return null;
        }

        // Collect resource path
        String resourcePath = getStringAttribute(info, ATTRIBUTE_RP);
        if (TextUtils.isEmpty(resourcePath)) {
            if (DEBUG) Log.d(TAG, "Missing RP " + info);
            return null;
        }
        if (resourcePath.startsWith("/")) {
            resourcePath = resourcePath.substring(1);
        }

        // Hopefully has a UUID
        Uri uuidUri = null;
        String uuid = getStringAttribute(info, ATTRIBUTE_UUID);
        if (!TextUtils.isEmpty(uuid)) {
            uuidUri = Uri.parse(PREFIX_URN_UUID + uuid);
        }

        // Must be IPv4
        if (!(info.getHost() instanceof Inet4Address)) {
            if (DEBUG) Log.d(TAG, "Not IPv4" + info);
            return null;
        }

        String scheme = info.getServiceType().contains(SERVICE_IPPS) ? SCHEME_IPPS : SCHEME_IPP;
        Uri path = Uri.parse(scheme + "://" + info.getHost().getHostAddress() + ":" + info.getPort()
                + "/" + resourcePath);
        String location = getStringAttribute(info, ATTRIBUTE_NOTE);

        return new DiscoveredPrinter(uuidUri, info.getServiceName(), path, location);
    }

    /** Return the value of an attribute or null if not present */
    private static String getStringAttribute(NsdServiceInfo info, String key) {
        key = key.toLowerCase(Locale.US);
        for (Map.Entry<String, byte[]> entry : info.getAttributes().entrySet()) {
            if (entry.getKey().toLowerCase(Locale.US).equals(key) && entry.getValue() != null) {
                return new String(entry.getValue());
            }
        }
        return null;
    }

    @Override
    void onStart() {
        if (DEBUG) Log.d(TAG, "onStart() " + mServiceName);
        NsdServiceListener serviceListener = new NsdServiceListener() {
            @Override
            public void onStartDiscoveryFailed(String s, int i) {
                // Do nothing
            }
        };

        WifiManager wifiManager = getPrintService().getSystemService(WifiManager.class);
        if (wifiManager != null) {
            if (mMulticastLock == null) {
                mMulticastLock = wifiManager.createMulticastLock(this.getClass().getName());
            }

            mMulticastLock.acquire();
        }

        NsdManager nsdManager = mNsdResolveQueue.getNsdManager();
        nsdManager.discoverServices(mServiceName, NsdManager.PROTOCOL_DNS_SD, serviceListener);
        mServiceListeners.add(serviceListener);
    }

    @Override
    void onStop() {
        if (DEBUG) Log.d(TAG, "onStop() " + mServiceName);
        NsdManager nsdManager = mNsdResolveQueue.getNsdManager();
        for (NsdServiceListener listener : mServiceListeners) {
            nsdManager.stopServiceDiscovery(listener);
        }
        mServiceListeners.clear();

        for (Resolver resolver : mResolvers) {
            resolver.cancel();
        }
        mResolvers.clear();

        if (mMulticastLock != null) {
            mMulticastLock.release();
        }
    }

    /**
     * Manage notifications from NsdManager
     */
    private abstract class NsdServiceListener implements NsdManager.DiscoveryListener {
        @Override
        public void onStopDiscoveryFailed(String s, int errorCode) {
            Log.w(TAG, "onStopDiscoveryFailed: " + errorCode);
        }

        @Override
        public void onDiscoveryStarted(String s) {
        }

        @Override
        public void onDiscoveryStopped(String service) {
            // On the main thread, notify loss of all known printers
            getHandler().post(MdnsDiscovery.this::allPrintersLost);
        }

        @Override
        public void onServiceFound(final NsdServiceInfo info) {
            if (DEBUG) Log.d(TAG, "found " + mServiceName + " name=" + info.getServiceName());
            getHandler().post(() -> mResolvers.add(new Resolver(info)));
        }

        @Override
        public void onServiceLost(final NsdServiceInfo info) {
            if (DEBUG) Log.d(TAG, "lost " + mServiceName + " name=" + info.getServiceName());

            // On the main thread, seek the missing printer by name and notify its loss
            getHandler().post(() -> {
                for (DiscoveredPrinter printer : getPrinters()) {
                    if (TextUtils.equals(printer.name, info.getServiceName())) {
                        printerLost(printer.getUri());
                        return;
                    }
                }
            });
        }
    }

    /**
     * Handle individual attempts to resolve
     */
    private class Resolver implements NsdManager.ResolveListener {
        private final NsdResolveQueue.NsdResolveRequest mResolveAttempt;

        Resolver(NsdServiceInfo info) {
            mResolveAttempt = mNsdResolveQueue.resolve(info, this);
        }

        @Override
        public void onResolveFailed(final NsdServiceInfo info, final int errorCode) {
            mResolvers.remove(this);
        }

        @Override
        public void onServiceResolved(final NsdServiceInfo info) {
            mResolvers.remove(this);
            if (!isStarted()) {
                return;
            }

            DiscoveredPrinter printer = toNetworkPrinter(info);
            if (DEBUG) Log.d(TAG, "Service " + info.getServiceName() + " resolved to " + printer);
            if (printer == null) {
                return;
            }
            printerFound(printer);
        }

        void cancel() {
            mResolveAttempt.cancel();
        }
    }
}
