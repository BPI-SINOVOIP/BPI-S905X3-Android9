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

import android.Manifest;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.net.wifi.p2p.WifiP2pDevice;
import android.os.Bundle;
import android.os.IBinder;
import android.preference.Preference;
import android.preference.PreferenceCategory;
import android.preference.PreferenceFragment;
import android.util.Log;

import com.android.bips.BuiltInPrintService;
import com.android.bips.R;
import com.android.bips.discovery.DiscoveredPrinter;
import com.android.bips.discovery.P2pDiscovery;
import com.android.bips.p2p.P2pMonitor;
import com.android.bips.p2p.P2pPeerListener;

/**
 * Present a list of previously-saved printers, and allow them to be removed
 */
public class FindP2pPrintersFragment extends PreferenceFragment implements ServiceConnection,
        AddPrintersActivity.OnPermissionChangeListener {
    private static final String TAG = FindP2pPrintersFragment.class.getSimpleName();
    private static final boolean DEBUG = false;

    private static final String KEY_AVAILABLE = "available";
    private static final int REQUEST_PERMISSION = 1;

    private BuiltInPrintService mPrintService;
    private P2pListener mPeerDiscoveryListener;
    private PreferenceCategory mAvailableCategory;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.find_p2p_prefs);
        mAvailableCategory = (PreferenceCategory) getPreferenceScreen()
                .findPreference(KEY_AVAILABLE);
    }

    @Override
    public void onStart() {
        super.onStart();
        if (DEBUG) Log.d(TAG, "onStart");
        getActivity().setTitle(R.string.wifi_direct_printers);
        getContext().bindService(new Intent(getContext(), BuiltInPrintService.class), this,
                Context.BIND_AUTO_CREATE);
    }

    @Override
    public void onStop() {
        super.onStop();
        if (DEBUG) Log.d(TAG, "onStop");
        if (mPeerDiscoveryListener != null) {
            mPrintService.getP2pMonitor().stopDiscover(mPeerDiscoveryListener);
            mPeerDiscoveryListener = null;
        }
        getContext().unbindService(this);
        mPrintService = null;
    }

    @Override
    public void onServiceConnected(ComponentName componentName, IBinder iBinder) {
        if (DEBUG) Log.d(TAG, "onServiceConnected");
        mPrintService = BuiltInPrintService.getInstance();
        if (mPrintService == null) {
            return;
        }

        // If we do not yet have permissions, ask.
        if (getContext().checkSelfPermission(Manifest.permission.ACCESS_COARSE_LOCATION)
                != PackageManager.PERMISSION_GRANTED) {
            getActivity().requestPermissions(
                    new String[]{Manifest.permission.ACCESS_COARSE_LOCATION},
                    REQUEST_PERMISSION);
        } else {
            startP2pDiscovery();
        }
    }

    private void startP2pDiscovery() {
        if (mPrintService != null && mPeerDiscoveryListener == null) {
            mPeerDiscoveryListener = new P2pListener();
            mPrintService.getP2pMonitor().discover(mPeerDiscoveryListener);
        }
    }

    @Override
    public void onPermissionChange() {
        // P2P discovery requires dangerous ACCESS_COARSE_LOCATION
        if (getContext().checkSelfPermission(Manifest.permission.ACCESS_COARSE_LOCATION)
                == PackageManager.PERMISSION_GRANTED) {
            startP2pDiscovery();
        } else {
            // Wind back out of this fragment
            getActivity().onBackPressed();
        }
    }

    @Override
    public void onServiceDisconnected(ComponentName componentName) {
        mPrintService = null;
    }

    /** Handle discovered P2P printers */
    private class P2pListener implements P2pPeerListener {
        @Override
        public void onPeerFound(WifiP2pDevice peer) {
            if (DEBUG) Log.d(TAG, "onPeerFound: " + P2pMonitor.toString(peer));
            if (mPrintService == null) {
                return;
            }

            DiscoveredPrinter printer = P2pDiscovery.toPrinter(peer);

            // Ignore printers we have already added
            for (DiscoveredPrinter prior : mPrintService.getP2pDiscovery().getSavedPrinters()) {
                if (prior.path.equals(printer.path)) {
                    return;
                }
            }

            // Install a preference so the user can add this printer
            PrinterPreference pref = getPrinterPreference(printer.getUri());
            if (pref != null) {
                pref.updatePrinter(printer);
            } else {
                pref = new PrinterPreference(getContext(), mPrintService, printer, true);
                pref.setOnPreferenceClickListener(preference -> {
                    if (DEBUG) Log.d(TAG, "add " + P2pDiscovery.toPrinter(peer));
                    new AddP2pPrinterDialog(FindP2pPrintersFragment.this, mPrintService, peer)
                            .show();
                    return true;
                });
                mAvailableCategory.addPreference(pref);
            }
        }

        @Override
        public void onPeerLost(WifiP2pDevice peer) {
            if (DEBUG) Log.d(TAG, "onPeerLost: " + P2pMonitor.toString(peer));
            if (mPrintService == null) {
                return;
            }

            DiscoveredPrinter printer = P2pDiscovery.toPrinter(peer);

            // Remove this preference because the printer is no longer available
            PrinterPreference pref = getPrinterPreference(printer.path);
            if (pref != null) {
                mAvailableCategory.removePreference(pref);
            }
        }

        private PrinterPreference getPrinterPreference(Uri printerPath) {
            for (int i = 0; i < mAvailableCategory.getPreferenceCount(); i++) {
                Preference pref = mAvailableCategory.getPreference(i);
                if (pref instanceof PrinterPreference
                        && ((PrinterPreference) pref).getPrinter().path.equals(printerPath)) {
                    return (PrinterPreference) pref;
                }
            }
            return null;
        }
    }
}
