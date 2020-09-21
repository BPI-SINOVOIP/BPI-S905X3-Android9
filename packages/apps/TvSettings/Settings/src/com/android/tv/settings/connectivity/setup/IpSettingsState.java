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
import android.content.Context;
import android.net.IpConfiguration;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.v17.leanback.widget.GuidanceStylist;
import android.support.v17.leanback.widget.GuidedAction;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentActivity;
import android.text.TextUtils;
import android.view.View;

import com.android.tv.settings.R;
import com.android.tv.settings.connectivity.util.AdvancedOptionsFlowUtil;
import com.android.tv.settings.connectivity.util.State;
import com.android.tv.settings.connectivity.util.StateMachine;

import java.util.List;

/**
 * State responsible for choosing IP settings.
 */
public class IpSettingsState implements State {
    private final FragmentActivity mActivity;
    private Fragment mFragment;

    public IpSettingsState(FragmentActivity activity) {
        mActivity = activity;
    }

    @Override
    public void processForward() {
        mFragment = new IpSettingsFragment();
        FragmentChangeListener listener = (FragmentChangeListener) mActivity;
        if (listener != null) {
            listener.onFragmentChange(mFragment, true);
        }
    }

    @Override
    public void processBackward() {
        mFragment = new IpSettingsFragment();
        FragmentChangeListener listener = (FragmentChangeListener) mActivity;
        if (listener != null) {
            listener.onFragmentChange(mFragment, false);
        }
    }


    @Override
    public Fragment getFragment() {
        return mFragment;
    }

    /**
     * Fragment that makes user determine IP settings.
     */
    public static class IpSettingsFragment extends WifiConnectivityGuidedStepFragment {
        private static final int WIFI_ACTION_DHCP = 100001;
        private static final int WIFI_ACTION_STATIC = 100002;
        private StateMachine mStateMachine;
        private AdvancedOptionsFlowInfo mAdvancedOptionsFlowInfo;

        @NonNull
        @Override
        public GuidanceStylist.Guidance onCreateGuidance(Bundle savedInstanceState) {
            return new GuidanceStylist.Guidance(
                    getString(R.string.title_wifi_ip_settings),
                    null,
                    null,
                    null);
        }

        @Override
        public void onCreate(Bundle savedInstanceState) {
            mAdvancedOptionsFlowInfo = ViewModelProviders
                    .of(getActivity())
                    .get(AdvancedOptionsFlowInfo.class);
            mStateMachine = ViewModelProviders
                    .of(getActivity())
                    .get(StateMachine.class);
            super.onCreate(savedInstanceState);
        }

        @Override
        public void onViewCreated(@NonNull View view, Bundle savedInstanceState) {
            super.onViewCreated(view, savedInstanceState);
            CharSequence title;
            if (mAdvancedOptionsFlowInfo.containsPage(AdvancedOptionsFlowInfo.IP_SETTINGS)) {
                title = mAdvancedOptionsFlowInfo.get(AdvancedOptionsFlowInfo.IP_SETTINGS);
            } else if (mAdvancedOptionsFlowInfo.getIpConfiguration().getIpAssignment()
                        == IpConfiguration.IpAssignment.STATIC) {
                title = getString(R.string.wifi_action_static);
            } else {
                title = getString(R.string.wifi_action_dhcp);
            }
            moveToPosition(title);
        }

        private void moveToPosition(CharSequence title) {
            if (title == null) return;
            for (int i = 0; i < getActions().size(); i++) {
                if (TextUtils.equals(getActions().get(i).getTitle(), title)) {
                    setSelectedActionPosition(i);
                    break;
                }
            }
        }

        @Override
        public void onCreateActions(@NonNull List<GuidedAction> actions,
                Bundle savedInstanceState) {
            Context context = getActivity();
            actions.add(new GuidedAction.Builder(context)
                    .title(getString(R.string.wifi_action_dhcp))
                    .id(WIFI_ACTION_DHCP)
                    .build());
            actions.add(new GuidedAction.Builder(context)
                    .title(getString(R.string.wifi_action_static))
                    .id(WIFI_ACTION_STATIC)
                    .build());
        }

        @Override
        public void onGuidedActionClicked(GuidedAction action) {
            mAdvancedOptionsFlowInfo.put(AdvancedOptionsFlowInfo.IP_SETTINGS, action.getTitle());
            if (action.getId() == WIFI_ACTION_DHCP) {
                AdvancedOptionsFlowUtil.processIpSettings(getActivity());
                mStateMachine.getListener().onComplete(StateMachine.ADVANCED_FLOW_COMPLETE);
            } else if (action.getId() == WIFI_ACTION_STATIC) {
                mStateMachine.getListener().onComplete(StateMachine.CONTINUE);
            }
        }
    }
}
