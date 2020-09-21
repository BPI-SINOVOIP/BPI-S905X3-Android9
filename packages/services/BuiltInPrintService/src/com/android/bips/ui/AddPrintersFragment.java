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

package com.android.bips.ui;

import android.app.AlertDialog;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.preference.Preference;
import android.preference.PreferenceCategory;
import android.preference.PreferenceFragment;
import android.util.Log;

import com.android.bips.BuiltInPrintService;
import com.android.bips.R;
import com.android.bips.discovery.DiscoveredPrinter;
import com.android.bips.p2p.P2pUtils;

/**
 * Presents a list of printers and the ability to add a new one
 */
public class AddPrintersFragment extends PreferenceFragment implements ServiceConnection {
    private static final String TAG = AddPrintersFragment.class.getSimpleName();
    private static final boolean DEBUG = false;

    private static final String KEY_ADD_BY_IP = "add_by_ip";
    private static final String KEY_FIND_WIFI_DIRECT = "find_wifi_direct";
    private static final String KEY_SAVED_PRINTERS = "saved_printers";
    private static final int ORDER_SAVED = 2;

    private PreferenceCategory mSavedPrintersCategory;
    private Preference mAddPrinterByIpPreference;
    private BuiltInPrintService mPrintService;

    @Override
    public void onCreate(Bundle in) {
        if (DEBUG) Log.d(TAG, "onCreate");
        super.onCreate(in);
        addPreferencesFromResource(R.xml.add_printers_prefs);
        mAddPrinterByIpPreference = getPreferenceScreen().findPreference(KEY_ADD_BY_IP);

        Preference findP2pPrintersPreference = getPreferenceScreen().findPreference(
                KEY_FIND_WIFI_DIRECT);
        findP2pPrintersPreference.setOnPreferenceClickListener(preference -> {
            getFragmentManager().beginTransaction()
                    .replace(android.R.id.content, new FindP2pPrintersFragment())
                    .addToBackStack(null)
                    .commit();
            return true;
        });

        mSavedPrintersCategory = (PreferenceCategory) getPreferenceScreen()
                .findPreference(KEY_SAVED_PRINTERS);
    }

    @Override
    public void onStart() {
        super.onStart();
        if (DEBUG) Log.d(TAG, "onStart");

        getActivity().setTitle(R.string.title_activity_add_printer);
        getContext().bindService(new Intent(getContext(), BuiltInPrintService.class), this,
                Context.BIND_AUTO_CREATE);
    }

    @Override
    public void onStop() {
        super.onStop();
        if (DEBUG) Log.d(TAG, "onStop");

        getContext().unbindService(this);
    }

    @Override
    public void onServiceConnected(ComponentName componentName, IBinder iBinder) {
        if (DEBUG) Log.d(TAG, "onServiceConnected");
        mPrintService = BuiltInPrintService.getInstance();

        mAddPrinterByIpPreference.setOnPreferenceClickListener(preference -> {
            AlertDialog dialog = new AddManualPrinterDialog(getActivity(),
                    mPrintService.getManualDiscovery());
            dialog.setOnDismissListener(d -> updateSavedPrinters());
            dialog.show();
            return true;
        });

        updateSavedPrinters();
    }

    @Override
    public void onServiceDisconnected(ComponentName componentName) {
        mPrintService = null;
    }

    private void updateSavedPrinters() {
        int savedCount = mPrintService.getDiscovery().getSavedPrinters().size();

        if (savedCount == 0) {
            if (getPreferenceScreen().findPreference(mSavedPrintersCategory.getKey()) != null) {
                getPreferenceScreen().removePreference(mSavedPrintersCategory);
            }
        } else {
            if (getPreferenceScreen().findPreference(mSavedPrintersCategory.getKey()) == null) {
                getPreferenceScreen().addPreference(mSavedPrintersCategory);
            }

            mSavedPrintersCategory.removeAll();

            // With the service enumerate all saved printers
            for (DiscoveredPrinter printer : mPrintService.getDiscovery().getSavedPrinters()) {
                if (DEBUG) Log.d(TAG, "Adding saved printer " + printer);
                PrinterPreference pref = new PrinterPreference(getContext(), mPrintService,
                        printer, false);
                pref.setOrder(ORDER_SAVED);
                pref.setOnPreferenceClickListener(preference -> {
                    showRemovalDialog(printer);
                    return true;
                });
                mSavedPrintersCategory.addPreference(pref);
            }
        }
    }

    private void showRemovalDialog(DiscoveredPrinter printer) {
        String message;
        if (P2pUtils.isP2p(printer)) {
            message = mPrintService.getString(R.string.connects_via_wifi_direct);
        } else {
            message = mPrintService.getString(R.string.connects_via_network, printer.path);
        }
        new AlertDialog.Builder(getContext())
                .setTitle(printer.name)
                .setMessage(message)
                .setNegativeButton(android.R.string.cancel, null)
                .setPositiveButton(R.string.forget, (dialog, which) -> {
                    mPrintService.getDiscovery().removeSavedPrinter(printer.path);
                    updateSavedPrinters();
                }).show();
    }
}
