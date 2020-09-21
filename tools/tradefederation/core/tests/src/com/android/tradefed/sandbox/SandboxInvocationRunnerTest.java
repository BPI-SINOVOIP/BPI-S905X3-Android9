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
package com.android.tradefed.sandbox;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.fail;

import com.android.tradefed.config.Configuration;
import com.android.tradefed.config.IConfiguration;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.invoker.InvocationContext;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;

import org.easymock.EasyMock;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Unit tests for {@link SandboxInvocationRunner}. */
@RunWith(JUnit4.class)
public class SandboxInvocationRunnerTest {

    private IConfiguration mConfig;
    private ISandbox mMockSandbox;
    private IInvocationContext mContext;
    private ITestInvocationListener mMockListener;

    @Before
    public void setUp() {
        mConfig = new Configuration("name", "description");
        mContext = new InvocationContext();
        mMockSandbox = EasyMock.createMock(ISandbox.class);
        mMockListener = EasyMock.createMock(ITestInvocationListener.class);
    }

    /** Test a run that is successful. */
    @Test
    public void testPrepareAndRun() throws Throwable {
        mConfig.setConfigurationObject(Configuration.SANDBOX_TYPE_NAME, mMockSandbox);
        EasyMock.expect(mMockSandbox.prepareEnvironment(mContext, mConfig, mMockListener))
                .andReturn(null);
        CommandResult res = new CommandResult(CommandStatus.SUCCESS);
        EasyMock.expect(mMockSandbox.run(mConfig, mMockListener)).andReturn(res);
        mMockSandbox.tearDown();
        EasyMock.replay(mMockSandbox, mMockListener);
        SandboxInvocationRunner.prepareAndRun(mConfig, mContext, mMockListener);
        EasyMock.verify(mMockSandbox, mMockListener);
    }

    /** Test a failure to prepare the environment. The exception will be send up. */
    @Test
    public void testPrepareAndRun_prepFailure() throws Throwable {
        mConfig.setConfigurationObject(Configuration.SANDBOX_TYPE_NAME, mMockSandbox);
        EasyMock.expect(mMockSandbox.prepareEnvironment(mContext, mConfig, mMockListener))
                .andReturn(new RuntimeException("my exception"));
        mMockSandbox.tearDown();
        EasyMock.replay(mMockSandbox, mMockListener);
        try {
            SandboxInvocationRunner.prepareAndRun(mConfig, mContext, mMockListener);
            fail("Should have thrown an exception.");
        } catch (RuntimeException expected) {
            assertEquals("my exception", expected.getMessage());
        }
        EasyMock.verify(mMockSandbox, mMockListener);
    }
}
