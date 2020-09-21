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
package com.android.car.settings.wifi;

import android.net.NetworkInfo.State;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.support.annotation.StringRes;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

import com.android.car.list.SimpleTextLineItem;
import com.android.car.list.TypedPagedListAdapter;
import com.android.car.settings.R;
import com.android.car.settings.common.ListSettingsFragment;
import com.android.car.settings.common.Logger;
import com.android.settingslib.wifi.AccessPoint;

import java.util.ArrayList;

/**
 * Shows details about a wifi network, including actions related to the network,
 * e.g. ignore, disconnect, etc. The intent should include information about
 * access point, use that to render UI, e.g. show SSID etc.
 */
public class WifiDetailFragment extends ListSettingsFragment {
    public static final String EXTRA_AP_STATE = "extra_ap_state";
    private static final Logger LOG = new Logger(WifiDetailFragment.class);

    private AccessPoint mAccessPoint;
    private WifiManager mWifiManager;

    private class ActionFailListener implements WifiManager.ActionListener {
        @StringRes
        private final int mMessageResId;

        public ActionFailListener(@StringRes int messageResId) {
            mMessageResId = messageResId;
        }

        @Override
        public void onSuccess() {
        }

        @Override
        public void onFailure(int reason) {
            Toast.makeText(getContext(),
                    R.string.wifi_failed_connect_message,
                    Toast.LENGTH_SHORT).show();
        }
    }

    public static WifiDetailFragment getInstance(AccessPoint accessPoint) {
        WifiDetailFragment wifiDetailFragment = new WifiDetailFragment();
        Bundle bundle = ListSettingsFragment.getBundle();
        bundle.putInt(EXTRA_TITLE_ID, R.string.wifi_settings);
        bundle.putInt(EXTRA_ACTION_BAR_LAYOUT, R.layout.action_bar_with_button);
        Bundle accessPointState = new Bundle();
        accessPoint.saveWifiState(accessPointState);
        bundle.putBundle(EXTRA_AP_STATE, accessPointState);
        wifiDetailFragment.setArguments(bundle);
        return wifiDetailFragment;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mAccessPoint = new AccessPoint(getContext(), getArguments().getBundle(EXTRA_AP_STATE));
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        mWifiManager = getContext().getSystemService(WifiManager.class);

        super.onActivityCreated(savedInstanceState);
        ((TextView) getActivity().findViewById(R.id.title)).setText(mAccessPoint.getSsid());
        Button forgetButton = (Button) getActivity().findViewById(R.id.action_button1);
        forgetButton.setText(R.string.forget);
        forgetButton.setOnClickListener(v -> {
            forget();
            getFragmentController().goBack();
        });

        if (mAccessPoint.isSaved() && !mAccessPoint.isActive()) {
            Button connectButton = (Button) getActivity().findViewById(R.id.action_button2);
            connectButton.setVisibility(View.VISIBLE);
            connectButton.setText(R.string.wifi_setup_connect);
            connectButton.setOnClickListener(v -> {
                mWifiManager.connect(mAccessPoint.getConfig(),
                        new ActionFailListener(R.string.wifi_failed_connect_message));
                getFragmentController().goBack();
            });
        }
    }

    @Override
    public ArrayList<TypedPagedListAdapter.LineItem> getLineItems() {
        ArrayList<TypedPagedListAdapter.LineItem> lineItems = new ArrayList<>();
        lineItems.add(
                new SimpleTextLineItem(getText(R.string.wifi_status), mAccessPoint.getSummary()));
        lineItems.add(
                new SimpleTextLineItem(getText(R.string.wifi_signal), getSignalString()));
        lineItems.add(new SimpleTextLineItem(getText(R.string.wifi_security),
                mAccessPoint.getSecurityString(/* concise= */ true)));
        return lineItems;
    }

    private String getSignalString() {
        String[] signalStrings = getResources().getStringArray(R.array.wifi_signals);

        int level = WifiManager.calculateSignalLevel(
                mAccessPoint.getRssi(), signalStrings.length);
        return signalStrings[level];
    }

    private void forget() {
        if (!mAccessPoint.isSaved()) {
            if (mAccessPoint.getNetworkInfo() != null &&
                    mAccessPoint.getNetworkInfo().getState() != State.DISCONNECTED) {
                // Network is active but has no network ID - must be ephemeral.
                mWifiManager.disableEphemeralNetwork(
                        AccessPoint.convertToQuotedString(mAccessPoint.getSsidStr()));
            } else {
                // Should not happen, but a monkey seems to trigger it
                LOG.e("Failed to forget invalid network " + mAccessPoint.getConfig());
                return;
            }
        } else {
            mWifiManager.forget(mAccessPoint.getConfig().networkId,
                    new ActionFailListener(R.string.wifi_failed_forget_message));
        }
    }
}
