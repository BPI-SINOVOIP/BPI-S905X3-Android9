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

package com.android.tradefed.command;

import com.android.tradefed.command.CommandRunner.ExitCode;
import com.android.tradefed.config.ConfigurationException;
import com.android.tradefed.config.IConfigurationFactory;
import com.android.tradefed.device.FreeDeviceState;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.device.NoDeviceException;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.invoker.ITestInvocation;
import com.android.tradefed.result.ITestInvocationListener;

import java.io.PrintWriter;
import java.util.List;
import java.util.Map;

/**
 * A scheduler for running TradeFederation commands.
 */
public interface ICommandScheduler {

    /**
    * Listener for invocation events when invocation completes.
    * @see #execCommand(IScheduledInvocationListener, String[])
    */
    public static interface IScheduledInvocationListener extends ITestInvocationListener {
        /**
         * Callback when an invocation is initiated. This is called before any builds are fetched.
         *
         * @param context
         */
        public default void invocationInitiated(IInvocationContext context) {}

        /**
         * Callback when entire invocation has completed, including all {@link
         * ITestInvocationListener#invocationEnded(long)} events.
         *
         * @param context
         * @param devicesStates
         */
        public void invocationComplete(
                IInvocationContext context, Map<ITestDevice, FreeDeviceState> devicesStates);
    }

    /**
     * Adds a command to the scheduler.
     * <p/>
     * A command is essentially an instance of a configuration to run and its associated arguments.
     * <p/>
     * If "--help" argument is specified the help text for
     * the config will be outputed to stdout. Otherwise, the config will be added to the queue to
     * run.
     *
     * @param args the config arguments.
     * @return <code>true</code> if command was added successfully
     * @throws ConfigurationException if command could not be parsed
     *
     * @see IConfigurationFactory#createConfigurationFromArgs(String[])
     */
    public boolean addCommand(String[] args) throws ConfigurationException;

    /**
     * Adds all commands from given file to the scheduler
     *
     * @param cmdFile the filesystem path of comand file
     * @param extraArgs a {@link List} of {@link String} arguments to append to each command parsed
     *            from file. Can be empty but should not be null.
     * @throws ConfigurationException if command file could not be parsed
     * @see CommandFileParser
     */
    public void addCommandFile(String cmdFile, List<String> extraArgs)
            throws ConfigurationException;

    /**
     * An alternate {@link #addCommand(String[])} that accepts an initial total
     * execution time for the command.
     * <p/>
     * Useful when transitioning pre-existing commands from another tradefed process
     *
     * @param args the config arguments.
     * @param totalExecTime the accumulated elapsed execution time of the command
     * @return <code>true</code> if command was added successfully
     * @throws ConfigurationException if command was invalid
     */
    public boolean addCommand(String[] args, long totalExecTime) throws ConfigurationException;

    /**
     * Directly allocates a device and executes a command without adding it to the command queue.
     *
     * @param listener the {@link ICommandScheduler.IScheduledInvocationListener} to be informed
     * @param args the command arguments
     *
     * @throws ConfigurationException if command was invalid
     * @throws NoDeviceException if there is no device to use
     */
    public void execCommand(IScheduledInvocationListener listener, String[] args)
            throws ConfigurationException, NoDeviceException;

    /**
     * Directly execute command on already allocated device.
     *
     * @param listener the {@link ICommandScheduler.IScheduledInvocationListener} to be informed
     * @param device the {@link ITestDevice} to use
     * @param args the command arguments
     *
     * @throws ConfigurationException if command was invalid
     */
    public void execCommand(IScheduledInvocationListener listener, ITestDevice device,
            String[] args) throws ConfigurationException;

    /**
     * Directly allocates a device and executes a command without adding it to the command queue
     * using an already existing {@link IInvocationContext}.
     *
     * @param context an existing {@link IInvocationContext}.
     * @param listener the {@link ICommandScheduler.IScheduledInvocationListener} to be informed
     * @param args the command arguments
     * @throws ConfigurationException if command was invalid
     * @throws NoDeviceException if there is no device to use
     */
    public void execCommand(
            IInvocationContext context, IScheduledInvocationListener listener, String[] args)
            throws ConfigurationException, NoDeviceException;

    /**
     * Remove all commands from scheduler
     */
    public void removeAllCommands();

    /**
     * Attempt to gracefully shutdown the command scheduler.
     * <p/>
     * Clears commands waiting to be tested, and requests that all invocations in progress
     * shut down gracefully.
     * <p/>
     * After shutdown is called, the scheduler main loop will wait for all invocations in progress
     * to complete before exiting completely.
     */
    public void shutdown();

    /**
     * Similar to {@link #shutdown()}, but will instead wait for all commands to be executed
     * before exiting.
     * <p/>
     * Note that if any commands are in loop mode, the scheduler will never exit.
     */
    public void shutdownOnEmpty();

    /**
     * Initiates a {@link #shutdown()} and handover to another tradefed process on this same host.
     * <p/>
     * The scheduler will inform the remote tradefed process listening on that port of freed devices
     * as they become available.
     *
     * @return <code>true</code> if handover initiation was successful, <code>false</code>
     * otherwise
     */
    public boolean handoverShutdown(int handoverPort);

    /**
     * Informs the command scheduler that initial handover exchange of devices and commands in use
     * is complete, and it can begin scheduling operation.
     */
    public void handoverInitiationComplete();

    /**
     * Informs the command scheduler that a initiated handover sequence is fully complete, and it
     * should re-initialize its remote manager on the default port.
     */
    public void completeHandover();

    /**
     * Attempt to forcefully shutdown the command scheduler.
     * <p/>
     * Similar to {@link #shutdown()}, but will also forcefully kill the adb connection, in an
     * attempt to 'inspire' invocations in progress to complete quicker.
     */
    public void shutdownHard();

    /**
     * Start the {@link ICommandScheduler}.
     * <p/>
     * Must be called before calling other methods.
     * <p/>
     * Will run until {@link #shutdown()} is called.
     *
     * see {@link Thread#start()}.
     */
    public void start();

    /**
     * Waits for scheduler to complete.
     *
     * @see Thread#join()
     */
    public void join() throws InterruptedException;

    /**
     * Waits for scheduler to complete or timeout after the duration specified in milliseconds.
     *
     * @see Thread#join(long)
     */
    public void join(long millis) throws InterruptedException;

    /**
     * Waits for scheduler to start running, including waiting for handover from old TF to complete
     * if applicable.
     */
    public void await() throws InterruptedException;

    /**
     * Displays a list of current invocations.
     *
     * @param printWriter the {@link PrintWriter} to output to.
     */
    public void displayInvocationsInfo(PrintWriter printWriter);

    /**
     * Stop a running invocation.
     *
     * @return true if the invocation was stopped, false otherwise
     * @throws UnsupportedOperationException if the implementation doesn't support this
     */
    public boolean stopInvocation(ITestInvocation invocation) throws UnsupportedOperationException;

    /**
     * Stop a running invocation by specifying it's id.
     *
     * @return true if the invocation was stopped, false otherwise
     * @throws UnsupportedOperationException if the implementation doesn't support this
     */
    public boolean stopInvocation(int invocationId) throws UnsupportedOperationException;

    /**
     * Return the information on an invocation bu specifying the invocation id.
     *
     * @param invocationId the tracking id of the invocation.
     * @return A {@link String} containing information about the invocation.
     */
    public String getInvocationInfo(int invocationId);

    /**
     * Output a list of current commands.
     *
     * @param printWriter the {@link PrintWriter} to output to.
     * @param regex the regular expression to which commands should be matched in order to be
     * printed.  If null, then all commands will be printed.
     */
    public void displayCommandsInfo(PrintWriter printWriter, String regex);

    /**
     * Dump the expanded xml file for the command with all
     * {@link com.android.tradefed.config.Option} values specified for all current commands.
     *
     * @param printWriter the {@link PrintWriter} to output the status to.
     * @param regex the regular expression to which commands should be matched in order for the
     * xml file to be dumped.  If null, then all commands will be dumped.
     */
    public void dumpCommandsXml(PrintWriter printWriter, String regex);

    /**
     * Output detailed debug info on state of command execution queue.
     *
     * @param printWriter
     */
    public void displayCommandQueue(PrintWriter printWriter);

    /**
     * Get the appropriate {@link CommandFileWatcher} for this scheduler
     */
    public CommandFileWatcher getCommandFileWatcher();

    /**
     * Return true if we need to shutdown the scheduler on a command errors
     */
    public boolean shouldShutdownOnCmdfileError();

    /**
     * Return the error code of the last invocation that ran.
     * Return 0 (no error), if no invocation has ran yet.
     */
    public ExitCode getLastInvocationExitCode();

    /**
     * Return the {@link Throwable} from the last invocation that ran.
     * Return null, if no throwable is available.
     */
    public Throwable getLastInvocationThrowable();

    /**
     * Helper method, when running inside a {@link CommandRunner} context, set an exit error code
     * and a stack trace that can be returned.
     */
    public void setLastInvocationExitCode(ExitCode code, Throwable stack);

    /** Returns the number of Commands in ready state in the queue. */
    public int getReadyCommandCount();
}
