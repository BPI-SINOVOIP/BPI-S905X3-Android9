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
package com.android.tradefed.config;

import static org.easymock.EasyMock.anyObject;
import static org.easymock.EasyMock.eq;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.fail;

import com.android.tradefed.sandbox.ISandbox;
import com.android.tradefed.sandbox.SandboxConfigDump;
import com.android.tradefed.sandbox.SandboxConfigDump.DumpCmd;
import com.android.tradefed.sandbox.SandboxConfigurationException;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.IRunUtil;
import com.android.tradefed.util.IRunUtil.EnvPriority;
import com.android.tradefed.util.keystore.StubKeyStoreClient;

import org.easymock.EasyMock;
import org.easymock.IAnswer;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.io.File;
import java.io.IOException;

/** Unit tests for {@link SandboxConfigurationFactory}. */
@RunWith(JUnit4.class)
public class SandboxConfigurationFactoryTest {

    private SandboxConfigurationFactory mFactory;
    private File mConfig;
    private ISandbox mFakeSandbox;
    private IRunUtil mMockRunUtil;

    @Before
    public void setUp() throws IOException, ConfigurationException {
        mFactory = SandboxConfigurationFactory.getInstance();
        mConfig = FileUtil.createTempFile("sandbox-config-test", ".xml");
        mFakeSandbox = EasyMock.createMock(ISandbox.class);
        mMockRunUtil = EasyMock.createMock(IRunUtil.class);
        try {
            GlobalConfiguration.createGlobalConfiguration(new String[] {});
        } catch (IllegalStateException ignore) {
            // ignore the global config re-init
        }
    }

    @After
    public void tearDown() {
        FileUtil.deleteFile(mConfig);
    }

    private void expectDumpCmd(CommandResult res) {
        EasyMock.expect(
                        mMockRunUtil.runTimedCmd(
                                EasyMock.anyLong(),
                                eq("java"),
                                eq("-cp"),
                                anyObject(),
                                eq(SandboxConfigDump.class.getCanonicalName()),
                                eq(DumpCmd.NON_VERSIONED_CONFIG.toString()),
                                EasyMock.anyObject(),
                                eq(mConfig.getAbsolutePath())))
                .andAnswer(
                        new IAnswer<CommandResult>() {
                            @Override
                            public CommandResult answer() throws Throwable {
                                String resFile = (String) EasyMock.getCurrentArguments()[6];
                                FileUtil.writeToFile(
                                        "<configuration><test class=\"com.android.tradefed.test"
                                                + "type.StubTest\" /></configuration>",
                                        new File(resFile));
                                return res;
                            }
                        });
    }

    /**
     * Test that creating a configuration using a sandbox properly create a {@link IConfiguration}.
     */
    @Test
    public void testCreateConfigurationFromArgs() throws ConfigurationException {
        String[] args = new String[] {mConfig.getAbsolutePath()};
        mMockRunUtil.unsetEnvVariable(GlobalConfiguration.GLOBAL_CONFIG_VARIABLE);
        EasyMock.expectLastCall().times(2);
        mMockRunUtil.setEnvVariable(
                EasyMock.eq(GlobalConfiguration.GLOBAL_CONFIG_VARIABLE), EasyMock.anyObject());
        mMockRunUtil.setEnvVariablePriority(EnvPriority.SET);
        CommandResult results = new CommandResult();
        results.setStatus(CommandStatus.SUCCESS);
        expectDumpCmd(results);
        EasyMock.replay(mFakeSandbox, mMockRunUtil);
        IConfiguration config =
                mFactory.createConfigurationFromArgs(
                        args, new StubKeyStoreClient(), mFakeSandbox, mMockRunUtil);
        EasyMock.verify(mFakeSandbox, mMockRunUtil);
        assertNotNull(config.getConfigurationObject(Configuration.SANDBOX_TYPE_NAME));
        assertEquals(mFakeSandbox, config.getConfigurationObject(Configuration.SANDBOX_TYPE_NAME));
    }

    /** Test that when the dump config failed, we throw a SandboxConfigurationException. */
    @Test
    public void testCreateConfigurationFromArgs_fail() throws Exception {
        String[] args = new String[] {mConfig.getAbsolutePath()};
        mMockRunUtil.unsetEnvVariable(GlobalConfiguration.GLOBAL_CONFIG_VARIABLE);
        EasyMock.expectLastCall().times(2);
        mMockRunUtil.setEnvVariable(
                EasyMock.eq(GlobalConfiguration.GLOBAL_CONFIG_VARIABLE), EasyMock.anyObject());
        mMockRunUtil.setEnvVariablePriority(EnvPriority.SET);
        CommandResult results = new CommandResult();
        results.setStatus(CommandStatus.FAILED);
        results.setStderr("I failed");
        expectDumpCmd(results);
        // Thin launcher is attempted, and in this case fails, so original exception is thrown.
        EasyMock.expect(
                        mFakeSandbox.createThinLauncherConfig(
                                EasyMock.anyObject(), EasyMock.anyObject(),
                                EasyMock.anyObject(), EasyMock.anyObject()))
                .andReturn(null);
        // in case of failure, tearDown is called right away for cleaning up
        mFakeSandbox.tearDown();
        EasyMock.replay(mFakeSandbox, mMockRunUtil);
        try {
            mFactory.createConfigurationFromArgs(
                    args, new StubKeyStoreClient(), mFakeSandbox, mMockRunUtil);
            fail("Should have thrown an exception.");
        } catch (SandboxConfigurationException expected) {
            // expected
        }
        EasyMock.verify(mFakeSandbox, mMockRunUtil);
    }
}
