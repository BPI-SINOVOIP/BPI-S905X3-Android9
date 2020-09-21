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

import static android.autofillservice.cts.Helper.ID_STATIC_TEXT;
import static android.autofillservice.cts.Helper.assertTextAndValue;
import static android.autofillservice.cts.Helper.findNodeByResourceId;
import static android.autofillservice.cts.LoginActivity.ID_USERNAME_CONTAINER;
import static android.autofillservice.cts.PreSimpleSaveActivity.ID_PRE_INPUT;
import static android.autofillservice.cts.SimpleSaveActivity.ID_INPUT;
import static android.autofillservice.cts.SimpleSaveActivity.ID_LABEL;
import static android.service.autofill.SaveInfo.SAVE_DATA_TYPE_EMAIL_ADDRESS;
import static android.service.autofill.SaveInfo.SAVE_DATA_TYPE_PASSWORD;

import static com.google.common.truth.Truth.assertThat;

import android.autofillservice.cts.InstrumentedAutoFillService.SaveRequest;
import android.content.Intent;
import android.service.autofill.BatchUpdates;
import android.service.autofill.CustomDescription;
import android.service.autofill.RegexValidator;
import android.service.autofill.Validator;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.UiObject2;
import android.view.View;
import android.widget.RemoteViews;

import org.junit.After;
import org.junit.Rule;

import java.util.regex.Pattern;

public class PreSimpleSaveActivityTest extends CustomDescriptionWithLinkTestCase {

    @Rule
    public final AutofillActivityTestRule<PreSimpleSaveActivity> mActivityRule =
            new AutofillActivityTestRule<PreSimpleSaveActivity>(PreSimpleSaveActivity.class, false);

    private PreSimpleSaveActivity mActivity;

    private void startActivity(boolean remainOnRecents) {
        final Intent intent = new Intent(mContext, PreSimpleSaveActivity.class);
        if (remainOnRecents) {
            intent.setFlags(
                    Intent.FLAG_ACTIVITY_RETAIN_IN_RECENTS | Intent.FLAG_ACTIVITY_NEW_TASK);
        }
        mActivity = mActivityRule.launchActivity(intent);
    }

    @After
    public void finishSimpleSaveActivity() {
        SimpleSaveActivity.finishIt(mUiBot);
    }

    @Override
    protected void saveUiRestoredAfterTappingLinkTest(PostSaveLinkTappedAction type)
            throws Exception {
        startActivity(false);
        // Set service.
        enableService();

        // Set expectations.
        sReplier.addResponse(new CannedFillResponse.Builder()
                .setCustomDescription(newCustomDescription(WelcomeActivity.class))
                .setRequiredSavableIds(SAVE_DATA_TYPE_PASSWORD, ID_PRE_INPUT)
                .build());

        // Trigger autofill.
        mActivity.syncRunOnUiThread(() -> mActivity.mPreInput.requestFocus());
        sReplier.getNextFillRequest();
        Helper.assertHasSessions(mPackageName);

        // Trigger save.
        mActivity.syncRunOnUiThread(() -> {
            mActivity.mPreInput.setText("108");
            mActivity.mSubmit.performClick();
        });
        // Make sure post-save activity is shown...
        mUiBot.assertShownByRelativeId(ID_INPUT);

        // Tap the link.
        final UiObject2 saveUi = assertSaveUiWithLinkIsShown(SAVE_DATA_TYPE_PASSWORD);
        tapSaveUiLink(saveUi);

        // Make sure new activity is shown...
        WelcomeActivity.assertShowingDefaultMessage(mUiBot);
        mUiBot.assertSaveNotShowing(SAVE_DATA_TYPE_PASSWORD);

        // .. then do something to return to previous activity...
        switch (type) {
            case ROTATE_THEN_TAP_BACK_BUTTON:
                mUiBot.setScreenOrientation(UiBot.LANDSCAPE);
                WelcomeActivity.assertShowingDefaultMessage(mUiBot);
                // not breaking on purpose
            case TAP_BACK_BUTTON:
                mUiBot.pressBack();
                break;
            case FINISH_ACTIVITY:
                // ..then finishes it.
                WelcomeActivity.finishIt(mUiBot);
                break;
            default:
                throw new IllegalArgumentException("invalid type: " + type);
        }

        // ... and tap save.
        final UiObject2 newSaveUi = assertSaveUiWithLinkIsShown(SAVE_DATA_TYPE_PASSWORD);
        mUiBot.saveForAutofill(newSaveUi, true);

        final SaveRequest saveRequest = sReplier.getNextSaveRequest();
        assertTextAndValue(findNodeByResourceId(saveRequest.structure, ID_PRE_INPUT), "108");
    }

    @Override
    protected void tapLinkThenTapBackThenStartOverTest(PostSaveLinkTappedAction action,
            boolean manualRequest) throws Exception {
        startActivity(false);
        // Set service.
        enableService();

        // Set expectations.
        sReplier.addResponse(new CannedFillResponse.Builder()
                .setCustomDescription(newCustomDescription(WelcomeActivity.class))
                .setRequiredSavableIds(SAVE_DATA_TYPE_PASSWORD, ID_PRE_INPUT)
                .build());

        // Trigger autofill.
        mActivity.syncRunOnUiThread(() -> mActivity.mPreInput.requestFocus());
        sReplier.getNextFillRequest();
        Helper.assertHasSessions(mPackageName);

        // Trigger save.
        mActivity.syncRunOnUiThread(() -> {
            mActivity.mPreInput.setText("108");
            mActivity.mSubmit.performClick();
        });
        // Make sure post-save activity is shown...
        mUiBot.assertShownByRelativeId(ID_INPUT);

        // Tap the link.
        final UiObject2 saveUi = assertSaveUiWithLinkIsShown(SAVE_DATA_TYPE_PASSWORD);
        tapSaveUiLink(saveUi);

        // Make sure new activity is shown...
        WelcomeActivity.assertShowingDefaultMessage(mUiBot);
        mUiBot.assertSaveNotShowing(SAVE_DATA_TYPE_PASSWORD);

        // Tap back to restore the Save UI...
        mUiBot.pressBack();

        // ...but don't tap it...
        final UiObject2 saveUi2 = mUiBot.assertSaveShowing(SAVE_DATA_TYPE_PASSWORD);

        // ...instead, do something to dismiss it:
        switch (action) {
            case TOUCH_OUTSIDE:
                mUiBot.assertShownByRelativeId(ID_LABEL).longClick();
                break;
            case TAP_NO_ON_SAVE_UI:
                mUiBot.saveForAutofill(saveUi2, false);
                break;
            case TAP_YES_ON_SAVE_UI:
                mUiBot.saveForAutofill(true, SAVE_DATA_TYPE_PASSWORD);

                final SaveRequest saveRequest = sReplier.getNextSaveRequest();
                assertTextAndValue(findNodeByResourceId(saveRequest.structure, ID_PRE_INPUT),
                        "108");
                break;
            default:
                throw new IllegalArgumentException("invalid action: " + action);
        }
        mUiBot.assertSaveNotShowing(SAVE_DATA_TYPE_PASSWORD);

        // Make sure previous session was finished.

        // Now triggers a new session in the new activity (SaveActivity) and do business as usual...
        sReplier.addResponse(new CannedFillResponse.Builder()
                .setRequiredSavableIds(SAVE_DATA_TYPE_EMAIL_ADDRESS, ID_INPUT)
                .build());

        // Trigger autofill.
        final SimpleSaveActivity newActivty = SimpleSaveActivity.getInstance();
        if (manualRequest) {
            newActivty.getAutofillManager().requestAutofill(newActivty.mInput);
        } else {
            newActivty.syncRunOnUiThread(() -> newActivty.mPassword.requestFocus());
        }

        sReplier.getNextFillRequest();
        Helper.assertHasSessions(mPackageName);

        // Trigger save.
        newActivty.syncRunOnUiThread(() -> {
            newActivty.mInput.setText("42");
            newActivty.mCommit.performClick();
        });
        // Make sure post-save activity is shown...
        mUiBot.assertShownByRelativeId(ID_INPUT);

        // Save it...
        mUiBot.saveForAutofill(true, SAVE_DATA_TYPE_EMAIL_ADDRESS);

        // ... and assert results
        final SaveRequest saveRequest = sReplier.getNextSaveRequest();
        assertTextAndValue(findNodeByResourceId(saveRequest.structure, ID_INPUT), "42");
    }

    @Override
    protected void saveUiCancelledAfterTappingLinkTest(PostSaveLinkTappedAction type)
            throws Exception {
        startActivity(false);
        // Set service.
        enableService();

        // Set expectations.
        sReplier.addResponse(new CannedFillResponse.Builder()
                .setCustomDescription(newCustomDescription(WelcomeActivity.class))
                .setRequiredSavableIds(SAVE_DATA_TYPE_PASSWORD, ID_PRE_INPUT)
                .build());

        // Trigger autofill.
        mActivity.syncRunOnUiThread(() -> mActivity.mPreInput.requestFocus());
        sReplier.getNextFillRequest();
        Helper.assertHasSessions(mPackageName);

        // Trigger save.
        mActivity.syncRunOnUiThread(() -> {
            mActivity.mPreInput.setText("108");
            mActivity.mSubmit.performClick();
        });
        // Make sure post-save activity is shown...
        mUiBot.assertShownByRelativeId(ID_INPUT);

        // Tap the link.
        final UiObject2 saveUi = assertSaveUiWithLinkIsShown(SAVE_DATA_TYPE_PASSWORD);
        tapSaveUiLink(saveUi);

        // Make sure linked activity is shown...
        WelcomeActivity.assertShowingDefaultMessage(mUiBot);
        mUiBot.assertSaveNotShowing(SAVE_DATA_TYPE_PASSWORD);

        switch (type) {
            case LAUNCH_PREVIOUS_ACTIVITY:
                startActivityOnNewTask(PreSimpleSaveActivity.class);
                mUiBot.assertShownByRelativeId(ID_INPUT);
                break;
            case LAUNCH_NEW_ACTIVITY:
                // Launch a 3rd activity...
                startActivityOnNewTask(LoginActivity.class);
                mUiBot.assertShownByRelativeId(ID_USERNAME_CONTAINER);
                // ...then go back
                mUiBot.pressBack();
                mUiBot.assertShownByRelativeId(ID_INPUT);
                break;
            default:
                throw new IllegalArgumentException("invalid type: " + type);
        }

        mUiBot.assertSaveNotShowing(SAVE_DATA_TYPE_PASSWORD);
    }

    @Override
    protected void tapLinkLaunchTrampolineActivityThenTapBackAndStartNewSessionTest()
            throws Exception {
        // Prepare activity.
        startActivity(false);
        mActivity.mPreInput.getRootView()
                .setImportantForAutofill(View.IMPORTANT_FOR_AUTOFILL_NO_EXCLUDE_DESCENDANTS);

        // Set service.
        enableService();

        // Set expectations.
        sReplier.addResponse(new CannedFillResponse.Builder()
                .setCustomDescription(newCustomDescription(TrampolineWelcomeActivity.class))
                .setRequiredSavableIds(SAVE_DATA_TYPE_PASSWORD, ID_PRE_INPUT)
                .build());

        // Trigger autofill.
        mActivity.getAutofillManager().requestAutofill(mActivity.mPreInput);
        sReplier.getNextFillRequest();
        Helper.assertHasSessions(mPackageName);

        // Trigger save.
        mActivity.syncRunOnUiThread(() -> {
            mActivity.mPreInput.setText("108");
            mActivity.mSubmit.performClick();
        });
        final UiObject2 saveUi = assertSaveUiWithLinkIsShown(SAVE_DATA_TYPE_PASSWORD);

        // Tap the link.
        tapSaveUiLink(saveUi);

        // Make sure new activity is shown...
        WelcomeActivity.assertShowingDefaultMessage(mUiBot);

        // Save UI should be showing as well, since Trampoline finished.
        mUiBot.assertSaveShowing(SAVE_DATA_TYPE_PASSWORD);

        // Go back and make sure it's showing the right activity.
        // first BACK cancels save dialog
        mUiBot.pressBack();
        // second BACK cancel WelcomeActivity
        mUiBot.pressBack();
        mUiBot.assertShownByRelativeId(ID_INPUT);

        // Now triggers a new session in the new activity (SaveActivity) and do business as usual...
        sReplier.addResponse(new CannedFillResponse.Builder()
                .setRequiredSavableIds(SAVE_DATA_TYPE_EMAIL_ADDRESS, ID_INPUT)
                .build());

        // Trigger autofill.
        final SimpleSaveActivity newActivty = SimpleSaveActivity.getInstance();
        newActivty.getAutofillManager().requestAutofill(newActivty.mInput);

        sReplier.getNextFillRequest();
        Helper.assertHasSessions(mPackageName);

        // Trigger save.
        newActivty.syncRunOnUiThread(() -> {
            newActivty.mInput.setText("42");
            newActivty.mCommit.performClick();
        });
        // Make sure post-save activity is shown...
        mUiBot.assertShownByRelativeId(ID_INPUT);

        // Save it...
        mUiBot.saveForAutofill(true, SAVE_DATA_TYPE_EMAIL_ADDRESS);

        // ... and assert results
        final SaveRequest saveRequest1 = sReplier.getNextSaveRequest();
        assertTextAndValue(findNodeByResourceId(saveRequest1.structure, ID_INPUT), "42");
    }

    @Override
    protected void tapLinkAfterUpdateAppliedTest(boolean updateLinkView) throws Exception {
        startActivity(false);
        // Set service.
        enableService();

        final CustomDescription.Builder customDescription =
                newCustomDescriptionBuilder(WelcomeActivity.class);
        final RemoteViews update = newTemplate();
        if (updateLinkView) {
            update.setCharSequence(R.id.link, "setText", "TAP ME IF YOU CAN");
        } else {
            update.setCharSequence(R.id.static_text, "setText", "ME!");
        }
        Validator validCondition = new RegexValidator(mActivity.mPreInput.getAutofillId(),
                Pattern.compile(".*"));
        customDescription.batchUpdate(validCondition,
                new BatchUpdates.Builder().updateTemplate(update).build());

        // Set expectations.
        sReplier.addResponse(new CannedFillResponse.Builder()
                .setCustomDescription(customDescription.build())
                .setRequiredSavableIds(SAVE_DATA_TYPE_PASSWORD, ID_PRE_INPUT)
                .build());

        // Trigger autofill.
        mActivity.syncRunOnUiThread(() -> mActivity.mPreInput.requestFocus());
        sReplier.getNextFillRequest();
        Helper.assertHasSessions(mPackageName);

        // Trigger save.
        mActivity.syncRunOnUiThread(() -> {
            mActivity.mPreInput.setText("108");
            mActivity.mSubmit.performClick();
        });
        // Make sure post-save activity is shown...
        mUiBot.assertShownByRelativeId(ID_INPUT);

        // Tap the link.
        final UiObject2 saveUi;
        if (updateLinkView) {
            saveUi = assertSaveUiWithLinkIsShown(SAVE_DATA_TYPE_PASSWORD, "TAP ME IF YOU CAN");
        } else {
            saveUi = assertSaveUiWithLinkIsShown(SAVE_DATA_TYPE_PASSWORD);
            final UiObject2 changed = saveUi.findObject(By.res(mPackageName, ID_STATIC_TEXT));
            assertThat(changed.getText()).isEqualTo("ME!");
        }
        tapSaveUiLink(saveUi);

        // Make sure new activity is shown...
        WelcomeActivity.assertShowingDefaultMessage(mUiBot);
        mUiBot.assertSaveNotShowing(SAVE_DATA_TYPE_PASSWORD);
    }
}
