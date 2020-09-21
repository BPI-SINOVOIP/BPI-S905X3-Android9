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

import static com.google.common.truth.Truth.assertThat;

import static org.testng.Assert.assertThrows;

import android.icu.text.SimpleDateFormat;
import android.icu.util.Calendar;
import android.platform.test.annotations.AppModeFull;
import android.service.autofill.DateValueSanitizer;
import android.support.test.runner.AndroidJUnit4;
import android.util.Log;
import android.view.autofill.AutofillValue;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.Date;

@RunWith(AndroidJUnit4.class)
@AppModeFull // Unit test
public class DateValueSanitizerTest {

    private static final String TAG = "DateValueSanitizerTest";

    private final SimpleDateFormat mDateFormat = new SimpleDateFormat("MM/yyyy");

    @Test
    public void testConstructor_nullDateFormat() {
        assertThrows(NullPointerException.class, () -> new DateValueSanitizer(null));
    }

    @Test
    public void testSanitize_nullValue() throws Exception {
        final DateValueSanitizer sanitizer = new DateValueSanitizer(new SimpleDateFormat());
        assertThat(sanitizer.sanitize(null)).isNull();
    }

    @Test
    public void testSanitize_invalidValue() throws Exception {
        final DateValueSanitizer sanitizer = new DateValueSanitizer(new SimpleDateFormat());
        assertThat(sanitizer.sanitize(AutofillValue.forText("D'OH!"))).isNull();
    }

    @Test
    public void testSanitize_ok() throws Exception {
        final Calendar inputCal = Calendar.getInstance();
        inputCal.set(Calendar.YEAR, 2012);
        inputCal.set(Calendar.MONTH, Calendar.DECEMBER);
        inputCal.set(Calendar.DAY_OF_MONTH, 20);
        final long inputDate = inputCal.getTimeInMillis();
        final AutofillValue inputValue = AutofillValue.forDate(inputDate);
        Log.v(TAG, "Input date: " + inputDate + " >> " + new Date(inputDate));

        final Calendar expectedCal = Calendar.getInstance();
        expectedCal.clear(); // We just care for year and month...
        expectedCal.set(Calendar.YEAR, 2012);
        expectedCal.set(Calendar.MONTH, Calendar.DECEMBER);
        final long expectedDate = expectedCal.getTimeInMillis();
        final AutofillValue expectedValue = AutofillValue.forDate(expectedDate);
        Log.v(TAG, "Exected date: " + expectedDate + " >> " + new Date(expectedDate));

        final DateValueSanitizer sanitizer = new DateValueSanitizer(
                mDateFormat);
        final AutofillValue sanitizedValue = sanitizer.sanitize(inputValue);
        final long sanitizedDate = sanitizedValue.getDateValue();
        Log.v(TAG, "Sanitized date: " + sanitizedDate + " >> " + new Date(sanitizedDate));
        assertThat(sanitizedDate).isEqualTo(expectedDate);
        assertThat(sanitizedValue).isEqualTo(expectedValue);
    }
}
