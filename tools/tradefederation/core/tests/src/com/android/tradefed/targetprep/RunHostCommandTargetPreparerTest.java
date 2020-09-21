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

package com.android.tradefed.targetprep;

import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;
import com.android.tradefed.util.IRunUtil;

import org.easymock.EasyMock;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

/** Unit test for {@link RunHostCommandTargetPreparer}. */
@RunWith(JUnit4.class)
public final class RunHostCommandTargetPreparerTest {

    private static final String DEVICE_SERIAL_PLACEHOLDER = "$SERIAL";
    private static final String DEVICE_SERIAL = "123456";

    private ITestDevice mDevice;
    private RunHostCommandTargetPreparer.BgCommandLog mBgCommandLog;
    private RunHostCommandTargetPreparer mPreparer;
    private IRunUtil mRunUtil;

    @Before
    public void setUp() {
        mDevice = EasyMock.createMock(ITestDevice.class);
        mRunUtil = EasyMock.createMock(IRunUtil.class);
        mBgCommandLog = EasyMock.createMock(RunHostCommandTargetPreparer.BgCommandLog.class);
        mPreparer =
                new RunHostCommandTargetPreparer() {
                    @Override
                    protected IRunUtil getRunUtil() {
                        return mRunUtil;
                    }

                    @Override
                    protected List<BgCommandLog> createBgCommandLogs() {
                        return Collections.singletonList(mBgCommandLog);
                    }
                };
    }

    @Test
    public void testSetUp() throws Exception {
        final String command = "command    \t\t\t  \t  argument " + DEVICE_SERIAL_PLACEHOLDER;
        final String[] commandArray = new String[] {"command", "argument", DEVICE_SERIAL};

        final OptionSetter optionSetter = new OptionSetter(mPreparer);
        optionSetter.setOptionValue("host-setup-command", command);
        optionSetter.setOptionValue("host-cmd-timeout", "10");

        CommandResult result = new CommandResult(CommandStatus.SUCCESS);
        result.setStdout("");
        result.setStderr("");

        EasyMock.expect(mRunUtil.runTimedCmd(10L, commandArray)).andReturn(result);
        EasyMock.expect(mDevice.getSerialNumber()).andReturn(DEVICE_SERIAL);

        EasyMock.replay(mRunUtil, mDevice);
        mPreparer.setUp(mDevice, null);
        EasyMock.verify(mRunUtil, mDevice);
    }

    @Test
    public void testTearDown() throws Exception {
        final String command = "command    \t\t\t  \t  argument " + DEVICE_SERIAL_PLACEHOLDER;
        final String[] commandArray = new String[] {"command", "argument", DEVICE_SERIAL};

        final OptionSetter optionSetter = new OptionSetter(mPreparer);
        optionSetter.setOptionValue("host-teardown-command", command);
        optionSetter.setOptionValue("host-cmd-timeout", "10");

        CommandResult result = new CommandResult(CommandStatus.SUCCESS);
        result.setStdout("");
        result.setStderr("");

        EasyMock.expect(mRunUtil.runTimedCmd(10L, commandArray)).andReturn(result);
        EasyMock.expect(mDevice.getSerialNumber()).andReturn(DEVICE_SERIAL);

        EasyMock.replay(mRunUtil, mDevice);
        mPreparer.tearDown(mDevice, null, null);
        EasyMock.verify(mRunUtil, mDevice);
    }

    @Test
    public void testBgCommand() throws Exception {
        final String command = "command    \t\t\t  \t  argument " + DEVICE_SERIAL;
        final String[] commandArray = new String[] {"command", "argument", DEVICE_SERIAL};

        final OptionSetter optionSetter = new OptionSetter(mPreparer);
        optionSetter.setOptionValue("host-background-command", command);

        OutputStream mockOut =
                new OutputStream() {
                    @Override
                    public void write(int i) throws IOException {}
                };

        EasyMock.expect(mRunUtil.runCmdInBackground(Arrays.asList(commandArray), mockOut))
                .andReturn(getMockProcess());
        EasyMock.expect(mDevice.getSerialNumber()).andReturn(DEVICE_SERIAL);
        EasyMock.expect(mBgCommandLog.getOutputStream()).andReturn(mockOut);

        EasyMock.replay(mRunUtil, mDevice, mBgCommandLog);
        mPreparer.setUp(mDevice, null);
        EasyMock.verify(mRunUtil, mDevice, mBgCommandLog);
    }

    private Process getMockProcess() {
        return new Process() {
            @Override
            public void destroy() {}

            @Override
            public int exitValue() {
                return 0;
            }

            @Override
            public InputStream getErrorStream() {
                return null;
            }

            @Override
            public InputStream getInputStream() {
                return null;
            }

            @Override
            public OutputStream getOutputStream() {
                return null;
            }

            @Override
            public int waitFor() throws InterruptedException {
                return 0;
            }
        };
    }
}
