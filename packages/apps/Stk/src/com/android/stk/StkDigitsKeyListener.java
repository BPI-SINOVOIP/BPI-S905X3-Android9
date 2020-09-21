/*
 * Copyright (C) 2006 The Android Open Source Project
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

package com.android.stk;

import android.text.method.NumberKeyListener;
import android.view.KeyEvent;
import android.view.inputmethod.EditorInfo;

/**
 * For entering dates in a text field.
 */
public class StkDigitsKeyListener extends NumberKeyListener {
    @Override
    protected char[] getAcceptedChars() {
        return CHARACTERS;
    }

    public int getInputType() {
        return EditorInfo.TYPE_CLASS_PHONE;
    }

    public static StkDigitsKeyListener getInstance() {
        if (sInstance != null) {
            return sInstance;
        }
        sInstance = new StkDigitsKeyListener();
        return sInstance;
    }

    /**
     * The characters that are used.
     *
     * @see KeyEvent#getMatch
     * @see #getAcceptedChars
     */
    public static final char[] CHARACTERS = new char[] {
            '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '*', '#', '+'};

    private static StkDigitsKeyListener sInstance;
}
