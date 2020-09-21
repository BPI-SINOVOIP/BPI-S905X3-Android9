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

import android.app.Activity;
import android.arch.lifecycle.ViewModelProviders;
import android.content.Context;
import android.os.Bundle;
import android.support.v17.leanback.widget.GuidanceStylist;
import android.support.v17.leanback.widget.GuidedAction;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentActivity;

import com.android.tv.settings.R;
import com.android.tv.settings.connectivity.util.State;
import com.android.tv.settings.connectivity.util.StateMachine;

import java.util.List;

/**
 * State responsible for displaying summary of connected Wi-Fi.
 */
public class SummaryConnectedWifiState implements State {
    private final FragmentActivity mActivity;
    private Fragment mFragment;

    public SummaryConnectedWifiState(FragmentActivity activity) {
        mActivity = activity;
    }

    @Override
    public void processForward() {
        mFragment = new SummaryConnectWifiFragment();
        FragmentChangeListener listener = (FragmentChangeListener) mActivity;
        if (listener != null) {
            listener.onFragmentChange(mFragment, true);
        }
    }

    @Override
    public void processBackward() {
        mFragment = new SummaryConnectWifiFragment();
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
     * Fragment that shows the Wi-Fi is connected.
     */
    public static class SummaryConnectWifiFragment extends WifiConnectivityGuidedStepFragment {
        private static final int DO_NOT_CHANGE_NETWORK = 100003;
        private static final int WIFI_ACTION_CHANGE_NETWORK = 100004;
        private StateMachine mStateMachine;
        private UserChoiceInfo mUserChoiceInfo;

        @Override
        public void onCreateActions(List<GuidedAction> actions, Bundle savedInstanceState) {
            Context context = getActivity();
            actions.add(new GuidedAction.Builder(context)
                    .title(R.string.wifi_setup_action_dont_change_network)
                    .id(DO_NOT_CHANGE_NETWORK)
                    .build());
            actions.add(new GuidedAction.Builder(context)
                    .title(R.string.wifi_setup_action_change_network)
                    .id(WIFI_ACTION_CHANGE_NETWORK)
                    .build());
        }

        @Override
        public GuidanceStylist.Guidance onCreateGuidance(Bundle savedInstanceState) {
            String title = getString(
                    R.string.wifi_setup_summary_title_connected,
                    mUserChoiceInfo.getConnectedNetwork());
            return new GuidanceStylist.Guidance(
                    title,
                    null,
                    null,
                    null);
        }

        @Override
        public void onCreate(Bundle savedInstanceState) {
            mUserChoiceInfo = ViewModelProviders
                    .of(getActivity())
                    .get(UserChoiceInfo.class);
            mStateMachine = ViewModelProviders
                    .of(getActivity())
                    .get(StateMachine.class);
            super.onCreate(savedInstanceState);
        }

        @Override
        public void onGuidedActionClicked(GuidedAction action) {
            long id = action.getId();
            if (id == DO_NOT_CHANGE_NETWORK) {
                mStateMachine.finish(Activity.RESULT_OK);
            } else if (id == WIFI_ACTION_CHANGE_NETWORK) {
                mStateMachine.getListener().onComplete(StateMachine.SELECT_WIFI);
            }
        }
    }
}
