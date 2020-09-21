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

import android.media.AudioManager;
import android.media.ToneGenerator;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.util.SparseArray;
import android.util.SparseIntArray;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;

import com.android.car.dialer.R;
import com.android.car.dialer.telecom.UiCallManager;

/**
 * Dialpad Fragment which displays a dialpad.
 */
public class DialpadFragment extends Fragment {
    private static final SparseIntArray sToneMap = new SparseIntArray();
    private static final SparseArray<String> sDialValueMap = new SparseArray<>();
    private static final SparseArray<Integer> sRIdMap = new SparseArray<>();

    private static final int TONE_LENGTH_INFINITE = -1;
    private static final int TONE_RELATIVE_VOLUME = 80;

    static {
        sToneMap.put(KeyEvent.KEYCODE_1, ToneGenerator.TONE_DTMF_1);
        sToneMap.put(KeyEvent.KEYCODE_2, ToneGenerator.TONE_DTMF_2);
        sToneMap.put(KeyEvent.KEYCODE_3, ToneGenerator.TONE_DTMF_3);
        sToneMap.put(KeyEvent.KEYCODE_4, ToneGenerator.TONE_DTMF_4);
        sToneMap.put(KeyEvent.KEYCODE_5, ToneGenerator.TONE_DTMF_5);
        sToneMap.put(KeyEvent.KEYCODE_6, ToneGenerator.TONE_DTMF_6);
        sToneMap.put(KeyEvent.KEYCODE_7, ToneGenerator.TONE_DTMF_7);
        sToneMap.put(KeyEvent.KEYCODE_8, ToneGenerator.TONE_DTMF_8);
        sToneMap.put(KeyEvent.KEYCODE_9, ToneGenerator.TONE_DTMF_9);
        sToneMap.put(KeyEvent.KEYCODE_0, ToneGenerator.TONE_DTMF_0);
        sToneMap.put(KeyEvent.KEYCODE_STAR, ToneGenerator.TONE_DTMF_S);
        sToneMap.put(KeyEvent.KEYCODE_POUND, ToneGenerator.TONE_DTMF_P);

        sDialValueMap.put(KeyEvent.KEYCODE_1, "1");
        sDialValueMap.put(KeyEvent.KEYCODE_2, "2");
        sDialValueMap.put(KeyEvent.KEYCODE_3, "3");
        sDialValueMap.put(KeyEvent.KEYCODE_4, "4");
        sDialValueMap.put(KeyEvent.KEYCODE_5, "5");
        sDialValueMap.put(KeyEvent.KEYCODE_6, "6");
        sDialValueMap.put(KeyEvent.KEYCODE_7, "7");
        sDialValueMap.put(KeyEvent.KEYCODE_8, "8");
        sDialValueMap.put(KeyEvent.KEYCODE_9, "9");
        sDialValueMap.put(KeyEvent.KEYCODE_0, "0");
        sDialValueMap.put(KeyEvent.KEYCODE_STAR, "*");
        sDialValueMap.put(KeyEvent.KEYCODE_POUND, "#");

        sRIdMap.put(KeyEvent.KEYCODE_1, R.id.one);
        sRIdMap.put(KeyEvent.KEYCODE_2, R.id.two);
        sRIdMap.put(KeyEvent.KEYCODE_3, R.id.three);
        sRIdMap.put(KeyEvent.KEYCODE_4, R.id.four);
        sRIdMap.put(KeyEvent.KEYCODE_5, R.id.five);
        sRIdMap.put(KeyEvent.KEYCODE_6, R.id.six);
        sRIdMap.put(KeyEvent.KEYCODE_7, R.id.seven);
        sRIdMap.put(KeyEvent.KEYCODE_8, R.id.eight);
        sRIdMap.put(KeyEvent.KEYCODE_9, R.id.nine);
        sRIdMap.put(KeyEvent.KEYCODE_0, R.id.zero);
        sRIdMap.put(KeyEvent.KEYCODE_STAR, R.id.star);
        sRIdMap.put(KeyEvent.KEYCODE_POUND, R.id.pound);
    }

    public static DialpadFragment newInstance() {
        return new DialpadFragment();
    }

    /**
     * Callback for dialpad to interact with its host.
     */
    public interface DialpadCallback {
        /**
         * Called when voice mail should be dialed.
         */
        void onDialVoiceMail();

        /**
         * Called when a digit should be append.
         */
        void onAppendDigit(String digit);
    }

    private ToneGenerator mToneGenerator;
    private DialpadCallback mDialpadCallback;

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mToneGenerator = new ToneGenerator(AudioManager.STREAM_MUSIC, TONE_RELATIVE_VOLUME);
    }

    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        if (getParentFragment() != null && getParentFragment() instanceof DialpadCallback) {
            mDialpadCallback = (DialpadCallback) getParentFragment();
        } else if (getHost() instanceof DialpadCallback) {
            mDialpadCallback = (DialpadCallback) getHost();
        }

        View dialpadView = inflater.inflate(R.layout.dialpad, container, false);
        setupKeypadClickListeners(dialpadView);
        return dialpadView;
    }

    @Override
    public void onPause() {
        super.onPause();
        stopTone();
    }

    /**
     * The click listener for all dialpad buttons.  Reacts to touch-down and touch-up events, as
     * well as long-press for certain keys.  Mimics the behavior of the phone dialer app.
     */
    private class DialpadClickListener implements View.OnTouchListener,
            View.OnLongClickListener {
        private final int mTone;
        private final String mValue;

        DialpadClickListener(int keyCode) {
            mTone = sToneMap.get(keyCode);
            mValue = sDialValueMap.get(keyCode);
        }

        @Override
        public boolean onLongClick(View v) {
            switch (mValue) {
                case "0":
                    if (mDialpadCallback != null) {
                        mDialpadCallback.onAppendDigit("+");
                    }
                    stopTone();
                    return true;
                case "1":
                    // TODO: this currently does not work (at least over bluetooth HFP), because
                    // the framework is unable to get the voicemail number. Revisit later...
                    if (mDialpadCallback != null) {
                        mDialpadCallback.onDialVoiceMail();
                    }
                    return true;
                default:
                    return false;
            }
        }

        @Override
        public boolean onTouch(View v, MotionEvent event) {
            UiCallManager uiCallmanager = UiCallManager.get();
            boolean hasActiveCall = uiCallmanager.getPrimaryCall() != null;
            if (event.getAction() == MotionEvent.ACTION_DOWN) {
                if (mDialpadCallback != null) {
                    mDialpadCallback.onAppendDigit(mValue);
                }
                if (hasActiveCall) {
                    uiCallmanager.playDtmfTone(uiCallmanager.getPrimaryCall(), mValue.charAt(0));
                } else {
                    playTone(mTone);
                }
            } else if (event.getAction() == MotionEvent.ACTION_UP) {
                if (hasActiveCall) {
                    uiCallmanager.stopDtmfTone(uiCallmanager.getPrimaryCall());
                } else {
                    stopTone();
                }
            }

            // Continue propagating the touch event
            return false;
        }
    }

    private void playTone(int tone) {
        if (mToneGenerator == null) {
            return;
        }

        // Start the new tone
        mToneGenerator.startTone(tone, TONE_LENGTH_INFINITE);
    }

    private void stopTone() {
        if (mToneGenerator == null) {
            return;
        }

        mToneGenerator.stopTone();
    }

    private void setupKeypadClickListeners(View parent) {
        for (int i = 0; i < sRIdMap.size(); i++) {
            int key = sRIdMap.keyAt(i);
            DialpadClickListener clickListener = new DialpadClickListener(key);
            View v = parent.findViewById(sRIdMap.get(key));
            v.setOnTouchListener(clickListener);
            v.setOnLongClickListener(clickListener);
        }
    }
}
