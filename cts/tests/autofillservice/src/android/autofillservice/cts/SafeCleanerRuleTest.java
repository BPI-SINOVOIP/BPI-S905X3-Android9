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

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.Mockito.verify;
import static org.testng.Assert.expectThrows;

import android.autofillservice.cts.SafeCleanerRule.Dumper;
import android.platform.test.annotations.AppModeFull;

import com.google.common.collect.ImmutableList;

import org.junit.Test;
import org.junit.runner.Description;
import org.junit.runner.RunWith;
import org.junit.runners.model.Statement;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnitRunner;

import java.util.List;
import java.util.concurrent.Callable;

@RunWith(MockitoJUnitRunner.class)
@AppModeFull // Unit test
public class SafeCleanerRuleTest {

    private static class FailureStatement extends Statement {
        private final Throwable mThrowable;

        FailureStatement(Throwable t) {
            mThrowable = t;
        }

        @Override
        public void evaluate() throws Throwable {
            throw mThrowable;
        }
    }

    private final Description mDescription = Description.createSuiteDescription("Whatever");
    private final RuntimeException mRuntimeException = new RuntimeException("D'OH!");

    @Mock private Dumper mDumper;

    // Use mocks for objects that don't throw any exception.
    @Mock private Runnable mGoodGuyRunner1;
    @Mock private Runnable mGoodGuyRunner2;
    @Mock private Callable<List<Throwable>> mGoodGuyExtraExceptions1;
    @Mock private Callable<List<Throwable>> mGoodGuyExtraExceptions2;
    @Mock private Statement mGoodGuyStatement;

    @Test
    public void testEmptyRule_testPass() throws Throwable {
        final SafeCleanerRule rule = new SafeCleanerRule();
        rule.apply(mGoodGuyStatement, mDescription).evaluate();
    }

    @Test
    public void testEmptyRule_testFails() throws Throwable {
        final SafeCleanerRule rule = new SafeCleanerRule();
        final Throwable actualException = expectThrows(RuntimeException.class,
                () -> rule.apply(new FailureStatement(mRuntimeException), mDescription).evaluate());
        assertThat(actualException).isSameAs(mRuntimeException);
    }

    @Test
    public void testEmptyRule_testFails_withDumper() throws Throwable {
        final SafeCleanerRule rule = new SafeCleanerRule().setDumper(mDumper);
        final Throwable actualException = expectThrows(RuntimeException.class,
                () -> rule.apply(new FailureStatement(mRuntimeException), mDescription).evaluate());
        assertThat(actualException).isSameAs(mRuntimeException);
        verify(mDumper).dump("Whatever", actualException);
    }

    @Test
    public void testOnlyTestFails() throws Throwable {
        final SafeCleanerRule rule = new SafeCleanerRule()
                .run(mGoodGuyRunner1)
                .add(mGoodGuyExtraExceptions1);
        final Throwable actualException = expectThrows(RuntimeException.class,
                () -> rule.apply(new FailureStatement(mRuntimeException), mDescription).evaluate());
        assertThat(actualException).isSameAs(mRuntimeException);
        verify(mGoodGuyRunner1).run();
        verify(mGoodGuyExtraExceptions1).call();
    }

    @Test
    public void testOnlyTestFails_withDumper() throws Throwable {
        final SafeCleanerRule rule = new SafeCleanerRule()
                .setDumper(mDumper)
                .run(mGoodGuyRunner1)
                .add(mGoodGuyExtraExceptions1);
        final Throwable actualException = expectThrows(RuntimeException.class,
                () -> rule.apply(new FailureStatement(mRuntimeException), mDescription).evaluate());
        assertThat(actualException).isSameAs(mRuntimeException);
        verify(mGoodGuyRunner1).run();
        verify(mGoodGuyExtraExceptions1).call();
        verify(mDumper).dump("Whatever", actualException);
    }

    @Test
    public void testTestPass_oneRunnerFails() throws Throwable {
        final SafeCleanerRule rule = new SafeCleanerRule()
                .run(mGoodGuyRunner1)
                .run(() -> { throw mRuntimeException; })
                .run(mGoodGuyRunner2)
                .add(mGoodGuyExtraExceptions1);
        final Throwable actualException = expectThrows(RuntimeException.class,
                () -> rule.apply(mGoodGuyStatement, mDescription).evaluate());
        assertThat(actualException).isSameAs(mRuntimeException);
        verify(mGoodGuyRunner1).run();
        verify(mGoodGuyRunner2).run();
        verify(mGoodGuyExtraExceptions1).call();
    }

    @Test
    public void testTestPass_oneRunnerFails_withDumper() throws Throwable {
        final SafeCleanerRule rule = new SafeCleanerRule()
                .setDumper(mDumper)
                .run(mGoodGuyRunner1)
                .run(() -> {
                    throw mRuntimeException;
                })
                .run(mGoodGuyRunner2)
                .add(mGoodGuyExtraExceptions1);
        final Throwable actualException = expectThrows(RuntimeException.class,
                () -> rule.apply(mGoodGuyStatement, mDescription).evaluate());
        assertThat(actualException).isSameAs(mRuntimeException);
        verify(mGoodGuyRunner1).run();
        verify(mGoodGuyRunner2).run();
        verify(mGoodGuyExtraExceptions1).call();
        verify(mDumper).dump("Whatever", actualException);
    }

    @Test
    public void testTestPass_oneExtraExceptionThrown() throws Throwable {
        final SafeCleanerRule rule = new SafeCleanerRule()
                .run(mGoodGuyRunner1)
                .add(() -> {
                    return ImmutableList.of(mRuntimeException);
                })
                .add(mGoodGuyExtraExceptions1)
                .run(mGoodGuyRunner2);
        final Throwable actualException = expectThrows(RuntimeException.class,
                () -> rule.apply(mGoodGuyStatement, mDescription).evaluate());
        assertThat(actualException).isSameAs(mRuntimeException);
        verify(mGoodGuyRunner1).run();
        verify(mGoodGuyRunner2).run();
        verify(mGoodGuyExtraExceptions1).call();
    }

    @Test
    public void testTestPass_oneExtraExceptionThrown_withDumper() throws Throwable {
        final SafeCleanerRule rule = new SafeCleanerRule()
                .setDumper(mDumper)
                .run(mGoodGuyRunner1)
                .add(() -> { return ImmutableList.of(mRuntimeException); })
                .add(mGoodGuyExtraExceptions1)
                .run(mGoodGuyRunner2);
        final Throwable actualException = expectThrows(RuntimeException.class,
                () -> rule.apply(mGoodGuyStatement, mDescription).evaluate());
        assertThat(actualException).isSameAs(mRuntimeException);
        verify(mGoodGuyRunner1).run();
        verify(mGoodGuyRunner2).run();
        verify(mGoodGuyExtraExceptions1).call();
        verify(mDumper).dump("Whatever", actualException);
    }

    @Test
    public void testThrowTheKitchenSinkAKAEverybodyThrows() throws Throwable {
        final Exception extra1 = new Exception("1");
        final Exception extra2 = new Exception("2");
        final Exception extra3 = new Exception("3");
        final Error error1 = new Error("one");
        final Error error2 = new Error("two");
        final RuntimeException testException  = new RuntimeException("TEST, Y U NO PASS?");
        final SafeCleanerRule rule = new SafeCleanerRule()
                .run(mGoodGuyRunner1)
                .add(mGoodGuyExtraExceptions1)
                .add(() -> {
                    return ImmutableList.of(extra1, extra2);
                })
                .run(() -> {
                    throw error1;
                })
                .run(mGoodGuyRunner2)
                .add(() -> {
                    return ImmutableList.of(extra3);
                })
                .add(mGoodGuyExtraExceptions2)
                .run(() -> {
                    throw error2;
                });

        final SafeCleanerRule.MultipleExceptions actualException = expectThrows(
                SafeCleanerRule.MultipleExceptions.class,
                () -> rule.apply(new FailureStatement(testException), mDescription).evaluate());
        assertThat(actualException.getThrowables())
                .containsExactly(testException, error1, error2, extra1, extra2, extra3)
                .inOrder();
        verify(mGoodGuyRunner1).run();
        verify(mGoodGuyRunner2).run();
        verify(mGoodGuyExtraExceptions1).call();
    }

    @Test
    public void testThrowTheKitchenSinkAKAEverybodyThrows_withDumper() throws Throwable {
        final Exception extra1 = new Exception("1");
        final Exception extra2 = new Exception("2");
        final Exception extra3 = new Exception("3");
        final Error error1 = new Error("one");
        final Error error2 = new Error("two");
        final RuntimeException testException  = new RuntimeException("TEST, Y U NO PASS?");
        final SafeCleanerRule rule = new SafeCleanerRule()
                .setDumper(mDumper)
                .run(mGoodGuyRunner1)
                .add(mGoodGuyExtraExceptions1)
                .add(() -> {
                    return ImmutableList.of(extra1, extra2);
                })
                .run(() -> {
                    throw error1;
                })
                .run(mGoodGuyRunner2)
                .add(() -> { return ImmutableList.of(extra3); })
                .add(mGoodGuyExtraExceptions2)
                .run(() -> { throw error2; });

        final SafeCleanerRule.MultipleExceptions actualException = expectThrows(
                SafeCleanerRule.MultipleExceptions.class,
                () -> rule.apply(new FailureStatement(testException), mDescription).evaluate());
        assertThat(actualException.getThrowables())
                .containsExactly(testException, error1, error2, extra1, extra2, extra3)
                .inOrder();
        verify(mGoodGuyRunner1).run();
        verify(mGoodGuyRunner2).run();
        verify(mGoodGuyExtraExceptions1).call();
        verify(mDumper).dump("Whatever", actualException);
    }
}
