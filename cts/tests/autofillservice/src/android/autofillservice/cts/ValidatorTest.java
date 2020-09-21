/*
 * Copyright (C) 2017 The Android Open Source Project
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

import static android.autofillservice.cts.Helper.ID_PASSWORD;
import static android.autofillservice.cts.Helper.ID_USERNAME;
import static android.service.autofill.SaveInfo.SAVE_DATA_TYPE_GENERIC;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;

import android.platform.test.annotations.AppModeFull;
import android.service.autofill.InternalValidator;
import android.service.autofill.LuhnChecksumValidator;
import android.service.autofill.ValueFinder;
import android.view.View;
import android.view.autofill.AutofillId;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;

/**
 * Simple integration test to verify that the UI is only shown if the validator passes.
 */
@AppModeFull // Service-specific test
public class ValidatorTest extends AutoFillServiceTestCase {
    @Rule
    public final AutofillActivityTestRule<LoginActivity> mActivityRule =
        new AutofillActivityTestRule<>(LoginActivity.class);

    private LoginActivity mActivity;

    @Before
    public void setActivity() {
        mActivity = mActivityRule.getActivity();
    }

    @Test
    public void testShowUiWhenValidatorPass() throws Exception {
        integrationTest(true);
    }

    @Test
    public void testDontShowUiWhenValidatorFails() throws Exception {
        integrationTest(false);
    }

    private void integrationTest(boolean willSaveBeShown) throws Exception {
        enableService();

        final AutofillId usernameId = mActivity.getUsername().getAutofillId();

        final String username = willSaveBeShown ? "7992739871-3" : "4815162342-108";
        final LuhnChecksumValidator validator = new LuhnChecksumValidator(usernameId);
        // Sanity check to make sure the validator is properly configured
        assertValidator(validator, usernameId, username, willSaveBeShown);

        // Set response
        sReplier.addResponse(new CannedFillResponse.Builder()
                .setRequiredSavableIds(SAVE_DATA_TYPE_GENERIC, ID_USERNAME, ID_PASSWORD)
                .setValidator(validator)
                .build());

        // Trigger auto-fill
        mActivity.onPassword(View::requestFocus);

        // Wait for onFill() before proceeding.
        sReplier.getNextFillRequest();

        // Trigger save.
        mActivity.onUsername((v) -> v.setText(username));
        mActivity.onPassword((v) -> v.setText("pass"));
        mActivity.tapLogin();

        if (willSaveBeShown) {
            mUiBot.saveForAutofill(true, SAVE_DATA_TYPE_GENERIC);
            sReplier.getNextSaveRequest();
        } else {
            mUiBot.assertSaveNotShowing(SAVE_DATA_TYPE_GENERIC);
        }
    }

    private void assertValidator(InternalValidator validator, AutofillId id, String text,
            boolean valid) {
        final ValueFinder valueFinder = mock(ValueFinder.class);
        doReturn(text).when(valueFinder).findByAutofillId(id);
        assertThat(validator.isValid(valueFinder)).isEqualTo(valid);
    }
}
