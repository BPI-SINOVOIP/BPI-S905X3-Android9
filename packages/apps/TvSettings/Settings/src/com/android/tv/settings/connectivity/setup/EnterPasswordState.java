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
import android.net.wifi.WifiConfiguration;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.v17.leanback.widget.GuidanceStylist;
import android.support.v17.leanback.widget.GuidedAction;
import android.support.v17.leanback.widget.GuidedActionsStylist;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentActivity;
import android.text.InputType;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CheckBox;
import android.widget.EditText;

import com.android.settingslib.wifi.AccessPoint;
import com.android.tv.settings.R;
import com.android.tv.settings.connectivity.util.GuidedActionsAlignUtil;
import com.android.tv.settings.connectivity.util.State;
import com.android.tv.settings.connectivity.util.StateMachine;

import java.util.List;

/**
 * State responsible for entering the password.
 */
public class EnterPasswordState implements State {
    private final FragmentActivity mActivity;
    private Fragment mFragment;

    public EnterPasswordState(FragmentActivity activity) {
        mActivity = activity;
    }

    @Override
    public void processForward() {
        mFragment = new EnterPasswordFragment();
        FragmentChangeListener listener = (FragmentChangeListener) mActivity;
        listener.onFragmentChange(mFragment, true);
    }

    @Override
    public void processBackward() {
        mFragment = new EnterPasswordFragment();
        FragmentChangeListener listener = (FragmentChangeListener) mActivity;
        listener.onFragmentChange(mFragment, false);
    }

    @Override
    public Fragment getFragment() {
        return mFragment;
    }

    /**
     * Fragment that enters password of the Wi-Fi.
     */
    public static class EnterPasswordFragment extends WifiConnectivityGuidedStepFragment {
        private static final int PSK_MIN_LENGTH = 8;
        private static final int WEP_MIN_LENGTH = 5;
        private UserChoiceInfo mUserChoiceInfo;
        private StateMachine mStateMachine;
        private EditText mTextInput;
        private CheckBox mCheckBox;

        @NonNull
        @Override
        public GuidanceStylist.Guidance onCreateGuidance(Bundle savedInstanceState) {
            String title = getString(
                    R.string.wifi_setup_input_password,
                    mUserChoiceInfo.getWifiConfiguration().getPrintableSsid());
            return new GuidanceStylist.Guidance(
                    title,
                    null,
                    null,
                    null);
        }

        @Override
        public GuidedActionsStylist onCreateActionsStylist() {
            return new GuidedActionsStylist() {
                @Override
                public ViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
                    LayoutInflater inflater = LayoutInflater.from(parent.getContext());
                    View v = inflater.inflate(onProvideItemLayoutId(viewType), parent, false);
                    return new PasswordViewHolder(v);
                }

                @Override
                public void onBindViewHolder(ViewHolder vh, GuidedAction action) {
                    super.onBindViewHolder(vh, action);
                    if (action.getId() == GuidedAction.ACTION_ID_CONTINUE) {
                        PasswordViewHolder viewHolder = (PasswordViewHolder) vh;
                        mTextInput = (EditText) viewHolder.getTitleView();
                        mCheckBox = viewHolder.mCheckbox;
                        mCheckBox.setOnClickListener(view -> {
                            updatePasswordInputObfuscation();
                            EnterPasswordFragment.this.openInEditMode(action);
                        });
                        mCheckBox.setChecked(mUserChoiceInfo.isPasswordHidden());
                        updatePasswordInputObfuscation();
                        openInEditMode(action);
                    }
                }

                @Override
                public int onProvideItemLayoutId() {
                    return R.layout.setup_password_item;
                }
            };
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
        public void onCreateActions(@NonNull List<GuidedAction> actions,
                Bundle savedInstanceState) {
            Context context = getActivity();
            CharSequence prevPassword = mUserChoiceInfo.getPageSummary(UserChoiceInfo.PASSWORD);
            boolean isPasswordHidden = mUserChoiceInfo.isPasswordHidden();
            actions.add(new GuidedAction.Builder(context)
                    .title(prevPassword == null ? "" : prevPassword)
                    .editInputType(InputType.TYPE_CLASS_TEXT
                            | (isPasswordHidden
                            ? InputType.TYPE_TEXT_VARIATION_PASSWORD
                            : InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD))
                    .id(GuidedAction.ACTION_ID_CONTINUE)
                    .editable(true)
                    .build());
        }

        @Override
        public long onGuidedActionEditedAndProceed(GuidedAction action) {
            if (action.getId() == GuidedAction.ACTION_ID_CONTINUE) {
                String password = action.getTitle().toString();
                if (password.length() >= WEP_MIN_LENGTH) {
                    mUserChoiceInfo.put(UserChoiceInfo.PASSWORD, action.getTitle().toString());
                    mUserChoiceInfo.setPasswordHidden(mCheckBox.isChecked());
                    setWifiConfigurationPassword(password);
                    mStateMachine.getListener().onComplete(StateMachine.OPTIONS_OR_CONNECT);
                }
            }
            return action.getId();
        }

        private void updatePasswordInputObfuscation() {
            mTextInput.setInputType(InputType.TYPE_CLASS_TEXT
                    | (mCheckBox.isChecked()
                    ? InputType.TYPE_TEXT_VARIATION_PASSWORD
                    : InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD));
        }

        private void setWifiConfigurationPassword(String password) {
            int wifiSecurity = mUserChoiceInfo.getWifiSecurity();
            WifiConfiguration wifiConfiguration = mUserChoiceInfo.getWifiConfiguration();
            if (wifiSecurity == AccessPoint.SECURITY_WEP) {
                int length = password.length();
                // WEP-40, WEP-104, and 256-bit WEP (WEP-232?)
                if ((length == 10 || length == 26 || length == 32 || length == 58)
                        && password.matches("[0-9A-Fa-f]*")) {
                    wifiConfiguration.wepKeys[0] = password;
                } else if (length == 5 || length == 13 || length == 16 || length == 29) {
                    wifiConfiguration.wepKeys[0] = '"' + password + '"';
                }
            } else {
                if (wifiSecurity == AccessPoint.SECURITY_PSK
                        && password.length() < PSK_MIN_LENGTH) {
                    return;
                }
                if (password.matches("[0-9A-Fa-f]{64}")) {
                    wifiConfiguration.preSharedKey = password;
                } else {
                    wifiConfiguration.preSharedKey = '"' + password + '"';
                }
            }
        }

        private static class PasswordViewHolder extends GuidedActionsAlignUtil.SetupViewHolder {
            CheckBox mCheckbox;

            PasswordViewHolder(View v) {
                super(v);
                mCheckbox = v.findViewById(R.id.password_checkbox);
            }
        }
    }
}
