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

import android.app.admin.DevicePolicyManager;
import android.app.admin.PasswordMetrics;
import android.content.Context;

import com.android.car.settings.R;

import java.util.LinkedList;
import java.util.List;

/**
 * Presenter used by ChooseLockPinPasswordFragment
 */
public class PasswordHelper {
    /**
     * Password must contain at least one number, one letter, can not have white space, should be
     * between 4 to 8 characters.
     */
    private static final String PASSWORD_PATTERN = "^(?=.*[0-9])(?=.*[a-zA-Z])(?=\\S+$).{4,8}$";

    /**
     * Allow non-control Latin-1 characters only.
     */
    private static final String VALID_CHAR_PATTERN = "^[\\x20-\\x7F]*$";

    private static final int MIN_PIN_LENGTH = 4;

    // Error code returned from validate(String).
    static final int NO_ERROR = 0;
    static final int CONTAINS_INVALID_CHARACTERS = 1;
    static final int DOES_NOT_MATCH_PATTERN = 1 << 1;
    static final int CONTAINS_NON_DIGITS = 1 << 2;
    static final int CONTAINS_SEQUENTIAL_DIGITS = 1 << 3;
    static final int TOO_FEW_DIGITS = 1 << 4;

    private final boolean mIsPin;

    PasswordHelper(boolean isPin) {
        mIsPin = isPin;
    }

    /**
     * Returns one of the password quality values defined in {@link DevicePolicyManager}, such
     * as NUMERIC, ALPHANUMERIC etc.
     */
    public int getPasswordQuality() {
        return mIsPin ? DevicePolicyManager.PASSWORD_QUALITY_NUMERIC :
                DevicePolicyManager.PASSWORD_QUALITY_ALPHANUMERIC;
    }

    /**
     * Validates PIN/Password and returns the validation result.
     *
     * @param password the raw password the user typed in
     * @return the error code which should be non-zero where there is error. Otherwise
     * {@link #NO_ERROR} should be returned.
     */
    public int validate(String password) {
        return mIsPin ? validatePin(password) : validatePassword(password);
    }

    /**
     * Converts error code from validatePassword to an array of messages describing the errors with
     * important message comes first.  The messages are concatenated with a space in between.
     * Please make sure each message ends with a period.
     * @param errorCode the code returned by {@link #validatePassword(String) validatePassword}
     */
    public List<String> convertErrorCodeToMessages(Context context, int errorCode) {
        return mIsPin ? convertPinErrorCodeToMessages(context, errorCode) :
                convertPasswordErrorCodeToMessages(context, errorCode);
    }

    private int validatePassword(String password) {
        int errorCode = NO_ERROR;

        if (!password.matches(VALID_CHAR_PATTERN)) {
            errorCode |= CONTAINS_INVALID_CHARACTERS;
        }

        if (!password.matches(PASSWORD_PATTERN)) {
            errorCode |= DOES_NOT_MATCH_PATTERN;
        }

        return errorCode;
    }

    private int validatePin(String pin) {
        int errorCode = NO_ERROR;

        PasswordMetrics metrics = PasswordMetrics.computeForPassword(pin);
        int passwordQuality = getPasswordQuality();

        if (metrics.length > 0 && metrics.length < MIN_PIN_LENGTH) {
            errorCode |= TOO_FEW_DIGITS;
        }

        if (metrics.letters > 0 || metrics.symbols > 0) {
            errorCode |= CONTAINS_NON_DIGITS;
        }

        if (passwordQuality == DevicePolicyManager.PASSWORD_QUALITY_NUMERIC_COMPLEX) {
            // Check for repeated characters or sequences (e.g. '1234', '0000', '2468')
            int sequence = PasswordMetrics.maxLengthSequence(pin);
            if (sequence > PasswordMetrics.MAX_ALLOWED_SEQUENCE) {
                errorCode |= CONTAINS_SEQUENTIAL_DIGITS;
            }
        }

        return errorCode;
    }

    private List<String> convertPasswordErrorCodeToMessages(Context context, int errorCode) {
        List<String> messages = new LinkedList<>();

        if ((errorCode & CONTAINS_INVALID_CHARACTERS) > 0) {
            messages.add(context.getString(R.string.lockpassword_illegal_character));
        }

        if ((errorCode & DOES_NOT_MATCH_PATTERN) > 0) {
            messages.add(context.getString(R.string.lockpassword_invalid_password));
        }

        return messages;
    }

    private List<String> convertPinErrorCodeToMessages(Context context, int errorCode) {
        List<String> messages = new LinkedList<>();

        if ((errorCode & CONTAINS_NON_DIGITS) > 0) {
            messages.add(context.getString(R.string.lockpassword_pin_contains_non_digits));
        }

        if ((errorCode & CONTAINS_SEQUENTIAL_DIGITS) > 0) {
            messages.add(context.getString(R.string.lockpassword_pin_no_sequential_digits));
        }

        if ((errorCode & TOO_FEW_DIGITS) > 0) {
            messages.add(context.getString(R.string.lockpin_invalid_pin));
        }

        return messages;
    }
}
