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
package com.android.tradefed.testtype.suite;

import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionCopier;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ByteArrayInputStreamSource;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.testtype.IAbi;
import com.android.tradefed.testtype.IAbiReceiver;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.testtype.IRuntimeHintProvider;
import com.android.tradefed.testtype.IShardableTest;
import com.android.tradefed.testtype.ITestCollector;
import com.android.tradefed.testtype.ITestFilterReceiver;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Set;

/** A test Stub that can be used to fake some runs for suite's testing. */
public class TestSuiteStub
        implements IRemoteTest,
                IAbiReceiver,
                IRuntimeHintProvider,
                ITestCollector,
                ITestFilterReceiver,
                IShardableTest {

    @Option(name = "module")
    private String mModule;

    @Option(name = "foo")
    protected String mFoo;

    @Option(name = "blah")
    protected String mBlah;

    @Option(name = "report-test")
    protected boolean mReportTest = false;

    @Option(name = "run-complete")
    protected boolean mIsComplete = true;

    @Option(name = "test-fail")
    protected boolean mDoesOneTestFail = true;

    @Option(name = "internal-retry")
    protected boolean mRetry = false;

    @Option(name = "throw-device-not-available")
    protected boolean mThrow = false;

    @Option(name = "log-fake-files")
    protected boolean mLogFiles = false;

    protected List<TestDescription> mShardedTestToRun;
    protected Integer mShardIndex = null;

    /** Tests attempt. */
    private void testAttempt(ITestInvocationListener listener) throws DeviceNotAvailableException {
        listener.testRunStarted(mModule, 3);
        TestDescription tid = new TestDescription("TestStub", "test1");
        listener.testStarted(tid);
        if (mLogFiles) {
            listener.testLog(
                    tid.toString() + "-file",
                    LogDataType.LOGCAT,
                    new ByteArrayInputStreamSource("test".getBytes()));
        }
        listener.testEnded(tid, new HashMap<String, Metric>());

        if (mIsComplete) {
            // possibly skip this one to create some not_executed case.
            TestDescription tid2 = new TestDescription("TestStub", "test2");
            listener.testStarted(tid2);
            if (mThrow) {
                throw new DeviceNotAvailableException();
            }
            if (mLogFiles) {
                listener.testLog(
                        tid2.toString() + "-file",
                        LogDataType.BUGREPORT,
                        new ByteArrayInputStreamSource("test".getBytes()));
            }
            listener.testEnded(tid2, new HashMap<String, Metric>());
        }

        TestDescription tid3 = new TestDescription("TestStub", "test3");
        listener.testStarted(tid3);
        if (mDoesOneTestFail) {
            listener.testFailed(tid3, "ouch this is bad.");
        }
        if (mLogFiles) {
            listener.testLog(
                    tid3.toString() + "-file",
                    LogDataType.BUGREPORT,
                    new ByteArrayInputStreamSource("test".getBytes()));
        }
        listener.testEnded(tid3, new HashMap<String, Metric>());

        if (mLogFiles) {
            // One file logged at run level
            listener.testLog(
                    mModule + "-file",
                    LogDataType.EAR,
                    new ByteArrayInputStreamSource("test".getBytes()));
        }
        listener.testRunEnded(0, new HashMap<String, Metric>());
    }

    /** {@inheritDoc} */
    @Override
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
        if (mReportTest) {
            if (mShardedTestToRun == null) {
                if (!mRetry) {
                    testAttempt(listener);
                } else {
                    // We fake an internal retry by calling testRunStart/Ended again.
                    listener.testRunStarted(mModule, 3);
                    listener.testRunEnded(0, new HashMap<String, Metric>());
                    testAttempt(listener);
                }
            } else {
                // Run the shard
                if (mDoesOneTestFail) {
                    listener.testRunStarted(mModule, mShardedTestToRun.size() + 1);
                } else {
                    listener.testRunStarted(mModule, mShardedTestToRun.size());
                }

                if (mIsComplete) {
                    for (TestDescription tid : mShardedTestToRun) {
                        listener.testStarted(tid);
                        listener.testEnded(tid, new HashMap<String, Metric>());
                    }
                } else {
                    TestDescription tid = mShardedTestToRun.get(0);
                    listener.testStarted(tid);
                    listener.testEnded(tid, new HashMap<String, Metric>());
                }

                if (mDoesOneTestFail) {
                    TestDescription tid = new TestDescription("TestStub", "failed" + mShardIndex);
                    listener.testStarted(tid);
                    listener.testFailed(tid, "shard failed this one.");
                    listener.testEnded(tid, new HashMap<String, Metric>());
                }
                listener.testRunEnded(0, new HashMap<String, Metric>());
            }
        }
    }

    @Override
    public Collection<IRemoteTest> split(int shardCountHint) {
        if (mShardedTestToRun == null) {
            return null;
        }
        Collection<IRemoteTest> listTest = new ArrayList<>();
        for (TestDescription id : mShardedTestToRun) {
            TestSuiteStub stub = new TestSuiteStub();
            OptionCopier.copyOptionsNoThrow(this, stub);
            stub.mShardedTestToRun = new ArrayList<>();
            stub.mShardedTestToRun.add(id);
            listTest.add(stub);
        }
        return listTest;
    }

    @Override
    public void setAbi(IAbi abi) {
        // Do nothing
    }

    @Override
    public IAbi getAbi() {
        return null;
    }

    @Override
    public long getRuntimeHint() {
        return 1L;
    }

    @Override
    public void setCollectTestsOnly(boolean shouldCollectTest) {
        // Do nothing
    }

    @Override
    public void addIncludeFilter(String filter) {}

    @Override
    public void addAllIncludeFilters(Set<String> filters) {}

    @Override
    public void addExcludeFilter(String filter) {}

    @Override
    public void addAllExcludeFilters(Set<String> filters) {}
}
