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

package vogar.target.junit;

import java.util.concurrent.atomic.AtomicReference;
import org.junit.runner.JUnitCore;
import org.junit.runner.Runner;
import org.junit.runner.manipulation.NoTestsRemainException;
import org.junit.runner.manipulation.Sorter;
import org.junit.runners.model.RunnerBuilder;
import vogar.monitor.TargetMonitor;
import vogar.target.SkipPastFilter;
import vogar.target.TargetMonitorRunListener;
import vogar.target.TargetRunner;
import vogar.target.TestEnvironment;
import vogar.target.TestEnvironmentRunListener;

/**
 * Adapts a JUnit3 test for use by vogar.
 */
public final class JUnitTargetRunner implements TargetRunner {

    private static final Sorter DESCRIPTION_SORTER =
            new Sorter(DescriptionComparator.getInstance());

    private final TargetMonitor monitor;
    private final AtomicReference<String> skipPastReference;

    private final TestEnvironment testEnvironment;
    private final Class<?> testClass;
    private final RunnerParams runnerParams;

    public JUnitTargetRunner(TargetMonitor monitor, AtomicReference<String> skipPastReference,
                             TestEnvironment testEnvironment,
                             int timeoutSeconds, Class<?> testClass,
                             String qualification, String[] args) {
        this.monitor = monitor;
        this.skipPastReference = skipPastReference;
        this.testEnvironment = testEnvironment;
        this.testClass = testClass;

        TimeoutAndAbortRunRule timeoutRule = new TimeoutAndAbortRunRule(timeoutSeconds);
        runnerParams = new RunnerParams(qualification, args, timeoutRule);
    }

    public boolean run() {
        // Use JUnit infrastructure to run the tests.
        RunnerBuilder builder = new VogarRunnerBuilder(runnerParams);
        Runner runner = builder.safeRunnerForClass(testClass);
        if (runner == null) {
            throw new IllegalStateException("Cannot create runner for: " + testClass.getName());
        }

        // Sort to ensure consistent ordering, that is done before applying any filters as the
        // SkipPastFilter requires a consistent ordering across runs in order to work properly.
        DESCRIPTION_SORTER.apply(runner);

        String skipPast = skipPastReference.get();
        if (skipPast != null) {
            try {
                new SkipPastFilter(skipPastReference).apply(runner);
            } catch (NoTestsRemainException ignored) {
                return true;
            }
        }

        try {
            JUnitCore core = new JUnitCore();
            // The TestEnvironmentRunListener resets static state between tests.
            core.addListener(new TestEnvironmentRunListener(testEnvironment));
            // The TargetMonitorRunListener sends the result of the tests back to the main Vogar
            // process.
            core.addListener(new TargetMonitorRunListener(monitor));
            core.run(runner);
        } catch (VmIsUnstableException e) {
            // If a test reports that the VM is unstable then inform the caller so that the
            // current process can be exited abnormally which will trigger the vogar main process
            // to rerun the tests from after the timing out test.
            return false;
        }

        return true;
    }
}
