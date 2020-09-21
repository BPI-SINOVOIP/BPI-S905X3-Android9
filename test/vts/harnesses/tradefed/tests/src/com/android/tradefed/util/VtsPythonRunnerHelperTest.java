/*
 * Copyright (C) 2018 The Android Open Source Project
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
package com.android.tradefed.util;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.fail;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.build.IFolderBuildInfo;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.IRunUtil;
import com.android.tradefed.util.ProcessHelper;
import com.android.tradefed.util.RunInterruptedException;
import com.android.tradefed.util.VtsPythonRunnerHelper;
import java.io.File;
import java.io.IOException;
import org.easymock.EasyMock;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/**
 * Unit tests for {@link VtsPythonRunnerHelper}.
 */
@RunWith(JUnit4.class)
public class VtsPythonRunnerHelperTest {
    private static final String OS_NAME = VtsPythonRunnerHelper.OS_NAME;
    private static final String WINDOWS = VtsPythonRunnerHelper.WINDOWS;
    private static final String VTS = VtsPythonRunnerHelper.VTS;
    private static final String PYTHONPATH = VtsPythonRunnerHelper.PYTHONPATH;
    private static final String VIRTUAL_ENV_PATH = VtsPythonRunnerHelper.VIRTUAL_ENV_PATH;
    private static final String ANDROID_BUILD_TOP = VtsPythonRunnerHelper.ANDROID_BUILD_TOP;
    private static final String ALT_HOST_TESTCASE_DIR = "ANDROID_HOST_OUT_TESTCASES";
    private static final String LINUX = "Linux";
    private static final String[] mPythonCmd = {"python"};
    private static final long mTestTimeout = 1000 * 5;

    private ProcessHelper mProcessHelper = null;
    private VtsPythonRunnerHelper mVtsPythonRunnerHelper = null;

    @Before
    public void setUp() throws Exception {
        IFolderBuildInfo buildInfo = EasyMock.createNiceMock(IFolderBuildInfo.class);
        EasyMock.replay(buildInfo);
        mVtsPythonRunnerHelper = new VtsPythonRunnerHelper(buildInfo) {
            @Override
            protected ProcessHelper createProcessHelper(String[] cmd) {
                return mProcessHelper;
            }
        };
    }

    /**
     * Create a process helper which mocks status of a running process.
     */
    private static ProcessHelper createMockProcessHelper(
            CommandStatus status, boolean interrupted, boolean keepRunning) {
        Process process;
        try {
            process = new ProcessBuilder("true").start();
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
        return new ProcessHelper(process) {
            @Override
            public CommandStatus waitForProcess(long timeoutMsecs) throws RunInterruptedException {
                if (interrupted) {
                    throw new RunInterruptedException();
                }
                return status;
            }

            @Override
            public boolean isRunning() {
                return keepRunning;
            }
        };
    }

    private static ProcessHelper createMockProcessHelper(
            CommandStatus status, boolean interrupted) {
        return createMockProcessHelper(status, interrupted, /*keepRunning=*/false);
    }

    private static ProcessHelper createMockProcessHelper(CommandStatus status) {
        return createMockProcessHelper(status, /*interrupted=*/false, /*keepRunning=*/false);
    }

    /**
     * Create a mock runUtil with returns the expected results.
     */
    private IRunUtil createMockRunUtil(CommandResult result) {
        IRunUtil runUtil = EasyMock.createMock(IRunUtil.class);
        EasyMock.expect(runUtil.runTimedCmd(
                                EasyMock.anyLong(), EasyMock.eq("which"), EasyMock.eq("python")))
                .andReturn(result)
                .anyTimes();
        EasyMock.expect(runUtil.runTimedCmd(EasyMock.anyLong(), EasyMock.eq("where"),
                                EasyMock.eq("python.exe")))
                .andReturn(result)
                .anyTimes();
        EasyMock.replay(runUtil);
        return runUtil;
    }

    @Test
    public void testProcessRunSuccess() {
        CommandResult commandResult = new CommandResult();
        mProcessHelper = createMockProcessHelper(CommandStatus.SUCCESS);
        String interruptMessage =
                mVtsPythonRunnerHelper.runPythonRunner(mPythonCmd, commandResult, mTestTimeout);
        assertEquals(interruptMessage, null);
        assertEquals(commandResult.getStatus(), CommandStatus.SUCCESS);
    }

    @Test
    public void testProcessRunFailed() {
        CommandResult commandResult = new CommandResult();
        mProcessHelper = createMockProcessHelper(CommandStatus.FAILED);
        String interruptMessage =
                mVtsPythonRunnerHelper.runPythonRunner(mPythonCmd, commandResult, mTestTimeout);
        assertEquals(interruptMessage, null);
        assertEquals(commandResult.getStatus(), CommandStatus.FAILED);
    }

    @Test
    public void testProcessRunTimeout() {
        CommandResult commandResult = new CommandResult();
        mProcessHelper = createMockProcessHelper(CommandStatus.TIMED_OUT);
        String interruptMessage =
                mVtsPythonRunnerHelper.runPythonRunner(mPythonCmd, commandResult, mTestTimeout);
        assertEquals(interruptMessage, null);
        assertEquals(commandResult.getStatus(), CommandStatus.TIMED_OUT);
    }

    @Test
    public void testProcessRunInterrupted() {
        CommandResult commandResult = new CommandResult();
        mProcessHelper = createMockProcessHelper(null, /*interrupted=*/true);
        String interruptMessage =
                mVtsPythonRunnerHelper.runPythonRunner(mPythonCmd, commandResult, mTestTimeout);
        assertNotEquals(interruptMessage, null);
        assertEquals(commandResult.getStatus(), CommandStatus.TIMED_OUT);
    }

    @Test
    public void testGetPythonBinaryNullBuildInfo() {
        CommandResult findPythonresult = new CommandResult();
        findPythonresult.setStatus(CommandStatus.SUCCESS);
        findPythonresult.setStdout("/user/bin/python");
        IRunUtil runUtil = createMockRunUtil(findPythonresult);
        mVtsPythonRunnerHelper = new VtsPythonRunnerHelper(null);
        mVtsPythonRunnerHelper.setRunUtil(runUtil);
        String pythonBinary = mVtsPythonRunnerHelper.getPythonBinary();
        assertEquals(pythonBinary, "/user/bin/python");
    }

    @Test
    public void testGetPythonBinaryPythonBinaryNotExists() {
        CommandResult findPythonresult = new CommandResult();
        findPythonresult.setStatus(CommandStatus.SUCCESS);
        findPythonresult.setStdout("/user/bin/python");
        IRunUtil runUtil = createMockRunUtil(findPythonresult);
        IBuildInfo mockBuildInfo = EasyMock.createNiceMock(IBuildInfo.class);
        EasyMock.expect(mockBuildInfo.getFile(EasyMock.eq("VIRTUALENVPATH")))
                .andReturn(new File("NonExists"))
                .atLeastOnce();
        EasyMock.replay(mockBuildInfo);
        mVtsPythonRunnerHelper = new VtsPythonRunnerHelper(mockBuildInfo);
        mVtsPythonRunnerHelper.setRunUtil(runUtil);
        try {
            String pythonBinary = mVtsPythonRunnerHelper.getPythonBinary();
            assertEquals(pythonBinary, "/user/bin/python");
        } catch (RuntimeException e) {
            fail();
        }

        EasyMock.verify(mockBuildInfo);
    }

    @Test
    public void testGetPythonBinaryException() {
        CommandResult findPythonresult = new CommandResult();
        findPythonresult.setStatus(CommandStatus.FAILED);
        findPythonresult.setStdout("");
        IRunUtil runUtil = createMockRunUtil(findPythonresult);
        mVtsPythonRunnerHelper = new VtsPythonRunnerHelper(null);
        mVtsPythonRunnerHelper.setRunUtil(runUtil);
        try {
            String pythonBinary = mVtsPythonRunnerHelper.getPythonBinary();
        } catch (RuntimeException e) {
            assertEquals("Could not find python binary on host machine", e.getMessage());
            return;
        }
        fail();
    }

    @Test
    public void testGetPythonBinaryNormalOnLinux() {
        String originalName = System.getProperty(OS_NAME);
        if (originalName.contains(WINDOWS)) {
            System.setProperty(OS_NAME, LINUX);
        }
        try {
            runTestPythonBinaryNormal(false);
        } catch (IOException e) {
            fail();
        } finally {
            System.setProperty(OS_NAME, originalName);
        }
    }

    @Test
    public void testGetPythonBinaryNormalOnWindows() {
        String originalName = System.getProperty(OS_NAME);
        if (!originalName.contains(WINDOWS)) {
            System.setProperty(OS_NAME, WINDOWS);
        }
        try {
            runTestPythonBinaryNormal(true);
        } catch (IOException e) {
            fail();
        } finally {
            System.setProperty(OS_NAME, originalName);
        }
    }

    public void runTestPythonBinaryNormal(boolean isWindows) throws IOException {
        String python = isWindows ? "python.exe" : "python";
        String binDir = isWindows ? "Scripts" : "bin";
        File testDir = FileUtil.createTempDir("testVirtualEnv");
        File testPython = new File(testDir, binDir + File.separator + python);
        testPython.getParentFile().mkdirs();
        testPython.createNewFile();
        CLog.i("creating test file: " + testPython.getAbsolutePath());
        IBuildInfo mockBuildInfo = EasyMock.createNiceMock(IBuildInfo.class);
        EasyMock.expect(mockBuildInfo.getFile(EasyMock.eq("VIRTUALENVPATH")))
                .andReturn(testDir)
                .atLeastOnce();
        EasyMock.replay(mockBuildInfo);
        mVtsPythonRunnerHelper = new VtsPythonRunnerHelper(mockBuildInfo);
        String pythonBinary = mVtsPythonRunnerHelper.getPythonBinary();
        assertEquals(pythonBinary, testPython.getAbsolutePath());
        FileUtil.recursiveDelete(testDir);
        EasyMock.verify(mockBuildInfo);
    }

    @Test
    public void testGetPythonPath() {
        StringBuilder sb = new StringBuilder();
        String separator = File.pathSeparator;
        if (System.getenv(PYTHONPATH) != null) {
            sb.append(separator);
            sb.append(System.getenv(PYTHONPATH));
        }

        File testVts = new File("TEST_VTS");
        File testPythonPath = new File("TEST_PYTHON_PATH");

        String altTestsDir = System.getenv().get(ALT_HOST_TESTCASE_DIR);
        if (altTestsDir != null) {
            File testsDir = new File(altTestsDir);
            sb.append(separator);
            sb.append(testsDir.getAbsolutePath());
        } else {
            sb.append(separator);
            sb.append(testVts.getAbsolutePath()).append("/..");
        }

        sb.append(separator);
        sb.append(testPythonPath.getAbsolutePath());
        if (System.getenv("ANDROID_BUILD_TOP") != null) {
            sb.append(separator);
            sb.append(System.getenv("ANDROID_BUILD_TOP")).append("/test");
        }

        IBuildInfo mockBuildInfo = EasyMock.createNiceMock(IBuildInfo.class);
        EasyMock.expect(mockBuildInfo.getFile(EasyMock.eq(VTS))).andReturn(testVts).anyTimes();
        EasyMock.expect(mockBuildInfo.getFile(EasyMock.eq(PYTHONPATH)))
                .andReturn(testPythonPath)
                .times(2);
        EasyMock.replay(mockBuildInfo);

        mVtsPythonRunnerHelper = new VtsPythonRunnerHelper(mockBuildInfo);
        assertEquals(sb.substring(1), mVtsPythonRunnerHelper.getPythonPath());

        EasyMock.verify(mockBuildInfo);
    }
}
