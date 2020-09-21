/*
 * Copyright (C) 2015 The Android Open Source Project
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

package com.android.compatibility.common.tradefed.result;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.util.AbiUtils;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.util.HashMap;

/**
 * Unit tests for {@link ConsoleReporter}.
 */
@RunWith(JUnit4.class)
public class ConsoleReporterTest {

    private static final String NAME = "ModuleName";
    private static final String NAME2 = "ModuleName2";
    private static final String ABI = "mips64";
    private static final String ID = AbiUtils.createId(ABI, NAME);
    private static final String ID2 = AbiUtils.createId(ABI, NAME2);
    private static final String CLASS = "android.test.FoorBar";
    private static final String METHOD_1 = "testBlah1";
    private static final String METHOD_2 = "testBlah2";
    private static final String METHOD_3 = "testBlah3";
    private static final String STACK_TRACE = "Something small is not alright\n " +
            "at four.big.insects.Marley.sing(Marley.java:10)";

    private ConsoleReporter mReporter;
    private IInvocationContext mContext;

    @Before
    public void setUp() throws Exception {
        mReporter = new ConsoleReporter();
        OptionSetter setter = new OptionSetter(mReporter);
        setter.setOptionValue("quiet-output", "true");
    }

    @After
    public void tearDown() throws Exception {
        mReporter = null;
    }

    @Test
    public void testResultReporting_singleModule() throws Exception {
        mReporter.invocationStarted(mContext);
        mReporter.testRunStarted(ID, 4);
        runTests();

        mReporter.testRunEnded(10, new HashMap<String, String>());
        mReporter.invocationEnded(10);

        assertEquals(ID, mReporter.getModuleId());
        assertEquals(2, mReporter.getFailedTests());
        assertEquals(1, mReporter.getPassedTests());
        assertEquals(4, mReporter.getCurrentTestNum());
        assertEquals(4, mReporter.getTotalTestsInModule());
    }

    @Test
    public void testResultReporting_multipleModules() throws Exception {
        mReporter.invocationStarted(mContext);
        mReporter.testRunStarted(ID, 4);
        runTests();

        assertEquals(ID, mReporter.getModuleId());
        assertEquals(2, mReporter.getFailedTests());
        assertEquals(1, mReporter.getPassedTests());
        assertEquals(4, mReporter.getCurrentTestNum());
        assertEquals(4, mReporter.getTotalTestsInModule());

        // Should reset counters
        mReporter.testRunStarted(ID2, 4);
        assertEquals(ID2, mReporter.getModuleId());
        assertEquals(0, mReporter.getFailedTests());
        assertEquals(0, mReporter.getPassedTests());
        assertEquals(0, mReporter.getCurrentTestNum());
        assertEquals(4, mReporter.getTotalTestsInModule());
    }

    /** Run 4 test, but one is ignored */
    private void runTests() {
        TestDescription test1 = new TestDescription(CLASS, METHOD_1);
        mReporter.testStarted(test1);
        mReporter.testEnded(test1, new HashMap<String, Metric>());
        assertFalse(mReporter.getTestFailed());

        TestDescription test2 = new TestDescription(CLASS, METHOD_2);
        mReporter.testStarted(test2);
        assertFalse(mReporter.getTestFailed());
        mReporter.testFailed(test2, STACK_TRACE);
        assertTrue(mReporter.getTestFailed());

        TestDescription test3 = new TestDescription(CLASS, METHOD_3);
        mReporter.testStarted(test3);
        assertFalse(mReporter.getTestFailed());
        mReporter.testFailed(test3, STACK_TRACE);
        assertTrue(mReporter.getTestFailed());

        TestDescription test4 = new TestDescription(CLASS, METHOD_3);
        mReporter.testStarted(test4);
        assertFalse(mReporter.getTestFailed());
        mReporter.testIgnored(test4);
        assertFalse(mReporter.getTestFailed());
    }
}
