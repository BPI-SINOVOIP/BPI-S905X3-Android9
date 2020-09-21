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

import android.app.Activity;
import android.arch.lifecycle.ViewModelProviders;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentActivity;

import com.android.tv.settings.R;
import com.android.tv.settings.connectivity.setup.MessageFragment;
import com.android.tv.settings.connectivity.util.State;
import com.android.tv.settings.connectivity.util.StateMachine;

/**
 * State responsible for showing the save success page.
 */
public class SaveSuccessState implements State {
    private final FragmentActivity mActivity;
    private Fragment mFragment;

    public SaveSuccessState(FragmentActivity activity) {
        mActivity = activity;
    }

    @Override
    public void processForward() {
        mFragment = SaveSuccessFragment.newInstance(
                mActivity.getString(R.string.wifi_setup_save_success));
        State.FragmentChangeListener listener = (FragmentChangeListener) mActivity;
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
     * Fragment that shows network is successfully connected.
     */
    public static class SaveSuccessFragment extends MessageFragment {
        private static final int MSG_TIME_OUT = 1;
        private static final int TIME_OUT_MS = 3 * 1000;
        private static final String KEY_TIME_OUT_DURATION = "time_out_duration";
        private Handler mTimeoutHandler;

        /**
         * Get the fragment based on the title.
         *
         * @param title title of the fragment.
         * @return the fragment.
         */
        public static SaveSuccessFragment newInstance(String title) {
            SaveSuccessFragment fragment = new SaveSuccessFragment();
            Bundle args = new Bundle();
            addArguments(args, title, false);
            fragment.setArguments(args);
            return fragment;
        }

        @Override
        public void onCreate(Bundle savedInstanceState) {
            StateMachine stateMachine = ViewModelProviders
                    .of(getActivity())
                    .get(StateMachine.class);
            mTimeoutHandler = new Handler() {
                @Override
                public void handleMessage(Message msg) {
                    switch (msg.what) {
                        case MSG_TIME_OUT:
                            stateMachine.finish(Activity.RESULT_OK);
                            break;
                        default:
                            break;
                    }
                }
            };
            super.onCreate(savedInstanceState);
        }

        @Override
        public void onResume() {
            super.onResume();
            mTimeoutHandler.sendEmptyMessageDelayed(MSG_TIME_OUT,
                    getArguments().getInt(KEY_TIME_OUT_DURATION, TIME_OUT_MS));
        }

        @Override
        public void onPause() {
            super.onPause();
            mTimeoutHandler.removeMessages(MSG_TIME_OUT);
        }
    }
}
