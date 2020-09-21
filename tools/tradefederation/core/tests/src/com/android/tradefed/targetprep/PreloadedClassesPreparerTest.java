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

import static org.junit.Assert.fail;
import static org.mockito.Mockito.when;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;
import com.android.tradefed.util.IRunUtil;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.mockito.Mockito;

import java.io.File;

/** Unit tests for {@link PushFilePreparer} */
@RunWith(JUnit4.class)
public class PreloadedClassesPreparerTest {
    private static final String FAKE_FILE_PATH = "/file/path";
    private static final String FAKE_TOOL_PATH = "/tool/path";
    private static final String PRELOAD_TOOL_NAME = "preload2.jar";
    private static final String WRITE_COMMAND =
            "java -cp %s com.android.preload.Main --seq SERIAL write %s";

    private IBuildInfo mMockBuildInfo;
    private ITestDevice mMockDevice;
    private IRunUtil mMockRunUtil;

    private PreloadedClassesPreparer mRealPreparer;
    private PreloadedClassesPreparer mSpyPreparer;

    @Before
    public void setUp() throws Exception {
        // Setup mocks and spies
        mMockDevice = Mockito.mock(ITestDevice.class);
        mMockBuildInfo = Mockito.mock(IBuildInfo.class);
        mMockRunUtil = Mockito.mock(IRunUtil.class);
        mRealPreparer = new PreloadedClassesPreparer();
        mSpyPreparer = Mockito.spy(mRealPreparer);
        // Setup mock returns
        when(mMockDevice.getDeviceDescriptor()).thenReturn(null);
        when(mMockDevice.getSerialNumber()).thenReturn("SERIAL");
        when(mSpyPreparer.getRunUtil()).thenReturn(mMockRunUtil);
    }

    // Using the build info to get the preload tool is specific to remote runs.
    @Test
    public void testSetUp_RemoteSuccess() throws Exception {
        // Create a fully mocked success case
        File tool = Mockito.mock(File.class);
        when(tool.exists()).thenReturn(true);
        when(tool.getAbsolutePath()).thenReturn(FAKE_TOOL_PATH);
        when(mMockBuildInfo.getFile(PRELOAD_TOOL_NAME)).thenReturn(tool);
        when(mSpyPreparer.getPreloadedClassesPath()).thenReturn(FAKE_FILE_PATH);
        CommandResult result = new CommandResult();
        result.setStatus(CommandStatus.SUCCESS);
        // Expected output command based on the above.
        String[] command = String.format(WRITE_COMMAND, FAKE_TOOL_PATH, FAKE_FILE_PATH).split(" ");
        when(mMockRunUtil.runTimedCmd(PreloadedClassesPreparer.DEFAULT_TIMEOUT_MS, command))
                .thenReturn(result);
        // Run and don't encounter issues
        mSpyPreparer.setUp(mMockDevice, mMockBuildInfo);
    }

    // Using the build info to get the preload tool is specific to remote runs.
    @Test
    public void testSetUp_RemoteNoTool() throws Exception {
        // Set mocks to fail returning the tool
        when(mSpyPreparer.getPreloadedClassesPath()).thenReturn(FAKE_FILE_PATH);
        when(mMockBuildInfo.getFile(PRELOAD_TOOL_NAME)).thenReturn(null);
        try {
            mSpyPreparer.setUp(mMockDevice, mMockBuildInfo);
            fail("Did not fail when there was no tool available.");
        } catch (TargetSetupError e) {
            // Good, this should throw
        }
    }

    @Test
    public void testSetUp_LocalSuccess() throws Exception {
        when(mSpyPreparer.getPreloadToolPath()).thenReturn(FAKE_TOOL_PATH);
        when(mSpyPreparer.getPreloadedClassesPath()).thenReturn(FAKE_FILE_PATH);
        CommandResult result = new CommandResult();
        result.setStatus(CommandStatus.SUCCESS);
        // Expected output command based on the above.
        String[] command = String.format(WRITE_COMMAND, FAKE_TOOL_PATH, FAKE_FILE_PATH).split(" ");
        when(mMockRunUtil.runTimedCmd(PreloadedClassesPreparer.DEFAULT_TIMEOUT_MS, command))
                .thenReturn(result);
        // Run and don't encounter issues
        mSpyPreparer.setUp(mMockDevice, mMockBuildInfo);
    }

    @Test
    public void testSetUp_NoFile() throws Exception {
        // If not skipped, expect this to error out.
        mSpyPreparer.setUp(mMockDevice, mMockBuildInfo);
    }

    @Test
    public void testSetUp_WriteFailure() throws Exception {
        when(mSpyPreparer.getPreloadToolPath()).thenReturn(FAKE_TOOL_PATH);
        when(mSpyPreparer.getPreloadedClassesPath()).thenReturn(FAKE_FILE_PATH);
        CommandResult result = new CommandResult();
        result.setStatus(CommandStatus.FAILED);
        // Expected output command based on the above.
        String[] command = String.format(WRITE_COMMAND, FAKE_TOOL_PATH, FAKE_FILE_PATH).split(" ");
        when(mMockRunUtil.runTimedCmd(PreloadedClassesPreparer.DEFAULT_TIMEOUT_MS, command))
                .thenReturn(result);
        // Run and encounter a write issue
        try {
            mSpyPreparer.setUp(mMockDevice, mMockBuildInfo);
            fail("Did not fail when writing with the tool failed.");
        } catch (TargetSetupError e) {
            // Good, this should throw.
        }
    }
}
