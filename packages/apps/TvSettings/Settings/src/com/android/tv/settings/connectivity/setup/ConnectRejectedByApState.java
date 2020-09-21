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
 * State responsible for handling the case when connection is rejected by AP.
 */
public class ConnectRejectedByApState implements State {
    private final FragmentActivity mActivity;
    private Fragment mFragment;

    public ConnectRejectedByApState(FragmentActivity activity) {
        mActivity = activity;
    }

    @Override
    public void processForward() {
        mFragment = new ConnectRejectedByApFragment();
        FragmentChangeListener listener = (FragmentChangeListener) mActivity;
        if (listener != null) {
            listener.onFragmentChange(mFragment, true);
        }
    }

    @Override
    public void processBackward() {
        mFragment = new ConnectRejectedByApFragment();
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
     * Fragment that shows wifi could not connect AP.
     */
    public static class ConnectRejectedByApFragment extends WifiConnectivityGuidedStepFragment {
        private static final int ACTION_ID_TRY_AGAIN = 100001;
        private static final int ACTION_ID_VIEW_AVAILABLE_NETWORK = 100002;
        private StateMachine mStateMachine;
        private UserChoiceInfo mUserChoiceInfo;

        @Override
        public void onCreateActions(List<GuidedAction> actions, Bundle savedInstanceState) {
            Context context = getActivity();
            actions.add(new GuidedAction.Builder(context)
                    .title(R.string.wifi_action_try_again)
                    .id(ACTION_ID_TRY_AGAIN)
                    .build());
            actions.add(new GuidedAction.Builder(context)
                    .title(R.string.wifi_action_view_available_networks)
                    .id(ACTION_ID_VIEW_AVAILABLE_NETWORK)
                    .build());
        }

        @Override
        public GuidanceStylist.Guidance onCreateGuidance(Bundle savedInstanceState) {
            String title = getString(R.string.title_wifi_could_not_connect_ap_reject);
            return new GuidanceStylist.Guidance(title, null, null, null);
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
            if (action.getId() == ACTION_ID_TRY_AGAIN) {
                mStateMachine.getListener()
                        .onComplete(StateMachine.TRY_AGAIN);
            } else if (action.getId() == ACTION_ID_VIEW_AVAILABLE_NETWORK) {
                mUserChoiceInfo.removePageSummary(UserChoiceInfo.SELECT_WIFI);
                mUserChoiceInfo.removePageSummary(UserChoiceInfo.PASSWORD);
                mUserChoiceInfo.removePageSummary(UserChoiceInfo.SECURITY);
                mStateMachine.getListener()
                        .onComplete(StateMachine.SELECT_WIFI);
            }
        }
    }
}
