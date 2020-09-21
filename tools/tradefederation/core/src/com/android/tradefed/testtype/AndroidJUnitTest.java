/*
 * Copyright (C) 2015 The Android Open Source Project
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

import com.android.ddmlib.testrunner.IRemoteAndroidTestRunner;
import com.android.tradefed.config.ConfigurationException;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionCopier;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.util.ArrayUtil;
import com.android.tradefed.util.ListInstrumentationParser;

import com.google.common.annotations.VisibleForTesting;

import org.junit.runner.notification.RunListener;

import java.io.File;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Set;

/**
 * A Test that runs an instrumentation test package on given device using the
 * android.support.test.runner.AndroidJUnitRunner.
 */
public class AndroidJUnitTest extends InstrumentationTest implements IRuntimeHintProvider,
        ITestFileFilterReceiver, ITestFilterReceiver, ITestAnnotationFilterReceiver,
        IShardableTest, IStrictShardableTest {

    protected static final String AJUR = "android.support.test.runner.AndroidJUnitRunner";

    /** instrumentation test runner argument key used for including a class/test */
    private static final String INCLUDE_CLASS_INST_ARGS_KEY = "class";
    /** instrumentation test runner argument key used for excluding a class/test */
    private static final String EXCLUDE_CLASS_INST_ARGS_KEY = "notClass";
    /** instrumentation test runner argument key used for including a package */
    private static final String INCLUDE_PACKAGE_INST_ARGS_KEY = "package";
    /** instrumentation test runner argument key used for excluding a package */
    private static final String EXCLUDE_PACKAGE_INST_ARGS_KEY = "notPackage";
    /** instrumentation test runner argument key used for adding annotation filter */
    private static final String ANNOTATION_INST_ARGS_KEY = "annotation";
    /** instrumentation test runner argument key used for adding notAnnotation filter */
    private static final String NOT_ANNOTATION_INST_ARGS_KEY = "notAnnotation";
    /** instrumentation test runner argument used for adding testFile filter */
    private static final String TEST_FILE_INST_ARGS_KEY = "testFile";
    /** instrumentation test runner argument used for adding notTestFile filter */
    private static final String NOT_TEST_FILE_INST_ARGS_KEY = "notTestFile";
    /** instrumentation test runner argument used to specify the shardIndex of the test */
    private static final String SHARD_INDEX_INST_ARGS_KEY = "shardIndex";
    /** instrumentation test runner argument used to specify the total number of shards */
    private static final String NUM_SHARD_INST_ARGS_KEY = "numShards";
    /**
     * instrumentation test runner argument used to enable the new {@link RunListener} order on
     * device side.
     */
    public static final String NEW_RUN_LISTENER_ORDER_KEY = "newRunListenerMode";

    private static final String INCLUDE_FILE = "includes.txt";
    private static final String EXCLUDE_FILE = "excludes.txt";

    @Option(name = "runtime-hint",
            isTimeVal=true,
            description="The hint about the test's runtime.")
    private long mRuntimeHint = 60000;// 1 minute

    @Option(name = "include-filter",
            description="The include filters of the test name to run.")
    private List<String> mIncludeFilters = new ArrayList<>();

    @Option(name = "exclude-filter",
            description="The exclude filters of the test name to run.")
    private List<String> mExcludeFilters = new ArrayList<>();

    @Option(name = "include-annotation",
            description="The annotation class name of the test name to run, can be repeated")
    private List<String> mIncludeAnnotation = new ArrayList<>();

    @Option(name = "exclude-annotation",
            description="The notAnnotation class name of the test name to run, can be repeated")
    private List<String> mExcludeAnnotation = new ArrayList<>();

    @Option(name = "test-file-include-filter",
            description="A file containing a list of line separated test classes and optionally"
            + " methods to include")
    private File mIncludeTestFile = null;

    @Option(name = "test-file-exclude-filter",
            description="A file containing a list of line separated test classes and optionally"
            + " methods to exclude")
    private File mExcludeTestFile = null;

    @Option(name = "test-filter-dir",
            description="The device directory path to which the test filtering files are pushed")
    private String mTestFilterDir = "/data/local/tmp/ajur";

    @Option(
        name = "ajur-max-shard",
        description = "The maximum number of shard we want to allow the AJUR test to shard into"
    )
    private Integer mMaxShard = null;

    @Option(
        name = "device-listeners",
        description =
                "Specify a device side instrumentation listener to be added for the run. "
                        + "Can be repeated."
    )
    private List<String> mExtraDeviceListener = new ArrayList<>();

    @Option(
        name = "use-new-run-listener-order",
        description = "Enables the new RunListener Order for AJUR."
    )
    // Default to true as it is harmless if not supported.
    private boolean mNewRunListenerOrderMode = true;

    private String mDeviceIncludeFile = null;
    private String mDeviceExcludeFile = null;
    private int mTotalShards = 0;
    private int mShardIndex = 0;
    // Flag to avoid re-sharding a test that already was.
    private boolean mIsSharded = false;

    public AndroidJUnitTest() {
        super();
        // Set the runner to AJUR, this can still be overwritten by the optionsetter/optioncopier
        setRunnerName(AJUR);
        setEnforceFormat(true);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public long getRuntimeHint() {
        return mRuntimeHint;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void addIncludeFilter(String filter) {
        mIncludeFilters.add(filter);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void addAllIncludeFilters(Set<String> filters) {
        mIncludeFilters.addAll(filters);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void addExcludeFilter(String filter) {
        mExcludeFilters.add(filter);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void addAllExcludeFilters(Set<String> filters) {
        mExcludeFilters.addAll(filters);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setIncludeTestFile(File testFile) {
        mIncludeTestFile = testFile;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setExcludeTestFile(File testFile) {
        mExcludeTestFile = testFile;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void addIncludeAnnotation(String annotation) {
        mIncludeAnnotation.add(annotation);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void addAllIncludeAnnotation(Set<String> annotations) {
        mIncludeAnnotation.addAll(annotations);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void addExcludeAnnotation(String excludeAnnotation) {
        mExcludeAnnotation.add(excludeAnnotation);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void addAllExcludeAnnotation(Set<String> excludeAnnotations) {
        mExcludeAnnotation.addAll(excludeAnnotations);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void run(final ITestInvocationListener listener) throws DeviceNotAvailableException {
        if (getDevice() == null) {
            throw new IllegalArgumentException("Device has not been set");
        }
        // if mIncludeTestFile is set, perform filtering with this file
        if (mIncludeTestFile != null) {
            mDeviceIncludeFile = mTestFilterDir.replaceAll("/$", "") + "/" + INCLUDE_FILE;
            pushTestFile(mIncludeTestFile, mDeviceIncludeFile, listener);
        }

        // if mExcludeTestFile is set, perform filtering with this file
        if (mExcludeTestFile != null) {
            mDeviceExcludeFile = mTestFilterDir.replaceAll("/$", "") + "/" + EXCLUDE_FILE;
            pushTestFile(mExcludeTestFile, mDeviceExcludeFile, listener);
        }
        if (mTotalShards > 0 && !isShardable() && mShardIndex != 0) {
            // If not shardable, only first shard can run.
            CLog.i("%s is not shardable.", getRunnerName());
            return;
        }
        super.run(listener);
        if (mIncludeTestFile != null) {
            removeTestFile(mDeviceIncludeFile);
        }
        if (mExcludeTestFile != null) {
            removeTestFile(mDeviceExcludeFile);
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    protected void setRunnerArgs(IRemoteAndroidTestRunner runner) {
        super.setRunnerArgs(runner);

        // if mIncludeTestFile is set, perform filtering with this file
        if (mDeviceIncludeFile != null) {
            runner.addInstrumentationArg(TEST_FILE_INST_ARGS_KEY, mDeviceIncludeFile);
        }

        // if mExcludeTestFile is set, perform filtering with this file
        if (mDeviceExcludeFile != null) {
            runner.addInstrumentationArg(NOT_TEST_FILE_INST_ARGS_KEY, mDeviceExcludeFile);
        }

        // Split filters into class, notClass, package and notPackage
        List<String> classArg = new ArrayList<String>();
        List<String> notClassArg = new ArrayList<String>();
        List<String> packageArg = new ArrayList<String>();
        List<String> notPackageArg = new ArrayList<String>();
        for (String test : mIncludeFilters) {
            if (isClassOrMethod(test)) {
                classArg.add(test);
            } else {
                packageArg.add(test);
            }
        }
        for (String test : mExcludeFilters) {
            if (isClassOrMethod(test)) {
                notClassArg.add(test);
            } else {
                notPackageArg.add(test);
            }
        }
        if (!classArg.isEmpty()) {
            runner.addInstrumentationArg(INCLUDE_CLASS_INST_ARGS_KEY,
                    ArrayUtil.join(",", classArg));
        }
        if (!notClassArg.isEmpty()) {
            runner.addInstrumentationArg(EXCLUDE_CLASS_INST_ARGS_KEY,
                    ArrayUtil.join(",", notClassArg));
        }
        if (!packageArg.isEmpty()) {
            runner.addInstrumentationArg(INCLUDE_PACKAGE_INST_ARGS_KEY,
                    ArrayUtil.join(",", packageArg));
        }
        if (!notPackageArg.isEmpty()) {
            runner.addInstrumentationArg(EXCLUDE_PACKAGE_INST_ARGS_KEY,
                    ArrayUtil.join(",", notPackageArg));
        }
        if (!mIncludeAnnotation.isEmpty()) {
            runner.addInstrumentationArg(ANNOTATION_INST_ARGS_KEY,
                    ArrayUtil.join(",", mIncludeAnnotation));
        }
        if (!mExcludeAnnotation.isEmpty()) {
            runner.addInstrumentationArg(NOT_ANNOTATION_INST_ARGS_KEY,
                    ArrayUtil.join(",", mExcludeAnnotation));
        }
        if (mTotalShards > 0 && isShardable()) {
            runner.addInstrumentationArg(SHARD_INDEX_INST_ARGS_KEY, Integer.toString(mShardIndex));
            runner.addInstrumentationArg(NUM_SHARD_INST_ARGS_KEY, Integer.toString(mTotalShards));
        }
        if (mNewRunListenerOrderMode) {
            runner.addInstrumentationArg(
                    NEW_RUN_LISTENER_ORDER_KEY, Boolean.toString(mNewRunListenerOrderMode));
        }
        // Add the listeners received from Options
        addDeviceListener(mExtraDeviceListener);
    }

    /**
     * Push the testFile to the requested destination. This should only be called for a non-null
     * testFile
     *
     * @param testFile file to be pushed from the host to the device.
     * @param destination the path on the device to which testFile is pushed
     * @param listener {@link ITestInvocationListener} to report failures.
     */
    private void pushTestFile(File testFile, String destination, ITestInvocationListener listener)
            throws DeviceNotAvailableException {
        if (!testFile.canRead() || !testFile.isFile()) {
            String message = String.format("Cannot read test file %s", testFile.getAbsolutePath());
            reportEarlyFailure(listener, message);
            throw new IllegalArgumentException(message);
        }
        ITestDevice device = getDevice();
        try {
            if (!device.pushFile(testFile, destination)) {
                String message =
                        String.format(
                                "Failed to push file %s to %s for %s in pushTestFile",
                                testFile.getAbsolutePath(), destination, device.getSerialNumber());
                reportEarlyFailure(listener, message);
                throw new RuntimeException(message);
            }
            // in case the folder was created as 'root' we make is usable.
            device.executeShellCommand(String.format("chown -R shell:shell %s", mTestFilterDir));
        } catch (DeviceNotAvailableException e) {
            reportEarlyFailure(listener, e.getMessage());
            throw e;
        }
    }

    private void removeTestFile(String deviceTestFile) throws DeviceNotAvailableException {
        ITestDevice device = getDevice();
        device.executeShellCommand(String.format("rm %s", deviceTestFile));
    }

    private void reportEarlyFailure(ITestInvocationListener listener, String errorMessage) {
        listener.testRunStarted("AndroidJUnitTest_setupError", 0);
        listener.testRunFailed(errorMessage);
        listener.testRunEnded(0, new HashMap<String, Metric>());
    }

    /**
     * Return if a string is the name of a Class or a Method.
     */
    @VisibleForTesting
    public boolean isClassOrMethod(String filter) {
        if (filter.contains("#")) {
            return true;
        }
        String[] parts = filter.split("\\.");
        if (parts.length > 0) {
            // FIXME Assume java package names starts with lowercase and class names start with
            // uppercase.
            // Return true iff the first character of the last word is uppercase
            // com.android.foobar.Test
            return Character.isUpperCase(parts[parts.length - 1].charAt(0));
        }
        return false;
    }

    /**
     * Helper to return if the runner is one that support sharding.
     */
    private boolean isShardable() {
        return ListInstrumentationParser.SHARDABLE_RUNNERS.contains(getRunnerName());
    }

    /** {@inheritDoc} */
    @Override
    public Collection<IRemoteTest> split(int shardCount) {
        if (!isShardable()) {
            return null;
        }
        if (mMaxShard != null) {
            shardCount = Math.min(shardCount, mMaxShard);
        }
        if (!mIsSharded && shardCount > 1) {
            mIsSharded = true;
            Collection<IRemoteTest> shards = new ArrayList<>(shardCount);
            for (int index = 0; index < shardCount; index++) {
                shards.add(getTestShard(shardCount, index));
            }
            return shards;
        }
        return null;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public IRemoteTest getTestShard(int shardCount, int shardIndex) {
        AndroidJUnitTest shard;
        // ensure we handle runners that extend AndroidJUnitRunner
        try {
            shard = this.getClass().newInstance();
        } catch (InstantiationException | IllegalAccessException e) {
            throw new RuntimeException(e);
        }
        try {
            OptionCopier.copyOptions(this, shard);
        } catch (ConfigurationException e) {
            CLog.e("Failed to copy instrumentation options: %s", e.getMessage());
        }
        shard.mShardIndex = shardIndex;
        shard.mTotalShards = shardCount;
        shard.mIsSharded = true;
        shard.setAbi(getAbi());
        // We approximate the runtime of each shard to be equal since we can't know.
        shard.mRuntimeHint = mRuntimeHint / shardCount;
        return shard;
    }
}
