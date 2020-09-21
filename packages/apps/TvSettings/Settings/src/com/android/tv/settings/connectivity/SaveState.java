/*
 * Copyright (C) 2018 The Android Open Source Project
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

package com.android.tv.settings.connectivity;

import android.arch.lifecycle.ViewModelProviders;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentActivity;

import com.android.tv.settings.R;
import com.android.tv.settings.connectivity.setup.AdvancedOptionsFlowInfo;
import com.android.tv.settings.connectivity.setup.MessageFragment;
import com.android.tv.settings.connectivity.util.State;
import com.android.tv.settings.connectivity.util.StateMachine;

/**
 * State responsible for showing the save page.
 */
public class SaveState implements State {
    private final FragmentActivity mActivity;
    private Fragment mFragment;

    public SaveState(FragmentActivity activity) {
        mActivity = activity;
    }

    @Override
    public void processForward() {
        EditSettingsInfo editSettingsInfo =
                    ViewModelProviders.of(mActivity).get(EditSettingsInfo.class);
        NetworkConfiguration networkConfig = editSettingsInfo.getNetworkConfiguration();
        AdvancedOptionsFlowInfo advInfo =
                    ViewModelProviders.of(mActivity).get(AdvancedOptionsFlowInfo.class);
        networkConfig.setIpConfiguration(advInfo.getIpConfiguration());
        mFragment = SaveWifiConfigurationFragment.newInstance(
                    mActivity.getString(R.string.wifi_saving, networkConfig.getPrintableName()));
        FragmentChangeListener listener = (FragmentChangeListener) mActivity;
        listener.onFragmentChange(mFragment, true);
    }

    @Override
    public void processBackward() {
        StateMachine stateMachine = ViewModelProviders.of(mActivity).get(StateMachine.class);
        stateMachine.back();
    }

    @Override
    public Fragment getFragment() {
        return mFragment;
    }

    /**
     * Saves a wifi network's configuration.
     */
    public static class SaveWifiConfigurationFragment extends MessageFragment
            implements WifiManager.ActionListener {

        public static SaveWifiConfigurationFragment newInstance(String title) {
            SaveWifiConfigurationFragment
                    fragment = new SaveWifiConfigurationFragment();
            Bundle args = new Bundle();
            addArguments(args, title, true);
            fragment.setArguments(args);
            return fragment;
        }

        @Override
        public void onCreate(Bundle icicle) {
            super.onCreate(icicle);
            EditSettingsInfo editSettingsInfo =
                    ViewModelProviders.of(getActivity()).get(EditSettingsInfo.class);
            NetworkConfiguration configuration = editSettingsInfo.getNetworkConfiguration();
            configuration.save(this);
        }

        @Override
        public void onSuccess() {
            FragmentActivity activity = getActivity();
            if (activity != null) {
                StateMachine sm = ViewModelProviders
                        .of(activity).get(StateMachine.class);
                sm.getListener().onComplete(StateMachine.RESULT_SUCCESS);
            }
        }

        @Override
        public void onFailure(int reason) {
            FragmentActivity activity = getActivity();
            if (activity != null) {
                StateMachine sm = ViewModelProviders
                        .of(activity).get(StateMachine.class);
                sm.getListener().onComplete(StateMachine.RESULT_FAILURE);
            }
        }
    }
}
