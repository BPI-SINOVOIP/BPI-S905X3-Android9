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
package com.android.tradefed.log;

import com.android.ddmlib.Log;
import com.android.ddmlib.Log.LogLevel;

import junit.framework.TestCase;

import org.easymock.EasyMock;

/**
 * Unit tests for {@link LogRegistry}.
 */
public class LogRegistryTest extends TestCase {

    private static final String LOG_TAG = "LogRegistryTest";

    private LogRegistry mLogRegistry;
    private ThreadGroup mStubThreadGroup;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mStubThreadGroup = new ThreadGroup("LogRegistryTest");
        mLogRegistry =
                new LogRegistry() {
                    // override thread group to avoid conflict with the "real" LogRegistry and the
                    // logger in use for this test run
                    @Override
                    ThreadGroup getCurrentThreadGroup() {
                        return mStubThreadGroup;
                    }

                    @Override
                    public void saveGlobalLog() {
                        // empty on purpose, avoid leaving logs that we can't clean.
                    }
                };
    }

    @Override
    protected void tearDown() throws Exception {
        super.tearDown();
        mLogRegistry.closeAndRemoveAllLogs();
    }

    /**
     * Tests that {@link LogRegistry#getLogger} returns the logger that was previously registered.
     */
    public void testGetLogger() {
        StdoutLogger stdoutLogger = new StdoutLogger();
        mLogRegistry.registerLogger(stdoutLogger);

        ILeveledLogOutput returnedLogger = mLogRegistry.getLogger();
        assertEquals(stdoutLogger, returnedLogger);
        mLogRegistry.unregisterLogger();
    }

    /**
     * Tests that {@link LogRegistry#printLog} calls into the underlying logger's printLog method
     * when the logging level is appropriate for printing.
     */
    public void testPrintLog_sameLogLevel() {
        String testMessage = "This is a test message.";
        ILeveledLogOutput mockLogger = EasyMock.createMock(ILeveledLogOutput.class);
        mLogRegistry.registerLogger(mockLogger);

        EasyMock.expect(mockLogger.getLogLevel()).andReturn(LogLevel.VERBOSE);
        mockLogger.printLog(LogLevel.VERBOSE, LOG_TAG, testMessage);

        EasyMock.replay(mockLogger);
        mLogRegistry.printLog(LogLevel.VERBOSE, LOG_TAG, testMessage);
        mLogRegistry.unregisterLogger();
    }

    /**
     * Tests that {@link LogRegistry#printLog} does not call into the underlying logger's printLog
     * method when the logging level is lower than necessary to log.
     */
    public void testPrintLog_lowerLogLevel() {
        String testMessage = "This is a test message.";
        ILeveledLogOutput mockLogger = EasyMock.createMock(ILeveledLogOutput.class);
        mLogRegistry.registerLogger(mockLogger);

        // Setting LogLevel == ERROR will let everything print
        EasyMock.expect(mockLogger.getLogLevel()).andReturn(LogLevel.ERROR);

        EasyMock.replay(mockLogger);
        mLogRegistry.printLog(LogLevel.VERBOSE, LOG_TAG, testMessage);
        mLogRegistry.unregisterLogger();
    }

    /**
     * Tests for ensuring new threads spawned without an explicit ThreadGroup will inherit the
     * same logger as the parent's logger.
     */
    public void testThreadedLogging() {
        final String testMessage = "Another test message!";
        final ILeveledLogOutput mockLogger = EasyMock.createMock(ILeveledLogOutput.class);

        // Simple class that will run in a thread, simulating a new thread spawed inside an Invocation
        class SecondThread implements Runnable {
            @Override
            public void run() {
                Log.e(LOG_TAG, testMessage);
            }
        }
        // Simple class that will run in a thread, simulating a new Invocation
        class FirstThread implements Runnable {
            @Override
            public void run() {
                mLogRegistry.registerLogger(mockLogger);
                Log.v(LOG_TAG, testMessage);
                Thread secondThread = new Thread(new SecondThread());  // no explicit ThreadGroup
                secondThread.start();
                try {
                    secondThread.join();  // threaded, but force serialization for testing
                }
                catch (InterruptedException ie) {
                    fail("Thread was unexpectedly interrupted.");
                }
                finally {
                    mLogRegistry.unregisterLogger();
                }
            }
        }

        // first thread calls
        EasyMock.expect(mockLogger.getLogLevel()).andReturn(LogLevel.VERBOSE);
        mockLogger.printLog(LogLevel.VERBOSE, LOG_TAG, testMessage);
        // second thread should inherit same logger
        EasyMock.expect(mockLogger.getLogLevel()).andReturn(LogLevel.ERROR);
        mockLogger.printLog(LogLevel.ERROR, LOG_TAG, testMessage);

        EasyMock.replay(mockLogger);

        ThreadGroup tg = new ThreadGroup("TestThreadGroup");
        Thread firstThread = new Thread(tg, new FirstThread());
        firstThread.start();

        try {
            firstThread.join();  // threaded, but force serialization for testing
        }
        catch (InterruptedException ie) {
            fail("Thread was unexpectedly interrupted.");
        }
    }
}
