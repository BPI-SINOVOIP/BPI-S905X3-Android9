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

import android.longevity.core.LongevitySuite;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.ConfigurationException;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.testtype.DeviceJUnit4ClassRunner;
import com.android.tradefed.testtype.HostTest;
import com.android.tradefed.testtype.IAbi;
import com.android.tradefed.testtype.IAbiReceiver;
import com.android.tradefed.testtype.IBuildReceiver;
import com.android.tradefed.testtype.IDeviceTest;
import com.android.tradefed.testtype.IInvocationContextReceiver;
import com.android.tradefed.testtype.IMultiDeviceTest;
import com.android.tradefed.testtype.ISetOptionReceiver;
import com.android.tradefed.testtype.junit4.builder.DeviceJUnit4ClassRunnerBuilder;
import com.android.tradefed.util.TimeVal;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Map;
import java.util.Set;
import org.junit.runner.Description;
import org.junit.runner.Runner;
import org.junit.runner.notification.RunNotifier;
import org.junit.runners.Suite.SuiteClasses;
import org.junit.runners.model.InitializationError;

/**
 * A JUnit4-based {@link Runner} that composes tests run with {@link DeviceJUnit4ClassRunner} into a
 * {@link LongevitySuite}, which runs tests repeatedly to induce stress and randomness. Tests should
 * specify this inside a @RunWith annotation with a list of {@link SuiteClasses} to include.
 *
 * <p>
 *
 * @see LongevitySuite
 */
@OptionClass(alias = "longevity-runner")
public class LongevityHostRunner extends Runner
        implements IDeviceTest,
                IBuildReceiver,
                IAbiReceiver,
                IMultiDeviceTest,
                IInvocationContextReceiver,
                ISetOptionReceiver {
    static final String ITERATIONS_OPTION = "iterations";
    static final String TOTAL_TIMEOUT_OPTION = "total-timeout";

    private ITestDevice mDevice;
    private IBuildInfo mBuildInfo;
    private IAbi mAbi;
    private IInvocationContext mContext;
    private Map<ITestDevice, IBuildInfo> mDeviceInfos;

    @Option(name = HostTest.SET_OPTION_NAME, description = HostTest.SET_OPTION_DESC)
    private Set<String> mKeyValueOptions = new HashSet<>();

    @Option(
        name = ITERATIONS_OPTION,
        description = "The number of times to repeat the tests in this suite."
    )
    private int mIterations = 1;

    @Option(
        name = TOTAL_TIMEOUT_OPTION,
        description = "The overall timeout for this suite.",
        isTimeVal = true
    )
    private long mTotalTimeoutMsec = 30 * 60 * 1000; // 30 min

    private Class<?> mSuiteKlass;

    public LongevityHostRunner(Class<?> klass) {
        mSuiteKlass = klass;
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

    private Map<String, String> getOptions() {
        // Note: *TS does not have a convenient mechanism for sending options to test runners. To
        // get around that issue, this runner will also pull any options intended for tests that
        // match the runner options and override it's own options with these.
        //
        // TODO: Remove this code when better option passing is available.
        Iterator<String> iterator = mKeyValueOptions.iterator();
        while (iterator.hasNext()) {
            String kvPair = iterator.next();
            if (kvPair.contains(ITERATIONS_OPTION)) {
                try {
                    mIterations = Integer.parseInt(kvPair.split(":")[1]);
                    // Remove this from options passed down, due to aggressive unused option errors.
                    iterator.remove();
                } catch (ArrayIndexOutOfBoundsException | NumberFormatException e) {
                    throw new RuntimeException(
                            String.format(
                                    "Malformed input, %s, should be iterations:<int>.", kvPair));
                }
            } else if (kvPair.contains(TOTAL_TIMEOUT_OPTION)) {
                try {
                    mTotalTimeoutMsec = TimeVal.fromString(kvPair.split(":")[1]);
                    // Remove this from options passed down, due to aggressive unused option errors.
                    iterator.remove();
                } catch (ArrayIndexOutOfBoundsException | NumberFormatException e) {
                    throw new RuntimeException(
                            String.format(
                                    "Malformed input, %s, should be total-timeout:<long|time>.",
                                    kvPair));
                }
            }
        }
        // Construct runner option map from TF options.
        Map<String, String> options = new HashMap<>();
        options.put(ITERATIONS_OPTION, String.valueOf(mIterations));
        options.put(TOTAL_TIMEOUT_OPTION, String.valueOf(mTotalTimeoutMsec));
        return options;
    }

    /** Constructs an underlying {@link LongevitySuite} using options from this {@link Runner}. */
    private LongevitySuite constructSuite() {
        // Create the suite and return the suite, verifying construction.
        try {
            return new LongevitySuite(
                    mSuiteKlass, new DeviceJUnit4ClassRunnerBuilder(), getOptions());
        } catch (InitializationError e) {
            throw new RuntimeException("Unable to construct longevity suite", e);
        }
    }

    @Override
    public Description getDescription() {
        return constructSuite().getDescription();
    }

    @Override
    public void run(RunNotifier notifier) {
        // Create and verify the suite.
        LongevitySuite suite = constructSuite();
        // Pass the feature set to the runners.
        for (Runner child : suite.getRunners()) {
            if (child instanceof IDeviceTest) {
                ((IDeviceTest) child).setDevice(mDevice);
            }
            if (child instanceof IAbiReceiver) {
                ((IAbiReceiver) child).setAbi(mAbi);
            }
            if (child instanceof IBuildReceiver) {
                ((IBuildReceiver) child).setBuild(mBuildInfo);
            }
            if (child instanceof IInvocationContextReceiver) {
                ((IInvocationContextReceiver) child).setInvocationContext(mContext);
            }
            if (child instanceof IMultiDeviceTest) {
                ((IMultiDeviceTest) child).setDeviceInfos(mDeviceInfos);
            }
            try {
                OptionSetter setter = new OptionSetter(child);
                for (String kvPair : mKeyValueOptions) {
                    setter.setOptionValue(HostTest.SET_OPTION_NAME, kvPair);
                }
            } catch (ConfigurationException e) {
                throw new RuntimeException(e);
            }
        }
        // Run the underlying longevity suite.
        suite.run(notifier);
    }

    @Override
    public int testCount() {
        return constructSuite().testCount();
    }
}
