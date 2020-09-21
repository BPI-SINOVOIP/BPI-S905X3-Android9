/*
 * Copyright (C) 2016 The Android Open Source Project
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
 * limitations under the License
 */

package com.android.cts.verifier.location.base;

import junit.framework.AssertionFailedError;
import junit.framework.Protectable;
import junit.framework.Test;
import junit.framework.TestCase;
import junit.framework.TestFailure;
import junit.framework.TestListener;
import junit.framework.TestResult;

import android.content.Context;
import android.location.cts.GnssTestCase;

import java.util.Enumeration;

/**
 * A wrapper class for a {@link TestResult}.
 *
 * It provides a way to inject augmented data and helper objects during the execution of tests.
 * i.e. inject a Context object for use by tests.
 */
public class GnssCtsTestResult extends TestResult {
    private final Context mContext;
    private final TestResult mWrappedTestResult;

    private volatile boolean mInterrupted;

    public GnssCtsTestResult(Context context, TestResult testResult) {
        mContext = context;
        mWrappedTestResult = testResult;
    }

    @Override
    public void addError(Test test, Throwable throwable) {
        mWrappedTestResult.addError(test, throwable);
    }

    @Override
    public void addFailure(Test test, AssertionFailedError assertionFailedError) {
        mWrappedTestResult.addFailure(test, assertionFailedError);
    }

    @Override
    public void addListener(TestListener testListener) {
        mWrappedTestResult.addListener(testListener);
    }

    @Override
    public void removeListener(TestListener testListener) {
        mWrappedTestResult.removeListener(testListener);
    }

    @Override
    public void endTest(Test test) {
        mWrappedTestResult.endTest(test);
    }

    @Override
    public int errorCount() {
        return mWrappedTestResult.errorCount();
    }

    @Override
    public Enumeration<TestFailure> errors() {
        return mWrappedTestResult.errors();
    }

    @Override
    public int failureCount() {
        return mWrappedTestResult.failureCount();
    }

    @Override
    public Enumeration<TestFailure> failures() {
        return mWrappedTestResult.failures();
    }

    @Override
    public int runCount() {
        return mWrappedTestResult.runCount();
    }

    @Override
    public void runProtected(Test test, Protectable protectable) {
        try {
            protectable.protect();
        } catch (AssertionFailedError e) {
            addFailure(test, e);
        } catch (ThreadDeath e) {
            throw e;
        } catch (InterruptedException e) {
            mInterrupted = true;
            addError(test, e);
        } catch (Throwable e) {
            addError(test, e);
        }
    }

    @Override
    public boolean shouldStop() {
        return mInterrupted || mWrappedTestResult.shouldStop();
    }

    @Override
    public void startTest(Test test) {
        mWrappedTestResult.startTest(test);
    }

    @Override
    public void stop() {
        mWrappedTestResult.stop();
    }

    @Override
    public boolean wasSuccessful() {
        return mWrappedTestResult.wasSuccessful();
    }

    @Override
    protected void run(TestCase testCase) {
        if (testCase instanceof GnssTestCase) {
            GnssTestCase gnssTestCase = (GnssTestCase) testCase;
            gnssTestCase.setContext(mContext);
            gnssTestCase.setTestAsCtsVerifierTest(true);
        } else {
            throw new IllegalStateException("TestCase invalid.");
        }
        super.run(testCase);
    }
}