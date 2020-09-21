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

package com.android.car.settings.security;

import android.annotation.DrawableRes;
import android.annotation.Nullable;
import android.content.Context;
import android.support.annotation.VisibleForTesting;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.GridLayout;
import android.widget.ImageButton;
import android.widget.TextView;

import com.android.car.settings.R;

import java.util.ArrayList;
import java.util.List;

/**
 * A custom view for the PIN pad.
 */
public class PinPadView extends GridLayout {
    // Number of keys in the pin pad, 0-9 plus backspace and enter keys.
    @VisibleForTesting(otherwise = VisibleForTesting.PRIVATE)
    static final int NUM_KEYS = 12;

    @VisibleForTesting(otherwise = VisibleForTesting.PRIVATE)
    static final int[] PIN_PAD_DIGIT_KEYS = { R.id.key0, R.id.key1, R.id.key2, R.id.key3,
            R.id.key4, R.id.key5, R.id.key6, R.id.key7, R.id.key8, R.id.key9 };

    private final List<View> mPinKeys = new ArrayList<>(NUM_KEYS);
    private PinPadClickListener mOnClickListener;
    private ImageButton mEnterKey;

    public PinPadView(Context context) {
        super(context);
        init();
    }

    public PinPadView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    public PinPadView(Context context, @Nullable AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init();
    }

    public PinPadView(Context context, @Nullable AttributeSet attrs, int defStyleAttr,
            int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
        init();
    }

    /**
     * Set the call back for key click.
     *
     * @param pinPadClickListener The call back.
     */
    public void setPinPadClickListener(PinPadClickListener pinPadClickListener) {
        mOnClickListener = pinPadClickListener;
    }

    @Override
    public void setEnabled(boolean enabled) {
        super.setEnabled(enabled);
        for (View key: mPinKeys) {
            key.setEnabled(enabled);
        }
    }

    /**
     * Set the resource Id of the enter key icon.
     *
     * @param drawableId  The resource Id of the drawable.
     */
    public void setEnterKeyIcon(@DrawableRes int drawableId) {
        mEnterKey.setImageResource(drawableId);
    }

    private void init() {
        LayoutInflater inflater = LayoutInflater.from(getContext());
        inflater.inflate(R.layout.pin_pad_view, this, true);

        for (int keyId : PIN_PAD_DIGIT_KEYS) {
            TextView key = (TextView) findViewById(keyId);
            String digit = key.getTag().toString();
            key.setOnClickListener(v -> {
                mOnClickListener.onDigitKeyClick(digit);
            });
            mPinKeys.add(key);
        }

        View backspace = findViewById(R.id.key_backspace);
        backspace.setOnClickListener(v -> mOnClickListener.onBackspaceClick());
        mPinKeys.add(backspace);

        mEnterKey = (ImageButton) findViewById(R.id.key_enter);
        mEnterKey.setOnClickListener(v -> mOnClickListener.onEnterKeyClick());
        mPinKeys.add(mEnterKey);
    }

    /**
     * The call back interface for onClick event in the view.
     */
    public interface PinPadClickListener {
        /**
         * One of the digit key has been clicked.
         *
         * @param digit A String representing a digit between 0 and 9.
         */
        void onDigitKeyClick(String digit);

        /**
         * The backspace key has been clicked.
         */
        void onBackspaceClick();

        /**
         * The enter key has been clicked.
         */
        void onEnterKeyClick();
    }
}
