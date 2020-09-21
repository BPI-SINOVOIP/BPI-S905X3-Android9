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

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.build.IFolderBuildInfo;
import com.android.tradefed.config.GlobalConfiguration;
import com.android.tradefed.config.IConfiguration;
import com.android.tradefed.config.IConfigurationReceiver;
import com.android.tradefed.config.Option;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.FileInputStreamSource;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.IRunUtil;
import com.android.tradefed.util.IRunUtil.EnvPriority;
import com.android.tradefed.util.RunUtil;
import com.android.tradefed.util.StreamUtil;
import com.android.tradefed.util.SubprocessTestResultsParser;
import com.android.tradefed.util.TimeUtil;
import com.android.tradefed.util.UniqueMultiMap;

import org.junit.Assert;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

/**
 * A {@link IRemoteTest} for running tests against a separate TF installation.
 *
 * <p>Launches an external java process to run the tests. Used for running the TF unit or functional
 * tests continuously.
 */
public abstract class SubprocessTfLauncher
        implements IBuildReceiver, IInvocationContextReceiver, IRemoteTest, IConfigurationReceiver {

    @Option(name = "max-run-time", description =
            "The maximum time to allow for a TF test run.", isTimeVal = true)
    private long mMaxTfRunTime = 20 * 60 * 1000;

    @Option(name = "remote-debug", description =
            "Start the TF java process in remote debug mode.")
    private boolean mRemoteDebug = false;

    @Option(name = "config-name", description = "The config that runs the TF tests")
    private String mConfigName;

    @Option(name = "use-event-streaming", description = "Use a socket to receive results as they"
            + "arrived instead of using a temporary file and parsing at the end.")
    private boolean mEventStreaming = true;

    @Option(name = "sub-global-config", description = "The global config name to pass to the"
            + "sub process, can be local or from jar resources. Be careful of conflicts with "
            + "parent process.")
    private String mGlobalConfig = null;

    @Option(
        name = "inject-invocation-data",
        description = "Pass the invocation-data to the subprocess if enabled."
    )
    private boolean mInjectInvocationData = false;

    // Temp global configuration filtered from the parent process.
    private String mFilteredGlobalConfig = null;

    /** Timeout to wait for the events received from subprocess to finish being processed.*/
    private static final long EVENT_THREAD_JOIN_TIMEOUT_MS = 30 * 1000;

    protected static final String TF_GLOBAL_CONFIG = "TF_GLOBAL_CONFIG";

    protected IRunUtil mRunUtil =  new RunUtil();

    protected IBuildInfo mBuildInfo = null;
    // Temp directory to run the TF process.
    protected File mTmpDir = null;
    // List of command line arguments to run the TF process.
    protected List<String> mCmdArgs = null;
    // The absolute path to the build's root directory.
    protected String mRootDir = null;
    protected IConfiguration mConfig;
    private IInvocationContext mContext;

    @Override
    public void setInvocationContext(IInvocationContext invocationContext) {
        mContext = invocationContext;
    }

    @Override
    public void setConfiguration(IConfiguration configuration) {
        mConfig = configuration;
    }

    /**
     * Set use-event-streaming.
     *
     * Exposed for unit testing.
     */
    protected void setEventStreaming(boolean eventStreaming) {
        mEventStreaming = eventStreaming;
    }

    /**
     * Set IRunUtil.
     *
     * Exposed for unit testing.
     */
    protected void setRunUtil(IRunUtil runUtil) {
        mRunUtil = runUtil;
    }

    /** Returns the {@link IRunUtil} that will be used for the subprocess command. */
    protected IRunUtil getRunUtil() {
        return mRunUtil;
    }

    /**
     * Setup before running the test.
     */
    protected void preRun() {
        Assert.assertNotNull(mBuildInfo);
        Assert.assertNotNull(mConfigName);
        IFolderBuildInfo tfBuild = (IFolderBuildInfo) mBuildInfo;
        mRootDir = tfBuild.getRootDir().getAbsolutePath();
        String jarClasspath = FileUtil.getPath(mRootDir, "*");

        mCmdArgs = new ArrayList<String>();
        mCmdArgs.add("java");

        try {
            mTmpDir = FileUtil.createTempDir("subprocess-" + tfBuild.getBuildId());
            mCmdArgs.add(String.format("-Djava.io.tmpdir=%s", mTmpDir.getAbsolutePath()));
        } catch (IOException e) {
            CLog.e(e);
            throw new RuntimeException(e);
        }

        addJavaArguments(mCmdArgs);

        if (mRemoteDebug) {
            mCmdArgs.add("-agentlib:jdwp=transport=dt_socket,server=y,suspend=y,address=10088");
        }
        // FIXME: b/72742216: This prevent the illegal reflective access
        mCmdArgs.add("--add-opens=java.base/java.nio=ALL-UNNAMED");
        mCmdArgs.add("-cp");

        mCmdArgs.add(jarClasspath);
        mCmdArgs.add("com.android.tradefed.command.CommandRunner");
        mCmdArgs.add(mConfigName);

        // clear the TF_GLOBAL_CONFIG env, so another tradefed will not reuse the global config file
        mRunUtil.unsetEnvVariable(TF_GLOBAL_CONFIG);
        if (mGlobalConfig == null) {
            // If the global configuration is not set in option, create a filtered global
            // configuration for subprocess to use.
            try {
                String[] configs =
                        new String[] {
                            GlobalConfiguration.DEVICE_MANAGER_TYPE_NAME,
                            GlobalConfiguration.KEY_STORE_TYPE_NAME
                        };
                File filteredGlobalConfig =
                        FileUtil.createTempFile("filtered_global_config", ".config");
                GlobalConfiguration.getInstance()
                        .cloneConfigWithFilter(filteredGlobalConfig, configs);
                mFilteredGlobalConfig = filteredGlobalConfig.getAbsolutePath();
                mGlobalConfig = mFilteredGlobalConfig;
            } catch (IOException e) {
                CLog.e("Failed to create filtered global configuration");
                CLog.e(e);
            }
        }
        if (mGlobalConfig != null) {
            // We allow overriding this global config and then set it for the subprocess.
            mRunUtil.setEnvVariablePriority(EnvPriority.SET);
            mRunUtil.setEnvVariable(TF_GLOBAL_CONFIG, mGlobalConfig);
        }
    }

    /**
     * Allow to add extra java parameters to the subprocess invocation.
     *
     * @param args the current list of arguments to which we need to add the extra ones.
     */
    protected void addJavaArguments(List<String> args) {}

    /**
     * Actions to take after the TF test is finished.
     *
     * @param listener the original {@link ITestInvocationListener} where to report results.
     * @param exception True if exception was raised inside the test.
     * @param elapsedTime the time taken to run the tests.
     */
    protected void postRun(ITestInvocationListener listener, boolean exception, long elapsedTime) {}

    /** Pipe to the subprocess the invocation-data so that it can use them if needed. */
    private void addInvocationData() {
        if (!mInjectInvocationData) {
            return;
        }
        UniqueMultiMap<String, String> data = mConfig.getCommandOptions().getInvocationData();
        for (String key : data.keySet()) {
            for (String value : data.get(key)) {
                mCmdArgs.add("--invocation-data");
                mCmdArgs.add(key);
                mCmdArgs.add(value);
            }
        }
    }

    /** {@inheritDoc} */
    @Override
    public void run(ITestInvocationListener listener) {
        preRun();
        addInvocationData();

        File stdoutFile = null;
        File stderrFile = null;
        File eventFile = null;
        SubprocessTestResultsParser eventParser = null;
        FileOutputStream stdout = null;
        FileOutputStream stderr = null;

        boolean exception = false;
        long startTime = 0l;
        long elapsedTime = -1l;
        try {
            stdoutFile = FileUtil.createTempFile("stdout_subprocess_", ".log");
            stderrFile = FileUtil.createTempFile("stderr_subprocess_", ".log");
            stderr = new FileOutputStream(stderrFile);
            stdout = new FileOutputStream(stdoutFile);

            eventParser = new SubprocessTestResultsParser(listener, mEventStreaming, mContext);
            if (mEventStreaming) {
                mCmdArgs.add("--subprocess-report-port");
                mCmdArgs.add(Integer.toString(eventParser.getSocketServerPort()));
            } else {
                eventFile = FileUtil.createTempFile("event_subprocess_", ".log");
                mCmdArgs.add("--subprocess-report-file");
                mCmdArgs.add(eventFile.getAbsolutePath());
            }
            startTime = System.currentTimeMillis();
            CommandResult result = mRunUtil.runTimedCmd(mMaxTfRunTime, stdout,
                    stderr, mCmdArgs.toArray(new String[0]));
            if (eventParser.getStartTime() != null) {
                startTime = eventParser.getStartTime();
            }
            elapsedTime = System.currentTimeMillis() - startTime;
            // We possibly allow for a little more time if the thread is still processing events.
            if (!eventParser.joinReceiver(EVENT_THREAD_JOIN_TIMEOUT_MS)) {
                elapsedTime = -1l;
                throw new RuntimeException(String.format("Event receiver thread did not complete:"
                        + "\n%s", FileUtil.readStringFromFile(stderrFile)));
            }
            if (result.getStatus().equals(CommandStatus.SUCCESS)) {
                CLog.d("Successfully ran TF tests for build %s", mBuildInfo.getBuildId());
                testCleanStdErr(stderrFile, listener);
            } else {
                CLog.w("Failed ran TF tests for build %s, status %s",
                        mBuildInfo.getBuildId(), result.getStatus());
                CLog.v("TF tests output:\nstdout:\n%s\nstderror:\n%s",
                        result.getStdout(), result.getStderr());
                exception = true;
                String errMessage = null;
                if (result.getStatus().equals(CommandStatus.TIMED_OUT)) {
                    errMessage = String.format("Timeout after %s",
                            TimeUtil.formatElapsedTime(mMaxTfRunTime));
                } else {
                    errMessage = FileUtil.readStringFromFile(stderrFile);
                }
                throw new RuntimeException(
                        String.format("%s Tests subprocess failed due to:\n%s\n", mConfigName,
                                errMessage));
            }
        } catch (IOException e) {
            exception = true;
            throw new RuntimeException(e);
        } finally {
            StreamUtil.close(stdout);
            StreamUtil.close(stderr);
            logAndCleanFile(stdoutFile, listener);
            logAndCleanFile(stderrFile, listener);
            if (eventFile != null) {
                eventParser.parseFile(eventFile);
                logAndCleanFile(eventFile, listener);
            }
            StreamUtil.close(eventParser);

            postRun(listener, exception, elapsedTime);

            if (mTmpDir != null) {
                FileUtil.recursiveDelete(mTmpDir);
            }

            if (mFilteredGlobalConfig != null) {
                FileUtil.deleteFile(new File(mFilteredGlobalConfig));
            }
        }
    }

    /**
     * Log the content of given file to listener, then remove the file.
     *
     * @param fileToExport the {@link File} pointing to the file to log.
     * @param listener the {@link ITestInvocationListener} where to report the test.
     */
    private void logAndCleanFile(File fileToExport, ITestInvocationListener listener) {
        if (fileToExport == null)
            return;

        try (FileInputStreamSource inputStream = new FileInputStreamSource(fileToExport)) {
            listener.testLog(fileToExport.getName(), LogDataType.TEXT, inputStream);
        }
        FileUtil.deleteFile(fileToExport);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setBuild(IBuildInfo buildInfo) {
        mBuildInfo = buildInfo;
    }

    /**
     * Extra test to ensure no abnormal logging is made to stderr when all the tests pass.
     *
     * @param stdErrFile the stderr log file of the subprocess.
     * @param listener the {@link ITestInvocationListener} where to report the test.
     */
    private void testCleanStdErr(File stdErrFile, ITestInvocationListener listener)
            throws IOException {
        listener.testRunStarted("StdErr", 1);
        TestDescription tid = new TestDescription("stderr-test", "checkIsEmpty");
        listener.testStarted(tid);
        if (!FileUtil.readStringFromFile(stdErrFile).isEmpty()) {
            String trace =
                    String.format(
                            "Found some output in stderr:\n%s",
                            FileUtil.readStringFromFile(stdErrFile));
            listener.testFailed(tid, trace);
        }
        listener.testEnded(tid, new HashMap<String, Metric>());
        listener.testRunEnded(0, new HashMap<String, Metric>());
    }
}
