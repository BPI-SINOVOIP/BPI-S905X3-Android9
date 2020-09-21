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

import android.platform.test.annotations.AppModeFull;
import android.support.test.runner.AndroidJUnit4;

import org.junit.Test;
import org.junit.runner.Description;
import org.junit.runner.RunWith;
import org.junit.runners.model.Statement;

@RunWith(AndroidJUnit4.class)
@AppModeFull // Unit test
public class RetryRuleTest {

    private final Description mDescription = Description.createSuiteDescription("Whatever");

    private static final RetryableException sRetryableException =
            new RetryableException("Y U NO RETRY?");

    private static class RetryableStatement<T extends Exception> extends Statement {
        private final int mNumberFailures;
        private int mNumberCalls;
        private final T mException;

        RetryableStatement(int numberFailures, T exception) {
            mNumberFailures = numberFailures;
            mException = exception;
        }

        @Override
        public void evaluate() throws Throwable {
            mNumberCalls ++;
            if (mNumberCalls <= mNumberFailures) {
                throw mException;
            }
        }

        @Override
        public String toString() {
            return "RetryableStatement: failures=" + mNumberFailures + ", calls=" + mNumberCalls;
        }
    }

    @Test
    public void testPassOnRetryableException() throws Throwable {
        final RetryRule rule = new RetryRule(2);
        rule.apply(new RetryableStatement<RetryableException>(1, sRetryableException), mDescription)
                .evaluate();
    }

    @Test
    public void testPassOnRetryableExceptionWithTimeout() throws Throwable {
        final Timeout timeout = new Timeout("YOUR TIME IS GONE", 1, 2, 10);
        final RetryableException exception = new RetryableException(timeout, "Y U NO?");
        final RetryRule rule = new RetryRule(2);
        rule.apply(new RetryableStatement<RetryableException>(1, exception), mDescription)
                .evaluate();
        // Assert timeout was increased
        assertThat(timeout.ms()).isEqualTo(2);
    }

    @Test
    public void testFailOnRetryableException() throws Throwable {
        final RetryRule rule = new RetryRule(2);
        try {
            rule.apply(new RetryableStatement<RetryableException>(2, sRetryableException),
                    mDescription).evaluate();
            throw new AssertionError("2ND CALL, Y U NO FAIL?");
        } catch (RetryableException e) {
            assertThat(e).isSameAs(sRetryableException);
        }
    }
}
