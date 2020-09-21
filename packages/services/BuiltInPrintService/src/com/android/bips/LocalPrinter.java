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

package com.android.bips;

import android.net.Uri;
import android.print.PrinterCapabilitiesInfo;
import android.print.PrinterId;
import android.print.PrinterInfo;
import android.util.Log;
import android.widget.Toast;

import com.android.bips.discovery.ConnectionListener;
import com.android.bips.discovery.DiscoveredPrinter;
import com.android.bips.ipp.CapabilitiesCache;
import com.android.bips.jni.LocalPrinterCapabilities;
import com.android.bips.p2p.P2pPrinterConnection;
import com.android.bips.p2p.P2pUtils;

import java.net.InetAddress;
import java.util.Collections;

/**
 * A session-specific printer record. Encapsulates logic for getting the latest printer
 * capabilities as necessary.
 */
class LocalPrinter implements CapabilitiesCache.OnLocalPrinterCapabilities {
    private static final String TAG = LocalPrinter.class.getSimpleName();
    private static final boolean DEBUG = false;

    private final BuiltInPrintService mPrintService;
    private final LocalDiscoverySession mSession;
    private final PrinterId mPrinterId;
    private long mLastSeenTime = System.currentTimeMillis();
    private boolean mFound = true;
    private boolean mTracking = false;
    private LocalPrinterCapabilities mCapabilities;
    private DiscoveredPrinter mDiscoveredPrinter;
    private P2pPrinterConnection mTrackingConnection;

    LocalPrinter(BuiltInPrintService printService, LocalDiscoverySession session,
            DiscoveredPrinter discoveredPrinter) {
        mPrintService = printService;
        mSession = session;
        mDiscoveredPrinter = discoveredPrinter;
        mPrinterId = discoveredPrinter.getId(printService);
    }

    /** Return the address of the printer or {@code null} if not known */
    public InetAddress getAddress() {
        if (mCapabilities != null) {
            return mCapabilities.inetAddress;
        }
        return null;
    }

    /** Return true if this printer should be aged out */
    boolean isExpired() {
        return !mFound && (System.currentTimeMillis() - mLastSeenTime)
                > LocalDiscoverySession.PRINTER_EXPIRATION_MILLIS;
    }

    /** Return capabilities or null if not present */
    LocalPrinterCapabilities getCapabilities() {
        return mCapabilities;
    }

    /** Create a PrinterInfo from this record or null if not possible */
    PrinterInfo createPrinterInfo(boolean knownGood) {
        if (mCapabilities == null) {
            if (P2pUtils.isP2p(mDiscoveredPrinter)) {
                // Allow user to select a P2P to establish a connection
                PrinterInfo.Builder builder = new PrinterInfo.Builder(
                        mPrinterId, mDiscoveredPrinter.name,
                        PrinterInfo.STATUS_IDLE)
                        .setIconResourceId(R.drawable.ic_printer)
                        .setDescription(mPrintService.getDescription(mDiscoveredPrinter));
                return builder.build();
            } else if (!knownGood) {
                // Ignore unknown LAN printers with no caps
                return null;
            }
        } else if (!mCapabilities.isSupported) {
            // Fail out if capabilities indicate not-supported.
            return null;
        }

        // Get the most recently discovered version of this printer
        DiscoveredPrinter printer = mPrintService.getDiscovery()
                .getPrinter(mDiscoveredPrinter.getUri());
        if (printer == null) {
            return null;
        }

        boolean idle = mFound && mCapabilities != null;
        PrinterInfo.Builder builder = new PrinterInfo.Builder(
                mPrinterId, printer.name,
                idle ? PrinterInfo.STATUS_IDLE : PrinterInfo.STATUS_UNAVAILABLE)
                .setIconResourceId(R.drawable.ic_printer)
                .setDescription(mPrintService.getDescription(mDiscoveredPrinter));

        if (mCapabilities != null) {
            // Add capabilities if we have them
            PrinterCapabilitiesInfo.Builder capabilitiesBuilder =
                    new PrinterCapabilitiesInfo.Builder(mPrinterId);
            mCapabilities.buildCapabilities(mPrintService, capabilitiesBuilder);
            builder.setCapabilities(capabilitiesBuilder.build());
        }

        return builder.build();
    }

    @Override
    public void onCapabilities(LocalPrinterCapabilities capabilities) {
        if (mSession.isDestroyed() || !mSession.isKnown(mPrinterId)) {
            return;
        }

        if (capabilities == null) {
            if (DEBUG) Log.d(TAG, "No capabilities so removing printer " + this);
            mSession.removePrinters(Collections.singletonList(mPrinterId));
        } else {
            mCapabilities = capabilities;
            mSession.handlePrinter(this);
        }
    }

    PrinterId getPrinterId() {
        return mPrinterId;
    }

    /** Return true if the printer is in a "found" state according to discoveries */
    boolean isFound() {
        return mFound;
    }

    /**
     * Indicate the printer was found and gather capabilities if we don't have them
     */
    void found(DiscoveredPrinter printer) {
        mDiscoveredPrinter = printer;
        mLastSeenTime = System.currentTimeMillis();
        mFound = true;

        // Check for cached capabilities
        LocalPrinterCapabilities capabilities = mPrintService.getCapabilitiesCache()
                .get(mDiscoveredPrinter);

        if (capabilities != null) {
            // Report current capabilities
            onCapabilities(capabilities);
        } else {
            // Announce printer and fetch capabilities immediately if possible
            mSession.handlePrinter(this);
            if (!P2pUtils.isP2p(mDiscoveredPrinter)) {
                mPrintService.getCapabilitiesCache().request(mDiscoveredPrinter,
                        mSession.isPriority(mPrinterId), this);
            } else if (mTracking) {
                startTracking();
            }
        }
    }

    /**
     * Begin tracking (getting latest capabilities) for this printer
     */
    public void track() {
        if (DEBUG) Log.d(TAG, "track " + mDiscoveredPrinter);
        startTracking();
    }

    private void startTracking() {
        mTracking = true;
        if (mTrackingConnection != null) {
            return;
        }

        // For any P2P printer, obtain a connection
        if (P2pUtils.isP2p(mDiscoveredPrinter)
                || P2pUtils.isOnConnectedInterface(mPrintService, mDiscoveredPrinter)) {
            ConnectionListener listener = new ConnectionListener() {
                @Override
                public void onConnectionComplete(DiscoveredPrinter printer) {
                    if (DEBUG) Log.d(TAG, "connection complete " + printer);
                    if (printer == null) {
                        mTrackingConnection = null;
                    }
                }

                @Override
                public void onConnectionDelayed(boolean delayed) {
                    if (DEBUG) Log.d(TAG, "connection delayed=" + delayed);
                    if (delayed) {
                        Toast.makeText(mPrintService, R.string.connect_hint_text,
                                Toast.LENGTH_LONG).show();
                    }
                }
            };
            mTrackingConnection = new P2pPrinterConnection(mPrintService,
                    mDiscoveredPrinter, listener);
        }
    }

    /**
     * Stop tracking this printer
     */
    void stopTracking() {
        if (mTrackingConnection != null) {
            mTrackingConnection.close();
            mTrackingConnection = null;
        }
        mTracking = false;
    }

    /**
     * Mark this printer as not found (will eventually expire)
     */
    void notFound() {
        mFound = false;
        mLastSeenTime = System.currentTimeMillis();
    }

    /** Return the UUID for this printer if it is known */
    public Uri getUuid() {
        return mDiscoveredPrinter.uuid;
    }

    @Override
    public String toString() {
        return mDiscoveredPrinter.toString();
    }
}
