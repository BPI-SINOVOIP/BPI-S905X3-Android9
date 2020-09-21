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

import com.android.ddmlib.Log.LogLevel;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;

import junit.textui.ResultPrinter;

import java.util.HashMap;

/**
 * A test result reporter that forwards results to the JUnit text result printer.
 */
@OptionClass(alias = "stdout")
public class TextResultReporter extends InvocationToJUnitResultForwarder
        implements ITestInvocationListener, ILogSaverListener {

    /**
     * Creates a {@link TextResultReporter}.
     */
    public TextResultReporter() {
        super(new ResultPrinter(System.out));
    }

    /**
     * Overrides parent to explicitly print out failures. The ResultPrinter relies on the runner
     * calling "print" at end of test run to do this. {@inheritDoc}
     */
    @Override
    public void testFailed(TestDescription testId, String trace) {
        ResultPrinter printer = (ResultPrinter)getJUnitListener();
        printer.getWriter().format("\nTest %s: failed \n stack: %s ", testId, trace);
    }

    @Override
    public void testAssumptionFailure(TestDescription testId, String trace) {
        ResultPrinter printer = (ResultPrinter)getJUnitListener();
        printer.getWriter().format("\nTest %s: assumption failed \n stack: %s ", testId, trace);
    }

    /** Overrides parent to explicitly print out test metrics. */
    @Override
    public void testEnded(TestDescription testId, HashMap<String, Metric> metrics) {
        super.testEnded(testId, metrics);
        if (!metrics.isEmpty()) {
            ResultPrinter printer = (ResultPrinter)getJUnitListener();
            printer.getWriter().format("\n%s metrics: %s\n", testId, metrics);
        }
    }

    /** Overrides parent to explicitly print out metrics. */
    @Override
    public void testRunEnded(long elapsedTime, HashMap<String, Metric> metrics) {
        super.testRunEnded(elapsedTime, metrics);
        if (!metrics.isEmpty()) {
            ResultPrinter printer = (ResultPrinter) getJUnitListener();
            printer.getWriter().format("\nMetrics: %s\n", metrics);
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void testLog(String dataName, LogDataType dataType, InputStreamSource dataStream) {
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void testLogSaved(String dataName, LogDataType dataType, InputStreamSource dataStream,
            LogFile logFile) {
        CLog.logAndDisplay(LogLevel.INFO, "Saved %s log to %s", dataName, logFile.getPath());
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setLogSaver(ILogSaver logSaver) {
        // Ignore
    }
}
