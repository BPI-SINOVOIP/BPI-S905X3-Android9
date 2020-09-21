/*
 * Copyright (C) 2017 The Android Open Source Project
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
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

/**
 * Test cases for {@link ProcessHelper}.
 */
@RunWith(JUnit4.class)
public class ProcessHelperTest {
    private ProcessHelper mProcessHelper;

    /**
     * Return a mock process.
     */
    private Process createMockProcess(
            String stdout, String stderr, int exitValue, long executionTimeMsecs) {
        // No need to close OutputStream and ByteArrayInputStream because doing so has no effect.
        OutputStream stdinStream = new OutputStream() {
            @Override
            public void write(int b) throws IOException {}
        };
        InputStream stdoutStream = new ByteArrayInputStream(stdout.getBytes());
        InputStream stderrStream = new ByteArrayInputStream(stderr.getBytes());
        long endTime = System.currentTimeMillis() + executionTimeMsecs;

        return new Process() {
            private boolean destroyed = false;

            private boolean isRunning() {
                return System.currentTimeMillis() <= endTime && !destroyed;
            }

            @Override
            public void destroy() {
                destroyed = true;
            }

            @Override
            public int exitValue() {
                if (isRunning()) {
                    throw new IllegalThreadStateException();
                }
                return exitValue;
            }

            @Override
            public InputStream getInputStream() {
                return stdoutStream;
            }

            @Override
            public OutputStream getOutputStream() {
                return stdinStream;
            }

            @Override
            public InputStream getErrorStream() {
                return stderrStream;
            }

            @Override
            public int waitFor() throws InterruptedException {
                while (isRunning()) {
                    Thread.sleep(50);
                }
                return exitValue;
            }
        };
    }

    /**
     * Reset the ProcessHelper.
     */
    @Before
    public void setUp() {
        mProcessHelper = null;
    }

    /**
     * Terminate the process, join threads and close IO streams.
     */
    @After
    public void tearDown() {
        if (mProcessHelper != null) {
            mProcessHelper.cleanUp();
        }
    }

    /**
     * Test running a process that returns zero.
     */
    @Test
    public void testSuccess() {
        mProcessHelper = new ProcessHelper(createMockProcess("123\n", "456\n", 0, 10));
        CommandStatus status = mProcessHelper.waitForProcess(10000);
        assertEquals(CommandStatus.SUCCESS, status);
        assertFalse(mProcessHelper.isRunning());
        assertTrue(mProcessHelper.getStdout().equals("123\n"));
        assertTrue(mProcessHelper.getStderr().equals("456\n"));
    }

    /**
     * Test running a process that returns non-zero.
     */
    @Test
    public void testFailure() {
        mProcessHelper = new ProcessHelper(createMockProcess("123\n", "456\n", 1, 10));
        CommandStatus status = mProcessHelper.waitForProcess(10000);
        assertEquals(CommandStatus.FAILED, status);
        assertFalse(mProcessHelper.isRunning());
        assertTrue(mProcessHelper.getStdout().equals("123\n"));
        assertTrue(mProcessHelper.getStderr().equals("456\n"));
    }

    /**
     * Test running a process that times out.
     */
    @Test
    public void testTimeout() {
        mProcessHelper = new ProcessHelper(createMockProcess("", "", 1, 10000));
        CommandStatus status = mProcessHelper.waitForProcess(100);
        assertEquals(CommandStatus.TIMED_OUT, status);
        assertTrue(mProcessHelper.isRunning());
    }

    /**
     * Test running a process and being interrupted.
     */
    @Test
    public void testInterrupt() throws InterruptedException {
        mProcessHelper = new ProcessHelper(createMockProcess("", "", 1, 10000));
        IRunUtil runUtil = RunUtil.getDefault();
        Thread testThread = Thread.currentThread();

        Thread timer = new Thread() {
            @Override
            public void run() {
                try {
                    Thread.sleep(50);
                } catch (InterruptedException e) {
                    fail();
                }
                runUtil.interrupt(testThread, "unit test");
            }
        };

        runUtil.allowInterrupt(true);
        timer.start();
        try {
            mProcessHelper.waitForProcess(100);
            fail();
        } catch (RunInterruptedException e) {
            assertTrue(mProcessHelper.isRunning());
        } finally {
            timer.join(1000);
        }
    }
}
