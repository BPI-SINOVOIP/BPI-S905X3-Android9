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

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.Mockito.verify;
import static org.mockito.MockitoAnnotations.initMocks;

import android.view.View;

import com.android.car.settings.CarSettingsRobolectricTestRunner;
import com.android.car.settings.R;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.robolectric.RuntimeEnvironment;

import java.util.Arrays;

@RunWith(CarSettingsRobolectricTestRunner.class)
public class PinPadViewTest {

    private static int[] sAllKeys =
            Arrays.copyOf(PinPadView.PIN_PAD_DIGIT_KEYS, PinPadView.NUM_KEYS);
    static {
        sAllKeys[PinPadView.PIN_PAD_DIGIT_KEYS.length] = R.id.key_backspace;
        sAllKeys[PinPadView.PIN_PAD_DIGIT_KEYS.length + 1] = R.id.key_enter;
    }

    private PinPadView mPinPadView;

    @Mock
    private PinPadView.PinPadClickListener mClickListener;

    @Before
    public void initPinPad() {
        initMocks(this);
        mPinPadView = new PinPadView(RuntimeEnvironment.application);
        mPinPadView.setPinPadClickListener(mClickListener);
    }

    // Verify that when the pin pad is enabled or disabled, all the keys are in the same state.
    @Test
    public void testEnableDisablePinPad() {
        mPinPadView.setEnabled(false);

        for (int id : sAllKeys) {
            View key = mPinPadView.findViewById(id);
            assertThat(key.isEnabled()).isFalse();
        }

        mPinPadView.setEnabled(true);

        for (int id : sAllKeys) {
            View key = mPinPadView.findViewById(id);
            assertThat(key.isEnabled()).isTrue();
        }
    }

    // Verify that the click handler is called when the backspace key is clicked.
    @Test
    public void testBackspaceClickHandler() {
        mPinPadView.findViewById(R.id.key_backspace).performClick();

        verify(mClickListener).onBackspaceClick();
    }

    // Verify that the click handler is called when the enter key is clicked.
    @Test
    public void testEnterKeyClickHandler() {
        mPinPadView.findViewById(R.id.key_enter).performClick();

        verify(mClickListener).onEnterKeyClick();
    }

    // Verify that the click handler is called with the right argument when a digit key is clicked.
    @Test
    public void testDigitKeyClickHandler() {
        for (int i = 0; i < PinPadView.PIN_PAD_DIGIT_KEYS.length; ++i) {
            mPinPadView.findViewById(PinPadView.PIN_PAD_DIGIT_KEYS[i]).performClick();
            verify(mClickListener).onDigitKeyClick(String.valueOf(i));
        }
    }
}
