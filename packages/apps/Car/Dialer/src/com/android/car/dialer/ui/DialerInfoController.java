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

import android.content.Context;
import android.telecom.Call;
import android.text.TextUtils;
import android.view.View;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.TextView;

import com.android.car.apps.common.FabDrawable;
import com.android.car.dialer.R;
import com.android.car.dialer.telecom.TelecomUtils;
import com.android.car.dialer.telecom.UiCall;
import com.android.car.dialer.telecom.UiCallManager;

/**
 * Controls dialer information such as dialed number and shows proper action based on current call
 * state.
 */
public class DialerInfoController {
    private static final int MAX_DIAL_NUMBER = 20;

    private TextView mTitleView;
    private TextView mBodyView;

    private ImageButton mCallButton;
    private ImageButton mDeleteButton;

    private ImageButton mEndCallButton;
    private ImageButton mMuteButton;

    private Context mContext;

    private final StringBuffer mNumber = new StringBuffer(MAX_DIAL_NUMBER);

    public DialerInfoController(Context context, View container) {
        mContext = context;
        init(container);
    }

    public View init(View container) {
        mTitleView = container.findViewById(R.id.title);
        mBodyView = container.findViewById(R.id.body);
        mCallButton = container.findViewById(R.id.call_button);
        mDeleteButton = container.findViewById(R.id.delete_button);
        mEndCallButton = container.findViewById(R.id.end_call_button);
        mMuteButton = container.findViewById(R.id.mute_button);

        FabDrawable answerCallDrawable = new FabDrawable(mContext);
        answerCallDrawable.setFabAndStrokeColor(mContext.getColor(R.color.phone_call));
        mCallButton.setBackground(answerCallDrawable);
        mCallButton.setOnClickListener((unusedView) -> {
            if (!TextUtils.isEmpty(mNumber.toString())) {
                UiCallManager.get().safePlaceCall(mNumber.toString(), false);
            }
        });
        mDeleteButton.setOnClickListener(v -> {
            removeLastDigit();
        });
        mDeleteButton.setOnLongClickListener(v -> {
            // Clear all on long-press
            clearDialedNumber();
            return true;
        });

        updateView();

        return container;
    }

    /**
     * Append more number to the end of dialed number.
     */
    public void appendDialedNumber(String number) {
        if (mNumber.length() < MAX_DIAL_NUMBER) {
            mNumber.append(number);
            mTitleView.setText(getFormattedNumber(mNumber.toString()));
        }
    }

    /**
     * Remove last digit of the dialed number. If there's no number left to delete, there's no
     * operation to be take.
     */
    public void removeLastDigit() {
        if (mNumber.length() != 0) {
            mNumber.deleteCharAt(mNumber.length() - 1);
            mTitleView.setText(getFormattedNumber(mNumber.toString()));
        }
        UiCall primaryCall = UiCallManager.get().getPrimaryCall();

        if (mNumber.length() == 0 && primaryCall != null
                && primaryCall.getState() != Call.STATE_ACTIVE) {
            mTitleView.setText(R.string.dial_a_number);
        }
    }

    private void updateView() {
        UiCall onGoingCall = UiCallManager.get().getPrimaryCall();
        if (onGoingCall == null) {
            showPreDialUi();
        } else if (onGoingCall.getState() == Call.STATE_CONNECTING) {
            showDialingUi(onGoingCall);
        } else if (onGoingCall.getState() == Call.STATE_ACTIVE) {
            showInCallUi();
        }
    }

    private void showPreDialUi() {
        mCallButton.setVisibility(View.VISIBLE);
        mDeleteButton.setVisibility(View.VISIBLE);

        mEndCallButton.setVisibility(View.GONE);
        mMuteButton.setVisibility(View.GONE);
    }

    private void showDialingUi(UiCall uiCall) {
        if (mTitleView.getText().equals(mContext.getString(R.string.dial_a_number))) {
            mTitleView.setText("");
        }
        FabDrawable endCallDrawable = new FabDrawable(mContext);
        endCallDrawable.setFabAndStrokeColor(mContext.getColor(R.color.phone_end_call));
        mEndCallButton.setBackground(endCallDrawable);
        mEndCallButton.setVisibility(View.VISIBLE);
        mMuteButton.setVisibility(View.VISIBLE);
        mBodyView.setVisibility(View.VISIBLE);

        mDeleteButton.setVisibility(View.GONE);
        mCallButton.setVisibility(View.GONE);
        bindUserProfileView(uiCall);
    }

    private void showInCallUi() {
        if (mTitleView.getText().equals(mContext.getString(R.string.dial_a_number))) {
            mTitleView.setText("");
        }
        mEndCallButton.setVisibility(View.GONE);
        mDeleteButton.setVisibility(View.GONE);
        mCallButton.setVisibility(View.GONE);
    }

    private String getFormattedNumber(String number) {
        return TelecomUtils.getFormattedNumber(mContext, number);
    }

    private void clearDialedNumber() {
        mNumber.setLength(0);
        mTitleView.setText(getFormattedNumber(mNumber.toString()));
    }

    private void bindUserProfileView(UiCall primaryCall) {
        if (primaryCall == null) {
            return;
        }
        mTitleView.setText(TelecomUtils.getDisplayName(mContext, primaryCall));
    }
}
