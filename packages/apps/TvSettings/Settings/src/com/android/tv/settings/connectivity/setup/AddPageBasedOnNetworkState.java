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

package com.android.tv.settings.connectivity.setup;

import android.arch.lifecycle.ViewModelProviders;
import android.net.wifi.ScanResult;
import android.net.wifi.WifiConfiguration;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentActivity;

import com.android.tv.settings.R;
import com.android.tv.settings.connectivity.WifiConfigHelper;
import com.android.tv.settings.connectivity.util.State;
import com.android.tv.settings.connectivity.util.StateMachine;
import com.android.tv.settings.connectivity.util.WifiSecurityUtil;

/**
 * State responsible for adding page based on network.
 */
public class AddPageBasedOnNetworkState implements State {
    private final StateMachine mStateMachine;
    private final UserChoiceInfo mUserChoiceInfo;
    private final FragmentActivity mActivity;
    private Fragment mFragment;

    public AddPageBasedOnNetworkState(FragmentActivity activity) {
        mActivity = activity;
        mUserChoiceInfo = ViewModelProviders.of(mActivity).get(UserChoiceInfo.class);
        mStateMachine = ViewModelProviders.of(mActivity).get(StateMachine.class);
    }

    @Override
    public void processForward() {
        mFragment = null;
        if (mUserChoiceInfo.choiceChosen(
                getString(R.string.other_network), UserChoiceInfo.SELECT_WIFI)) {
            mUserChoiceInfo.getWifiConfiguration().hiddenSSID = true;
            mStateMachine.getListener().onComplete(StateMachine.OTHER_NETWORK);
        } else {
            ScanResult scanResult = mUserChoiceInfo.getChosenNetwork();
            String chosenNetwork = mUserChoiceInfo.getChosenNetwork().SSID;
            WifiConfiguration prevWifiConfig = mUserChoiceInfo.getWifiConfiguration();
            if (mUserChoiceInfo.getPageSummary(UserChoiceInfo.PASSWORD) != null
                    && (prevWifiConfig == null
                    || !chosenNetwork.equals(prevWifiConfig.getPrintableSsid()))) {
                mUserChoiceInfo.removePageSummary(UserChoiceInfo.PASSWORD);
            }
            int wifiSecurity = WifiSecurityUtil.getSecurity(scanResult);
            WifiConfiguration wifiConfiguration = WifiConfigHelper.getConfiguration(
                    mActivity,
                    scanResult.SSID,
                    wifiSecurity);
            mUserChoiceInfo.setWifiSecurity(wifiSecurity);
            mUserChoiceInfo.setWifiConfiguration(wifiConfiguration);
            if (WifiConfigHelper.isNetworkSaved(wifiConfiguration)) {
                mStateMachine.getListener().onComplete(StateMachine.KNOWN_NETWORK);
            } else {
                mStateMachine.getListener().onComplete(StateMachine.ADD_START);
            }
        }
    }

    private String getString(int id) {
        return mActivity.getString(id);
    }

    @Override
    public void processBackward() {
        mStateMachine.back();
    }

    @Override
    public Fragment getFragment() {
        return mFragment;
    }
}
