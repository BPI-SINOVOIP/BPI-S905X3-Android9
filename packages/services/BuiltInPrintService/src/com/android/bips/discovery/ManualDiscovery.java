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
import android.text.TextUtils;
import android.util.Log;

import com.android.bips.BuiltInPrintService;
import com.android.bips.ipp.CapabilitiesCache;
import com.android.bips.jni.LocalPrinterCapabilities;
import com.android.bips.util.WifiMonitor;

import java.util.ArrayList;
import java.util.Collection;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Set;

/**
 * Manage a list of printers manually added by the user.
 */
public class ManualDiscovery extends SavedDiscovery {
    private static final String TAG = ManualDiscovery.class.getSimpleName();
    private static final boolean DEBUG = false;

    // Likely paths at which a print service may be found
    private static final Uri[] IPP_URIS = {Uri.parse("ipp://host:631/ipp/print"),
            Uri.parse("ipp://host:80/ipp/print"), Uri.parse("ipp://host:631/ipp/printer"),
            Uri.parse("ipp://host:631/ipp"), Uri.parse("ipp://host:631/"),
            Uri.parse("ipps://host:631/ipp/print"), Uri.parse("ipps://host:443/ipp/print"),
            Uri.parse("ipps://host:10443/ipp/print")};

    private WifiMonitor mWifiMonitor;
    private CapabilitiesCache mCapabilitiesCache;
    private List<CapabilitiesFinder> mAddRequests = new ArrayList<>();

    public ManualDiscovery(BuiltInPrintService printService) {
        super(printService);
        mCapabilitiesCache = getPrintService().getCapabilitiesCache();
    }

    @Override
    void onStart() {
        if (DEBUG) Log.d(TAG, "onStart");

        // Upon any network change scan for all manually added printers
        mWifiMonitor = new WifiMonitor(getPrintService(), isConnected -> {
            if (isConnected) {
                for (DiscoveredPrinter printer : getSavedPrinters()) {
                    mCapabilitiesCache.request(printer, false, capabilities -> {
                        if (capabilities != null) {
                            printerFound(printer);
                        }
                    });
                }
            } else {
                allPrintersLost();
            }
        });
    }

    @Override
    void onStop() {
        if (DEBUG) Log.d(TAG, "onStop");
        mWifiMonitor.close();
        allPrintersLost();
    }

    /**
     * Asynchronously attempt to add a new manual printer, calling back with success if
     * printer capabilities were discovered.
     *
     * The supplied URI must include a hostname and may also include a scheme (either ipp:// or
     * ipps://), a port (such as :443), and/or a path (like /ipp/print). If any parts are missing,
     * typical known values are substituted and searched until success is found, or all are
     * tried unsuccessfully.
     *
     * @param printerUri URI to search
     */
    public void addManualPrinter(Uri printerUri, PrinterAddCallback callback) {
        if (DEBUG) Log.d(TAG, "addManualPrinter " + printerUri);

        int givenPort = printerUri.getPort();
        String givenPath = printerUri.getPath();
        String hostname = printerUri.getHost();
        String givenScheme = printerUri.getScheme();

        // Use LinkedHashSet to eliminate duplicates but maintain order
        Set<Uri> uris = new LinkedHashSet<>();
        for (Uri uri : IPP_URIS) {
            String scheme = uri.getScheme();
            if (!TextUtils.isEmpty(givenScheme) && !scheme.equals(givenScheme)) {
                // If scheme was supplied and doesn't match this uri template, skip
                continue;
            }
            String authority = hostname + ":" + (givenPort == -1 ? uri.getPort() : givenPort);
            String path = TextUtils.isEmpty(givenPath) ? uri.getPath() : givenPath;
            Uri targetUri = uri.buildUpon().scheme(scheme).encodedAuthority(authority).path(path)
                    .build();
            uris.add(targetUri);
        }

        mAddRequests.add(new CapabilitiesFinder(uris, callback));
    }

    /**
     * Cancel a prior {@link #addManualPrinter(Uri, PrinterAddCallback)} attempt having the same
     * callback
     */
    public void cancelAddManualPrinter(PrinterAddCallback callback) {
        for (CapabilitiesFinder finder : mAddRequests) {
            if (finder.mFinalCallback == callback) {
                mAddRequests.remove(finder);
                finder.cancel();
                return;
            }
        }
    }

    /** Used to convey response to {@link #addManualPrinter} */
    public interface PrinterAddCallback {
        /**
         * The requested manual printer was found.
         *
         * @param printer   information about the discovered printer
         * @param supported true if the printer is supported (and was therefore added), or false
         *                  if the printer was found but is not supported (and was therefore not
         *                  added)
         */
        void onFound(DiscoveredPrinter printer, boolean supported);

        /**
         * The requested manual printer was not found.
         */
        void onNotFound();
    }

    /**
     * Search common printer paths for a successful response
     */
    private class CapabilitiesFinder {
        private final PrinterAddCallback mFinalCallback;
        private final List<CapabilitiesCache.OnLocalPrinterCapabilities> mRequests =
                new ArrayList<>();

        /**
         * Constructs a new finder
         *
         * @param uris     Locations to check for IPP endpoints
         * @param callback Callback to issue when the first successful response arrives, or
         *                 when all responses have failed.
         */
        CapabilitiesFinder(Collection<Uri> uris, PrinterAddCallback callback) {
            mFinalCallback = callback;
            for (Uri uri : uris) {
                CapabilitiesCache.OnLocalPrinterCapabilities capabilitiesCallback =
                        new CapabilitiesCache.OnLocalPrinterCapabilities() {
                            @Override
                            public void onCapabilities(LocalPrinterCapabilities capabilities) {
                                mRequests.remove(this);
                                handleCapabilities(uri, capabilities);
                            }
                        };
                mRequests.add(capabilitiesCallback);

                // Force a clean attempt from scratch
                mCapabilitiesCache.remove(uri);
                mCapabilitiesCache.request(new DiscoveredPrinter(null, "", uri, null),
                        true, capabilitiesCallback);
            }
        }

        /** Capabilities have arrived (or not) for the printer at a given path */
        void handleCapabilities(Uri printerPath, LocalPrinterCapabilities capabilities) {
            if (DEBUG) Log.d(TAG, "request " + printerPath + " cap=" + capabilities);

            if (capabilities == null) {
                if (mRequests.isEmpty()) {
                    mAddRequests.remove(this);
                    mFinalCallback.onNotFound();
                }
                return;
            }

            // Success, so cancel all other requests
            for (CapabilitiesCache.OnLocalPrinterCapabilities request : mRequests) {
                mCapabilitiesCache.cancel(request);
            }
            mRequests.clear();

            // Deliver a successful response
            Uri uuid = TextUtils.isEmpty(capabilities.uuid) ? null : Uri.parse(capabilities.uuid);
            String name = TextUtils.isEmpty(capabilities.name) ? printerPath.getHost()
                    : capabilities.name;

            DiscoveredPrinter resolvedPrinter = new DiscoveredPrinter(uuid, name, printerPath,
                    capabilities.location);

            // Only add supported printers
            if (capabilities.isSupported) {
                if (addSavedPrinter(resolvedPrinter)) {
                    printerFound(resolvedPrinter);
                }
            }
            mAddRequests.remove(this);
            mFinalCallback.onFound(resolvedPrinter, capabilities.isSupported);
        }

        /** Stop all in-progress capability requests that are in progress */
        public void cancel() {
            for (CapabilitiesCache.OnLocalPrinterCapabilities callback : mRequests) {
                mCapabilitiesCache.cancel(callback);
            }
            mRequests.clear();
        }
    }
}
