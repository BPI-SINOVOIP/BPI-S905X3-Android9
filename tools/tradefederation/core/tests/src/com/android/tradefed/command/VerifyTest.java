/*
 * Copyright (C) 2015 The Android Open Source Project
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

import com.android.tradefed.config.ConfigurationException;
import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.util.FileUtil;

import junit.framework.TestCase;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;

/**
 * Unit tests for {@link Verify}
 */
public class VerifyTest extends TestCase {
    private static final String TEST_CMD_FILE_PATH = "/testCmdFiles";
    private final Verify mVerify;

    public VerifyTest() throws ConfigurationException {
        mVerify = new Verify();

        OptionSetter option = new OptionSetter(mVerify);
        option.setOptionValue("quiet", "true");
    }

    /**
     * Extract an embedded command file into a temporary file, which we can feed to the
     * CommandFileParser
     */
    private File extractTestCmdFile(String name) throws IOException {
        final InputStream cmdFileStream = getClass().getResourceAsStream(
                String.format("%s/%s.txt", TEST_CMD_FILE_PATH, name));
        final String tmpFileName = String.format("VerifyTest_%s_", name);
        File tmpFile = FileUtil.createTempFile(tmpFileName, ".txt");
        try {
            FileUtil.writeToFile(cmdFileStream, tmpFile);
        } catch (Throwable t) {
            // Clean up tmpFile, if it was created.
            FileUtil.deleteFile(tmpFile);
            throw t;
        }

        return tmpFile;
    }

    /**
     * Assert that the specified command file parses correctly, and clean up any temporary files
     */
    private void assertGoodCmdFile(String name) throws IOException {
        File cmdFile = extractTestCmdFile(name);
        try {
            assertTrue(mVerify.runVerify(cmdFile));
        } finally {
            FileUtil.deleteFile(cmdFile);
        }
    }

    /**
     * Assert that the specified command file does not parse correctly, and clean up any temporary
     * files
     */
    private void assertBadCmdFile(String name) throws IOException {
        File cmdFile = extractTestCmdFile(name);
        try {
            assertFalse(mVerify.runVerify(cmdFile));
        } finally {
            FileUtil.deleteFile(cmdFile);
        }
    }

    public void testBasic() throws IOException {
        assertGoodCmdFile("basic");
    }

    public void testMissingMacroDef() throws IOException {
        assertBadCmdFile("missing-macro-def");
    }

    public void testMissingBeginMacro() throws IOException {
        assertBadCmdFile("missing-begin-macro");
    }

    public void testMissingEndMacro() throws IOException {
        assertBadCmdFile("missing-end-macro");
    }
}
