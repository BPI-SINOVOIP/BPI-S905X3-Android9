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

package com.android.continuous;

import com.android.ddmlib.testrunner.TestResult.TestStatus;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.result.TestFailureEmailResultReporter;
import com.android.tradefed.result.TestResult;
import com.android.tradefed.result.TestRunResult;
import com.android.tradefed.util.Email;
import com.android.tradefed.util.IEmail;

import java.util.Map;

/**
 * A customized failure reporter that sends emails in a specific format
 */
@OptionClass(alias = "smoke-failure-email")
public class SmokeTestFailureReporter extends TestFailureEmailResultReporter {
    /**
     * Default constructor
     */
    public SmokeTestFailureReporter() {
        this(new Email());
    }

    /**
     * Create a {@link SmokeTestFailureReporter} with a custom {@link IEmail} instance to use.
     *
     * <p>Exposed for unit testing.
     *
     * @param mailer the {@link IEmail} instance to use.
     */
    protected SmokeTestFailureReporter(IEmail mailer) {
        super(mailer);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    protected String generateEmailSubject() {
        final IInvocationContext context = getInvocationContext();
        StringBuilder sb = new StringBuilder();
        sb.append(context.getTestTag());
        sb.append(" SmokeFAST failed on: ");
        for (IBuildInfo build : context.getBuildInfos()) {
            sb.append(build.toString());
        }
        return sb.toString();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    protected String generateEmailBody() {
        StringBuilder sb = new StringBuilder();

        for (TestRunResult run : getRunResults()) {
            if (run.hasFailedTests()) {
                final Map<TestDescription, TestResult> tests = run.getTestResults();
                for (Map.Entry<TestDescription, TestResult> test : tests.entrySet()) {
                    final TestDescription id = test.getKey();
                    final TestResult result = test.getValue();

                    if (result.getStatus() == TestStatus.PASSED) continue;

                    sb.append(String.format("%s#%s %s\n",
                            id.getClassName(), id.getTestName(),
                            describeStatus(result.getStatus())));

                    final String trace = result.getStackTrace();
                    if (trace != null) {
                        sb.append("Stack trace:\n");
                        sb.append(trace);
                        sb.append("\n");
                    }
                }
            }
        }

        sb.append("\n");
        sb.append(super.generateEmailBody());
        return sb.toString();
    }

    private String describeStatus(TestStatus status) {
        switch (status) {
            case FAILURE:
                return "failed";
            case PASSED:
                return "passed";
            case INCOMPLETE:
                return "did not complete";
            case ASSUMPTION_FAILURE:
                return "assumption failed";
            case IGNORED:
                return "ignored";
        }
        return "had an unknown result";
    }
}
