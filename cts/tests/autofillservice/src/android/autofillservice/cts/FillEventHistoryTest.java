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
import static android.autofillservice.cts.CheckoutActivity.ID_CC_NUMBER;
import static android.autofillservice.cts.Helper.ID_PASSWORD;
import static android.autofillservice.cts.Helper.ID_USERNAME;
import static android.autofillservice.cts.Helper.NULL_DATASET_ID;
import static android.autofillservice.cts.Helper.assertDeprecatedClientState;
import static android.autofillservice.cts.Helper.assertFillEventForAuthenticationSelected;
import static android.autofillservice.cts.Helper.assertFillEventForDatasetAuthenticationSelected;
import static android.autofillservice.cts.Helper.assertFillEventForDatasetSelected;
import static android.autofillservice.cts.Helper.assertFillEventForSaveShown;
import static android.autofillservice.cts.Helper.assertNoDeprecatedClientState;
import static android.autofillservice.cts.InstrumentedAutoFillService.waitUntilConnected;
import static android.autofillservice.cts.InstrumentedAutoFillService.waitUntilDisconnected;
import static android.autofillservice.cts.LoginActivity.BACKDOOR_USERNAME;
import static android.autofillservice.cts.LoginActivity.getWelcomeMessage;
import static android.service.autofill.FillEventHistory.Event.TYPE_CONTEXT_COMMITTED;
import static android.service.autofill.SaveInfo.SAVE_DATA_TYPE_GENERIC;
import static android.service.autofill.SaveInfo.SAVE_DATA_TYPE_PASSWORD;

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;

import android.autofillservice.cts.CannedFillResponse.CannedDataset;
import android.content.Intent;
import android.content.IntentSender;
import android.os.Bundle;
import android.platform.test.annotations.AppModeFull;
import android.service.autofill.FillEventHistory;
import android.service.autofill.FillEventHistory.Event;
import android.service.autofill.FillResponse;
import android.support.test.uiautomator.UiObject2;
import android.view.View;
import android.view.autofill.AutofillId;

import com.google.common.collect.ImmutableMap;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;

import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * Test that uses {@link LoginActivity} to test {@link FillEventHistory}.
 */
@AppModeFull // Service-specific test
public class FillEventHistoryTest extends AutoFillServiceTestCase {

    @Rule
    public final AutofillActivityTestRule<LoginActivity> mActivityRule =
            new AutofillActivityTestRule<LoginActivity>(LoginActivity.class);

    private LoginActivity mActivity;

    @Before
    public void setActivity() {
        mActivity = mActivityRule.getActivity();
    }

    @Test
    public void testDatasetAuthenticationSelected() throws Exception {
        enableService();

        // Set up FillResponse with dataset authentication
        Bundle clientState = new Bundle();
        clientState.putCharSequence("clientStateKey", "clientStateValue");

        // Prepare the authenticated response
        final IntentSender authentication = AuthenticationActivity.createSender(mContext, 1,
                new CannedDataset.Builder()
                        .setField(ID_USERNAME, "dude")
                        .setField(ID_PASSWORD, "sweet")
                        .setPresentation(createPresentation("Dataset"))
                        .build());

        sReplier.addResponse(new CannedFillResponse.Builder().addDataset(
                new CannedDataset.Builder()
                        .setField(ID_USERNAME, "username")
                        .setId("name")
                        .setPresentation(createPresentation("authentication"))
                        .setAuthentication(authentication)
                        .build())
                .setExtras(clientState).build());
        mActivity.expectAutoFill("dude", "sweet");

        // Trigger autofill.
        mActivity.onUsername(View::requestFocus);

        // Authenticate
        sReplier.getNextFillRequest();
        mUiBot.selectDataset("authentication");
        mActivity.assertAutoFilled();

        // Verify fill selection
        final List<Event> events = InstrumentedAutoFillService.getFillEvents(1);
        assertFillEventForDatasetAuthenticationSelected(events.get(0), "name",
                "clientStateKey", "clientStateValue");
    }

    @Test
    public void testAuthenticationSelected() throws Exception {
        enableService();

        // Set up FillResponse with response wide authentication
        Bundle clientState = new Bundle();
        clientState.putCharSequence("clientStateKey", "clientStateValue");

        // Prepare the authenticated response
        final IntentSender authentication = AuthenticationActivity.createSender(mContext, 1,
                new CannedFillResponse.Builder().addDataset(
                        new CannedDataset.Builder()
                                .setField(ID_USERNAME, "username")
                                .setId("name")
                                .setPresentation(createPresentation("dataset"))
                                .build())
                        .setExtras(clientState).build());

        sReplier.addResponse(new CannedFillResponse.Builder().setExtras(clientState)
                .setPresentation(createPresentation("authentication"))
                .setAuthentication(authentication, ID_USERNAME)
                .build());

        // Trigger autofill.
        mActivity.onUsername(View::requestFocus);

        // Authenticate
        sReplier.getNextFillRequest();
        mUiBot.selectDataset("authentication");
        mUiBot.assertDatasets("dataset");

        // Verify fill selection
        final FillEventHistory selection = InstrumentedAutoFillService.getFillEventHistory(1);
        assertDeprecatedClientState(selection, "clientStateKey", "clientStateValue");
        List<Event> events = selection.getEvents();
        assertFillEventForAuthenticationSelected(events.get(0), NULL_DATASET_ID,
                "clientStateKey", "clientStateValue");
    }

    @Test
    public void testDatasetSelected_twoResponses() throws Exception {
        enableService();

        // Set up first partition with an anonymous dataset
        Bundle clientState1 = new Bundle();
        clientState1.putCharSequence("clientStateKey", "Value1");

        sReplier.addResponse(new CannedFillResponse.Builder().addDataset(
                new CannedDataset.Builder()
                        .setField(ID_USERNAME, "username")
                        .setPresentation(createPresentation("dataset1"))
                        .build())
                .setExtras(clientState1)
                .build());
        mActivity.expectAutoFill("username");

        // Trigger autofill on username
        mActivity.onUsername(View::requestFocus);
        waitUntilConnected();
        sReplier.getNextFillRequest();
        mUiBot.selectDataset("dataset1");
        mActivity.assertAutoFilled();

        {
            // Verify fill selection
            final FillEventHistory selection = InstrumentedAutoFillService.getFillEventHistory(1);
            assertDeprecatedClientState(selection, "clientStateKey", "Value1");
            final List<Event> events = selection.getEvents();
            assertFillEventForDatasetSelected(events.get(0), NULL_DATASET_ID,
                    "clientStateKey", "Value1");
        }

        // Set up second partition with a named dataset
        Bundle clientState2 = new Bundle();
        clientState2.putCharSequence("clientStateKey", "Value2");

        sReplier.addResponse(new CannedFillResponse.Builder()
                .addDataset(
                        new CannedDataset.Builder()
                                .setField(ID_PASSWORD, "password2")
                                .setPresentation(createPresentation("dataset2"))
                                .setId("name2")
                                .build())
                .addDataset(
                        new CannedDataset.Builder()
                                .setField(ID_PASSWORD, "password3")
                                .setPresentation(createPresentation("dataset3"))
                                .setId("name3")
                                .build())
                .setExtras(clientState2)
                .setRequiredSavableIds(SAVE_DATA_TYPE_GENERIC, ID_PASSWORD).build());
        mActivity.expectPasswordAutoFill("password3");

        // Trigger autofill on password
        mActivity.onPassword(View::requestFocus);
        sReplier.getNextFillRequest();
        mUiBot.selectDataset("dataset3");
        mActivity.assertAutoFilled();

        {
            // Verify fill selection
            final FillEventHistory selection = InstrumentedAutoFillService.getFillEventHistory(1);
            assertDeprecatedClientState(selection, "clientStateKey", "Value2");
            final List<Event> events = selection.getEvents();
            assertFillEventForDatasetSelected(events.get(0), "name3",
                    "clientStateKey", "Value2");
        }

        mActivity.onPassword((v) -> v.setText("new password"));
        mActivity.syncRunOnUiThread(() -> mActivity.finish());
        waitUntilDisconnected();

        {
            // Verify fill selection
            final FillEventHistory selection = InstrumentedAutoFillService.getFillEventHistory(2);
            assertDeprecatedClientState(selection, "clientStateKey", "Value2");

            final List<Event> events = selection.getEvents();
            assertFillEventForDatasetSelected(events.get(0), "name3",
                    "clientStateKey", "Value2");
            assertFillEventForSaveShown(events.get(1), NULL_DATASET_ID,
                    "clientStateKey", "Value2");
        }
    }

    @Test
    public void testNoEvents_whenServiceReturnsNullResponse() throws Exception {
        enableService();

        // First reset
        sReplier.addResponse(new CannedFillResponse.Builder().addDataset(
                new CannedDataset.Builder()
                        .setField(ID_USERNAME, "username")
                        .setPresentation(createPresentation("dataset1"))
                        .build())
                .build());
        mActivity.expectAutoFill("username");

        mActivity.onUsername(View::requestFocus);
        waitUntilConnected();
        sReplier.getNextFillRequest();
        mUiBot.selectDataset("dataset1");
        mActivity.assertAutoFilled();

        {
            // Verify fill selection
            final FillEventHistory selection = InstrumentedAutoFillService.getFillEventHistory(1);
            assertNoDeprecatedClientState(selection);
            final List<Event> events = selection.getEvents();
            assertFillEventForDatasetSelected(events.get(0), NULL_DATASET_ID);
        }

        // Second request
        sReplier.addResponse(NO_RESPONSE);
        mActivity.onPassword(View::requestFocus);
        sReplier.getNextFillRequest();
        mUiBot.assertNoDatasets();
        waitUntilDisconnected();

        InstrumentedAutoFillService.assertNoFillEventHistory();
    }

    @Test
    public void testNoEvents_whenServiceReturnsFailure() throws Exception {
        enableService();

        // First reset
        sReplier.addResponse(new CannedFillResponse.Builder().addDataset(
                new CannedDataset.Builder()
                        .setField(ID_USERNAME, "username")
                        .setPresentation(createPresentation("dataset1"))
                        .build())
                .build());
        mActivity.expectAutoFill("username");

        mActivity.onUsername(View::requestFocus);
        waitUntilConnected();
        sReplier.getNextFillRequest();
        mUiBot.selectDataset("dataset1");
        mActivity.assertAutoFilled();

        {
            // Verify fill selection
            final FillEventHistory selection = InstrumentedAutoFillService.getFillEventHistory(1);
            assertNoDeprecatedClientState(selection);
            final List<Event> events = selection.getEvents();
            assertFillEventForDatasetSelected(events.get(0), NULL_DATASET_ID);
        }

        // Second request
        sReplier.addResponse(new CannedFillResponse.Builder().returnFailure("D'OH!").build());
        mActivity.onPassword(View::requestFocus);
        sReplier.getNextFillRequest();
        mUiBot.assertNoDatasets();
        waitUntilDisconnected();

        InstrumentedAutoFillService.assertNoFillEventHistory();
    }

    @Test
    public void testNoEvents_whenServiceTimesout() throws Exception {
        enableService();

        // First reset
        sReplier.addResponse(new CannedFillResponse.Builder().addDataset(
                new CannedDataset.Builder()
                        .setField(ID_USERNAME, "username")
                        .setPresentation(createPresentation("dataset1"))
                        .build())
                .build());
        mActivity.expectAutoFill("username");

        mActivity.onUsername(View::requestFocus);
        waitUntilConnected();
        sReplier.getNextFillRequest();
        mUiBot.selectDataset("dataset1");
        mActivity.assertAutoFilled();

        {
            // Verify fill selection
            final FillEventHistory selection = InstrumentedAutoFillService.getFillEventHistory(1);
            assertNoDeprecatedClientState(selection);
            final List<Event> events = selection.getEvents();
            assertFillEventForDatasetSelected(events.get(0), NULL_DATASET_ID);
        }

        // Second request
        sReplier.addResponse(DO_NOT_REPLY_RESPONSE);
        mActivity.onPassword(View::requestFocus);
        sReplier.getNextFillRequest();
        waitUntilDisconnected();

        InstrumentedAutoFillService.assertNoFillEventHistory();
    }

    private Bundle getBundle(String key, String value) {
        final Bundle bundle = new Bundle();
        bundle.putString(key, value);
        return bundle;
    }

    /**
     * Tests the following scenario:
     *
     * <ol>
     *    <li>Activity A is launched.
     *    <li>Activity A triggers autofill.
     *    <li>Activity B is launched.
     *    <li>Activity B triggers autofill.
     *    <li>User goes back to Activity A.
     *    <li>User triggers save on Activity A - at this point, service should have stats of
     *        activity B, and stats for activity A should have beeen discarded.
     * </ol>
     */
    @Test
    public void testEventsFromPreviousSessionIsDiscarded() throws Exception {
        enableService();

        // Launch activity A
        sReplier.addResponse(new CannedFillResponse.Builder()
                .setExtras(getBundle("activity", "A"))
                .setRequiredSavableIds(SAVE_DATA_TYPE_PASSWORD, ID_USERNAME, ID_PASSWORD)
                .build());

        // Trigger autofill on activity A
        mActivity.onUsername(View::requestFocus);
        waitUntilConnected();
        sReplier.getNextFillRequest();

        // Verify fill selection for Activity A
        final FillEventHistory selectionA = InstrumentedAutoFillService.getFillEventHistory(0);
        assertDeprecatedClientState(selectionA, "activity", "A");

        // Launch activity B
        mActivity.startActivity(new Intent(mActivity, CheckoutActivity.class));
        mUiBot.assertShownByRelativeId(ID_CC_NUMBER);

        // Trigger autofill on activity B
        sReplier.addResponse(new CannedFillResponse.Builder()
                .setExtras(getBundle("activity", "B"))
                .addDataset(new CannedDataset.Builder()
                        .setField(ID_CC_NUMBER, "4815162342")
                        .setPresentation(createPresentation("datasetB"))
                        .build())
                .build());
        mUiBot.focusByRelativeId(ID_CC_NUMBER);
        sReplier.getNextFillRequest();

        // Verify fill selection for Activity B
        final FillEventHistory selectionB = InstrumentedAutoFillService.getFillEventHistory(0);
        assertDeprecatedClientState(selectionB, "activity", "B");

        // Now switch back to A...
        mUiBot.pressBack(); // dismiss autofill
        mUiBot.pressBack(); // dismiss keyboard (or task, if there was no keyboard)
        final AtomicBoolean focusOnA = new AtomicBoolean();
        mActivity.syncRunOnUiThread(() -> focusOnA.set(mActivity.hasWindowFocus()));
        if (!focusOnA.get()) {
            mUiBot.pressBack(); // dismiss task, if the last pressBack dismissed only the keyboard
        }
        mUiBot.assertShownByRelativeId(ID_USERNAME);
        assertWithMessage("root window has no focus")
                .that(mActivity.getWindow().getDecorView().hasWindowFocus()).isTrue();

        // ...and trigger save
        // Set credentials...
        mActivity.onUsername((v) -> v.setText("malkovich"));
        mActivity.onPassword((v) -> v.setText("malkovich"));
        final String expectedMessage = getWelcomeMessage("malkovich");
        final String actualMessage = mActivity.tapLogin();
        assertWithMessage("Wrong welcome msg").that(actualMessage).isEqualTo(expectedMessage);
        mUiBot.saveForAutofill(true, SAVE_DATA_TYPE_PASSWORD);
        sReplier.getNextSaveRequest();

        // Finally, make sure history is right
        final FillEventHistory finalSelection = InstrumentedAutoFillService.getFillEventHistory(0);
        assertDeprecatedClientState(finalSelection, "activity", "B");
    }

    @Test
    public void testContextCommitted_whenServiceDidntDoAnything() throws Exception {
        enableService();

        sReplier.addResponse(CannedFillResponse.NO_RESPONSE);

        // Trigger autofill on username
        mActivity.onUsername(View::requestFocus);
        sReplier.getNextFillRequest();
        mUiBot.assertNoDatasetsEver();

        // Trigger save
        mActivity.onUsername((v) -> v.setText("malkovich"));
        mActivity.onPassword((v) -> v.setText("malkovich"));
        final String expectedMessage = getWelcomeMessage("malkovich");
        final String actualMessage = mActivity.tapLogin();
        assertWithMessage("Wrong welcome msg").that(actualMessage).isEqualTo(expectedMessage);
        mUiBot.assertSaveNotShowing(SAVE_DATA_TYPE_PASSWORD);

        // Assert no events where generated
        InstrumentedAutoFillService.assertNoFillEventHistory();
    }

    @Test
    public void textContextCommitted_withoutDatasets() throws Exception {
        enableService();

        sReplier.addResponse(new CannedFillResponse.Builder()
                .setFillResponseFlags(FillResponse.FLAG_TRACK_CONTEXT_COMMITED)
                .setRequiredSavableIds(SAVE_DATA_TYPE_PASSWORD, ID_USERNAME, ID_PASSWORD)
                .build());

        // Trigger autofill on username
        mActivity.onUsername(View::requestFocus);
        sReplier.getNextFillRequest();
        mUiBot.assertNoDatasetsEver();

        // Trigger save
        mActivity.onUsername((v) -> v.setText("malkovich"));
        mActivity.onPassword((v) -> v.setText("malkovich"));
        final String expectedMessage = getWelcomeMessage("malkovich");
        final String actualMessage = mActivity.tapLogin();
        assertWithMessage("Wrong welcome msg").that(actualMessage).isEqualTo(expectedMessage);
        mUiBot.saveForAutofill(true, SAVE_DATA_TYPE_PASSWORD);
        sReplier.getNextSaveRequest();

        // Assert it
        final List<Event> events = InstrumentedAutoFillService.getFillEvents(1);
        assertFillEventForSaveShown(events.get(0), NULL_DATASET_ID);
    }

    @Test
    public void testContextCommitted_withoutFlagOnLastResponse() throws Exception {
        enableService();
        // Trigger 1st autofill request
        sReplier.addResponse(new CannedFillResponse.Builder().addDataset(
                new CannedDataset.Builder()
                        .setId("id1")
                        .setField(ID_USERNAME, BACKDOOR_USERNAME)
                        .setPresentation(createPresentation("dataset1"))
                        .build())
                .setFillResponseFlags(FillResponse.FLAG_TRACK_CONTEXT_COMMITED)
                .build());
        mActivity.expectAutoFill(BACKDOOR_USERNAME);
        mActivity.onUsername(View::requestFocus);
        sReplier.getNextFillRequest();
        mUiBot.selectDataset("dataset1");
        mActivity.assertAutoFilled();
        // Verify fill history
        {
            final List<Event> events = InstrumentedAutoFillService.getFillEvents(1);
            assertFillEventForDatasetSelected(events.get(0), "id1");
        }

        // Trigger 2st autofill request (which will clear the fill event history)
        sReplier.addResponse(new CannedFillResponse.Builder().addDataset(
                new CannedDataset.Builder()
                        .setId("id2")
                        .setField(ID_PASSWORD, "whatever")
                        .setPresentation(createPresentation("dataset2"))
                        .build())
                // don't set flags
                .build());
        mActivity.expectPasswordAutoFill("whatever");
        mActivity.onPassword(View::requestFocus);
        sReplier.getNextFillRequest();
        mUiBot.selectDataset("dataset2");
        mActivity.assertAutoFilled();
        // Verify fill history
        {
            final List<Event> events = InstrumentedAutoFillService.getFillEvents(1);
            assertFillEventForDatasetSelected(events.get(0), "id2");
        }

        // Finish the context by login in
        final String expectedMessage = getWelcomeMessage(BACKDOOR_USERNAME);
        final String actualMessage = mActivity.tapLogin();
        assertWithMessage("Wrong welcome msg").that(actualMessage).isEqualTo(expectedMessage);
        mUiBot.assertSaveNotShowing(SAVE_DATA_TYPE_PASSWORD);

        {
            // Verify fill history
            final List<Event> events = InstrumentedAutoFillService.getFillEvents(1);

            assertFillEventForDatasetSelected(events.get(0), "id2");
        }
    }

    @Test
    public void testContextCommitted_idlessDatasets() throws Exception {
        enableService();

        sReplier.addResponse(new CannedFillResponse.Builder().addDataset(
                new CannedDataset.Builder()
                        .setField(ID_USERNAME, "username1")
                        .setField(ID_PASSWORD, "password1")
                        .setPresentation(createPresentation("dataset1"))
                        .build())
                .addDataset(new CannedDataset.Builder()
                        .setField(ID_USERNAME, "username2")
                        .setField(ID_PASSWORD, "password2")
                        .setPresentation(createPresentation("dataset2"))
                        .build())
                .setFillResponseFlags(FillResponse.FLAG_TRACK_CONTEXT_COMMITED)
                .build());
        mActivity.expectAutoFill("username1", "password1");

        // Trigger autofill on username
        mActivity.onUsername(View::requestFocus);
        sReplier.getNextFillRequest();

        final UiObject2 datasetPicker = mUiBot.assertDatasets("dataset1", "dataset2");
        mUiBot.selectDataset(datasetPicker, "dataset1");
        mActivity.assertAutoFilled();

        // Verify dataset selection
        {
            final List<Event> events = InstrumentedAutoFillService.getFillEvents(1);
            assertFillEventForDatasetSelected(events.get(0), NULL_DATASET_ID);
        }

        // Finish the context by login in
        mActivity.onUsername((v) -> v.setText("USERNAME"));
        mActivity.onPassword((v) -> v.setText("USERNAME"));

        final String expectedMessage = getWelcomeMessage("USERNAME");
        final String actualMessage = mActivity.tapLogin();
        assertWithMessage("Wrong welcome msg").that(actualMessage).isEqualTo(expectedMessage);
        mUiBot.assertSaveNotShowing(SAVE_DATA_TYPE_PASSWORD);

        // ...and check again
        {
            final List<Event> events = InstrumentedAutoFillService.getFillEvents(1);
            assertFillEventForDatasetSelected(events.get(0), NULL_DATASET_ID);
        }
    }

    @Test
    public void testContextCommitted_idlessDatasetSelected_datasetWithIdIgnored()
            throws Exception {
        enableService();

        sReplier.addResponse(new CannedFillResponse.Builder().addDataset(
                new CannedDataset.Builder()
                        .setField(ID_USERNAME, "username1")
                        .setField(ID_PASSWORD, "password1")
                        .setPresentation(createPresentation("dataset1"))
                        .build())
                .addDataset(new CannedDataset.Builder()
                        .setId("id2")
                        .setField(ID_USERNAME, "username2")
                        .setField(ID_PASSWORD, "password2")
                        .setPresentation(createPresentation("dataset2"))
                        .build())
                .setFillResponseFlags(FillResponse.FLAG_TRACK_CONTEXT_COMMITED)
                .build());
        mActivity.expectAutoFill("username1", "password1");

        // Trigger autofill on username
        mActivity.onUsername(View::requestFocus);
        sReplier.getNextFillRequest();

        final UiObject2 datasetPicker = mUiBot.assertDatasets("dataset1", "dataset2");
        mUiBot.selectDataset(datasetPicker, "dataset1");
        mActivity.assertAutoFilled();

        // Verify dataset selection
        {
            final List<Event> events = InstrumentedAutoFillService.getFillEvents(1);
            assertFillEventForDatasetSelected(events.get(0), NULL_DATASET_ID);
        }

        // Finish the context by login in
        mActivity.onPassword((v) -> v.setText("username1"));

        final String expectedMessage = getWelcomeMessage("username1");
        final String actualMessage = mActivity.tapLogin();
        assertWithMessage("Wrong welcome msg").that(actualMessage).isEqualTo(expectedMessage);
        mUiBot.assertSaveNotShowing(SAVE_DATA_TYPE_PASSWORD);

        // ...and check again
        {
            final List<Event> events = InstrumentedAutoFillService.getFillEvents(2);
            assertFillEventForDatasetSelected(events.get(0), NULL_DATASET_ID);

            FillEventHistory.Event event2 = events.get(1);
            assertThat(event2.getType()).isEqualTo(TYPE_CONTEXT_COMMITTED);
            assertThat(event2.getDatasetId()).isNull();
            assertThat(event2.getClientState()).isNull();
            assertThat(event2.getSelectedDatasetIds()).isEmpty();
            assertThat(event2.getIgnoredDatasetIds()).containsExactly("id2");
            final AutofillId passwordId = mActivity.getPassword().getAutofillId();
            final Map<AutofillId, String> changedFields = event2.getChangedFields();
            assertThat(changedFields).containsExactly(passwordId, "id2");
            assertThat(event2.getManuallyEnteredField()).isEmpty();
        }
    }

    @Test
    public void testContextCommitted_idlessDatasetIgnored_datasetWithIdSelected()
            throws Exception {
        enableService();

        sReplier.addResponse(new CannedFillResponse.Builder().addDataset(
                new CannedDataset.Builder()
                        .setField(ID_USERNAME, "username1")
                        .setField(ID_PASSWORD, "password1")
                        .setPresentation(createPresentation("dataset1"))
                        .build())
                .addDataset(new CannedDataset.Builder()
                        .setId("id2")
                        .setField(ID_USERNAME, "username2")
                        .setField(ID_PASSWORD, "password2")
                        .setPresentation(createPresentation("dataset2"))
                        .build())
                .setFillResponseFlags(FillResponse.FLAG_TRACK_CONTEXT_COMMITED)
                .build());
        mActivity.expectAutoFill("username2", "password2");

        // Trigger autofill on username
        mActivity.onUsername(View::requestFocus);
        sReplier.getNextFillRequest();

        final UiObject2 datasetPicker = mUiBot.assertDatasets("dataset1", "dataset2");
        mUiBot.selectDataset(datasetPicker, "dataset2");
        mActivity.assertAutoFilled();

        // Verify dataset selection
        {
            final List<Event> events = InstrumentedAutoFillService.getFillEvents(1);
            assertFillEventForDatasetSelected(events.get(0), "id2");
        }

        // Finish the context by login in
        mActivity.onPassword((v) -> v.setText("username2"));

        final String expectedMessage = getWelcomeMessage("username2");
        final String actualMessage = mActivity.tapLogin();
        assertWithMessage("Wrong welcome msg").that(actualMessage).isEqualTo(expectedMessage);
        mUiBot.assertSaveNotShowing(SAVE_DATA_TYPE_PASSWORD);

        // ...and check again
        {
            final List<Event> events = InstrumentedAutoFillService.getFillEvents(2);
            assertFillEventForDatasetSelected(events.get(0), "id2");

            final FillEventHistory.Event event2 = events.get(1);
            assertThat(event2.getType()).isEqualTo(TYPE_CONTEXT_COMMITTED);
            assertThat(event2.getDatasetId()).isNull();
            assertThat(event2.getClientState()).isNull();
            assertThat(event2.getSelectedDatasetIds()).containsExactly("id2");
            assertThat(event2.getIgnoredDatasetIds()).isEmpty();
            final AutofillId passwordId = mActivity.getPassword().getAutofillId();
            final Map<AutofillId, String> changedFields = event2.getChangedFields();
            assertThat(changedFields).containsExactly(passwordId, "id2");
            assertThat(event2.getManuallyEnteredField()).isEmpty();
        }
    }

    /**
     * Tests scenario where the context was committed, no dataset was selected by the user,
     * neither the user entered values that were present in these datasets.
     */
    @Test
    public void testContextCommitted_noDatasetSelected_valuesNotManuallyEntered() throws Exception {
        enableService();

        sReplier.addResponse(new CannedFillResponse.Builder().addDataset(
                new CannedDataset.Builder()
                        .setId("id1")
                        .setField(ID_USERNAME, "username1")
                        .setField(ID_PASSWORD, "password1")
                        .setPresentation(createPresentation("dataset1"))
                        .build())
                .addDataset(new CannedDataset.Builder()
                        .setId("id2")
                        .setField(ID_USERNAME, "username2")
                        .setField(ID_PASSWORD, "password2")
                        .setPresentation(createPresentation("dataset2"))
                        .build())
                .setFillResponseFlags(FillResponse.FLAG_TRACK_CONTEXT_COMMITED)
                .build());
        // Trigger autofill on username
        mActivity.onUsername(View::requestFocus);
        sReplier.getNextFillRequest();
        mUiBot.assertDatasets("dataset1", "dataset2");

        // Verify history
        InstrumentedAutoFillService.getFillEventHistory(0);

        // Enter values not present at the datasets
        mActivity.onUsername((v) -> v.setText("USERNAME"));
        mActivity.onPassword((v) -> v.setText("USERNAME"));

        // Finish the context by login in
        final String expectedMessage = getWelcomeMessage("USERNAME");
        final String actualMessage = mActivity.tapLogin();
        assertWithMessage("Wrong welcome msg").that(actualMessage).isEqualTo(expectedMessage);
        mUiBot.assertSaveNotShowing(SAVE_DATA_TYPE_PASSWORD);

        // Verify history again
        {
            final List<Event> events = InstrumentedAutoFillService.getFillEvents(1);
            final Event event = events.get(0);
            assertThat(event.getType()).isEqualTo(TYPE_CONTEXT_COMMITTED);
            assertThat(event.getDatasetId()).isNull();
            assertThat(event.getClientState()).isNull();
            assertThat(event.getIgnoredDatasetIds()).containsExactly("id1", "id2");
            assertThat(event.getChangedFields()).isEmpty();
            assertThat(event.getManuallyEnteredField()).isEmpty();
        }
    }

    /**
     * Tests scenario where the context was committed, just one dataset was selected by the user,
     * and the user changed the values provided by the service.
     */
    @Test
    public void testContextCommitted_oneDatasetSelected() throws Exception {
        enableService();

        sReplier.addResponse(new CannedFillResponse.Builder().addDataset(
                new CannedDataset.Builder()
                        .setId("id1")
                        .setField(ID_USERNAME, "username1")
                        .setField(ID_PASSWORD, "password1")
                        .setPresentation(createPresentation("dataset1"))
                        .build())
                .addDataset(new CannedDataset.Builder()
                        .setId("id2")
                        .setField(ID_USERNAME, "username2")
                        .setField(ID_PASSWORD, "password2")
                        .setPresentation(createPresentation("dataset2"))
                        .build())
                .setFillResponseFlags(FillResponse.FLAG_TRACK_CONTEXT_COMMITED)
                .build());
        mActivity.expectAutoFill("username1", "password1");

        // Trigger autofill on username
        mActivity.onUsername(View::requestFocus);
        sReplier.getNextFillRequest();

        final UiObject2 datasetPicker = mUiBot.assertDatasets("dataset1", "dataset2");
        mUiBot.selectDataset(datasetPicker, "dataset1");
        mActivity.assertAutoFilled();

        // Verify dataset selection
        {
            final List<Event> events = InstrumentedAutoFillService.getFillEvents(1);
            assertFillEventForDatasetSelected(events.get(0), "id1");
        }

        // Finish the context by login in
        mActivity.onUsername((v) -> v.setText("USERNAME"));
        mActivity.onPassword((v) -> v.setText("USERNAME"));

        final String expectedMessage = getWelcomeMessage("USERNAME");
        final String actualMessage = mActivity.tapLogin();
        assertWithMessage("Wrong welcome msg").that(actualMessage).isEqualTo(expectedMessage);
        mUiBot.assertSaveNotShowing(SAVE_DATA_TYPE_PASSWORD);

        // ...and check again
        {
            final List<Event> events = InstrumentedAutoFillService.getFillEvents(2);
            assertFillEventForDatasetSelected(events.get(0), "id1");

            final FillEventHistory.Event event2 = events.get(1);
            assertThat(event2.getType()).isEqualTo(TYPE_CONTEXT_COMMITTED);
            assertThat(event2.getDatasetId()).isNull();
            assertThat(event2.getClientState()).isNull();
            assertThat(event2.getSelectedDatasetIds()).containsExactly("id1");
            assertThat(event2.getIgnoredDatasetIds()).containsExactly("id2");
            final Map<AutofillId, String> changedFields = event2.getChangedFields();
            final AutofillId usernameId = mActivity.getUsername().getAutofillId();
            final AutofillId passwordId = mActivity.getPassword().getAutofillId();
            assertThat(changedFields).containsExactlyEntriesIn(
                    ImmutableMap.of(usernameId, "id1", passwordId, "id1"));
            assertThat(event2.getManuallyEnteredField()).isEmpty();
        }
    }

    /**
     * Tests scenario where the context was committed, both datasets were selected by the user,
     * and the user changed the values provided by the service.
     */
    @Test
    public void testContextCommitted_multipleDatasetsSelected() throws Exception {
        enableService();

        sReplier.addResponse(new CannedFillResponse.Builder().addDataset(
                new CannedDataset.Builder()
                        .setId("id1")
                        .setField(ID_USERNAME, "username")
                        .setPresentation(createPresentation("dataset1"))
                        .build())
                .addDataset(new CannedDataset.Builder()
                        .setId("id2")
                        .setField(ID_PASSWORD, "password")
                        .setPresentation(createPresentation("dataset2"))
                        .build())
                .setFillResponseFlags(FillResponse.FLAG_TRACK_CONTEXT_COMMITED)
                .build());
        mActivity.expectAutoFill("username");

        // Trigger autofill
        mActivity.onUsername(View::requestFocus);
        sReplier.getNextFillRequest();

        // Autofill username
        mUiBot.selectDataset("dataset1");
        mActivity.assertAutoFilled();
        {
            // Verify fill history
            final List<Event> events = InstrumentedAutoFillService.getFillEvents(1);
            assertFillEventForDatasetSelected(events.get(0), "id1");
        }

        // Autofill password
        mActivity.expectPasswordAutoFill("password");

        mActivity.onPassword(View::requestFocus);
        mUiBot.selectDataset("dataset2");
        mActivity.assertAutoFilled();

        {
            // Verify fill history
            final List<Event> events = InstrumentedAutoFillService.getFillEvents(2);

            assertFillEventForDatasetSelected(events.get(0), "id1");
            assertFillEventForDatasetSelected(events.get(1), "id2");
        }

        // Finish the context by login in
        mActivity.onPassword((v) -> v.setText("username"));

        final String expectedMessage = getWelcomeMessage("username");
        final String actualMessage = mActivity.tapLogin();
        assertWithMessage("Wrong welcome msg").that(actualMessage).isEqualTo(expectedMessage);
        mUiBot.assertSaveNotShowing(SAVE_DATA_TYPE_PASSWORD);

        {
            // Verify fill history
            final List<Event> events = InstrumentedAutoFillService.getFillEvents(3);

            assertFillEventForDatasetSelected(events.get(0), "id1");
            assertFillEventForDatasetSelected(events.get(1), "id2");

            final FillEventHistory.Event event3 = events.get(2);
            assertThat(event3.getType()).isEqualTo(TYPE_CONTEXT_COMMITTED);
            assertThat(event3.getDatasetId()).isNull();
            assertThat(event3.getClientState()).isNull();
            assertThat(event3.getSelectedDatasetIds()).containsExactly("id1", "id2");
            assertThat(event3.getIgnoredDatasetIds()).isEmpty();
            final Map<AutofillId, String> changedFields = event3.getChangedFields();
            final AutofillId passwordId = mActivity.getPassword().getAutofillId();
            assertThat(changedFields).containsExactly(passwordId, "id2");
            assertThat(event3.getManuallyEnteredField()).isEmpty();
        }
    }

    /**
     * Tests scenario where the context was committed, both datasets were selected by the user,
     * and the user didn't change the values provided by the service.
     */
    @Test
    public void testContextCommitted_multipleDatasetsSelected_butNotChanged() throws Exception {
        enableService();

        sReplier.addResponse(new CannedFillResponse.Builder().addDataset(
                new CannedDataset.Builder()
                        .setId("id1")
                        .setField(ID_USERNAME, BACKDOOR_USERNAME)
                        .setPresentation(createPresentation("dataset1"))
                        .build())
                .addDataset(new CannedDataset.Builder()
                        .setId("id2")
                        .setField(ID_PASSWORD, "whatever")
                        .setPresentation(createPresentation("dataset2"))
                        .build())
                .setFillResponseFlags(FillResponse.FLAG_TRACK_CONTEXT_COMMITED)
                .build());
        mActivity.expectAutoFill(BACKDOOR_USERNAME);

        // Trigger autofill
        mActivity.onUsername(View::requestFocus);
        sReplier.getNextFillRequest();

        // Autofill username
        mUiBot.selectDataset("dataset1");
        mActivity.assertAutoFilled();
        {
            // Verify fill history
            final List<Event> events = InstrumentedAutoFillService.getFillEvents(1);
            assertFillEventForDatasetSelected(events.get(0), "id1");
        }

        // Autofill password
        mActivity.expectPasswordAutoFill("whatever");

        mActivity.onPassword(View::requestFocus);
        mUiBot.selectDataset("dataset2");
        mActivity.assertAutoFilled();

        {
            // Verify fill history
            final List<Event> events = InstrumentedAutoFillService.getFillEvents(2);

            assertFillEventForDatasetSelected(events.get(0), "id1");
            assertFillEventForDatasetSelected(events.get(1), "id2");
        }

        // Finish the context by login in
        final String expectedMessage = getWelcomeMessage(BACKDOOR_USERNAME);
        final String actualMessage = mActivity.tapLogin();
        assertWithMessage("Wrong welcome msg").that(actualMessage).isEqualTo(expectedMessage);
        mUiBot.assertSaveNotShowing(SAVE_DATA_TYPE_PASSWORD);

        {
            // Verify fill history
            final List<Event> events = InstrumentedAutoFillService.getFillEvents(3);

            assertFillEventForDatasetSelected(events.get(0), "id1");
            assertFillEventForDatasetSelected(events.get(1), "id2");

            final FillEventHistory.Event event3 = events.get(2);
            assertThat(event3.getType()).isEqualTo(TYPE_CONTEXT_COMMITTED);
            assertThat(event3.getDatasetId()).isNull();
            assertThat(event3.getClientState()).isNull();
            assertThat(event3.getSelectedDatasetIds()).containsExactly("id1", "id2");
            assertThat(event3.getIgnoredDatasetIds()).isEmpty();
            assertThat(event3.getChangedFields()).isEmpty();
            assertThat(event3.getManuallyEnteredField()).isEmpty();
        }
    }

    /**
     * Tests scenario where the context was committed, the user selected the dataset, than changed
     * the autofilled values, but then change the values again so they match what was provided by
     * the service.
     */
    @Test
    public void testContextCommitted_oneDatasetSelected_Changed_thenChangedBack()
            throws Exception {
        enableService();

        sReplier.addResponse(new CannedFillResponse.Builder().addDataset(
                new CannedDataset.Builder()
                        .setId("id1")
                        .setField(ID_USERNAME, "username")
                        .setField(ID_PASSWORD, "username")
                        .setPresentation(createPresentation("dataset1"))
                        .build())
                .setFillResponseFlags(FillResponse.FLAG_TRACK_CONTEXT_COMMITED)
                .build());
        mActivity.expectAutoFill("username", "username");

        // Trigger autofill on username
        mActivity.onUsername(View::requestFocus);
        sReplier.getNextFillRequest();

        mUiBot.selectDataset("dataset1");
        mActivity.assertAutoFilled();

        // Verify dataset selection
        {
            final List<Event> events = InstrumentedAutoFillService.getFillEvents(1);
            assertFillEventForDatasetSelected(events.get(0), "id1");
        }

        // Change the fields to different values from datasets
        mActivity.onUsername((v) -> v.setText("USERNAME"));
        mActivity.onPassword((v) -> v.setText("USERNAME"));

        // Then change back to dataset values
        mActivity.onUsername((v) -> v.setText("username"));
        mActivity.onPassword((v) -> v.setText("username"));

        final String expectedMessage = getWelcomeMessage("username");
        final String actualMessage = mActivity.tapLogin();
        assertWithMessage("Wrong welcome msg").that(actualMessage).isEqualTo(expectedMessage);
        mUiBot.assertSaveNotShowing(SAVE_DATA_TYPE_PASSWORD);

        // ...and check again
        {
            final List<Event> events = InstrumentedAutoFillService.getFillEvents(2);
            assertFillEventForDatasetSelected(events.get(0), "id1");

            FillEventHistory.Event event2 = events.get(1);
            assertThat(event2.getType()).isEqualTo(TYPE_CONTEXT_COMMITTED);
            assertThat(event2.getDatasetId()).isNull();
            assertThat(event2.getClientState()).isNull();
            assertThat(event2.getSelectedDatasetIds()).containsExactly("id1");
            assertThat(event2.getIgnoredDatasetIds()).isEmpty();
            assertThat(event2.getChangedFields()).isEmpty();
            assertThat(event2.getManuallyEnteredField()).isEmpty();
        }
    }

    /**
     * Tests scenario where the context was committed, the user did not selected any dataset, but
     * the user manually entered values that match what was provided by the service.
     */
    @Test
    public void testContextCommitted_noDatasetSelected_butManuallyEntered()
            throws Exception {
        enableService();

        sReplier.addResponse(new CannedFillResponse.Builder().addDataset(
                new CannedDataset.Builder()
                        .setId("id1")
                        .setField(ID_USERNAME, BACKDOOR_USERNAME)
                        .setField(ID_PASSWORD, "NotUsedPassword")
                        .setPresentation(createPresentation("dataset1"))
                        .build())
                .addDataset(new CannedDataset.Builder()
                        .setId("id2")
                        .setField(ID_USERNAME, "NotUserUsername")
                        .setField(ID_PASSWORD, "whatever")
                        .setPresentation(createPresentation("dataset2"))
                        .build())
                .setFillResponseFlags(FillResponse.FLAG_TRACK_CONTEXT_COMMITED)
                .build());
        // Trigger autofill on username
        mActivity.onUsername(View::requestFocus);
        sReplier.getNextFillRequest();
        mUiBot.assertDatasets("dataset1", "dataset2");

        // Verify history
        InstrumentedAutoFillService.getFillEventHistory(0);

        // Enter values present at the datasets
        mActivity.onUsername((v) -> v.setText(BACKDOOR_USERNAME));
        mActivity.onPassword((v) -> v.setText("whatever"));

        // Finish the context by login in
        final String expectedMessage = getWelcomeMessage(BACKDOOR_USERNAME);
        final String actualMessage = mActivity.tapLogin();
        assertWithMessage("Wrong welcome msg").that(actualMessage).isEqualTo(expectedMessage);
        mUiBot.assertSaveNotShowing(SAVE_DATA_TYPE_PASSWORD);

        // Verify history
        {
            final List<Event> events = InstrumentedAutoFillService.getFillEvents(1);
            FillEventHistory.Event event = events.get(0);
            assertThat(event.getType()).isEqualTo(TYPE_CONTEXT_COMMITTED);
            assertThat(event.getDatasetId()).isNull();
            assertThat(event.getClientState()).isNull();
            assertThat(event.getSelectedDatasetIds()).isEmpty();
            assertThat(event.getIgnoredDatasetIds()).containsExactly("id1", "id2");
            assertThat(event.getChangedFields()).isEmpty();
            final AutofillId usernameId = mActivity.getUsername().getAutofillId();
            final AutofillId passwordId = mActivity.getPassword().getAutofillId();

            final Map<AutofillId, Set<String>> manuallyEnteredFields =
                    event.getManuallyEnteredField();
            assertThat(manuallyEnteredFields).isNotNull();
            assertThat(manuallyEnteredFields.size()).isEqualTo(2);
            assertThat(manuallyEnteredFields.get(usernameId)).containsExactly("id1");
            assertThat(manuallyEnteredFields.get(passwordId)).containsExactly("id2");
        }
    }

    /**
     * Tests scenario where the context was committed, the user did not selected any dataset, but
     * the user manually entered values that match what was provided by the service on different
     * datasets.
     */
    @Test
    public void testContextCommitted_noDatasetSelected_butManuallyEntered_matchingMultipleDatasets()
            throws Exception {
        enableService();

        sReplier.addResponse(new CannedFillResponse.Builder().addDataset(
                new CannedDataset.Builder()
                        .setId("id1")
                        .setField(ID_USERNAME, BACKDOOR_USERNAME)
                        .setField(ID_PASSWORD, "NotUsedPassword")
                        .setPresentation(createPresentation("dataset1"))
                        .build())
                .addDataset(new CannedDataset.Builder()
                        .setId("id2")
                        .setField(ID_USERNAME, "NotUserUsername")
                        .setField(ID_PASSWORD, "whatever")
                        .setPresentation(createPresentation("dataset2"))
                        .build())
                .addDataset(new CannedDataset.Builder()
                        .setId("id3")
                        .setField(ID_USERNAME, BACKDOOR_USERNAME)
                        .setField(ID_PASSWORD, "whatever")
                        .setPresentation(createPresentation("dataset3"))
                        .build())
                .setFillResponseFlags(FillResponse.FLAG_TRACK_CONTEXT_COMMITED)
                .build());
        // Trigger autofill on username
        mActivity.onUsername(View::requestFocus);
        sReplier.getNextFillRequest();
        mUiBot.assertDatasets("dataset1", "dataset2", "dataset3");

        // Verify history
        InstrumentedAutoFillService.getFillEventHistory(0);

        // Enter values present at the datasets
        mActivity.onUsername((v) -> v.setText(BACKDOOR_USERNAME));
        mActivity.onPassword((v) -> v.setText("whatever"));

        // Finish the context by login in
        final String expectedMessage = getWelcomeMessage(BACKDOOR_USERNAME);
        final String actualMessage = mActivity.tapLogin();
        assertWithMessage("Wrong welcome msg").that(actualMessage).isEqualTo(expectedMessage);
        mUiBot.assertSaveNotShowing(SAVE_DATA_TYPE_PASSWORD);

        // Verify history
        {
            final List<Event> events = InstrumentedAutoFillService.getFillEvents(1);

            final FillEventHistory.Event event = events.get(0);
            assertThat(event.getType()).isEqualTo(TYPE_CONTEXT_COMMITTED);
            assertThat(event.getDatasetId()).isNull();
            assertThat(event.getClientState()).isNull();
            assertThat(event.getSelectedDatasetIds()).isEmpty();
            assertThat(event.getIgnoredDatasetIds()).containsExactly("id1", "id2", "id3");
            assertThat(event.getChangedFields()).isEmpty();
            final AutofillId usernameId = mActivity.getUsername().getAutofillId();
            final AutofillId passwordId = mActivity.getPassword().getAutofillId();

            final Map<AutofillId, Set<String>> manuallyEnteredFields =
                    event.getManuallyEnteredField();
            assertThat(manuallyEnteredFields.size()).isEqualTo(2);
            assertThat(manuallyEnteredFields.get(usernameId)).containsExactly("id1", "id3");
            assertThat(manuallyEnteredFields.get(passwordId)).containsExactly("id2", "id3");
        }
    }
}
