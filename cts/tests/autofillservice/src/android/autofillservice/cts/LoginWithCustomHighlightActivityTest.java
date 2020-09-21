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

import static android.autofillservice.cts.Helper.ID_USERNAME;

import static com.google.common.truth.Truth.assertThat;

import android.autofillservice.cts.CannedFillResponse.CannedDataset;
import android.graphics.Rect;
import android.support.test.uiautomator.UiObject2;
import android.view.View;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;

import java.util.concurrent.TimeoutException;

public class LoginWithCustomHighlightActivityTest extends AutoFillServiceTestCase {

    @Rule
    public final AutofillActivityTestRule<LoginWithCustomHighlightActivity> mActivityRule =
            new AutofillActivityTestRule<LoginWithCustomHighlightActivity>(
            LoginWithCustomHighlightActivity.class);

    private LoginWithCustomHighlightActivity mActivity;

    @Before
    public void setActivity() {
        mActivity = mActivityRule.getActivity();
    }

    @Test
    public void testAutofillCustomHighligth() throws Exception {
        // Set service.
        enableService();

        // Set expectations.
        final CannedFillResponse.Builder builder = new CannedFillResponse.Builder()
                .addDataset(new CannedDataset.Builder()
                        .setField(ID_USERNAME, "dude")
                        .setPresentation(createPresentation("The Dude"))
                        .build());
        sReplier.addResponse(builder.build());
        mActivity.expectAutoFill("dude");

        // Dynamically set password to make sure it's sanitized.
        mActivity.onPassword((v) -> v.setText("I AM GROOT"));

        // Trigger auto-fill.
        requestFocusOnUsername();

        // Auto-fill it.
        final UiObject2 picker = mUiBot.assertDatasets("The Dude");
        sReplier.getNextFillRequest();

        mUiBot.selectDataset(picker, "The Dude");

        // Check the results.
        mActivity.assertAutoFilled();

        final Rect bounds = MyDrawable.getAutofilledBounds();
        assertThat(bounds).isNotNull();
        assertThat(bounds.right).isEqualTo(mActivity.getUsername().getWidth());
        assertThat(bounds.bottom).isEqualTo(mActivity.getUsername().getHeight());
    }

    /**
     * Requests focus on username and expect Window event happens.
     */
    protected void requestFocusOnUsername() throws TimeoutException {
        mUiBot.waitForWindowChange(() -> mActivity.onUsername(View::requestFocus),
                Timeouts.UI_TIMEOUT.getMaxValue());
    }
}
