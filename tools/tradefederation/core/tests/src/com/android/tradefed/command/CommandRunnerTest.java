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
package com.android.tradefed.command;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.doReturn;

import com.android.tradefed.command.CommandRunner.ExitCode;
import com.android.tradefed.config.ConfigurationException;
import com.android.tradefed.config.GlobalConfiguration;
import com.android.tradefed.device.MockDeviceManager;
import com.android.tradefed.util.FileUtil;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.mockito.Mockito;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.PrintWriter;

/**
 * Unit tests for {@link CommandRunner}. Always attempt to run on null-device (-n) to avoid hanging
 * if no physical device are available.
 */
@RunWith(JUnit4.class)
public class CommandRunnerTest {

    private static final String FAKE_CONFIG = "doesnotexit";
    private String mStackTraceOutput = null;
    private ICommandScheduler mMockScheduler;


    private static final String EMPTY_CONF_CONTENT =
            "<configuration description=\"Empty Config\" />";
    private File mConfig;
    private File mLogDir;

    @Before
    public void setUp() throws Exception {
        mStackTraceOutput = null;
        mConfig = FileUtil.createTempFile("empty", ".xml");
        FileUtil.writeToFile(EMPTY_CONF_CONTENT, mConfig);
        mLogDir = FileUtil.createTempDir("command-runner-unit-test");
        mMockScheduler =
                new CommandScheduler() {
                    @Override
                    protected void initLogging() {}

                    @Override
                    protected void cleanUp() {}

                    @Override
                    void initDeviceManager() {
                        try {
                            super.initDeviceManager();
                        } catch (IllegalStateException e) {
                            // ignore re-init
                        }
                    }
                };
    }

    @After
    public void tearDown() {
        GlobalConfiguration.getInstance()
                .getCommandScheduler()
                .setLastInvocationExitCode(ExitCode.NO_ERROR, null);
        FileUtil.deleteFile(mConfig);
        FileUtil.recursiveDelete(mLogDir);
    }

    private class TestableCommandRunner extends CommandRunner {

        @Override
        void initGlobalConfig(String[] args) throws ConfigurationException {
            try {
                GlobalConfiguration.createGlobalConfiguration(args);
            } catch (IllegalStateException e) {
                // ignore re-init.
            }
        }

        @Override
        ICommandScheduler getCommandScheduler() {
            return mMockScheduler;
        }

        /** We capture the stack trace if any. */
        @Override
        void printStackTrace(Throwable e) {
            ByteArrayOutputStream out = new ByteArrayOutputStream();
            PrintWriter pw = new PrintWriter(out);
            e.printStackTrace(pw);
            pw.flush();
            mStackTraceOutput = out.toString();
        }
    }

    /**
     * Run with a known empty config, should always pass.
     */
    @Test
    public void testRun_noError() throws Exception {
        mStackTraceOutput = null;
        CommandRunner mRunner = new TestableCommandRunner();
        String[] args = {
            mConfig.getAbsolutePath(),
            "-n",
            "--no-return-null",
            "--no-throw-build-error",
            "--log-file-path",
            mLogDir.getAbsolutePath()
        };
        mRunner.run(args);
        assertEquals(0, mRunner.getErrorCode().getCodeValue());
        assertNull(mStackTraceOutput);
    }

    /**
     * Run with a known empty config but fake a device unresponsive.
     */
    @Test
    public void testRun_deviceUnresponsive() {
        CommandRunner mRunner = new TestableCommandRunner();
        String[] args = {
            mConfig.getAbsolutePath(),
            "-n",
            "--test-throw-unresponsive",
            "--log-file-path",
            mLogDir.getAbsolutePath()
        };
        mRunner.run(args);
        assertEquals(ExitCode.DEVICE_UNRESPONSIVE, mRunner.getErrorCode());
        assertTrue(
                String.format("%s does not contains the expected output", mStackTraceOutput),
                mStackTraceOutput.contains(
                        "com.android.tradefed.device.DeviceUnresponsiveException: "
                                + "StubTest DeviceUnresponsiveException"));
    }

    /**
     * Run with a known empty config but fake a device unavailable.
     */
    @Test
    public void testRun_deviceUnavailable() {
        CommandRunner mRunner = new TestableCommandRunner();
        String[] args = {
            mConfig.getAbsolutePath(),
            "-n",
            "--test-throw-not-available",
            "--log-file-path",
            mLogDir.getAbsolutePath()
        };
        mRunner.run(args);
        assertEquals(ExitCode.DEVICE_UNAVAILABLE, mRunner.getErrorCode());
        assertTrue(
                String.format("%s does not contains the expected output", mStackTraceOutput),
                mStackTraceOutput.contains(
                        "com.android.tradefed.device.DeviceNotAvailableException: "
                                + "StubTest DeviceNotAvailableException"));
    }

    /**
     * Run with a known empty config but a throwable exception is caught.
     */
    @Test
    public void testRun_throwable() {
        CommandRunner mRunner = new TestableCommandRunner();
        String[] args = {
            mConfig.getAbsolutePath(),
            "-n",
            "--test-throw-runtime",
            "--log-file-path",
            mLogDir.getAbsolutePath()
        };
        mRunner.run(args);
        assertEquals(ExitCode.THROWABLE_EXCEPTION, mRunner.getErrorCode());
        assertTrue(
                String.format("%s does not contains the expected output", mStackTraceOutput),
                mStackTraceOutput.contains(
                        "java.lang.RuntimeException: StubTest RuntimeException"));
    }

    /**
     * Run with a non existant config and expect a configuration exception because of it.
     */
    @Test
    public void testRun_ConfigError() {
        String[] args = {FAKE_CONFIG};
        CommandRunner mRunner = new TestableCommandRunner();
        mRunner.run(args);
        assertEquals(ExitCode.CONFIG_EXCEPTION, mRunner.getErrorCode());
        assertTrue(
                "Stack does not contain expected message: " + mStackTraceOutput,
                mStackTraceOutput.contains(FAKE_CONFIG));
    }

    /** Test that if the device is not allocated after a timeout, we throw a NoDeviceException. */
    @Test
    public void testRun_noDevice() throws Exception {
        CommandScheduler mockScheduler = Mockito.spy(CommandScheduler.class);
        CommandRunner mRunner =
                new TestableCommandRunner() {
                    @Override
                    long getCheckDeviceTimeout() {
                        return 200l;
                    }

                    @Override
                    ICommandScheduler getCommandScheduler() {
                        return mockScheduler;
                    }
                };
        String[] args = {
            mConfig.getAbsolutePath(),
            "-s",
            "impossibleSerialThatWillNotBeFound",
            "--log-file-path",
            mLogDir.getAbsolutePath()
        };
        doNothing().when(mockScheduler).initDeviceManager();
        doReturn(new MockDeviceManager(1)).when(mockScheduler).getDeviceManager();
        doNothing().when(mockScheduler).shutdownOnEmpty();
        doNothing().when(mockScheduler).initLogging();
        doNothing().when(mockScheduler).cleanUp();
        mRunner.run(args);
        Mockito.verify(mockScheduler).shutdownOnEmpty();
        mockScheduler.join(5000);
        assertEquals(ExitCode.NO_DEVICE_ALLOCATED, mRunner.getErrorCode());
        assertTrue(
                String.format("%s does not contains the expected output", mStackTraceOutput),
                mStackTraceOutput.contains(
                        "com.android.tradefed.device.NoDeviceException: No device was allocated "
                                + "for the command."));
    }
}
