/*
 * Copyright (C) 2018 Google Inc.
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

package com.android.car.settingslib.log;

import com.android.car.settingslib.robolectric.CarSettingsLibRobolectricTestRunner;

import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.ExpectedException;
import org.junit.runner.RunWith;

/**
 * Tests {@link LoggerBase}
 */
@RunWith(CarSettingsLibRobolectricTestRunner.class)
public class LoggerBaseTest {

    @Rule
    public ExpectedException mExpectedException = ExpectedException.none();

    @Test
    public void testNullTag() {
        mExpectedException.expect(IllegalStateException.class);
        mExpectedException.expectMessage("Tag must be not null or empty");
        new NullTagLogger("NullLogger");
    }

    @Test
    public void testEmptyTag() {
        mExpectedException.expect(IllegalStateException.class);
        mExpectedException.expectMessage("Tag must be not null or empty");
        new EmptyTagLogger("EmptyTagLogger");
    }

    @Test
    public void testTooLongTag() {
        mExpectedException.expect(IllegalStateException.class);
        mExpectedException.expectMessage("Tag must be 23 characters or less");
        new TooLongTagLogger("TooLongTagLogger");
    }

    private class NullTagLogger extends LoggerBase {

        NullTagLogger(String prefix) {
            super(prefix);
        }

        @Override
        protected String getTag() {
            return null;
        }
    }

    private class EmptyTagLogger extends LoggerBase {

        EmptyTagLogger(String prefix) {
            super(prefix);
        }

        @Override
        protected String getTag() {
            return "";
        }
    }

    /**
     * A Logger with a tag that is 24 characters long.
     */
    private class TooLongTagLogger extends LoggerBase {

        TooLongTagLogger(String prefix) {
            super(prefix);
        }

        @Override
        protected String getTag() {
            return "LoremIpsumLoremIpsumLore";
        }
    }
}
