/*
 * Copyright (C) 2012 The Android Open Source Project
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
 * Unit tests for {@link InvocationFailureEmailResultReporter}.
 */
public class InvocationFailureEmailResultReporterTest extends TestCase {
    private class FakeEmailResultReporter extends InvocationFailureEmailResultReporter {
        private InvocationStatus mInvocationStatus;

        FakeEmailResultReporter(InvocationStatus invocationStatus) {
            mInvocationStatus = invocationStatus;
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
     * Test that {@link InvocationFailureEmailResultReporter#shouldSendMessage()} returns true if
     * {@link EmailResultReporter#getInvocationStatus()} is not {@link InvocationStatus#SUCCESS}.
     */
    public void testShouldSendMessage() {
        // Don't send email if there is no invocation failure.
        FakeEmailResultReporter r = new FakeEmailResultReporter(InvocationStatus.SUCCESS);
        assertFalse(r.shouldSendMessage());

        // Send email if there is an invocation failure.
        r = new FakeEmailResultReporter(InvocationStatus.BUILD_ERROR);
        assertTrue(r.shouldSendMessage());

        // Send email if there is an invocation failure.
        r = new FakeEmailResultReporter(InvocationStatus.FAILED);
        assertTrue(r.shouldSendMessage());
    }
}