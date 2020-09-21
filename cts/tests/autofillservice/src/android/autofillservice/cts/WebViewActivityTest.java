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

import static android.autofillservice.cts.WebViewActivity.HTML_NAME_PASSWORD;
import static android.autofillservice.cts.WebViewActivity.HTML_NAME_USERNAME;
import static android.autofillservice.cts.WebViewActivity.ID_OUTSIDE1;
import static android.autofillservice.cts.WebViewActivity.ID_OUTSIDE2;
import static android.service.autofill.SaveInfo.SAVE_DATA_TYPE_PASSWORD;

import static com.google.common.truth.Truth.assertThat;

import android.app.assist.AssistStructure.ViewNode;
import android.autofillservice.cts.CannedFillResponse.CannedDataset;
import android.autofillservice.cts.InstrumentedAutoFillService.FillRequest;
import android.autofillservice.cts.InstrumentedAutoFillService.SaveRequest;
import android.platform.test.annotations.AppModeFull;
import android.support.test.uiautomator.UiObject2;
import android.view.KeyEvent;
import android.view.ViewStructure.HtmlInfo;
import android.view.autofill.AutofillManager;

import org.junit.AfterClass;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Ignore;
import org.junit.Rule;
import org.junit.Test;

@AppModeFull(reason = "Flaky in instant mode")
public class WebViewActivityTest extends AutoFillServiceTestCase {

    // TODO(b/64951517): WebView currently does not trigger the autofill callbacks when values are
    // set using accessibility.
    private static final boolean INJECT_EVENTS = true;

    @Rule
    public final AutofillActivityTestRule<WebViewActivity> mActivityRule =
            new AutofillActivityTestRule<WebViewActivity>(WebViewActivity.class);

    private WebViewActivity mActivity;

    @Before
    public void setActivity() {
        mActivity = mActivityRule.getActivity();
    }

    @BeforeClass
    public static void setReplierMode() {
        sReplier.setIdMode(IdMode.HTML_NAME);
    }

    @AfterClass
    public static void resetReplierMode() {
        sReplier.setIdMode(IdMode.RESOURCE_ID);
    }

    @Test
    @AppModeFull // testAutofillOneDataset() is enough to test ephemeral apps support
    public void testAutofillNoDatasets() throws Exception {
        // Set service.
        enableService();

        // Load WebView
        mActivity.loadWebView(mUiBot);

        // Set expectations.
        sReplier.addResponse(CannedFillResponse.NO_RESPONSE);

        // Trigger autofill.
        mActivity.getUsernameInput(mUiBot).click();
        sReplier.getNextFillRequest();

        // Assert not shown.
        mUiBot.assertNoDatasetsEver();
    }

    @Test
    public void testAutofillOneDataset() throws Exception {
        autofillOneDatasetTest(false);
    }

    @Ignore("blocked on b/74793485")
    @Test
    @AppModeFull // testAutofillOneDataset() is enough to test ephemeral apps support
    public void testAutofillOneDataset_usingAppContext() throws Exception {
        autofillOneDatasetTest(true);
    }

    private void autofillOneDatasetTest(boolean usesAppContext) throws Exception {
        // Set service.
        enableService();

        // Load WebView
        final MyWebView myWebView = mActivity.loadWebView(mUiBot, usesAppContext);
        // Sanity check to make sure autofill is enabled in the application context
        assertThat(myWebView.getContext().getSystemService(AutofillManager.class).isEnabled())
                .isTrue();

        // Set expectations.
        myWebView.expectAutofill("dude", "sweet");
        final MyAutofillCallback callback = mActivity.registerCallback();
        sReplier.addResponse(new CannedDataset.Builder()
                .setField(HTML_NAME_USERNAME, "dude")
                .setField(HTML_NAME_PASSWORD, "sweet")
                .setPresentation(createPresentation("The Dude"))
                .build());

        // Trigger autofill.
        mActivity.getUsernameInput(mUiBot).click();
        final FillRequest fillRequest = sReplier.getNextFillRequest();
        mUiBot.assertDatasets("The Dude");

        // Change focus around.
        final int usernameChildId = callback.assertUiShownEventForVirtualChild(myWebView);
        mActivity.getUsernameLabel(mUiBot).click();
        callback.assertUiHiddenEvent(myWebView, usernameChildId);
        mUiBot.assertNoDatasets();
        mActivity.getPasswordInput(mUiBot).click();
        final int passwordChildId = callback.assertUiShownEventForVirtualChild(myWebView);
        final UiObject2 datasetPicker = mUiBot.assertDatasets("The Dude");

        // Now Autofill it.
        mUiBot.selectDataset(datasetPicker, "The Dude");
        myWebView.assertAutofilled();
        mUiBot.assertNoDatasets();
        callback.assertUiHiddenEvent(myWebView, passwordChildId);

        // Make sure screen was autofilled.
        assertThat(mActivity.getUsernameInput(mUiBot).getText()).isEqualTo("dude");
        // TODO: proper way to verify text (which is ..... because it's a password) - ideally it
        // should call passwordInput.isPassword(), but that's not exposed
        final String password = mActivity.getPasswordInput(mUiBot).getText();
        assertThat(password).isNotEqualTo("sweet");
        assertThat(password).hasLength(5);

        // Assert structure passed to service.
        try {
            final ViewNode webViewNode =
                    Helper.findWebViewNodeByFormName(fillRequest.structure, "FORM AM I");
            assertThat(webViewNode.getClassName()).isEqualTo("android.webkit.WebView");
            assertThat(webViewNode.getWebDomain()).isEqualTo(WebViewActivity.FAKE_DOMAIN);
            assertThat(webViewNode.getWebScheme()).isEqualTo("https");

            final ViewNode usernameNode =
                    Helper.findNodeByHtmlName(fillRequest.structure, HTML_NAME_USERNAME);
            Helper.assertTextIsSanitized(usernameNode);
            final HtmlInfo usernameHtmlInfo = Helper.assertHasHtmlTag(usernameNode, "input");
            Helper.assertHasAttribute(usernameHtmlInfo, "type", "text");
            Helper.assertHasAttribute(usernameHtmlInfo, "name", "username");
            assertThat(usernameNode.isFocused()).isTrue();
            assertThat(usernameNode.getAutofillHints()).asList().containsExactly("username");
            assertThat(usernameNode.getHint()).isEqualTo("There's no place like a holder");

            final ViewNode passwordNode =
                    Helper.findNodeByHtmlName(fillRequest.structure, HTML_NAME_PASSWORD);
            Helper.assertTextIsSanitized(passwordNode);
            final HtmlInfo passwordHtmlInfo = Helper.assertHasHtmlTag(passwordNode, "input");
            Helper.assertHasAttribute(passwordHtmlInfo, "type", "password");
            Helper.assertHasAttribute(passwordHtmlInfo, "name", "password");
            assertThat(passwordNode.getAutofillHints()).asList()
                    .containsExactly("current-password");
            assertThat(passwordNode.getHint()).isEqualTo("Holder it like it cannnot passer a word");
            assertThat(passwordNode.isFocused()).isFalse();
        } catch (RuntimeException | Error e) {
            Helper.dumpStructure("failed on testAutofillOneDataset()", fillRequest.structure);
            throw e;
        }
    }

    @Test
    public void testSaveOnly() throws Exception {
        // Set service.
        enableService();

        // Load WebView
        mActivity.loadWebView(mUiBot);

        // Set expectations.
        sReplier.addResponse(new CannedFillResponse.Builder()
                .setRequiredSavableIds(SAVE_DATA_TYPE_PASSWORD,
                        HTML_NAME_USERNAME, HTML_NAME_PASSWORD)
                .build());

        // Trigger autofill.
        mActivity.getUsernameInput(mUiBot).click();
        sReplier.getNextFillRequest();

        // Assert not shown.
        mUiBot.assertNoDatasetsEver();

        // Trigger save.
        if (INJECT_EVENTS) {
            mActivity.getUsernameInput(mUiBot).click();
            mActivity.dispatchKeyPress(KeyEvent.KEYCODE_U);
            mActivity.getPasswordInput(mUiBot).click();
            mActivity.dispatchKeyPress(KeyEvent.KEYCODE_P);
        } else {
            mActivity.getUsernameInput(mUiBot).setText("DUDE");
            mActivity.getPasswordInput(mUiBot).setText("SWEET");
        }
        mActivity.getLoginButton(mUiBot).click();

        // Assert save UI shown.
        mUiBot.saveForAutofill(true, SAVE_DATA_TYPE_PASSWORD);

        // Assert results
        final SaveRequest saveRequest = sReplier.getNextSaveRequest();
        final ViewNode usernameNode = Helper.findNodeByHtmlName(saveRequest.structure,
                HTML_NAME_USERNAME);
        final ViewNode passwordNode = Helper.findNodeByHtmlName(saveRequest.structure,
                HTML_NAME_PASSWORD);
        if (INJECT_EVENTS) {
            Helper.assertTextAndValue(usernameNode, "u");
            Helper.assertTextAndValue(passwordNode, "p");
        } else {
            Helper.assertTextAndValue(usernameNode, "DUDE");
            Helper.assertTextAndValue(passwordNode, "SWEET");
        }
    }

    @Test
    public void testAutofillAndSave() throws Exception {
        // Set service.
        enableService();

        // Load WebView
        final MyWebView myWebView = mActivity.loadWebView(mUiBot);

        // Set expectations.
        final MyAutofillCallback callback = mActivity.registerCallback();
        myWebView.expectAutofill("dude", "sweet");
        sReplier.addResponse(new CannedFillResponse.Builder()
                .setRequiredSavableIds(SAVE_DATA_TYPE_PASSWORD,
                        HTML_NAME_USERNAME, HTML_NAME_PASSWORD)
                .addDataset(new CannedDataset.Builder()
                        .setField(HTML_NAME_USERNAME, "dude")
                        .setField(HTML_NAME_PASSWORD, "sweet")
                        .setPresentation(createPresentation("The Dude"))
                        .build())
                .build());

        // Trigger autofill.
        mActivity.getUsernameInput(mUiBot).click();
        final FillRequest fillRequest = sReplier.getNextFillRequest();
        mUiBot.assertDatasets("The Dude");
        final int usernameChildId = callback.assertUiShownEventForVirtualChild(myWebView);

        // Assert structure passed to service.
        final ViewNode usernameNode = Helper.findNodeByHtmlName(fillRequest.structure,
                HTML_NAME_USERNAME);
        Helper.assertTextIsSanitized(usernameNode);
        assertThat(usernameNode.isFocused()).isTrue();
        assertThat(usernameNode.getAutofillHints()).asList().containsExactly("username");
        final ViewNode passwordNode = Helper.findNodeByHtmlName(fillRequest.structure,
                HTML_NAME_PASSWORD);
        Helper.assertTextIsSanitized(passwordNode);
        assertThat(passwordNode.getAutofillHints()).asList().containsExactly("current-password");
        assertThat(passwordNode.isFocused()).isFalse();

        // Autofill it.
        mUiBot.selectDataset("The Dude");
        myWebView.assertAutofilled();
        callback.assertUiHiddenEvent(myWebView, usernameChildId);

        // Make sure screen was autofilled.
        assertThat(mActivity.getUsernameInput(mUiBot).getText()).isEqualTo("dude");
        // TODO: proper way to verify text (which is ..... because it's a password) - ideally it
        // should call passwordInput.isPassword(), but that's not exposed
        final String password = mActivity.getPasswordInput(mUiBot).getText();
        assertThat(password).isNotEqualTo("sweet");
        assertThat(password).hasLength(5);

        // Now trigger save.
        if (INJECT_EVENTS) {
            mActivity.getUsernameInput(mUiBot).click();
            mActivity.dispatchKeyPress(KeyEvent.KEYCODE_U);
            mActivity.getPasswordInput(mUiBot).click();
            mActivity.dispatchKeyPress(KeyEvent.KEYCODE_P);
        } else {
            mActivity.getUsernameInput(mUiBot).setText("DUDE");
            mActivity.getPasswordInput(mUiBot).setText("SWEET");
        }
        mActivity.getLoginButton(mUiBot).click();

        // Assert save UI shown.
        mUiBot.saveForAutofill(true, SAVE_DATA_TYPE_PASSWORD);

        // Assert results
        final SaveRequest saveRequest = sReplier.getNextSaveRequest();
        final ViewNode usernameNode2 = Helper.findNodeByHtmlName(saveRequest.structure,
                HTML_NAME_USERNAME);
        final ViewNode passwordNode2 = Helper.findNodeByHtmlName(saveRequest.structure,
                HTML_NAME_PASSWORD);
        if (INJECT_EVENTS) {
            Helper.assertTextAndValue(usernameNode2, "dudeu");
            Helper.assertTextAndValue(passwordNode2, "sweetp");
        } else {
            Helper.assertTextAndValue(usernameNode2, "DUDE");
            Helper.assertTextAndValue(passwordNode2, "SWEET");
        }
    }

    @Test
    @AppModeFull // testAutofillAndSave() is enough to test ephemeral apps support
    public void testAutofillAndSave_withExternalViews_loadWebViewFirst() throws Exception {
        // Set service.
        enableService();

        // Load views
        final MyWebView myWebView = mActivity.loadWebView(mUiBot);
        mActivity.loadOutsideViews();

        // Set expectations.
        myWebView.expectAutofill("dude", "sweet");
        final OneTimeTextWatcher outside1Watcher = new OneTimeTextWatcher("outside1",
                mActivity.mOutside1, "duder");
        final OneTimeTextWatcher outside2Watcher = new OneTimeTextWatcher("outside2",
                mActivity.mOutside2, "sweeter");
        mActivity.mOutside1.addTextChangedListener(outside1Watcher);
        mActivity.mOutside2.addTextChangedListener(outside2Watcher);

        final MyAutofillCallback callback = mActivity.registerCallback();
        sReplier.setIdMode(IdMode.HTML_NAME_OR_RESOURCE_ID);
        sReplier.addResponse(new CannedFillResponse.Builder()
                .setRequiredSavableIds(SAVE_DATA_TYPE_PASSWORD,
                        HTML_NAME_USERNAME, HTML_NAME_PASSWORD, ID_OUTSIDE1, ID_OUTSIDE2)
                .addDataset(new CannedDataset.Builder()
                        .setField(HTML_NAME_USERNAME, "dude", createPresentation("USER"))
                        .setField(HTML_NAME_PASSWORD, "sweet", createPresentation("PASS"))
                        .setField(ID_OUTSIDE1, "duder", createPresentation("OUT1"))
                        .setField(ID_OUTSIDE2, "sweeter", createPresentation("OUT2"))
                        .build())
                .build());

        // Trigger autofill.
        mActivity.getUsernameInput(mUiBot).click();
        final FillRequest fillRequest = sReplier.getNextFillRequest();
        mUiBot.assertDatasets("USER");
        final int usernameChildId = callback.assertUiShownEventForVirtualChild(myWebView);

        // Assert structure passed to service.
        final ViewNode usernameFillNode = Helper.findNodeByHtmlName(fillRequest.structure,
                HTML_NAME_USERNAME);
        Helper.assertTextIsSanitized(usernameFillNode);
        assertThat(usernameFillNode.isFocused()).isTrue();
        assertThat(usernameFillNode.getAutofillHints()).asList().containsExactly("username");
        final ViewNode passwordFillNode = Helper.findNodeByHtmlName(fillRequest.structure,
                HTML_NAME_PASSWORD);
        Helper.assertTextIsSanitized(passwordFillNode);
        assertThat(passwordFillNode.getAutofillHints()).asList()
                .containsExactly("current-password");
        assertThat(passwordFillNode.isFocused()).isFalse();

        final ViewNode outside1FillNode = Helper.findNodeByResourceId(fillRequest.structure,
                ID_OUTSIDE1);
        Helper.assertTextIsSanitized(outside1FillNode);
        final ViewNode outside2FillNode = Helper.findNodeByResourceId(fillRequest.structure,
                ID_OUTSIDE2);
        Helper.assertTextIsSanitized(outside2FillNode);

        // Move focus around to make sure UI is shown accordingly
        mActivity.runOnUiThread(() -> mActivity.mOutside1.requestFocus());
        callback.assertUiHiddenEvent(myWebView, usernameChildId);
        mUiBot.assertDatasets("OUT1");
        callback.assertUiShownEvent(mActivity.mOutside1);

        mActivity.getPasswordInput(mUiBot).click();
        callback.assertUiHiddenEvent(mActivity.mOutside1);
        mUiBot.assertDatasets("PASS");
        final int passwordChildId = callback.assertUiShownEventForVirtualChild(myWebView);

        mActivity.runOnUiThread(() -> mActivity.mOutside2.requestFocus());
        callback.assertUiHiddenEvent(myWebView, passwordChildId);
        final UiObject2 datasetPicker = mUiBot.assertDatasets("OUT2");
        callback.assertUiShownEvent(mActivity.mOutside2);

        // Autofill it.
        mUiBot.selectDataset(datasetPicker, "OUT2");
        callback.assertUiHiddenEvent(mActivity.mOutside2);

        myWebView.assertAutofilled();
        outside1Watcher.assertAutoFilled();
        outside2Watcher.assertAutoFilled();

        // Now trigger save.
        if (INJECT_EVENTS) {
            mActivity.getUsernameInput(mUiBot).click();
            mActivity.dispatchKeyPress(KeyEvent.KEYCODE_U);
            mActivity.getPasswordInput(mUiBot).click();
            mActivity.dispatchKeyPress(KeyEvent.KEYCODE_P);
        } else {
            mActivity.getUsernameInput(mUiBot).setText("DUDE");
            mActivity.getPasswordInput(mUiBot).setText("SWEET");
        }
        mActivity.runOnUiThread(() -> {
            mActivity.mOutside1.setText("DUDER");
            mActivity.mOutside2.setText("SWEETER");
        });

        mActivity.getLoginButton(mUiBot).click();

        // Assert save UI shown.
        mUiBot.saveForAutofill(true, SAVE_DATA_TYPE_PASSWORD);

        // Assert results
        final SaveRequest saveRequest = sReplier.getNextSaveRequest();
        final ViewNode usernameSaveNode = Helper.findNodeByHtmlName(saveRequest.structure,
                HTML_NAME_USERNAME);
        final ViewNode passwordSaveNode = Helper.findNodeByHtmlName(saveRequest.structure,
                HTML_NAME_PASSWORD);
        if (INJECT_EVENTS) {
            Helper.assertTextAndValue(usernameSaveNode, "dudeu");
            Helper.assertTextAndValue(passwordSaveNode, "sweetp");
        } else {
            Helper.assertTextAndValue(usernameSaveNode, "DUDE");
            Helper.assertTextAndValue(passwordSaveNode, "SWEET");
        }

        final ViewNode outside1SaveNode = Helper.findNodeByResourceId(saveRequest.structure,
                ID_OUTSIDE1);
        Helper.assertTextAndValue(outside1SaveNode, "DUDER");
        final ViewNode outside2SaveNode = Helper.findNodeByResourceId(saveRequest.structure,
                ID_OUTSIDE2);
        Helper.assertTextAndValue(outside2SaveNode, "SWEETER");
    }


    @Test
    @Ignore("blocked on b/69461853")
    @AppModeFull // testAutofillAndSave() is enough to test ephemeral apps support
    public void testAutofillAndSave_withExternalViews_loadExternalViewsFirst() throws Exception {
        // Set service.
        enableService();

        // Load outside views
        mActivity.loadOutsideViews();

        // Set expectations.
        final OneTimeTextWatcher outside1Watcher = new OneTimeTextWatcher("outside1",
                mActivity.mOutside1, "duder");
        final OneTimeTextWatcher outside2Watcher = new OneTimeTextWatcher("outside2",
                mActivity.mOutside2, "sweeter");
        mActivity.mOutside1.addTextChangedListener(outside1Watcher);
        mActivity.mOutside2.addTextChangedListener(outside2Watcher);

        final MyAutofillCallback callback = mActivity.registerCallback();
        sReplier.setIdMode(IdMode.RESOURCE_ID);
        sReplier.addResponse(new CannedFillResponse.Builder()
                .addDataset(new CannedDataset.Builder()
                        .setField(ID_OUTSIDE1, "duder", createPresentation("OUT1"))
                        .setField(ID_OUTSIDE2, "sweeter", createPresentation("OUT2"))
                        .build())
                .build());

        // Trigger autofill.
        mActivity.runOnUiThread(() -> mActivity.mOutside1.requestFocus());
        final FillRequest fillRequest1 = sReplier.getNextFillRequest();
        mUiBot.assertDatasets("OUT1");
        callback.assertUiShownEvent(mActivity.mOutside1);

        // Move focus around to make sure UI is shown accordingly
        mActivity.runOnUiThread(() -> mActivity.mOutside2.requestFocus());
        callback.assertUiHiddenEvent(mActivity.mOutside1);
        mUiBot.assertDatasets("OUT2");
        callback.assertUiShownEvent(mActivity.mOutside2);

        // Assert structure passed to service.
        final ViewNode outside1FillNode = Helper.findNodeByResourceId(fillRequest1.structure,
                ID_OUTSIDE1);
        Helper.assertTextIsSanitized(outside1FillNode);
        final ViewNode outside2FillNode = Helper.findNodeByResourceId(fillRequest1.structure,
                ID_OUTSIDE2);
        Helper.assertTextIsSanitized(outside2FillNode);

        // Now load Webiew
        final MyWebView myWebView = mActivity.loadWebView(mUiBot);

        // Set expectations
        myWebView.expectAutofill("dude", "sweet");
        sReplier.setIdMode(IdMode.HTML_NAME_OR_RESOURCE_ID);
        sReplier.addResponse(new CannedFillResponse.Builder()
                .setRequiredSavableIds(SAVE_DATA_TYPE_PASSWORD,
                        HTML_NAME_USERNAME, HTML_NAME_PASSWORD, ID_OUTSIDE1, ID_OUTSIDE2)
                .addDataset(new CannedDataset.Builder()
                        .setField(HTML_NAME_USERNAME, "dude", createPresentation("USER"))
                        .setField(HTML_NAME_PASSWORD, "sweet", createPresentation("PASS"))
                        .build())
                .build());

        // Trigger autofill.
        mActivity.getUsernameInput(mUiBot).click();
        final FillRequest fillRequest2 = sReplier.getNextFillRequest();
        callback.assertUiHiddenEvent(mActivity.mOutside2);
        mUiBot.assertDatasets("USER");
        final int usernameChildId = callback.assertUiShownEventForVirtualChild(myWebView);

        // Move focus around to make sure UI is shown accordingly
        mActivity.runOnUiThread(() -> mActivity.mOutside1.requestFocus());
        callback.assertUiHiddenEvent(myWebView, usernameChildId);
        mUiBot.assertDatasets("OUT1");
        callback.assertUiShownEvent(mActivity.mOutside1);

        mActivity.runOnUiThread(() -> mActivity.mOutside2.requestFocus());
        callback.assertUiHiddenEvent(mActivity.mOutside1);
        mUiBot.assertDatasets("OUT2");
        callback.assertUiShownEvent(mActivity.mOutside2);

        mActivity.getPasswordInput(mUiBot).click();
        callback.assertUiHiddenEvent(mActivity.mOutside2);
        mUiBot.assertDatasets("PASS");
        final int passwordChildId = callback.assertUiShownEventForVirtualChild(myWebView);

        mActivity.runOnUiThread(() -> mActivity.mOutside2.requestFocus());
        callback.assertUiHiddenEvent(myWebView, passwordChildId);
        final UiObject2 datasetPicker = mUiBot.assertDatasets("OUT2");
        callback.assertUiShownEvent(mActivity.mOutside2);

        // Assert structure passed to service.
        final ViewNode usernameFillNode = Helper.findNodeByHtmlName(fillRequest2.structure,
                HTML_NAME_USERNAME);
        Helper.assertTextIsSanitized(usernameFillNode);
        assertThat(usernameFillNode.isFocused()).isTrue();
        assertThat(usernameFillNode.getAutofillHints()).asList().containsExactly("username");
        final ViewNode passwordFillNode = Helper.findNodeByHtmlName(fillRequest2.structure,
                HTML_NAME_PASSWORD);
        Helper.assertTextIsSanitized(passwordFillNode);
        assertThat(passwordFillNode.getAutofillHints()).asList()
                .containsExactly("current-password");
        assertThat(passwordFillNode.isFocused()).isFalse();

        // Autofill external views (2nd partition)
        mUiBot.selectDataset(datasetPicker, "OUT2");
        callback.assertUiHiddenEvent(mActivity.mOutside2);
        outside1Watcher.assertAutoFilled();
        outside2Watcher.assertAutoFilled();

        // Autofill Webview (1st partition)
        mActivity.getUsernameInput(mUiBot).click();
        callback.assertUiShownEventForVirtualChild(myWebView);
        mUiBot.selectDataset("USER");
        myWebView.assertAutofilled();

        // Now trigger save.
        if (INJECT_EVENTS) {
            mActivity.getUsernameInput(mUiBot).click();
            mActivity.dispatchKeyPress(KeyEvent.KEYCODE_U);
            mActivity.getPasswordInput(mUiBot).click();
            mActivity.dispatchKeyPress(KeyEvent.KEYCODE_P);
        } else {
            mActivity.getUsernameInput(mUiBot).setText("DUDE");
            mActivity.getPasswordInput(mUiBot).setText("SWEET");
        }
        mActivity.runOnUiThread(() -> {
            mActivity.mOutside1.setText("DUDER");
            mActivity.mOutside2.setText("SWEETER");
        });

        mActivity.getLoginButton(mUiBot).click();

        // Assert save UI shown.
        mUiBot.saveForAutofill(true, SAVE_DATA_TYPE_PASSWORD);

        // Assert results
        final SaveRequest saveRequest = sReplier.getNextSaveRequest();
        final ViewNode usernameSaveNode = Helper.findNodeByHtmlName(saveRequest.structure,
                HTML_NAME_USERNAME);
        final ViewNode passwordSaveNode = Helper.findNodeByHtmlName(saveRequest.structure,
                HTML_NAME_PASSWORD);
        if (INJECT_EVENTS) {
            Helper.assertTextAndValue(usernameSaveNode, "dudeu");
            Helper.assertTextAndValue(passwordSaveNode, "sweetp");
        } else {
            Helper.assertTextAndValue(usernameSaveNode, "DUDE");
            Helper.assertTextAndValue(passwordSaveNode, "SWEET");
        }

        final ViewNode outside1SaveNode = Helper.findNodeByResourceId(saveRequest.structure,
                ID_OUTSIDE1);
        Helper.assertTextAndValue(outside1SaveNode, "DUDER");
        final ViewNode outside2SaveNode = Helper.findNodeByResourceId(saveRequest.structure,
                ID_OUTSIDE2);
        Helper.assertTextAndValue(outside2SaveNode, "SWEETER");
    }
}
