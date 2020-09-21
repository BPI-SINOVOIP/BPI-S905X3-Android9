/*
 * Copyright (C) 2011 The Android Open Source Project
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
package com.android.tradefed.result;

import junit.framework.TestCase;

/**
 * Unit tests for {@link FailureEmailResultReporter}.
 */
public class FailureEmailResultReporterTest extends TestCase {
    private class FakeEmailResultReporter extends FailureEmailResultReporter {
        private boolean mHasFailedTests;
        private InvocationStatus mInvocationStatus;

        FakeEmailResultReporter(boolean hasFailedTests, InvocationStatus invocationStatus) {
            mHasFailedTests = hasFailedTests;
            mInvocationStatus = invocationStatus;
        }

        /**
         * {@inheritDoc}
         */
        @Override
        public boolean hasFailedTests() {
            return mHasFailedTests;
        }

        /**
         * {@inheritDoc}
         */
        @Override
        public InvocationStatus getInvocationStatus() {
            return mInvocationStatus;
        }
    }

    /**
     * Test that {@link FailureEmailResultReporter#shouldSendMessage()} returns true if
     * {@link EmailResultReporter#getInvocationStatus()} is not {@link InvocationStatus#SUCCESS}.
     */
    public void testShouldSendMessage() {
        // Don't send email if there is no test failure and no invocation failure.
        FakeEmailResultReporter r = new FakeEmailResultReporter(false, InvocationStatus.SUCCESS);
        assertFalse(r.shouldSendMessage());

        // Send email if there is an invocation failure.
        r = new FakeEmailResultReporter(false, InvocationStatus.BUILD_ERROR);
        assertTrue(r.shouldSendMessage());

        // Send email if there is an invocation failure.
        r = new FakeEmailResultReporter(false, InvocationStatus.FAILED);
        assertTrue(r.shouldSendMessage());

        // Send email if there is an test failure.
        r = new FakeEmailResultReporter(true, InvocationStatus.SUCCESS);
        assertTrue(r.shouldSendMessage());

        // Send email if there is an test failure and an invocation failure.
        r = new FakeEmailResultReporter(true, InvocationStatus.FAILED);
        assertTrue(r.shouldSendMessage());
    }
}