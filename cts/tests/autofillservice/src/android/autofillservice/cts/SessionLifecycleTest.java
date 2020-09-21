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

import static android.autofillservice.cts.Helper.ID_LOGIN;
import static android.autofillservice.cts.Helper.ID_PASSWORD;
import static android.autofillservice.cts.Helper.ID_USERNAME;
import static android.autofillservice.cts.Helper.assertTextAndValue;
import static android.autofillservice.cts.Helper.findNodeByResourceId;
import static android.autofillservice.cts.Helper.getContext;
import static android.autofillservice.cts.OutOfProcessLoginActivity.getStartedMarker;
import static android.autofillservice.cts.OutOfProcessLoginActivity.getStoppedMarker;
import static android.autofillservice.cts.UiBot.LANDSCAPE;
import static android.autofillservice.cts.UiBot.PORTRAIT;
import static android.autofillservice.cts.common.ShellHelper.runShellCommand;
import static android.service.autofill.SaveInfo.SAVE_DATA_TYPE_PASSWORD;
import static android.service.autofill.SaveInfo.SAVE_DATA_TYPE_USERNAME;

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;

import static org.junit.Assume.assumeTrue;

import android.app.PendingIntent;
import android.app.assist.AssistStructure;
import android.content.Intent;
import android.content.IntentSender;
import android.os.Bundle;
import android.os.SystemClock;
import android.platform.test.annotations.AppModeFull;
import android.support.test.uiautomator.UiObject2;
import android.view.autofill.AutofillValue;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import java.util.concurrent.Callable;

/**
 * Test the lifecycle of a autofill session
 */
@AppModeFull // This test requires android.permission.WRITE_EXTERNAL_STORAGE
public class SessionLifecycleTest extends AutoFillServiceTestCase {
    private static final String ID_BUTTON = "button";
    private static final String ID_CANCEL = "cancel";

    /**
     * Delay for activity start/stop.
     */
    // TODO: figure out a better way to wait without using sleep().
    private static final long WAIT_ACTIVITY_MS = 1000;

    private static final Timeout SESSION_LIFECYCLE_TIMEOUT = new Timeout(
            "SESSION_LIFECYCLE_TIMEOUT", 2000, 2F, 5000);

    /**
     * Runs an {@code assertion}, retrying until {@code timeout} is reached.
     */
    private static void eventually(String description, Callable<Boolean> assertion)
            throws Exception {
        SESSION_LIFECYCLE_TIMEOUT.run(description, assertion);
    }

    public SessionLifecycleTest() {
        super(new UiBot(SESSION_LIFECYCLE_TIMEOUT));
    }

    /**
     * Prevents the screen to rotate by itself
     */
    @Before
    public void disableAutoRotation() throws Exception {
        Helper.disableAutoRotation(mUiBot);
    }

    /**
     * Allows the screen to rotate by itself
     */
    @After
    public void allowAutoRotation() {
        Helper.allowAutoRotation();
    }

    private void killOfProcessLoginActivityProcess() throws Exception {
        // Waiting for activity to stop (stop marker appears)
        eventually("getStoppedMarker()", () -> {
            return getStoppedMarker(getContext()).exists();
        });

        // onStop might not be finished, hence wait more
        SystemClock.sleep(WAIT_ACTIVITY_MS);

        // Kill activity that is in the background
        runShellCommand("am broadcast --receiver-foreground "
                + "-n android.autofillservice.cts/.SelfDestructReceiver");
    }

    private void startAndWaitExternalActivity() throws Exception {
        final Intent outOfProcessAcvitityStartIntent = new Intent(getContext(),
                OutOfProcessLoginActivity.class).setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        getStartedMarker(getContext()).delete();
        getContext().startActivity(outOfProcessAcvitityStartIntent);
        eventually("getStartedMarker()", () -> {
            return getStartedMarker(getContext()).exists();
        });
        getStartedMarker(getContext()).delete();
        // Even if we wait the activity started, UiObject still fails. Have to wait a little bit.
        SystemClock.sleep(WAIT_ACTIVITY_MS);

        mUiBot.assertShownByRelativeId(ID_USERNAME);
    }

    @Test
    public void testDatasetAuthResponseWhileAutofilledAppIsLifecycled() throws Exception {
        assumeTrue("Rotation is supported", Helper.isRotationSupported(mContext));

        // Set service.
        enableService();

        // Start activity that is autofilled in a separate process so it can be killed
        startAndWaitExternalActivity();

        // Set expectations.
        final Bundle extras = new Bundle();
        extras.putString("numbers", "4815162342");

        // Create the authentication intent (launching a full screen activity)
        IntentSender authentication = PendingIntent.getActivity(getContext(), 0,
                new Intent(getContext(), ManualAuthenticationActivity.class),
                0).getIntentSender();

        // Prepare the authenticated response
        ManualAuthenticationActivity.setResponse(new CannedFillResponse.Builder()
                .addDataset(new CannedFillResponse.CannedDataset.Builder()
                        .setField(ID_USERNAME, AutofillValue.forText("autofilled username"))
                        .setPresentation(createPresentation("dataset")).build())
                .setRequiredSavableIds(SAVE_DATA_TYPE_PASSWORD, ID_USERNAME, ID_PASSWORD)
                .setExtras(extras).build());

        CannedFillResponse response = new CannedFillResponse.Builder()
                .setAuthentication(authentication, ID_USERNAME, ID_PASSWORD)
                .setPresentation(createPresentation("authenticate"))
                .build();
        sReplier.addResponse(response);

        // Trigger autofill on username
        mUiBot.selectByRelativeId(ID_USERNAME);

        // Wait for fill request to be processed
        sReplier.getNextFillRequest();

        // Wait until authentication is shown
        mUiBot.assertDatasets("authenticate");

        // Change orientation which triggers a destroy -> create in the app as the activity
        // cannot deal with such situations
        mUiBot.setScreenOrientation(LANDSCAPE);
        mUiBot.setScreenOrientation(PORTRAIT);

        // Wait context and Views being recreated in rotation
        mUiBot.assertShownByRelativeId(ID_USERNAME);

        // Delete stopped marker
        getStoppedMarker(getContext()).delete();

        // Authenticate
        mUiBot.selectDataset("authenticate");

        // Kill activity that is in the background
        killOfProcessLoginActivityProcess();

        // Change orientation which triggers a destroy -> create in the app as the activity
        // cannot deal with such situations
        mUiBot.setScreenOrientation(PORTRAIT);

        // Approve authentication
        mUiBot.selectByRelativeId(ID_BUTTON);

        // Wait for dataset to be shown
        mUiBot.assertDatasets("dataset");

        // Change orientation which triggers a destroy -> create in the app as the activity
        // cannot deal with such situations
        mUiBot.setScreenOrientation(LANDSCAPE);

        // Select dataset
        mUiBot.selectDataset("dataset");

        // Check the results.
        eventually("getTextById(" + ID_USERNAME + ")", () -> {
            return mUiBot.getTextByRelativeId(ID_USERNAME).equals("autofilled username");
        });

        // Set password
        mUiBot.setTextByRelativeId(ID_PASSWORD, "new password");

        // Login
        mUiBot.selectByRelativeId(ID_LOGIN);

        // Wait for save UI to be shown
        mUiBot.assertSaveShowing(SAVE_DATA_TYPE_PASSWORD);

        // Change orientation to make sure save UI can handle this
        mUiBot.setScreenOrientation(PORTRAIT);

        // Tap "Save".
        mUiBot.saveForAutofill(true, SAVE_DATA_TYPE_PASSWORD);

        // Get save request
        InstrumentedAutoFillService.SaveRequest saveRequest = sReplier.getNextSaveRequest();
        assertWithMessage("onSave() not called").that(saveRequest).isNotNull();

        // Make sure data is correctly saved
        final AssistStructure.ViewNode username = findNodeByResourceId(saveRequest.structure,
                ID_USERNAME);
        assertTextAndValue(username, "autofilled username");
        final AssistStructure.ViewNode password = findNodeByResourceId(saveRequest.structure,
                ID_PASSWORD);
        assertTextAndValue(password, "new password");

        // Make sure extras were passed back on onSave()
        assertThat(saveRequest.data).isNotNull();
        final String extraValue = saveRequest.data.getString("numbers");
        assertWithMessage("extras not passed on save").that(extraValue).isEqualTo("4815162342");
    }

    @Test
    public void testAuthCanceledWhileAutofilledAppIsLifecycled() throws Exception {
        // Set service.
        enableService();

        // Start activity that is autofilled in a separate process so it can be killed
        startAndWaitExternalActivity();

        // Create the authentication intent (launching a full screen activity)
        IntentSender authentication = PendingIntent.getActivity(getContext(), 0,
                new Intent(getContext(), ManualAuthenticationActivity.class),
                0).getIntentSender();

        CannedFillResponse response = new CannedFillResponse.Builder()
                .setAuthentication(authentication, ID_USERNAME, ID_PASSWORD)
                .setPresentation(createPresentation("authenticate"))
                .build();
        sReplier.addResponse(response);

        // Trigger autofill on username
        mUiBot.selectByRelativeId(ID_USERNAME);

        // Wait for fill request to be processed
        sReplier.getNextFillRequest();

        // Wait until authentication is shown
        mUiBot.assertDatasets("authenticate");

        // Delete stopped marker
        getStoppedMarker(getContext()).delete();

        // Authenticate
        mUiBot.selectDataset("authenticate");

        // Kill activity that is in the background
        killOfProcessLoginActivityProcess();

        // Cancel authentication activity
        mUiBot.pressBack();

        // Authentication should still be shown
        mUiBot.assertDatasets("authenticate");
    }

    @Test
    public void testDatasetVisibleWhileAutofilledAppIsLifecycled() throws Exception {
        // Set service.
        enableService();

        // Start activity that is autofilled in a separate process so it can be killed
        startAndWaitExternalActivity();

        CannedFillResponse response = new CannedFillResponse.Builder()
                .addDataset(new CannedFillResponse.CannedDataset.Builder(
                        createPresentation("dataset"))
                                .setField(ID_USERNAME, "filled").build())
                .build();
        sReplier.addResponse(response);

        // Trigger autofill on username
        mUiBot.selectByRelativeId(ID_USERNAME);

        // Wait for fill request to be processed
        sReplier.getNextFillRequest();

        // Wait until dataset is shown
        mUiBot.assertDatasets("dataset");

        // Delete stopped marker
        getStoppedMarker(getContext()).delete();

        // Start an activity on top of the autofilled activity
        Intent intent = new Intent(getContext(), EmptyActivity.class);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);

        getContext().startActivity(intent);

        // Kill activity that is in the background
        killOfProcessLoginActivityProcess();

        // Cancel activity on top
        mUiBot.pressBack();

        // Dataset should still be shown
        mUiBot.assertDatasets("dataset");
    }

    @Test
    public void testAutofillNestedActivitiesWhileAutofilledAppIsLifecycled() throws Exception {
        // Set service.
        enableService();

        // Start activity that is autofilled in a separate process so it can be killed
        startAndWaitExternalActivity();

        // Prepare response for first activity
        CannedFillResponse response = new CannedFillResponse.Builder()
                .addDataset(new CannedFillResponse.CannedDataset.Builder(
                        createPresentation("dataset1"))
                                .setField(ID_USERNAME, "filled").build())
                .build();
        sReplier.addResponse(response);

        // Trigger autofill on username
        mUiBot.selectByRelativeId(ID_USERNAME);

        // Wait for fill request to be processed
        sReplier.getNextFillRequest();

        // Wait until dataset1 is shown
        mUiBot.assertDatasets("dataset1");

        // Delete stopped marker
        getStoppedMarker(getContext()).delete();

        // Prepare response for nested activity
        response = new CannedFillResponse.Builder()
                .addDataset(new CannedFillResponse.CannedDataset.Builder(
                        createPresentation("dataset2"))
                                .setField(ID_USERNAME, "filled").build())
                .build();
        sReplier.addResponse(response);

        // Start nested login activity
        Intent intent = new Intent(getContext(), LoginActivity.class);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        getContext().startActivity(intent);

        // Kill activity that is in the background
        killOfProcessLoginActivityProcess();

        // Trigger autofill on username in nested activity
        mUiBot.selectByRelativeId(ID_USERNAME);

        // Wait for fill request to be processed
        sReplier.getNextFillRequest();

        // Wait until dataset in nested activity is shown
        mUiBot.assertDatasets("dataset2");

        // Tap "Cancel".
        mUiBot.selectByRelativeId(ID_CANCEL);

        // Dataset should still be shown
        mUiBot.assertDatasets("dataset1");
    }

    @Test
    public void testDatasetGoesAwayWhenAutofilledAppIsKilled() throws Exception {
        // Set service.
        enableService();

        // Start activity that is autofilled in a separate process so it can be killed
        startAndWaitExternalActivity();

        final CannedFillResponse response = new CannedFillResponse.Builder()
                .addDataset(new CannedFillResponse.CannedDataset.Builder(
                        createPresentation("dataset"))
                                .setField(ID_USERNAME, "filled").build())
                .build();
        sReplier.addResponse(response);

        // Trigger autofill on username
        mUiBot.selectByRelativeId(ID_USERNAME);

        // Wait for fill request to be processed
        sReplier.getNextFillRequest();

        // Wait until dataset is shown
        mUiBot.assertDatasets("dataset");

        // Kill activity
        killOfProcessLoginActivityProcess();

        // Make sure dataset is not shown anymore
        mUiBot.assertNoDatasetsEver();

        // Restart activity an make sure the dataset is still not shown
        startAndWaitExternalActivity();
        mUiBot.assertNoDatasets();
    }

    @Test
    public void testSaveRemainsWhenAutofilledAppIsKilled() throws Exception {
        // Set service.
        enableService();

        // Start activity that is autofilled in a separate process so it can be killed
        startAndWaitExternalActivity();

        final CannedFillResponse response = new CannedFillResponse.Builder()
                .setRequiredSavableIds(SAVE_DATA_TYPE_USERNAME, ID_USERNAME)
                .build();
        sReplier.addResponse(response);

        // Trigger autofill on username
        mUiBot.selectByRelativeId(ID_USERNAME);

        // Wait for fill request to be processed
        sReplier.getNextFillRequest();

        // Wait until dataset is shown
        mUiBot.assertNoDatasetsEver();

        // Trigger save
        mUiBot.setTextByRelativeId(ID_USERNAME, "dude");
        mUiBot.selectByRelativeId(ID_LOGIN);
        mUiBot.assertSaveShowing(SAVE_DATA_TYPE_USERNAME);

        // Kill activity
        killOfProcessLoginActivityProcess();

        // Make sure save is still showing
        final UiObject2 saveSnackBar = mUiBot.assertSaveShowing(SAVE_DATA_TYPE_USERNAME);

        mUiBot.saveForAutofill(saveSnackBar, true);

        final InstrumentedAutoFillService.SaveRequest saveRequest = sReplier.getNextSaveRequest();

        // Make sure data is correctly saved
        final AssistStructure.ViewNode username = findNodeByResourceId(saveRequest.structure,
                ID_USERNAME);
        assertTextAndValue(username, "dude");
    }
}
