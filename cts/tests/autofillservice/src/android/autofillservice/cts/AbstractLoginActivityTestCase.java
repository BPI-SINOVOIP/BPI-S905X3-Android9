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

package android.autofillservice.cts;

import android.view.View;

import org.junit.Before;
import org.junit.Rule;

import java.util.concurrent.TimeoutException;

/**
 * Base class for test cases using {@link LoginActivity}.
 */
abstract class AbstractLoginActivityTestCase extends AutoFillServiceTestCase {
    @Rule
    public final AutofillActivityTestRule<LoginActivity> mActivityRule =
            new AutofillActivityTestRule<LoginActivity>(LoginActivity.class);

    @Rule
    public final AutofillActivityTestRule<CheckoutActivity> mCheckoutActivityRule =
            new AutofillActivityTestRule<CheckoutActivity>(CheckoutActivity.class, false);

    protected LoginActivity mActivity;

    @Before
    public void setActivity() {
        mActivity = mActivityRule.getActivity();
    }

    /**
     * Requests focus on username and expect Window event happens.
     */
    protected void requestFocusOnUsername() throws TimeoutException {
        mUiBot.waitForWindowChange(() -> mActivity.onUsername(View::requestFocus),
                Timeouts.UI_TIMEOUT.getMaxValue());
    }

    /**
     * Requests focus on username and expect no Window event happens.
     */
    protected void requestFocusOnUsernameNoWindowChange() {
        try {
            // TODO: define a small value in Timeout
            mUiBot.waitForWindowChange(() -> mActivity.onUsername(View::requestFocus),
                    Timeouts.UI_TIMEOUT.ms());
        } catch (TimeoutException ex) {
            // no window events! looking good
            return;
        }
        throw new IllegalStateException("Expect no window event when focusing to"
                + " username, but event happened");
    }

    /**
     * Requests focus on password and expect Window event happens.
     */
    protected void requestFocusOnPassword() throws TimeoutException {
        mUiBot.waitForWindowChange(() -> mActivity.onPassword(View::requestFocus),
                Timeouts.UI_TIMEOUT.getMaxValue());
    }

}
