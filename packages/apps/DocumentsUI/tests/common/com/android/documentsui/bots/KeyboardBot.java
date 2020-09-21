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

package com.android.documentsui.bots;

import static android.support.test.espresso.Espresso.onView;
import static android.support.test.espresso.action.ViewActions.pressImeActionButton;
import static android.support.test.espresso.matcher.ViewMatchers.isAssignableFrom;

import android.content.Context;
import android.support.test.uiautomator.UiDevice;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;

/**
 * A test helper class that provides support for keyboard manipulation.
 */
public class KeyboardBot extends Bots.BaseBot {

    public KeyboardBot(UiDevice device, Context context, int timeout) {
        super(device, context, timeout);
    }

    public void dismissKeyboardIfPresent() {
        if(isKeyboardPresent()) {
            mDevice.pressBack();
        }
    }

    // Indirect way to detect the keyboard.
    private boolean isKeyboardPresent() {
        InputMethodManager inputManager = (InputMethodManager) mContext
                .getSystemService(Context.INPUT_METHOD_SERVICE);
        return inputManager.isAcceptingText();
    }

    public void pressEnter() {
      //TODO: There seems to be a bug on N/Espresso that makes pressing Enter not work
      // This is a temporary workaround that somehow works
      // See b/28399576
        onView(isAssignableFrom(EditText.class)).perform(pressImeActionButton());
    }

    public void pressKey(int keyCode) {
        mDevice.pressKeyCode(keyCode);
    }

    public void pressKey(int keyCode, int metaState) {
        mDevice.pressKeyCode(keyCode, metaState);
    }
}
