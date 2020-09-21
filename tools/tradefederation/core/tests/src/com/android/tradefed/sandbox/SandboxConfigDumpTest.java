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
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import com.android.tradefed.sandbox.SandboxConfigDump.DumpCmd;
import com.android.tradefed.util.FileUtil;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.io.File;

/** Unit tests for {@link com.android.tradefed.sandbox.SandboxConfigDump}. */
@RunWith(JUnit4.class)
public class SandboxConfigDumpTest {
    private SandboxConfigDump mConfigDump;
    private File mOutputFile;

    @Before
    public void setUp() throws Exception {
        mConfigDump = new SandboxConfigDump();
        mOutputFile = FileUtil.createTempFile("temp-file-config", ".xml");
    }

    @After
    public void tearDown() {
        FileUtil.deleteFile(mOutputFile);
    }

    /**
     * Test {@link com.android.tradefed.sandbox.SandboxConfigDump#parse(String[])} parse and output
     * and full xml based on command line.
     */
    @Test
    public void testParseCommandLine() throws Exception {
        String[] commandLine =
                new String[] {DumpCmd.FULL_XML.toString(), mOutputFile.getAbsolutePath(), "empty"};
        int res = mConfigDump.parse(commandLine);
        assertEquals(0, res);
        String output = FileUtil.readStringFromFile(mOutputFile);
        assertTrue(!output.isEmpty());
        assertTrue(output.contains("<test class"));
        assertTrue(
                output.contains(
                        "<result_reporter class=\"com.android.tradefed.result."
                                + "TextResultReporter\""));
    }

    /**
     * Test {@link com.android.tradefed.sandbox.SandboxConfigDump#parse(String[])} parse and output
     * a partial xml without versioned object (test, target_prep, multi_target_prep).
     */
    @Test
    public void testParseCommandLine_filtered() throws Exception {
        String[] commandLine =
                new String[] {
                    DumpCmd.NON_VERSIONED_CONFIG.toString(), mOutputFile.getAbsolutePath(), "empty"
                };
        int res = mConfigDump.parse(commandLine);
        assertEquals(0, res);
        String output = FileUtil.readStringFromFile(mOutputFile);
        assertTrue(!output.isEmpty());
        assertFalse(output.contains("<test class"));
        assertTrue(
                output.contains(
                        "<result_reporter class=\"com.android.tradefed.result."
                                + "TextResultReporter\""));
    }

    /**
     * Test {@link com.android.tradefed.sandbox.SandboxConfigDump#parse(String[])} parse and output
     * an xml meant to be run in subprocess, the subprocess result reporter has been added.
     */
    @Test
    public void testParseCommandLine_run() throws Exception {
        String[] commandLine =
                new String[] {
                    DumpCmd.RUN_CONFIG.toString(), mOutputFile.getAbsolutePath(), "empty"
                };
        int res = mConfigDump.parse(commandLine);
        assertEquals(0, res);
        String output = FileUtil.readStringFromFile(mOutputFile);
        assertTrue(!output.isEmpty());
        assertTrue(output.contains("<test class"));
        assertTrue(
                output.contains(
                        "<result_reporter class=\"com.android.tradefed.result."
                                + "SubprocessResultsReporter\""));
    }
}
