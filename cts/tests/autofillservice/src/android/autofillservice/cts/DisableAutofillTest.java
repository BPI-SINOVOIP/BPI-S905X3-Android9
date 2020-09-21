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

import android.autofillservice.cts.CannedFillResponse.CannedDataset;
import android.content.Intent;
import android.os.SystemClock;
import android.platform.test.annotations.AppModeFull;
import android.service.autofill.FillResponse;
import android.util.Log;

import org.junit.Test;

/**
 * Tests for the {@link android.service.autofill.FillResponse.Builder#disableAutofill(long)} API.
 */
public class DisableAutofillTest extends AutoFillServiceTestCase {

    private static final String TAG = "DisableAutofillTest";

    private SimpleSaveActivity startSimpleSaveActivity() throws Exception {
        final Intent intent = new Intent(mContext, SimpleSaveActivity.class)
                .setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        mContext.startActivity(intent);
        mUiBot.assertShownByRelativeId(SimpleSaveActivity.ID_LABEL);
        return SimpleSaveActivity.getInstance();
    }

    private PreSimpleSaveActivity startPreSimpleSaveActivity() throws Exception {
        final Intent intent = new Intent(mContext, PreSimpleSaveActivity.class)
                .setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        mContext.startActivity(intent);
        mUiBot.assertShownByRelativeId(PreSimpleSaveActivity.ID_PRE_LABEL);
        return PreSimpleSaveActivity.getInstance();
    }

    /**
     * Defines what to do after the activity being tested is launched.
     */
    enum PostLaunchAction {
        /**
         * Used when the service disables autofill in the fill response for this activty. As such:
         *
         * <ol>
         *   <li>There should be a fill request on {@code sReplier}.
         *   <li>The first UI focus should generate a
         *   {@link android.view.autofill.AutofillManager.AutofillCallback#EVENT_INPUT_UNAVAILABLE}
         *   event.
         *   <li>Subsequent UI focus should not trigger events.
         * </ol>
         */
        ASSERT_DISABLING,

        /**
         * Used when the service already disabled autofill prior to launching activty. As such:
         *
         * <ol>
         *   <li>There should be no fill request on {@code sReplier}.
         *   <li>There should be no callback calls when UI is focused
         * </ol>
         */
        ASSERT_DISABLED,

        /**
         * Used when autofill is enabled, so it tries to autofill the activity.
         */
        ASSERT_ENABLED_AND_AUTOFILL
    }

    private void launchSimpleSaveActivity(PostLaunchAction action) throws Exception {
        Log.v(TAG, "launchPreSimpleSaveActivity(): " + action);
        sReplier.assertNoUnhandledFillRequests();

        if (action == PostLaunchAction.ASSERT_ENABLED_AND_AUTOFILL) {
            sReplier.addResponse(new CannedFillResponse.Builder()
                    .addDataset(new CannedDataset.Builder()
                            .setField(SimpleSaveActivity.ID_INPUT, "id")
                            .setField(SimpleSaveActivity.ID_PASSWORD, "pass")
                            .setPresentation(createPresentation("YO"))
                            .build())
                    .build());

        }

        final SimpleSaveActivity activity = startSimpleSaveActivity();
        final MyAutofillCallback callback = activity.registerCallback();

        try {
            // Trigger autofill
            activity.syncRunOnUiThread(() -> activity.mInput.requestFocus());

            if (action == PostLaunchAction.ASSERT_DISABLING) {
                callback.assertUiUnavailableEvent(activity.mInput);
                sReplier.getNextFillRequest();

                // Make sure other fields are not triggered.
                activity.syncRunOnUiThread(() -> activity.mPassword.requestFocus());
                callback.assertNotCalled();
            } else if (action == PostLaunchAction.ASSERT_DISABLED) {
                // Make sure forced requests are ignored as well.
                activity.getAutofillManager().requestAutofill(activity.mInput);
                callback.assertNotCalled();
            } else if (action == PostLaunchAction.ASSERT_ENABLED_AND_AUTOFILL) {
                callback.assertUiShownEvent(activity.mInput);
                sReplier.getNextFillRequest();
                final SimpleSaveActivity.FillExpectation autofillExpectation =
                        activity.expectAutoFill("id", "pass");
                mUiBot.selectDataset("YO");
                autofillExpectation.assertAutoFilled();
            }

            // Asserts isEnabled() status.
            assertAutofillEnabled(activity, action == PostLaunchAction.ASSERT_ENABLED_AND_AUTOFILL);
        } finally {
            activity.finish();
        }
    }

    private void launchPreSimpleSaveActivity(PostLaunchAction action) throws Exception {
        Log.v(TAG, "launchPreSimpleSaveActivity(): " + action);
        sReplier.assertNoUnhandledFillRequests();

        if (action == PostLaunchAction.ASSERT_ENABLED_AND_AUTOFILL) {
            sReplier.addResponse(new CannedFillResponse.Builder()
                    .addDataset(new CannedDataset.Builder()
                            .setField(PreSimpleSaveActivity.ID_PRE_INPUT, "yo")
                            .setPresentation(createPresentation("YO"))
                            .build())
                    .build());
        }

        final PreSimpleSaveActivity activity = startPreSimpleSaveActivity();
        final MyAutofillCallback callback = activity.registerCallback();

        try {
            // Trigger autofill
            activity.syncRunOnUiThread(() -> activity.mPreInput.requestFocus());

            if (action == PostLaunchAction.ASSERT_DISABLING) {
                callback.assertUiUnavailableEvent(activity.mPreInput);
                sReplier.getNextFillRequest();
            } else if (action == PostLaunchAction.ASSERT_DISABLED) {
                activity.getAutofillManager().requestAutofill(activity.mPreInput);
                callback.assertNotCalled();
            } else if (action == PostLaunchAction.ASSERT_ENABLED_AND_AUTOFILL) {
                callback.assertUiShownEvent(activity.mPreInput);
                sReplier.getNextFillRequest();
                final PreSimpleSaveActivity.FillExpectation autofillExpectation =
                        activity.expectAutoFill("yo");
                mUiBot.selectDataset("YO");
                autofillExpectation.assertAutoFilled();
            }

            // Asserts isEnabled() status.
            assertAutofillEnabled(activity, action == PostLaunchAction.ASSERT_ENABLED_AND_AUTOFILL);
        } finally {
            activity.finish();
        }
    }

    @Test
    public void testDisableApp() throws Exception {
        // Set service.
        enableService();

        // Set expectations.
        sReplier.addResponse(
                new CannedFillResponse.Builder().disableAutofill(Long.MAX_VALUE).build());

        // Trigger autofill for the first time.
        launchSimpleSaveActivity(PostLaunchAction.ASSERT_DISABLING);

        // Launch activity again.
        launchSimpleSaveActivity(PostLaunchAction.ASSERT_DISABLED);

        // Now try it using a different activity - should be disabled too.
        launchPreSimpleSaveActivity(PostLaunchAction.ASSERT_DISABLED);
    }

    @Test
    @AppModeFull // testDisableApp() is enough to test ephemeral apps support
    public void testDisableAppThenWaitToReenableIt() throws Exception {
        // Set service.
        enableService();

        // Need to wait the equivalent of launching 2 activities, plus some extra legging room
        final long duration = 2 * Timeouts.ACTIVITY_RESURRECTION.ms() + 500;

        // Set expectations.
        sReplier.addResponse(new CannedFillResponse.Builder().disableAutofill(duration).build());

        // Trigger autofill for the first time.
        launchSimpleSaveActivity(PostLaunchAction.ASSERT_DISABLING);

        // Launch activity again.
        launchSimpleSaveActivity(PostLaunchAction.ASSERT_DISABLED);

        // Wait for the timeout, then try again, autofilling it this time.
        SystemClock.sleep(duration + 1);
        launchSimpleSaveActivity(PostLaunchAction.ASSERT_ENABLED_AND_AUTOFILL);

        // Also try it on another activity.
        launchPreSimpleSaveActivity(PostLaunchAction.ASSERT_ENABLED_AND_AUTOFILL);
    }

    @Test
    @AppModeFull // testDisableApp() is enough to test ephemeral apps support
    public void testDisableAppThenResetServiceToReenableIt() throws Exception {
        enableService();
        sReplier.addResponse(new CannedFillResponse.Builder()
                .disableAutofill(Long.MAX_VALUE).build());

        // Trigger autofill for the first time.
        launchSimpleSaveActivity(PostLaunchAction.ASSERT_DISABLING);
        // Launch activity again.
        launchSimpleSaveActivity(PostLaunchAction.ASSERT_DISABLED);

        // Then "reset" service to re-enable autofill.
        disableService();
        enableService();

        // Try again on activity that disabled it.
        launchSimpleSaveActivity(PostLaunchAction.ASSERT_ENABLED_AND_AUTOFILL);

        // Try again on other activity.
        launchPreSimpleSaveActivity(PostLaunchAction.ASSERT_ENABLED_AND_AUTOFILL);
    }

    @Test
    public void testDisableActivity() throws Exception {
        // Set service.
        enableService();

        // Set expectations.
        sReplier.addResponse(new CannedFillResponse.Builder()
                .disableAutofill(Long.MAX_VALUE)
                .setFillResponseFlags(FillResponse.FLAG_DISABLE_ACTIVITY_ONLY)
                .build());

        // Trigger autofill for the first time.
        launchSimpleSaveActivity(PostLaunchAction.ASSERT_DISABLING);

        // Launch activity again.
        launchSimpleSaveActivity(PostLaunchAction.ASSERT_DISABLED);

        // Now try it using a different activity - should work.
        launchPreSimpleSaveActivity(PostLaunchAction.ASSERT_ENABLED_AND_AUTOFILL);
    }

    @Test
    @AppModeFull // testDisableActivity() is enough to test ephemeral apps support
    public void testDisableActivityThenWaitToReenableIt() throws Exception {
        // Set service.
        enableService();

        // Need to wait the equivalent of launching 2 activities, plus some extra legging room
        final long duration = 2 * Timeouts.ACTIVITY_RESURRECTION.ms() + 500;

        // Set expectations.
        sReplier.addResponse(new CannedFillResponse.Builder()
                .disableAutofill(duration)
                .setFillResponseFlags(FillResponse.FLAG_DISABLE_ACTIVITY_ONLY)
                .build());

        // Trigger autofill for the first time.
        launchSimpleSaveActivity(PostLaunchAction.ASSERT_DISABLING);

        // Launch activity again.
        launchSimpleSaveActivity(PostLaunchAction.ASSERT_DISABLED);

        // Make sure other app is working.
        launchPreSimpleSaveActivity(PostLaunchAction.ASSERT_ENABLED_AND_AUTOFILL);

        // Wait for the timeout, then try again, autofilling it this time.
        SystemClock.sleep(duration + 1);
        launchSimpleSaveActivity(PostLaunchAction.ASSERT_ENABLED_AND_AUTOFILL);
    }

    @Test
    @AppModeFull // testDisableActivity() is enough to test ephemeral apps support
    public void testDisableActivityThenResetServiceToReenableIt() throws Exception {
        enableService();
        sReplier.addResponse(new CannedFillResponse.Builder()
                .disableAutofill(Long.MAX_VALUE)
                .setFillResponseFlags(FillResponse.FLAG_DISABLE_ACTIVITY_ONLY)
                .build());

        // Trigger autofill for the first time.
        launchSimpleSaveActivity(PostLaunchAction.ASSERT_DISABLING);
        // Launch activity again.
        launchSimpleSaveActivity(PostLaunchAction.ASSERT_DISABLED);

        // Make sure other app is working.
        launchPreSimpleSaveActivity(PostLaunchAction.ASSERT_ENABLED_AND_AUTOFILL);

        // Then "reset" service to re-enable autofill.
        disableService();
        enableService();

        // Try again on activity that disabled it.
        launchSimpleSaveActivity(PostLaunchAction.ASSERT_ENABLED_AND_AUTOFILL);
    }

    private void assertAutofillEnabled(AbstractAutoFillActivity activity, boolean expected)
            throws Exception {
        Timeouts.ACTIVITY_RESURRECTION.run(
                "assertAutofillEnabled(" + activity.getComponentName().flattenToShortString() + ")",
                () -> {
                    return activity.getAutofillManager().isEnabled() == expected
                            ? Boolean.TRUE : null;
                });
    }
}
