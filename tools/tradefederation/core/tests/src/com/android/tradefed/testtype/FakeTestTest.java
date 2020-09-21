/*
 * Copyright (C) 2013 The Android Open Source Project
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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.fail;

import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.TestDescription;

import org.easymock.EasyMock;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.util.HashMap;
import java.util.Map;

@RunWith(JUnit4.class)
public class FakeTestTest {

    private FakeTest mTest = null;
    private ITestInvocationListener mListener = null;
    private OptionSetter mOption = null;

    @Before
    public void setUp() throws Exception {
        mTest = new FakeTest();
        mOption = new OptionSetter(mTest);
        mListener = EasyMock.createStrictMock(ITestInvocationListener.class);
    }

    @Test
    public void testIntOrDefault() throws IllegalArgumentException {
        assertEquals(55, mTest.toIntOrDefault(null, 55));
        assertEquals(45, mTest.toIntOrDefault("45", 55));
        assertEquals(-13, mTest.toIntOrDefault("-13", 55));
        try {
            mTest.toIntOrDefault("thirteen", 55);
            fail("No IllegalArgumentException thrown for input \"thirteen\"");
        } catch (IllegalArgumentException e) {
            // expected
        }
    }

    @Test
    public void testDecodeRle() throws IllegalArgumentException {
        // testcases: input -> output
        final Map<String, String> t = new HashMap<String, String>();
        t.put("PFE", "PFE");
        t.put("P1F1E1", "PFE");
        t.put("P2F2E2", "PPFFEE");
        t.put("P3F2E1", "PPPFFE");
        t.put("", "");
        for (Map.Entry<String, String> testcase : t.entrySet()) {
            final String input = testcase.getKey();
            final String output = testcase.getValue();
            assertEquals(output, mTest.decodeRle(input));
        }

        final String[] errors = {"X", "X1", "P0", "P2F1E0", "2PF", "2PF1", " "};
        for (String error : errors) {
            try {
                mTest.decodeRle(error);
                fail(String.format("No IllegalArgumentException thrown for input \"%s\"", error));
            } catch (IllegalArgumentException e) {
                // expected
            }
        }
    }

    @Test
    public void testDecode() throws IllegalArgumentException {
        // testcases: input -> output
        final Map<String, String> t = new HashMap<String, String>();
        // Valid input for decodeRle should be valid for decode
        t.put("PFE", "PFE");
        t.put("P1F1E1", "PFE");
        t.put("P2F2E2", "PPFFEE");
        t.put("P3F2E1", "PPPFFE");
        t.put("", "");

        // decode should also handle parens and lowercase input
        t.put("(PFE)", "PFE");
        t.put("pfe", "PFE");
        t.put("(PFE)2", "PFEPFE");
        t.put("((PF)2)2E3", "PFPFPFPFEEE");
        t.put("()PFE", "PFE");
        t.put("PFE()", "PFE");
        t.put("(PF)(FE)", "PFFE");
        t.put("(PF()FE)", "PFFE");

        for (Map.Entry<String, String> testcase : t.entrySet()) {
            final String input = testcase.getKey();
            final String output = testcase.getValue();
            try {
                assertEquals(output, mTest.decode(input));
            } catch (IllegalArgumentException e) {
                // Add input to error message to make debugging easier
                final Exception cause = new IllegalArgumentException(String.format(
                        "Encountered exception for input \"%s\" with expected output \"%s\"",
                        input, output));
                throw new IllegalArgumentException(e.getMessage(), cause);
            }
        }

        final String[] errors = {
                "X", "X1", "P0", "P2F1E0", "2PF", "2PF1", " ",  // common decodeRle errors
                "(", "(PFE()", "(PFE))", "2(PFE)", "2(PFE)2", "(PF))((FE)", "(PFE)0"};
        for (String error : errors) {
            try {
                mTest.decode(error);
                fail(String.format("No IllegalArgumentException thrown for input \"%s\"", error));
            } catch (IllegalArgumentException e) {
                // expected
            }
        }
    }

    @Test
    public void testRun_empty() throws Exception {
        final String name = "com.moo.cow";
        mListener.testRunStarted(EasyMock.eq(name), EasyMock.eq(0));
        mListener.testRunEnded(EasyMock.eq(0l), EasyMock.<HashMap<String, Metric>>anyObject());

        EasyMock.replay(mListener);
        mOption.setOptionValue("run", name, "");
        mTest.run(mListener);
        EasyMock.verify(mListener);
    }

    @Test
    public void testRun_simplePass() throws Exception {
        final String name = "com.moo.cow";
        mListener.testRunStarted(EasyMock.eq(name), EasyMock.eq(1));
        testPassExpectations(mListener, name, 1);
        mListener.testRunEnded(EasyMock.eq(0l), EasyMock.<HashMap<String, Metric>>anyObject());

        EasyMock.replay(mListener);
        mOption.setOptionValue("run", name, "P");
        mTest.run(mListener);
        EasyMock.verify(mListener);
    }

    @Test
    public void testRun_simpleFail() throws Exception {
        final String name = "com.moo.cow";
        mListener.testRunStarted(EasyMock.eq(name), EasyMock.eq(1));
        testFailExpectations(mListener, name, 1);
        mListener.testRunEnded(EasyMock.eq(0l), EasyMock.<HashMap<String, Metric>>anyObject());

        EasyMock.replay(mListener);
        mOption.setOptionValue("run", name, "F");
        mTest.run(mListener);
        EasyMock.verify(mListener);
    }

    @Test
    public void testRun_basicSequence() throws Exception {
        final String name = "com.moo.cow";
        int i = 1;
        mListener.testRunStarted(EasyMock.eq(name), EasyMock.eq(3));
        testPassExpectations(mListener, name, i++);
        testFailExpectations(mListener, name, i++);
        testPassExpectations(mListener, name, i++);
        mListener.testRunEnded(EasyMock.eq(0l), EasyMock.<HashMap<String, Metric>>anyObject());

        EasyMock.replay(mListener);
        mOption.setOptionValue("run", name, "PFP");
        mTest.run(mListener);
        EasyMock.verify(mListener);
    }

    @Test
    public void testRun_basicParens() throws Exception {
        final String name = "com.moo.cow";
        int i = 1;
        mListener.testRunStarted(EasyMock.eq(name), EasyMock.eq(4));
        testPassExpectations(mListener, name, i++);
        testFailExpectations(mListener, name, i++);
        testPassExpectations(mListener, name, i++);
        testFailExpectations(mListener, name, i++);
        mListener.testRunEnded(EasyMock.eq(0l), EasyMock.<HashMap<String, Metric>>anyObject());

        EasyMock.replay(mListener);
        mOption.setOptionValue("run", name, "(PF)2");
        mTest.run(mListener);
        EasyMock.verify(mListener);
    }

    @Test
    public void testRun_recursiveParens() throws Exception {
        final String name = "com.moo.cow";
        int i = 1;
        mListener.testRunStarted(EasyMock.eq(name), EasyMock.eq(8));
        testPassExpectations(mListener, name, i++);
        testFailExpectations(mListener, name, i++);

        testPassExpectations(mListener, name, i++);
        testFailExpectations(mListener, name, i++);

        testPassExpectations(mListener, name, i++);
        testFailExpectations(mListener, name, i++);

        testPassExpectations(mListener, name, i++);
        testFailExpectations(mListener, name, i++);

        mListener.testRunEnded(EasyMock.eq(0l), EasyMock.<HashMap<String, Metric>>anyObject());

        EasyMock.replay(mListener);
        mOption.setOptionValue("run", name, "((PF)2)2");
        mTest.run(mListener);
        EasyMock.verify(mListener);
    }

    @Test
    public void testMultiRun() throws Exception {
        final String name1 = "com.moo.cow";
        int i = 1;
        mListener.testRunStarted(EasyMock.eq(name1), EasyMock.eq(2));
        testPassExpectations(mListener, name1, i++);
        testFailExpectations(mListener, name1, i++);
        mListener.testRunEnded(EasyMock.eq(0l), EasyMock.<HashMap<String, Metric>>anyObject());

        final String name2 = "com.quack.duck";
        i = 1;
        mListener.testRunStarted(EasyMock.eq(name2), EasyMock.eq(2));
        testFailExpectations(mListener, name2, i++);
        testPassExpectations(mListener, name2, i++);
        mListener.testRunEnded(EasyMock.eq(0l), EasyMock.<HashMap<String, Metric>>anyObject());

        final String name3 = "com.oink.pig";
        i = 1;
        mListener.testRunStarted(EasyMock.eq(name3), EasyMock.eq(0));
        // empty run
        mListener.testRunEnded(EasyMock.eq(0l), EasyMock.<HashMap<String, Metric>>anyObject());

        EasyMock.replay(mListener);
        mOption.setOptionValue("run", name1, "PF");
        mOption.setOptionValue("run", name2, "FP");
        mOption.setOptionValue("run", name3, "");
        mTest.run(mListener);
        EasyMock.verify(mListener);
    }

    private void testPassExpectations(ITestInvocationListener l, String klass,
            int idx) {
        final String name = String.format("testMethod%d", idx);
        final TestDescription test = new TestDescription(klass, name);
        l.testStarted(test);
        l.testEnded(EasyMock.eq(test), EasyMock.<HashMap<String, Metric>>anyObject());
    }

    private void testFailExpectations(ITestInvocationListener l, String klass,
            int idx) {
        final String name = String.format("testMethod%d", idx);
        final TestDescription test = new TestDescription(klass, name);
        l.testStarted(test);
        l.testFailed(EasyMock.eq(test), EasyMock.<String>anyObject());
        l.testEnded(EasyMock.eq(test), EasyMock.<HashMap<String, Metric>>anyObject());
    }
}

