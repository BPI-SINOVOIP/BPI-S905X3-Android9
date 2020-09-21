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
package com.android.tradefed.device.metric;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.Option;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.result.TestDescription;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * Base implementation of {@link IMetricCollector} that allows to start and stop collection on
 * {@link #onTestRunStart(DeviceMetricData)} and {@link #onTestRunEnd(DeviceMetricData, Map)}.
 */
public class BaseDeviceMetricCollector implements IMetricCollector {

    public static final String TEST_CASE_INCLUDE_GROUP_OPTION = "test-case-include-group";
    public static final String TEST_CASE_EXCLUDE_GROUP_OPTION = "test-case-exclude-group";

    @Option(
        name = TEST_CASE_INCLUDE_GROUP_OPTION,
        description =
                "Specify a group to include as part of the collection,"
                        + "group can be specified via @MetricOption. Can be repeated."
                        + "Usage: @MetricOption(group = \"groupname\") to your test methods, then"
                        + "use --test-case-include-anotation groupename to only run your group."
    )
    private List<String> mTestCaseIncludeAnnotationGroup = new ArrayList<>();

    @Option(
        name = TEST_CASE_EXCLUDE_GROUP_OPTION,
        description =
                "Specify a group to exclude from the metric collection,"
                        + "group can be specified via @MetricOption. Can be repeated."
    )
    private List<String> mTestCaseExcludeAnnotationGroup = new ArrayList<>();

    private IInvocationContext mContext;
    private ITestInvocationListener mForwarder;
    private DeviceMetricData mRunData;
    private DeviceMetricData mTestData;
    private String mTag;
    private String mRunName;
    /**
     * Variable for whether or not to skip the collection of one test case because it was filtered.
     */
    private boolean mSkipTestCase = false;

    @Override
    public ITestInvocationListener init(
            IInvocationContext context, ITestInvocationListener listener) {
        mContext = context;
        mForwarder = listener;
        return this;
    }

    @Override
    public final List<ITestDevice> getDevices() {
        return mContext.getDevices();
    }

    @Override
    public final List<IBuildInfo> getBuildInfos() {
        return mContext.getBuildInfos();
    }

    @Override
    public final ITestInvocationListener getInvocationListener() {
        return mForwarder;
    }

    @Override
    public void onTestRunStart(DeviceMetricData runData) {
        // Does nothing
    }

    @Override
    public void onTestRunEnd(
            DeviceMetricData runData, final Map<String, Metric> currentRunMetrics) {
        // Does nothing
    }

    @Override
    public void onTestStart(DeviceMetricData testData) {
        // Does nothing
    }

    @Override
    public void onTestEnd(
            DeviceMetricData testData, final Map<String, Metric> currentTestCaseMetrics) {
        // Does nothing
    }

    /** =================================== */
    /** Invocation Listeners for forwarding */
    @Override
    public final void invocationStarted(IInvocationContext context) {
        mForwarder.invocationStarted(context);
    }

    @Override
    public final void invocationFailed(Throwable cause) {
        mForwarder.invocationFailed(cause);
    }

    @Override
    public final void invocationEnded(long elapsedTime) {
        mForwarder.invocationEnded(elapsedTime);
    }

    @Override
    public final void testLog(String dataName, LogDataType dataType, InputStreamSource dataStream) {
        mForwarder.testLog(dataName, dataType, dataStream);
    }

    /** Test run callbacks */
    @Override
    public final void testRunStarted(String runName, int testCount) {
        mRunData = new DeviceMetricData(mContext);
        mRunName = runName;
        try {
            onTestRunStart(mRunData);
        } catch (Throwable t) {
            // Prevent exception from messing up the status reporting.
            CLog.e(t);
        }
        mForwarder.testRunStarted(runName, testCount);
    }

    @Override
    public final void testRunFailed(String errorMessage) {
        mForwarder.testRunFailed(errorMessage);
    }

    @Override
    public final void testRunStopped(long elapsedTime) {
        mForwarder.testRunStopped(elapsedTime);
    }

    @Override
    public final void testRunEnded(long elapsedTime, HashMap<String, Metric> runMetrics) {
        try {
            onTestRunEnd(mRunData, runMetrics);
            mRunData.addToMetrics(runMetrics);
        } catch (Throwable t) {
            // Prevent exception from messing up the status reporting.
            CLog.e(t);
        }
        mForwarder.testRunEnded(elapsedTime, runMetrics);
    }

    /** Test cases callbacks */
    @Override
    public final void testStarted(TestDescription test) {
        testStarted(test, System.currentTimeMillis());
    }

    @Override
    public final void testStarted(TestDescription test, long startTime) {
        mTestData = new DeviceMetricData(mContext);
        mSkipTestCase = shouldSkip(test);
        if (!mSkipTestCase) {
            try {
                onTestStart(mTestData);
            } catch (Throwable t) {
                // Prevent exception from messing up the status reporting.
                CLog.e(t);
            }
        }
        mForwarder.testStarted(test, startTime);
    }

    @Override
    public final void testFailed(TestDescription test, String trace) {
        mForwarder.testFailed(test, trace);
    }

    @Override
    public final void testEnded(TestDescription test, HashMap<String, Metric> testMetrics) {
        testEnded(test, System.currentTimeMillis(), testMetrics);
    }

    @Override
    public final void testEnded(
            TestDescription test, long endTime, HashMap<String, Metric> testMetrics) {
        if (!mSkipTestCase) {
            try {
                onTestEnd(mTestData, testMetrics);
                mTestData.addToMetrics(testMetrics);
            } catch (Throwable t) {
                // Prevent exception from messing up the status reporting.
                CLog.e(t);
            }
        } else {
            CLog.d("Skipping %s collection for %s.", this.getClass().getName(), test.toString());
        }
        mForwarder.testEnded(test, endTime, testMetrics);
    }

    @Override
    public final void testAssumptionFailure(TestDescription test, String trace) {
        mForwarder.testAssumptionFailure(test, trace);
    }

    @Override
    public final void testIgnored(TestDescription test) {
        mForwarder.testIgnored(test);
    }

    /**
     * Sets the {@code mTag} of the collector. It can be used to specify the interval of the
     * collector.
     *
     * @param tag the unique identifier of the collector.
     */
    public void setTag(String tag) {
        mTag = tag;
    }

    /**
     * Returns the identifier {@code mTag} of the collector.
     *
     * @return mTag, the unique identifier of the collector.
     */
    public String getTag() {
        return mTag;
    }

    /**
     * Returns the name of test run {@code mRunName} that triggers the collector.
     *
     * @return mRunName, the current test run name.
     */
    public String getRunName() {
        return mRunName;
    }

    /**
     * Helper to decide if a test case should or not run the collector method associated.
     *
     * @param desc the identifier of the test case.
     * @return True the collector should be skipped. False otherwise.
     */
    private boolean shouldSkip(TestDescription desc) {
        Set<String> testCaseGroups = new HashSet<>();
        if (desc.getAnnotation(MetricOption.class) != null) {
            String groupName = desc.getAnnotation(MetricOption.class).group();
            testCaseGroups.addAll(Arrays.asList(groupName.split(",")));
        } else {
            // Add empty group name for default case.
            testCaseGroups.add("");
        }
        // Exclusion has priority: if any of the groups is excluded, exclude the test case.
        for (String groupName : testCaseGroups) {
            if (mTestCaseExcludeAnnotationGroup.contains(groupName)) {
                return true;
            }
        }
        // Inclusion filter: if any of the group is included, include the test case.
        for (String includeGroupName : mTestCaseIncludeAnnotationGroup) {
            if (testCaseGroups.contains(includeGroupName)) {
                return false;
            }
        }

        // If we had filters and did not match any groups
        if (!mTestCaseIncludeAnnotationGroup.isEmpty()) {
            return true;
        }
        return false;
    }
}
