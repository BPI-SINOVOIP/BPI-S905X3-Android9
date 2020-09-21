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
package com.android.tradefed.testtype;

import junit.framework.TestCase;

/**
 * Unit tests for {@link NativeStressTestParser}.
 */
public class NativeStressTestParserTest extends TestCase {

    private static final String RUN_NAME = "run-name";
    private NativeStressTestParser mParser;

    /**
     * {@inheritDoc}
     */
    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mParser = new NativeStressTestParser(RUN_NAME);
    }

    /**
     * Test a run with no content.
     * <p/>
     * Expect 0 iterations to be reported
     */
    public void testParse_empty() {
        mParser.processNewLines(new String[] {});
        mParser.done();
        verifyExpectedIterations(0);
    }

    /**
     * Test a run with garbage content.
     * <p/>
     * Expect 0 iterations to be reported
     */
    public void testParse_garbage() {
        mParser.processNewLines(new String[] {"blah blah", "more blah"});
        mParser.done();
        verifyExpectedIterations(0);
    }

    /**
     * Test a run with valid one iteration.
     * <p/>
     * Expect 1 iterations to be reported
     */
    public void testParse_oneIteration() {
        mParser.processNewLines(new String[] {"==== Completed pass: 0"});
        mParser.done();
        verifyExpectedIterations(1);
    }

    /**
     * Test that iteration data with varying whitespace is parsed successfully.
     * <p/>
     * Expect 2 iterations to be reported
     */
    public void testParse_whitespace() {
        mParser.processNewLines(new String[] {"====Completedpass:0succeeded",
                "====  Completed  pass:   1  "});
        mParser.done();
        verifyExpectedIterations(2);
    }

    /**
     * Test that an invalid iteration value is ignored
     */
    public void testParse_invalidIterationValue() {
        mParser.processNewLines(new String[] {"==== Completed pass: 0", "==== Completed pass: 1",
                "==== Completed pass: b", "Successfully completed 3 passes"});
        mParser.done();
        verifyExpectedIterations(2);
    }

    /**
     * Test that failed iterations are ignored
     */
    public void testParse_failedIteration() {
        mParser.processNewLines(new String[] {"==== Completed pass: 0", "==== pass: 1 failed"});
        mParser.done();
        verifyExpectedIterations(1);
    }

    /**
     * Verify the iteration count collected by the parser
     */
    private void verifyExpectedIterations(int iterations) {
        assertEquals(iterations, mParser.getIterationsCompleted());
    }
}
