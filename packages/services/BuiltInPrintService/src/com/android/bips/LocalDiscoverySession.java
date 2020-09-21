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

import android.print.PrintManager;
import android.print.PrinterId;
import android.print.PrinterInfo;
import android.printservice.PrintServiceInfo;
import android.printservice.PrinterDiscoverySession;
import android.printservice.recommendation.RecommendationInfo;
import android.util.ArrayMap;
import android.util.ArraySet;
import android.util.Log;

import com.android.bips.discovery.DiscoveredPrinter;
import com.android.bips.discovery.Discovery;

import java.net.InetAddress;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

class LocalDiscoverySession extends PrinterDiscoverySession implements Discovery.Listener,
        PrintManager.PrintServiceRecommendationsChangeListener,
        PrintManager.PrintServicesChangeListener {
    private static final String TAG = LocalDiscoverySession.class.getSimpleName();
    private static final boolean DEBUG = false;

    // Printers are removed after not being seen for this long
    static final int PRINTER_EXPIRATION_MILLIS = 3000;

    private final BuiltInPrintService mPrintService;
    private final Map<PrinterId, LocalPrinter> mPrinters = new HashMap<>();
    private final Set<PrinterId> mTrackingIds = new HashSet<>();
    private final LocalDiscoverySessionInfo mInfo;
    private DelayedAction mExpirePrinters;
    private PrintManager mPrintManager;

    /** Package names of all currently enabled print services beside this one */
    private ArraySet<String> mEnabledServices = new ArraySet<>();

    /**
     * Address of printers that can be handled by print services, ordered by package name of the
     * print service. The print service might not be enabled. For that, look at
     * {@link #mEnabledServices}.
     *
     * <p>This print service only shows a printer if another print service does not show it.
     */
    private final ArrayMap<InetAddress, ArrayList<String>> mPrintersOfOtherService =
            new ArrayMap<>();

    LocalDiscoverySession(BuiltInPrintService service) {
        mPrintService = service;
        mPrintManager = mPrintService.getSystemService(PrintManager.class);
        mInfo = new LocalDiscoverySessionInfo(service);
    }

    @Override
    public void onStartPrinterDiscovery(List<PrinterId> priorityList) {
        if (DEBUG) Log.d(TAG, "onStartPrinterDiscovery() " + priorityList);

        // Mark all known printers as "not found". They may return shortly or may expire
        for (LocalPrinter printer : mPrinters.values()) {
            printer.notFound();
        }
        monitorExpiredPrinters();

        mPrintService.getDiscovery().start(this);

        mPrintManager.addPrintServicesChangeListener(this, null);
        onPrintServicesChanged();

        mPrintManager.addPrintServiceRecommendationsChangeListener(this, null);
        onPrintServiceRecommendationsChanged();
    }

    @Override
    public void onStopPrinterDiscovery() {
        if (DEBUG) Log.d(TAG, "onStopPrinterDiscovery()");
        mPrintService.getDiscovery().stop(this);

        PrintManager printManager = mPrintService.getSystemService(PrintManager.class);
        printManager.removePrintServicesChangeListener(this);
        printManager.removePrintServiceRecommendationsChangeListener(this);

        if (mExpirePrinters != null) {
            mExpirePrinters.cancel();
            mExpirePrinters = null;
        }
    }

    @Override
    public void onValidatePrinters(List<PrinterId> printerIds) {
        if (DEBUG) Log.d(TAG, "onValidatePrinters() " + printerIds);
    }

    @Override
    public void onStartPrinterStateTracking(final PrinterId printerId) {
        if (DEBUG) Log.d(TAG, "onStartPrinterStateTracking() " + printerId);
        LocalPrinter localPrinter = mPrinters.get(printerId);
        mTrackingIds.add(printerId);

        // We cannot track the printer yet; wait until it is discovered
        if (localPrinter == null || !localPrinter.isFound()) {
            return;
        }
        localPrinter.track();
    }

    @Override
    public void onStopPrinterStateTracking(PrinterId printerId) {
        if (DEBUG) Log.d(TAG, "onStopPrinterStateTracking() " + printerId.getLocalId());
        LocalPrinter localPrinter = mPrinters.get(printerId);
        if (localPrinter != null) {
            localPrinter.stopTracking();
        }
        mTrackingIds.remove(printerId);
    }

    @Override
    public void onDestroy() {
        if (DEBUG) Log.d(TAG, "onDestroy");
        mInfo.save();
    }

    /**
     * A printer was found during discovery
     */
    @Override
    public void onPrinterFound(DiscoveredPrinter discoveredPrinter) {
        if (DEBUG) Log.d(TAG, "onPrinterFound() " + discoveredPrinter);
        if (isDestroyed()) {
            Log.w(TAG, "Destroyed; ignoring");
            return;
        }

        PrinterId printerId = discoveredPrinter.getId(mPrintService);
        LocalPrinter localPrinter = mPrinters.computeIfAbsent(printerId,
                id -> new LocalPrinter(mPrintService, this, discoveredPrinter));

        localPrinter.found(discoveredPrinter);
        if (mTrackingIds.contains(printerId)) {
            localPrinter.track();
        }
    }

    /**
     * A printer was lost during discovery
     */
    @Override
    public void onPrinterLost(DiscoveredPrinter lostPrinter) {
        if (DEBUG) Log.d(TAG, "onPrinterLost() " + lostPrinter);

        mPrintService.getCapabilitiesCache().remove(lostPrinter.path);

        PrinterId printerId = lostPrinter.getId(mPrintService);
        LocalPrinter localPrinter = mPrinters.get(printerId);
        if (localPrinter == null) {
            return;
        }

        localPrinter.notFound();
        handlePrinter(localPrinter);
        monitorExpiredPrinters();
    }

    private void monitorExpiredPrinters() {
        if (mExpirePrinters == null && !mPrinters.isEmpty()) {
            mExpirePrinters = mPrintService.delay(PRINTER_EXPIRATION_MILLIS, () -> {
                mExpirePrinters = null;
                boolean allFound = true;
                List<PrinterId> idsToRemove = new ArrayList<>();

                for (LocalPrinter localPrinter : mPrinters.values()) {
                    if (localPrinter.isExpired()) {
                        if (DEBUG) Log.d(TAG, "Expiring " + localPrinter);
                        idsToRemove.add(localPrinter.getPrinterId());
                    }
                    if (!localPrinter.isFound()) {
                        allFound = false;
                    }
                }
                for (PrinterId id : idsToRemove) {
                    mPrinters.remove(id);
                }
                removePrinters(idsToRemove);
                if (!allFound) {
                    monitorExpiredPrinters();
                }
            });
        }
    }

    /** A complete printer record is available */
    void handlePrinter(LocalPrinter localPrinter) {
        if (DEBUG) Log.d(TAG, "handlePrinter record " + localPrinter);

        boolean knownGood = mInfo.isKnownGood(localPrinter.getPrinterId());
        PrinterInfo info = localPrinter.createPrinterInfo(knownGood);
        if (info == null) {
            return;
        }

        if (info.getStatus() == PrinterInfo.STATUS_IDLE && localPrinter.getUuid() != null) {
            // Mark UUID-based printers with IDLE status as known-good
            mInfo.setKnownGood(localPrinter.getPrinterId());
        }

        for (PrinterInfo knownInfo : getPrinters()) {
            if (knownInfo.getId().equals(info.getId()) && (info.getCapabilities() == null)) {
                if (DEBUG) Log.d(TAG, "Ignore update with no caps " + localPrinter);
                return;
            }
        }

        if (DEBUG) {
            Log.d(TAG, "handlePrinter: reporting " + localPrinter
                    + " caps=" + (info.getCapabilities() != null) + " status=" + info.getStatus()
                    + " summary=" + info.getDescription());
        }

        if (!isHandledByOtherService(localPrinter)) {
            addPrinters(Collections.singletonList(info));
        }
    }

    /**
     * Return true if the {@link PrinterId} corresponds to a high-priority printer
     */
    boolean isPriority(PrinterId printerId) {
        return mTrackingIds.contains(printerId);
    }

    /**
     * Return true if the {@link PrinterId} corresponds to a known printer
     */
    boolean isKnown(PrinterId printerId) {
        return mPrinters.containsKey(printerId);
    }

    /**
     * Is this printer handled by another print service and should be suppressed?
     *
     * @param printer The printer that might need to be suppressed
     *
     * @return {@code true} iff the printer should be suppressed
     */
    private boolean isHandledByOtherService(LocalPrinter printer) {
        InetAddress address = printer.getAddress();
        if (address == null) {
            return false;
        }

        ArrayList<String> printerServices = mPrintersOfOtherService.get(printer.getAddress());

        if (printerServices != null) {
            int numServices = printerServices.size();
            for (int i = 0; i < numServices; i++) {
                if (mEnabledServices.contains(printerServices.get(i))) {
                    return true;
                }
            }
        }

        return false;
    }

    /**
     * If the system's print service state changed some printer might be newly suppressed or not
     * suppressed anymore.
     */
    private void onPrintServicesStateUpdated() {
        ArrayList<PrinterInfo> printersToAdd = new ArrayList<>();
        ArrayList<PrinterId> printersToRemove = new ArrayList<>();
        for (LocalPrinter printer : mPrinters.values()) {
            boolean knownGood = mInfo.isKnownGood(printer.getPrinterId());
            PrinterInfo info = printer.createPrinterInfo(knownGood);

            if (printer.getCapabilities() != null && printer.isFound()
                    && !isHandledByOtherService(printer) && info != null) {
                printersToAdd.add(info);
            } else {
                printersToRemove.add(printer.getPrinterId());
            }
        }

        removePrinters(printersToRemove);
        addPrinters(printersToAdd);
    }

    @Override
    public void onPrintServiceRecommendationsChanged() {
        mPrintersOfOtherService.clear();

        List<RecommendationInfo> infos = mPrintManager.getPrintServiceRecommendations();

        int numInfos = infos.size();
        for (int i = 0; i < numInfos; i++) {
            RecommendationInfo info = infos.get(i);
            String packageName = info.getPackageName().toString();

            if (!packageName.equals(mPrintService.getPackageName())) {
                for (InetAddress address : info.getDiscoveredPrinters()) {
                    ArrayList<String> services = mPrintersOfOtherService.get(address);

                    if (services == null) {
                        services = new ArrayList<>(1);
                        mPrintersOfOtherService.put(address, services);
                    }

                    services.add(packageName);
                }
            }
        }

        onPrintServicesStateUpdated();
    }

    @Override
    public void onPrintServicesChanged() {
        mEnabledServices.clear();

        List<PrintServiceInfo> infos = mPrintManager.getPrintServices(
                PrintManager.ENABLED_SERVICES);

        int numInfos = infos.size();
        for (int i = 0; i < numInfos; i++) {
            PrintServiceInfo info = infos.get(i);
            String packageName = info.getComponentName().getPackageName();

            if (!packageName.equals(mPrintService.getPackageName())) {
                mEnabledServices.add(packageName);
            }
        }

        onPrintServicesStateUpdated();
    }
}
