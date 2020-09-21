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

import static android.autofillservice.cts.AbstractDatePickerActivity.ID_DATE_PICKER;
import static android.autofillservice.cts.AbstractDatePickerActivity.ID_OUTPUT;
import static android.autofillservice.cts.Helper.getContext;
import static android.service.autofill.SaveInfo.SAVE_DATA_TYPE_GENERIC;

import static com.google.common.truth.Truth.assertThat;

import android.autofillservice.cts.CannedFillResponse.CannedDataset;
import android.icu.text.SimpleDateFormat;
import android.platform.test.annotations.AppModeFull;
import android.service.autofill.CustomDescription;
import android.service.autofill.DateTransformation;
import android.service.autofill.DateValueSanitizer;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.UiObject2;
import android.view.autofill.AutofillId;
import android.widget.RemoteViews;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;

import java.util.Calendar;

@AppModeFull // Service-specific test
public class CustomDescriptionDateTest extends AutoFillServiceTestCase {

    @Rule
    public final AutofillActivityTestRule<DatePickerSpinnerActivity> mActivityRule = new
            AutofillActivityTestRule<DatePickerSpinnerActivity>(DatePickerSpinnerActivity.class);

    private DatePickerSpinnerActivity mActivity;

    @Before
    public void setActivity() {
        mActivity = mActivityRule.getActivity();
    }

    @Test
    public void testCustomSave() throws Exception {
        // Set service.
        enableService();

        final AutofillId id = mActivity.getDatePicker().getAutofillId();

        // Set expectations.
        sReplier.addResponse(new CannedFillResponse.Builder()
                .setCustomDescription(new CustomDescription
                        .Builder(newTemplate(R.layout.two_horizontal_text_fields))
                        .addChild(R.id.first,
                                new DateTransformation(id, new SimpleDateFormat("MM/yyyy")))
                        .addChild(R.id.second,
                                new DateTransformation(id, new SimpleDateFormat("MM-yy")))
                        .build())
                .setRequiredSavableIds(SAVE_DATA_TYPE_GENERIC, ID_OUTPUT, ID_DATE_PICKER)
                .build());

        // Trigger auto-fill.
        mActivity.onOutput((v) -> v.requestFocus());
        sReplier.getNextFillRequest();

        // Autofill it.
        mUiBot.assertNoDatasetsEver();

        // Trigger save.
        mActivity.setDate(2010, Calendar.DECEMBER, 12);
        mActivity.tapOk();

        // First, make sure the UI is shown...
        final UiObject2 saveUi = mUiBot.assertSaveShowing(SAVE_DATA_TYPE_GENERIC);

        // Then, make sure it does have the custom view on it...
        final UiObject2 staticText = saveUi.findObject(By.res(mPackageName, "static_text"));
        assertThat(staticText).isNotNull();
        assertThat(staticText.getText()).isEqualTo("YO:");

        // Finally, assert the custom lines are shown
        mUiBot.assertChild(saveUi, "first", (o) -> assertThat(o.getText()).isEqualTo("12/2010"));
        mUiBot.assertChild(saveUi, "second", (o) -> assertThat(o.getText()).isEqualTo("12-10"));
    }

    @Test
    public void testSaveSameValue_usingSanitization() throws Exception {
        sanitizationTest(true);
    }

    @Test
    public void testSaveSameValue_withoutSanitization() throws Exception {
        sanitizationTest(false);
    }

    private void sanitizationTest(boolean withSanitization) throws Exception {
        // Set service.
        enableService();

        final AutofillId id = mActivity.getDatePicker().getAutofillId();

        // Set expectations.
        final Calendar cal = Calendar.getInstance();
        cal.clear();
        cal.set(Calendar.YEAR, 2012);
        cal.set(Calendar.MONTH, Calendar.DECEMBER);

        // Set expectations.

        // NOTE: ID_OUTPUT is used to trigger autofill, but it's value will be automatically
        // changed, hence we need to set the expected value as the formated one. Ideally
        // we shouldn't worry about that, but that would require creating a new activitiy with
        // a custom edit text that uses date autofill values...
        final CannedFillResponse.Builder response = new CannedFillResponse.Builder()
                .addDataset(new CannedDataset.Builder()
                        .setPresentation(createPresentation("The end of the world"))
                        .setField(ID_OUTPUT, "2012/11/25")
                        .setField(ID_DATE_PICKER, cal.getTimeInMillis())
                        .build())
                .setRequiredSavableIds(SAVE_DATA_TYPE_GENERIC, ID_OUTPUT, ID_DATE_PICKER);

        if (withSanitization) {
            response.addSanitizer(new DateValueSanitizer(new SimpleDateFormat("MM/yyyy")), id);
        }
        sReplier.addResponse(response.build());

        // Trigger autofill.
        mActivity.onOutput((v) -> v.requestFocus());
        sReplier.getNextFillRequest();
        mUiBot.assertDatasets("The end of the world");

        // Manually set same values as dataset.
        mActivity.onOutput((v) -> v.setText("whatever"));
        mActivity.setDate(2012, Calendar.DECEMBER, 25);
        mActivity.tapOk();

        // Verify save behavior.
        if (withSanitization) {
            mUiBot.assertSaveNotShowing(SAVE_DATA_TYPE_GENERIC);
        } else {
            mUiBot.assertSaveShowing(SAVE_DATA_TYPE_GENERIC);
        }
    }

    private RemoteViews newTemplate(int resourceId) {
        return new RemoteViews(getContext().getPackageName(), resourceId);
    }
}
