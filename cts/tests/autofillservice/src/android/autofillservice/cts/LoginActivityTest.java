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

import static android.autofillservice.cts.CannedFillResponse.DO_NOT_REPLY_RESPONSE;
import static android.autofillservice.cts.CannedFillResponse.NO_RESPONSE;
import static android.autofillservice.cts.Helper.ID_PASSWORD;
import static android.autofillservice.cts.Helper.ID_PASSWORD_LABEL;
import static android.autofillservice.cts.Helper.ID_USERNAME;
import static android.autofillservice.cts.Helper.ID_USERNAME_LABEL;
import static android.autofillservice.cts.Helper.assertHasFlags;
import static android.autofillservice.cts.Helper.assertNumberOfChildren;
import static android.autofillservice.cts.Helper.assertTextAndValue;
import static android.autofillservice.cts.Helper.assertTextIsSanitized;
import static android.autofillservice.cts.Helper.assertTextOnly;
import static android.autofillservice.cts.Helper.assertValue;
import static android.autofillservice.cts.Helper.dumpStructure;
import static android.autofillservice.cts.Helper.findNodeByResourceId;
import static android.autofillservice.cts.Helper.isAutofillWindowFullScreen;
import static android.autofillservice.cts.Helper.setUserComplete;
import static android.autofillservice.cts.InstrumentedAutoFillService.SERVICE_CLASS;
import static android.autofillservice.cts.InstrumentedAutoFillService.SERVICE_PACKAGE;
import static android.autofillservice.cts.InstrumentedAutoFillService.waitUntilConnected;
import static android.autofillservice.cts.InstrumentedAutoFillService.waitUntilDisconnected;
import static android.autofillservice.cts.LoginActivity.AUTHENTICATION_MESSAGE;
import static android.autofillservice.cts.LoginActivity.BACKDOOR_USERNAME;
import static android.autofillservice.cts.LoginActivity.ID_USERNAME_CONTAINER;
import static android.autofillservice.cts.LoginActivity.getWelcomeMessage;
import static android.autofillservice.cts.common.ShellHelper.runShellCommand;
import static android.autofillservice.cts.common.ShellHelper.tap;
import static android.service.autofill.FillRequest.FLAG_MANUAL_REQUEST;
import static android.service.autofill.SaveInfo.SAVE_DATA_TYPE_ADDRESS;
import static android.service.autofill.SaveInfo.SAVE_DATA_TYPE_CREDIT_CARD;
import static android.service.autofill.SaveInfo.SAVE_DATA_TYPE_EMAIL_ADDRESS;
import static android.service.autofill.SaveInfo.SAVE_DATA_TYPE_GENERIC;
import static android.service.autofill.SaveInfo.SAVE_DATA_TYPE_PASSWORD;
import static android.service.autofill.SaveInfo.SAVE_DATA_TYPE_USERNAME;
import static android.text.InputType.TYPE_NULL;
import static android.text.InputType.TYPE_TEXT_VARIATION_PASSWORD;
import static android.view.View.IMPORTANT_FOR_AUTOFILL_NO;
import static android.view.WindowManager.LayoutParams.FLAG_SECURE;

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;

import android.app.PendingIntent;
import android.app.assist.AssistStructure.ViewNode;
import android.autofillservice.cts.CannedFillResponse.CannedDataset;
import android.autofillservice.cts.InstrumentedAutoFillService.FillRequest;
import android.autofillservice.cts.InstrumentedAutoFillService.SaveRequest;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.IntentSender;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.Rect;
import android.os.Bundle;
import android.os.SystemClock;
import android.platform.test.annotations.AppModeFull;
import android.service.autofill.SaveInfo;
import android.support.test.uiautomator.UiObject2;
import android.util.Log;
import android.view.View;
import android.view.View.AccessibilityDelegate;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.view.accessibility.AccessibilityNodeInfo;
import android.view.accessibility.AccessibilityNodeProvider;
import android.view.autofill.AutofillManager;
import android.widget.RemoteViews;

import org.junit.Test;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * This is the test case covering most scenarios - other test cases will cover characteristics
 * specific to that test's activity (for example, custom views).
 */
public class LoginActivityTest extends AbstractLoginActivityTestCase {

    private static final String TAG = "LoginActivityTest";

    @Test
    public void testAutoFillNoDatasets() throws Exception {
        // Set service.
        enableService();

        // Set expectations.
        sReplier.addResponse(NO_RESPONSE);

        // Trigger autofill.
        mActivity.onUsername(View::requestFocus);

        // Make sure a fill request is called but don't check for connected() - as we're returning
        // a null response, the service might have been disconnected already by the time we assert
        // it.
        sReplier.getNextFillRequest();

        // Make sure UI is not shown.
        mUiBot.assertNoDatasetsEver();

        // Test connection lifecycle.
        waitUntilDisconnected();
    }

    @Test
    @AppModeFull // testAutoFillOneDataset() is enough to test ephemeral apps support
    public void testAutofillManuallyAfterServiceReturnedNoDatasets() throws Exception {
        autofillAfterServiceReturnedNoDatasets(true);
    }

    @Test
    @AppModeFull // testAutoFillOneDataset() is enough to test ephemeral apps support
    public void testAutofillAutomaticallyAfterServiceReturnedNoDatasets() throws Exception {
        autofillAfterServiceReturnedNoDatasets(false);
    }

    private void autofillAfterServiceReturnedNoDatasets(boolean manually) throws Exception {
        // Set service.
        enableService();

        // Set expectations.
        sReplier.addResponse(NO_RESPONSE);
        mActivity.expectAutoFill("dude", "sweet");

        // Trigger autofill.
        mActivity.onUsername(View::requestFocus);
        sReplier.getNextFillRequest();

        // Make sure UI is not shown.
        mUiBot.assertNoDatasetsEver();

        // Try again, forcing it
        sReplier.addResponse(new CannedDataset.Builder()
                .setField(ID_USERNAME, "dude")
                .setField(ID_PASSWORD, "sweet")
                .setPresentation(createPresentation("The Dude"))
                .build());

        final int expectedFlags;
        if (manually) {
            expectedFlags = FLAG_MANUAL_REQUEST;
            mActivity.forceAutofillOnUsername();
        } else {
            expectedFlags = 0;
            requestFocusOnPassword();
        }

        final FillRequest fillRequest = sReplier.getNextFillRequest();
        assertHasFlags(fillRequest.flags, expectedFlags);

        // Select the dataset.
        mUiBot.selectDataset("The Dude");

        // Check the results.
        mActivity.assertAutoFilled();
    }

    @Test
    @AppModeFull // testAutoFillOneDataset() is enough to test ephemeral apps support
    public void testAutofillManuallyAndSaveAfterServiceReturnedNoDatasets() throws Exception {
        autofillAndSaveAfterServiceReturnedNoDatasets(true);
    }

    @Test
    @AppModeFull // testAutoFillOneDataset() is enough to test ephemeral apps support
    public void testAutofillAutomaticallyAndSaveAfterServiceReturnedNoDatasets() throws Exception {
        autofillAndSaveAfterServiceReturnedNoDatasets(false);
    }

    private void autofillAndSaveAfterServiceReturnedNoDatasets(boolean manually) throws Exception {
        // Set service.
        enableService();

        // Set expectations.
        sReplier.addResponse(NO_RESPONSE);

        // Trigger autofill.
        // NOTE: must be on password, as saveOnlyTest() will trigger on username
        mActivity.onPassword(View::requestFocus);
        sReplier.getNextFillRequest();

        // Make sure UI is not shown.
        mUiBot.assertNoDatasetsEver();
        sReplier.assertNoUnhandledFillRequests();
        mActivity.onPassword(View::requestFocus);
        mUiBot.assertNoDatasetsEver();
        sReplier.assertNoUnhandledFillRequests();

        // Try again, forcing it
        saveOnlyTest(manually);
    }

    /**
     * More detailed test of what should happen after a service returns a {@code null} FillResponse:
     * views that have already been visit should not trigger a new session, unless a manual autofill
     * workflow was requested.
     */
    @Test
    @AppModeFull // testAutoFillNoDatasets() is enough to test ephemeral apps support
    public void testMultipleIterationsAfterServiceReturnedNoDatasets() throws Exception {
        // Set service.
        enableService();

        // Trigger autofill on username - should call service
        sReplier.addResponse(NO_RESPONSE);
        mActivity.onUsername(View::requestFocus);
        sReplier.getNextFillRequest();
        waitUntilDisconnected();

        // Trigger autofill on password - should call service
        sReplier.addResponse(NO_RESPONSE);
        mActivity.onPassword(View::requestFocus);
        sReplier.getNextFillRequest();
        waitUntilDisconnected();

        // Tap username again - should be ignored
        mActivity.onUsername(View::requestFocus);
        sReplier.assertOnFillRequestNotCalled();
        waitUntilDisconnected();

        // Tap password again - should be ignored
        mActivity.onPassword(View::requestFocus);
        sReplier.assertOnFillRequestNotCalled();
        waitUntilDisconnected();

        // Trigger autofill by manually requesting username - should call service
        sReplier.addResponse(NO_RESPONSE);
        mActivity.forceAutofillOnUsername();
        final FillRequest manualRequest1 = sReplier.getNextFillRequest();
        assertHasFlags(manualRequest1.flags, FLAG_MANUAL_REQUEST);
        waitUntilDisconnected();

        // Trigger autofill by manually requesting password - should call service
        sReplier.addResponse(NO_RESPONSE);
        mActivity.forceAutofillOnPassword();
        final FillRequest manualRequest2 = sReplier.getNextFillRequest();
        assertHasFlags(manualRequest2.flags, FLAG_MANUAL_REQUEST);
        waitUntilDisconnected();
    }

    @Test
    @AppModeFull // testAutofillManuallyOneDataset() is enough to test ephemeral apps support
    public void testAutofillManuallyAlwaysCallServiceAgain() throws Exception {
        // Set service.
        enableService();

        // First request
        sReplier.addResponse(new CannedDataset.Builder()
                .setField(ID_USERNAME, "dude")
                .setField(ID_PASSWORD, "sweet")
                .setPresentation(createPresentation("The Dude"))
                .build());
        mActivity.onUsername(View::requestFocus);
        sReplier.getNextFillRequest();
        mUiBot.assertDatasets("The Dude");

        // Second request
        sReplier.addResponse(new CannedDataset.Builder()
                .setField(ID_USERNAME, "DUDE")
                .setField(ID_PASSWORD, "SWEET")
                .setPresentation(createPresentation("THE DUDE"))
                .build());

        mActivity.forceAutofillOnUsername();
        final FillRequest secondRequest = sReplier.getNextFillRequest();
        assertHasFlags(secondRequest.flags, FLAG_MANUAL_REQUEST);
        mUiBot.assertDatasets("THE DUDE");
    }

    @Test
    public void testAutoFillOneDataset() throws Exception {
        autofillOneDatasetTest(BorderType.NONE);
    }

    @Test
    @AppModeFull // testAutoFillOneDataset_withHeaderAndFooter() is enough to test ephemeral apps
    public void testAutoFillOneDataset_withHeader() throws Exception {
        autofillOneDatasetTest(BorderType.HEADER_ONLY);
    }

    @Test
    @AppModeFull // testAutoFillOneDataset_withHeaderAndFooter() is enough to test ephemeral apps
    public void testAutoFillOneDataset_withFooter() throws Exception {
        autofillOneDatasetTest(BorderType.FOOTER_ONLY);
    }

    @Test
    public void testAutoFillOneDataset_withHeaderAndFooter() throws Exception {
        autofillOneDatasetTest(BorderType.BOTH);
    }

    private enum BorderType {
        NONE,
        HEADER_ONLY,
        FOOTER_ONLY,
        BOTH
    }

    private void autofillOneDatasetTest(BorderType borderType) throws Exception {
        // Set service.
        enableService();

        // Set expectations.
        String expectedHeader = null, expectedFooter = null;

        final CannedFillResponse.Builder builder = new CannedFillResponse.Builder()
                .addDataset(new CannedDataset.Builder()
                        .setField(ID_USERNAME, "dude")
                        .setField(ID_PASSWORD, "sweet")
                        .setPresentation(createPresentation("The Dude"))
                        .build());
        if (borderType == BorderType.BOTH || borderType == BorderType.HEADER_ONLY) {
            expectedHeader = "Head";
            builder.setHeader(createPresentation(expectedHeader));
        }
        if (borderType == BorderType.BOTH || borderType == BorderType.FOOTER_ONLY) {
            expectedFooter = "Tails";
            builder.setFooter(createPresentation(expectedFooter));
        }
        sReplier.addResponse(builder.build());
        mActivity.expectAutoFill("dude", "sweet");

        // Dynamically set password to make sure it's sanitized.
        mActivity.onPassword((v) -> v.setText("I AM GROOT"));

        // Trigger auto-fill.
        requestFocusOnUsername();

        // Auto-fill it.
        final UiObject2 picker = mUiBot.assertDatasetsWithBorders(expectedHeader, expectedFooter,
                "The Dude");

        mUiBot.selectDataset(picker, "The Dude");

        // Check the results.
        mActivity.assertAutoFilled();

        // Sanity checks.

        // Make sure input was sanitized.
        final FillRequest request = sReplier.getNextFillRequest();
        assertWithMessage("CancelationSignal is null").that(request.cancellationSignal).isNotNull();
        assertTextIsSanitized(request.structure, ID_PASSWORD);

        // Make sure initial focus was properly set.
        assertWithMessage("Username node is not focused").that(
                findNodeByResourceId(request.structure, ID_USERNAME).isFocused()).isTrue();
        assertWithMessage("Password node is focused").that(
                findNodeByResourceId(request.structure, ID_PASSWORD).isFocused()).isFalse();
    }

    @Test
    public void testDatasetPickerPosition() throws Exception {
        // TODO: currently disabled because the screenshot contains elements external to the
        // activity that can change (for exmaple, clock), which causes flakiness to the test.
        final boolean compareBitmaps = false;
        final boolean pickerAndViewBoundsMatches = !isAutofillWindowFullScreen(mContext);

        // Set service.
        enableService();
        final MyAutofillCallback callback = mActivity.registerCallback();
        final View username = mActivity.getUsername();
        final View password = mActivity.getPassword();

        // Set expectations.
        final CannedFillResponse.Builder builder = new CannedFillResponse.Builder()
                .addDataset(new CannedDataset.Builder()
                        .setField(ID_USERNAME, "dude", createPresentation("DUDE"))
                        .setField(ID_PASSWORD, "sweet", createPresentation("SWEET"))
                        .build());
        sReplier.addResponse(builder.build());

        // Trigger autofill on username
        final Rect usernameBoundaries1 = mUiBot.selectByRelativeId(ID_USERNAME).getVisibleBounds();
        sReplier.getNextFillRequest();
        callback.assertUiShownEvent(username);
        final Rect usernamePickerBoundaries1 = mUiBot.assertDatasets("DUDE").getVisibleBounds();
        final Bitmap usernameScreenshot1 = compareBitmaps ? mUiBot.takeScreenshot() : null;
        Log.v(TAG,
                "Username1 at " + usernameBoundaries1 + "; picker at " + usernamePickerBoundaries1);
        // TODO(b/37566627): assertions below might be too aggressive - use range instead?
        if (pickerAndViewBoundsMatches) {
            assertThat(usernamePickerBoundaries1.top).isEqualTo(usernameBoundaries1.bottom);
            assertThat(usernamePickerBoundaries1.left).isEqualTo(usernameBoundaries1.left);
        }

        // Move to password
        final Rect passwordBoundaries1 = mUiBot.selectByRelativeId(ID_PASSWORD).getVisibleBounds();
        callback.assertUiHiddenEvent(username);
        callback.assertUiShownEvent(password);
        final Rect passwordPickerBoundaries1 = mUiBot.assertDatasets("SWEET").getVisibleBounds();
        final Bitmap passwordScreenshot1 = compareBitmaps ? mUiBot.takeScreenshot() : null;
        Log.v(TAG,
                "Password1 at " + passwordBoundaries1 + "; picker at " + passwordPickerBoundaries1);
        // TODO(b/37566627): assertions below might be too aggressive - use range instead?
        if (pickerAndViewBoundsMatches) {
            assertThat(passwordPickerBoundaries1.top).isEqualTo(passwordBoundaries1.bottom);
            assertThat(passwordPickerBoundaries1.left).isEqualTo(passwordBoundaries1.left);
        }

        // Then back to username
        final Rect usernameBoundaries2 = mUiBot.selectByRelativeId(ID_USERNAME).getVisibleBounds();
        callback.assertUiHiddenEvent(password);
        callback.assertUiShownEvent(username);
        final Rect usernamePickerBoundaries2 = mUiBot.assertDatasets("DUDE").getVisibleBounds();
        final Bitmap usernameScreenshot2 = compareBitmaps ? mUiBot.takeScreenshot() : null;
        Log.v(TAG,
                "Username2 at " + usernameBoundaries2 + "; picker at " + usernamePickerBoundaries2);

        // And back to the password again..
        final Rect passwordBoundaries2 = mUiBot.selectByRelativeId(ID_PASSWORD).getVisibleBounds();
        callback.assertUiHiddenEvent(username);
        callback.assertUiShownEvent(password);
        final Rect passwordPickerBoundaries2 = mUiBot.assertDatasets("SWEET").getVisibleBounds();
        final Bitmap passwordScreenshot2 = compareBitmaps ? mUiBot.takeScreenshot() : null;
        Log.v(TAG,
                "Password2 at " + passwordBoundaries2 + "; picker at " + passwordPickerBoundaries2);

        // Assert final state matches initial...
        // ... for username
        assertThat(usernameBoundaries2).isEqualTo(usernameBoundaries1);
        assertThat(usernamePickerBoundaries2).isEqualTo(usernamePickerBoundaries1);
        if (compareBitmaps) {
            Helper.assertBitmapsAreSame("username", usernameScreenshot1, usernameScreenshot2);
        }
        // ... for password
        assertThat(passwordBoundaries2).isEqualTo(passwordBoundaries1);
        assertThat(passwordPickerBoundaries2).isEqualTo(passwordPickerBoundaries1);
        if (compareBitmaps) {
            Helper.assertBitmapsAreSame("password", passwordScreenshot1, passwordScreenshot2);
        }

        // Final sanity check
        callback.assertNumberUnhandledEvents(0);
    }

    @Test
    @AppModeFull // testAutoFillOneDataset() is enough to test ephemeral apps
    public void testAutoFillTwoDatasetsSameNumberOfFields() throws Exception {
        // Set service.
        enableService();

        // Set expectations.
        sReplier.addResponse(new CannedFillResponse.Builder()
                .addDataset(new CannedDataset.Builder()
                        .setField(ID_USERNAME, "dude")
                        .setField(ID_PASSWORD, "sweet")
                        .setPresentation(createPresentation("The Dude"))
                        .build())
                .addDataset(new CannedDataset.Builder()
                        .setField(ID_USERNAME, "DUDE")
                        .setField(ID_PASSWORD, "SWEET")
                        .setPresentation(createPresentation("THE DUDE"))
                        .build())
                .build());
        mActivity.expectAutoFill("dude", "sweet");

        // Trigger auto-fill.
        requestFocusOnUsername();
        sReplier.getNextFillRequest();

        // Make sure all datasets are available...
        mUiBot.assertDatasets("The Dude", "THE DUDE");

        // ... on all fields.
        requestFocusOnPassword();
        mUiBot.assertDatasets("The Dude", "THE DUDE");

        // Auto-fill it.
        mUiBot.selectDataset("The Dude");

        // Check the results.
        mActivity.assertAutoFilled();
    }

    @Test
    @AppModeFull // testAutoFillOneDataset() is enough to test ephemeral apps
    public void testAutoFillTwoDatasetsUnevenNumberOfFieldsFillsAll() throws Exception {
        autoFillTwoDatasetsUnevenNumberOfFieldsTest(true);
    }

    @Test
    @AppModeFull // testAutoFillOneDataset() is enough to test ephemeral apps
    public void testAutoFillTwoDatasetsUnevenNumberOfFieldsFillsOne() throws Exception {
        autoFillTwoDatasetsUnevenNumberOfFieldsTest(false);
    }

    private void autoFillTwoDatasetsUnevenNumberOfFieldsTest(boolean fillsAll) throws Exception {
        // Set service.
        enableService();

        // Set expectations.
        sReplier.addResponse(new CannedFillResponse.Builder()
                .addDataset(new CannedDataset.Builder()
                        .setField(ID_USERNAME, "dude")
                        .setField(ID_PASSWORD, "sweet")
                        .setPresentation(createPresentation("The Dude"))
                        .build())
                .addDataset(new CannedDataset.Builder()
                        .setField(ID_USERNAME, "DUDE")
                        .setPresentation(createPresentation("THE DUDE"))
                        .build())
                .build());
        if (fillsAll) {
            mActivity.expectAutoFill("dude", "sweet");
        } else {
            mActivity.expectAutoFill("DUDE");
        }

        // Trigger auto-fill.
        requestFocusOnUsername();
        sReplier.getNextFillRequest();

        // Make sure all datasets are available on username...
        mUiBot.assertDatasets("The Dude", "THE DUDE");

        // ... but just one for password
        requestFocusOnPassword();
        mUiBot.assertDatasets("The Dude");

        // Auto-fill it.
        requestFocusOnUsername();
        mUiBot.assertDatasets("The Dude", "THE DUDE");
        if (fillsAll) {
            mUiBot.selectDataset("The Dude");
        } else {
            mUiBot.selectDataset("THE DUDE");
        }

        // Check the results.
        mActivity.assertAutoFilled();
    }

    @Test
    @AppModeFull // testAutoFillOneDataset() is enough to test ephemeral apps
    public void testAutoFillDatasetWithoutFieldIsIgnored() throws Exception {
        // Set service.
        enableService();

        // Set expectations.
        sReplier.addResponse(new CannedFillResponse.Builder()
                .addDataset(new CannedDataset.Builder()
                        .setField(ID_USERNAME, "dude")
                        .setField(ID_PASSWORD, "sweet")
                        .setPresentation(createPresentation("The Dude"))
                        .build())
                .addDataset(new CannedDataset.Builder()
                        .setField(ID_USERNAME, "DUDE")
                        .setField(ID_PASSWORD, "SWEET")
                        .build())
                .build());
        mActivity.expectAutoFill("dude", "sweet");

        // Trigger auto-fill.
        requestFocusOnUsername();
        sReplier.getNextFillRequest();

        // Make sure all datasets are available...
        mUiBot.assertDatasets("The Dude");

        // ... on all fields.
        requestFocusOnPassword();
        mUiBot.assertDatasets("The Dude");

        // Auto-fill it.
        mUiBot.selectDataset("The Dude");

        // Check the results.
        mActivity.assertAutoFilled();
    }

    @Test
    public void testAutoFillWhenViewHasChildAccessibilityNodes() throws Exception {
        mActivity.onUsername((v) -> v.setAccessibilityDelegate(new AccessibilityDelegate() {
            @Override
            public AccessibilityNodeProvider getAccessibilityNodeProvider(View host) {
                return new AccessibilityNodeProvider() {
                    @Override
                    public AccessibilityNodeInfo createAccessibilityNodeInfo(int virtualViewId) {
                        final AccessibilityNodeInfo info = AccessibilityNodeInfo.obtain();
                        if (virtualViewId == View.NO_ID) {
                            info.addChild(v, 108);
                        }
                        return info;
                    }
                };
            }
        }));

        testAutoFillOneDataset();
    }

    @Test
    public void testAutoFillOneDatasetAndMoveFocusAround() throws Exception {
        // Set service.
        enableService();

        // Set expectations.
        sReplier.addResponse(new CannedDataset.Builder()
                .setField(ID_USERNAME, "dude")
                .setField(ID_PASSWORD, "sweet")
                .setPresentation(createPresentation("The Dude"))
                .build());
        mActivity.expectAutoFill("dude", "sweet");

        // Trigger auto-fill.
        requestFocusOnUsername();
        sReplier.getNextFillRequest();

        // Make sure tapping on other fields from the dataset does not trigger it again
        requestFocusOnPassword();
        sReplier.assertNoUnhandledFillRequests();

        requestFocusOnUsername();
        sReplier.assertNoUnhandledFillRequests();

        // Auto-fill it.
        mUiBot.selectDataset("The Dude");

        // Check the results.
        mActivity.assertAutoFilled();

        // Make sure tapping on other fields from the dataset does not trigger it again
        requestFocusOnPassword();
        mUiBot.assertNoDatasets();
        requestFocusOnUsernameNoWindowChange();
        mUiBot.assertNoDatasetsEver();
    }

    @Test
    public void testUiNotShownAfterAutofilled() throws Exception {
        // Set service.
        enableService();

        // Set expectations.
        sReplier.addResponse(new CannedDataset.Builder()
                .setField(ID_USERNAME, "dude")
                .setField(ID_PASSWORD, "sweet")
                .setPresentation(createPresentation("The Dude"))
                .build());
        mActivity.expectAutoFill("dude", "sweet");

        // Trigger auto-fill.
        requestFocusOnUsername();
        sReplier.getNextFillRequest();
        mUiBot.selectDataset("The Dude");

        // Check the results.
        mActivity.assertAutoFilled();

        // Make sure tapping on autofilled field does not trigger it again
        requestFocusOnPassword();
        mUiBot.assertNoDatasets();

        requestFocusOnUsernameNoWindowChange();
        mUiBot.assertNoDatasetsEver();
    }

    @Test
    public void testAutofillTapOutside() throws Exception {
        // Set service.
        enableService();
        final MyAutofillCallback callback = mActivity.registerCallback();

        // Set expectations.
        sReplier.addResponse(new CannedDataset.Builder()
                .setField(ID_USERNAME, "dude")
                .setField(ID_PASSWORD, "sweet")
                .setPresentation(createPresentation("The Dude"))
                .build());
        mActivity.expectAutoFill("dude", "sweet");

        // Trigger autofill.
        requestFocusOnUsername();
        sReplier.getNextFillRequest();
        final View username = mActivity.getUsername();

        callback.assertUiShownEvent(username);
        mUiBot.assertDatasets("The Dude");

        // tapping outside autofill window should close it and raise ui hidden event
        mUiBot.waitForWindowChange(() -> tap(mActivity.getUsernameLabel()),
                Timeouts.UI_TIMEOUT.getMaxValue());
        callback.assertUiHiddenEvent(username);

        mUiBot.assertNoDatasets();
    }

    @Test
    public void testAutofillCallbacks() throws Exception {
        // Set service.
        enableService();
        final MyAutofillCallback callback = mActivity.registerCallback();

        // Set expectations.
        sReplier.addResponse(new CannedDataset.Builder()
                .setField(ID_USERNAME, "dude")
                .setField(ID_PASSWORD, "sweet")
                .setPresentation(createPresentation("The Dude"))
                .build());
        mActivity.expectAutoFill("dude", "sweet");

        // Trigger autofill.
        requestFocusOnUsername();
        sReplier.getNextFillRequest();
        final View username = mActivity.getUsername();
        final View password = mActivity.getPassword();

        callback.assertUiShownEvent(username);

        requestFocusOnPassword();
        callback.assertUiHiddenEvent(username);
        callback.assertUiShownEvent(password);

        // Unregister callback to make sure no more events are received
        mActivity.unregisterCallback();
        requestFocusOnUsername();
        // Blindly sleep - we cannot wait on any event as none should have been sent
        SystemClock.sleep(MyAutofillCallback.MY_TIMEOUT.ms());
        callback.assertNumberUnhandledEvents(0);

        // Autofill it.
        mUiBot.selectDataset("The Dude");

        // Check the results.
        mActivity.assertAutoFilled();
    }

    @Test
    @AppModeFull // testAutofillCallbacks() is enough to test ephemeral apps
    public void testAutofillCallbackDisabled() throws Exception {
        // Set service.
        disableService();

        final MyAutofillCallback callback = mActivity.registerCallback();

        // Trigger auto-fill.
        mActivity.onUsername(View::requestFocus);

        // Assert callback was called
        final View username = mActivity.getUsername();
        callback.assertUiUnavailableEvent(username);
    }

    @Test
    @AppModeFull // testAutofillCallbacks() is enough to test ephemeral apps
    public void testAutofillCallbackNoDatasets() throws Exception {
        callbackUnavailableTest(NO_RESPONSE);
    }

    @Test
    @AppModeFull // testAutofillCallbacks() is enough to test ephemeral apps
    public void testAutofillCallbackNoDatasetsButSaveInfo() throws Exception {
        callbackUnavailableTest(new CannedFillResponse.Builder()
                .setRequiredSavableIds(SAVE_DATA_TYPE_PASSWORD, ID_USERNAME, ID_PASSWORD)
                .build());
    }

    private void callbackUnavailableTest(CannedFillResponse response) throws Exception {
        // Set service.
        enableService();
        final MyAutofillCallback callback = mActivity.registerCallback();

        // Set expectations.
        sReplier.addResponse(response);

        // Trigger auto-fill.
        mActivity.onUsername(View::requestFocus);
        sReplier.getNextFillRequest();

        // Auto-fill it.
        mUiBot.assertNoDatasetsEver();

        // Assert callback was called
        final View username = mActivity.getUsername();
        callback.assertUiUnavailableEvent(username);
    }

    @Test
    public void testAutoFillOneDatasetAndSave() throws Exception {
        // Set service.
        enableService();

        // Set expectations.
        final Bundle extras = new Bundle();
        extras.putString("numbers", "4815162342");

        sReplier.addResponse(new CannedFillResponse.Builder()
                .addDataset(new CannedDataset.Builder()
                        .setId("I'm the alpha and the omega")
                        .setField(ID_USERNAME, "dude")
                        .setField(ID_PASSWORD, "sweet")
                        .setPresentation(createPresentation("The Dude"))
                        .build())
                .setRequiredSavableIds(SAVE_DATA_TYPE_PASSWORD, ID_USERNAME, ID_PASSWORD)
                .setExtras(extras)
                .build());
        mActivity.expectAutoFill("dude", "sweet");

        // Trigger auto-fill.
        requestFocusOnUsername();

        // Since this is a Presubmit test, wait for connection to avoid flakiness.
        waitUntilConnected();

        final FillRequest fillRequest = sReplier.getNextFillRequest();

        // Make sure input was sanitized...
        assertTextIsSanitized(fillRequest.structure, ID_USERNAME);
        assertTextIsSanitized(fillRequest.structure, ID_PASSWORD);

        // ...but labels weren't
        assertTextOnly(fillRequest.structure, ID_USERNAME_LABEL, "Username");
        assertTextOnly(fillRequest.structure, ID_PASSWORD_LABEL, "Password");

        // Auto-fill it.
        mUiBot.selectDataset("The Dude");

        // Check the results.
        mActivity.assertAutoFilled();

        // Try to login, it will fail.
        final String loginMessage = mActivity.tapLogin();

        assertWithMessage("Wrong login msg").that(loginMessage).isEqualTo(AUTHENTICATION_MESSAGE);

        // Set right password...
        mActivity.onPassword((v) -> v.setText("dude"));

        // ... and try again
        final String expectedMessage = getWelcomeMessage("dude");
        final String actualMessage = mActivity.tapLogin();
        assertWithMessage("Wrong welcome msg").that(actualMessage).isEqualTo(expectedMessage);

        // Assert the snack bar is shown and tap "Save".
        mUiBot.saveForAutofill(true, SAVE_DATA_TYPE_PASSWORD);

        final SaveRequest saveRequest = sReplier.getNextSaveRequest();

        assertThat(saveRequest.datasetIds).containsExactly("I'm the alpha and the omega");

        // Assert value of expected fields - should not be sanitized.
        assertTextAndValue(saveRequest.structure, ID_USERNAME, "dude");
        assertTextAndValue(saveRequest.structure, ID_PASSWORD, "dude");
        assertTextOnly(saveRequest.structure, ID_USERNAME_LABEL, "Username");
        assertTextOnly(saveRequest.structure, ID_PASSWORD_LABEL, "Password");

        // Make sure extras were passed back on onSave()
        assertThat(saveRequest.data).isNotNull();
        final String extraValue = saveRequest.data.getString("numbers");
        assertWithMessage("extras not passed on save").that(extraValue).isEqualTo("4815162342");
    }

    @Test
    public void testAutoFillOneDatasetAndSaveHidingOverlays() throws Exception {
        // Set service.
        enableService();

        // Set expectations.
        final Bundle extras = new Bundle();
        extras.putString("numbers", "4815162342");

        sReplier.addResponse(new CannedFillResponse.Builder()
                .addDataset(new CannedDataset.Builder()
                        .setField(ID_USERNAME, "dude")
                        .setField(ID_PASSWORD, "sweet")
                        .setPresentation(createPresentation("The Dude"))
                        .build())
                .setRequiredSavableIds(SAVE_DATA_TYPE_PASSWORD, ID_USERNAME, ID_PASSWORD)
                .setExtras(extras)
                .build());
        mActivity.expectAutoFill("dude", "sweet");

        // Trigger auto-fill.
        requestFocusOnUsername();

        // Since this is a Presubmit test, wait for connection to avoid flakiness.
        waitUntilConnected();

        sReplier.getNextFillRequest();

        // Add an overlay on top of the whole screen
        final View[] overlay = new View[1];
        try {
            // Allow ourselves to add overlays
            runShellCommand("appops set %s SYSTEM_ALERT_WINDOW allow", mPackageName);

            // Make sure the fill UI is shown.
            mUiBot.assertDatasets("The Dude");

            final CountDownLatch latch = new CountDownLatch(1);

            mActivity.runOnUiThread(() -> {
                // This overlay is focusable, full-screen, which should block interaction
                // with the fill UI unless the platform successfully hides overlays.
                final WindowManager.LayoutParams params = new WindowManager.LayoutParams();
                params.type = WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY;
                params.flags = WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN;
                params.width = ViewGroup.LayoutParams.MATCH_PARENT;
                params.height = ViewGroup.LayoutParams.MATCH_PARENT;

                final View view = new View(mContext) {
                    @Override
                    protected void onAttachedToWindow() {
                        super.onAttachedToWindow();
                        latch.countDown();
                    }
                };
                view.setBackgroundColor(Color.RED);
                WindowManager windowManager = mContext.getSystemService(WindowManager.class);
                windowManager.addView(view, params);
                overlay[0] = view;
            });

            // Wait for the window being added.
            assertThat(latch.await(5, TimeUnit.SECONDS)).isTrue();

            // Auto-fill it.
            mUiBot.selectDataset("The Dude");

            // Check the results.
            mActivity.assertAutoFilled();

            // Try to login, it will fail.
            final String loginMessage = mActivity.tapLogin();

            assertWithMessage("Wrong login msg").that(loginMessage).isEqualTo(
                    AUTHENTICATION_MESSAGE);

            // Set right password...
            mActivity.onPassword((v) -> v.setText("dude"));

            // ... and try again
            final String expectedMessage = getWelcomeMessage("dude");
            final String actualMessage = mActivity.tapLogin();
            assertWithMessage("Wrong welcome msg").that(actualMessage).isEqualTo(expectedMessage);

            // Assert the snack bar is shown and tap "Save".
            mUiBot.saveForAutofill(true, SAVE_DATA_TYPE_PASSWORD);

            final SaveRequest saveRequest = sReplier.getNextSaveRequest();

            // Assert value of expected fields - should not be sanitized.
            final ViewNode username = findNodeByResourceId(saveRequest.structure, ID_USERNAME);
            assertTextAndValue(username, "dude");
            final ViewNode password = findNodeByResourceId(saveRequest.structure, ID_PASSWORD);
            assertTextAndValue(password, "dude");

            // Make sure extras were passed back on onSave()
            assertThat(saveRequest.data).isNotNull();
            final String extraValue = saveRequest.data.getString("numbers");
            assertWithMessage("extras not passed on save").that(extraValue).isEqualTo("4815162342");
        } finally {
            // Make sure we can no longer add overlays
            runShellCommand("appops set %s SYSTEM_ALERT_WINDOW ignore", mPackageName);
            // Make sure the overlay is removed
            mActivity.runOnUiThread(() -> {
                WindowManager windowManager = mContext.getSystemService(WindowManager.class);
                windowManager.removeView(overlay[0]);
            });
        }
    }

    @Test
    @AppModeFull // testAutoFillOneDataset() is enough to test ephemeral apps
    public void testAutoFillMultipleDatasetsPickFirst() throws Exception {
        multipleDatasetsTest(1);
    }

    @Test
    @AppModeFull // testAutoFillOneDataset() is enough to test ephemeral apps
    public void testAutoFillMultipleDatasetsPickSecond() throws Exception {
        multipleDatasetsTest(2);
    }

    @Test
    @AppModeFull // testAutoFillOneDataset() is enough to test ephemeral apps
    public void testAutoFillMultipleDatasetsPickThird() throws Exception {
        multipleDatasetsTest(3);
    }

    private void multipleDatasetsTest(int number) throws Exception {
        // Set service.
        enableService();

        // Set expectations.
        sReplier.addResponse(new CannedFillResponse.Builder()
                .addDataset(new CannedDataset.Builder()
                        .setField(ID_USERNAME, "mr_plow")
                        .setField(ID_PASSWORD, "D'OH!")
                        .setPresentation(createPresentation("Mr Plow"))
                        .build())
                .addDataset(new CannedDataset.Builder()
                        .setField(ID_USERNAME, "el barto")
                        .setField(ID_PASSWORD, "aycaramba!")
                        .setPresentation(createPresentation("El Barto"))
                        .build())
                .addDataset(new CannedDataset.Builder()
                        .setField(ID_USERNAME, "mr sparkle")
                        .setField(ID_PASSWORD, "Aw3someP0wer")
                        .setPresentation(createPresentation("Mr Sparkle"))
                        .build())
                .build());
        final String name;

        switch (number) {
            case 1:
                name = "Mr Plow";
                mActivity.expectAutoFill("mr_plow", "D'OH!");
                break;
            case 2:
                name = "El Barto";
                mActivity.expectAutoFill("el barto", "aycaramba!");
                break;
            case 3:
                name = "Mr Sparkle";
                mActivity.expectAutoFill("mr sparkle", "Aw3someP0wer");
                break;
            default:
                throw new IllegalArgumentException("invalid dataset number: " + number);
        }

        // Trigger auto-fill.
        requestFocusOnUsername();
        sReplier.getNextFillRequest();

        // Make sure all datasets are shown.
        final UiObject2 picker = mUiBot.assertDatasets("Mr Plow", "El Barto", "Mr Sparkle");

        // Auto-fill it.
        mUiBot.selectDataset(picker, name);

        // Check the results.
        mActivity.assertAutoFilled();
    }

    /**
     * Tests the scenario where the service uses custom remote views for different fields (username
     * and password).
     */
    @Test
    public void testAutofillOneDatasetCustomPresentation() throws Exception {
        // Set service.
        enableService();

        // Set expectations.
        sReplier.addResponse(new CannedDataset.Builder()
                .setField(ID_USERNAME, "dude",
                        createPresentation("The Dude"))
                .setField(ID_PASSWORD, "sweet",
                        createPresentation("Dude's password"))
                .build());
        mActivity.expectAutoFill("dude", "sweet");

        // Trigger auto-fill.
        requestFocusOnUsername();
        sReplier.getNextFillRequest();

        // Check initial field.
        mUiBot.assertDatasets("The Dude");

        // Then move around...
        requestFocusOnPassword();
        mUiBot.assertDatasets("Dude's password");
        requestFocusOnUsername();
        mUiBot.assertDatasets("The Dude");

        // Auto-fill it.
        requestFocusOnPassword();
        mUiBot.selectDataset("Dude's password");

        // Check the results.
        mActivity.assertAutoFilled();
    }

    /**
     * Tests the scenario where the service uses custom remote views for different fields (username
     * and password) and the dataset itself, and each dataset has the same number of fields.
     */
    @Test
    @AppModeFull // testAutofillOneDatasetCustomPresentation() is enough to test ephemeral apps
    public void testAutofillMultipleDatasetsCustomPresentations() throws Exception {
        // Set service.
        enableService();

        // Set expectations.
        sReplier.addResponse(new CannedFillResponse.Builder()
                .addDataset(new CannedDataset.Builder(createPresentation("Dataset1"))
                        .setField(ID_USERNAME, "user1") // no presentation
                        .setField(ID_PASSWORD, "pass1", createPresentation("Pass1"))
                        .build())
                .addDataset(new CannedDataset.Builder()
                        .setField(ID_USERNAME, "user2", createPresentation("User2"))
                        .setField(ID_PASSWORD, "pass2") // no presentation
                        .setPresentation(createPresentation("Dataset2"))
                        .build())
                .build());
        mActivity.expectAutoFill("user1", "pass1");

        // Trigger auto-fill.
        requestFocusOnUsername();
        sReplier.getNextFillRequest();

        // Check initial field.
        mUiBot.assertDatasets("Dataset1", "User2");

        // Then move around...
        requestFocusOnPassword();
        mUiBot.assertDatasets("Pass1", "Dataset2");
        requestFocusOnUsername();
        mUiBot.assertDatasets("Dataset1", "User2");

        // Auto-fill it.
        requestFocusOnPassword();
        mUiBot.selectDataset("Pass1");

        // Check the results.
        mActivity.assertAutoFilled();
    }

    /**
     * Tests the scenario where the service uses custom remote views for different fields (username
     * and password), and each dataset has the same number of fields.
     */
    @Test
    @AppModeFull // testAutofillOneDatasetCustomPresentation() is enough to test ephemeral apps
    public void testAutofillMultipleDatasetsCustomPresentationSameFields() throws Exception {
        // Set service.
        enableService();

        // Set expectations.
        sReplier.addResponse(new CannedFillResponse.Builder()
                .addDataset(new CannedDataset.Builder()
                        .setField(ID_USERNAME, "user1", createPresentation("User1"))
                        .setField(ID_PASSWORD, "pass1", createPresentation("Pass1"))
                        .build())
                .addDataset(new CannedDataset.Builder()
                        .setField(ID_USERNAME, "user2", createPresentation("User2"))
                        .setField(ID_PASSWORD, "pass2", createPresentation("Pass2"))
                        .build())
                .build());
        mActivity.expectAutoFill("user1", "pass1");

        // Trigger auto-fill.
        requestFocusOnUsername();
        sReplier.getNextFillRequest();

        // Check initial field.
        mUiBot.assertDatasets("User1", "User2");

        // Then move around...
        requestFocusOnPassword();
        mUiBot.assertDatasets("Pass1", "Pass2");
        requestFocusOnUsername();
        mUiBot.assertDatasets("User1", "User2");

        // Auto-fill it.
        requestFocusOnPassword();
        mUiBot.selectDataset("Pass1");

        // Check the results.
        mActivity.assertAutoFilled();
    }

    /**
     * Tests the scenario where the service uses custom remote views for different fields (username
     * and password), but each dataset has a different number of fields.
     */
    @Test
    @AppModeFull // testAutofillOneDatasetCustomPresentation() is enough to test ephemeral apps
    public void testAutofillMultipleDatasetsCustomPresentationFirstDatasetMissingSecondField()
            throws Exception {
        // Set service.
        enableService();

        // Set expectations.
        sReplier.addResponse(new CannedFillResponse.Builder()
                .addDataset(new CannedDataset.Builder()
                        .setField(ID_USERNAME, "user1", createPresentation("User1"))
                        .build())
                .addDataset(new CannedDataset.Builder()
                        .setField(ID_USERNAME, "user2", createPresentation("User2"))
                        .setField(ID_PASSWORD, "pass2", createPresentation("Pass2"))
                        .build())
                .build());
        mActivity.expectAutoFill("user2", "pass2");

        // Trigger auto-fill.
        requestFocusOnUsername();
        sReplier.getNextFillRequest();

        // Check initial field.
        mUiBot.assertDatasets("User1", "User2");

        // Then move around...
        requestFocusOnPassword();
        mUiBot.assertDatasets("Pass2");
        requestFocusOnUsername();
        mUiBot.assertDatasets("User1", "User2");

        // Auto-fill it.
        mUiBot.selectDataset("User2");

        // Check the results.
        mActivity.assertAutoFilled();
    }

    /**
     * Tests the scenario where the service uses custom remote views for different fields (username
     * and password), but each dataset has a different number of fields.
     */
    @Test
    @AppModeFull // testAutofillOneDatasetCustomPresentation() is enough to test ephemeral apps
    public void testAutofillMultipleDatasetsCustomPresentationSecondDatasetMissingFirstField()
            throws Exception {
        // Set service.
        enableService();

        // Set expectations.
        sReplier.addResponse(new CannedFillResponse.Builder()
                .addDataset(new CannedDataset.Builder()
                        .setField(ID_USERNAME, "user1", createPresentation("User1"))
                        .setField(ID_PASSWORD, "pass1", createPresentation("Pass1"))
                        .build())
                .addDataset(new CannedDataset.Builder()
                        .setField(ID_PASSWORD, "pass2", createPresentation("Pass2"))
                        .build())
                .build());
        mActivity.expectAutoFill("user1", "pass1");

        // Trigger auto-fill.
        requestFocusOnUsername();
        sReplier.getNextFillRequest();

        // Check initial field.
        mUiBot.assertDatasets("User1");

        // Then move around...
        requestFocusOnPassword();
        mUiBot.assertDatasets("Pass1", "Pass2");
        requestFocusOnUsername();
        mUiBot.assertDatasets("User1");

        // Auto-fill it.
        mUiBot.selectDataset("User1");

        // Check the results.
        mActivity.assertAutoFilled();
    }

    @Test
    @AppModeFull // testAutoFillOneDatasetAndSave() is enough to test ephemeral apps
    public void testSaveOnly() throws Exception {
        saveOnlyTest(false);
    }

    @Test
    @AppModeFull // testAutoFillOneDatasetAndSave() is enough to test ephemeral apps
    public void testSaveOnlyTriggeredManually() throws Exception {
        saveOnlyTest(false);
    }

    private void saveOnlyTest(boolean manually) throws Exception {
        enableService();

        // Set expectations.
        sReplier.addResponse(new CannedFillResponse.Builder()
                .setRequiredSavableIds(SAVE_DATA_TYPE_PASSWORD, ID_USERNAME, ID_PASSWORD)
                .build());

        // Trigger auto-fill.
        if (manually) {
            mActivity.forceAutofillOnUsername();
        } else {
            mActivity.onUsername(View::requestFocus);
        }

        // Sanity check.
        mUiBot.assertNoDatasetsEver();

        // Wait for onFill() before proceeding, otherwise the fields might be changed before
        // the session started
        sReplier.getNextFillRequest();

        // Set credentials...
        mActivity.onUsername((v) -> v.setText("malkovich"));
        mActivity.onPassword((v) -> v.setText("malkovich"));

        // ...and login
        final String expectedMessage = getWelcomeMessage("malkovich");
        final String actualMessage = mActivity.tapLogin();
        assertWithMessage("Wrong welcome msg").that(actualMessage).isEqualTo(expectedMessage);

        // Assert the snack bar is shown and tap "Save".
        mUiBot.saveForAutofill(true, SAVE_DATA_TYPE_PASSWORD);

        final SaveRequest saveRequest = sReplier.getNextSaveRequest();
        sReplier.assertNoUnhandledSaveRequests();
        assertThat(saveRequest.datasetIds).isNull();

        // Assert value of expected fields - should not be sanitized.
        try {
            final ViewNode username = findNodeByResourceId(saveRequest.structure, ID_USERNAME);
            assertTextAndValue(username, "malkovich");
            final ViewNode password = findNodeByResourceId(saveRequest.structure, ID_PASSWORD);
            assertTextAndValue(password, "malkovich");
        } catch (AssertionError | RuntimeException e) {
            dumpStructure("saveOnlyTest() failed", saveRequest.structure);
            throw e;
        }
    }

    @Test
    public void testSaveGoesAwayWhenTappingHomeButton() throws Exception {
        saveGoesAway(DismissType.HOME_BUTTON);
    }

    @Test
    public void testSaveGoesAwayWhenTappingBackButton() throws Exception {
        saveGoesAway(DismissType.BACK_BUTTON);
    }

    @Test
    public void testSaveGoesAwayWhenTouchingOutside() throws Exception {
        saveGoesAway(DismissType.TOUCH_OUTSIDE);
    }

    private void saveGoesAway(DismissType dismissType) throws Exception {
        enableService();

        // Set expectations.
        sReplier.addResponse(new CannedFillResponse.Builder()
                .setRequiredSavableIds(SAVE_DATA_TYPE_PASSWORD, ID_USERNAME, ID_PASSWORD)
                .build());

        // Trigger auto-fill.
        mActivity.onUsername(View::requestFocus);

        // Sanity check.
        mUiBot.assertNoDatasetsEver();

        // Wait for onFill() before proceeding, otherwise the fields might be changed before
        // the session started
        sReplier.getNextFillRequest();

        // Set credentials...
        mActivity.onUsername((v) -> v.setText("malkovich"));
        mActivity.onPassword((v) -> v.setText("malkovich"));

        // ...and login
        final String expectedMessage = getWelcomeMessage("malkovich");
        final String actualMessage = mActivity.tapLogin();
        assertWithMessage("Wrong welcome msg").that(actualMessage).isEqualTo(expectedMessage);

        // Assert the snack bar is shown and tap "Save".
        mUiBot.assertSaveShowing(SAVE_DATA_TYPE_PASSWORD);

        // Then make sure it goes away when user doesn't want it..
        switch (dismissType) {
            case BACK_BUTTON:
                mUiBot.pressBack();
                break;
            case HOME_BUTTON:
                mUiBot.pressHome();
                break;
            case TOUCH_OUTSIDE:
                mUiBot.assertShownByText(expectedMessage).click();
                break;
            default:
                throw new IllegalArgumentException("invalid dismiss type: " + dismissType);
        }
        mUiBot.assertSaveNotShowing(SAVE_DATA_TYPE_PASSWORD);
    }

    @Test
    @AppModeFull // testAutoFillOneDatasetAndSave() is enough to test ephemeral apps
    public void testSaveOnlyPreFilled() throws Exception {
        saveOnlyTestPreFilled(false);
    }

    @Test
    @AppModeFull // testAutoFillOneDatasetAndSave() is enough to test ephemeral apps
    public void testSaveOnlyTriggeredManuallyPreFilled() throws Exception {
        saveOnlyTestPreFilled(true);
    }

    private void saveOnlyTestPreFilled(boolean manually) throws Exception {
        enableService();

        // Set expectations.
        sReplier.addResponse(new CannedFillResponse.Builder()
                .setRequiredSavableIds(SAVE_DATA_TYPE_PASSWORD, ID_USERNAME, ID_PASSWORD)
                .build());

        // Set activity
        mActivity.onUsername((v) -> v.setText("user_before"));
        mActivity.onPassword((v) -> v.setText("pass_before"));

        // Trigger auto-fill.
        if (manually) {
            mActivity.forceAutofillOnUsername();
        } else {
            mActivity.onUsername(View::requestFocus);
        }

        // Sanity check.
        mUiBot.assertNoDatasetsEver();

        // Wait for onFill() before proceeding, otherwise the fields might be changed before
        // the session started
        sReplier.getNextFillRequest();

        // Set credentials...
        mActivity.onUsername((v) -> v.setText("user_after"));
        mActivity.onPassword((v) -> v.setText("pass_after"));

        // ...and login
        final String expectedMessage = getWelcomeMessage("user_after");
        final String actualMessage = mActivity.tapLogin();
        assertWithMessage("Wrong welcome msg").that(actualMessage).isEqualTo(expectedMessage);

        // Assert the snack bar is shown and tap "Save".
        mUiBot.saveForAutofill(true, SAVE_DATA_TYPE_PASSWORD);

        final SaveRequest saveRequest = sReplier.getNextSaveRequest();
        sReplier.assertNoUnhandledSaveRequests();

        // Assert value of expected fields - should not be sanitized.
        try {
            final ViewNode username = findNodeByResourceId(saveRequest.structure, ID_USERNAME);
            assertTextAndValue(username, "user_after");
            final ViewNode password = findNodeByResourceId(saveRequest.structure, ID_PASSWORD);
            assertTextAndValue(password, "pass_after");
        } catch (AssertionError | RuntimeException e) {
            dumpStructure("saveOnlyTest() failed", saveRequest.structure);
            throw e;
        }
    }

    @Test
    @AppModeFull // testAutoFillOneDatasetAndSave() is enough to test ephemeral apps
    public void testSaveOnlyTwoRequiredFieldsOnePrefilled() throws Exception {
        enableService();

        // Set expectations.
        sReplier.addResponse(new CannedFillResponse.Builder()
                .setRequiredSavableIds(SAVE_DATA_TYPE_PASSWORD, ID_USERNAME, ID_PASSWORD)
                .build());

        // Set activity
        mActivity.onUsername((v) -> v.setText("I_AM_USER"));

        // Trigger auto-fill.
        mActivity.onPassword(View::requestFocus);

        // Wait for onFill() before changing value, otherwise the fields might be changed before
        // the session started
        sReplier.getNextFillRequest();
        mUiBot.assertNoDatasetsEver();

        // Set credentials...
        mActivity.onPassword((v) -> v.setText("thou should pass")); // contains pass

        // ...and login
        final String expectedMessage = getWelcomeMessage("I_AM_USER"); // contains pass
        final String actualMessage = mActivity.tapLogin();
        assertWithMessage("Wrong welcome msg").that(actualMessage).isEqualTo(expectedMessage);

        // Assert the snack bar is shown and tap "Save".
        mUiBot.saveForAutofill(true, SAVE_DATA_TYPE_PASSWORD);

        final SaveRequest saveRequest = sReplier.getNextSaveRequest();
        sReplier.assertNoUnhandledSaveRequests();

        // Assert value of expected fields - should not be sanitized.
        try {
            final ViewNode username = findNodeByResourceId(saveRequest.structure, ID_USERNAME);
            assertTextAndValue(username, "I_AM_USER");
            final ViewNode password = findNodeByResourceId(saveRequest.structure, ID_PASSWORD);
            assertTextAndValue(password, "thou should pass");
        } catch (AssertionError | RuntimeException e) {
            dumpStructure("saveOnlyTest() failed", saveRequest.structure);
            throw e;
        }
    }

    @Test
    @AppModeFull // testAutoFillOneDatasetAndSave() is enough to test ephemeral apps
    public void testSaveOnlyOptionalField() throws Exception {
        enableService();

        // Set expectations.
        sReplier.addResponse(new CannedFillResponse.Builder()
                .setRequiredSavableIds(SAVE_DATA_TYPE_PASSWORD, ID_USERNAME)
                .setOptionalSavableIds(ID_PASSWORD)
                .build());

        // Trigger auto-fill.
        mActivity.onUsername(View::requestFocus);

        // Sanity check.
        mUiBot.assertNoDatasetsEver();

        // Wait for onFill() before proceeding, otherwise the fields might be changed before
        // the session started
        sReplier.getNextFillRequest();

        // Set credentials...
        mActivity.onUsername((v) -> v.setText("malkovich"));
        mActivity.onPassword(View::requestFocus);
        mActivity.onPassword((v) -> v.setText("malkovich"));

        // ...and login
        final String expectedMessage = getWelcomeMessage("malkovich");
        final String actualMessage = mActivity.tapLogin();
        assertWithMessage("Wrong welcome msg").that(actualMessage).isEqualTo(expectedMessage);

        // Assert the snack bar is shown and tap "Save".
        mUiBot.saveForAutofill(true, SAVE_DATA_TYPE_PASSWORD);

        final SaveRequest saveRequest = sReplier.getNextSaveRequest();

        // Assert value of expected fields - should not be sanitized.
        final ViewNode username = findNodeByResourceId(saveRequest.structure, ID_USERNAME);
        assertTextAndValue(username, "malkovich");
        final ViewNode password = findNodeByResourceId(saveRequest.structure, ID_PASSWORD);
        assertTextAndValue(password, "malkovich");
    }

    @Test
    @AppModeFull // testAutoFillOneDatasetAndSave() is enough to test ephemeral apps
    public void testSaveNoRequiredField_NoneFilled() throws Exception {
        optionalOnlyTest(FilledFields.NONE);
    }

    @Test
    @AppModeFull // testAutoFillOneDatasetAndSave() is enough to test ephemeral apps
    public void testSaveNoRequiredField_OneFilled() throws Exception {
        optionalOnlyTest(FilledFields.USERNAME_ONLY);
    }

    @Test
    @AppModeFull // testAutoFillOneDatasetAndSave() is enough to test ephemeral apps
    public void testSaveNoRequiredField_BothFilled() throws Exception {
        optionalOnlyTest(FilledFields.BOTH);
    }

    enum FilledFields {
        NONE,
        USERNAME_ONLY,
        BOTH
    }

    private void optionalOnlyTest(FilledFields filledFields) throws Exception {
        enableService();

        // Set expectations.
        sReplier.addResponse(new CannedFillResponse.Builder()
                .setRequiredSavableIds(SAVE_DATA_TYPE_PASSWORD)
                .setOptionalSavableIds(ID_USERNAME, ID_PASSWORD)
                .build());

        // Trigger auto-fill.
        mActivity.onUsername(View::requestFocus);

        // Sanity check.
        mUiBot.assertNoDatasetsEver();

        // Wait for onFill() before proceeding, otherwise the fields might be changed before
        // the session started
        sReplier.getNextFillRequest();

        // Set credentials...
        final String expectedUsername;
        if (filledFields == FilledFields.USERNAME_ONLY || filledFields == FilledFields.BOTH) {
            expectedUsername = BACKDOOR_USERNAME;
            mActivity.onUsername((v) -> v.setText(BACKDOOR_USERNAME));
        } else {
            expectedUsername = "";
        }
        mActivity.onPassword(View::requestFocus);
        if (filledFields == FilledFields.BOTH) {
            mActivity.onPassword((v) -> v.setText("whatever"));
        }

        // ...and login
        final String expectedMessage = getWelcomeMessage(expectedUsername);
        final String actualMessage = mActivity.tapLogin();
        assertWithMessage("Wrong welcome msg").that(actualMessage).isEqualTo(expectedMessage);

        if (filledFields == FilledFields.NONE) {
            mUiBot.assertSaveNotShowing(SAVE_DATA_TYPE_PASSWORD);
            return;
        }

        // Assert the snack bar is shown and tap "Save".
        mUiBot.saveForAutofill(true, SAVE_DATA_TYPE_PASSWORD);

        final SaveRequest saveRequest = sReplier.getNextSaveRequest();

        // Assert value of expected fields - should not be sanitized.
        final ViewNode username = findNodeByResourceId(saveRequest.structure, ID_USERNAME);
        assertTextAndValue(username, BACKDOOR_USERNAME);

        if (filledFields == FilledFields.BOTH) {
            final ViewNode password = findNodeByResourceId(saveRequest.structure, ID_PASSWORD);
            assertTextAndValue(password, "whatever");
        }
    }

    @Test
    @AppModeFull // testAutoFillOneDatasetAndSave() is enough to test ephemeral apps
    public void testGenericSave() throws Exception {
        customizedSaveTest(SAVE_DATA_TYPE_GENERIC);
    }

    @Test
    @AppModeFull // testAutoFillOneDatasetAndSave() is enough to test ephemeral apps
    public void testCustomizedSavePassword() throws Exception {
        customizedSaveTest(SAVE_DATA_TYPE_PASSWORD);
    }

    @Test
    @AppModeFull // testAutoFillOneDatasetAndSave() is enough to test ephemeral apps
    public void testCustomizedSaveAddress() throws Exception {
        customizedSaveTest(SAVE_DATA_TYPE_ADDRESS);
    }

    @Test
    @AppModeFull // testAutoFillOneDatasetAndSave() is enough to test ephemeral apps
    public void testCustomizedSaveCreditCard() throws Exception {
        customizedSaveTest(SAVE_DATA_TYPE_CREDIT_CARD);
    }

    @Test
    @AppModeFull // testAutoFillOneDatasetAndSave() is enough to test ephemeral apps
    public void testCustomizedSaveUsername() throws Exception {
        customizedSaveTest(SAVE_DATA_TYPE_USERNAME);
    }

    @Test
    @AppModeFull // testAutoFillOneDatasetAndSave() is enough to test ephemeral apps
    public void testCustomizedSaveEmailAddress() throws Exception {
        customizedSaveTest(SAVE_DATA_TYPE_EMAIL_ADDRESS);
    }

    private void customizedSaveTest(int type) throws Exception {
        // Set service.
        enableService();

        // Set expectations.
        final String saveDescription = "Your data will be saved with love and care...";
        sReplier.addResponse(new CannedFillResponse.Builder()
                .setRequiredSavableIds(type, ID_USERNAME, ID_PASSWORD)
                .setSaveDescription(saveDescription)
                .build());

        // Trigger auto-fill.
        mActivity.onUsername(View::requestFocus);

        // Sanity check.
        mUiBot.assertNoDatasetsEver();

        // Wait for onFill() before proceeding, otherwise the fields might be changed before
        // the session started.
        sReplier.getNextFillRequest();

        // Set credentials...
        mActivity.onUsername((v) -> v.setText("malkovich"));
        mActivity.onPassword((v) -> v.setText("malkovich"));

        // ...and login
        final String expectedMessage = getWelcomeMessage("malkovich");
        final String actualMessage = mActivity.tapLogin();
        assertWithMessage("Wrong welcome msg").that(actualMessage).isEqualTo(expectedMessage);

        // Assert the snack bar is shown and tap "Save".
        final UiObject2 saveSnackBar = mUiBot.assertSaveShowing(saveDescription, type);
        mUiBot.saveForAutofill(saveSnackBar, true);

        // Assert save was called.
        sReplier.getNextSaveRequest();
    }

    @Test
    public void testDontTriggerSaveOnFinishWhenRequestedByFlag() throws Exception {
        enableService();

        // Set expectations.
        sReplier.addResponse(new CannedFillResponse.Builder()
                .setRequiredSavableIds(SAVE_DATA_TYPE_PASSWORD, ID_USERNAME, ID_PASSWORD)
                .setSaveInfoFlags(SaveInfo.FLAG_DONT_SAVE_ON_FINISH)
                .build());

        // Trigger auto-fill.
        mActivity.onUsername(View::requestFocus);

        // Sanity check.
        mUiBot.assertNoDatasetsEver();

        // Wait for onFill() before proceeding, otherwise the fields might be changed before
        // the session started
        sReplier.getNextFillRequest();

        // Set credentials...
        mActivity.onUsername((v) -> v.setText("malkovich"));
        mActivity.onPassword((v) -> v.setText("malkovich"));

        // ...and login
        final String expectedMessage = getWelcomeMessage("malkovich");
        final String actualMessage = mActivity.tapLogin();
        assertWithMessage("Wrong welcome msg").that(actualMessage).isEqualTo(expectedMessage);

        // Make sure it didn't trigger save.
        mUiBot.assertSaveNotShowing(SAVE_DATA_TYPE_PASSWORD);
    }

    @Test
    public void testAutoFillOneDatasetAndSaveWhenFlagSecure() throws Exception {
        mActivity.setFlags(FLAG_SECURE);
        testAutoFillOneDatasetAndSave();
    }

    @Test
    public void testAutoFillOneDatasetWhenFlagSecure() throws Exception {
        mActivity.setFlags(FLAG_SECURE);
        testAutoFillOneDataset();
    }

    @Test
    @AppModeFull // Service-specific test
    public void testDisableSelf() throws Exception {
        enableService();

        // Can disable while connected.
        mActivity.runOnUiThread(() -> mContext.getSystemService(
                AutofillManager.class).disableAutofillServices());

        // Ensure disabled.
        assertServiceDisabled();
    }

    @Test
    public void testRejectStyleNegativeSaveButton() throws Exception {
        enableService();

        // Set service behavior.

        final String intentAction = "android.autofillservice.cts.CUSTOM_ACTION";

        // Configure the save UI.
        final IntentSender listener = PendingIntent.getBroadcast(
                mContext, 0, new Intent(intentAction), 0).getIntentSender();

        sReplier.addResponse(new CannedFillResponse.Builder()
                .setRequiredSavableIds(SAVE_DATA_TYPE_PASSWORD, ID_USERNAME, ID_PASSWORD)
                .setNegativeAction(SaveInfo.NEGATIVE_BUTTON_STYLE_REJECT, listener)
                .build());

        // Trigger auto-fill.
        mActivity.onUsername(View::requestFocus);

        // Wait for onFill() before proceeding.
        sReplier.getNextFillRequest();

        // Trigger save.
        mActivity.onUsername((v) -> v.setText("foo"));
        mActivity.onPassword((v) -> v.setText("foo"));
        mActivity.tapLogin();

        // Start watching for the negative intent
        final CountDownLatch latch = new CountDownLatch(1);
        final IntentFilter intentFilter = new IntentFilter(intentAction);
        mContext.registerReceiver(new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                mContext.unregisterReceiver(this);
                latch.countDown();
            }
        }, intentFilter);

        // Trigger the negative button.
        mUiBot.saveForAutofill(SaveInfo.NEGATIVE_BUTTON_STYLE_REJECT,
                false, SAVE_DATA_TYPE_PASSWORD);

        // Wait for the custom action.
        assertThat(latch.await(5, TimeUnit.SECONDS)).isTrue();
    }

    @Test
    public void testCancelStyleNegativeSaveButton() throws Exception {
        enableService();

        // Set service behavior.

        final String intentAction = "android.autofillservice.cts.CUSTOM_ACTION";

        // Configure the save UI.
        final IntentSender listener = PendingIntent.getBroadcast(
                mContext, 0, new Intent(intentAction), 0).getIntentSender();

        sReplier.addResponse(new CannedFillResponse.Builder()
                .setRequiredSavableIds(SAVE_DATA_TYPE_PASSWORD, ID_USERNAME, ID_PASSWORD)
                .setNegativeAction(SaveInfo.NEGATIVE_BUTTON_STYLE_CANCEL, listener)
                .build());

        // Trigger auto-fill.
        mActivity.onUsername(View::requestFocus);

        // Wait for onFill() before proceeding.
        sReplier.getNextFillRequest();

        // Trigger save.
        mActivity.onUsername((v) -> v.setText("foo"));
        mActivity.onPassword((v) -> v.setText("foo"));
        mActivity.tapLogin();

        // Start watching for the negative intent
        final CountDownLatch latch = new CountDownLatch(1);
        final IntentFilter intentFilter = new IntentFilter(intentAction);
        mContext.registerReceiver(new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                mContext.unregisterReceiver(this);
                latch.countDown();
            }
        }, intentFilter);

        // Trigger the negative button.
        mUiBot.saveForAutofill(SaveInfo.NEGATIVE_BUTTON_STYLE_CANCEL,
                false, SAVE_DATA_TYPE_PASSWORD);

        // Wait for the custom action.
        assertThat(latch.await(500, TimeUnit.SECONDS)).isTrue();
    }

    @Test
    @AppModeFull // Unit test
    public void testGetTextInputType() throws Exception {
        // Set service.
        enableService();

        // Set expectations.
        sReplier.addResponse(NO_RESPONSE);

        // Trigger auto-fill.
        mActivity.onUsername(View::requestFocus);

        // Assert input text on fill request:
        final FillRequest fillRequest = sReplier.getNextFillRequest();

        final ViewNode label = findNodeByResourceId(fillRequest.structure, ID_PASSWORD_LABEL);
        assertThat(label.getInputType()).isEqualTo(TYPE_NULL);
        final ViewNode password = findNodeByResourceId(fillRequest.structure, ID_PASSWORD);
        assertWithMessage("No TYPE_TEXT_VARIATION_PASSWORD on %s", password.getInputType())
                .that(password.getInputType() & TYPE_TEXT_VARIATION_PASSWORD)
                .isEqualTo(TYPE_TEXT_VARIATION_PASSWORD);
    }

    @Test
    @AppModeFull // Unit test
    public void testNoContainers() throws Exception {
        // Set service.
        enableService();

        // Set expectations.
        sReplier.addResponse(NO_RESPONSE);

        // Trigger auto-fill.
        mActivity.onUsername(View::requestFocus);

        mUiBot.assertNoDatasetsEver();

        final FillRequest fillRequest = sReplier.getNextFillRequest();

        // Assert it only has 1 root view with 10 "leaf" nodes:
        // 1.text view for app title
        // 2.username text label
        // 3.username text field
        // 4.password text label
        // 5.password text field
        // 6.output text field
        // 7.clear button
        // 8.save button
        // 9.login button
        // 10.cancel button
        //
        // But it also has an intermediate container (for username) that should be included because
        // it has a resource id.

        assertNumberOfChildren(fillRequest.structure, 12);

        // Make sure container with a resource id was included:
        final ViewNode usernameContainer = findNodeByResourceId(fillRequest.structure,
                ID_USERNAME_CONTAINER);
        assertThat(usernameContainer).isNotNull();
        assertThat(usernameContainer.getChildCount()).isEqualTo(2);
    }

    @Test
    public void testAutofillManuallyOneDataset() throws Exception {
        // Set service.
        enableService();

        // And activity.
        mActivity.onUsername((v) -> v.setImportantForAutofill(IMPORTANT_FOR_AUTOFILL_NO));

        // Set expectations.
        sReplier.addResponse(new CannedDataset.Builder()
                .setField(ID_USERNAME, "dude")
                .setField(ID_PASSWORD, "sweet")
                .setPresentation(createPresentation("The Dude"))
                .build());
        mActivity.expectAutoFill("dude", "sweet");

        // Explicitly uses the contextual menu to test that functionality.
        mUiBot.getAutofillMenuOption(ID_USERNAME).click();

        final FillRequest fillRequest = sReplier.getNextFillRequest();
        assertHasFlags(fillRequest.flags, FLAG_MANUAL_REQUEST);

        // Should have been automatically filled.
        mUiBot.selectDataset("The Dude");

        // Check the results.
        mActivity.assertAutoFilled();
    }

    @Test
    @AppModeFull // testAutofillManuallyOneDataset() is enough to test ephemeral apps support
    public void testAutofillManuallyTwoDatasetsPickFirst() throws Exception {
        autofillManuallyTwoDatasets(true);
    }

    @Test
    @AppModeFull // testAutofillManuallyOneDataset() is enough to test ephemeral apps support
    public void testAutofillManuallyTwoDatasetsPickSecond() throws Exception {
        autofillManuallyTwoDatasets(false);
    }

    private void autofillManuallyTwoDatasets(boolean pickFirst) throws Exception {
        // Set service.
        enableService();

        // Set expectations.
        sReplier.addResponse(new CannedFillResponse.Builder()
                .addDataset(new CannedDataset.Builder()
                        .setField(ID_USERNAME, "dude")
                        .setField(ID_PASSWORD, "sweet")
                        .setPresentation(createPresentation("The Dude"))
                        .build())
                .addDataset(new CannedDataset.Builder()
                        .setField(ID_USERNAME, "jenny")
                        .setField(ID_PASSWORD, "8675309")
                        .setPresentation(createPresentation("Jenny"))
                        .build())
                .build());
        if (pickFirst) {
            mActivity.expectAutoFill("dude", "sweet");
        } else {
            mActivity.expectAutoFill("jenny", "8675309");

        }

        // Force a manual autofill request.
        mActivity.forceAutofillOnUsername();

        final FillRequest fillRequest = sReplier.getNextFillRequest();
        assertHasFlags(fillRequest.flags, FLAG_MANUAL_REQUEST);

        // Auto-fill it.
        final UiObject2 picker = mUiBot.assertDatasets("The Dude", "Jenny");
        mUiBot.selectDataset(picker, pickFirst ? "The Dude" : "Jenny");

        // Check the results.
        mActivity.assertAutoFilled();
    }

    @Test
    @AppModeFull // testAutofillManuallyOneDataset() is enough to test ephemeral apps support
    public void testAutofillManuallyPartialField() throws Exception {
        // Set service.
        enableService();

        // And activity.
        mActivity.onUsername((v) -> v.setText("dud"));
        mActivity.onPassword((v) -> v.setText("IamSecretMan"));

        // Set expectations.
        sReplier.addResponse(new CannedDataset.Builder()
                .setField(ID_USERNAME, "dude")
                .setField(ID_PASSWORD, "sweet")
                .setPresentation(createPresentation("The Dude"))
                .build());
        mActivity.expectAutoFill("dude", "sweet");

        // Force a manual autofill request.
        mActivity.forceAutofillOnUsername();

        final FillRequest fillRequest = sReplier.getNextFillRequest();
        assertHasFlags(fillRequest.flags, FLAG_MANUAL_REQUEST);
        // Username value should be available because it triggered the manual request...
        assertValue(fillRequest.structure, ID_USERNAME, "dud");
        // ... but password didn't
        assertTextIsSanitized(fillRequest.structure, ID_PASSWORD);

        // Selects the dataset.
        mUiBot.selectDataset("The Dude");

        // Check the results.
        mActivity.assertAutoFilled();
    }

    @Test
    @AppModeFull // testAutofillManuallyOneDataset() is enough to test ephemeral apps support
    public void testAutofillManuallyAgainAfterAutomaticallyAutofilledBefore() throws Exception {
        // Set service.
        enableService();

        /*
         * 1st fill (automatic).
         */
        // Set expectations.
        sReplier.addResponse(new CannedDataset.Builder()
                .setField(ID_USERNAME, "dude")
                .setField(ID_PASSWORD, "sweet")
                .setPresentation(createPresentation("The Dude"))
                .build());
        mActivity.expectAutoFill("dude", "sweet");

        // Trigger auto-fill.
        requestFocusOnUsername();

        // Assert request.
        final FillRequest fillRequest1 = sReplier.getNextFillRequest();
        assertThat(fillRequest1.flags).isEqualTo(0);
        assertTextIsSanitized(fillRequest1.structure, ID_USERNAME);
        assertTextIsSanitized(fillRequest1.structure, ID_PASSWORD);

        // Select it.
        mUiBot.selectDataset("The Dude");

        // Check the results.
        mActivity.assertAutoFilled();

        /*
         * 2nd fill (manual).
         */
        // Set expectations.
        sReplier.addResponse(new CannedDataset.Builder()
                .setField(ID_USERNAME, "DUDE")
                .setField(ID_PASSWORD, "SWEET")
                .setPresentation(createPresentation("THE DUDE"))
                .build());
        mActivity.expectAutoFill("DUDE", "SWEET");
        // Change password to make sure it's not sent to the service.
        mActivity.onPassword((v) -> v.setText("IamSecretMan"));

        // Trigger auto-fill.
        mActivity.forceAutofillOnUsername();

        // Assert request.
        final FillRequest fillRequest2 = sReplier.getNextFillRequest();
        assertHasFlags(fillRequest2.flags, FLAG_MANUAL_REQUEST);
        assertValue(fillRequest2.structure, ID_USERNAME, "dude");
        assertTextIsSanitized(fillRequest2.structure, ID_PASSWORD);

        // Select it.
        mUiBot.selectDataset("THE DUDE");

        // Check the results.
        mActivity.assertAutoFilled();
    }

    @Test
    @AppModeFull // testAutofillManuallyOneDataset() is enough to test ephemeral apps support
    public void testAutofillManuallyAgainAfterManuallyAutofilledBefore() throws Exception {
        // Set service.
        enableService();

        /*
         * 1st fill (manual).
         */
        // Set expectations.
        sReplier.addResponse(new CannedDataset.Builder()
                .setField(ID_USERNAME, "dude")
                .setField(ID_PASSWORD, "sweet")
                .setPresentation(createPresentation("The Dude"))
                .build());
        mActivity.expectAutoFill("dude", "sweet");

        // Trigger auto-fill.
        mActivity.forceAutofillOnUsername();

        // Assert request.
        final FillRequest fillRequest1 = sReplier.getNextFillRequest();
        assertHasFlags(fillRequest1.flags, FLAG_MANUAL_REQUEST);
        assertValue(fillRequest1.structure, ID_USERNAME, "");
        assertTextIsSanitized(fillRequest1.structure, ID_PASSWORD);

        // Select it.
        mUiBot.selectDataset("The Dude");

        // Check the results.
        mActivity.assertAutoFilled();

        /*
         * 2nd fill (manual).
         */
        // Set expectations.
        sReplier.addResponse(new CannedDataset.Builder()
                .setField(ID_USERNAME, "DUDE")
                .setField(ID_PASSWORD, "SWEET")
                .setPresentation(createPresentation("THE DUDE"))
                .build());
        mActivity.expectAutoFill("DUDE", "SWEET");
        // Change password to make sure it's not sent to the service.
        mActivity.onPassword((v) -> v.setText("IamSecretMan"));

        // Trigger auto-fill.
        mActivity.forceAutofillOnUsername();

        // Assert request.
        final FillRequest fillRequest2 = sReplier.getNextFillRequest();
        assertHasFlags(fillRequest2.flags, FLAG_MANUAL_REQUEST);
        assertValue(fillRequest2.structure, ID_USERNAME, "dude");
        assertTextIsSanitized(fillRequest2.structure, ID_PASSWORD);

        // Select it.
        mUiBot.selectDataset("THE DUDE");

        // Check the results.
        mActivity.assertAutoFilled();
    }

    @Test
    public void testCommitMultipleTimes() throws Throwable {
        // Set service.
        enableService();

        final CannedFillResponse response = new CannedFillResponse.Builder()
                .setRequiredSavableIds(SAVE_DATA_TYPE_PASSWORD, ID_USERNAME, ID_PASSWORD)
                .build();

        for (int i = 1; i <= 3; i++) {
            Log.i(TAG, "testCommitMultipleTimes(): step " + i);
            final String username = "user-" + i;
            final String password = "pass-" + i;
            try {
                // Set expectations.
                sReplier.addResponse(response);

                // Trigger auto-fill.
                mActivity.onUsername(View::requestFocus);

                // Wait for onFill() before proceeding, otherwise the fields might be changed before
                // the session started
                waitUntilConnected();
                sReplier.getNextFillRequest();

                // Sanity check.
                mUiBot.assertNoDatasetsEver();

                // Set credentials...
                mActivity.onUsername((v) -> v.setText(username));
                mActivity.onPassword((v) -> v.setText(password));

                // Change focus to prepare for next step - must do it before session is gone
                mActivity.onPassword(View::requestFocus);

                // ...and save them
                mActivity.tapSave();

                // Assert the snack bar is shown and tap "Save".
                mUiBot.saveForAutofill(true, SAVE_DATA_TYPE_PASSWORD);

                final SaveRequest saveRequest = sReplier.getNextSaveRequest();

                // Assert value of expected fields - should not be sanitized.
                final ViewNode usernameNode = findNodeByResourceId(saveRequest.structure,
                        ID_USERNAME);
                assertTextAndValue(usernameNode, username);
                final ViewNode passwordNode = findNodeByResourceId(saveRequest.structure,
                        ID_PASSWORD);
                assertTextAndValue(passwordNode, password);

                waitUntilDisconnected();
            } catch (RetryableException e) {
                throw new RetryableException(e, "on step %d", i);
            } catch (Throwable t) {
                throw new Throwable("Error on step " + i, t);
            }
        }
    }

    @Test
    public void testCancelMultipleTimes() throws Throwable {
        // Set service.
        enableService();

        for (int i = 1; i <= 3; i++) {
            Log.i(TAG, "testCancelMultipleTimes(): step " + i);
            final String username = "user-" + i;
            final String password = "pass-" + i;
            sReplier.addResponse(new CannedDataset.Builder()
                    .setField(ID_USERNAME, username)
                    .setField(ID_PASSWORD, password)
                    .setPresentation(createPresentation("The Dude"))
                    .build());
            mActivity.expectAutoFill(username, password);
            try {
                // Trigger auto-fill.
                requestFocusOnUsername();

                waitUntilConnected();
                sReplier.getNextFillRequest();

                // Auto-fill it.
                mUiBot.selectDataset("The Dude");

                // Check the results.
                mActivity.assertAutoFilled();

                // Change focus to prepare for next step - must do it before session is gone
                requestFocusOnPassword();

                // Rinse and repeat...
                mActivity.tapClear();

                waitUntilDisconnected();
            } catch (RetryableException e) {
                throw e;
            } catch (Throwable t) {
                throw new Throwable("Error on step " + i, t);
            }
        }
    }

    @Test
    public void testClickCustomButton() throws Exception {
        // Set service.
        enableService();

        Intent intent = new Intent(mContext, EmptyActivity.class);
        IntentSender sender = PendingIntent.getActivity(mContext, 0, intent,
                PendingIntent.FLAG_ONE_SHOT | PendingIntent.FLAG_CANCEL_CURRENT)
                .getIntentSender();

        RemoteViews presentation = new RemoteViews(mPackageName, R.layout.list_item);
        presentation.setTextViewText(R.id.text1, "Poke");
        Intent firstIntent = new Intent(mContext, DummyActivity.class);
        presentation.setOnClickPendingIntent(R.id.text1, PendingIntent.getActivity(
                mContext, 0, firstIntent, PendingIntent.FLAG_ONE_SHOT
                        | PendingIntent.FLAG_CANCEL_CURRENT));

        // Set expectations.
        sReplier.addResponse(new CannedFillResponse.Builder()
                .setAuthentication(sender, ID_USERNAME)
                .setPresentation(presentation)
                .build());

        // Trigger auto-fill.
        requestFocusOnUsername();

        // Wait for onFill() before proceeding.
        sReplier.getNextFillRequest();

        // Click on the custom button
        mUiBot.selectByText("Poke");

        // Make sure the click worked
        mUiBot.selectByText("foo");

        // Go back to the filled app.
        mUiBot.pressBack();
    }

    @Test
    public void testIsServiceEnabled() throws Exception {
        disableService();
        final AutofillManager afm = mActivity.getAutofillManager();
        assertThat(afm.hasEnabledAutofillServices()).isFalse();
        try {
            enableService();
            assertThat(afm.hasEnabledAutofillServices()).isTrue();
        } finally {
            disableService();
        }
    }

    @Test
    public void testGetAutofillServiceComponentName() throws Exception {
        final AutofillManager afm = mActivity.getAutofillManager();

        enableService();
        final ComponentName componentName = afm.getAutofillServiceComponentName();
        assertThat(componentName.getPackageName()).isEqualTo(SERVICE_PACKAGE);
        assertThat(componentName.getClassName()).endsWith(SERVICE_CLASS);

        disableService();
        assertThat(afm.getAutofillServiceComponentName()).isNull();
    }

    @Test
    public void testSetupComplete() throws Exception {
        enableService();

        // Sanity check.
        final AutofillManager afm = mActivity.getAutofillManager();
        assertThat(afm.isEnabled()).isTrue();

        // Now disable user_complete and try again.
        try {
            setUserComplete(mContext, false);
            assertThat(afm.isEnabled()).isFalse();
        } finally {
            setUserComplete(mContext, true);
        }
    }

    @Test
    public void testPopupGoesAwayWhenServiceIsChanged() throws Exception {
        // Set service.
        enableService();

        // Set expectations.
        sReplier.addResponse(new CannedDataset.Builder()
                .setField(ID_USERNAME, "dude")
                .setField(ID_PASSWORD, "sweet")
                .setPresentation(createPresentation("The Dude"))
                .build());
        mActivity.expectAutoFill("dude", "sweet");

        // Trigger auto-fill.
        requestFocusOnUsername();
        sReplier.getNextFillRequest();
        mUiBot.assertDatasets("The Dude");

        // Now disable service by setting another service
        Helper.enableAutofillService(mContext, NoOpAutofillService.SERVICE_NAME);

        // ...and make sure popup's gone
        mUiBot.assertNoDatasets();
    }

    // TODO(b/70682223): add a new test to make sure service with BIND_AUTOFILL permission works
    @Test
    @AppModeFull // Service-specific test
    public void testServiceIsDisabledWhenNewServiceInfoIsInvalid() throws Exception {
        serviceIsDisabledWhenNewServiceIsInvalid(BadAutofillService.SERVICE_NAME);
    }

    @Test
    @AppModeFull // Service-specific test
    public void testServiceIsDisabledWhenNewServiceNameIsInvalid() throws Exception {
        serviceIsDisabledWhenNewServiceIsInvalid("Y_U_NO_VALID");
    }

    private void serviceIsDisabledWhenNewServiceIsInvalid(String serviceName) throws Exception {
        // Set service.
        enableService();

        // Set expectations.
        sReplier.addResponse(new CannedDataset.Builder()
                .setField(ID_USERNAME, "dude")
                .setField(ID_PASSWORD, "sweet")
                .setPresentation(createPresentation("The Dude"))
                .build());
        mActivity.expectAutoFill("dude", "sweet");

        // Trigger autofill.
        requestFocusOnUsername();
        sReplier.getNextFillRequest();
        mUiBot.assertDatasets("The Dude");

        // Now disable service by setting another service...
        Helper.enableAutofillService(mContext, serviceName);

        // ...and make sure popup's gone
        mUiBot.assertNoDatasets();

        // Then try to trigger autofill again...
        mActivity.onPassword(View::requestFocus);
        //...it should not work!
        mUiBot.assertNoDatasetsEver();
    }

    @Test
    public void testAutofillMovesCursorToTheEnd() throws Exception {
        // Set service.
        enableService();

        // Set expectations.
        sReplier.addResponse(new CannedDataset.Builder()
                .setField(ID_USERNAME, "dude")
                .setField(ID_PASSWORD, "sweet")
                .setPresentation(createPresentation("The Dude"))
                .build());
        mActivity.expectAutoFill("dude", "sweet");

        // Trigger auto-fill.
        requestFocusOnUsername();
        sReplier.getNextFillRequest();

        // Auto-fill it.
        mUiBot.selectDataset("The Dude");

        // Check the results.
        mActivity.assertAutoFilled();

        // NOTE: need to call getSelectionEnd() inside the UI thread, otherwise it returns 0
        final AtomicInteger atomicBombToKillASmallInsect = new AtomicInteger();

        mActivity.onUsername((v) -> atomicBombToKillASmallInsect.set(v.getSelectionEnd()));
        assertWithMessage("Wrong position on username").that(atomicBombToKillASmallInsect.get())
                .isEqualTo(4);

        mActivity.onPassword((v) -> atomicBombToKillASmallInsect.set(v.getSelectionEnd()));
        assertWithMessage("Wrong position on password").that(atomicBombToKillASmallInsect.get())
                .isEqualTo(5);
    }

    @Test
    public void testAutofillLargeNumberOfDatasets() throws Exception {
        // Set service.
        enableService();

        final StringBuilder bigStringBuilder = new StringBuilder();
        for (int i = 0; i < 10_000 ; i++) {
            bigStringBuilder.append("BigAmI");
        }
        final String bigString = bigStringBuilder.toString();

        final int size = 100;
        Log.d(TAG, "testAutofillLargeNumberOfDatasets(): " + size + " datasets with "
                + bigString.length() +"-bytes id");

        final CannedFillResponse.Builder response = new CannedFillResponse.Builder();
        for (int i = 0; i < size; i++) {
            final String suffix = "-" + (i + 1);
            response.addDataset(new CannedDataset.Builder()
                    .setField(ID_USERNAME, "user" + suffix)
                    .setField(ID_PASSWORD, "pass" + suffix)
                    .setId(bigString)
                    .setPresentation(createPresentation("DS" + suffix))
                    .build());
        }

        // Set expectations.
        sReplier.addResponse(response.build());

        // Trigger auto-fill.
        requestFocusOnUsername();
        sReplier.getNextFillRequest();

        // Make sure all datasets are shown.
        // TODO: improve assertDatasets() so it supports scrolling, and assert all of them are
        // shown. In fullscreen there are 4 items, otherwise there are 3 items.
        mUiBot.assertDatasetsContains("DS-1", "DS-2", "DS-3");

        // TODO: once it supports scrolling, selects the last dataset and asserts it's filled.
    }

    @Test
    public void testCancellationSignalCalledAfterTimeout() throws Exception {
        // Set service.
        enableService();

        // Set expectations.
        final OneTimeCancellationSignalListener listener =
                new OneTimeCancellationSignalListener(Timeouts.FILL_TIMEOUT.ms() + 2000);
        sReplier.addResponse(DO_NOT_REPLY_RESPONSE);

        // Trigger auto-fill.
        mActivity.onUsername(View::requestFocus);

        // Attach listener to CancellationSignal.
        waitUntilConnected();
        sReplier.getNextFillRequest().cancellationSignal.setOnCancelListener(listener);

        // Assert results
        listener.assertOnCancelCalled();
    }

    @Test
    @AppModeFull // Unit test
    public void testNewTextAttributes() throws Exception {
        enableService();
        sReplier.addResponse(NO_RESPONSE);
        mActivity.onUsername(View::requestFocus);

        final FillRequest request = sReplier.getNextFillRequest();
        final ViewNode username = findNodeByResourceId(request.structure, ID_USERNAME);
        assertThat(username.getMinTextEms()).isEqualTo(2);
        assertThat(username.getMaxTextEms()).isEqualTo(5);
        assertThat(username.getMaxTextLength()).isEqualTo(25);

        final ViewNode container = findNodeByResourceId(request.structure, ID_USERNAME_CONTAINER);
        assertThat(container.getMinTextEms()).isEqualTo(-1);
        assertThat(container.getMaxTextEms()).isEqualTo(-1);
        assertThat(container.getMaxTextLength()).isEqualTo(-1);

        final ViewNode password = findNodeByResourceId(request.structure, ID_PASSWORD);
        assertThat(password.getMinTextEms()).isEqualTo(-1);
        assertThat(password.getMaxTextEms()).isEqualTo(-1);
        assertThat(password.getMaxTextLength()).isEqualTo(-1);
    }
}
