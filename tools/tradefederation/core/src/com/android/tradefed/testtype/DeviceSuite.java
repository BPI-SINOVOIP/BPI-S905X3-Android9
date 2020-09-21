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
import com.android.tradefed.config.ConfigurationException;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;

import junit.framework.Test;
import junit.framework.TestSuite;

import org.junit.internal.runners.JUnit38ClassRunner;
import org.junit.runner.Runner;
import org.junit.runner.notification.RunNotifier;
import org.junit.runners.Suite;
import org.junit.runners.model.InitializationError;
import org.junit.runners.model.RunnerBuilder;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.Enumeration;
import java.util.HashSet;
import java.util.Set;

/**
 * Extends the JUnit4 container {@link Suite} in order to provide a {@link ITestDevice} to the tests
 * that requires it.
 */
public class DeviceSuite extends Suite
        implements IDeviceTest, IBuildReceiver, IAbiReceiver, ISetOptionReceiver {
    private ITestDevice mDevice;
    private IBuildInfo mBuildInfo;
    private IAbi mAbi;

    @Option(name = HostTest.SET_OPTION_NAME, description = HostTest.SET_OPTION_DESC)
    private Set<String> mKeyValueOptions = new HashSet<>();

    public DeviceSuite(Class<?> klass, RunnerBuilder builder) throws InitializationError {
        super(klass, builder);
    }

    @Override
    public void setDevice(ITestDevice device) {
        mDevice = device;
        for (Runner r : getChildren()) {
            // propagate to runner if it needs a device.
            if (r instanceof IDeviceTest) {
                if (mDevice == null) {
                    throw new IllegalArgumentException("Missing device");
                }
                ((IDeviceTest)r).setDevice(mDevice);
            }
        }
    }

    @Override
    public ITestDevice getDevice() {
        return mDevice;
    }

    @Override
    public void setAbi(IAbi abi) {
        mAbi = abi;
        for (Runner r : getChildren()) {
            // propagate to runner if it needs an abi.
            if (r instanceof IAbiReceiver) {
                ((IAbiReceiver)r).setAbi(mAbi);
            }
        }
    }

    @Override
    public IAbi getAbi() {
        return mAbi;
    }

    @Override
    public void setBuild(IBuildInfo buildInfo) {
        mBuildInfo = buildInfo;
        for (Runner r : getChildren()) {
            // propagate to runner if it needs a buildInfo.
            if (r instanceof IBuildReceiver) {
                if (mBuildInfo == null) {
                    throw new IllegalArgumentException("Missing build information");
                }
                ((IBuildReceiver)r).setBuild(mBuildInfo);
            }
        }
    }

    @Override
    protected void runChild(Runner runner, RunNotifier notifier) {
        // Handle legacy JUnit3 style
        if (runner instanceof JUnit38ClassRunner) {
            JUnit38ClassRunner junit3Runner = (JUnit38ClassRunner) runner;
            try {
                // getTest is private so we use reflection to get the test object.
                Method getTest = junit3Runner.getClass().getDeclaredMethod("getTest");
                getTest.setAccessible(true);
                Test test = (Test) getTest.invoke(junit3Runner);
                if (test instanceof TestSuite) {
                    TestSuite testSuite = (TestSuite) test;
                    Enumeration<Test> testEnum = testSuite.tests();
                    while (testEnum.hasMoreElements()) {
                        Test t = testEnum.nextElement();
                        injectValues(t);
                    }
                } else {
                    injectValues(test);
                }
            } catch (NoSuchMethodException
                    | SecurityException
                    | IllegalAccessException
                    | IllegalArgumentException
                    | InvocationTargetException e) {
                throw new RuntimeException(
                        String.format(
                                "Failed to invoke junit3 runner: %s",
                                junit3Runner.getClass().getName()),
                        e);
            }
        }
        try {
            OptionSetter setter = new OptionSetter(runner);
            for (String kv : mKeyValueOptions) {
                setter.setOptionValue(HostTest.SET_OPTION_NAME, kv);
            }
        } catch (ConfigurationException e) {
            CLog.d("Could not set option set-option on '%s', reason: '%s'", runner, e.getMessage());
        }
        super.runChild(runner, notifier);
    }

    /** Inject the options to the tests. */
    private void injectValues(Object testObj) {
        if (testObj instanceof IDeviceTest) {
            if (mDevice == null) {
                throw new IllegalArgumentException("Missing device");
            }
            ((IDeviceTest) testObj).setDevice(mDevice);
        }
        if (testObj instanceof IBuildReceiver) {
            if (mBuildInfo == null) {
                throw new IllegalArgumentException("Missing build information");
            }
            ((IBuildReceiver) testObj).setBuild(mBuildInfo);
        }
        // We are more flexible about abi information since not always available.
        if (testObj instanceof IAbiReceiver) {
            ((IAbiReceiver) testObj).setAbi(mAbi);
        }
    }
}
