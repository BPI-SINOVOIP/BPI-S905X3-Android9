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

import static android.autofillservice.cts.common.ShellHelper.runShellCommand;

import static com.google.common.truth.Truth.assertWithMessage;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.platform.test.annotations.AppModeFull;
import android.provider.Settings;
import android.support.test.uiautomator.UiObject2;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;

@AppModeFull // Service-specific test
public class SettingsIntentTest extends AutoFillServiceTestCase {

    private static final int MY_REQUEST_CODE = 42;

    @Rule
    public final AutofillActivityTestRule<TrampolineForResultActivity> mActivityRule =
            new AutofillActivityTestRule<TrampolineForResultActivity>(
                    TrampolineForResultActivity.class);

    protected TrampolineForResultActivity mActivity;

    @Before
    public void setActivity() {
        mActivity = mActivityRule.getActivity();
    }

    @After
    public void killSettings() {
        // Make sure there's no Settings activity left , as it could fail future tests.
        runShellCommand("am force-stop com.android.settings");
    }

    @Test
    public void testMultipleServicesShown() throws Exception {
        disableService();

        // Launches Settings.
        mActivity.startForResult(newSettingsIntent(), MY_REQUEST_CODE);

        // Asserts services are shown.
        mUiBot.assertShownByText(InstrumentedAutoFillService.sServiceLabel);
        mUiBot.assertShownByText(InstrumentedAutoFillServiceCompatMode.sServiceLabel);
        mUiBot.assertShownByText(NoOpAutofillService.SERVICE_LABEL);
        mUiBot.assertNotShowingForSure(BadAutofillService.SERVICE_LABEL);

        // Finishes and asserts result.
        mUiBot.pressBack();
        mActivity.assertResult(Activity.RESULT_CANCELED);
    }

    @Test
    public void testWarningShown_userRejectsByTappingBack() throws Exception {
        disableService();

        // Launches Settings.
        mActivity.startForResult(newSettingsIntent(), MY_REQUEST_CODE);

        // Asserts services are shown.
        final UiObject2 object = mUiBot
                .assertShownByText(InstrumentedAutoFillService.sServiceLabel);
        object.click();

        // TODO(b/79615759): should assert that "autofill_confirmation_message" is shown, but that
        // string belongs to Settings - we need to move it to frameworks/base first (and/or use
        // a resource id, also on framework).
        // So, for now, just asserts the service name is showing again (in the popup), and the other
        // services are not showing (because the popup hides then).

        final UiObject2 msgObj = mUiBot.assertShownById("android:id/message");
        final String msg = msgObj.getText();
        assertWithMessage("Wrong warning message").that(msg)
                .contains(InstrumentedAutoFillService.sServiceLabel);

        // NOTE: assertion below is fine because it looks for the full text, not a substring
        mUiBot.assertNotShowingForSure(InstrumentedAutoFillService.sServiceLabel);
        mUiBot.assertNotShowingForSure(InstrumentedAutoFillServiceCompatMode.sServiceLabel);
        mUiBot.assertNotShowingForSure(NoOpAutofillService.SERVICE_LABEL);
        mUiBot.assertNotShowingForSure(BadAutofillService.SERVICE_LABEL);

        // Finishes and asserts result.
        mUiBot.pressBack();
        mActivity.assertResult(Activity.RESULT_CANCELED);
    }

    // TODO(b/79615759): add testWarningShown_userRejectsByTappingCancel() and
    // testWarningShown_userAccepts() - these tests would require adding the strings and resource
    // ids to frameworks/base

    private Intent newSettingsIntent() {
        return new Intent(Settings.ACTION_REQUEST_SET_AUTOFILL_SERVICE)
                .setFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                .setData(Uri.parse("package:" + Helper.MY_PACKAGE));
    }
}
