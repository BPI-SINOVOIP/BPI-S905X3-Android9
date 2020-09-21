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

import static android.service.autofill.Validators.and;
import static android.service.autofill.Validators.not;
import static android.service.autofill.Validators.or;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.testng.Assert.assertThrows;

import android.platform.test.annotations.AppModeFull;
import android.service.autofill.InternalValidator;
import android.service.autofill.Validator;
import android.service.autofill.ValueFinder;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnitRunner;

@RunWith(MockitoJUnitRunner.class)
@AppModeFull // Unit test
public class ValidatorsTest extends AutoFillServiceTestCase {

    @Mock private Validator mInvalidValidator;
    @Mock private ValueFinder mValueFinder;
    @Mock private InternalValidator mValidValidator;
    @Mock private InternalValidator mValidValidator2;

    @Test
    public void testAnd_null() {
        assertThrows(NullPointerException.class, () -> and((Validator) null));
        assertThrows(NullPointerException.class, () -> and(mValidValidator, null));
        assertThrows(NullPointerException.class, () -> and(null, mValidValidator));
    }

    @Test
    public void testAnd_invalid() {
        assertThrows(IllegalArgumentException.class, () -> and(mInvalidValidator));
        assertThrows(IllegalArgumentException.class, () -> and(mValidValidator, mInvalidValidator));
        assertThrows(IllegalArgumentException.class, () -> and(mInvalidValidator, mValidValidator));
    }

    @Test
    public void testAnd_firstFailed() {
        doReturn(false).when(mValidValidator).isValid(mValueFinder);
        assertThat(((InternalValidator) and(mValidValidator, mValidValidator2))
                .isValid(mValueFinder)).isFalse();
        verify(mValidValidator2, never()).isValid(mValueFinder);
    }

    @Test
    public void testAnd_firstPassedSecondFailed() {
        doReturn(true).when(mValidValidator).isValid(mValueFinder);
        doReturn(false).when(mValidValidator2).isValid(mValueFinder);
        assertThat(((InternalValidator) and(mValidValidator, mValidValidator2))
                .isValid(mValueFinder)).isFalse();
    }

    @Test
    public void testAnd_AllPassed() {
        doReturn(true).when(mValidValidator).isValid(mValueFinder);
        doReturn(true).when(mValidValidator2).isValid(mValueFinder);
        assertThat(((InternalValidator) and(mValidValidator, mValidValidator2))
                .isValid(mValueFinder)).isTrue();
    }

    @Test
    public void testOr_null() {
        assertThrows(NullPointerException.class, () -> or((Validator) null));
        assertThrows(NullPointerException.class, () -> or(mValidValidator, null));
        assertThrows(NullPointerException.class, () -> or(null, mValidValidator));
    }

    @Test
    public void testOr_invalid() {
        assertThrows(IllegalArgumentException.class, () -> or(mInvalidValidator));
        assertThrows(IllegalArgumentException.class, () -> or(mValidValidator, mInvalidValidator));
        assertThrows(IllegalArgumentException.class, () -> or(mInvalidValidator, mValidValidator));
    }

    @Test
    public void testOr_AllFailed() {
        doReturn(false).when(mValidValidator).isValid(mValueFinder);
        doReturn(false).when(mValidValidator2).isValid(mValueFinder);
        assertThat(((InternalValidator) or(mValidValidator, mValidValidator2))
                .isValid(mValueFinder)).isFalse();
    }

    @Test
    public void testOr_firstPassed() {
        doReturn(true).when(mValidValidator).isValid(mValueFinder);
        assertThat(((InternalValidator) or(mValidValidator, mValidValidator2))
                .isValid(mValueFinder)).isTrue();
        verify(mValidValidator2, never()).isValid(mValueFinder);
    }

    @Test
    public void testOr_secondPassed() {
        doReturn(false).when(mValidValidator).isValid(mValueFinder);
        doReturn(true).when(mValidValidator2).isValid(mValueFinder);
        assertThat(((InternalValidator) or(mValidValidator, mValidValidator2))
                .isValid(mValueFinder)).isTrue();
    }

    @Test
    public void testNot_null() {
        assertThrows(IllegalArgumentException.class, () -> not(null));
    }

    @Test
    public void testNot_invalidClass() {
        assertThrows(IllegalArgumentException.class, () -> not(mInvalidValidator));
    }

    @Test
    public void testNot_falseToTrue() {
        doReturn(false).when(mValidValidator).isValid(mValueFinder);
        final InternalValidator notValidator = (InternalValidator) not(mValidValidator);
        assertThat(notValidator.isValid(mValueFinder)).isTrue();
    }

    @Test
    public void testNot_trueToFalse() {
        doReturn(true).when(mValidValidator).isValid(mValueFinder);
        final InternalValidator notValidator = (InternalValidator) not(mValidValidator);
        assertThat(notValidator.isValid(mValueFinder)).isFalse();
    }
}
