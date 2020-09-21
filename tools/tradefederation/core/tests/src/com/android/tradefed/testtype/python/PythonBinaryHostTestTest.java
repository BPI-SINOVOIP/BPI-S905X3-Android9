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
package com.android.tradefed.testtype.python;

import static org.junit.Assert.*;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.device.StubDevice;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.IRunUtil;

import org.easymock.EasyMock;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.io.File;

/** Unit tests for {@link PythonBinaryHostTest}. */
@RunWith(JUnit4.class)
public class PythonBinaryHostTestTest {
    private PythonBinaryHostTest mTest;
    private IRunUtil mMockRunUtil;
    private IBuildInfo mMockBuildInfo;
    private ITestDevice mMockDevice;
    private ITestInvocationListener mMockListener;

    @Before
    public void setUp() {
        mMockRunUtil = EasyMock.createMock(IRunUtil.class);
        mMockBuildInfo = EasyMock.createMock(IBuildInfo.class);
        mMockListener = EasyMock.createMock(ITestInvocationListener.class);
        mMockDevice = EasyMock.createMock(ITestDevice.class);
        mTest =
                new PythonBinaryHostTest() {
                    @Override
                    IRunUtil getRunUtil() {
                        return mMockRunUtil;
                    }
                };
        mTest.setBuild(mMockBuildInfo);
        mTest.setDevice(mMockDevice);
    }

    /** Test that when running a python binary the output is parsed to obtain results. */
    @Test
    public void testRun() throws Exception {
        File binary = FileUtil.createTempFile("python-dir", "");
        try {
            OptionSetter setter = new OptionSetter(mTest);
            setter.setOptionValue("python-binaries", binary.getAbsolutePath());
            CommandResult res = new CommandResult();
            res.setStatus(CommandStatus.SUCCESS);
            res.setStderr("TEST_RUN_STARTED {\"testCount\": 5, \"runName\": \"TestSuite\"}");
            EasyMock.expect(
                            mMockRunUtil.runTimedCmd(
                                    EasyMock.anyLong(),
                                    EasyMock.eq(binary.getAbsolutePath()),
                                    EasyMock.eq("-q")))
                    .andReturn(res);
            mMockListener.testRunStarted(binary.getName(), 5);
            mMockListener.testLog(
                    EasyMock.eq(PythonBinaryHostTest.PYTHON_OUTPUT),
                    EasyMock.eq(LogDataType.TEXT),
                    EasyMock.anyObject());
            EasyMock.expect(mMockDevice.getIDevice()).andReturn(new StubDevice("serial"));
            EasyMock.replay(mMockRunUtil, mMockBuildInfo, mMockListener, mMockDevice);
            mTest.run(mMockListener);
            EasyMock.verify(mMockRunUtil, mMockBuildInfo, mMockListener, mMockDevice);
        } finally {
            FileUtil.deleteFile(binary);
        }
    }

    /**
     * If the binary returns an exception status, we should throw a runtime exception since
     * something went wrong with the binary setup.
     */
    @Test
    public void testRunFail_exception() throws Exception {
        File binary = FileUtil.createTempFile("python-dir", "");
        try {
            OptionSetter setter = new OptionSetter(mTest);
            setter.setOptionValue("python-binaries", binary.getAbsolutePath());
            CommandResult res = new CommandResult();
            res.setStatus(CommandStatus.EXCEPTION);
            res.setStderr("Could not execute.");
            EasyMock.expect(
                            mMockRunUtil.runTimedCmd(
                                    EasyMock.anyLong(),
                                    EasyMock.eq(binary.getAbsolutePath()),
                                    EasyMock.eq("-q")))
                    .andReturn(res);
            EasyMock.expect(mMockDevice.getIDevice()).andReturn(new StubDevice("serial"));
            EasyMock.replay(mMockRunUtil, mMockBuildInfo, mMockListener, mMockDevice);
            try {
                mTest.run(mMockListener);
                fail("Should have thrown an exception.");
            } catch (RuntimeException expected) {
                // expected
            }
            EasyMock.verify(mMockRunUtil, mMockBuildInfo, mMockListener, mMockDevice);
        } finally {
            FileUtil.deleteFile(binary);
        }
    }

    /**
     * If the binary reports a FAILED status but the output actually have some tests, it most likely
     * means that some tests failed. So we simply continue with parsing the results.
     */
    @Test
    public void testRunFail_failureOnly() throws Exception {
        File binary = FileUtil.createTempFile("python-dir", "");
        try {
            OptionSetter setter = new OptionSetter(mTest);
            setter.setOptionValue("python-binaries", binary.getAbsolutePath());
            CommandResult res = new CommandResult();
            res.setStatus(CommandStatus.FAILED);
            res.setStderr("TEST_RUN_STARTED {\"testCount\": 5, \"runName\": \"TestSuite\"}");
            EasyMock.expect(
                            mMockRunUtil.runTimedCmd(
                                    EasyMock.anyLong(),
                                    EasyMock.eq(binary.getAbsolutePath()),
                                    EasyMock.eq("-q")))
                    .andReturn(res);
            mMockListener.testRunStarted(binary.getName(), 5);
            mMockListener.testLog(
                    EasyMock.eq(PythonBinaryHostTest.PYTHON_OUTPUT),
                    EasyMock.eq(LogDataType.TEXT),
                    EasyMock.anyObject());
            EasyMock.expect(mMockDevice.getIDevice()).andReturn(new StubDevice("serial"));
            EasyMock.replay(mMockRunUtil, mMockBuildInfo, mMockListener, mMockDevice);
            mTest.run(mMockListener);
            EasyMock.verify(mMockRunUtil, mMockBuildInfo, mMockListener, mMockDevice);
        } finally {
            FileUtil.deleteFile(binary);
        }
    }
}
