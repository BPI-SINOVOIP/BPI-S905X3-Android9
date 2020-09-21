/*
 * Copyright (C) 2014 The Android Open Source Project
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

import com.android.tradefed.device.CollectingOutputReceiver;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;

import java.util.concurrent.Callable;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

/**
 * Contains utility methods and classes for concurrent device side command execution
 * <p>
 * Use {@link ExecutorService} to run commands implemented as {@link ShellCommandCallable}, and use
 * {@link #joinFuture(String, Future, long)} for synchronization against the {@link Future} as
 * returned by {@link ExecutorService} for the command execution.
 */
public class DeviceConcurrentUtil {

    /* private constructor for singleton */
    private DeviceConcurrentUtil() {}

    /**
     * Convenience method to join current thread on the <code>task</code>
     * <p>
     * {@link DeviceNotAvailableException} and {@link TimeoutException} occurred during execution
     * are passed through transparently, others are logged as error but not otherwise handled.
     * @param taskDesc description of task for logging purpose
     * @param task {@link Future} representing the task to join
     * @param timeout timeout for waiting on the task
     * @return The result of the task with the template type.
     * @throws DeviceNotAvailableException
     * @throws TimeoutException
     */
    public static <T> T joinFuture(String taskDesc, Future<T> task, long timeout)
            throws DeviceNotAvailableException, TimeoutException {
        try {
            T ret = task.get(timeout, TimeUnit.MILLISECONDS);
            return ret;
        } catch (InterruptedException e) {
            CLog.e("interrupted while executing " + taskDesc);
        } catch (ExecutionException e) {
            Throwable t = e.getCause();
            if (t instanceof DeviceNotAvailableException) {
                // passthru
                throw (DeviceNotAvailableException)t;
            } else {
                CLog.e("%s while executing %s", t.getClass().getSimpleName(), taskDesc);
                CLog.e(t);
            }
        }
        return null;
    }

    /**
     * A {@link Callable} that wraps the details of executing shell command on
     * an {@link ITestDevice}.
     * <p>
     * Must implement {@link #processOutput(String)} to process the command
     * output and determine return of the <code>Callable</code>
     *
     * @param <V> passthru of the {@link Callable} return type, see
     *            {@link Callable}
     */
    public static abstract class ShellCommandCallable<V> implements Callable<V> {
        private String mCommand;
        private long mTimeout;
        private ITestDevice mDevice;

        public ShellCommandCallable() {
            // do nothing for default
        }

        public ShellCommandCallable(ITestDevice device, String command, long timeout) {
            this();
            mCommand = command;
            mTimeout = timeout;
            mDevice = device;
        }

        public ShellCommandCallable<V> setCommand(String command) {
            mCommand = command;
            return this;
        }

        public ShellCommandCallable<V> setTimeout(long timeout) {
            mTimeout = timeout;
            return this;
        }

        public ShellCommandCallable<V> setDevice(ITestDevice device) {
            mDevice = device;
            return this;
        }

        @Override
        public V call() throws Exception {
            CollectingOutputReceiver receiver = new CollectingOutputReceiver();
            mDevice.executeShellCommand(mCommand, receiver, mTimeout, TimeUnit.MILLISECONDS, 1);
            String output = receiver.getOutput();
            CLog.v("raw output for \"%s\"\n%s", mCommand, output);
            return processOutput(output);
        }

        public abstract V processOutput(String output);
    }

}