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
import static android.autofillservice.cts.common.ShellHelper.runShellCommand;

import android.autofillservice.cts.CannedFillResponse.CannedDataset;
import android.content.IntentSender;
import android.platform.test.annotations.AppModeFull;

import org.junit.AfterClass;
import org.junit.BeforeClass;
import org.junit.Test;

import java.util.regex.Pattern;

public class DatasetFilteringTest extends AbstractLoginActivityTestCase {

    private static String sMaxDatasets;

    @BeforeClass
    public static void setMaxDatasets() {
        sMaxDatasets = runShellCommand("cmd autofill get max_visible_datasets");
        runShellCommand("cmd autofill set max_visible_datasets 4");
    }

    @AfterClass
    public static void restoreMaxDatasets() {
        runShellCommand("cmd autofill set max_visible_datasets %s", sMaxDatasets);
    }

    private static void sendKeyEvents(String keyCode) {
        runShellCommand("input keyevent " + keyCode);
    }

    private void changeUsername(CharSequence username) {
        mActivity.onUsername((v) -> v.setText(username));
    }


    @Test
    public void testFilter() throws Exception {
        final String aa = "Two A's";
        final String ab = "A and B";
        final String b = "Only B";

        enableService();

        // Set expectations.
        sReplier.addResponse(new CannedFillResponse.Builder()
                .addDataset(new CannedDataset.Builder()
                        .setField(ID_USERNAME, "aa")
                        .setPresentation(createPresentation(aa))
                        .build())
                .addDataset(new CannedDataset.Builder()
                        .setField(ID_USERNAME, "ab")
                        .setPresentation(createPresentation(ab))
                        .build())
                .addDataset(new CannedDataset.Builder()
                        .setField(ID_USERNAME, "b")
                        .setPresentation(createPresentation(b))
                        .build())
                .build());

        // Trigger auto-fill.
        requestFocusOnUsername();
        sReplier.getNextFillRequest();

        // With no filter text all datasets should be shown
        mUiBot.assertDatasets(aa, ab, b);

        // Only two datasets start with 'a'
        changeUsername("a");
        mUiBot.assertDatasets(aa, ab);

        // Only one dataset start with 'aa'
        changeUsername("aa");
        mUiBot.assertDatasets(aa);

        // Only two datasets start with 'a'
        changeUsername("a");
        mUiBot.assertDatasets(aa, ab);

        // With no filter text all datasets should be shown
        changeUsername("");
        mUiBot.assertDatasets(aa, ab, b);

        // No dataset start with 'aaa'
        final MyAutofillCallback callback = mActivity.registerCallback();
        changeUsername("aaa");
        callback.assertUiHiddenEvent(mActivity.getUsername());
        mUiBot.assertNoDatasets();
    }

    @Test
    public void testFilter_usingKeyboard() throws Exception {
        final String aa = "Two A's";
        final String ab = "A and B";
        final String b = "Only B";

        enableService();

        // Set expectations.
        sReplier.addResponse(new CannedFillResponse.Builder()
                .addDataset(new CannedDataset.Builder()
                        .setField(ID_USERNAME, "aa")
                        .setPresentation(createPresentation(aa))
                        .build())
                .addDataset(new CannedDataset.Builder()
                        .setField(ID_USERNAME, "ab")
                        .setPresentation(createPresentation(ab))
                        .build())
                .addDataset(new CannedDataset.Builder()
                        .setField(ID_USERNAME, "b")
                        .setPresentation(createPresentation(b))
                        .build())
                .build());

        // Trigger auto-fill.
        requestFocusOnUsername();
        sReplier.getNextFillRequest();

        // With no filter text all datasets should be shown
        mUiBot.assertDatasets(aa, ab, b);

        // Only two datasets start with 'a'
        sendKeyEvents("KEYCODE_A");
        mUiBot.assertDatasets(aa, ab);

        // Only one dataset start with 'aa'
        sendKeyEvents("KEYCODE_A");
        mUiBot.assertDatasets(aa);

        // Only two datasets start with 'a'
        sendKeyEvents("KEYCODE_DEL");
        mUiBot.assertDatasets(aa, ab);

        // With no filter text all datasets should be shown
        sendKeyEvents("KEYCODE_DEL");
        mUiBot.assertDatasets(aa, ab, b);

        // No dataset start with 'aaa'
        final MyAutofillCallback callback = mActivity.registerCallback();
        sendKeyEvents("KEYCODE_A");
        sendKeyEvents("KEYCODE_A");
        sendKeyEvents("KEYCODE_A");
        callback.assertUiHiddenEvent(mActivity.getUsername());
        mUiBot.assertNoDatasets();
    }

    @Test
    @AppModeFull // testFilter() is enough to test ephemeral apps support
    public void testFilter_nullValuesAlwaysMatched() throws Exception {
        final String aa = "Two A's";
        final String ab = "A and B";
        final String b = "Only B";

        enableService();

        // Set expectations.
        sReplier.addResponse(new CannedFillResponse.Builder()
                .addDataset(new CannedDataset.Builder()
                        .setField(ID_USERNAME, "aa")
                        .setPresentation(createPresentation(aa))
                        .build())
                .addDataset(new CannedDataset.Builder()
                        .setField(ID_USERNAME, "ab")
                        .setPresentation(createPresentation(ab))
                        .build())
                .addDataset(new CannedDataset.Builder()
                        .setField(ID_USERNAME, (String) null)
                        .setPresentation(createPresentation(b))
                        .build())
                .build());

        // Trigger auto-fill.
        requestFocusOnUsername();
        sReplier.getNextFillRequest();

        // With no filter text all datasets should be shown
        mUiBot.assertDatasets(aa, ab, b);

        // Two datasets start with 'a' and one with null value always shown
        changeUsername("a");
        mUiBot.assertDatasets(aa, ab, b);

        // One dataset start with 'aa' and one with null value always shown
        changeUsername("aa");
        mUiBot.assertDatasets(aa, b);

        // Two datasets start with 'a' and one with null value always shown
        changeUsername("a");
        mUiBot.assertDatasets(aa, ab, b);

        // With no filter text all datasets should be shown
        changeUsername("");
        mUiBot.assertDatasets(aa, ab, b);

        // No dataset start with 'aaa' and one with null value always shown
        changeUsername("aaa");
        mUiBot.assertDatasets(b);
    }

    @Test
    @AppModeFull // testFilter() is enough to test ephemeral apps support
    public void testFilter_differentPrefixes() throws Exception {
        final String a = "aaa";
        final String b = "bra";
        final String c = "cadabra";

        enableService();

        // Set expectations.
        sReplier.addResponse(new CannedFillResponse.Builder()
                .addDataset(new CannedDataset.Builder()
                        .setField(ID_USERNAME, a)
                        .setPresentation(createPresentation(a))
                        .build())
                .addDataset(new CannedDataset.Builder()
                        .setField(ID_USERNAME, b)
                        .setPresentation(createPresentation(b))
                        .build())
                .addDataset(new CannedDataset.Builder()
                        .setField(ID_USERNAME, c)
                        .setPresentation(createPresentation(c))
                        .build())
                .build());

        // Trigger auto-fill.
        requestFocusOnUsername();
        sReplier.getNextFillRequest();

        // With no filter text all datasets should be shown
        mUiBot.assertDatasets(a, b, c);

        changeUsername("a");
        mUiBot.assertDatasets(a);

        changeUsername("b");
        mUiBot.assertDatasets(b);

        changeUsername("c");
        mUiBot.assertDatasets(c);
    }

    @Test
    @AppModeFull // testFilter() is enough to test ephemeral apps support
    public void testFilter_usingRegex() throws Exception {
        // Dataset presentations.
        final String aa = "Two A's";
        final String ab = "A and B";
        final String b = "Only B";

        enableService();

        // Set expectations.
        sReplier.addResponse(new CannedFillResponse.Builder()
                .addDataset(new CannedDataset.Builder()
                        .setField(ID_USERNAME, "whatever", Pattern.compile("a|aa"))
                        .setPresentation(createPresentation(aa))
                        .build())
                .addDataset(new CannedDataset.Builder()
                        .setField(ID_USERNAME, "whatsoever", createPresentation(ab),
                                Pattern.compile("a|ab"))
                        .build())
                .addDataset(new CannedDataset.Builder()
                        .setField(ID_USERNAME, (String) null, Pattern.compile("b"))
                        .setPresentation(createPresentation(b))
                        .build())
                .build());

        // Trigger auto-fill.
        requestFocusOnUsername();
        sReplier.getNextFillRequest();

        // With no filter text all datasets should be shown
        mUiBot.assertDatasets(aa, ab, b);

        // Only two datasets start with 'a'
        changeUsername("a");
        mUiBot.assertDatasets(aa, ab);

        // Only one dataset start with 'aa'
        changeUsername("aa");
        mUiBot.assertDatasets(aa);

        // Only two datasets start with 'a'
        changeUsername("a");
        mUiBot.assertDatasets(aa, ab);

        // With no filter text all datasets should be shown
        changeUsername("");
        mUiBot.assertDatasets(aa, ab, b);

        // No dataset start with 'aaa'
        final MyAutofillCallback callback = mActivity.registerCallback();
        changeUsername("aaa");
        callback.assertUiHiddenEvent(mActivity.getUsername());
        mUiBot.assertNoDatasets();
    }

    @Test
    @AppModeFull // testFilter() is enough to test ephemeral apps support
    public void testFilter_disabledUsingNullRegex() throws Exception {
        // Dataset presentations.
        final String unfilterable = "Unfilterabled";
        final String aOrW = "A or W";
        final String w = "Wazzup";

        enableService();

        // Set expectations.
        sReplier.addResponse(new CannedFillResponse.Builder()
                // This dataset has a value but filter is disabled
                .addDataset(new CannedDataset.Builder()
                        .setUnfilterableField(ID_USERNAME, "a am I")
                        .setPresentation(createPresentation(unfilterable))
                        .build())
                // This dataset uses pattern to filter
                .addDataset(new CannedDataset.Builder()
                        .setField(ID_USERNAME, "whatsoever", createPresentation(aOrW),
                                Pattern.compile("a|aw"))
                        .build())
                // This dataset uses value to filter
                .addDataset(new CannedDataset.Builder()
                        .setField(ID_USERNAME, "wazzup")
                        .setPresentation(createPresentation(w))
                        .build())
                .build());

        // Trigger auto-fill.
        requestFocusOnUsername();
        sReplier.getNextFillRequest();

        // With no filter text all datasets should be shown
        mUiBot.assertDatasets(unfilterable, aOrW, w);

        // Only one dataset start with 'a'
        changeUsername("a");
        mUiBot.assertDatasets(aOrW);

        // No dataset starts with 'aa'
        changeUsername("aa");
        mUiBot.assertNoDatasets();

        // Only one datasets start with 'a'
        changeUsername("a");
        mUiBot.assertDatasets(aOrW);

        // With no filter text all datasets should be shown
        changeUsername("");
        mUiBot.assertDatasets(unfilterable, aOrW, w);

        // Only one datasets start with 'w'
        changeUsername("w");
        mUiBot.assertDatasets(w);

        // No dataset start with 'aaa'
        final MyAutofillCallback callback = mActivity.registerCallback();
        changeUsername("aaa");
        callback.assertUiHiddenEvent(mActivity.getUsername());
        mUiBot.assertNoDatasets();
    }

    @Test
    @AppModeFull // testFilter() is enough to test ephemeral apps support
    public void testFilter_mixPlainAndRegex() throws Exception {
        final String plain = "Plain";
        final String regexPlain = "RegexPlain";
        final String authRegex = "AuthRegex";
        final String kitchnSync = "KitchenSync";
        final Pattern everything = Pattern.compile(".*");

        enableService();

        // Set expectations.
        final IntentSender authentication = AuthenticationActivity.createSender(mContext, 1,
                new CannedDataset.Builder()
                        .setField(ID_USERNAME, "dude")
                        .build());
        sReplier.addResponse(new CannedFillResponse.Builder()
                .addDataset(new CannedDataset.Builder()
                        .setField(ID_USERNAME, "aword")
                        .setPresentation(createPresentation(plain))
                        .build())
                .addDataset(new CannedDataset.Builder()
                        .setField(ID_USERNAME, "a ignore", everything)
                        .setPresentation(createPresentation(regexPlain))
                        .build())
                .addDataset(new CannedDataset.Builder()
                        .setField(ID_USERNAME, "ab ignore", everything)
                        .setAuthentication(authentication)
                        .setPresentation(createPresentation(authRegex))
                        .build())
                .addDataset(new CannedDataset.Builder()
                        .setField(ID_USERNAME, "ab ignore", createPresentation(kitchnSync),
                                everything)
                        .build())
                .build());

        // Trigger auto-fill.
        requestFocusOnUsername();
        sReplier.getNextFillRequest();

        // With no filter text all datasets should be shown
        mUiBot.assertDatasets(plain, regexPlain, authRegex, kitchnSync);

        // All datasets start with 'a'
        changeUsername("a");
        mUiBot.assertDatasets(plain, regexPlain, authRegex, kitchnSync);

        // Only the regex datasets should start with 'ab'
        changeUsername("ab");
        mUiBot.assertDatasets(regexPlain, authRegex, kitchnSync);
    }

    @Test
    @AppModeFull // testFilter_usingKeyboard() is enough to test ephemeral apps support
    public void testFilter_mixPlainAndRegex_usingKeyboard() throws Exception {
        final String plain = "Plain";
        final String regexPlain = "RegexPlain";
        final String authRegex = "AuthRegex";
        final String kitchnSync = "KitchenSync";
        final Pattern everything = Pattern.compile(".*");

        enableService();

        // Set expectations.
        final IntentSender authentication = AuthenticationActivity.createSender(mContext, 1,
                new CannedDataset.Builder()
                        .setField(ID_USERNAME, "dude")
                        .build());
        sReplier.addResponse(new CannedFillResponse.Builder()
                .addDataset(new CannedDataset.Builder()
                        .setField(ID_USERNAME, "aword")
                        .setPresentation(createPresentation(plain))
                        .build())
                .addDataset(new CannedDataset.Builder()
                        .setField(ID_USERNAME, "a ignore", everything)
                        .setPresentation(createPresentation(regexPlain))
                        .build())
                .addDataset(new CannedDataset.Builder()
                        .setField(ID_USERNAME, "ab ignore", everything)
                        .setAuthentication(authentication)
                        .setPresentation(createPresentation(authRegex))
                        .build())
                .addDataset(new CannedDataset.Builder()
                        .setField(ID_USERNAME, "ab ignore", createPresentation(kitchnSync),
                                everything)
                        .build())
                .build());

        // Trigger auto-fill.
        requestFocusOnUsername();
        sReplier.getNextFillRequest();

        // With no filter text all datasets should be shown
        mUiBot.assertDatasets(plain, regexPlain, authRegex, kitchnSync);

        // All datasets start with 'a'
        sendKeyEvents("KEYCODE_A");
        mUiBot.assertDatasets(plain, regexPlain, authRegex, kitchnSync);

        // Only the regex datasets should start with 'ab'
        sendKeyEvents("KEYCODE_B");
        mUiBot.assertDatasets(regexPlain, authRegex, kitchnSync);
    }
}
