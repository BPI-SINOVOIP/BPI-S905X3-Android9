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

import com.android.ddmlib.DdmPreferences;
import com.android.ddmlib.IDevice;
import com.android.ddmlib.Log;
import com.android.ddmlib.Log.LogLevel;
import com.android.tradefed.command.CommandFileParser.CommandLine;
import com.android.tradefed.command.CommandFileWatcher.ICommandFileListener;
import com.android.tradefed.command.CommandRunner.ExitCode;
import com.android.tradefed.command.remote.DeviceDescriptor;
import com.android.tradefed.command.remote.IRemoteClient;
import com.android.tradefed.command.remote.RemoteClient;
import com.android.tradefed.command.remote.RemoteException;
import com.android.tradefed.command.remote.RemoteManager;
import com.android.tradefed.config.ConfigurationException;
import com.android.tradefed.config.ConfigurationFactory;
import com.android.tradefed.config.GlobalConfiguration;
import com.android.tradefed.config.IConfiguration;
import com.android.tradefed.config.IConfigurationFactory;
import com.android.tradefed.config.IDeviceConfiguration;
import com.android.tradefed.config.IGlobalConfiguration;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.SandboxConfigurationFactory;
import com.android.tradefed.device.DeviceAllocationState;
import com.android.tradefed.device.DeviceManager;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.DeviceUnresponsiveException;
import com.android.tradefed.device.FreeDeviceState;
import com.android.tradefed.device.IDeviceManager;
import com.android.tradefed.device.IDeviceMonitor;
import com.android.tradefed.device.IManagedTestDevice;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.device.ITestDevice.RecoveryMode;
import com.android.tradefed.device.NoDeviceException;
import com.android.tradefed.device.StubDevice;
import com.android.tradefed.device.TestDeviceState;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.invoker.IRescheduler;
import com.android.tradefed.invoker.ITestInvocation;
import com.android.tradefed.invoker.InvocationContext;
import com.android.tradefed.invoker.TestInvocation;
import com.android.tradefed.log.ILogRegistry.EventType;
import com.android.tradefed.log.LogRegistry;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.ResultForwarder;
import com.android.tradefed.sandbox.ISandbox;
import com.android.tradefed.sandbox.TradefedSandbox;
import com.android.tradefed.util.ArrayUtil;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.QuotationAwareTokenizer;
import com.android.tradefed.util.RunUtil;
import com.android.tradefed.util.TableFormatter;
import com.android.tradefed.util.TimeUtil;
import com.android.tradefed.util.hostmetric.IHostMonitor;
import com.android.tradefed.util.hostmetric.IHostMonitor.HostDataPoint;
import com.android.tradefed.util.hostmetric.IHostMonitor.HostMetricType;
import com.android.tradefed.util.keystore.IKeyStoreClient;
import com.android.tradefed.util.keystore.IKeyStoreFactory;
import com.android.tradefed.util.keystore.KeyStoreException;

import com.google.common.annotations.VisibleForTesting;

import org.json.JSONException;

import java.io.File;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.Timer;
import java.util.TimerTask;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ScheduledThreadPoolExecutor;
import java.util.concurrent.TimeUnit;
import java.util.regex.Pattern;

/**
 * A scheduler for running TradeFederation commands across all available devices.
 * <p/>
 * Will attempt to prioritize commands to run based on a total running count of their execution
 * time. e.g. infrequent or fast running commands will get prioritized over long running commands.
 * <p/>
 * Runs forever in background until shutdown.
 */
public class CommandScheduler extends Thread implements ICommandScheduler, ICommandFileListener {

    /** the queue of commands ready to be executed. */
    private List<ExecutableCommand> mReadyCommands;
    private Set<ExecutableCommand> mUnscheduledWarning;

    /** the queue of commands sleeping. */
    private Set<ExecutableCommand> mSleepingCommands;

    /** the queue of commands current executing. */
    private Set<ExecutableCommand> mExecutingCommands;

    /** map of device to active invocation threads */
    private Map<IInvocationContext, InvocationThread> mInvocationThreadMap;

    /** timer for scheduling commands to be re-queued for execution */
    private ScheduledThreadPoolExecutor mCommandTimer;

    private IRemoteClient mRemoteClient = null;
    private RemoteManager mRemoteManager = null;

    private CommandFileWatcher mCommandFileWatcher = null;

    /** latch used to notify other threads that this thread is running */
    private final CountDownLatch mRunLatch;

    /** maximum time to wait for handover initiation to complete */
    private static final long MAX_HANDOVER_INIT_TIME = 2 * 60 * 1000;

    /** Maximum time to wait for adb to initialize and get the physical devices discovered */
    private static final long ADB_INIT_TIME_MS = 500;

    /** used to assign unique ids to each CommandTracker created */
    private int mCurrentCommandId = 0;

    /** flag for instructing scheduler to exit when no commands are present */
    private boolean mShutdownOnEmpty = false;

    private boolean mStarted = false;

    // flag to indicate this scheduler is currently handing over control to another remote TF
    private boolean mPerformingHandover = false;

    private WaitObj mHandoverHandshake = new WaitObj();

    private WaitObj mCommandProcessWait = new WaitObj();

    /** The last {@link InvocationThread} that ran error code and error stack*/
    private ExitCode mLastInvocationExitCode = ExitCode.NO_ERROR;
    private Throwable mLastInvocationThrowable = null;

    @Option(name = "reload-cmdfiles", description =
            "Whether to enable the command file autoreload mechanism")
    // FIXME: enable this to be enabled or disabled on a per-cmdfile basis
    private boolean mReloadCmdfiles = false;

    @Option(
        name = "max-poll-time",
        description = "ms between forced command scheduler execution time"
    )
    private long mPollTime = 30 * 1000; // 30 seconds

    @Option(name = "shutdown-on-cmdfile-error", description =
            "terminate TF session if a configuration exception on command file occurs")
    private boolean mShutdownOnCmdfileError = false;

    @Option(name = "shutdown-delay", description =
            "maximum time to wait before allowing final interruption of scheduler to terminate"
            + " TF session. If value is zero, interruption will only be triggered"
            + " when Invocation become interruptible. (Default behavior).", isTimeVal = true)
    private long mShutdownTimeout = 0;

    private enum CommandState {
        WAITING_FOR_DEVICE("Wait_for_device"),
        EXECUTING("Executing"),
        SLEEPING("Sleeping");

        private final String mDisplayName;

        CommandState(String displayName) {
            mDisplayName = displayName;
        }

        public String getDisplayName() {
            return mDisplayName;
        }
    }

    /**
     * Represents one active command added to the scheduler. Will track total execution time of all
     * instances of this command
     */
     static class CommandTracker {
        private final int mId;
        private final String[] mArgs;
        private final String mCommandFilePath;

        /** the total amount of time this command was executing. Used to prioritize */
        private long mTotalExecTime = 0;

        CommandTracker(int id, String[] args, String commandFilePath) {
            mId = id;
            mArgs = args;
            mCommandFilePath = commandFilePath;
        }

        synchronized void incrementExecTime(long execTime) {
            mTotalExecTime += execTime;
        }

        /**
         * @return the total amount of execution time for this command.
         */
        synchronized long getTotalExecTime() {
            return mTotalExecTime;
        }

        /**
         * Get the full list of config arguments associated with this command.
         */
        String[] getArgs() {
            return mArgs;
        }

        int getId() {
            return mId;
        }

        /**
         * Return the path of the file this command is associated with. null if not applicable
         */
        String getCommandFilePath() {
            return mCommandFilePath;
        }
    }

    /**
     * Represents one instance of a command to be executed.
     */
    private class ExecutableCommand {
        private final CommandTracker mCmdTracker;
        private final IConfiguration mConfig;
        private final boolean mRescheduled;
        private final long mCreationTime;
        private Long mSleepTime;

        private ExecutableCommand(CommandTracker tracker, IConfiguration config,
                boolean rescheduled) {
            mConfig = config;
            mCmdTracker = tracker;
            mRescheduled = rescheduled;
            mCreationTime = System.currentTimeMillis();
        }

        /**
         * Gets the {@link IConfiguration} for this command instance
         */
        public IConfiguration getConfiguration() {
            return mConfig;
        }

        /**
         * Gets the associated {@link CommandTracker}.
         */
        CommandTracker getCommandTracker() {
            return mCmdTracker;
        }

        /**
         * Callback to inform listener that command has started execution.
         */
        void commandStarted() {
            mSleepTime = null;
        }

        public void commandFinished(long elapsedTime) {
            getCommandTracker().incrementExecTime(elapsedTime);
            CLog.d("removing exec command for id %d", getCommandTracker().getId());
            synchronized (CommandScheduler.this) {
                mExecutingCommands.remove(this);
            }
            if (isShuttingDown()) {
                mCommandProcessWait.signalEventReceived();
            }
        }

        public boolean isRescheduled() {
            return mRescheduled;
        }

        public long getCreationTime() {
            return mCreationTime;
        }

        public boolean isLoopMode() {
            return mConfig.getCommandOptions().isLoopMode();
        }

        public Long getSleepTime() {
            return mSleepTime;
        }

        public String getCommandFilePath() {
            return mCmdTracker.getCommandFilePath();
        }
    }

    /**
     * struct for a command and its state
     */
    private static class ExecutableCommandState {
        final ExecutableCommand cmd;
        final CommandState state;

        ExecutableCommandState(ExecutableCommand cmd, CommandState state) {
            this.cmd = cmd;
            this.state = state;
        }
    }

    /**
     * A {@link IRescheduler} that will add a command back to the queue.
     */
    private class Rescheduler implements IRescheduler {

        private CommandTracker mCmdTracker;

        Rescheduler(CommandTracker cmdTracker) {
            mCmdTracker = cmdTracker;
        }

        /**
         * {@inheritDoc}
         */
        @Override
        public boolean scheduleConfig(IConfiguration config) {
            // force loop mode to off, otherwise each rescheduled config will be treated as
            // a new command and added back to queue
            config.getCommandOptions().setLoopMode(false);
            ExecutableCommand rescheduledCmd = createExecutableCommand(mCmdTracker, config, true);
            return addExecCommandToQueue(rescheduledCmd, 0);
        }

        /**
         * {@inheritDoc}
         */
        @Override
        public boolean rescheduleCommand() {
            try {
                CLog.d("rescheduling for command %d", mCmdTracker.getId());
                IConfiguration config = getConfigFactory().createConfigurationFromArgs(
                        mCmdTracker.getArgs());
                ExecutableCommand execCmd = createExecutableCommand(mCmdTracker, config, true);
                return addExecCommandToQueue(execCmd, config.getCommandOptions().getLoopTime());
            } catch (ConfigurationException e) {
                // FIXME: do this with jline somehow for ANSI support
                // note: make sure not to log (aka record) this line, as (args) may contain
                // passwords.
                System.out.println(String.format("Error while processing args: %s",
                        Arrays.toString(mCmdTracker.getArgs())));
                System.out.println(e.getMessage());
                System.out.println();
                return false;
            }
        }
    }

    /**
     * Comparator for {@link ExecutableCommand}.
     * <p/>
     * Delegates to {@link CommandTrackerTimeComparator}.
     */
    private static class ExecutableCommandComparator implements Comparator<ExecutableCommand> {
        CommandTrackerTimeComparator mTrackerComparator = new CommandTrackerTimeComparator();

        /**
         * {@inheritDoc}
         */
        @Override
        public int compare(ExecutableCommand c1, ExecutableCommand c2) {
            return mTrackerComparator.compare(c1.getCommandTracker(), c2.getCommandTracker());
        }
    }

    /**
     * Comparator for {@link CommandTracker}.
     * <p/>
     * Compares by mTotalExecTime, prioritizing configs with lower execution time
     */
    private static class CommandTrackerTimeComparator implements Comparator<CommandTracker> {

        @Override
        public int compare(CommandTracker c1, CommandTracker c2) {
            if (c1.getTotalExecTime() == c2.getTotalExecTime()) {
                return 0;
            } else if (c1.getTotalExecTime() < c2.getTotalExecTime()) {
                return -1;
            } else {
                return 1;
            }
        }
    }

    /**
     * Comparator for {@link CommandTracker}.
     * <p/>
     * Compares by id.
     */
    static class CommandTrackerIdComparator implements Comparator<CommandTracker> {

        @Override
        public int compare(CommandTracker c1, CommandTracker c2) {
            if (c1.getId() == c2.getId()) {
                return 0;
            } else if (c1.getId() < c2.getId()) {
                return -1;
            } else {
                return 1;
            }
        }
    }

    /**
     * An {@link com.android.tradefed.command.ICommandScheduler.IScheduledInvocationListener}
     * for locally scheduled commands added via addCommand.
     * <p/>
     * Returns device to device manager and remote handover server if applicable.
     */
    private class FreeDeviceHandler extends ResultForwarder implements
            IScheduledInvocationListener {

        private final IDeviceManager mDeviceManager;

        FreeDeviceHandler(IDeviceManager deviceManager,
                IScheduledInvocationListener... listeners) {
            super(listeners);
            mDeviceManager = deviceManager;
        }

        @Override
        public void invocationComplete(IInvocationContext context,
                Map<ITestDevice, FreeDeviceState> devicesStates) {
            for (ITestInvocationListener listener : getListeners()) {
                ((IScheduledInvocationListener) listener).invocationComplete(context, devicesStates);
            }

            for (ITestDevice device : context.getDevices()) {
                mDeviceManager.freeDevice(device, devicesStates.get(device));
                remoteFreeDevice(device);
                if (device instanceof IManagedTestDevice) {
                    // This quite an important setting so we do make sure it's reset.
                    ((IManagedTestDevice)device).setFastbootPath(mDeviceManager.getFastbootPath());
                }
            }
        }
    }

    /**
     * Monitor Class for {@link InvocationThread} to make sure it does not run for too long.
     */
    private class InvocationThreadMonitor extends TimerTask {

        private InvocationThread mInvocationThread = null;
        private boolean mTriggered = false;

        public InvocationThreadMonitor(InvocationThread toMonitor) {
            mInvocationThread = toMonitor;
        }

        @Override
        public void run() {
            if (mInvocationThread != null) {
                mTriggered = true;
                mInvocationThread.stopInvocation("Invocation Timeout Reached.");
            }
        }

        public boolean isTriggered() {
            return mTriggered;
        }
    }

    private class InvocationThread extends Thread {
        /**
         * time to wait for device adb shell responsive connection before declaring it unavailable
         * for the next iteration of testing.
         */
        private static final long CHECK_WAIT_DEVICE_AVAIL_MS = 30 * 1000;
        private static final int EXPECTED_THREAD_COUNT = 1;
        private static final String INVOC_END_EVENT_ID_KEY = "id";
        private static final String INVOC_END_EVENT_ELAPSED_KEY = "elapsed-time";
        private static final String INVOC_END_EVENT_TAG_KEY = "test-tag";

        private final IScheduledInvocationListener[] mListeners;
        private final IInvocationContext mInvocationContext;
        private final ExecutableCommand mCmd;
        private final ITestInvocation mInvocation;
        private final InvocationThreadMonitor mInvocationThreadMonitor;
        private final Timer mExecutionTimer;
        private long mStartTime = -1;

        public InvocationThread(String name, IInvocationContext invocationContext,
                ExecutableCommand command, IScheduledInvocationListener... listeners) {
            // create a thread group so LoggerRegistry can identify this as an invocationThread
            super(new ThreadGroup(name), name);
            mListeners = listeners;
            mInvocationContext = invocationContext;
            mCmd = command;
            mInvocation = createRunInstance();

            // Daemon timer
            mExecutionTimer = new Timer(true);
            mInvocationThreadMonitor = new InvocationThreadMonitor(this);
        }

        public long getStartTime() {
            return mStartTime;
        }

        @Override
        public void run() {
            Map<ITestDevice, FreeDeviceState> deviceStates = new HashMap<>();
            for (ITestDevice device : mInvocationContext.getDevices()) {
                deviceStates.put(device, FreeDeviceState.AVAILABLE);
            }
            mStartTime = System.currentTimeMillis();
            ITestInvocation instance = getInvocation();
            IConfiguration config = mCmd.getConfiguration();

            for (final IScheduledInvocationListener listener : mListeners) {
                try {
                    listener.invocationInitiated(mInvocationContext);
                } catch (Throwable anyException) {
                    CLog.e("Exception caught while calling invocationInitiated:");
                    CLog.e(anyException);
                }
            }

            try {
                // Copy the command options invocation attributes to the invocation if it has not
                // been already done.
                if (!config.getConfigurationDescription().shouldUseSandbox()
                        && !config.getCommandOptions().getInvocationData().isEmpty()) {
                    mInvocationContext.addInvocationAttributes(
                            config.getCommandOptions().getInvocationData());
                }
                mCmd.commandStarted();
                long invocTimeout = config.getCommandOptions().getInvocationTimeout();
                if (invocTimeout > 0) {
                    CLog.i("Setting a timer for the invocation in %sms", invocTimeout);
                    mExecutionTimer.schedule(mInvocationThreadMonitor, invocTimeout);
                }
                instance.invoke(mInvocationContext, config,
                        new Rescheduler(mCmd.getCommandTracker()), mListeners);
            } catch (DeviceUnresponsiveException e) {
                CLog.w("Device %s is unresponsive. Reason: %s", e.getSerial(), e.getMessage());
                ITestDevice badDevice = mInvocationContext.getDeviceBySerial(e.getSerial());
                if (badDevice != null) {
                    deviceStates.put(badDevice, FreeDeviceState.UNRESPONSIVE);
                }
                setLastInvocationExitCode(ExitCode.DEVICE_UNRESPONSIVE, e);
            } catch (DeviceNotAvailableException e) {
                CLog.w("Device %s is not available. Reason: %s", e.getSerial(), e.getMessage());
                ITestDevice badDevice = mInvocationContext.getDeviceBySerial(e.getSerial());
                if (badDevice != null) {
                    deviceStates.put(badDevice, FreeDeviceState.UNAVAILABLE);
                }
                setLastInvocationExitCode(ExitCode.DEVICE_UNAVAILABLE, e);
            } catch (FatalHostError e) {
                CLog.wtf(String.format("Fatal error occurred: %s, shutting down", e.getMessage()),
                        e);
                setLastInvocationExitCode(ExitCode.FATAL_HOST_ERROR, e);
                shutdown();
            } catch (Throwable e) {
                setLastInvocationExitCode(ExitCode.THROWABLE_EXCEPTION, e);
                CLog.e(e);
            } finally {
                mExecutionTimer.cancel();
                if (mInvocationThreadMonitor.isTriggered()) {
                    CLog.e("Invocation reached its timeout. Cleaning up.");
                }
                long elapsedTime = System.currentTimeMillis() - mStartTime;
                CLog.i("Updating command %d with elapsed time %d ms",
                       mCmd.getCommandTracker().getId(), elapsedTime);
                // remove invocation thread first so another invocation can be started on device
                // when freed
                removeInvocationThread(this);
                for (ITestDevice device : mInvocationContext.getDevices()) {
                    if (device.getIDevice() instanceof StubDevice) {
                        // Never release stub and Tcp devices, otherwise they will disappear
                        // during deallocation since they are only placeholder.
                        deviceStates.put(device, FreeDeviceState.AVAILABLE);
                    } else if (!TestDeviceState.ONLINE.equals(device.getDeviceState())) {
                        // If the device is offline at the end of the test
                        deviceStates.put(device, FreeDeviceState.UNAVAILABLE);
                    } else if (!isDeviceResponsive(device)) {
                        // If device cannot pass basic shell responsiveness test.
                        deviceStates.put(device, FreeDeviceState.UNAVAILABLE);
                    }
                    // Reset the recovery mode at the end of the invocation.
                    device.setRecoveryMode(RecoveryMode.AVAILABLE);
                }

                checkStrayThreads();

                for (final IScheduledInvocationListener listener : mListeners) {
                    try {
                        listener.invocationComplete(mInvocationContext, deviceStates);
                    } catch (Throwable anyException) {
                        CLog.e("Exception caught while calling invocationComplete:");
                        CLog.e(anyException);
                    }
                }
                mCmd.commandFinished(elapsedTime);
                logInvocationEndedEvent(
                        mCmd.getCommandTracker().getId(), elapsedTime, mInvocationContext);
            }
        }

        /** Check the number of thread in the ThreadGroup, only one should exists (itself). */
        private void checkStrayThreads() {
            int numThread = this.getThreadGroup().activeCount();
            if (numThread == EXPECTED_THREAD_COUNT) {
                // No stray thread detected at the end of invocation
                return;
            }
            List<String> cmd = Arrays.asList(mCmd.getCommandTracker().getArgs());
            CLog.e(
                    "Stray thread detected for command %d, %s. %d threads instead of %d",
                    mCmd.getCommandTracker().getId(), cmd, numThread, EXPECTED_THREAD_COUNT);
            // This is the best we have for debug, it prints to std out.
            Thread[] listThreads = new Thread[numThread];
            this.getThreadGroup().enumerate(listThreads);
            CLog.e("List of remaining threads: %s", Arrays.asList(listThreads));
            List<IHostMonitor> hostMonitors = GlobalConfiguration.getHostMonitorInstances();
            if (hostMonitors != null) {
                for (IHostMonitor hm : hostMonitors) {
                    HostDataPoint data = new HostDataPoint("numThread", numThread, cmd.toString());
                    hm.addHostEvent(HostMetricType.INVOCATION_STRAY_THREAD, data);
                }
            }
            // printing to stderr will help to catch them.
            System.err.println(
                    String.format(
                            "We have %s threads instead of 1: %s. Check the logs for list of "
                                    + "threads.",
                            numThread, Arrays.asList(listThreads)));
        }

        /** Helper to log an invocation ended event. */
        private void logInvocationEndedEvent(
                int invocId, long elapsedTime, final IInvocationContext context) {
            Map<String, String> args = new HashMap<>();
            args.put(INVOC_END_EVENT_ID_KEY, Integer.toString(invocId));
            args.put(INVOC_END_EVENT_ELAPSED_KEY, TimeUtil.formatElapsedTime(elapsedTime));
            args.put(INVOC_END_EVENT_TAG_KEY, context.getTestTag());
            // Add all the invocation attributes to the final event logging.
            // UniqueMultiMap cannot be easily converted to Map so we just iterate.
            for (String key : context.getAttributes().keySet()) {
                args.put(key, context.getAttributes().get(key).get(0));
            }
            logEvent(EventType.INVOCATION_END, args);
        }

        /** Basic responsiveness check at the end of an invocation. */
        private boolean isDeviceResponsive(ITestDevice device) {
            return device.waitForDeviceShell(CHECK_WAIT_DEVICE_AVAIL_MS);
        }

        ITestInvocation getInvocation() {
            return mInvocation;
        }

        IInvocationContext getInvocationContext() {
            return mInvocationContext;
        }

        /**
         * Stops a running invocation. {@link CommandScheduler#shutdownHard()} will stop
         * all running invocations.
         */
        public void stopInvocation(String message) {
            getInvocation().notifyInvocationStopped();
            for (ITestDevice device : mInvocationContext.getDevices()) {
                if (device != null && device.getIDevice().isOnline()) {
                    // Kill all running processes on device.
                    try {
                        device.executeShellCommand("am kill-all");
                    } catch (DeviceNotAvailableException e) {
                        CLog.e("failed to kill process on device %s",
                                device.getSerialNumber());
                        CLog.e(e);
                    }

                }
            }
            // If invocation is not currently in an interruptible state we provide a timer
            // after which it will become interruptible.
            // If timeout is 0, we do not enforce future interruption.
            if (!mInvocationThreadMonitor.isTriggered() && getShutdownTimeout() != 0) {
                RunUtil.getDefault().setInterruptibleInFuture(this, getShutdownTimeout());
            }
            RunUtil.getDefault().interrupt(this, message);

            if (mInvocationThreadMonitor.isTriggered()) {
                // if we enforce the invocation timeout, we force interrupt the thread.
                CLog.e("Forcing the interruption.");
                this.interrupt();
            }
        }

        /**
         * Disable the reporting from reporters that implements a non-default
         * {@link ITestInvocationListener#invocationInterrupted()}.
         * Should be called on shutdown.
         */
        public void disableReporters() {
            for (ITestInvocationListener listener :
                    mCmd.getConfiguration().getTestInvocationListeners()) {
                listener.invocationInterrupted();
            }
        }

        /**
         * Checks whether the device battery level is above the required value to keep running the
         * invocation.
         */
        public void checkDeviceBatteryLevel() {
            for (String deviceName : mInvocationContext.getDeviceConfigNames()) {
                if (mCmd.getConfiguration().getDeviceConfigByName(deviceName)
                        .getDeviceOptions() == null) {
                    CLog.d("No deviceOptions in the configuration, cannot do Battery level check");
                    return;
                }
                final Integer cutoffBattery = mCmd.getConfiguration()
                        .getDeviceConfigByName(deviceName).getDeviceOptions().getCutoffBattery();

                if (mInvocationContext.getDevice(deviceName) != null && cutoffBattery != null) {
                    final IDevice device = mInvocationContext.getDevice(deviceName).getIDevice();
                    int batteryLevel = -1;
                    try {
                        batteryLevel = device.getBattery(500, TimeUnit.MILLISECONDS).get();
                    } catch (InterruptedException | ExecutionException e) {
                        // fall through
                    }
                    CLog.d("device %s: battery level=%d%%", device.getSerialNumber(), batteryLevel);
                    // This logic is based on the assumption that batterLevel will be 0 or -1 if TF
                    // fails to fetch a valid battery level or the device is not using a battery.
                    // So batteryLevel=0 will not trigger a stop.
                    if (0 < batteryLevel && batteryLevel < cutoffBattery) {
                        if (RunUtil.getDefault().isInterruptAllowed()) {
                            CLog.i("Stopping %s: battery too low (%d%% < %d%%)",
                                    getName(), batteryLevel, cutoffBattery);
                            stopInvocation(String.format(
                                    "battery too low (%d%% < %d%%)", batteryLevel, cutoffBattery));
                        } else {
                            // In this case, the battery is check periodically by CommandScheduler
                            // so there will be more opportunity to terminate the invocation when
                            // it's interruptible.
                            CLog.w("device: %s has a low battery but is in uninterruptible state.",
                                    device.getSerialNumber());
                        }
                    }
                }
            }
        }
    }

    /**
     * A {@link IDeviceMonitor} that signals scheduler to process commands when an available device
     * is added.
     */
    private class AvailDeviceMonitor implements IDeviceMonitor {

        @Override
        public void run() {
           // ignore
        }

        @Override
        public void stop() {
            // ignore
        }

        @Override
        public void setDeviceLister(DeviceLister lister) {
            // ignore
        }

        @Override
        public void notifyDeviceStateChange(String serial, DeviceAllocationState oldState,
                DeviceAllocationState newState) {
            if (newState.equals(DeviceAllocationState.Available)) {
                // new avail device was added, wake up scheduler
                mCommandProcessWait.signalEventReceived();
            }
        }
    }

    /**
     * Creates a {@link CommandScheduler}.
     * <p />
     * Note: start must be called before use.
     */
    public CommandScheduler() {
        super("CommandScheduler");  // set the thread name
        mReadyCommands = new LinkedList<>();
        mUnscheduledWarning = new HashSet<>();
        mSleepingCommands = new HashSet<>();
        mExecutingCommands = new HashSet<>();
        mInvocationThreadMap = new HashMap<IInvocationContext, InvocationThread>();
        // use a ScheduledThreadPoolExecutorTimer as a single-threaded timer. This class
        // is used instead of a java.util.Timer because it offers advanced shutdown options
        mCommandTimer = new ScheduledThreadPoolExecutor(1);
        mRunLatch = new CountDownLatch(1);
    }

    /**
     * Starts the scheduler including setting up of logging, init of {@link DeviceManager} etc
     */
    @Override
    public void start() {
        synchronized (this) {
            if (mStarted) {
                throw new IllegalStateException("scheduler has already been started");
            }
            initLogging();

            initDeviceManager();

            mStarted = true;
        }
        super.start();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public synchronized CommandFileWatcher getCommandFileWatcher() {
        assertStarted();
        if (mCommandFileWatcher == null) {
            mCommandFileWatcher = new CommandFileWatcher(this);
            mCommandFileWatcher.start();
        }
        return mCommandFileWatcher;
    }

    /**
     * Initialize the device manager, optionally using a global device filter if specified.
     */
    void initDeviceManager() {
        getDeviceManager().init();
        if (getDeviceManager().waitForFirstDeviceAdded(ADB_INIT_TIME_MS)) {
            // If a first device is added we wait a short extra time to allow more devices to be
            // discovered.
            RunUtil.getDefault().sleep(ADB_INIT_TIME_MS);
        }
    }

    /**
     * Factory method for creating a {@link TestInvocation}.
     *
     * @return the {@link ITestInvocation} to use
     */
    ITestInvocation createRunInstance() {
        return new TestInvocation();
    }

    /**
     * Factory method for getting a reference to the {@link IDeviceManager}
     *
     * @return the {@link IDeviceManager} to use
     */
    protected IDeviceManager getDeviceManager() {
        return GlobalConfiguration.getDeviceManagerInstance();
    }

     /**
      * Factory method for getting a reference to the {@link IHostMonitor}
      *
      * @return the {@link IHostMonitor} to use
      */
     List<IHostMonitor> getHostMonitor() {
         return GlobalConfiguration.getHostMonitorInstances();
     }

    /**
     * Factory method for getting a reference to the {@link IConfigurationFactory}
     *
     * @return the {@link IConfigurationFactory} to use
     */
    protected IConfigurationFactory getConfigFactory() {
        return ConfigurationFactory.getInstance();
    }

    /**
     * Fetches a {@link IKeyStoreClient} using the {@link IKeyStoreFactory}
     * declared in {@link IGlobalConfiguration} or null if none is defined.
     * @return IKeyStoreClient
     */
    protected IKeyStoreClient getKeyStoreClient() {
       try {
           IKeyStoreFactory f = GlobalConfiguration.getInstance().getKeyStoreFactory();
           if (f != null) {
               try {
                  return f.createKeyStoreClient();
               } catch (KeyStoreException e) {
                   CLog.e("Failed to create key store client");
                   CLog.e(e);
               }
           }
       } catch (IllegalStateException e) {
           CLog.w("Global configuration has not been created, failed to get keystore");
           CLog.e(e);
       }
       return null;
    }

    /**
     * The main execution block of this thread.
     */
    @Override
    public void run() {
        assertStarted();
        try {
            IDeviceManager manager = getDeviceManager();

            startRemoteManager();

            // Notify other threads that we're running.
            mRunLatch.countDown();

            // add a listener that will wake up scheduler when a new avail device is added
            manager.addDeviceMonitor(new AvailDeviceMonitor());

            while (!isShutdown()) {
                // wait until processing is required again
                mCommandProcessWait.waitAndReset(mPollTime);
                checkInvocations();
                processReadyCommands(manager);
                postProcessReadyCommands();
            }
            mCommandTimer.shutdown();
            // We signal the device manager to stop device recovery threads because it could
            // potentially create more invocations.
            manager.terminateDeviceRecovery();
            manager.terminateDeviceMonitor();
            CLog.i("Waiting for invocation threads to complete");
            waitForAllInvocationThreads();
            closeRemoteClient();
            if (mRemoteManager != null) {
                mRemoteManager.cancelAndWait();
            }
            exit(manager);
            cleanUp();
            CLog.logAndDisplay(LogLevel.INFO, "All done");
        } finally {
            // Make sure that we don't quit with messages still in the buffers
            System.err.flush();
            System.out.flush();
        }
    }

    /**
     * Placeholder method within the scheduler main loop, called after {@link
     * #processReadyCommands(IDeviceManager)}. Default implementation is empty and does not provide
     * any extra actions.
     */
    protected void postProcessReadyCommands() {}

    void checkInvocations() {
        CLog.d("Checking invocations...");
        final List<InvocationThread> copy;
        synchronized(this) {
            copy = new ArrayList<InvocationThread>(mInvocationThreadMap.values());
        }
        for (InvocationThread thread : copy) {
            thread.checkDeviceBatteryLevel();
        }
    }

    protected void processReadyCommands(IDeviceManager manager) {
        CLog.d("processReadyCommands...");
        Map<ExecutableCommand, IInvocationContext> scheduledCommandMap = new HashMap<>();
        // minimize length of synchronized block by just matching commands with device first,
        // then scheduling invocations/adding looping commands back to queue
        synchronized (this) {
            // sort ready commands by priority, so high priority commands are matched first
            Collections.sort(mReadyCommands, new ExecutableCommandComparator());
            Iterator<ExecutableCommand> cmdIter = mReadyCommands.iterator();
            while (cmdIter.hasNext()) {
                ExecutableCommand cmd = cmdIter.next();
                IConfiguration config = cmd.getConfiguration();
                IInvocationContext context = new InvocationContext();
                context.setConfigurationDescriptor(config.getConfigurationDescription());
                Map<String, ITestDevice> devices = allocateDevices(config, manager);
                if (!devices.isEmpty()) {
                    cmdIter.remove();
                    mExecutingCommands.add(cmd);
                    context.addAllocatedDevice(devices);

                    // track command matched with device
                    scheduledCommandMap.put(cmd, context);
                    // clean warned list to avoid piling over time.
                    mUnscheduledWarning.remove(cmd);
                } else {
                    if (!mUnscheduledWarning.contains(cmd)) {
                        CLog.logAndDisplay(LogLevel.DEBUG, "No available device matching all the "
                                + "config's requirements for cmd id %d.",
                                cmd.getCommandTracker().getId());
                        // make sure not to record since it may contains password
                        System.out.println(
                                String.format(
                                        "The command %s will be rescheduled.",
                                        Arrays.toString(cmd.getCommandTracker().getArgs())));
                        mUnscheduledWarning.add(cmd);
                    }
                }
            }
        }

        // now actually execute the commands
        for (Map.Entry<ExecutableCommand, IInvocationContext> cmdDeviceEntry : scheduledCommandMap
                .entrySet()) {
            ExecutableCommand cmd = cmdDeviceEntry.getKey();
            startInvocation(cmdDeviceEntry.getValue(), cmd,
                    new FreeDeviceHandler(getDeviceManager()));
            if (cmd.isLoopMode()) {
                addNewExecCommandToQueue(cmd.getCommandTracker());
            }
        }
        CLog.d("done processReadyCommands...");
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void await() throws InterruptedException {
        while (mRunLatch.getCount() > 0) {
            mRunLatch.await();
        }
    }

    private void closeRemoteClient() {
        if (mRemoteClient != null) {
            try {
                mRemoteClient.sendHandoverComplete();
                mRemoteClient.close();
            } catch (RemoteException e) {
                CLog.e(e);
            }
        }
    }

    private void waitForThread(Thread thread) {
        try {
            thread.join();
        } catch (InterruptedException e) {
            // ignore
            waitForThread(thread);
        }
    }

    /** Wait until all invocation threads complete. */
    protected void waitForAllInvocationThreads() {
        List<InvocationThread> threadListCopy;
        synchronized (this) {
            threadListCopy = new ArrayList<InvocationThread>(mInvocationThreadMap.size());
            threadListCopy.addAll(mInvocationThreadMap.values());
        }
        for (Thread thread : threadListCopy) {
            waitForThread(thread);
        }
    }

    private void exit(IDeviceManager manager) {
        if (manager != null) {
            manager.terminate();
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean addCommand(String[] args) throws ConfigurationException {
        return addCommand(args, 0);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean addCommand(String[] args, long totalExecTime) throws ConfigurationException {
        return internalAddCommand(args, totalExecTime, null);
    }

    /** Returns true if {@link CommandOptions#USE_SANDBOX} is part of the command line. */
    private boolean isCommandSandboxed(String[] args) {
        for (String arg : args) {
            if (("--" + CommandOptions.USE_SANDBOX).equals(arg)) {
                return true;
            }
        }
        return false;
    }

    /** Create a {@link ISandbox} that the invocation will use to run. */
    public ISandbox createSandbox() {
        return new TradefedSandbox();
    }

    private IConfiguration createConfiguration(String[] args) throws ConfigurationException {
        // check if the command should be sandboxed
        if (isCommandSandboxed(args)) {
            // Create an sandboxed configuration based on the sandbox of the scheduler.
            ISandbox sandbox = createSandbox();
            return SandboxConfigurationFactory.getInstance()
                    .createConfigurationFromArgs(args, getKeyStoreClient(), sandbox, new RunUtil());
        }
        return getConfigFactory().createConfigurationFromArgs(args, null, getKeyStoreClient());
    }

    private boolean internalAddCommand(String[] args, long totalExecTime, String cmdFilePath)
            throws ConfigurationException {
        assertStarted();
        IConfiguration config = createConfiguration(args);
        if (config.getCommandOptions().isHelpMode()) {
            getConfigFactory().printHelpForConfig(args, true, System.out);
        } else if (config.getCommandOptions().isFullHelpMode()) {
            getConfigFactory().printHelpForConfig(args, false, System.out);
        } else if (config.getCommandOptions().isJsonHelpMode()) {
            try {
                // Convert the JSON usage to a string (with 4 space indentation) and print to stdout
                System.out.println(config.getJsonCommandUsage().toString(4));
            } catch (JSONException e) {
                CLog.logAndDisplay(LogLevel.ERROR, "Failed to get json command usage: %s", e);
            }
        } else if (config.getCommandOptions().isDryRunMode()) {
            config.validateOptions();
            String cmdLine = QuotationAwareTokenizer.combineTokens(args);
            CLog.d("Dry run mode; skipping adding command: %s", cmdLine);
            if (config.getCommandOptions().isNoisyDryRunMode()) {
                System.out.println(cmdLine.replace("--noisy-dry-run", ""));
                System.out.println("");
            }
        } else {
            config.validateOptions();

            if (config.getCommandOptions().runOnAllDevices()) {
                addCommandForAllDevices(totalExecTime, args, cmdFilePath);
            } else {
                CommandTracker cmdTracker = createCommandTracker(args, cmdFilePath);
                cmdTracker.incrementExecTime(totalExecTime);
                ExecutableCommand cmdInstance = createExecutableCommand(cmdTracker, config, false);
                addExecCommandToQueue(cmdInstance, 0);
            }
            return true;
        }
        return false;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void addCommandFile(String cmdFilePath, List<String> extraArgs)
            throws ConfigurationException {
        // verify we aren't already watching this command file, don't want to add it twice!
        File cmdFile = new File(cmdFilePath);
        if (mReloadCmdfiles && getCommandFileWatcher().isFileWatched(cmdFile)) {
            CLog.logAndDisplay(LogLevel.INFO,
                    "cmd file %s is already running and being watched for changes. Reloading",
                    cmdFilePath);
            removeCommandsFromFile(cmdFile);
        }
        internalAddCommandFile(cmdFile, extraArgs);
    }

    /**
     * Adds a command file without verifying if its already being watched
     */
    private void internalAddCommandFile(File cmdFile, List<String> extraArgs)
            throws ConfigurationException {
        try {
            CommandFileParser parser = createCommandFileParser();

            List<CommandLine> commands = parser.parseFile(cmdFile);
            if (mReloadCmdfiles) {
                // note always should re-register for command file, even if already listening,
                // since the dependent file list might have changed
                getCommandFileWatcher().addCmdFile(cmdFile, extraArgs, parser.getIncludedFiles());
            }
            for (CommandLine command : commands) {
                command.addAll(extraArgs);
                String[] arrayCommand = command.asArray();
                final String prettyCmdLine = QuotationAwareTokenizer.combineTokens(arrayCommand);
                CLog.d("Adding command %s", prettyCmdLine);

                try {
                    internalAddCommand(arrayCommand, 0, cmdFile.getAbsolutePath());
                } catch (ConfigurationException e) {
                    throw new ConfigurationException(String.format(
                            "Failed to add command '%s': %s", prettyCmdLine, e.getMessage()), e);
                }
            }
        } catch (IOException e) {
            throw new ConfigurationException("Failed to read file " + cmdFile.getAbsolutePath(), e);
        }
    }

    /**
     * Factory method for creating a {@link CommandFileParser}.
     *
     * <p>Exposed for unit testing.
     */
    CommandFileParser createCommandFileParser() {
        return new CommandFileParser();
    }

    /**
     * Creates a new command for each connected device, and adds each to the queue.
     * <p/>
     * Note this won't have the desired effect if user has specified other
     * conflicting {@link IConfiguration#getDeviceRequirements()}in the command.
     */
    private void addCommandForAllDevices(long totalExecTime, String[] args, String cmdFilePath)
            throws ConfigurationException {
        List<DeviceDescriptor> deviceDescs = getDeviceManager().listAllDevices();

        for (DeviceDescriptor deviceDesc : deviceDescs) {
            if (!deviceDesc.isStubDevice()) {
                String device = deviceDesc.getSerial();
                String[] argsWithDevice = Arrays.copyOf(args, args.length + 2);
                argsWithDevice[argsWithDevice.length - 2] = "-s";
                argsWithDevice[argsWithDevice.length - 1] = device;
                CommandTracker cmdTracker = createCommandTracker(argsWithDevice, cmdFilePath);
                cmdTracker.incrementExecTime(totalExecTime);
                IConfiguration config = getConfigFactory().createConfigurationFromArgs(
                        cmdTracker.getArgs(), null, getKeyStoreClient());
                CLog.logAndDisplay(LogLevel.INFO, "Scheduling '%s' on '%s'",
                        cmdTracker.getArgs()[0], device);
                config.getDeviceRequirements().setSerial(device);
                ExecutableCommand execCmd = createExecutableCommand(cmdTracker, config, false);
                addExecCommandToQueue(execCmd, 0);
            }
        }
    }

    /**
     * Creates a new {@link CommandTracker} with a unique id.
     */
    private synchronized CommandTracker createCommandTracker(String[] args,
            String commandFilePath) {
        mCurrentCommandId++;
        CLog.d("Creating command tracker id %d for command args: '%s'", mCurrentCommandId,
                ArrayUtil.join(" ", (Object[])args));
        return new CommandTracker(mCurrentCommandId, args, commandFilePath);
    }

    /**
     * Creates a new {@link ExecutableCommand}.
     */
    private ExecutableCommand createExecutableCommand(CommandTracker cmdTracker,
            IConfiguration config, boolean rescheduled) {
        ExecutableCommand cmd = new ExecutableCommand(cmdTracker, config, rescheduled);
        CLog.d("creating exec command for id %d", cmdTracker.getId());
        return cmd;
    }

    /**
     * Creates a new {@link ExecutableCommand}, and adds it to queue
     *
     * @param commandTracker
     */
    private void addNewExecCommandToQueue(CommandTracker commandTracker) {
        try {
            IConfiguration config = createConfiguration(commandTracker.getArgs());
            ExecutableCommand execCmd = createExecutableCommand(commandTracker, config, false);
            addExecCommandToQueue(execCmd, config.getCommandOptions().getLoopTime());
        } catch (ConfigurationException e) {
            CLog.e(e);
        }
    }

    /**
     * Adds executable command instance to queue, with optional delay.
     *
     * @param cmd the {@link ExecutableCommand} to return to queue
     * @param delayTime the time in ms to delay before adding command to queue
     * @return <code>true</code> if command will be added to queue, <code>false</code> otherwise
     */
    private synchronized boolean addExecCommandToQueue(final ExecutableCommand cmd,
            long delayTime) {
        if (isShutdown()) {
            return false;
        }
        if (delayTime > 0) {
            mSleepingCommands.add(cmd);
            // delay before making command active
            Runnable delayCommand = new Runnable() {
                @Override
                public void run() {
                    synchronized (CommandScheduler.this) {
                        if (mSleepingCommands.remove(cmd)) {
                            mReadyCommands.add(cmd);
                            mCommandProcessWait.signalEventReceived();
                        }
                    }
                }
            };
            mCommandTimer.schedule(delayCommand, delayTime, TimeUnit.MILLISECONDS);
        } else {
            mReadyCommands.add(cmd);
            mCommandProcessWait.signalEventReceived();
        }
        return true;
    }

    /**
     * Helper method to return an array of {@link String} elements as a readable {@link String}
     *
     * @param args the {@link String}[] to use
     * @return a display friendly {@link String} of args contents
     */
    private String getArgString(String[] args) {
        return ArrayUtil.join(" ", (Object[])args);
    }

    /** {@inheritDoc} */
    @Override
    public void execCommand(
            IInvocationContext context, IScheduledInvocationListener listener, String[] args)
            throws ConfigurationException, NoDeviceException {
        assertStarted();
        IDeviceManager manager = getDeviceManager();
        CommandTracker cmdTracker = createCommandTracker(args, null);
        IConfiguration config = createConfiguration(cmdTracker.getArgs());
        config.validateOptions();

        ExecutableCommand execCmd = createExecutableCommand(cmdTracker, config, false);
        context.setConfigurationDescriptor(config.getConfigurationDescription());
        Map<String, ITestDevice> devices = allocateDevices(config, manager);
        if (!devices.isEmpty()) {
            context.addAllocatedDevice(devices);
            synchronized (this) {
                mExecutingCommands.add(execCmd);
            }
            CLog.d("Executing '%s' on '%s'", cmdTracker.getArgs()[0], devices);
            startInvocation(context, execCmd, listener, new FreeDeviceHandler(manager));
        } else {
            throw new NoDeviceException(
                    "no devices is available for command: " + Arrays.asList(args));
        }
    }

    /** {@inheritDoc} */
    @Override
    public void execCommand(IScheduledInvocationListener listener, String[] args)
            throws ConfigurationException, NoDeviceException {
        execCommand(new InvocationContext(), listener, args);
    }

    /**
     * Allocate devices for a config.
     * @param config a {@link IConfiguration} has device requirements.
     * @param manager a {@link IDeviceManager}
     * @return allocated devices
     */
    Map<String, ITestDevice> allocateDevices(IConfiguration config, IDeviceManager manager) {
        Map<String, ITestDevice> devices = new LinkedHashMap<String, ITestDevice>();
        ITestDevice device = null;
        synchronized(this) {
            if (!config.getDeviceConfig().isEmpty()) {
                for (IDeviceConfiguration deviceConfig : config.getDeviceConfig()) {
                    device = manager.allocateDevice(deviceConfig.getDeviceRequirements());
                    if (device != null) {
                        devices.put(deviceConfig.getDeviceName(), device);
                    } else {
                        // If one of the several device cannot be allocated, we de-allocate
                        // all the previous one.
                        for (ITestDevice allocatedDevice : devices.values()) {
                            FreeDeviceState deviceState = FreeDeviceState.AVAILABLE;
                            if (allocatedDevice.getIDevice() instanceof StubDevice) {
                                deviceState = FreeDeviceState.AVAILABLE;
                            } else if (!TestDeviceState.ONLINE.equals(
                                    allocatedDevice.getDeviceState())) {
                                // If the device is offline at the end of the test
                                deviceState = FreeDeviceState.UNAVAILABLE;
                            }
                            manager.freeDevice(allocatedDevice, deviceState);
                        }
                        // Could not allocate all devices
                        devices.clear();
                        break;
                    }
                }
            }
            return devices;
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void execCommand(IScheduledInvocationListener listener, ITestDevice device,
            String[] args) throws ConfigurationException {
        // TODO: add support for execCommand multi-device allocation
        assertStarted();
        CommandTracker cmdTracker = createCommandTracker(args, null);
        IConfiguration config = getConfigFactory().createConfigurationFromArgs(
                cmdTracker.getArgs(), null, getKeyStoreClient());
        config.validateOptions();
        CLog.i("Executing '%s' on '%s'", cmdTracker.getArgs()[0], device.getSerialNumber());
        ExecutableCommand execCmd = createExecutableCommand(cmdTracker, config, false);
        if (config.getDeviceConfig().size() > 1) {
            throw new RuntimeException("execCommand assume single device invocation.");
        }

        synchronized(this) {
            mExecutingCommands.add(execCmd);
        }
        IInvocationContext context = createInvocationContext();
        context.setConfigurationDescriptor(config.getConfigurationDescription());
        context.addAllocatedDevice(config.getDeviceConfig().get(0).getDeviceName(), device);
        startInvocation(context, execCmd, listener);
    }

    @VisibleForTesting
    protected IInvocationContext createInvocationContext() {
        return new InvocationContext();
    }

    /** Optional initialization step before test invocation starts */
    protected void initInvocation() {}

    /**
     * Spawns off thread to run invocation for given device.
     *
     * @param context the {@link IInvocationContext}
     * @param cmd the {@link ExecutableCommand} to execute
     * @param listeners the {@link
     *     com.android.tradefed.command.ICommandScheduler.IScheduledInvocationListener}s to invoke
     *     when complete
     */
    private void startInvocation(
            IInvocationContext context,
            ExecutableCommand cmd,
            IScheduledInvocationListener... listeners) {
        initInvocation();

        // Check if device is not used in another invocation.
        throwIfDeviceInInvocationThread(context.getDevices());

        CLog.d("starting invocation for command id %d", cmd.getCommandTracker().getId());
        // Name invocation with first device serial
        final String invocationName = String.format("Invocation-%s",
                context.getSerials().get(0));
        InvocationThread invocationThread = new InvocationThread(invocationName, context, cmd,
                listeners);
        logInvocationStartedEvent(cmd.getCommandTracker(), context);
        invocationThread.start();
        addInvocationThread(invocationThread);
    }

    /** Helper to log an invocation started event. */
    private void logInvocationStartedEvent(CommandTracker tracker, IInvocationContext context) {
        Map<String, String> args = new HashMap<>();
        args.put("id", Integer.toString(tracker.getId()));
        int i = 0;
        for (String serial : context.getSerials()) {
            args.put("serial" + i, serial);
            i++;
        }
        args.put("args", ArrayUtil.join(" ", Arrays.asList(tracker.getArgs())));
        logEvent(EventType.INVOCATION_START, args);
    }

    /** Removes a {@link InvocationThread} from the active list. */
    private synchronized void removeInvocationThread(InvocationThread invThread) {
        mInvocationThreadMap.remove(invThread.getInvocationContext());
    }

    private synchronized void throwIfDeviceInInvocationThread(List<ITestDevice> devices) {
        for (ITestDevice device : devices) {
            for (IInvocationContext context : mInvocationThreadMap.keySet()) {
                if (context.getDevices().contains(device)) {
                    throw new IllegalStateException(
                            String.format(
                                    "Attempting invocation on device %s when one is already "
                                            + "running",
                                    device.getSerialNumber()));
                }
            }
        }
    }

    /**
     * Adds a {@link InvocationThread} to the active list.
     */
    private synchronized void addInvocationThread(InvocationThread invThread) {
        mInvocationThreadMap.put(invThread.getInvocationContext(), invThread);
    }

    protected synchronized boolean isShutdown() {
        return mCommandTimer.isShutdown() || (mShutdownOnEmpty && getAllCommandsSize() == 0);
    }

    protected synchronized boolean isShuttingDown() {
        return mCommandTimer.isShutdown() || mShutdownOnEmpty;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public synchronized void shutdown() {
        assertStarted();
        if (!isShuttingDown()) {
            CLog.d("initiating shutdown");
            removeAllCommands();
            if (mCommandFileWatcher != null) {
                mCommandFileWatcher.cancel();
            }
            if (mCommandTimer != null) {
                mCommandTimer.shutdownNow();
            }
            mCommandProcessWait.signalEventReceived();
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public synchronized void shutdownOnEmpty() {
        assertStarted();
        if (!isShuttingDown()) {
            CLog.d("initiating shutdown on empty");
            mShutdownOnEmpty = true;
            mCommandProcessWait.signalEventReceived();
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public synchronized void removeAllCommands() {
        assertStarted();
        CLog.d("removing all commands");
        if (mReloadCmdfiles) {
            getCommandFileWatcher().removeAllFiles();
        }
        if (mCommandTimer != null) {
            for (Runnable task : mCommandTimer.getQueue()) {
                mCommandTimer.remove(task);
            }
        }
        mReadyCommands.clear();
        mSleepingCommands.clear();
        if (isShuttingDown()) {
            mCommandProcessWait.signalEventReceived();
        }
    }

    /**
     * Remove commands originally added via the given command file
     * @param cmdFile
     */
    private synchronized void removeCommandsFromFile(File cmdFile) {
        Iterator<ExecutableCommand> cmdIter = mReadyCommands.iterator();
        while (cmdIter.hasNext()) {
            ExecutableCommand cmd = cmdIter.next();
            String path = cmd.getCommandFilePath();
            if (path != null &&
                    path.equals(cmdFile.getAbsolutePath())) {
                cmdIter.remove();
            }
        }
        cmdIter = mSleepingCommands.iterator();
        while (cmdIter.hasNext()) {
            ExecutableCommand cmd = cmdIter.next();
            String path = cmd.getCommandFilePath();
            if (path != null &&
                    path.equals(cmdFile.getAbsolutePath())) {
                cmdIter.remove();
            }
        }
        if (isShuttingDown()) {
            mCommandProcessWait.signalEventReceived();
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public synchronized boolean handoverShutdown(int handoverPort) {
        assertStarted();
        if (mRemoteClient != null || mPerformingHandover) {
            CLog.e("A handover has already been initiated");
            return false;
        }
        mPerformingHandover = true;
        try {
            mRemoteClient = RemoteClient.connect(handoverPort);
            CLog.d("Connected to remote manager at %d", handoverPort);
            handoverDevices(mRemoteClient);
            CLog.i("Done with device handover.");
            mRemoteClient.sendHandoverInitComplete();
            shutdown();
            return true;
        } catch (RemoteException e) {
            CLog.e(e);
            // TODO: reset state and recover
        }
        return false;
    }

    /** Informs remote manager of the physical devices we are still using. */
    private void handoverDevices(IRemoteClient client) throws RemoteException {
        for (DeviceDescriptor deviceDesc : getDeviceManager().listAllDevices()) {
            if (DeviceAllocationState.Allocated.equals(deviceDesc.getState())
                    && !deviceDesc.isStubDevice()) {
                client.sendAllocateDevice(deviceDesc.getSerial());
                CLog.d("Sent filter device %s command", deviceDesc.getSerial());
            }
        }
    }

    /**
     * @return the list of active {@link CommandTracker}. 'Active' here means all commands added
     * to the scheduler that are either executing, waiting for a device to execute on, or looping.
     */
    List<CommandTracker> getCommandTrackers() {
        List<ExecutableCommandState> cmdCopy = getAllCommands();
        Set<CommandTracker> cmdTrackers = new LinkedHashSet<CommandTracker>();
        for (ExecutableCommandState cmdState : cmdCopy) {
            cmdTrackers.add(cmdState.cmd.getCommandTracker());
        }
        return new ArrayList<CommandTracker>(cmdTrackers);
    }

    /**
     * Inform the remote listener of the freed device. Has no effect if there is no remote listener.
     *
     * @param device the freed {@link ITestDevice}
     */
    private void remoteFreeDevice(ITestDevice device) {
        // TODO: send freed device state too
        if (mPerformingHandover && mRemoteClient != null) {
            try {
                mRemoteClient.sendFreeDevice(device.getSerialNumber());
            } catch (RemoteException e) {
                CLog.e("Failed to send unfilter device %s to remote manager",
                        device.getSerialNumber());
                CLog.e(e);
                // TODO: send handover failed op?
            }
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public synchronized void shutdownHard() {
        shutdown();

        CLog.logAndDisplay(LogLevel.WARN, "Stopping invocation threads...");
        for (InvocationThread thread : mInvocationThreadMap.values()) {
            thread.disableReporters();
            thread.stopInvocation("TF is shutting down");
        }
        getDeviceManager().terminateHard();
    }

    /**
     * Initializes the ddmlib log.
     *
     * <p>Exposed so unit tests can mock.
     */
    @SuppressWarnings("deprecation")
    protected void initLogging() {
        DdmPreferences.setLogLevel(LogLevel.VERBOSE.getStringValue());
        Log.setLogOutput(LogRegistry.getLogRegistry());
    }

    /**
     * Closes the logs and does any other necessary cleanup before we quit.
     *
     * <p>Exposed so unit tests can mock.
     */
    protected void cleanUp() {
        LogRegistry.getLogRegistry().closeAndRemoveAllLogs();
    }

    /** log an event to the registry history logger. */
    @VisibleForTesting
    void logEvent(EventType event, Map<String, String> args) {
        LogRegistry.getLogRegistry().logEvent(LogLevel.DEBUG, event, args);
    }

    /** {@inheritDoc} */
    @Override
    public void displayInvocationsInfo(PrintWriter printWriter) {
        assertStarted();
        if (mInvocationThreadMap == null || mInvocationThreadMap.size() == 0) {
            return;
        }
        List<InvocationThread> copy = new ArrayList<InvocationThread>();
        synchronized(this) {
            copy.addAll(mInvocationThreadMap.values());
        }
        ArrayList<List<String>> displayRows = new ArrayList<List<String>>();
        displayRows.add(Arrays.asList("Command Id", "Exec Time", "Device", "State"));
        long curTime = System.currentTimeMillis();

        for (InvocationThread invThread : copy) {
            displayRows.add(Arrays.asList(
                    Integer.toString(invThread.mCmd.getCommandTracker().getId()),
                    getTimeString(curTime - invThread.getStartTime()),
                    invThread.getInvocationContext().getSerials().toString(),
                    invThread.getInvocation().toString()));
        }
        new TableFormatter().displayTable(displayRows, printWriter);
    }

    private String getTimeString(long elapsedTime) {
        long duration = elapsedTime / 1000;
        long secs = duration % 60;
        long mins = (duration / 60) % 60;
        long hrs = duration / (60 * 60);
        String time = "unknown";
        if (hrs > 0) {
            time = String.format("%dh:%02d:%02d", hrs, mins, secs);
        } else {
            time = String.format("%dm:%02d", mins, secs);
        }
        return time;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public synchronized boolean stopInvocation(ITestInvocation invocation) {
        for (InvocationThread thread : mInvocationThreadMap.values()) {
            if (thread.getInvocation() == invocation) {
                thread.interrupt();
                return true;
            }
        }
        return false;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public synchronized boolean stopInvocation(int invocationId) {
        // TODO: make invocationID part of InvocationContext
        for (InvocationThread thread : mInvocationThreadMap.values()) {
            if (thread.mCmd.getCommandTracker().mId == invocationId) {
                thread.stopInvocation("User requested stopping invocation " + invocationId);
                return true;
            }
        }
        CLog.w("No invocation found matching the id: %s", invocationId);
        return false;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public synchronized String getInvocationInfo(int invocationId) {
        for (InvocationThread thread : mInvocationThreadMap.values()) {
            if (thread.mCmd.getCommandTracker().mId == invocationId) {
                String info = Arrays.toString(thread.mCmd.getCommandTracker().getArgs());
                return info;
            }
        }
        CLog.w("No invocation found matching the id: %s", invocationId);
        return null;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void displayCommandsInfo(PrintWriter printWriter, String regex) {
        assertStarted();
        Pattern regexPattern = null;
        if (regex != null) {
            regexPattern = Pattern.compile(regex);
        }

        List<CommandTracker> cmds = getCommandTrackers();
        Collections.sort(cmds, new CommandTrackerIdComparator());
        for (CommandTracker cmd : cmds) {
            String argString = getArgString(cmd.getArgs());
            if (regexPattern == null || regexPattern.matcher(argString).find()) {
                String cmdDesc = String.format("Command %d: [%s] %s", cmd.getId(),
                        getTimeString(cmd.getTotalExecTime()), argString);
                printWriter.println(cmdDesc);
            }
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void dumpCommandsXml(PrintWriter printWriter, String regex) {
        assertStarted();
        Pattern regexPattern = null;
        if (regex != null) {
            regexPattern = Pattern.compile(regex);
        }

        List<ExecutableCommandState> cmdCopy = getAllCommands();
        for (ExecutableCommandState cmd : cmdCopy) {
            String[] args = cmd.cmd.getCommandTracker().getArgs();
            String argString = getArgString(args);
            if (regexPattern == null || regexPattern.matcher(argString).find()) {
                // Use the config name prefixed by config__ for the file path
                String xmlPrefix = "config__" + args[0].replace("/", "__") + "__";

                // If the command line contains --template:map test config, use that config for the
                // file path.  This is because in the template system, many tests will have same
                // base config and the distinguishing feature is the test included.
                boolean templateIncludeFound = false;
                boolean testFound = false;
                for (String arg : args) {
                    if ("--template:map".equals(arg)) {
                        templateIncludeFound = true;
                    } else if (templateIncludeFound && "test".equals(arg)) {
                        testFound = true;
                    } else {
                        if (templateIncludeFound && testFound) {
                            xmlPrefix = "config__" + arg.replace("/", "__") + "__";
                        }
                        templateIncludeFound = false;
                        testFound = false;
                    }
                }

                try {
                    File xmlFile = FileUtil.createTempFile(xmlPrefix, ".xml");
                    PrintWriter writer = new PrintWriter(xmlFile);
                    cmd.cmd.getConfiguration().dumpXml(writer);
                    printWriter.println(String.format("Saved command dump to %s",
                            xmlFile.getAbsolutePath()));
                } catch (IOException e) {
                    // Log exception and continue
                    CLog.e("Could not dump config xml");
                    CLog.e(e);
                }
            }
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void displayCommandQueue(PrintWriter printWriter) {
        assertStarted();
        List<ExecutableCommandState> cmdCopy = getAllCommands();
        if (cmdCopy.size() == 0) {
            return;
        }
        ArrayList<List<String>> displayRows = new ArrayList<List<String>>();
        displayRows.add(Arrays.asList("Id", "Config", "Created", "Exec time", "State", "Sleep time",
                "Rescheduled", "Loop"));
        long curTime = System.currentTimeMillis();
        for (ExecutableCommandState cmd : cmdCopy) {
            dumpCommand(curTime, cmd, displayRows);
        }
        new TableFormatter().displayTable(displayRows, printWriter);
    }

    private void dumpCommand(long curTime, ExecutableCommandState cmdAndState,
            ArrayList<List<String>> displayRows) {
        ExecutableCommand cmd = cmdAndState.cmd;
        String sleepTime = cmd.getSleepTime() == null ? "N/A" : getTimeString(cmd.getSleepTime());
        displayRows.add(Arrays.asList(
                Integer.toString(cmd.getCommandTracker().getId()),
                cmd.getCommandTracker().getArgs()[0],
                getTimeString(curTime - cmd.getCreationTime()),
                getTimeString(cmd.mCmdTracker.getTotalExecTime()),
                cmdAndState.state.getDisplayName(),
                sleepTime,
                Boolean.toString(cmd.isRescheduled()),
                Boolean.toString(cmd.isLoopMode())));
    }

    /**
     * Starts remote manager to listen to remote commands.
     *
     * <p>TODO: refactor to throw exception on failure
     */
    private void startRemoteManager() {
        if (mRemoteManager != null && !mRemoteManager.isCanceled()) {
            String error = String.format("A remote manager is already running at port %d",
                    mRemoteManager.getPort());
            throw new IllegalStateException(error);
        }
        mRemoteManager = new RemoteManager(getDeviceManager(), this);
        // Read the args that were set by the global config.
        boolean startRmtMgrOnBoot = mRemoteManager.getStartRemoteMgrOnBoot();
        int defaultRmtMgrPort = mRemoteManager.getRemoteManagerPort();
        boolean autoHandover = mRemoteManager.getAutoHandover();

        if (!startRmtMgrOnBoot) {
            mRemoteManager = null;
            return;
        }
        if (mRemoteManager.connect()) {
            mRemoteManager.start();
            CLog.logAndDisplay(LogLevel.INFO, "Started remote manager at port %d",
                    mRemoteManager.getPort());
            return;
        }
        CLog.logAndDisplay(LogLevel.INFO, "Failed to start remote manager at port %d",
                defaultRmtMgrPort);
        if (!autoHandover) {
           if (mRemoteManager.connectAnyPort()) {
               mRemoteManager.start();
               CLog.logAndDisplay(LogLevel.INFO,
                       "Started remote manager at port %d with no handover",
                       mRemoteManager.getPort());
               return;
           } else {
               CLog.logAndDisplay(LogLevel.ERROR, "Failed to auto start a remote manager on boot.");
               return;
           }
        }
        try {
            CLog.logAndDisplay(LogLevel.INFO, "Initiating handover with remote TF instance!");
            mHandoverHandshake.reset();
            initiateHandover(defaultRmtMgrPort);
            waitForHandoverHandshake();
            CLog.logAndDisplay(LogLevel.INFO, "Handover initiation complete.");
        } catch (RemoteException e) {
            CLog.e(e);
        }
    }

    private void waitForHandoverHandshake() {
        // block and wait to receive all the commands and 'device still in use' messages from remote
        mHandoverHandshake.waitForEvent(MAX_HANDOVER_INIT_TIME);
        // TODO: throw exception if not received
    }

    /**
     * Helper object for allowing multiple threads to synchronize on an event.
     *
     * <p>Basically a modest wrapper around Object's wait and notify methods, that supports
     * remembering if a notify call was made.
     */
    private static class WaitObj {
        boolean mEventReceived = false;

        /**
         * Wait for signal for a max of given ms.
         * @return true if event received before time elapsed, false otherwise
         */
        public synchronized boolean waitForEvent(long maxWaitTime) {
            if (maxWaitTime == 0) {
                return waitForEvent();
            }
            long startTime = System.currentTimeMillis();
            long remainingTime = maxWaitTime;
            while (!mEventReceived && remainingTime > 0) {
                try {
                    wait(remainingTime);
                } catch (InterruptedException e) {
                    CLog.w("interrupted");
                }
                remainingTime = maxWaitTime - (System.currentTimeMillis() - startTime);
            }
            return mEventReceived;
        }

        /**
         * Wait for signal indefinitely or until interrupted.
         * @return true if event received, false otherwise
         */
        public synchronized boolean waitForEvent() {
            if (!mEventReceived) {
                try {
                    wait();
                } catch (InterruptedException e) {
                    CLog.w("interrupted");
                }
            }
            return mEventReceived;
        }

        /**
         * Reset the event received flag.
         */
        public synchronized void reset() {
            mEventReceived = false;
        }

        /**
         * Wait for given ms for event to be received, and reset state back to 'no event received'
         * upon completion.
         */
        public synchronized void waitAndReset(long maxWaitTime) {
            waitForEvent(maxWaitTime);
            reset();
        }

        /**
         * Notify listeners that event was received.
         */
        public synchronized void signalEventReceived() {
            mEventReceived = true;
            notifyAll();
        }
    }

    @Override
    public void handoverInitiationComplete() {
        mHandoverHandshake.signalEventReceived();
    }

    @Override
    public void completeHandover() {
        CLog.logAndDisplay(LogLevel.INFO, "Completing handover.");
        if (mRemoteClient != null) {
            mRemoteClient.close();
            mRemoteClient = null;
        } else {
            CLog.e("invalid state: received handover complete when remote client is null");
        }

        if (mRemoteManager != null) {
            mRemoteManager.cancelAndWait();
            mRemoteManager = null;
        } else {
            CLog.e("invalid state: received handover complete when remote manager is null");
        }

        // Start a new remote manager and attempt to capture the original default port.
        mRemoteManager = new RemoteManager(getDeviceManager(), this);
        boolean success = false;
        for (int i=0; i < 10 && !success; i++) {
            try {
                sleep(2000);
            } catch (InterruptedException e) {
                CLog.e(e);
                return;
            }
            success = mRemoteManager.connect();
        }
        if (!success) {
            CLog.e("failed to connect to default remote manager port");
            return;
        }

        mRemoteManager.start();
        CLog.logAndDisplay(LogLevel.INFO,
                "Successfully started remote manager after handover on port %d",
                mRemoteManager.getPort());
    }

    private void initiateHandover(int port) throws RemoteException {
        mRemoteClient = RemoteClient.connect(port);
        CLog.i("Connecting local client with existing remote TF at %d - Attempting takeover", port);
        // Start up a temporary local remote manager for handover.
        if (mRemoteManager.connectAnyPort()) {
            mRemoteManager.start();
            CLog.logAndDisplay(LogLevel.INFO,
                    "Started local tmp remote manager for handover at port %d",
                    mRemoteManager.getPort());
            mRemoteClient.sendStartHandover(mRemoteManager.getPort());
        }
    }

    private synchronized void assertStarted() {
        if(!mStarted) {
            throw new IllegalStateException("start() must be called before this method");
        }
    }

    @Override
    public void notifyFileChanged(File cmdFile, List<String> extraArgs) {
        CLog.logAndDisplay(LogLevel.INFO, "Detected update for cmdfile '%s'. Reloading",
                cmdFile.getAbsolutePath());
        removeCommandsFromFile(cmdFile);
        try {
            // just add the file again, including re-registering for command file watcher
            // don't want to remove the registration here in case file fails to load
            internalAddCommandFile(cmdFile, extraArgs);
        } catch (ConfigurationException e) {
            CLog.wtf(String.format("Failed to automatically reload cmdfile %s",
                    cmdFile.getAbsolutePath()), e);
        }
    }

    /**
     * Set the command file reloading flag.
     */
    @VisibleForTesting
    void setCommandFileReload(boolean b) {
        mReloadCmdfiles = b;
    }

    synchronized int getAllCommandsSize() {
        return mReadyCommands.size() + mExecutingCommands.size() + mSleepingCommands.size();
    }

    synchronized List<ExecutableCommandState> getAllCommands() {
        List<ExecutableCommandState> cmds = new ArrayList<>(getAllCommandsSize());
        for (ExecutableCommand cmd : mExecutingCommands) {
            cmds.add(new ExecutableCommandState(cmd, CommandState.EXECUTING));
        }
        for (ExecutableCommand cmd : mReadyCommands) {
            cmds.add(new ExecutableCommandState(cmd, CommandState.WAITING_FOR_DEVICE));
        }
        for (ExecutableCommand cmd : mSleepingCommands) {
            cmds.add(new ExecutableCommandState(cmd, CommandState.SLEEPING));
        }
        return cmds;
    }

    @Override
    public boolean shouldShutdownOnCmdfileError() {
        return mShutdownOnCmdfileError;
    }

    public long getShutdownTimeout() {
        return mShutdownTimeout;
    }

    @Override
    public ExitCode getLastInvocationExitCode() {
        return mLastInvocationExitCode;
    }

    @Override
    public Throwable getLastInvocationThrowable() {
        return mLastInvocationThrowable;
    }

    @Override
    public void setLastInvocationExitCode(ExitCode code, Throwable throwable) {
        mLastInvocationExitCode = code;
        mLastInvocationThrowable = throwable;
    }

    @Override
    public synchronized int getReadyCommandCount() {
        return mReadyCommands.size();
    }
}
