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

import com.android.tradefed.log.LogUtil.CLog;

import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.io.Reader;
import java.io.Writer;

/**
 * A helper for interruptible process execution.
 */
public class ProcessHelper {
    // Timeout values of destroying the process.
    private static final int DESTROY_PROCESS_MAX_POLL_COUNT = 3;
    private static final long DESTROY_PROCESS_POLL_INTERVAL_MSECS = 500;

    // Timeout value of joining the stdout and stderr threads.
    private static final int THREAD_JOIN_TIMEOUT_MSECS = 1000;

    // The process being monitored.
    private final Process mProcess;

    // The stdout and stderr of the process.
    private final Reader mStdoutReader;
    private final Reader mStderrReader;

    // The threads redirecting the stdout and stderr to buffers.
    private final ReaderThread mStdoutThread;
    private final ReaderThread mStderrThread;

    // The buffers of stdout and stderr.
    private final StringBuilder mStdout;
    private final StringBuilder mStderr;

    // The stdin of the process.
    private final Writer mStdinWriter;

    /**
     * A thread that keeps reading string from an input stream.
     */
    static class ReaderThread extends Thread {
        private static final int BUF_SIZE = 16 * 1024;
        private Reader mReader;
        private StringBuilder mBuffer;

        static enum LogType {
            STDOUT,
            STDERR;
        }

        private LogType mLogType;

        /**
         * @param reader the input stream to read from.
         * @param buffer the buffer containing the input data.
         * @param name the name of the thread.
         * @param logType enum, type of log output.
         */
        public ReaderThread(Reader reader, StringBuilder buffer, String name, LogType logType) {
            super(name);
            mReader = reader;
            mBuffer = buffer;
            mLogType = logType;
        }

        /**
         * Read string from the input stream until EOF.
         */
        @Override
        public void run() {
            char[] charBuffer = new char[BUF_SIZE];
            // reader will be closed in cleanUp()
            try {
                while (true) {
                    int readCount = mReader.read(charBuffer, 0, charBuffer.length);
                    if (readCount < 0) {
                        break;
                    }
                    String newRead = new String(charBuffer, 0, readCount);

                    int newLineLen = 0;
                    if (newRead.endsWith("\r\n")) {
                        newLineLen = 2;
                    } else if (newRead.endsWith("\n")) {
                        newLineLen = 1;
                    }

                    String newReadPrint = newRead.substring(0, newRead.length() - newLineLen);
                    switch (mLogType) {
                        case STDOUT:
                            CLog.i(newReadPrint);
                            break;
                        case STDERR:
                            CLog.e(newReadPrint);
                            break;
                    }
                    mBuffer.append(newRead);
                }
            } catch (IOException e) {
                CLog.e("IOException during ProcessHelper#ReaderThread run.");
                CLog.e(e);
            }
        }
    }

    /**
     * This class waits for a process. It is run by {@link IRunUtil}.
     */
    class VtsRunnable implements IRunUtil.IRunnableResult {
        private boolean mCancelled = false;
        private Thread mExecutionThread = null;
        private final Object mLock = new Object();

        /**
         * @return whether the command is successful. {@link RunUtil} returns
         * {@link CommandStatus#SUCCESS} or {@link CommandStatus#FAILED} according to the
         * this return value.
         */
        @Override
        public boolean run() {
            synchronized (mLock) {
                mExecutionThread = Thread.currentThread();
                if (mCancelled) {
                    CLog.i("Process was cancelled before being awaited.");
                    return false;
                }
            }
            boolean success;
            try {
                success = (mProcess.waitFor() == 0);
                CLog.i("Process terminates normally.");
            } catch (InterruptedException e) {
                success = false;
                CLog.i("Process is interrupted.");
            }
            return success;
        }

        /**
         * This method makes {@link #run()} method stop waiting for the process. It can be called
         * more than once. {@link RunUtil} calls this method if {@link CommandStatus} is TIMED_OUT
         * or EXCEPTION.
         */
        @Override
        public void cancel() {
            CLog.i("Attempt to interrupt execution thread.");
            synchronized (mLock) {
                if (!mCancelled) {
                    mCancelled = true;
                    if (mExecutionThread != null) {
                        mExecutionThread.interrupt();
                    } else {
                        CLog.i("Execution thread has not started.");
                    }
                } else {
                    CLog.i("Execution thread has been cancelled.");
                }
            }
        }

        /**
         * @return the thread of {@link #run()}; null if the thread has not started.
         */
        public Thread getExecutionThread() {
            synchronized (mLock) {
                return mExecutionThread;
            }
        }
    }

    /**
     * Create an instance that monitors a running process.
     *
     * @param process the process to monitor.
     */
    public ProcessHelper(Process process) {
        mProcess = process;
        mStdout = new StringBuilder();
        mStderr = new StringBuilder();
        mStdinWriter = new OutputStreamWriter(mProcess.getOutputStream());
        mStdoutReader = new InputStreamReader(mProcess.getInputStream());
        mStderrReader = new InputStreamReader(mProcess.getErrorStream());
        mStdoutThread = new ReaderThread(
                mStdoutReader, mStdout, "process-helper-stdout", ReaderThread.LogType.STDOUT);
        mStderrThread = new ReaderThread(
                mStderrReader, mStderr, "process-helper-stderr", ReaderThread.LogType.STDERR);
        mStdoutThread.start();
        mStderrThread.start();
    }

    /**
     * Wait for the process until termination, timeout, or interrupt.
     *
     * @param timeoutMsecs the time to wait in milliseconds.
     * @return {@link CommandStatus#SUCCESS} or {@link CommandStatus#FAILED} if the process
     * terminated. {@link CommandStatus#TIMED_OUT} if timeout. {@link CommandStatus#EXCEPTION} for
     * other types of errors.
     * @throws RunInterruptedException if TradeFed interrupts the test invocation.
     */
    public CommandStatus waitForProcess(long timeoutMsecs) throws RunInterruptedException {
        VtsRunnable vtsRunnable = new VtsRunnable();
        CommandStatus status;
        // Use default RunUtil because it can receive the notification of "invocation stop".
        try {
            status = RunUtil.getDefault().runTimed(timeoutMsecs, vtsRunnable, true);
        } catch (RunInterruptedException e) {
            // clear the flag set by default RunUtil.
            Thread.interrupted();
            // RunUtil does not always call cancel() and join() when interrupted.
            vtsRunnable.cancel();
            Thread execThread = vtsRunnable.getExecutionThread();
            if (execThread != null) {
                joinThread(execThread, THREAD_JOIN_TIMEOUT_MSECS);
            }
            throw e;
        }
        if (CommandStatus.SUCCESS.equals(status) || CommandStatus.FAILED.equals(status)) {
            // Join the receiver threads otherwise output might not be available yet.
            joinThread(mStdoutThread, THREAD_JOIN_TIMEOUT_MSECS);
            joinThread(mStderrThread, THREAD_JOIN_TIMEOUT_MSECS);
        } else {
            CLog.w("Process status is %s", status);
        }
        return status;
    }

    /**
     * Write a string to stdin of the process.
     *
     * @param data the string.
     * @throws IOException if the operation fails.
     */
    public void writeStdin(String data) throws IOException {
        mStdinWriter.write(data);
        mStdinWriter.flush();
    }

    /**
     * Close stdin of the process.
     *
     * @throws IOException if the operation fails.
     */
    public void closeStdin() throws IOException {
        mStdinWriter.close();
    }

    /**
     * @return the stdout of the process. As the buffer is not thread safe, the caller must call
     * {@link #cleanUp()} or {@link #waitForProcess(long)} to ensure process termination before
     * calling this method.
     */
    public String getStdout() {
        return mStdout.toString();
    }

    /**
     * @return the stderr of the process. As the buffer is not thread safe, the caller must call
     * {@link #cleanUp()} or {@link #waitForProcess(long)} to ensure process termination before
     * calling this method.
     */
    public String getStderr() {
        return mStderr.toString();
    }

    /**
     * @return whether the process is running.
     */
    public boolean isRunning() {
        try {
            mProcess.exitValue();
            return false;
        } catch (IllegalThreadStateException e) {
            return true;
        }
    }

    /**
     * Kill the process if it is running. Clean up all threads and streams.
     */
    public void cleanUp() {
        try {
            for (int pollCount = 0; isRunning(); pollCount++) {
                if (pollCount >= DESTROY_PROCESS_MAX_POLL_COUNT) {
                    CLog.e("Cannot destroy the process.");
                    break;
                }
                if (pollCount == 0) {
                    CLog.w("Kill the running process.");
                    mProcess.destroy();
                } else {
                    Thread.sleep(DESTROY_PROCESS_POLL_INTERVAL_MSECS);
                }
            }
        } catch (InterruptedException e) {
            CLog.e(e);
        }
        try {
            closeStdin();
        } catch (IOException e) {
            CLog.e(e);
        }
        try {
            mStdoutReader.close();
        } catch (IOException e) {
            CLog.e(e);
        }
        try {
            mStderrReader.close();
        } catch (IOException e) {
            CLog.e(e);
        }
        joinThread(mStdoutThread, THREAD_JOIN_TIMEOUT_MSECS);
        joinThread(mStderrThread, THREAD_JOIN_TIMEOUT_MSECS);
    }

    /**
     * Join a thread and log error.
     *
     * @param thread the thread to join.
     * @param timeoutMsec the timeout in milliseconds.
     * @return whether the thread is joined successfully.
     */
    private static boolean joinThread(Thread thread, long timeoutMsec) {
        try {
            thread.join(timeoutMsec);
        } catch (InterruptedException e) {
            CLog.e(e);
        }
        if (thread.isAlive()) {
            CLog.e("Failed to join %s.", thread.getName());
            return false;
        }
        return true;
    }
}
