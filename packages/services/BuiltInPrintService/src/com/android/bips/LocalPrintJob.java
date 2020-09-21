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

import android.net.Uri;
import android.print.PrintJobId;
import android.printservice.PrintJob;
import android.util.Log;

import com.android.bips.discovery.ConnectionListener;
import com.android.bips.discovery.DiscoveredPrinter;
import com.android.bips.discovery.MdnsDiscovery;
import com.android.bips.ipp.Backend;
import com.android.bips.ipp.CapabilitiesCache;
import com.android.bips.ipp.JobStatus;
import com.android.bips.jni.BackendConstants;
import com.android.bips.jni.LocalPrinterCapabilities;
import com.android.bips.p2p.P2pPrinterConnection;
import com.android.bips.p2p.P2pUtils;

import java.util.function.Consumer;

/**
 * Manage the process of delivering a print job
 */
class LocalPrintJob implements MdnsDiscovery.Listener, ConnectionListener,
        CapabilitiesCache.OnLocalPrinterCapabilities {
    private static final String TAG = LocalPrintJob.class.getSimpleName();
    private static final boolean DEBUG = false;

    /** Maximum time to wait to find a printer before failing the job */
    private static final int DISCOVERY_TIMEOUT = 2 * 60 * 1000;

    // Internal job states
    private static final int STATE_INIT = 0;
    private static final int STATE_DISCOVERY = 1;
    private static final int STATE_CAPABILITIES = 2;
    private static final int STATE_DELIVERING = 3;
    private static final int STATE_CANCEL = 4;
    private static final int STATE_DONE = 5;

    private final BuiltInPrintService mPrintService;
    private final PrintJob mPrintJob;
    private final Backend mBackend;

    private int mState;
    private Consumer<LocalPrintJob> mCompleteConsumer;
    private Uri mPath;
    private DelayedAction mDiscoveryTimeout;
    private P2pPrinterConnection mConnection;

    /**
     * Construct the object; use {@link #start(Consumer)} to begin job processing.
     */
    LocalPrintJob(BuiltInPrintService printService, Backend backend, PrintJob printJob) {
        mPrintService = printService;
        mBackend = backend;
        mPrintJob = printJob;
        mState = STATE_INIT;

        // Tell the job it is blocked (until start())
        mPrintJob.start();
        mPrintJob.block(printService.getString(R.string.waiting_to_send));
    }

    /**
     * Begin the process of delivering the job. Internally, discovers the target printer,
     * obtains its capabilities, delivers the job to the printer, and waits for job completion.
     *
     * @param callback Callback to be issued when job processing is complete
     */
    void start(Consumer<LocalPrintJob> callback) {
        if (DEBUG) Log.d(TAG, "start() " + mPrintJob);
        if (mState != STATE_INIT) {
            Log.w(TAG, "Invalid start state " + mState);
            return;
        }
        mPrintJob.start();

        // Acquire a lock so that WiFi isn't put to sleep while we send the job
        mPrintService.lockWifi();

        mState = STATE_DISCOVERY;
        mCompleteConsumer = callback;
        mDiscoveryTimeout = mPrintService.delay(DISCOVERY_TIMEOUT, () -> {
            if (DEBUG) Log.d(TAG, "Discovery timeout");
            if (mState == STATE_DISCOVERY) {
                finish(false, mPrintService.getString(R.string.printer_offline));
            }
        });

        mPrintService.getDiscovery().start(this);
    }

    void cancel() {
        if (DEBUG) Log.d(TAG, "cancel() " + mPrintJob + " in state " + mState);

        switch (mState) {
            case STATE_DISCOVERY:
            case STATE_CAPABILITIES:
                // Cancel immediately
                mState = STATE_CANCEL;
                finish(false, null);
                break;

            case STATE_DELIVERING:
                // Request cancel and wait for completion
                mState = STATE_CANCEL;
                mBackend.cancel();
                break;
        }
    }

    PrintJobId getPrintJobId() {
        return mPrintJob.getId();
    }

    @Override
    public void onPrinterFound(DiscoveredPrinter printer) {
        if (mState != STATE_DISCOVERY) {
            return;
        }
        if (!printer.getId(mPrintService).equals(mPrintJob.getInfo().getPrinterId())) {
            return;
        }

        if (DEBUG) Log.d(TAG, "onPrinterFound() " + printer.name + " state=" + mState);

        if (P2pUtils.isP2p(printer)) {
            // Launch a P2P connection attempt
            mConnection = new P2pPrinterConnection(mPrintService, printer, this);
            return;
        }

        if (P2pUtils.isOnConnectedInterface(mPrintService, printer) && mConnection == null) {
            // Hold the P2P connection up during printing
            mConnection = new P2pPrinterConnection(mPrintService, printer, this);
        }

        // We have a good path so stop discovering and get capabilities
        mPrintService.getDiscovery().stop(this);
        mState = STATE_CAPABILITIES;
        mPath = printer.path;
        mPrintService.getCapabilitiesCache().request(printer, true, this);
    }

    @Override
    public void onPrinterLost(DiscoveredPrinter printer) {
        // Ignore (the capability request, if any, will fail)
    }

    @Override
    public void onConnectionComplete(DiscoveredPrinter printer) {
        // Ignore late connection events
        if (mState != STATE_DISCOVERY) {
            return;
        }

        if (printer == null) {
            finish(false, mPrintService.getString(R.string.failed_printer_connection));
        } else if (mPrintJob.isBlocked()) {
            mPrintJob.start();
        }
    }

    @Override
    public void onConnectionDelayed(boolean delayed) {
        if (DEBUG) Log.d(TAG, "onConnectionDelayed " + delayed);

        // Ignore late events
        if (mState != STATE_DISCOVERY) {
            return;
        }

        if (delayed) {
            mPrintJob.block(mPrintService.getString(R.string.connect_hint_text));
        } else {
            // Remove block message
            mPrintJob.start();
        }
    }

    PrintJob getPrintJob() {
        return mPrintJob;
    }

    @Override
    public void onCapabilities(LocalPrinterCapabilities capabilities) {
        if (DEBUG) Log.d(TAG, "Capabilities for " + mPath + " are " + capabilities);
        if (mState != STATE_CAPABILITIES) {
            return;
        }

        if (capabilities == null) {
            finish(false, mPrintService.getString(R.string.printer_offline));
        } else {
            if (DEBUG) Log.d(TAG, "Starting backend print of " + mPrintJob);
            if (mDiscoveryTimeout != null) {
                mDiscoveryTimeout.cancel();
            }
            mState = STATE_DELIVERING;
            mPrintJob.start();
            mBackend.print(mPath, mPrintJob, capabilities, this::handleJobStatus);
        }
    }

    private void handleJobStatus(JobStatus jobStatus) {
        if (DEBUG) Log.d(TAG, "onJobStatus() " + jobStatus);
        switch (jobStatus.getJobState()) {
            case BackendConstants.JOB_STATE_DONE:
                switch (jobStatus.getJobResult()) {
                    case BackendConstants.JOB_DONE_OK:
                        finish(true, null);
                        break;
                    case BackendConstants.JOB_DONE_CANCELLED:
                        mState = STATE_CANCEL;
                        finish(false, null);
                        break;
                    case BackendConstants.JOB_DONE_CORRUPT:
                        finish(false, mPrintService.getString(R.string.unreadable_input));
                        break;
                    default:
                        // Job failed
                        finish(false, null);
                        break;
                }
                break;

            case BackendConstants.JOB_STATE_BLOCKED:
                if (mState == STATE_CANCEL) {
                    return;
                }
                int blockedId = jobStatus.getBlockedReasonId();
                blockedId = (blockedId == 0) ? R.string.printer_check : blockedId;
                String blockedReason = mPrintService.getString(blockedId);
                mPrintJob.block(blockedReason);
                break;

            case BackendConstants.JOB_STATE_RUNNING:
                if (mState == STATE_CANCEL) {
                    return;
                }
                mPrintJob.start();
                break;
        }
    }

    /**
     * Terminate the job, issuing appropriate notifications.
     *
     * @param success true if the printer reported successful job completion
     * @param error   reason for job failure if known
     */
    private void finish(boolean success, String error) {
        if (DEBUG) Log.d(TAG, "finish() success=" + success + ", error=" + error);
        mPrintService.getDiscovery().stop(this);
        if (mDiscoveryTimeout != null) {
            mDiscoveryTimeout.cancel();
        }
        if (mConnection != null) {
            mConnection.close();
        }
        mPrintService.unlockWifi();
        mBackend.closeDocument();
        if (success) {
            // Job must not be blocked before completion
            mPrintJob.start();
            mPrintJob.complete();
        } else if (mState == STATE_CANCEL) {
            mPrintJob.cancel();
        } else {
            mPrintJob.fail(error);
        }
        mState = STATE_DONE;
        mCompleteConsumer.accept(LocalPrintJob.this);
    }
}
