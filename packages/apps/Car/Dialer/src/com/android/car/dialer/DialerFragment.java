/*
 * Copyright (C) 2015 The Android Open Source Project
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
package com.android.car.dialer;

import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.android.car.dialer.log.L;
import com.android.car.dialer.telecom.UiCallManager;
import com.android.car.dialer.ui.DialerInfoController;
import com.android.car.dialer.ui.DialpadFragment;

/**
 * Fragment that controls the dialpad.
 */
public class DialerFragment extends Fragment implements DialpadFragment.DialpadCallback {
    private static final String TAG = "Em.DialerFragment";

    private static final String DIAL_NUMBER_KEY = "DIAL_NUMBER_KEY";
    private static final String PLUS_DIGIT = "+";

    private DialerInfoController mDialerInfoController;
    private L logger = L.logger(TAG);

    /**
     * Creates a new instance of the {@link DialerFragment} and display the given number as the one
     * to dial.
     */
    static DialerFragment newInstance(@Nullable String dialNumber) {
        DialerFragment fragment = new DialerFragment();

        if (!TextUtils.isEmpty(dialNumber)) {
            Bundle args = new Bundle();
            args.putString(DIAL_NUMBER_KEY, dialNumber);
            fragment.setArguments(args);
        }

        return fragment;
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        logger.d("onCreateView");
        View view = inflater.inflate(R.layout.dialer_fragment, container, false);

        Fragment dialpadFragment = DialpadFragment.newInstance();
        getChildFragmentManager().beginTransaction()
                .replace(R.id.dialpad_fragment_container, dialpadFragment)
                .commit();

        View dialerInfoContainer = view.findViewById(R.id.dialer_info_fragment_container);
        mDialerInfoController = new DialerInfoController(getContext(), dialerInfoContainer);

        if (getArguments() != null) {
            mDialerInfoController.appendDialedNumber(getArguments().getString(DIAL_NUMBER_KEY));
        }

        return view;
    }

    @Override
    public void onDialVoiceMail() {
        UiCallManager.get().callVoicemail();
    }

    @Override
    public void onAppendDigit(String digit) {
        if (PLUS_DIGIT.equals(digit)) {
            mDialerInfoController.removeLastDigit();
        }
        mDialerInfoController.appendDialedNumber(digit);
    }
}
