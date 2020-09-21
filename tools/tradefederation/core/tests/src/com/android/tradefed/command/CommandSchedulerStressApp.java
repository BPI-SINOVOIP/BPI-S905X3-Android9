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

package com.android.tradefed.command;

import com.android.tradefed.config.IConfiguration;
import com.android.tradefed.config.IConfigurationFactory;
import com.android.tradefed.device.DeviceSelectionOptions;
import com.android.tradefed.device.IDeviceManager;
import com.android.tradefed.device.MockDeviceManager;

import junit.framework.TestCase;

import org.easymock.EasyMock;

/**
 * Longer running stress java app that stresses {@link CommandScheduler}.
 * <p/>
 * Intended to be executed under a profiler.
 */
public class CommandSchedulerStressApp extends TestCase {

    /** the {@link CommandScheduler} under test, with all dependencies mocked out */
    private CommandScheduler mCommandScheduler;
    private IDeviceManager mMockDeviceManager;
    private IConfiguration mMockConfig;
    private IConfigurationFactory mMockConfigFactory;
    private CommandOptions mCommandOptions;

    public CommandSchedulerStressApp() throws Exception {
        mMockConfig = EasyMock.createNiceMock(IConfiguration.class);
        mMockDeviceManager = new MockDeviceManager(100);
        mMockConfigFactory = EasyMock.createMock(IConfigurationFactory.class);
        mCommandOptions = new CommandOptions();
        mCommandOptions.setLoopMode(false);
        mCommandOptions.setMinLoopTime(0);
        EasyMock.expect(mMockConfig.getCommandOptions()).andStubReturn(mCommandOptions);
        EasyMock.expect(mMockConfig.getDeviceRequirements()).andStubReturn(
                new DeviceSelectionOptions());

        mCommandScheduler =
                new CommandScheduler() {
                    @Override
                    protected IDeviceManager getDeviceManager() {
                        return mMockDeviceManager;
                    }

                    @Override
                    protected IConfigurationFactory getConfigFactory() {
                        return mMockConfigFactory;
                    }
                };
    }

    /**
     * Test config priority scheduling under load with a large number of commands, when device
     * resources are scarce.
     * <p/>
     * Intended to verify that the device polling scheme used by the scheduler is not overly
     * expensive.
     * <p/>
     * Lacks automated verification - intended to be executed under a profiler.
     */
    public void testRunLoad() throws Exception {
        String[] configArgs = new String[] {"arrgs"};

        EasyMock.expect(
                mMockConfigFactory.createConfigurationFromArgs(EasyMock.aryEq(configArgs)))
                .andReturn(mMockConfig).anyTimes();

        EasyMock.replay(mMockConfig, mMockConfigFactory);
        mCommandScheduler.start();

        for (int i=0; i < 10000; i++) {
            mCommandScheduler.addCommand(configArgs);
        }

        // let scheduler churn through the commands for 30 seconds
        Thread.sleep(30 * 1000);
        mCommandScheduler.shutdown();
        mCommandScheduler.join();
    }

    public static void main(String[] args) {
        try {
            long startTime = System.currentTimeMillis();
            CommandSchedulerStressApp stressApp = new CommandSchedulerStressApp();
            stressApp.testRunLoad();
            System.out.printf("Stress app ran for %s ms", System.currentTimeMillis() - startTime);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}
