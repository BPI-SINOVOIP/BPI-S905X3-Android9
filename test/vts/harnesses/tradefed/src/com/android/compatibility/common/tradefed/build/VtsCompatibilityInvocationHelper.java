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
package com.android.compatibility.common.tradefed.build;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.testtype.suite.TestSuiteInfo;

import java.io.File;
import java.io.FileNotFoundException;

/**
 * A simple helper that stores and retrieves information from a {@link IBuildInfo}.
 */
public class VtsCompatibilityInvocationHelper {
    private File mTestCasesDir = null;

    /**
     * Get test case binaries directory
     */
    public File getTestsDir() throws FileNotFoundException {
        if (mTestCasesDir == null) {
            String testCasesRootDirPath;
            testCasesRootDirPath = System.getenv(
                    String.format("%s_TESTCASES", TestSuiteInfo.getInstance().getName()));
            File testCaseDir;
            if (testCasesRootDirPath != null && !testCasesRootDirPath.trim().equals("")) {
                testCaseDir = new File(testCasesRootDirPath);
            } else {
                String rootDirPath;
                rootDirPath = System.getProperty(
                        String.format("%s_ROOT", TestSuiteInfo.getInstance().getName()));
                if (rootDirPath == null || rootDirPath.trim().equals("")) {
                    throw new IllegalArgumentException(
                            String.format("Missing install path property %s_ROOT",
                                    TestSuiteInfo.getInstance().getName()));
                }
                testCaseDir = new File(rootDirPath, "android-vts/testcases");
            }
            if (!testCaseDir.exists()) {
                throw new FileNotFoundException(String.format(
                        "Root directory doesn't exist %s", testCaseDir.getAbsolutePath()));
            }
            mTestCasesDir = testCaseDir.getAbsoluteFile();
            CLog.i(String.format(
                    "%s TEST CASES DIR: %s", TestSuiteInfo.getInstance().getName(), mTestCasesDir));
        }

        return mTestCasesDir;
    }
}