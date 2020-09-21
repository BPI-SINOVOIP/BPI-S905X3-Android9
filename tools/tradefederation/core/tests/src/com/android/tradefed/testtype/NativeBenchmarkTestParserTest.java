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

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Unit tests for {@link NativeStressTestParser}. */
@RunWith(JUnit4.class)
public class NativeBenchmarkTestParserTest {

    private static final String RUN_NAME = "run-name";
    private NativeBenchmarkTestParser mParser;

    @Before
    public void setUp() throws Exception {
        mParser = new NativeBenchmarkTestParser(RUN_NAME);
    }

    /** Test a run with all integer values */
    @Test
    public void testParse_allInts() {
        mParser.processNewLines(new String[] {"Time per iteration min: 5 avg: 55 max: 35"});
        mParser.done();
        verifyAvgTime(55);
    }

    /**
     * Test a run with all float values.
     *
     * <p>Expect 1 iterations to be reported
     */
    @Test
    public void testParse_allFloats() {
        mParser.processNewLines(new String[] {
                "Time per iteration min: 0.5 avg: 0.000272995 max: 0.000435"});
        mParser.done();
        verifyAvgTime(0.000272995);
    }

    /**
     * Test a run with scientific values.
     *
     * <p>Expect 1 iterations to be reported
     */
    @Test
    public void testParse_scientific() {
        mParser.processNewLines(new String[] {
                "Time per iteration min: 5.9e-05 avg: 5.9e-05 max: 0.000435"});
        mParser.done();
        verifyAvgTime(0.000059);
        System.out.printf("%12f\n", 0.00000059);
        System.out.printf("%s\n", Double.valueOf(0.00000059).toString());
    }

    /** Verify the iteration count collected by the parser */
    private void verifyAvgTime(double averageTime) {
        Assert.assertEquals(averageTime, mParser.getAvgOperationTime(), 0.0);
    }
}
