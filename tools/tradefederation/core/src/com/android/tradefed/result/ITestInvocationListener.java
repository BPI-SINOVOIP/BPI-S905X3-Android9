/*
 * Copyright (C) 2010 The Android Open Source Project
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

import com.android.tradefed.command.ICommandScheduler;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.log.ITestLogger;
import com.android.tradefed.testtype.suite.ITestSuite;

/**
 * Listener for test results from the test invocation.
 *
 * <p>A test invocation can itself include multiple test runs, so the sequence of calls will be
 *
 * <ul>
 *   <li>invocationStarted(BuildInfo)
 *   <li>testRunStarted
 *   <li>testStarted
 *   <li>[testFailed]
 *   <li>testEnded
 *   <li>...
 *   <li>testRunEnded
 *   <li>...
 *   <li>testRunStarted
 *   <li>...
 *   <li>testRunEnded
 *   <li>[invocationFailed]
 *   <li>[testLog+]
 *   <li>invocationEnded
 *   <li>getSummary
 * </ul>
 */
public interface ITestInvocationListener extends ITestLogger, ITestLifeCycleReceiver {

    /**
     * Reports the start of the test invocation.
     *
     * <p>Will be automatically called by the TradeFederation framework. Reporters need to override
     * this method to support multiple devices reporting.
     *
     * @param context information about the invocation
     */
    public default void invocationStarted(IInvocationContext context) {}

    /**
     * Reports that the invocation has terminated, whether successfully or due to some error
     * condition.
     * <p/>
     * Will be automatically called by the TradeFederation framework.
     *
     * @param elapsedTime the elapsed time of the invocation in ms
     */
    default public void invocationEnded(long elapsedTime) { }

    /**
     * Reports an incomplete invocation due to some error condition.
     * <p/>
     * Will be automatically called by the TradeFederation framework.
     *
     * @param cause the {@link Throwable} cause of the failure
     */
    default public void invocationFailed(Throwable cause) { }

    /**
     * Allows the InvocationListener to return a summary.
     *
     * @return A {@link TestSummary} summarizing the run, or null
     */
    default public TestSummary getSummary() { return null; }

    /**
     * Called on {@link ICommandScheduler#shutdown()}, gives the invocation the opportunity to do
     * something before terminating.
     */
    default public void invocationInterrupted() {
        // do nothing in default implementation.
    }

    /**
     * Reports the beginning of a module running. This callback is associated with {@link
     * #testModuleEnded()} and is optional in the sequence. It is only used during a run that uses
     * modules: {@link ITestSuite} based runners.
     *
     * @param moduleContext the {@link IInvocationContext} of the module.
     */
    public default void testModuleStarted(IInvocationContext moduleContext) {}

    /** Reports the end of a module run. */
    public default void testModuleEnded() {}
}
