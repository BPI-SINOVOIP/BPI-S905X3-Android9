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

import com.android.car.settings.CarSettingsRobolectricTestRunner;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

/**
 * Tests for PasswordHelper class.
 */
@RunWith(CarSettingsRobolectricTestRunner.class)
public class PasswordHelperTest {

    private PasswordHelper mPasswordHelper;
    private PasswordHelper mPinPresenter;

    @Before
    public void initObjects() {
        mPasswordHelper = new PasswordHelper(false);
        mPinPresenter = new PasswordHelper(true);
    }

    /**
     * A test to check validate works as expected for alphanumeric password
     * that are too short.
     */
    @Test
    public void testValidatePasswordTooShort() {
        String password = "lov";
        assertThat(mPasswordHelper.validate(password))
                .isEqualTo(PasswordHelper.DOES_NOT_MATCH_PATTERN);
    }

    /**
     * A test to check validate works as expected for alphanumeric password
     * that are too long.
     */
    @Test
    public void testValidatePasswordTooLong() {
        String password = "passwordtoolong";
        assertThat(mPasswordHelper.validate(password))
                .isEqualTo(PasswordHelper.DOES_NOT_MATCH_PATTERN);
    }

    /**
     * A test to check validate works as expected for alphanumeric password
     * that contains white space.
     */
    @Test
    public void testValidatePasswordWhiteSpace() {
        String password = "pass wd";
        assertThat(mPasswordHelper.validate(password))
                .isEqualTo(PasswordHelper.DOES_NOT_MATCH_PATTERN);
    }

    /**
     * A test to check validate works as expected for alphanumeric password
     * that don't have a digit.
     */
    @Test
    public void testValidatePasswordNoDigit() {
        String password = "password";
        assertThat(mPasswordHelper.validate(password))
                .isEqualTo(PasswordHelper.DOES_NOT_MATCH_PATTERN);
    }

    /**
     * A test to check validate works as expected for alphanumeric password
     * that contains invalid character.
     */
    @Test
    public void testValidatePasswordNonAscii() {
        String password = "1passwýd";
        assertThat(mPasswordHelper.validate(password))
                .isEqualTo(PasswordHelper.CONTAINS_INVALID_CHARACTERS);
    }

    /**
     * A test to check validate works as expected for alphanumeric password
     * that don't have a letter.
     */
    @Test
    public void testValidatePasswordNoLetter() {
        String password = "123456";
        assertThat(mPasswordHelper.validate(password))
                .isEqualTo(PasswordHelper.DOES_NOT_MATCH_PATTERN);
    }

    /**
     * A test to check validate works as expected for pin that contains non digits.
     */
    @Test
    public void testValidatePinContainingNonDigits() {
        String password = "1a34";
        assertThat(mPinPresenter.validate(password))
                .isEqualTo(PasswordHelper.CONTAINS_NON_DIGITS);
    }

    /**
     * A test to check validate works as expected for pin with too few digits
     */
    @Test
    public void testValidatePinWithTooFewDigits() {
        String password = "12";
        assertThat(mPinPresenter.validate(password))
                .isEqualTo(PasswordHelper.TOO_FEW_DIGITS);
    }
}
