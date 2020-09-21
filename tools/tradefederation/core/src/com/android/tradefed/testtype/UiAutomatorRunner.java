/*
 * Copyright (C) 2012 The Android Open Source Project
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

import com.android.ddmlib.AdbCommandRejectedException;
import com.android.ddmlib.IDevice;
import com.android.ddmlib.ShellCommandUnresponsiveException;
import com.android.ddmlib.TimeoutException;
import com.android.ddmlib.testrunner.IRemoteAndroidTestRunner;
import com.android.ddmlib.testrunner.ITestRunListener;
import com.android.ddmlib.testrunner.InstrumentationResultParser;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.ArrayUtil;

import java.io.IOException;
import java.util.Arrays;
import java.util.Collection;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.Map.Entry;
import java.util.concurrent.TimeUnit;

/**
 * Runs UI Automator test on device and reports results.
 *
 * UI Automator test is a dedicated test runner for running UI automation tests that
 * utilizes UI Automator framework. The test runner on device emulates instrumentation
 * test output format so that existing parsing code in ddmlib and TF can be reused.
 *
 * Essentially, this is a wrapper around this command:
 * adb shell uiautomator runtest (jar files) -e class (test classes) ...
 *
 */
public class UiAutomatorRunner implements IRemoteAndroidTestRunner {

    private static final String CLASS_ARG_NAME = "class";
    private static final String DEBUG_ARG_NAME = "debug";
    private static final String RUNNER_ARG_NAME = "runner";
    private static final char METHOD_SEPARATOR = '#';
    private static final char CLASS_SEPARATOR = ',';
    private static final String DEFAULT_RUNNER_NAME =
            "com.android.uiautomator.testrunner.UiAutomatorTestRunner";
    private static final String UIAUTOMATOR_RUNNER_PATH = "/system/bin/uiautomator";

    private Map<String, String> mArgsMap;
    private String[] mJarPaths;
    private String mPackageName;
    // default to no timeout
    private long mMaxTimeout = 0l;
    private long mMaxTimeToOutputResponse = 0;
    private IDevice mRemoteDevice;
    private String mRunName;
    private InstrumentationResultParser mParser;
    private String mRunnerPath = UIAUTOMATOR_RUNNER_PATH;
    private String mRunnerName = DEFAULT_RUNNER_NAME;
    private boolean mIgnoreSighup = false;

    /**
     * Create a UiAutomatorRunner for running UI automation tests
     *
     * @param remoteDevice the remote device to interact with: run test, collect results etc
     * @param jarPaths the paths to jar files where UI Automator test cases are; the paths must be
     *                absolute or relative to /data/local/tmp/ on device
     * @param runnerPath alternative uiautomator runner to use, may be <code>null</code> and default
     *                   will be used in this case
     */
    public UiAutomatorRunner(IDevice remoteDevice, String[] jarPaths, String runnerPath) {
        mRemoteDevice = remoteDevice;
        mJarPaths = jarPaths;
        mArgsMap = new LinkedHashMap<String, String>(); // ensure the order that the args are added
        if (runnerPath != null) {
            mRunnerPath = runnerPath;
        }
    }

    /**
     * Returns the package name of last Java class added
     */
    @Override
    public String getPackageName() {
        return mPackageName;
    }

    /**
     * Returns default UiAutomatorTestRunner class name
     */
    @Override
    public String getRunnerName() {
        return mRunnerName;
    }

    protected String getRunnerPath() {
        return mRunnerPath;
    }

    /**
     * Sets the option in the uiautomator to ignore SIGHUP.
     * @param value ignore the signal if set to true
     */
    public void setIgnoreSighup(boolean value) {
        mIgnoreSighup = value;
    }

    protected String getRunCommand() {
        String jarArg = ArrayUtil.join(" ", (Object[])mJarPaths);
        String command = String.format("%s runtest %s %s",
                getRunnerPath(), jarArg, getArgsCommand());
        if (mIgnoreSighup) {
            return command + " --nohup";
        } else {
            return command;
        }
    }

    /**
     * Returns the full instrumentation command line syntax for the provided instrumentation
     * arguments.
     * Returns an empty string if no arguments were specified.
     */
    private String getArgsCommand() {
        StringBuilder commandBuilder = new StringBuilder();
        for (Entry<String, String> argPair : mArgsMap.entrySet()) {
            final String argCmd = String.format(" -e %1$s %2$s", argPair.getKey(),
                    argPair.getValue());
            commandBuilder.append(argCmd);
        }
        return commandBuilder.toString();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setClassName(String className) {
        int pos = className.lastIndexOf('.');
        // get package name segment
        if (pos == -1) {
            mPackageName = "(default)";
        } else {
            mPackageName = className.substring(0, pos);
        }
        addInstrumentationArg(CLASS_ARG_NAME, className);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setClassNames(String[] classNames) {
        StringBuilder classArgBuilder = new StringBuilder();

        for (int i = 0; i < classNames.length; i++) {
            if (i != 0) {
                classArgBuilder.append(CLASS_SEPARATOR);
            }
            classArgBuilder.append(classNames[i]);
        }
        setClassName(classArgBuilder.toString());
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setMethodName(String className, String testName) {
        setClassName(className + METHOD_SEPARATOR + testName);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setTestPackageName(String packageName) {
        throw new UnsupportedOperationException("specifying package name is not supported");
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setTestSize(TestSize size) {
        throw new UnsupportedOperationException("specifying test size is not supported");
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void addInstrumentationArg(String name, String value) {
        if (name == null) {
            throw new IllegalArgumentException("name cannot be null");
        }
        if (RUNNER_ARG_NAME.equals(name)) {
            mRunnerName = name;
        }
        mArgsMap.put(name, value);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void removeInstrumentationArg(String name) {
        if (name == null) {
            throw new IllegalArgumentException("name cannot be null");
        }
        mArgsMap.remove(name);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void addBooleanArg(String name, boolean value) {
        addInstrumentationArg(name, Boolean.toString(value));
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setLogOnly(boolean logOnly) {
        //TODO: we need to support log only for Eclipse and re-run support
        throw new UnsupportedOperationException("log only mode is not supported");
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setDebug(boolean debug) {
        addBooleanArg(DEBUG_ARG_NAME, debug);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setCoverage(boolean coverage) {
        // TODO potentially it's possible to run with coverage, need more investigation
        throw new UnsupportedOperationException("coverage mode is not supported");
    }

    @Override
    public void setTestCollection(boolean b) {
        throw new UnsupportedOperationException("Test Collection mode is not supported");
    }

    /**
     * {@inheritDoc}
     * @deprecated use {@link #setMaxTimeToOutputResponse(long, TimeUnit)} instead.
     */
    @Deprecated
    @Override
    public void setMaxtimeToOutputResponse(int maxTimeToOutputResponse) {
        setMaxTimeToOutputResponse(maxTimeToOutputResponse, TimeUnit.MILLISECONDS);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setMaxTimeToOutputResponse(long timeout, TimeUnit unit) {
        mMaxTimeToOutputResponse = unit.toMillis(timeout);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setRunName(String runName) {
        mRunName = runName;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void run(ITestRunListener... listeners) throws TimeoutException,
            AdbCommandRejectedException, ShellCommandUnresponsiveException, IOException {
        run(Arrays.asList(listeners));
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void run(Collection<ITestRunListener> listeners) throws TimeoutException,
            AdbCommandRejectedException, ShellCommandUnresponsiveException, IOException {
        String cmdLine = getRunCommand();
        CLog.i("Running %s on %s", cmdLine, mRemoteDevice.getSerialNumber());
        String runName = mRunName == null ? mPackageName : mRunName;
        mParser = new InstrumentationResultParser(runName, listeners);

        try {
            mRemoteDevice.executeShellCommand(
                    cmdLine, mParser, mMaxTimeout, mMaxTimeToOutputResponse, TimeUnit.MILLISECONDS);
        } catch (IOException e) {
            CLog.w(String.format("IOException %1$s when running tests %2$s on %3$s",
                    e.toString(), getPackageName(), mRemoteDevice.getSerialNumber()));
            // rely on parser to communicate results to listeners
            mParser.handleTestRunFailed(e.toString());
            throw e;
        } catch (ShellCommandUnresponsiveException e) {
            CLog.w("ShellCommandUnresponsiveException %1$s when running tests %2$s on %3$s",
                    e.toString(), getPackageName(), mRemoteDevice.getSerialNumber());
            mParser.handleTestRunFailed(String.format(
                    "Failed to receive adb shell test output within %1$d ms. " +
                    "Test may have timed out, or adb connection to device became unresponsive",
                    mMaxTimeToOutputResponse));
            throw e;
        } catch (TimeoutException e) {
            CLog.w("TimeoutException when running tests %1$s on %2$s", getPackageName(),
                    mRemoteDevice.getSerialNumber());
            mParser.handleTestRunFailed(e.toString());
            throw e;
        } catch (AdbCommandRejectedException e) {
            CLog.w("AdbCommandRejectedException %1$s when running tests %2$s on %3$s",
                    e.toString(), getPackageName(), mRemoteDevice.getSerialNumber());
            mParser.handleTestRunFailed(e.toString());
            throw e;
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void cancel() {
        if (mParser != null) {
            mParser.cancel();
        }
    }

    @Override
    public void setEnforceTimeStamp(boolean arg0) {
        // ignore, UiAutomator runner does not need this.
    }

    @Override
    public void setMaxTimeout(long maxTimeout, TimeUnit unit) {
        mMaxTimeout = unit.toMillis(maxTimeout);
    }
}
