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
import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import com.android.bips.BuiltInPrintService;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.concurrent.CopyOnWriteArrayList;

/**
 * Parent class for all printer discovery mechanisms. Subclasses must implement onStart and onStop.
 * While started, discovery mechanisms deliver DiscoveredPrinter objects via
 * {@link #printerFound(DiscoveredPrinter)} when they appear, and {@link #printerLost(Uri)} when
 * they become unavailable.
 */
public abstract class Discovery {
    private static final String TAG = Discovery.class.getSimpleName();
    private static final boolean DEBUG = false;

    private final BuiltInPrintService mPrintService;
    private final List<Listener> mListeners = new CopyOnWriteArrayList<>();
    private final Map<Uri, DiscoveredPrinter> mPrinters = new HashMap<>();
    private final Handler mHandler = new Handler(Looper.getMainLooper());

    private boolean mStarted = false;

    Discovery(BuiltInPrintService printService) {
        mPrintService = printService;
    }

    /**
     * Add a listener and begin receiving notifications from the Discovery object of any
     * printers it finds.
     */
    public void start(Listener listener) {
        mListeners.add(listener);

        // If printers are already present, signal them to the listener
        if (!mPrinters.isEmpty()) {
            if (!mListeners.contains(listener)) {
                return;
            }
            for (DiscoveredPrinter printer : new ArrayList<>(mPrinters.values())) {
                listener.onPrinterFound(printer);
            }
        }

        start();
    }

    /**
     * Remove a listener so that it no longer receives notifications of found printers.
     * Discovery will continue for other listeners until the last one is removed.
     */
    public void stop(Listener listener) {
        mListeners.remove(listener);
        if (mListeners.isEmpty()) {
            stop();
        }
    }

    /**
     * Return true if this object is in a started state
     */
    boolean isStarted() {
        return mStarted;
    }

    /**
     * Return the current print service instance
     */
    BuiltInPrintService getPrintService() {
        return mPrintService;
    }

    /**
     * Return a handler to defer actions to the main (UI) thread while started. All delayed actions
     * scheduled on this handler are cancelled at {@link #stop()} time.
     */
    Handler getHandler() {
        return mHandler;
    }

    /**
     * Start if not already started
     */
    private void start() {
        if (!mStarted) {
            mStarted = true;
            onStart();
        }
    }

    /**
     * Stop if not already stopped
     */
    private void stop() {
        if (mStarted) {
            mStarted = false;
            onStop();
            mPrinters.clear();
            mHandler.removeCallbacksAndMessages(null);
        }
    }

    /**
     * Start searching for printers
     */
    abstract void onStart();

    /**
     * Stop searching for printers, freeing any search-related resources
     */
    abstract void onStop();

    /**
     * Signal that a printer appeared or possibly changed state.
     */
    void printerFound(DiscoveredPrinter printer) {
        DiscoveredPrinter current = mPrinters.get(printer.getUri());
        if (Objects.equals(current, printer)) {
            if (DEBUG) Log.d(TAG, "Already have the reported printer, ignoring");
            return;
        }
        mPrinters.put(printer.getUri(), printer);
        for (Listener listener : mListeners) {
            listener.onPrinterFound(printer);
        }
    }

    /**
     * Signal that a printer is no longer visible
     */
    void printerLost(Uri printerUri) {
        DiscoveredPrinter printer = mPrinters.remove(printerUri);
        if (printer == null) {
            return;
        }
        for (Listener listener : mListeners) {
            listener.onPrinterLost(printer);
        }
    }

    /** Signal loss of all printers */
    void allPrintersLost() {
        for (Uri uri: new ArrayList<>(mPrinters.keySet())) {
            printerLost(uri);
        }
    }

    /**
     * Return the working collection of currently-found printers
     */
    public Collection<DiscoveredPrinter> getPrinters() {
        return mPrinters.values();
    }

    /**
     * Return printer matching the uri, or null if none
     */
    public DiscoveredPrinter getPrinter(Uri uri) {
        return mPrinters.get(uri);
    }

    /**
     * Return a collection of leaf objects. By default returns a collection containing this object.
     * Subclasses wrapping other {@link Discovery} objects should override this method.
     */
    Collection<Discovery> getChildren() {
        return Collections.singleton(this);
    }

    /**
     * Return a collection of saved printers. Subclasses supporting saved printers should override
     * this method.
     */
    public Collection<DiscoveredPrinter> getSavedPrinters() {
        List<DiscoveredPrinter> printers = new ArrayList<>();
        for (Discovery child : getChildren()) {
            if (child != this) {
                printers.addAll(child.getSavedPrinters());
            }
        }
        return printers;
    }

    /**
     * Remove a saved printer by its path. Subclasses supporting saved printers should override
     * this method.
     */
    public void removeSavedPrinter(Uri printerPath) {
        for (Discovery child : getChildren()) {
            if (child != this) {
                child.removeSavedPrinter(printerPath);
            }
        }
    }

    public interface Listener {
        /**
         * A new printer has been discovered, or an existing printer has been updated
         */
        void onPrinterFound(DiscoveredPrinter printer);

        /**
         * A previously-found printer is no longer discovered.
         */
        void onPrinterLost(DiscoveredPrinter printer);
    }
}
