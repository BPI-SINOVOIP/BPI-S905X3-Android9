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
import android.support.annotation.NonNull;
import android.support.v17.leanback.widget.GuidanceStylist;
import android.support.v17.leanback.widget.GuidedAction;
import android.support.v17.leanback.widget.GuidedActionsStylist;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentActivity;
import android.view.View;

import com.android.tv.settings.R;
import com.android.tv.settings.connectivity.util.State;
import com.android.tv.settings.connectivity.util.StateMachine;

import java.util.List;

/**
 * State responsible for entering proxy hostname.
 */
public class ProxyHostNameState implements State {
    private final FragmentActivity mActivity;
    private Fragment mFragment;

    public ProxyHostNameState(FragmentActivity activity) {
        mActivity = activity;
    }

    @Override
    public void processForward() {
        mFragment = new ProxyHostNameFragment();
        FragmentChangeListener listener = (FragmentChangeListener) mActivity;
        listener.onFragmentChange(mFragment, true);
    }

    @Override
    public void processBackward() {
        mFragment = new ProxyHostNameFragment();
        FragmentChangeListener listener = (FragmentChangeListener) mActivity;
        listener.onFragmentChange(mFragment, false);
    }

    @Override
    public Fragment getFragment() {
        return mFragment;
    }

    /**
     * Fragment that needs user to enter proxy hostname.
     */
    public static class ProxyHostNameFragment extends WifiConnectivityGuidedStepFragment {
        private StateMachine mStateMachine;
        private AdvancedOptionsFlowInfo mAdvancedOptionsFlowInfo;
        private GuidedAction mAction;

        @NonNull
        @Override
        public GuidanceStylist.Guidance onCreateGuidance(Bundle savedInstanceState) {
            String title = getString(R.string.title_wifi_proxy_hostname);
            return new GuidanceStylist.Guidance(
                    title,
                    getString(R.string.proxy_hostname_description),
                    null,
                    null);
        }

        @Override
        public GuidedActionsStylist onCreateActionsStylist() {
            return new GuidedActionsStylist() {
                @Override
                public int onProvideItemLayoutId() {
                    return R.layout.setup_text_input_item;
                }
            };
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
        public void onCreateActions(@NonNull List<GuidedAction> actions,
                Bundle savedInstanceState) {
            String title = getString(R.string.proxy_hostname_hint);
            if (mAdvancedOptionsFlowInfo.containsPage(AdvancedOptionsFlowInfo.PROXY_HOSTNAME)) {
                title = mAdvancedOptionsFlowInfo.get(AdvancedOptionsFlowInfo.PROXY_HOSTNAME);
            }  else if (mAdvancedOptionsFlowInfo.getInitialProxyInfo() != null) {
                title = mAdvancedOptionsFlowInfo.getInitialProxyInfo().getHost();
            }

            Context context = getActivity();
            mAction = new GuidedAction.Builder(context)
                    .title(title)
                    .editable(true)
                    .id(GuidedAction.ACTION_ID_CONTINUE)
                    .build();
            actions.add(mAction);
        }

        @Override
        public void onViewCreated(@NonNull View view, Bundle savedInstanceState) {
            openInEditMode(mAction);
        }

        @Override
        public long onGuidedActionEditedAndProceed(GuidedAction action) {
            if (action.getId() == GuidedAction.ACTION_ID_CONTINUE) {
                mAdvancedOptionsFlowInfo.put(AdvancedOptionsFlowInfo.PROXY_HOSTNAME,
                        action.getTitle());
                mStateMachine.getListener().onComplete(StateMachine.CONTINUE);
            }
            return action.getId();
        }
    }
}

