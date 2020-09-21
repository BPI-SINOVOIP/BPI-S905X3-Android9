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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;
import com.android.tradefed.util.IRunUtil;

import org.easymock.EasyMock;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.util.HashMap;

/** Unit tests for {@link PythonUnitTestRunner}. */
@RunWith(JUnit4.class)
public class PythonUnitTestRunnerTest {

    private static final String[] TEST_PASS_STDERR = {
        "b (a) ... ok", "", PythonUnitTestResultParser.DASH_LINE, "Ran 1 tests in 1s", "", "OK",
    };

    private static final String[] TEST_FAIL_STDERR = {
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
        "Ran 1 tests in 1s",
        "",
        "FAILED (errors=1)",
    };

    private static final String[] TEST_EXECUTION_FAIL_STDERR = {
        "Traceback (most recent call last):",
        "  File \"/usr/lib/python2.7/runpy.py\", line 162, in _run_module_as_main",
        "    \"__main__\", fname, loader, pkg_name)",
        "  File \"/usr/lib/python2.7/runpy.py\", line 72, in _run_code",
        "    exec code in run_globals",
        "  File \"/usr/lib/python2.7/unittest/__main__.py\", line 12, in <module>",
        "    main(module=None)",
        "  File \"/usr/lib/python2.7/unittest/main.py\", line 94, in __init__",
        "    self.parseArgs(argv)",
        "  File \"/usr/lib/python2.7/unittest/main.py\", line 149, in parseArgs",
        "    self.createTests()",
        "  File \"/usr/lib/python2.7/unittest/main.py\", line 158, in createTests",
        "    self.module)",
        "  File \"/usr/lib/python2.7/unittest/loader.py\", line 130, in loadTestsFromNames",
        "    suites = [self.loadTestsFromName(name, module) for name in names]",
        "  File \"/usr/lib/python2.7/unittest/loader.py\", line 91, in loadTestsFromName",
        "    module = __import__('.'.join(parts_copy))",
        "ImportError: No module named stub",
    };

    private enum UnitTestResult {
        PASS,
        FAIL,
        EXECUTION_FAIL,
        TIMEOUT;

        private String getStderr() {
            switch (this) {
                case PASS:
                    return String.join("\n", TEST_PASS_STDERR);
                case FAIL:
                    return String.join("\n", TEST_FAIL_STDERR);
                case EXECUTION_FAIL:
                    return String.join("\n", TEST_EXECUTION_FAIL_STDERR);
                case TIMEOUT:
                    return null;
            }
            return null;
        }

        private CommandStatus getStatus() {
            switch (this) {
                case PASS:
                    return CommandStatus.SUCCESS;
                case FAIL:
                    return CommandStatus.FAILED;
                case EXECUTION_FAIL:
                    // UnitTest runner returns with exit code 1 if execution failed.
                    return CommandStatus.FAILED;
                case TIMEOUT:
                    return CommandStatus.TIMED_OUT;
            }
            return null;
        }

        public CommandResult getCommandResult() {
            CommandResult cr = new CommandResult();
            cr.setStderr(this.getStderr());
            cr.setStatus(this.getStatus());
            return cr;
        }
    }

    private PythonUnitTestRunner mRunner;
    private ITestInvocationListener mMockListener;

    @Before
    public void setUp() throws Exception {
        mRunner = new PythonUnitTestRunner();
        mMockListener = EasyMock.createMock(ITestInvocationListener.class);
    }

    @Test
    public void testCheckPythonVersion_276given270min() {
        CommandResult c = new CommandResult();
        c.setStderr("Python 2.7.6");
        mRunner.checkPythonVersion(c);
    }

    @Test
    public void testCheckPythonVersion_276given331min() {
        CommandResult c = new CommandResult();
        c.setStderr("Python 2.7.6");
        mRunner.setMinPythonVersion("3.3.1");
        try {
            mRunner.checkPythonVersion(c);
            fail("Should have thrown an exception.");
        } catch (RuntimeException expected) {
            // expected
        }
    }

    @Test
    public void testCheckPythonVersion_300given276min() {
        CommandResult c = new CommandResult();
        c.setStderr("Python 3.0.0");
        mRunner.setMinPythonVersion("2.7.6");
        mRunner.checkPythonVersion(c);
    }

    private IRunUtil getMockRunUtil(UnitTestResult testResult) {
        CommandResult expectedResult = testResult.getCommandResult();
        IRunUtil mockRunUtil = EasyMock.createMock(IRunUtil.class);
        // EasyMock checks the number of arguments when verifying method call.
        // The actual runTimedCmd() expected here looks like:
        // runTimedCmd(300000, null, "-m", "unittest", "-v", "")
        EasyMock.expect(
                        mockRunUtil.runTimedCmd(
                                EasyMock.anyLong(),
                                (String) EasyMock.anyObject(),
                                (String) EasyMock.anyObject(),
                                (String) EasyMock.anyObject(),
                                (String) EasyMock.anyObject(),
                                (String) EasyMock.anyObject()))
                .andReturn(expectedResult)
                .times(1);
        return mockRunUtil;
    }

    private void setMockListenerExpectTestPass(boolean testPass) {
        mMockListener.testRunStarted((String) EasyMock.anyObject(), EasyMock.anyInt());
        EasyMock.expectLastCall().times(1);
        mMockListener.testStarted((TestDescription) EasyMock.anyObject());
        EasyMock.expectLastCall().times(1);
        if (!testPass) {
            mMockListener.testFailed(
                    (TestDescription) EasyMock.anyObject(), (String) EasyMock.anyObject());
            EasyMock.expectLastCall().times(1);
        }
        mMockListener.testEnded(
                (TestDescription) EasyMock.anyObject(),
                (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.expectLastCall().times(1);
        if (!testPass) {
            mMockListener.testRunFailed((String) EasyMock.anyObject());
            EasyMock.expectLastCall().times(1);
        }
        mMockListener.testRunEnded(
                EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.expectLastCall().times(1);
    }

    /** Test execution succeeds and all test cases pass. */
    @Test
    public void testRunPass() {
        IRunUtil mockRunUtil = getMockRunUtil(UnitTestResult.PASS);
        setMockListenerExpectTestPass(true);
        EasyMock.replay(mMockListener, mockRunUtil);
        mRunner.doRunTest(mMockListener, mockRunUtil, "");
        EasyMock.verify(mMockListener, mockRunUtil);
    }

    /** Test execution succeeds and some test cases fail. */
    @Test
    public void testRunFail() {
        IRunUtil mockRunUtil = getMockRunUtil(UnitTestResult.FAIL);
        setMockListenerExpectTestPass(false);
        EasyMock.replay(mMockListener, mockRunUtil);
        mRunner.doRunTest(mMockListener, mockRunUtil, "");
        EasyMock.verify(mMockListener, mockRunUtil);
    }

    /** Test execution fails. */
    @Test
    public void testRunExecutionFail() {
        IRunUtil mockRunUtil = getMockRunUtil(UnitTestResult.EXECUTION_FAIL);
        EasyMock.replay(mockRunUtil);
        try {
            mRunner.doRunTest(mMockListener, mockRunUtil, "");
            fail("Should not reach here.");
        } catch (RuntimeException e) {
            assertEquals("Failed to parse Python unittest result", e.getMessage());
        }
        EasyMock.verify(mockRunUtil);
    }

    /** Test execution times out. */
    @Test
    public void testRunTimeout() {
        IRunUtil mockRunUtil = getMockRunUtil(UnitTestResult.TIMEOUT);
        EasyMock.replay(mockRunUtil);
        try {
            mRunner.doRunTest(mMockListener, mockRunUtil, "");
            fail("Should not reach here.");
        } catch (RuntimeException e) {
            assertTrue(e.getMessage().startsWith("Python unit test timed out after"));
        }
        EasyMock.verify(mockRunUtil);
    }
}