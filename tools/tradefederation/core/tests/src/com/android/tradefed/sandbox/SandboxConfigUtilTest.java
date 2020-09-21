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
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.fail;
import static org.mockito.Mockito.doReturn;

import com.android.tradefed.config.ConfigurationException;
import com.android.tradefed.sandbox.SandboxConfigDump.DumpCmd;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.IRunUtil;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.mockito.Mockito;

import java.io.File;

/** Unit tests for {@link com.android.tradefed.sandbox.SandboxConfigUtil}. */
@RunWith(JUnit4.class)
public class SandboxConfigUtilTest {

    private IRunUtil mMockRunUtil;
    private File mTmpRootDir;

    @Before
    public void setUp() throws Exception {
        mMockRunUtil = Mockito.mock(IRunUtil.class);
        mTmpRootDir = FileUtil.createTempDir("sandbox-config-util-test");
        FileUtil.createTempFile("fakejar", ".jar", mTmpRootDir);
    }

    @After
    public void tearDown() {
        FileUtil.recursiveDelete(mTmpRootDir);
    }

    /**
     * Test {@link com.android.tradefed.sandbox.SandboxConfigUtil#dumpConfigForVersion(File,
     * IRunUtil, String[], DumpCmd, File)} for a success case when the command returns a valid file.
     */
    @Test
    public void testDumpVersion() throws Exception {
        CommandResult result = new CommandResult();
        result.setStatus(CommandStatus.SUCCESS);
        doReturn(result).when(mMockRunUtil).runTimedCmd(Mockito.anyLong(), Mockito.any());
        File res = null;
        try {
            res =
                    SandboxConfigUtil.dumpConfigForVersion(
                            mTmpRootDir,
                            mMockRunUtil,
                            new String[] {"empty"},
                            DumpCmd.FULL_XML,
                            null);
            assertNotNull(res);
        } finally {
            FileUtil.deleteFile(res);
        }
    }

    /**
     * Test {@link com.android.tradefed.sandbox.SandboxConfigUtil#dumpConfigForVersion(File,
     * IRunUtil, String[], DumpCmd, File)} for a failure case, the command throws an exception.
     */
    @Test
    public void testDumpVersion_failed() throws Exception {
        CommandResult result = new CommandResult();
        result.setStatus(CommandStatus.FAILED);
        result.setStderr("Ouch I failed");
        doReturn(result).when(mMockRunUtil).runTimedCmd(Mockito.anyLong(), Mockito.any());
        try {
            SandboxConfigUtil.dumpConfigForVersion(
                    mTmpRootDir, mMockRunUtil, new String[] {"empty"}, DumpCmd.FULL_XML, null);
            fail("Should have thrown an exception.");
        } catch (ConfigurationException expected) {
            assertEquals("Ouch I failed", expected.getMessage());
        }
    }

    /** If the classpath is empty, expect a dedicated exception to notify it. */
    @Test
    public void testDumpVersion_badClasspath() throws Exception {
        try {
            SandboxConfigUtil.dumpConfigForVersion(
                    "", mMockRunUtil, new String[] {"empty"}, DumpCmd.FULL_XML, null);
            fail("Should have thrown an exception.");
        } catch (SandboxConfigurationException expected) {
            assertEquals(
                    "Something went wrong with the sandbox setup, classpath was empty.",
                    expected.getMessage());
        }
    }
}
