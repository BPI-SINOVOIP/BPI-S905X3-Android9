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
package com.android.tradefed.util;

import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.IRunUtil.IRunnableResult;

import junit.framework.TestCase;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.io.Writer;

/**
 * Longer running tests for {@link RunUtilFuncTest}
 */
public class RunUtilFuncTest extends TestCase {

    private static final long VERY_SHORT_TIMEOUT_MS = 10l;
    private static final long SHORT_TIMEOUT_MS = 500l;
    private static final long LONG_TIMEOUT_MS = 5000l;

    private abstract class MyRunnable implements IRunUtil.IRunnableResult {
        boolean mCanceled = false;

        @Override
        public void cancel() {
            mCanceled = true;
        }
    }

    /**
     * Test timeout case for {@link RunUtil#runTimed(long, IRunnableResult, boolean)}.
     */
    public void testRunTimed_timeout() {
        MyRunnable mockRunnable = new MyRunnable() {
            @Override
            public boolean run() {
                try {
                    Thread.sleep(SHORT_TIMEOUT_MS * 5);
                } catch (InterruptedException e) {
                    // ignore
                }
                return true;
            }
        };
        assertEquals(CommandStatus.TIMED_OUT, RunUtil.getDefault().runTimed(SHORT_TIMEOUT_MS,
                mockRunnable, true));
        assertTrue(mockRunnable.mCanceled);
    }

    /**
     * Test method for {@link RunUtil#runTimedRetry(long, long, int, IRunnableResult)}.
     * Verify that multiple attempts are made.
     */
    public void testRunTimedRetry() {
        final int maxAttempts = 5;
        IRunUtil.IRunnableResult mockRunnable = new IRunUtil.IRunnableResult() {
            int attempts = 0;
            @Override
            public boolean run() {
                attempts++;
                return attempts == maxAttempts;
            }
            @Override
            public void cancel() {
                // ignore
            }
        };
        final long startTime = System.currentTimeMillis();
        assertTrue(RunUtil.getDefault().runTimedRetry(100, SHORT_TIMEOUT_MS, maxAttempts,
                mockRunnable));
        final long actualTime = System.currentTimeMillis() - startTime;
        // assert that time actually taken is at least, and no more than twice expected
        final long expectedPollTime = SHORT_TIMEOUT_MS * (maxAttempts-1);
        assertTrue(String.format("Expected poll time %d, got %d", expectedPollTime, actualTime),
                expectedPollTime <= actualTime && actualTime <= (2 * expectedPollTime));
    }

    /**
     * Test timeout case for {@link RunUtil#runTimedCmd(long, String...)} and ensure we
     * consistently get the right stdout for a fast running command.
     */
    public void testRunTimedCmd_repeatedOutput() {
        for (int i = 0; i < 1000; i++) {
            CommandResult result =
                    RunUtil.getDefault().runTimedCmd(LONG_TIMEOUT_MS, "echo", "hello");
            assertTrue("Failed at iteration " + i,
                    CommandStatus.SUCCESS.equals(result.getStatus()));
            CLog.d(result.getStdout());
            assertTrue(result.getStdout().trim().equals("hello"));
        }
    }

    /**
     * Test that that running a command with a 0 timeout results in no timeout being applied to it.
     */
    public void testRunTimedCmd_noTimeout() {
        // When there is no timeout, max_poll interval will be 30sec so we need a test with more
        // than 30sec
        CommandResult result = RunUtil.getDefault().runTimedCmd(0l, "sleep", "35");
        assertTrue(CommandStatus.SUCCESS.equals(result.getStatus()));
        assertTrue(result.getStdout().isEmpty());
    }

    /**
     * Test case for {@link RunUtil#runTimedCmd(long, String...)} for a command that produces
     * a large amount of output
     * @throws IOException
     */
    public void testRunTimedCmd_largeOutput() throws IOException {
        // 1M  chars
        int dataSize = 1000000;
        File f = FileUtil.createTempFile("foo", ".txt");
        Writer s = null;
        try {
            s = new BufferedWriter(new FileWriter(f));
            for (int i=0; i < dataSize; i++) {
                s.write('a');
            }
            s.close();

            // FIXME: this test case is not ideal, as it will only work on platforms that support
            // cat command.
            CommandResult result =
                    RunUtil.getDefault()
                            .runTimedCmd(3 * LONG_TIMEOUT_MS, "cat", f.getAbsolutePath());
            assertEquals(
                    String.format(
                            "We expected SUCCESS but got %s, with stdout: '%s'\nstderr: %s",
                            result.getStatus(), result.getStdout(), result.getStderr()),
                    CommandStatus.SUCCESS,
                    result.getStatus());
            assertTrue(result.getStdout().length() == dataSize);
        } finally {
            f.delete();
            StreamUtil.close(s);
        }
    }

    /**
     * Test case for {@link RunUtil#unsetEnvVariable(String key)}
     */
    public void testUnsetEnvVariable() {
        RunUtil runUtil = new RunUtil();
        runUtil.setEnvVariable("bar", "foo");
        // FIXME: this test case is not ideal, as it will only work on platforms that support
        // printenv
        CommandResult result =
                runUtil.runTimedCmdRetry(SHORT_TIMEOUT_MS, SHORT_TIMEOUT_MS, 3, "printenv", "bar");
        assertEquals(
                String.format(
                        "We expected SUCCESS but got %s, with stdout: '%s'\nstderr: %s",
                        result.getStatus(), result.getStdout(), result.getStderr()),
                CommandStatus.SUCCESS,
                result.getStatus());
        assertEquals("foo", result.getStdout().trim());

        // remove env variable
        runUtil.unsetEnvVariable("bar");
        // printenv with non-exist variable will fail
        result = runUtil.runTimedCmd(SHORT_TIMEOUT_MS, "printenv", "bar");
        assertEquals(CommandStatus.FAILED, result.getStatus());
        assertEquals("", result.getStdout().trim());
    }

    /**
     * Test that {@link RunUtil#runTimedCmd(long, String[])} returns timeout when the command is too
     * short and also clean up all its thread.
     */
    public void testRunTimedCmd_timeout() throws InterruptedException {
        RunUtil runUtil = new RunUtil();
        String[] command = {"sleep", "10000"};
        CommandResult result = runUtil.runTimedCmd(VERY_SHORT_TIMEOUT_MS, command);
        assertEquals(
                String.format(
                        "We expected TIMED_OUT but got %s, with stdout: '%s'\nstderr: %s",
                        result.getStatus(), result.getStdout(), result.getStderr()),
                CommandStatus.TIMED_OUT,
                result.getStatus());
        assertEquals("", result.getStdout());
        assertEquals("", result.getStderr());
        // We give it some times to clean up the process
        Thread.sleep(5000);
        Thread[] list = new Thread[Thread.currentThread().getThreadGroup().activeCount()];
        Thread.currentThread().getThreadGroup().enumerate(list);
        // Ensure the list of Threads does not contain the RunnableNotifier or InheritIO threads.
        for (Thread t : list) {
            assertFalse(
                    String.format("We found a thread: %s", t.getName()),
                    t.getName().contains(RunUtil.RUNNABLE_NOTIFIER_NAME));
            assertFalse(
                    String.format("We found a thread: %s", t.getName()),
                    t.getName().contains(RunUtil.INHERITIO_PREFIX));
        }
    }
}
