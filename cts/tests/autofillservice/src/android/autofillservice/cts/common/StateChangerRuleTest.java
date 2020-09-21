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

package android.autofillservice.cts.common;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.Mockito.doThrow;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoMoreInteractions;
import static org.mockito.Mockito.when;
import static org.testng.Assert.assertThrows;
import static org.testng.Assert.expectThrows;

import android.platform.test.annotations.AppModeFull;

import org.junit.Test;
import org.junit.runner.Description;
import org.junit.runner.RunWith;
import org.junit.runners.model.Statement;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnitRunner;

@RunWith(MockitoJUnitRunner.class)
@AppModeFull // Unit test
public class StateChangerRuleTest {

    private final RuntimeException mRuntimeException = new RuntimeException("D'OH");
    private final Description mDescription = Description.createSuiteDescription("Whatever");

    @Mock
    private StateManager<String> mStateManager;

    @Mock
    private Statement mStatement;

    @Test
    public void testInvalidConstructor() {
        assertThrows(NullPointerException.class,
                () -> new StateChangerRule<Object>(null, "value"));
    }

    @Test
    public void testSetAndRestoreOnSuccess() throws Throwable {
        final StateChangerRule<String> rule = new StateChangerRule<>(mStateManager,
                "newValue");
        when(mStateManager.get()).thenReturn("before", "changed");

        rule.apply(mStatement, mDescription).evaluate();

        verify(mStatement, times(1)).evaluate();
        verify(mStateManager, times(2)).get(); // Needed because of verifyNoMoreInteractions()
        verify(mStateManager, times(1)).set("newValue");
        verify(mStateManager, times(1)).set("before");
        verifyNoMoreInteractions(mStateManager); // Make sure set() was not called again
    }

    @Test
    public void testDontSetIfSameValueOnSuccess() throws Throwable {
        final StateChangerRule<String> rule = new StateChangerRule<>(mStateManager,
                "sameValue");
        when(mStateManager.get()).thenReturn("sameValue");

        rule.apply(mStatement, mDescription).evaluate();

        verify(mStatement, times(1)).evaluate();
        verify(mStateManager, never()).set(anyString());
    }

    @Test
    public void testSetButDontRestoreIfSameValueOnSuccess() throws Throwable {
        final StateChangerRule<String> rule = new StateChangerRule<>(mStateManager,
                "newValue");
        when(mStateManager.get()).thenReturn("before", "before");

        rule.apply(mStatement, mDescription).evaluate();

        verify(mStatement, times(1)).evaluate();
        verify(mStateManager, times(2)).get(); // Needed because of verifyNoMoreInteractions()
        verify(mStateManager, times(1)).set("newValue");
        verifyNoMoreInteractions(mStateManager); // Make sure set() was not called again
    }

    @Test
    public void testDontSetButRestoreIfValueChangedOnSuccess() throws Throwable {
        final StateChangerRule<String> rule = new StateChangerRule<>(mStateManager,
                "sameValue");
        when(mStateManager.get()).thenReturn("sameValue", "changed");

        rule.apply(mStatement, mDescription).evaluate();

        verify(mStateManager, times(2)).get(); // Needed because of verifyNoMoreInteractions()
        verify(mStatement, times(1)).evaluate();
        verify(mStateManager, times(1)).set("sameValue");
        verifyNoMoreInteractions(mStateManager); // Make sure set() was not called again
    }

    @Test
    public void testSetAndRestoreOnFailure() throws Throwable {
        final StateChangerRule<String> rule = new StateChangerRule<>(mStateManager,
                "newValue");
        when(mStateManager.get()).thenReturn("before", "changed");
        doThrow(mRuntimeException).when(mStatement).evaluate();

        final RuntimeException actualException = expectThrows(RuntimeException.class,
                () -> rule.apply(mStatement, mDescription).evaluate());
        assertThat(actualException).isSameAs(mRuntimeException);

        verify(mStateManager, times(2)).get(); // Needed because of verifyNoMoreInteractions()
        verify(mStateManager, times(1)).set("newValue");
        verify(mStateManager, times(1)).set("before");
        verifyNoMoreInteractions(mStateManager); // Make sure set() was not called again
    }

    @Test
    public void testDontSetIfSameValueOnFailure() throws Throwable {
        final StateChangerRule<String> rule = new StateChangerRule<>(mStateManager,
                "sameValue");
        when(mStateManager.get()).thenReturn("sameValue");
        doThrow(mRuntimeException).when(mStatement).evaluate();

        final RuntimeException actualException = expectThrows(RuntimeException.class,
                () -> rule.apply(mStatement, mDescription).evaluate());
        assertThat(actualException).isSameAs(mRuntimeException);

        verify(mStateManager, never()).set(anyString());
    }

    @Test
    public void testSetButDontRestoreIfSameValueOnFailure() throws Throwable {
        final StateChangerRule<String> rule = new StateChangerRule<>(mStateManager,
                "newValue");
        when(mStateManager.get()).thenReturn("before", "before");
        doThrow(mRuntimeException).when(mStatement).evaluate();

        final RuntimeException actualException = expectThrows(RuntimeException.class,
                () -> rule.apply(mStatement, mDescription).evaluate());
        assertThat(actualException).isSameAs(mRuntimeException);

        verify(mStateManager, times(2)).get(); // Needed because of verifyNoMoreInteractions()
        verify(mStateManager, times(1)).set("newValue");
        verifyNoMoreInteractions(mStateManager); // Make sure set() was not called again
    }

    @Test
    public void testDontSetButRestoreIfValueChangedOnFailure() throws Throwable {
        final StateChangerRule<String> rule = new StateChangerRule<>(mStateManager,
                "sameValue");
        when(mStateManager.get()).thenReturn("sameValue", "changed");
        doThrow(mRuntimeException).when(mStatement).evaluate();

        final RuntimeException actualException = expectThrows(RuntimeException.class,
                () -> rule.apply(mStatement, mDescription).evaluate());
        assertThat(actualException).isSameAs(mRuntimeException);

        verify(mStateManager, times(2)).get(); // Needed because of verifyNoMoreInteractions()
        verify(mStateManager, times(1)).set("sameValue");
        verifyNoMoreInteractions(mStateManager); // Make sure set() was not called again
    }
}
