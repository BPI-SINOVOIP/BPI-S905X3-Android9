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

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.argThat;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static org.testng.Assert.assertThrows;

import android.icu.text.SimpleDateFormat;
import android.icu.util.Calendar;
import android.platform.test.annotations.AppModeFull;
import android.service.autofill.DateTransformation;
import android.service.autofill.ValueFinder;
import android.view.autofill.AutofillId;
import android.view.autofill.AutofillValue;
import android.widget.RemoteViews;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnitRunner;

@RunWith(MockitoJUnitRunner.class)
@AppModeFull // Unit test
public class DateTransformationTest {

    @Mock private ValueFinder mValueFinder;
    @Mock private RemoteViews mTemplate;

    private final AutofillId mFieldId = new AutofillId(42);

    @Test
    public void testConstructor_nullFieldId() {
        assertThrows(NullPointerException.class,
                () -> new DateTransformation(null, new SimpleDateFormat()));
    }

    @Test
    public void testConstructor_nullDateFormat() {
        assertThrows(NullPointerException.class, () -> new DateTransformation(mFieldId, null));
    }

    @Test
    public void testFieldNotFound() throws Exception {
        final DateTransformation trans = new DateTransformation(mFieldId, new SimpleDateFormat());

        trans.apply(mValueFinder, mTemplate, 0);

        verify(mTemplate, never()).setCharSequence(eq(0), any(), any());
    }

    @Test
    public void testInvalidAutofillValueType() throws Exception {
        final DateTransformation trans = new DateTransformation(mFieldId, new SimpleDateFormat());

        when(mValueFinder.findRawValueByAutofillId(mFieldId))
                .thenReturn(AutofillValue.forText("D'OH"));
        trans.apply(mValueFinder, mTemplate, 0);

        verify(mTemplate, never()).setCharSequence(eq(0), any(), any());
    }

    @Test
    public void testValidAutofillValue() throws Exception {
        final DateTransformation trans = new DateTransformation(mFieldId,
                new SimpleDateFormat("MM/yyyy"));

        final Calendar cal = Calendar.getInstance();
        cal.set(Calendar.YEAR, 2012);
        cal.set(Calendar.MONTH, Calendar.DECEMBER);
        cal.set(Calendar.DAY_OF_MONTH, 20);

        when(mValueFinder.findRawValueByAutofillId(mFieldId))
                .thenReturn(AutofillValue.forDate(cal.getTimeInMillis()));

        trans.apply(mValueFinder, mTemplate, 0);

        verify(mTemplate).setCharSequence(eq(0), any(),
                argThat(new CharSequenceMatcher("12/2012")));
    }
}
