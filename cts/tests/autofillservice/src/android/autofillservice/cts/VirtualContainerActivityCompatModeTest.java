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

import static android.autofillservice.cts.Helper.ID_PASSWORD;
import static android.autofillservice.cts.Helper.ID_USERNAME;
import static android.autofillservice.cts.Helper.assertTextAndValue;
import static android.autofillservice.cts.Helper.assertTextIsSanitized;
import static android.autofillservice.cts.Helper.findNodeByResourceId;
import static android.autofillservice.cts.Helper.getContext;
import static android.autofillservice.cts.Helper.hasAutofillFeature;
import static android.autofillservice.cts.InstrumentedAutoFillServiceCompatMode.SERVICE_NAME;
import static android.autofillservice.cts.InstrumentedAutoFillServiceCompatMode.SERVICE_PACKAGE;
import static android.autofillservice.cts.VirtualContainerActivity.INITIAL_URL_BAR_VALUE;
import static android.autofillservice.cts.VirtualContainerView.ID_URL_BAR;
import static android.autofillservice.cts.VirtualContainerView.ID_URL_BAR2;
import static android.autofillservice.cts.common.SettingsHelper.NAMESPACE_GLOBAL;
import static android.provider.Settings.Global.AUTOFILL_COMPAT_MODE_ALLOWED_PACKAGES;
import static android.service.autofill.SaveInfo.SAVE_DATA_TYPE_PASSWORD;

import static com.google.common.truth.Truth.assertThat;

import android.app.assist.AssistStructure.ViewNode;
import android.autofillservice.cts.CannedFillResponse.CannedDataset;
import android.autofillservice.cts.InstrumentedAutoFillService.FillRequest;
import android.autofillservice.cts.InstrumentedAutoFillService.SaveRequest;
import android.autofillservice.cts.common.SettingsHelper;
import android.autofillservice.cts.common.SettingsStateChangerRule;
import android.content.Context;
import android.os.SystemClock;
import android.platform.test.annotations.AppModeFull;
import android.service.autofill.SaveInfo;
import android.support.test.InstrumentationRegistry;

import org.junit.After;
import org.junit.ClassRule;
import org.junit.Test;

/**
 * Test case for an activity containing virtual children but using the A11Y compat mode to implement
 * the Autofill APIs.
 */
public class VirtualContainerActivityCompatModeTest extends VirtualContainerActivityTest {
    private static final Context sContext = InstrumentationRegistry.getContext();

    @ClassRule
    public static final SettingsStateChangerRule sCompatModeChanger = new SettingsStateChangerRule(
            sContext, NAMESPACE_GLOBAL, AUTOFILL_COMPAT_MODE_ALLOWED_PACKAGES,
            SERVICE_PACKAGE + "[my_url_bar]");

    public VirtualContainerActivityCompatModeTest() {
        super(true);
    }

    @After
    public void resetCompatMode() {
        sContext.getApplicationContext().setAutofillCompatibilityEnabled(false);
    }

    @Override
    protected void preActivityCreated() {
        sContext.getApplicationContext().setAutofillCompatibilityEnabled(true);
    }

    @Override
    protected void postActivityLaunched(VirtualContainerActivity activity) {
        // Set our own compat mode as well..
        activity.mCustomView.setCompatMode(true);
    }

    @Override
    protected void enableService() {
        Helper.enableAutofillService(getContext(), SERVICE_NAME);
        InstrumentedAutoFillServiceCompatMode.setIgnoreUnexpectedRequests(false);
    }

    @Override
    protected void disableService() {
        if (!hasAutofillFeature()) return;

        Helper.disableAutofillService(getContext(), SERVICE_NAME);
        InstrumentedAutoFillServiceCompatMode.setIgnoreUnexpectedRequests(true);
    }

    @Override
    protected void assertUrlBarIsSanitized(ViewNode urlBar) {
        assertTextIsSanitized(urlBar);
        assertThat(urlBar.getWebDomain()).isEqualTo("dev.null");
        assertThat(urlBar.getWebScheme()).isEqualTo("ftp");
    }

    @Test
    public void testMultipleUrlBars_firstDoesNotExist() throws Exception {
        SettingsHelper.syncSet(sContext, NAMESPACE_GLOBAL, AUTOFILL_COMPAT_MODE_ALLOWED_PACKAGES,
                SERVICE_PACKAGE + "[first_am_i,my_url_bar]");

        // Set service.
        enableService();

        // Set expectations.
        sReplier.addResponse(new CannedDataset.Builder()
                .setField(ID_USERNAME, "dude", createPresentation("DUDE"))
                .build());

        // Trigger autofill.
        focusToUsername();
        assertDatasetShown(mActivity.mUsername, "DUDE");

        // Make sure input was sanitized.
        final FillRequest request = sReplier.getNextFillRequest();
        final ViewNode urlBar = findNodeByResourceId(request.structure, ID_URL_BAR);

        assertUrlBarIsSanitized(urlBar);
    }

    @Test
    @AppModeFull // testMultipleUrlBars_firstDoesNotExist() is enough to test ephemeral apps support
    public void testMultipleUrlBars_bothExist() throws Exception {
        SettingsHelper.syncSet(sContext, NAMESPACE_GLOBAL, AUTOFILL_COMPAT_MODE_ALLOWED_PACKAGES,
                SERVICE_PACKAGE + "[my_url_bar,my_url_bar2]");

        // Set service.
        enableService();

        // Set expectations.
        sReplier.addResponse(new CannedDataset.Builder()
                .setField(ID_USERNAME, "dude", createPresentation("DUDE"))
                .build());

        // Trigger autofill.
        focusToUsername();
        assertDatasetShown(mActivity.mUsername, "DUDE");

        // Make sure input was sanitized.
        final FillRequest request = sReplier.getNextFillRequest();
        final ViewNode urlBar = findNodeByResourceId(request.structure, ID_URL_BAR);
        final ViewNode urlBar2 = findNodeByResourceId(request.structure, ID_URL_BAR2);

        assertUrlBarIsSanitized(urlBar);
        assertTextIsSanitized(urlBar2);
    }

    @Test
    @AppModeFull // testMultipleUrlBars_firstDoesNotExist() is enough to test ephemeral apps support
    public void testFocusOnUrlBarIsIgnored() throws Throwable {
        // Set service.
        enableService();

        // Set expectations.
        sReplier.addResponse(new CannedFillResponse.Builder()
                .setRequiredSavableIds(SAVE_DATA_TYPE_PASSWORD, ID_USERNAME, ID_PASSWORD).build());

        // Trigger auto-fill.
        focusToUsernameExpectNoWindowEvent();
        sReplier.getNextFillRequest();

        mActivity.syncRunOnUiThread(() -> mActivity.mUrlBar.requestFocus());

        // Must force sleep, as there is no callback that we can wait upon.
        SystemClock.sleep(Timeouts.FILL_TIMEOUT.ms());

        sReplier.assertNoUnhandledFillRequests();
    }

    @Test
    @AppModeFull // testMultipleUrlBars_firstDoesNotExist() is enough to test ephemeral apps support
    public void testUrlBarChangeIgnoredWhenServiceCanSave() throws Throwable {
        // Set service.
        enableService();

        // Set expectations.
        sReplier.addResponse(new CannedFillResponse.Builder()
                .setSaveInfoFlags(SaveInfo.FLAG_SAVE_ON_ALL_VIEWS_INVISIBLE)
                .setRequiredSavableIds(SAVE_DATA_TYPE_PASSWORD, ID_USERNAME, ID_PASSWORD).build());

        // Trigger auto-fill.
        focusToUsernameExpectNoWindowEvent();
        sReplier.getNextFillRequest();

        // Fill in some stuff
        mActivity.mUsername.setText("foo");
        focusToPasswordExpectNoWindowEvent();
        mActivity.mPassword.setText("bar");

        // Change URL bar before views become invisible
        final OneTimeTextWatcher urlWatcher = new OneTimeTextWatcher("urlWatcher",
                mActivity.mUrlBar, "http://null/dev");
        mActivity.mUrlBar.addTextChangedListener(urlWatcher);
        mActivity.syncRunOnUiThread(() -> mActivity.mUrlBar.setText("http://null/dev"));
        urlWatcher.assertAutoFilled();

        // Trigger save.
        // TODO(b/76220569): ideally, save should be triggered by calling:
        //
        // setViewsInvisible(VisibilityIntegrationMode.OVERRIDE_IS_VISIBLE_TO_USER);
        //
        // But unfortunately that's not always working due to flakiness on showing the UI, hence
        // we're forcing commit - after all, the point here is the the URL update above didn't
        // cancel the session (which is the case on
        // testUrlBarChangeCancelSessionWhenServiceCannotSave()
        mActivity.getAutofillManager().commit();

        // Assert UI is showing.
        mUiBot.saveForAutofill(true, SAVE_DATA_TYPE_PASSWORD);

        // Assert results
        final SaveRequest saveRequest = sReplier.getNextSaveRequest();
        final ViewNode username = findNodeByResourceId(saveRequest.structure, ID_USERNAME);
        final ViewNode password = findNodeByResourceId(saveRequest.structure, ID_PASSWORD);
        final ViewNode urlBar = findNodeByResourceId(saveRequest.structure, ID_URL_BAR);

        assertTextAndValue(username, "foo");
        assertTextAndValue(password, "bar");
        // Make sure it's the URL bar from initial session.
        assertTextAndValue(urlBar, INITIAL_URL_BAR_VALUE);
    }

    @Test
    @AppModeFull // testMultipleUrlBars_firstDoesNotExist() is enough to test ephemeral apps support
    public void testUrlBarChangeCancelSessionWhenServiceCannotSave() throws Throwable {
        // Set service.
        enableService();

        // Set expectations.
        sReplier.addResponse(new CannedFillResponse.Builder()
                .addDataset(new CannedDataset.Builder()
                    .setField(ID_USERNAME, "dude")
                    .setField(ID_PASSWORD, "sweet")
                    .setPresentation(createPresentation("The Dude"))
                    .build())
                // there's no SaveInfo here
                .build());

        // Trigger auto-fill.
        focusToUsernameExpectNoWindowEvent();
        sReplier.getNextFillRequest();
        assertDatasetShown(mActivity.mUsername, "The Dude");

        // Fill in some stuff
        mActivity.mUsername.setText("foo");
        focusToPasswordExpectNoWindowEvent();
        mActivity.mPassword.setText("bar");

        // Change URL bar before views become invisible
        final OneTimeTextWatcher urlWatcher = new OneTimeTextWatcher("urlWatcher",
                mActivity.mUrlBar, "http://null/dev");
        mActivity.mUrlBar.addTextChangedListener(urlWatcher);
        mActivity.syncRunOnUiThread(() -> mActivity.mUrlBar.setText("http://null/dev"));
        urlWatcher.assertAutoFilled();

        // Trigger save...
        mActivity.getAutofillManager().commit();

        // ... should not be triggered because the session was already canceled...
        mUiBot.assertSaveNotShowing(SAVE_DATA_TYPE_PASSWORD);
    }

    @Test
    @AppModeFull // testMultipleUrlBars_firstDoesNotExist() is enough to test ephemeral apps support
    public void testUrlBarChangeCancelSessionWhenServiceReturnsNullResponse() throws Throwable {
        // Set service.
        enableService();

        // Set expectations.
        sReplier.addResponse(CannedFillResponse.NO_RESPONSE);

        // Trigger auto-fill.
        focusToUsernameExpectNoWindowEvent();
        sReplier.getNextFillRequest();

        // Fill in some stuff
        mActivity.mUsername.setText("foo");
        focusToPasswordExpectNoWindowEvent();
        mActivity.mPassword.setText("bar");

        // Change URL bar before views become invisible
        final OneTimeTextWatcher urlWatcher = new OneTimeTextWatcher("urlWatcher",
                mActivity.mUrlBar, "http://null/dev");
        mActivity.mUrlBar.addTextChangedListener(urlWatcher);
        mActivity.syncRunOnUiThread(() -> mActivity.mUrlBar.setText("http://null/dev"));
        urlWatcher.assertAutoFilled();

        // Trigger save...
        mActivity.getAutofillManager().commit();

        // ... should not be triggered because the session was already canceled...
        mUiBot.assertSaveNotShowing(SAVE_DATA_TYPE_PASSWORD);
    }
}
