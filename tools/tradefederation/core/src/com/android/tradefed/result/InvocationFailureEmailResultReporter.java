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

import com.android.tradefed.config.OptionClass;
import com.android.tradefed.util.Email;
import com.android.tradefed.util.IEmail;

/**
 * An {@link EmailResultReporter} that can also restrict notifications to just invocation failures.
 */
@OptionClass(alias = "invocation-failure-email")
public class InvocationFailureEmailResultReporter extends EmailResultReporter {

    private boolean mDisabled = false;

    /**
     * Default constructor
     */
    public InvocationFailureEmailResultReporter() {
        this(new Email());
    }

    /**
     * Create a {@link InvocationFailureEmailResultReporter} with a custom {@link IEmail} instance
     * to use.
     * <p/>
     * Exposed for unit testing.
     *
     * @param mailer the {@link IEmail} instance to use.
     */
    protected InvocationFailureEmailResultReporter(IEmail mailer) {
        super(mailer);
    }

    /**
     * Send a message if there was an invocation failure.
     *
     * @return {@code true} if the InvocationStatus is not {@link InvocationStatus#SUCCESS}, or
     * {@code false} if it is.
     */
    @Override
    protected boolean shouldSendMessage() {
        if (mDisabled) {
            // Inhibit the failure from sending emails if we are shutting down TF.
            return false;
        }
        return !getInvocationStatus().equals(InvocationStatus.SUCCESS);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void invocationInterrupted() {
        mDisabled = true;
    }
}
