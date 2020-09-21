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

package com.android.bips.ui;

import android.annotation.SuppressLint;
import android.app.AlertDialog;
import android.app.Fragment;
import android.net.wifi.p2p.WifiP2pDevice;
import android.os.Bundle;
import android.view.View;

import com.android.bips.BuiltInPrintService;
import com.android.bips.R;
import com.android.bips.discovery.ConnectionListener;
import com.android.bips.discovery.DiscoveredPrinter;
import com.android.bips.p2p.P2pPrinterConnection;

/**
 * Keep users aware of the P2P connection progress and allow them to cancel.
 */
public class AddP2pPrinterDialog extends AlertDialog implements ConnectionListener {
    private final WifiP2pDevice mPeer;
    private final BuiltInPrintService mPrintService;
    private final Fragment mFragment;

    private P2pPrinterConnection mValidating;

    AddP2pPrinterDialog(Fragment fragment, BuiltInPrintService printService, WifiP2pDevice peer) {
        super(fragment.getContext());
        mFragment = fragment;
        mPrintService = printService;
        mPeer = peer;
    }

    @SuppressLint("InflateParams")
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        // Prepare the dialog for display
        setView(getLayoutInflater().inflate(R.layout.manual_printer_add, null));
        setTitle(getContext().getString(R.string.connecting_to, mPeer.deviceName));
        setButton(AlertDialog.BUTTON_NEGATIVE, getContext().getString(android.R.string.cancel),
                (OnClickListener) null);
        super.onCreate(savedInstanceState);

        findViewById(R.id.labelHostname).setVisibility(View.GONE);
        findViewById(R.id.hostname).setVisibility(View.GONE);
        findViewById(R.id.progress).setVisibility(View.VISIBLE);

        setOnDismissListener(d -> mValidating.close());

        // Attempt to add the discovered device as a P2P printer
        mValidating = new P2pPrinterConnection(mPrintService, mPeer, this);
    }

    @Override
    public void onConnectionComplete(DiscoveredPrinter printer) {
        if (printer != null) {
            // Callback could arrive quickly if we are already connected, so run later
            mPrintService.getMainHandler().post(() -> {
                mValidating.close();
                mPrintService.getP2pDiscovery().addValidPrinter(printer);
                mFragment.getActivity().finish();
            });
            dismiss();
        } else {
            fail();
        }
    }

    @Override
    public void onConnectionDelayed(boolean delayed) {
        findViewById(R.id.connect_hint).setVisibility(delayed ? View.VISIBLE :
                View.GONE);
    }

    private void fail() {
        cancel();
        new AlertDialog.Builder(getContext())
                .setTitle(getContext().getString(R.string.failed_connection,
                        mPeer.deviceName))
                .setPositiveButton(android.R.string.ok, null)
                .show();
    }
}
