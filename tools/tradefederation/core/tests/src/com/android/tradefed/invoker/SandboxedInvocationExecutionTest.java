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
package com.android.tradefed.invoker;

import static org.mockito.Mockito.any;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.times;

import com.android.tradefed.build.IBuildProvider;
import com.android.tradefed.command.CommandRunner.ExitCode;
import com.android.tradefed.config.Configuration;
import com.android.tradefed.config.ConfigurationDescriptor;
import com.android.tradefed.config.GlobalConfiguration;
import com.android.tradefed.config.IConfiguration;
import com.android.tradefed.guice.InvocationScope;
import com.android.tradefed.invoker.sandbox.SandboxedInvocationExecution;
import com.android.tradefed.log.ILogRegistry;
import com.android.tradefed.result.ILogSaver;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.result.LogFile;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.MockitoAnnotations;

/** Unit tests for {@link SandboxedInvocationExecution}. */
@RunWith(JUnit4.class)
public class SandboxedInvocationExecutionTest {

    private TestInvocation mInvocation;
    @Mock ILogRegistry mMockLogRegistry;
    @Mock IRescheduler mMockRescheduler;
    @Mock ITestInvocationListener mMockListener;
    @Mock ILogSaver mMockLogSaver;
    @Mock IBuildProvider mMockProvider;

    private IConfiguration mConfig;
    private IInvocationContext mContext;

    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);

        try {
            GlobalConfiguration.createGlobalConfiguration(new String[] {"empty"});
        } catch (IllegalStateException e) {
            // Avoid double init issues.
        }

        mInvocation =
                new TestInvocation() {
                    @Override
                    ILogRegistry getLogRegistry() {
                        return mMockLogRegistry;
                    }

                    @Override
                    protected void setExitCode(ExitCode code, Throwable stack) {
                        // empty on purpose
                    }

                    @Override
                    InvocationScope getInvocationScope() {
                        // Avoid re-entry in the current TF invocation scope for unit tests.
                        return new InvocationScope();
                    }
                };
        mConfig = new Configuration("test", "test");
        mContext = new InvocationContext();
    }

    /** Basic test to go through the flow of a sandbox invocation. */
    @Test
    public void testSandboxInvocation() throws Throwable {
        // Setup as a sandbox invocation
        ConfigurationDescriptor descriptor = new ConfigurationDescriptor();
        descriptor.setSandboxed(true);
        mConfig.setConfigurationObject(
                Configuration.CONFIGURATION_DESCRIPTION_TYPE_NAME, descriptor);
        mConfig.setLogSaver(mMockLogSaver);
        mConfig.setBuildProvider(mMockProvider);

        doReturn(new LogFile("file", "url", LogDataType.TEXT))
                .when(mMockLogSaver)
                .saveLogData(any(), any(), any());

        mInvocation.invoke(mContext, mConfig, mMockRescheduler, mMockListener);

        // Ensure that in sandbox we don't download again.
        Mockito.verify(mMockProvider, times(0)).getBuild();
    }
}
