/*
 * Copyright (C) 2018 The Android Open Source Project
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

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.Option;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.testtype.HostTest;
import com.android.tradefed.testtype.IAbi;
import com.android.tradefed.testtype.IAbiReceiver;
import com.android.tradefed.testtype.IBuildReceiver;
import com.android.tradefed.testtype.IDeviceTest;
import com.android.tradefed.testtype.IInvocationContextReceiver;
import com.android.tradefed.testtype.IMultiDeviceTest;
import com.android.tradefed.testtype.ISetOptionReceiver;

import org.junit.runners.Parameterized;
import org.junit.runners.model.FrameworkMethod;
import org.junit.runners.model.InitializationError;
import org.junit.runners.model.Statement;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

import junitparams.JUnitParamsRunner;

/**
 * JUnit4 style parameterized runner for host-side driven parameterized tests.
 *
 * <p>This runner is based on {@link JUnitParamsRunner} and not JUnit4 native {@link Parameterized}
 * but the native parameterized runner is not really good and doesn't allow to run a single method.
 *
 * @see JUnitParamsRunner
 */
public class DeviceParameterizedRunner extends JUnitParamsRunner
        implements IDeviceTest,
                IBuildReceiver,
                IAbiReceiver,
                ISetOptionReceiver,
                IMultiDeviceTest,
                IInvocationContextReceiver {

    @Option(name = HostTest.SET_OPTION_NAME, description = HostTest.SET_OPTION_DESC)
    private List<String> mKeyValueOptions = new ArrayList<>();

    private ITestDevice mDevice;
    private IBuildInfo mBuildInfo;
    private IAbi mAbi;
    private IInvocationContext mContext;
    private Map<ITestDevice, IBuildInfo> mDeviceInfos;

    /**
     * @param klass
     * @throws InitializationError
     */
    public DeviceParameterizedRunner(Class<?> klass) throws InitializationError {
        super(klass);
    }

    @Override
    protected Statement methodInvoker(FrameworkMethod method, Object testObj) {
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
        if (testObj instanceof IMultiDeviceTest) {
            ((IMultiDeviceTest) testObj).setDeviceInfos(mDeviceInfos);
        }
        if (testObj instanceof IInvocationContextReceiver) {
            ((IInvocationContextReceiver) testObj).setInvocationContext(mContext);
        }
        // Set options of test object
        HostTest.setOptionToLoadedObject(testObj, mKeyValueOptions);
        return super.methodInvoker(method, testObj);
    }

    @Override
    public void setDevice(ITestDevice device) {
        mDevice = device;
    }

    @Override
    public ITestDevice getDevice() {
        return mDevice;
    }

    @Override
    public void setAbi(IAbi abi) {
        mAbi = abi;
    }

    @Override
    public IAbi getAbi() {
        return mAbi;
    }

    @Override
    public void setBuild(IBuildInfo buildInfo) {
        mBuildInfo = buildInfo;
    }

    @Override
    public void setInvocationContext(IInvocationContext invocationContext) {
        mContext = invocationContext;
    }

    @Override
    public void setDeviceInfos(Map<ITestDevice, IBuildInfo> deviceInfos) {
        mDeviceInfos = deviceInfos;
    }
}
