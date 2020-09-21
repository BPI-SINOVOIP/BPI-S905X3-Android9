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
package com.android.tradefed.testtype.testdefs;

import com.android.ddmlib.Log;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.Option.Importance;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.testtype.IDeviceTest;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.testtype.IResumableTest;
import com.android.tradefed.testtype.IShardableTest;
import com.android.tradefed.testtype.InstrumentationTest;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.proto.TfMetricProtoUtil;
import com.android.tradefed.util.xml.AbstractXmlParser.ParseException;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Queue;

/**
 * Runs a set of instrumentation test's defined in test_defs.xml files.
 * <p/>
 * The test definition files can either be one or more files on local file system, and/or one or
 * more files stored on the device under test.
 */
@OptionClass(alias = "xml-defs")
public class XmlDefsTest implements IDeviceTest, IResumableTest,
        IShardableTest {

    private static final String LOG_TAG = "XmlDefsTest";

    /** the metric key name for the test coverage target value */
    // TODO: move this to a more generic location
    public static final String COVERAGE_TARGET_KEY = "coverage_target";

    private ITestDevice mDevice;

    /**
     * @deprecated use shell-timeout or test-timeout instead.
     */
    @Deprecated
    @Option(name = "timeout",
            description="Deprecated - Use \"shell-timeout\" or \"test-timeout\" instead.")
    private Integer mTimeout = null;

    @Option(name = "shell-timeout",
            description="The defined timeout (in milliseconds) is used as a maximum waiting time "
                    + "when expecting the command output from the device. At any time, if the "
                    + "shell command does not output anything for a period longer than defined "
                    + "timeout the TF run terminates. For no timeout, set to 0.")
    private long mShellTimeout = 10 * 60 * 1000;  // default to 10 minutes

    @Option(name = "test-timeout",
            description="Sets timeout (in milliseconds) that will be applied to each test. In the "
                    + "event of a test timeout it will log the results and proceed with executing "
                    + "the next test. For no timeout, set to 0.")
    private int mTestTimeout = 10 * 60 * 1000;  // default to 10 minutes

    @Option(name = "size",
            description = "Restrict tests to a specific test size. " +
            "One of 'small', 'medium', 'large'",
            importance = Importance.IF_UNSET)
    private String mTestSize = null;

    @Option(name = "rerun",
            description = "Rerun unexecuted tests individually on same device if test run " +
            "fails to complete.")
    private boolean mIsRerunMode = true;

    @Option(name = "resume",
            description = "Schedule unexecuted tests for resumption on another device " +
            "if first device becomes unavailable.")
    private boolean mIsResumeMode = false;

    @Option(name = "local-file-path",
            description = "local file path to test_defs.xml file to run.")
    private Collection<File> mLocalFiles = new ArrayList<File>();

    @Option(name = "device-file-path",
            description = "file path on device to test_defs.xml file to run.",
            importance = Importance.IF_UNSET)
    private Collection<String> mRemotePaths = new ArrayList<String>();

    @Option(name = "send-coverage",
            description = "Send coverage target info to test listeners.")
    private boolean mSendCoverage = true;

    @Option(name = "num-shards",
            description = "Shard this test into given number of separately runnable chunks.")
    private int mNumShards = 0;

    private List<InstrumentationTest> mTests = null;

    public XmlDefsTest() {
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public ITestDevice getDevice() {
        return mDevice;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setDevice(ITestDevice device) {
        mDevice = device;
    }

    /**
     * Adds a remote test def file path.
     * <p/>
     * Exposed for unit testing.
     */
    void addRemoteFilePath(String path) {
        mRemotePaths.add(path);
    }

    /**
     * Adds a local test def file path.
     * <p/>
     * Exposed for unit testing.
     */
    void addLocalFilePath(File file) {
        mLocalFiles.add(file);
    }

    /**
     * Set the send coverage flag.
     * <p/>
     * Exposed for unit testing.
     */
    void setSendCoverage(boolean sendCoverage) {
        mSendCoverage = sendCoverage;
    }

    /**
     * Sets the number of shards test should be split into
     * <p/>
     * Exposed for unit testing.
     */
    void setNumShards(int shards) {
        mNumShards = shards;
    }

    /**
     * Gets the list of parsed {@link InstrumentationTest}s contained within.
     * <p/>
     * Exposed for unit testing.
     */
    List<InstrumentationTest> getTests() {
        return mTests;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
        if (getDevice() == null) {
            throw new IllegalArgumentException("Device has not been set");
        }
        buildTests();
        doRun(listener);
    }

    /**
     * Build the list of tests to run from the xml files, if not done already.
     * @throws DeviceNotAvailableException
     */
    private void buildTests() throws DeviceNotAvailableException {
        if (mTests == null) {
            if (mLocalFiles.isEmpty() && mRemotePaths.isEmpty()) {
                throw new IllegalArgumentException("No test definition files (local-file-path or " +
                        "device-file-path) have been provided.");
            }
            XmlDefsParser parser = createParser();
            for (File testDefFile : mLocalFiles) {
                parseFile(parser, testDefFile);
            }
            for (File testDefFile : getRemoteFile(mRemotePaths)) {
                try {
                    parseFile(parser, testDefFile);
                } finally {
                    testDefFile.delete();
                }
            }

            mTests = new LinkedList<InstrumentationTest>();
            for (InstrumentationTestDef def : parser.getTestDefs()) {
                // only run continuous for now. Consider making this configurable
                if (def.isContinuous()) {
                    InstrumentationTest test = createInstrumentationTest();

                    test.setDevice(getDevice());
                    test.setPackageName(def.getPackage());
                    if (def.getRunner() != null) {
                        test.setRunnerName(def.getRunner());
                    }
                    if (def.getClassName() != null) {
                        test.setClassName(def.getClassName());
                    }
                    test.setRerunMode(mIsRerunMode);
                    test.setResumeMode(mIsResumeMode);
                    test.setTestSize(getTestSize());
                    if (mTimeout != null) {
                        LogUtil.CLog
                                .w("\"timeout\" argument is deprecated and should not be used! \"shell-timeout\""
                                        + " argument value is overwritten with %d ms", mTimeout);
                        setShellTimeout(mTimeout);
                    }
                    test.setShellTimeout(getShellTimeout());
                    test.setTestTimeout(getTestTimeout());
                    test.setCoverageTarget(def.getCoverageTarget());
                    mTests.add(test);
                }
            }
        }
    }

    /**
     * Parse the given xml def file
     *
     * @param parser
     * @param testDefFile
     */
    private void parseFile(XmlDefsParser parser, File testDefFile) {
        try {
            Log.i(LOG_TAG, String.format("Parsing test def file %s",
                    testDefFile.getAbsolutePath()));
            parser.parse(new FileInputStream(testDefFile));
        } catch (FileNotFoundException e) {
            Log.e(LOG_TAG, String.format("Could not find test def file %s",
                    testDefFile.getAbsolutePath()));
        } catch (ParseException e) {
            Log.e(LOG_TAG, String.format("Could not parse test def file %s: %s",
                    testDefFile.getAbsolutePath(), e.getMessage()));
        }
    }

    /**
     * Run the previously built tests.
     *
     * @param listener the {@link ITestInvocationListener}
     * @throws DeviceNotAvailableException
     */
    private void doRun(ITestInvocationListener listener) throws DeviceNotAvailableException {
        while (!mTests.isEmpty()) {
            InstrumentationTest test = mTests.get(0);

            Log.d(LOG_TAG, String.format("Running test %s on %s", test.getPackageName(),
                        getDevice().getSerialNumber()));

            if (mSendCoverage && test.getCoverageTarget() != null) {
                sendCoverage(test.getPackageName(), test.getCoverageTarget(), listener);
            }
            test.setDevice(getDevice());
            test.run(listener);
            // test completed, remove from list
            mTests.remove(0);
        }
    }

    /**
     * Forwards the tests coverage target info as a test metric.
     *
     * @param packageName
     * @param coverageTarget
     * @param listener
     */
    private void sendCoverage(String packageName, String coverageTarget,
            ITestInvocationListener listener) {
        HashMap<String, Metric> coverageMetric = new HashMap<>(1);
        coverageMetric.put(COVERAGE_TARGET_KEY, TfMetricProtoUtil.stringToMetric(coverageTarget));
        listener.testRunStarted(packageName, 0);
        listener.testRunEnded(0, coverageMetric);
    }

    /**
     * Retrieves a set of files from device into temporary files on local filesystem.
     *
     * @param remoteFilePaths
     */
    private Collection<File> getRemoteFile(Collection<String> remoteFilePaths)
            throws DeviceNotAvailableException {
        Collection<File> files = new ArrayList<File>();
        if (getDevice() == null) {
            Log.d(LOG_TAG, "Device not set, skipping collection of remote file");
            return files;
        }
        for (String remoteFilePath : remoteFilePaths) {
            try {
                File tmpFile = FileUtil.createTempFile("test_defs_", ".xml");
                getDevice().pullFile(remoteFilePath, tmpFile);
                files.add(tmpFile);
            } catch (IOException e) {
                Log.e(LOG_TAG, "Failed to create temp file");
                Log.e(LOG_TAG, e);
            }
        }
        return files;
    }

    void setShellTimeout(long timeout) {
        mShellTimeout = timeout;
    }

    long getShellTimeout() {
        return mShellTimeout;
    }

    int getTestTimeout() {
        return mTestTimeout;
    }

    String getTestSize() {
        return mTestSize;
    }

    /**
     * Creates the {@link XmlDefsParser} to use. Exposed for unit testing.
     */
    XmlDefsParser createParser() {
        return new XmlDefsParser();
    }

    /**
     * Creates the {@link InstrumentationTest} to use. Exposed for unit testing.
     */
    InstrumentationTest createInstrumentationTest() {
        return new InstrumentationTest();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean isResumable() {
        // hack to not resume if tests were never run
        // TODO: fix this properly in TestInvocation
        if (mTests == null) {
            return false;
        }
        return mIsResumeMode;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public Collection<IRemoteTest> split() {
        if (mLocalFiles.isEmpty()) {
            Log.w(LOG_TAG, "sharding is only supported if local xml files have been specified");
            return null;
        }
        if (mNumShards <= 1) {
            return null;
        }

        try {
            buildTests();
        } catch (DeviceNotAvailableException e) {
            // should never happen
        }
        if (mTests.size() <= 1) {
            Log.w(LOG_TAG, "no tests to shard!");
            return null;
        }

        // treat shardQueue as a circular queue, to sequentially distribute tests among shards
        Queue<IRemoteTest> shardQueue = new LinkedList<IRemoteTest>();
        // don't create more shards than the number of tests we have!
        for (int i = 0; i < mNumShards && i < mTests.size(); i++) {
            XmlDefsTest shard = new XmlDefsTest();
            shard.mTests = new LinkedList<InstrumentationTest>();
            shardQueue.add(shard);
        }
        while (!mTests.isEmpty()) {
            InstrumentationTest test = mTests.remove(0);
            XmlDefsTest shard = (XmlDefsTest)shardQueue.poll();
            shard.mTests.add(test);
            shardQueue.add(shard);
        }
        return shardQueue;
    }
}
