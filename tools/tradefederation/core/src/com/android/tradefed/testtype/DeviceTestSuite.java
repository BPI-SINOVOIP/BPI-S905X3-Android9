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
package com.android.tradefed.testtype;

import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.result.ITestInvocationListener;

import junit.framework.Test;
import junit.framework.TestResult;
import junit.framework.TestSuite;

/**
 *  Helper JUnit test suite that provides the {@link IRemoteTest} and {@link IDeviceTest} services.
 */
public class DeviceTestSuite extends TestSuite implements IDeviceTest, IRemoteTest {

    private ITestDevice mDevice = null;

    public DeviceTestSuite(Class<?> testClass) {
        super(testClass);
    }

    public DeviceTestSuite() {
        super();
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
     * {@inheritDoc}
     */
    @Override
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
        JUnitRunUtil.runTest(listener, this);
    }

    /**
     * Adds the tests from the given class to the suite
     */
    @SuppressWarnings("rawtypes")
    @Override
    public void addTestSuite(Class testClass) {
        addTest(new DeviceTestSuite(testClass));
    }

    /**
     * Overrides parent method to pass in device to included test
     */
    @Override
    public void runTest(Test test, TestResult result) {
        if (test instanceof IDeviceTest) {
            IDeviceTest deviceTest = (IDeviceTest)test;
            deviceTest.setDevice(mDevice);
        }
        test.run(result);
    }
}
