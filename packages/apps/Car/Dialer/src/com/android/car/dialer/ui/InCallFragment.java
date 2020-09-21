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
package com.android.car.dialer.ui;

import static android.telecom.Call.STATE_RINGING;

import android.os.Bundle;
import android.os.Handler;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.telecom.Call;
import android.text.TextUtils;
import android.text.format.DateUtils;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import com.android.car.dialer.CallListener;
import com.android.car.dialer.DialerFragment;
import com.android.car.dialer.R;
import com.android.car.dialer.telecom.TelecomUtils;
import com.android.car.dialer.telecom.UiCall;
import com.android.car.dialer.telecom.UiCallManager;

/**
 * A fragment that displays information about an on-going call with options to hang up.
 */
public class InCallFragment extends Fragment implements
        OnGoingCallControllerBarFragment.OnGoingCallControllerBarCallback, CallListener {

    private Fragment mDialerFragment;
    private View mUserProfileContainerView;
    private View mDialerFragmentContainer;
    private TextView mUserProfileBodyText;

    private Handler mHandler = new Handler();
    private CharSequence mCallInfoLabel;

    public static InCallFragment newInstance() {
        return new InCallFragment();
    }

    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        View fragmentView = inflater.inflate(R.layout.in_call_fragment, container, false);
        mUserProfileContainerView = fragmentView.findViewById(R.id.user_profile_container);
        mDialerFragmentContainer = fragmentView.findViewById(R.id.dialer_container);
        mUserProfileBodyText = mUserProfileContainerView.findViewById(R.id.body);
        mDialerFragment = new DialerFragment();

        updateControllerBarFragment(UiCallManager.get().getPrimaryCall().getState());
        bindUserProfileView(fragmentView.findViewById(R.id.user_profile_container));
        return fragmentView;
    }

    @Override
    public void onPause() {
        super.onPause();
        mHandler.removeCallbacks(mUpdateDurationRunnable);
    }

    @Override
    public void onOpenDialpad() {
        mDialerFragment = new DialerFragment();
        getChildFragmentManager().beginTransaction()
                .replace(R.id.dialer_container, mDialerFragment)
                .commit();
        mDialerFragmentContainer.setVisibility(View.VISIBLE);
        mUserProfileContainerView.setVisibility(View.GONE);
    }

    @Override
    public void onCloseDialpad() {
        getFragmentManager().beginTransaction()
                .remove(mDialerFragment)
                .commit();
        mDialerFragmentContainer.setVisibility(View.GONE);
        mUserProfileContainerView.setVisibility(View.VISIBLE);
    }

    private void bindUserProfileView(View container) {
        UiCall primaryCall = UiCallManager.get().getPrimaryCall();
        if (primaryCall == null) {
            return;
        }
        String number = primaryCall.getNumber();
        String displayName = TelecomUtils.getDisplayName(getContext(), primaryCall);

        TextView nameView = container.findViewById(R.id.title);
        nameView.setText(displayName);

        ImageView avatar = container.findViewById(R.id.avatar);
        TelecomUtils.setContactBitmapAsync(getContext(), avatar, displayName, number);

        mCallInfoLabel = TelecomUtils.getTypeFromNumber(getContext(), primaryCall.getNumber());
    }

    private void updateControllerBarFragment(int callState) {
        Fragment controllerBarFragment;
        if (callState == Call.STATE_RINGING) {
            controllerBarFragment = RingingCallControllerBarFragment.newInstance();
        } else {
            controllerBarFragment = OnGoingCallControllerBarFragment.newInstance();
        }

        getChildFragmentManager().beginTransaction()
                .replace(R.id.controller_bar_container, controllerBarFragment)
                .commit();
    }

    @Override
    public void onAudioStateChanged(boolean isMuted, int route, int supportedRouteMask) {

    }

    @Override
    public void onCallStateChanged(UiCall call, int state) {
        int callState = call.getState();
        switch (callState) {
            case Call.STATE_NEW:
            case Call.STATE_CONNECTING:
            case Call.STATE_DIALING:
            case Call.STATE_SELECT_PHONE_ACCOUNT:
            case Call.STATE_HOLDING:
            case Call.STATE_DISCONNECTED:
                mHandler.removeCallbacks(mUpdateDurationRunnable);
                updateBody(call);
                break;
            case Call.STATE_ACTIVE:
                mHandler.post(mUpdateDurationRunnable);
                updateControllerBarFragment(call.getState());
                break;
        }
    }

    @Override
    public void onCallUpdated(UiCall call) {

    }

    @Override
    public void onCallAdded(UiCall call) {

    }

    @Override
    public void onCallRemoved(UiCall call) {

    }

    private final Runnable mUpdateDurationRunnable = new Runnable() {
        @Override
        public void run() {
            UiCall primaryCall = UiCallManager.get().getPrimaryCall();
            if (primaryCall.getState() != Call.STATE_ACTIVE) {
                return;
            }
            updateBody(primaryCall);
            mHandler.postDelayed(this /* runnable */, DateUtils.SECOND_IN_MILLIS);
        }
    };

    private void updateBody(UiCall primaryCall) {
        String callInfoText = TelecomUtils.getCallInfoText(getContext(),
                primaryCall, mCallInfoLabel);
        mUserProfileBodyText.setText(callInfoText);
        mUserProfileBodyText.setVisibility(
                TextUtils.isEmpty(callInfoText) ? View.GONE : View.VISIBLE);
    }
}
