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
import static android.autofillservice.cts.Helper.getContext;
import static android.service.autofill.SaveInfo.SAVE_DATA_TYPE_GENERIC;

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;

import android.platform.test.annotations.AppModeFull;
import android.service.autofill.BatchUpdates;
import android.service.autofill.CharSequenceTransformation;
import android.service.autofill.CustomDescription;
import android.service.autofill.ImageTransformation;
import android.service.autofill.RegexValidator;
import android.service.autofill.Validator;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.UiObject2;
import android.view.View;
import android.view.autofill.AutofillId;
import android.widget.RemoteViews;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;

import java.util.function.BiFunction;
import java.util.regex.Pattern;

@AppModeFull // Service-specific test
public class CustomDescriptionTest extends AutoFillServiceTestCase {
    @Rule
    public final AutofillActivityTestRule<LoginActivity> mActivityRule =
            new AutofillActivityTestRule<>(LoginActivity.class);

    private LoginActivity mActivity;

    @Before
    public void setActivity() {
        mActivity = mActivityRule.getActivity();
    }

    /**
     * Base test
     *
     * @param descriptionBuilder method to build a custom description
     * @param uiVerifier         Ran when the custom description is shown
     */
    private void testCustomDescription(
            @NonNull BiFunction<AutofillId, AutofillId, CustomDescription> descriptionBuilder,
            @Nullable Runnable uiVerifier) throws Exception {
        enableService();

        final AutofillId usernameId = mActivity.getUsername().getAutofillId();
        final AutofillId passwordId = mActivity.getPassword().getAutofillId();

        // Set response with custom description
        sReplier.addResponse(new CannedFillResponse.Builder()
                .setRequiredSavableIds(SAVE_DATA_TYPE_GENERIC, ID_USERNAME, ID_PASSWORD)
                .setCustomDescription(descriptionBuilder.apply(usernameId, passwordId))
                .build());

        // Trigger autofill with custom description
        mActivity.onPassword(View::requestFocus);

        // Wait for onFill() before proceeding.
        sReplier.getNextFillRequest();

        // Trigger save.
        mActivity.onUsername((v) -> v.setText("usernm"));
        mActivity.onPassword((v) -> v.setText("passwd"));
        mActivity.tapLogin();

        if (uiVerifier != null) {
            uiVerifier.run();
        }

        mUiBot.saveForAutofill(true, SAVE_DATA_TYPE_GENERIC);
        sReplier.getNextSaveRequest();
    }

    @Test
    public void validTransformation() throws Exception {
        testCustomDescription((usernameId, passwordId) -> {
            RemoteViews presentation = newTemplate(R.layout.two_horizontal_text_fields);

            CharSequenceTransformation trans1 = new CharSequenceTransformation
                    .Builder(usernameId, Pattern.compile("(.*)"), "$1")
                    .addField(passwordId, Pattern.compile(".*(..)"), "..$1")
                    .build();
            @SuppressWarnings("deprecation")
            ImageTransformation trans2 = new ImageTransformation
                    .Builder(usernameId, Pattern.compile(".*"),
                    R.drawable.android).build();

            return new CustomDescription.Builder(presentation)
                    .addChild(R.id.first, trans1)
                    .addChild(R.id.img, trans2)
                    .build();
        }, () -> assertSaveUiIsShownWithTwoLines("usernm..wd"));
    }

    @Test
    public void validTransformationWithOneTemplateUpdate() throws Exception {
        testCustomDescription((usernameId, passwordId) -> {
            RemoteViews presentation = newTemplate(R.layout.two_horizontal_text_fields);

            CharSequenceTransformation trans1 = new CharSequenceTransformation
                    .Builder(usernameId, Pattern.compile("(.*)"), "$1")
                    .addField(passwordId, Pattern.compile(".*(..)"), "..$1")
                    .build();
            @SuppressWarnings("deprecation")
            ImageTransformation trans2 = new ImageTransformation
                    .Builder(usernameId, Pattern.compile(".*"),
                    R.drawable.android).build();
            RemoteViews update = newTemplate(0); // layout id not really used
            update.setViewVisibility(R.id.second, View.GONE);
            Validator condition = new RegexValidator(usernameId, Pattern.compile(".*"));

            return new CustomDescription.Builder(presentation)
                    .addChild(R.id.first, trans1)
                    .addChild(R.id.img, trans2)
                    .batchUpdate(condition,
                            new BatchUpdates.Builder().updateTemplate(update).build())
                    .build();
        }, () -> assertSaveUiIsShownWithJustOneLine("usernm..wd"));
    }

    @Test
    public void validTransformationWithMultipleTemplateUpdates() throws Exception {
        testCustomDescription((usernameId, passwordId) -> {
            RemoteViews presentation = newTemplate(R.layout.two_horizontal_text_fields);

            CharSequenceTransformation trans1 = new CharSequenceTransformation.Builder(usernameId,
                    Pattern.compile("(.*)"), "$1")
                            .addField(passwordId, Pattern.compile(".*(..)"), "..$1")
                            .build();
            @SuppressWarnings("deprecation")
            ImageTransformation trans2 = new ImageTransformation.Builder(usernameId,
                    Pattern.compile(".*"), R.drawable.android)
                    .build();

            Validator validCondition = new RegexValidator(usernameId, Pattern.compile(".*"));
            Validator invalidCondition = new RegexValidator(usernameId, Pattern.compile("D'OH"));

            // Line 1 updates
            RemoteViews update1 = newTemplate(666); // layout id not really used
            update1.setContentDescription(R.id.first, "First am I"); // valid
            RemoteViews update2 = newTemplate(0); // layout id not really used
            update2.setViewVisibility(R.id.first, View.GONE); // invalid

            // Line 2 updates
            RemoteViews update3 = newTemplate(-666); // layout id not really used
            update3.setTextViewText(R.id.second, "First of his second name"); // valid
            RemoteViews update4 = newTemplate(0); // layout id not really used
            update4.setTextViewText(R.id.second, "SECOND of his second name"); // invalid

            return new CustomDescription.Builder(presentation)
                    .addChild(R.id.first, trans1)
                    .addChild(R.id.img, trans2)
                    .batchUpdate(validCondition,
                            new BatchUpdates.Builder().updateTemplate(update1).build())
                    .batchUpdate(invalidCondition,
                            new BatchUpdates.Builder().updateTemplate(update2).build())
                    .batchUpdate(validCondition,
                            new BatchUpdates.Builder().updateTemplate(update3).build())
                    .batchUpdate(invalidCondition,
                            new BatchUpdates.Builder().updateTemplate(update4).build())
                    .build();
        }, () -> assertSaveUiWithLinesIsShown(
                (line1) -> assertWithMessage("Wrong content description for line1")
                        .that(line1.getContentDescription()).isEqualTo("First am I"),
                (line2) -> assertWithMessage("Wrong text for line2").that(line2.getText())
                        .isEqualTo("First of his second name"),
                null));
    }

    @Test
    public void testMultipleBatchUpdates_noConditionPass() throws Exception {
        multipleBatchUpdatesTest(BatchUpdatesConditionType.NONE_PASS);
    }

    @Test
    public void testMultipleBatchUpdates_secondConditionPass() throws Exception {
        multipleBatchUpdatesTest(BatchUpdatesConditionType.SECOND_PASS);
    }

    @Test
    public void testMultipleBatchUpdates_thirdConditionPass() throws Exception {
        multipleBatchUpdatesTest(BatchUpdatesConditionType.THIRD_PASS);
    }

    @Test
    public void testMultipleBatchUpdates_allConditionsPass() throws Exception {
        multipleBatchUpdatesTest(BatchUpdatesConditionType.ALL_PASS);
    }

    private enum BatchUpdatesConditionType {
        NONE_PASS,
        SECOND_PASS,
        THIRD_PASS,
        ALL_PASS
    }

    /**
     * Tests a custom description that has 3 transformations, one applied directly and the other
     * 2 in batch updates.
     *
     * @param conditionsType defines which batch updates conditions will pass.
     */
    private void multipleBatchUpdatesTest(BatchUpdatesConditionType conditionsType)
            throws Exception {

        final boolean line2Pass = conditionsType == BatchUpdatesConditionType.SECOND_PASS
                || conditionsType == BatchUpdatesConditionType.ALL_PASS;
        final boolean line3Pass = conditionsType == BatchUpdatesConditionType.THIRD_PASS
                || conditionsType == BatchUpdatesConditionType.ALL_PASS;

        final Visitor<UiObject2> line1Visitor = (line1) -> assertWithMessage("Wrong text for line1")
                .that(line1.getText()).isEqualTo("L1-u");

        final Visitor<UiObject2> line2Visitor;
        if (line2Pass) {
            line2Visitor = (line2) -> assertWithMessage("Wrong text for line2")
                    .that(line2.getText()).isEqualTo("L2-u");
        } else {
            line2Visitor = null;
        }

        final Visitor<UiObject2> line3Visitor;
        if (line3Pass) {
            line3Visitor = (line3) -> assertWithMessage("Wrong text for line3")
                    .that(line3.getText()).isEqualTo("L3-p");
        } else {
            line3Visitor = null;
        }

        testCustomDescription((usernameId, passwordId) -> {
            Validator validCondition = new RegexValidator(usernameId, Pattern.compile(".*"));
            Validator invalidCondition = new RegexValidator(usernameId, Pattern.compile("D'OH"));
            Pattern firstCharGroupRegex = Pattern.compile("^(.).*$");

            final RemoteViews presentation =
                    newTemplate(R.layout.three_horizontal_text_fields_last_two_invisible);

            final CharSequenceTransformation line1Transformation =
                    new CharSequenceTransformation.Builder(usernameId, firstCharGroupRegex, "L1-$1")
                        .build();

            final CharSequenceTransformation line2Transformation =
                    new CharSequenceTransformation.Builder(usernameId, firstCharGroupRegex, "L2-$1")
                        .build();
            final RemoteViews line2Updates = newTemplate(666); // layout id not really used
            line2Updates.setViewVisibility(R.id.second, View.VISIBLE);

            final CharSequenceTransformation line3Transformation =
                    new CharSequenceTransformation.Builder(passwordId, firstCharGroupRegex, "L3-$1")
                        .build();
            final RemoteViews line3Updates = newTemplate(666); // layout id not really used
            line3Updates.setViewVisibility(R.id.third, View.VISIBLE);

            return new CustomDescription.Builder(presentation)
                    .addChild(R.id.first, line1Transformation)
                    .batchUpdate(line2Pass ? validCondition : invalidCondition,
                            new BatchUpdates.Builder()
                            .transformChild(R.id.second, line2Transformation)
                            .updateTemplate(line2Updates)
                            .build())
                    .batchUpdate(line3Pass ? validCondition : invalidCondition,
                            new BatchUpdates.Builder()
                            .transformChild(R.id.third, line3Transformation)
                            .updateTemplate(line3Updates)
                            .build())
                    .build();
        }, () -> assertSaveUiWithLinesIsShown(line1Visitor, line2Visitor, line3Visitor));
    }

    @Test
    public void testBatchUpdatesApplyUpdateFirstThenTransformations() throws Exception {

        final Visitor<UiObject2> line1Visitor = (line1) -> assertWithMessage("Wrong text for line1")
                .that(line1.getText()).isEqualTo("L1-u");
        final Visitor<UiObject2> line2Visitor = (line2) -> assertWithMessage("Wrong text for line2")
                .that(line2.getText()).isEqualTo("L2-u");
        final Visitor<UiObject2> line3Visitor = (line3) -> assertWithMessage("Wrong text for line3")
                .that(line3.getText()).isEqualTo("L3-p");

        testCustomDescription((usernameId, passwordId) -> {
            Validator validCondition = new RegexValidator(usernameId, Pattern.compile(".*"));
            Pattern firstCharGroupRegex = Pattern.compile("^(.).*$");

            final RemoteViews presentation =
                    newTemplate(R.layout.two_horizontal_text_fields);

            final CharSequenceTransformation line1Transformation =
                    new CharSequenceTransformation.Builder(usernameId, firstCharGroupRegex, "L1-$1")
                        .build();

            final CharSequenceTransformation line2Transformation =
                    new CharSequenceTransformation.Builder(usernameId, firstCharGroupRegex, "L2-$1")
                        .build();

            final CharSequenceTransformation line3Transformation =
                    new CharSequenceTransformation.Builder(passwordId, firstCharGroupRegex, "L3-$1")
                        .build();
            final RemoteViews line3Presentation = newTemplate(R.layout.third_line_only);
            final RemoteViews line3Updates = newTemplate(666); // layout id not really used
            line3Updates.addView(R.id.parent, line3Presentation);

            return new CustomDescription.Builder(presentation)
                    .addChild(R.id.first, line1Transformation)
                    .batchUpdate(validCondition,
                            new BatchUpdates.Builder()
                            .transformChild(R.id.second, line2Transformation)
                            .build())
                    .batchUpdate(validCondition,
                            new BatchUpdates.Builder()
                            .updateTemplate(line3Updates)
                            .transformChild(R.id.third, line3Transformation)
                            .build())
                    .build();
        }, () -> assertSaveUiWithLinesIsShown(line1Visitor, line2Visitor, line3Visitor));
    }

    @Test
    public void badImageTransformation() throws Exception {
        testCustomDescription((usernameId, passwordId) -> {
            RemoteViews presentation = newTemplate(R.layout.two_horizontal_text_fields);

            @SuppressWarnings("deprecation")
            ImageTransformation trans = new ImageTransformation.Builder(usernameId,
                    Pattern.compile(".*"), 1).build();

            return new CustomDescription.Builder(presentation)
                    .addChild(R.id.img, trans)
                    .build();
        }, () -> assertSaveUiWithCustomDescriptionIsShown());
    }

    @Test
    public void unusedImageTransformation() throws Exception {
        testCustomDescription((usernameId, passwordId) -> {
            RemoteViews presentation = newTemplate(R.layout.two_horizontal_text_fields);

            @SuppressWarnings("deprecation")
            ImageTransformation trans = new ImageTransformation
                    .Builder(usernameId, Pattern.compile("invalid"), R.drawable.android)
                    .build();

            return new CustomDescription.Builder(presentation)
                    .addChild(R.id.img, trans)
                    .build();
        }, () -> assertSaveUiWithCustomDescriptionIsShown());
    }

    @Test
    public void applyImageTransformationToTextView() throws Exception {
        testCustomDescription((usernameId, passwordId) -> {
            RemoteViews presentation = newTemplate(R.layout.two_horizontal_text_fields);

            @SuppressWarnings("deprecation")
            ImageTransformation trans = new ImageTransformation
                    .Builder(usernameId, Pattern.compile(".*"), R.drawable.android)
                    .build();

            return new CustomDescription.Builder(presentation)
                    .addChild(R.id.first, trans)
                    .build();
        }, () -> assertSaveUiWithoutCustomDescriptionIsShown());
    }

    @Test
    public void failFirstFailAll() throws Exception {
        testCustomDescription((usernameId, passwordId) -> {
            RemoteViews presentation = newTemplate(R.layout.two_horizontal_text_fields);

            CharSequenceTransformation trans = new CharSequenceTransformation
                    .Builder(usernameId, Pattern.compile("(.*)"), "$42")
                    .addField(passwordId, Pattern.compile(".*(..)"), "..$1")
                    .build();

            return new CustomDescription.Builder(presentation)
                    .addChild(R.id.first, trans)
                    .build();
        }, () -> assertSaveUiWithoutCustomDescriptionIsShown());
    }

    @Test
    public void failSecondFailAll() throws Exception {
        testCustomDescription((usernameId, passwordId) -> {
            RemoteViews presentation = newTemplate(R.layout.two_horizontal_text_fields);

            CharSequenceTransformation trans = new CharSequenceTransformation
                    .Builder(usernameId, Pattern.compile("(.*)"), "$1")
                    .addField(passwordId, Pattern.compile(".*(..)"), "..$42")
                    .build();

            return new CustomDescription.Builder(presentation)
                    .addChild(R.id.first, trans)
                    .build();
        }, () -> assertSaveUiWithoutCustomDescriptionIsShown());
    }

    @Test
    public void applyCharSequenceTransformationToImageView() throws Exception {
        testCustomDescription((usernameId, passwordId) -> {
            RemoteViews presentation = newTemplate(R.layout.two_horizontal_text_fields);

            CharSequenceTransformation trans = new CharSequenceTransformation
                    .Builder(usernameId, Pattern.compile("(.*)"), "$1")
                    .build();

            return new CustomDescription.Builder(presentation)
                    .addChild(R.id.img, trans)
                    .build();
        }, () -> assertSaveUiWithoutCustomDescriptionIsShown());
    }

    private void multipleTransformationsForSameFieldTest(boolean matchFirst) throws Exception {
        enableService();

        // Set response with custom description
        final AutofillId usernameId = mActivity.getUsername().getAutofillId();
        final CharSequenceTransformation firstTrans = new CharSequenceTransformation
                .Builder(usernameId, Pattern.compile("(marco)"), "polo")
                .build();
        final CharSequenceTransformation secondTrans = new CharSequenceTransformation
                .Builder(usernameId, Pattern.compile("(MARCO)"), "POLO")
                .build();
        RemoteViews presentation = newTemplate(R.layout.two_horizontal_text_fields);
        final CustomDescription customDescription = new CustomDescription.Builder(presentation)
                .addChild(R.id.first, firstTrans)
                .addChild(R.id.first, secondTrans)
                .build();
        sReplier.addResponse(new CannedFillResponse.Builder()
                .setRequiredSavableIds(SAVE_DATA_TYPE_GENERIC, ID_USERNAME)
                .setCustomDescription(customDescription)
                .build());

        // Trigger autofill with custom description
        mActivity.onPassword(View::requestFocus);

        // Wait for onFill() before proceeding.
        sReplier.getNextFillRequest();

        // Trigger save.
        final String username = matchFirst ? "marco" : "MARCO";
        mActivity.onUsername((v) -> v.setText(username));
        mActivity.onPassword((v) -> v.setText(LoginActivity.BACKDOOR_PASSWORD_SUBSTRING));
        mActivity.tapLogin();

        final String expectedText = matchFirst ? "polo" : "POLO";
        assertSaveUiIsShownWithTwoLines(expectedText);
    }

    @Test
    public void applyMultipleTransformationsForSameField_matchFirst() throws Exception {
        multipleTransformationsForSameFieldTest(true);
    }

    @Test
    public void applyMultipleTransformationsForSameField_matchSecond() throws Exception {
        multipleTransformationsForSameFieldTest(false);
    }

    private RemoteViews newTemplate(int resourceId) {
        return new RemoteViews(getContext().getPackageName(), resourceId);
    }

    private UiObject2 assertSaveUiShowing() {
        try {
            return mUiBot.assertSaveShowing(SAVE_DATA_TYPE_GENERIC);
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }

    private void assertSaveUiWithoutCustomDescriptionIsShown() {
        // First make sure the UI is shown...
        final UiObject2 saveUi = assertSaveUiShowing();

        // Then make sure it does not have the custom view on it.
        assertWithMessage("found static_text on SaveUI (%s)", mUiBot.getChildrenAsText(saveUi))
            .that(saveUi.findObject(By.res(mPackageName, "static_text"))).isNull();
    }

    private UiObject2 assertSaveUiWithCustomDescriptionIsShown() {
        // First make sure the UI is shown...
        final UiObject2 saveUi = assertSaveUiShowing();

        // Then make sure it does have the custom view on it...
        final UiObject2 staticText = saveUi.findObject(By.res(mPackageName, "static_text"));
        assertThat(staticText).isNotNull();
        assertThat(staticText.getText()).isEqualTo("YO:");

        return saveUi;
    }

    /**
     * Asserts the save ui only has {@code first} and {@code second} lines (i.e, {@code third} is
     * invisible), but only {@code first} has text.
     */
    private UiObject2 assertSaveUiIsShownWithTwoLines(String expectedTextOnFirst) {
        return assertSaveUiWithLinesIsShown(
                (line1) -> assertWithMessage("Wrong text for child with id 'first'")
                        .that(line1.getText()).isEqualTo(expectedTextOnFirst),
                (line2) -> assertWithMessage("Wrong text for child with id 'second'")
                        .that(line2.getText()).isNull(),
                null);
    }

    /**
     * Asserts the save ui only has {@code first} line (i.e., {@code second} and {@code third} are
     * invisible).
     */
    private void assertSaveUiIsShownWithJustOneLine(String expectedTextOnFirst) {
        assertSaveUiWithLinesIsShown(
                (line1) -> assertWithMessage("Wrong text for child with id 'first'")
                        .that(line1.getText()).isEqualTo(expectedTextOnFirst),
                null, null);
    }

    private UiObject2 assertSaveUiWithLinesIsShown(@Nullable Visitor<UiObject2> line1Visitor,
            @Nullable Visitor<UiObject2> line2Visitor, @Nullable Visitor<UiObject2> line3Visitor) {
        final UiObject2 saveUi = assertSaveUiWithCustomDescriptionIsShown();
        mUiBot.assertChild(saveUi, "first", line1Visitor);
        mUiBot.assertChild(saveUi, "second", line2Visitor);
        mUiBot.assertChild(saveUi, "third", line3Visitor);
        return saveUi;
    }
}
