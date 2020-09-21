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
 * limitations under the License.
 */
package com.android.tradefed.testtype;

import static org.easymock.EasyMock.anyObject;
import static org.easymock.EasyMock.createMock;
import static org.easymock.EasyMock.eq;
import static org.easymock.EasyMock.expectLastCall;
import static org.easymock.EasyMock.replay;
import static org.easymock.EasyMock.verify;

import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.util.ArrayUtil;

import junit.framework.AssertionFailedError;
import junit.framework.TestCase;

import java.util.Arrays;
import java.util.HashMap;

/** Unit tests for {@link PythonUnitTestResultParser}. */
public class PythonUnitTestResultParserTest extends TestCase {

    private PythonUnitTestResultParser mParser;
    private ITestInvocationListener mMockListener;

    @Override
    public void setUp() throws Exception {
        mMockListener = createMock(ITestInvocationListener.class);
        mParser = new PythonUnitTestResultParser(ArrayUtil.list(mMockListener), "test");
    }

    public void testRegexTestCase() {
        String s = "a (b) ... ok";
        assertTrue(PythonUnitTestResultParser.PATTERN_ONE_LINE_RESULT.matcher(s).matches());
        assertFalse(PythonUnitTestResultParser.PATTERN_TWO_LINE_RESULT_FIRST.matcher(s).matches());
        s = "a (b) ... FAIL";
        assertTrue(PythonUnitTestResultParser.PATTERN_ONE_LINE_RESULT.matcher(s).matches());
        s = "a (b) ... ERROR";
        assertTrue(PythonUnitTestResultParser.PATTERN_ONE_LINE_RESULT.matcher(s).matches());
        s = "a (b) ... expected failure";
        assertTrue(PythonUnitTestResultParser.PATTERN_ONE_LINE_RESULT.matcher(s).matches());
        s = "a (b) ... skipped 'reason foo'";
        assertTrue(PythonUnitTestResultParser.PATTERN_ONE_LINE_RESULT.matcher(s).matches());
        s = "a (b)";
        assertFalse(PythonUnitTestResultParser.PATTERN_ONE_LINE_RESULT.matcher(s).matches());
        assertTrue(PythonUnitTestResultParser.PATTERN_TWO_LINE_RESULT_FIRST.matcher(s).matches());
        s = "doc string foo bar ... ok";
        assertTrue(PythonUnitTestResultParser.PATTERN_TWO_LINE_RESULT_SECOND.matcher(s).matches());
        s = "docstringfoobar ... ok";
        assertTrue(PythonUnitTestResultParser.PATTERN_TWO_LINE_RESULT_SECOND.matcher(s).matches());
    }

    public void testRegexFailMessage() {
        String s = "FAIL: a (b)";
        assertTrue(PythonUnitTestResultParser.PATTERN_FAIL_MESSAGE.matcher(s).matches());
        s = "ERROR: a (b)";
        assertTrue(PythonUnitTestResultParser.PATTERN_FAIL_MESSAGE.matcher(s).matches());
    }

    public void testRegexRunSummary() {
        String s = "Ran 1 test in 1s";
        assertTrue(PythonUnitTestResultParser.PATTERN_RUN_SUMMARY.matcher(s).matches());
        s = "Ran 42 tests in 1s";
        assertTrue(PythonUnitTestResultParser.PATTERN_RUN_SUMMARY.matcher(s).matches());
        s = "Ran 1 tests in 0.000s";
        assertTrue(PythonUnitTestResultParser.PATTERN_RUN_SUMMARY.matcher(s).matches());
        s = "Ran 1 test in 0.001s";
        assertTrue(PythonUnitTestResultParser.PATTERN_RUN_SUMMARY.matcher(s).matches());
        s = "Ran 1 test in 12345s";
        assertTrue(PythonUnitTestResultParser.PATTERN_RUN_SUMMARY.matcher(s).matches());
    }

    public void testRegexRunResult() {
        String s = "OK";
        assertTrue(PythonUnitTestResultParser.PATTERN_RUN_RESULT.matcher(s).matches());
        s = "OK (expected failures=2) ";
        assertTrue(PythonUnitTestResultParser.PATTERN_RUN_RESULT.matcher(s).matches());
        s = "FAILED (errors=1)";
        assertTrue(PythonUnitTestResultParser.PATTERN_RUN_RESULT.matcher(s).matches());
    }

    public void testParseNoTests() throws Exception {
        String[] output = {
                "",
                PythonUnitTestResultParser.DASH_LINE,
                "Ran 0 tests in 0.000s",
                "",
                "OK"
        };
        setRunListenerChecks(0, 0, true);

        replay(mMockListener);
        mParser.processNewLines(output);
        verify(mMockListener);
    }

    public void testParseSingleTestPass() throws Exception {
        String[] output = {
                "b (a) ... ok",
                "",
                PythonUnitTestResultParser.DASH_LINE,
                "Ran 1 test in 1s",
                "",
                "OK"
        };
        TestDescription[] ids = {new TestDescription("a", "b")};
        boolean[] didPass = {true};
        setTestIdChecks(ids, didPass);
        setRunListenerChecks(1, 1000, true);

        replay(mMockListener);
        mParser.processNewLines(output);
        verify(mMockListener);
    }

    public void testParseSingleTestPassWithExpectedFailure() throws Exception {
        String[] output = {
                "b (a) ... expected failure",
                "",
                PythonUnitTestResultParser.DASH_LINE,
                "Ran 1 test in 1s",
                "",
                "OK (expected failures=1)"
        };
        TestDescription[] ids = {new TestDescription("a", "b")};
        boolean[] didPass = {true};
        setTestIdChecks(ids, didPass);
        setRunListenerChecks(1, 1000, true);

        replay(mMockListener);
        mParser.processNewLines(output);
        verify(mMockListener);
    }

    public void testParseMultiTestPass() throws Exception {
        String[] output = {
                "b (a) ... ok",
                "d (c) ... ok",
                "",
                PythonUnitTestResultParser.DASH_LINE,
                "Ran 2 tests in 1s",
                "",
                "OK"
        };
        TestDescription[] ids = {new TestDescription("a", "b"), new TestDescription("c", "d")};
        boolean didPass[] = {true, true};
        setTestIdChecks(ids, didPass);
        setRunListenerChecks(2, 1000, true);

        replay(mMockListener);
        mParser.processNewLines(output);
        verify(mMockListener);
    }

    public void testParseMultiTestPassWithOneExpectedFailure() throws Exception {
        String[] output = {
                "b (a) ... expected failure",
                "d (c) ... ok",
                "",
                PythonUnitTestResultParser.DASH_LINE,
                "Ran 2 tests in 1s",
                "",
                "OK (expected failures=1)"
        };
        TestDescription[] ids = {new TestDescription("a", "b"), new TestDescription("c", "d")};
        boolean[] didPass = {true, true};
        setTestIdChecks(ids, didPass);
        setRunListenerChecks(2, 1000, true);

        replay(mMockListener);
        mParser.processNewLines(output);
        verify(mMockListener);
    }

    public void testParseMultiTestPassWithAllExpectedFailure() throws Exception {
        String[] output = {
                "b (a) ... expected failure",
                "d (c) ... expected failure",
                "",
                PythonUnitTestResultParser.DASH_LINE,
                "Ran 2 tests in 1s",
                "",
                "OK (expected failures=2)"
        };
        TestDescription[] ids = {new TestDescription("a", "b"), new TestDescription("c", "d")};
        boolean[] didPass = {true, true};
        setTestIdChecks(ids, didPass);
        setRunListenerChecks(2, 1000, true);

        replay(mMockListener);
        mParser.processNewLines(output);
        verify(mMockListener);
    }

    public void testParseSingleTestFail() throws Exception {
        String[] output = {
                "b (a) ... ERROR",
                "",
                PythonUnitTestResultParser.EQUAL_LINE,
                "ERROR: b (a)",
                PythonUnitTestResultParser.DASH_LINE,
                "Traceback (most recent call last):",
                "  File \"test_rangelib.py\", line 129, in test_reallyfail",
                "    raise ValueError()",
                "ValueError",
                "",
                PythonUnitTestResultParser.DASH_LINE,
                "Ran 1 test in 1s",
                "",
                "FAILED (errors=1)"
        };
        TestDescription[] ids = {new TestDescription("a", "b")};
        boolean[] didPass = {false};
        setRunListenerChecks(1, 1000, false);
        setTestIdChecks(ids, didPass);

        replay(mMockListener);
        mParser.processNewLines(output);
        verify(mMockListener);
    }

    public void testParseMultiTestFailWithExpectedFailure() throws Exception {
        String[] output = {
                "b (a) ... expected failure",
                "d (c) ... ERROR",
                "",
                PythonUnitTestResultParser.EQUAL_LINE,
                "ERROR: d (c)",
                PythonUnitTestResultParser.DASH_LINE,
                "Traceback (most recent call last):",
                "  File \"test_rangelib.py\", line 129, in test_reallyfail",
                "    raise ValueError()",
                "ValueError",
                "",
                PythonUnitTestResultParser.DASH_LINE,
                "Ran 1 test in 1s",
                "",
                "FAILED (errors=1)"
        };
        TestDescription[] ids = {new TestDescription("a", "b"), new TestDescription("c", "d")};
        boolean[] didPass = {true, false};
        setRunListenerChecks(1, 1000, false);
        setTestIdChecks(ids, didPass);

        replay(mMockListener);
        mParser.processNewLines(output);
        verify(mMockListener);
    }

    public void testParseSingleTestUnexpectedSuccess() throws Exception {
        String[] output = {
                "b (a) ... unexpected success",
                "",
                PythonUnitTestResultParser.DASH_LINE,
                "Ran 1 test in 1s",
                "",
                "OK (unexpected success=1)",
        };
        TestDescription[] ids = {new TestDescription("a", "b")};
        boolean[] didPass = {false};
        setTestIdChecks(ids, didPass);
        setRunListenerChecks(1, 1000, false);

        replay(mMockListener);
        mParser.processNewLines(output);
        verify(mMockListener);
    }

    public void testParseSingleTestSkipped() throws Exception {
        String[] output = {
                "b (a) ... skipped 'reason foo'",
                "",
                PythonUnitTestResultParser.DASH_LINE,
                "Ran 1 test in 1s",
                "",
                "OK (skipped=1)",
        };
        TestDescription[] ids = {new TestDescription("a", "b")};
        boolean[] didPass = {false};
        boolean[] didSkip = {true};
        setTestIdChecks(ids, didPass, didSkip);
        setRunListenerChecks(1, 1000, true);

        replay(mMockListener);
        mParser.processNewLines(output);
        verify(mMockListener);
    }

    public void testParseSingleTestPassWithDocString() throws Exception {
        String[] output = {
                "b (a)",
                "doc string foo bar ... ok",
                "",
                PythonUnitTestResultParser.DASH_LINE,
                "Ran 1 test in 1s",
                "",
                "OK",
        };
        TestDescription[] ids = {new TestDescription("a", "b")};
        boolean[] didPass = {true};
        setTestIdChecks(ids, didPass);
        setRunListenerChecks(1, 1000, true);

        replay(mMockListener);
        mParser.processNewLines(output);
        verify(mMockListener);
    }

    public void testParseSingleTestFailWithDocString() throws Exception {
        String[] output = {
                "b (a)",
                "doc string foo bar ... ERROR",
                "",
                PythonUnitTestResultParser.EQUAL_LINE,
                "ERROR: b (a)",
                "doc string foo bar",
                PythonUnitTestResultParser.DASH_LINE,
                "Traceback (most recent call last):",
                "  File \"test_rangelib.py\", line 129, in test_reallyfail",
                "    raise ValueError()",
                "ValueError",
                "",
                PythonUnitTestResultParser.DASH_LINE,
                "Ran 1 test in 1s",
                "",
                "FAILED (errors=1)"
        };
        TestDescription[] ids = {new TestDescription("a", "b")};
        boolean[] didPass = {false};
        setTestIdChecks(ids, didPass);
        setRunListenerChecks(1, 1000, false);

        replay(mMockListener);
        mParser.processNewLines(output);
        verify(mMockListener);
    }

    public void testParseOneWithEverything() throws Exception {
        String[] output = {
                "testError (foo.testFoo) ... ERROR",
                "testExpectedFailure (foo.testFoo) ... expected failure",
                "testFail (foo.testFoo) ... FAIL",
                "testFailWithDocString (foo.testFoo)",
                "foo bar ... FAIL",
                "testOk (foo.testFoo) ... ok",
                "testOkWithDocString (foo.testFoo)",
                "foo bar ... ok",
                "testSkipped (foo.testFoo) ... skipped 'reason foo'",
                "testUnexpectedSuccess (foo.testFoo) ... unexpected success",
                "",
                PythonUnitTestResultParser.EQUAL_LINE,
                "ERROR: testError (foo.testFoo)",
                PythonUnitTestResultParser.DASH_LINE,
                "Traceback (most recent call last):",
                "File \"foo.py\", line 11, in testError",
                "self.assertEqual(2+2, 5/0)",
                "ZeroDivisionError: integer division or modulo by zero",
                "",
                PythonUnitTestResultParser.EQUAL_LINE,
                "FAIL: testFail (foo.testFoo)",
                PythonUnitTestResultParser.DASH_LINE,
                "Traceback (most recent call last):",
                "File \"foo.py\", line 8, in testFail",
                "self.assertEqual(2+2, 5)",
                "AssertionError: 4 != 5",
                "",
                PythonUnitTestResultParser.EQUAL_LINE,
                "FAIL: testFailWithDocString (foo.testFoo)",
                "foo bar",
                PythonUnitTestResultParser.DASH_LINE,
                "Traceback (most recent call last):",
                "File \"foo.py\", line 8, in testFail",
                "self.assertEqual(2+2, 5)",
                "AssertionError: 4 != 5",
                "",
                PythonUnitTestResultParser.DASH_LINE,
                "Ran 8 tests in 1s",
                "",
                "FAILED (failures=2, errors=1, skipped=1, expected failures=1, unexpected successes=1)",
        };
        TestDescription[] ids = {
            new TestDescription("foo.testFoo", "testError"),
            new TestDescription("foo.testFoo", "testExpectedFailure"),
            new TestDescription("foo.testFoo", "testFail"),
            new TestDescription("foo.testFoo", "testFailWithDocString"),
            new TestDescription("foo.testFoo", "testOk"),
            new TestDescription("foo.testFoo", "testOkWithDocString"),
            new TestDescription("foo.testFoo", "testSkipped"),
            new TestDescription("foo.testFoo", "testUnexpectedSuccess")
        };
        boolean[] didPass = {false, true, false, false, true, true, false, false};
        boolean[] didSkip = {false, false, false, false, false, false, true, false};
        setTestIdChecks(ids, didPass, didSkip);
        setRunListenerChecks(8, 1000, false);

        replay(mMockListener);
        mParser.processNewLines(output);
        verify(mMockListener);
    }

    public void testCaptureMultilineTraceback() {
        String[] output = {
                "b (a) ... ERROR",
                "",
                PythonUnitTestResultParser.EQUAL_LINE,
                "ERROR: b (a)",
                PythonUnitTestResultParser.DASH_LINE,
                "Traceback (most recent call last):",
                "  File \"test_rangelib.py\", line 129, in test_reallyfail",
                "    raise ValueError()",
                "ValueError",
                "",
                PythonUnitTestResultParser.DASH_LINE,
                "Ran 1 test in 1s",
                "",
                "FAILED (errors=1)"
        };
        String[] tracebackLines = Arrays.copyOfRange(output, 5, 10);
        String expectedTrackback = String.join(System.lineSeparator(), tracebackLines);

        mMockListener.testStarted(anyObject());
        expectLastCall().times(1);
        mMockListener.testFailed(anyObject(), eq(expectedTrackback));
        expectLastCall().times(1);
        mMockListener.testEnded(anyObject(), (HashMap<String, Metric>) anyObject());
        expectLastCall().times(1);
        setRunListenerChecks(1, 1000, false);

        replay(mMockListener);
        mParser.processNewLines(output);
        verify(mMockListener);
    }

    private void setRunListenerChecks(int numTests, long time, boolean didPass) {
        mMockListener.testRunStarted("test", numTests);
        expectLastCall().times(1);
        mMockListener.testRunFailed((String)anyObject());
        if (!didPass) {
            expectLastCall().times(1);
        } else {
            expectLastCall().andThrow(new AssertionFailedError()).anyTimes();
        }
        mMockListener.testRunEnded(time, new HashMap<String, Metric>());
        expectLastCall().times(1);
    }

    private void setTestIdChecks(TestDescription[] ids, boolean[] didPass) {
        for (int i = 0; i < ids.length; i++) {
            mMockListener.testStarted(ids[i]);
            expectLastCall().times(1);
            if (didPass[i]) {
                mMockListener.testEnded(ids[i], new HashMap<String, Metric>());
                expectLastCall().times(1);
                mMockListener.testFailed(eq(ids[i]), (String)anyObject());
                expectLastCall().andThrow(new AssertionFailedError()).anyTimes();
            } else {
                mMockListener.testFailed(eq(ids[i]), (String)anyObject());
                expectLastCall().times(1);
                mMockListener.testEnded(ids[i], new HashMap<String, Metric>());
                expectLastCall().times(1);
            }
        }
    }

    private void setTestIdChecks(TestDescription[] ids, boolean[] didPass, boolean[] didSkip) {
        for (int i = 0; i < ids.length; i++) {
            mMockListener.testStarted(ids[i]);
            expectLastCall().times(1);
            if (didPass[i]) {
                mMockListener.testEnded(ids[i], new HashMap<String, Metric>());
                expectLastCall().times(1);
                mMockListener.testFailed(eq(ids[i]), (String) anyObject());
                expectLastCall().andThrow(new AssertionFailedError()).anyTimes();
            } else if (didSkip[i]) {
                mMockListener.testIgnored(ids[i]);
                expectLastCall().times(1);
                mMockListener.testEnded(ids[i], new HashMap<String, Metric>());
                expectLastCall().times(1);
            } else {
                mMockListener.testFailed(eq(ids[i]), (String)anyObject());
                expectLastCall().times(1);
                mMockListener.testEnded(ids[i], new HashMap<String, Metric>());
                expectLastCall().times(1);
            }
        }
    }
}

