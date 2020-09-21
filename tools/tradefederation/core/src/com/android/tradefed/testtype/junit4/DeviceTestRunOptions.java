/*
 * Copyright (C) 2017 Google Inc.
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
package com.android.tradefed.testtype.junit4;

import com.android.tradefed.device.ITestDevice;

/** A builder class for options related to running device tests through BaseHostJUnit4Test. */
public class DeviceTestRunOptions {
    private ITestDevice mDevice; // optional
    private String mRunner = BaseHostJUnit4Test.AJUR_RUNNER; // optional
    private final String mPackageName; // required

    private String mTestClassName; // optional
    private String mTestMethodName; // optional
    private Integer mUserId; // optional
    private Long mTestTimeoutMs = BaseHostJUnit4Test.DEFAULT_TEST_TIMEOUT_MS; // optional
    private Long mMaxTimeToOutputMs; // optional
    private Long mMaxInstrumentationTimeoutMs; // optional
    private boolean mCheckResults = true; // optional
    private boolean mDisableHiddenApiCheck = false; // optional

    public DeviceTestRunOptions(String packageName) {
        this.mPackageName = packageName;
    }

    public ITestDevice getDevice() {
        return mDevice;
    }

    public DeviceTestRunOptions setDevice(ITestDevice device) {
        this.mDevice = device;
        return this;
    }

    public String getRunner() {
        return mRunner;
    }

    /**
     * Sets the instrumentation runner that should be used to run the instrumentation. Default
     * runner is 'android.support.test.runner.AndroidJUnitRunner'. Optional.
     */
    public DeviceTestRunOptions setRunner(String runner) {
        this.mRunner = runner;
        return this;
    }

    public String getPackageName() {
        return mPackageName;
    }

    public String getTestClassName() {
        return mTestClassName;
    }

    /**
     * Sets the classname that the instrumentation should run. The instrumentation will be filtered
     * to only run the class. Can be used with {@link #setTestMethodName(String)}. Optional.
     */
    public DeviceTestRunOptions setTestClassName(String testClassName) {
        this.mTestClassName = testClassName;
        return this;
    }

    public String getTestMethodName() {
        return mTestMethodName;
    }

    /**
     * Sets the method name that the instrumentation should run. Requires {@link
     * #setTestClassName(String)} to be set in order to work properly. Optional.
     */
    public DeviceTestRunOptions setTestMethodName(String testMethodName) {
        this.mTestMethodName = testMethodName;
        return this;
    }

    public Integer getUserId() {
        return mUserId;
    }

    /** Sets the user id against which the instrumentation should run. Optional. */
    public DeviceTestRunOptions setUserId(Integer userId) {
        this.mUserId = userId;
        return this;
    }

    public Long getTestTimeoutMs() {
        return mTestTimeoutMs;
    }

    /**
     * Sets the maximum time (in milliseconds) a test can run before being interrupted. Set to 0 for
     * no timeout. Optional.
     */
    public DeviceTestRunOptions setTestTimeoutMs(Long testTimeoutMs) {
        this.mTestTimeoutMs = testTimeoutMs;
        return this;
    }

    public Long getMaxTimeToOutputMs() {
        return mMaxTimeToOutputMs;
    }

    /**
     * Sets the maximum time (in milliseconds) the instrumentation can stop outputting before being
     * stopped. Set to 0 for no timeout. Optional.
     */
    public DeviceTestRunOptions setMaxTimeToOutputMs(Long maxTimeToOutputMs) {
        this.mMaxTimeToOutputMs = maxTimeToOutputMs;
        return this;
    }

    public Long getMaxInstrumentationTimeoutMs() {
        return mMaxInstrumentationTimeoutMs;
    }

    /**
     * Sets the maximum time (in milliseconds) the complete instrumentation will have to run and
     * complete. Set to 0 for no timeout. Optional.
     */
    public DeviceTestRunOptions setMaxInstrumentationTimeoutMs(Long maxInstrumentationTimeoutMs) {
        this.mMaxInstrumentationTimeoutMs = maxInstrumentationTimeoutMs;
        return this;
    }

    public boolean shouldCheckResults() {
        return mCheckResults;
    }

    /**
     * Sets whether or not the results of the instrumentation run should be checked and ensure no
     * failures occured.
     */
    public DeviceTestRunOptions setCheckResults(boolean checkResults) {
        this.mCheckResults = checkResults;
        return this;
    }

    /**
     * sets whether or not to add the --no-hidden-api-checks to the 'am instrument' used from the
     * host side.
     */
    public DeviceTestRunOptions setDisableHiddenApiCheck(boolean disableHiddenApiCheck) {
        this.mDisableHiddenApiCheck = disableHiddenApiCheck;
        return this;
    }

    public boolean isHiddenApiCheckDisabled() {
        return mDisableHiddenApiCheck;
    }
}
