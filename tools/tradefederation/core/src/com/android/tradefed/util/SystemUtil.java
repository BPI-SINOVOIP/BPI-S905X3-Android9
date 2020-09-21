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

package com.android.tradefed.util;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.build.IDeviceBuildInfo;
import com.android.tradefed.log.LogUtil.CLog;

import com.google.common.annotations.VisibleForTesting;

import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;


/** Utility class for making system calls. */
public class SystemUtil {

    @VisibleForTesting static SystemUtil singleton = new SystemUtil();

    // Environment variables for the test cases directory in target out directory and host out
    // directory.
    public enum EnvVariable {
        ANDROID_TARGET_OUT_TESTCASES,
        ANDROID_HOST_OUT_TESTCASES,
    }

    static final String ENV_ANDROID_PRODUCT_OUT = "ANDROID_PRODUCT_OUT";

    private static final String HOST_TESTCASES = "host/testcases";
    private static final String TARGET_TESTCASES = "target/testcases";

    /**
     * Get the value of an environment variable.
     *
     * <p>The wrapper function is created for mock in unit test.
     *
     * @param name the name of the environment variable.
     * @return {@link String} value of the given environment variable.
     */
    @VisibleForTesting
    String getEnv(String name) {
        return System.getenv(name);
    }

    /** Get a list of {@link File} pointing to tests directories external to Tradefed. */
    public static List<File> getExternalTestCasesDirs() {
        List<File> testCasesDirs = new ArrayList<File>();
        // TODO(b/36782030): Support running both HOST and TARGET tests.
        List<String> testCasesDirNames =
                // List order matters. ConfigurationFactory caller uses first dir with test config.
                Arrays.asList(
                        singleton.getEnv(EnvVariable.ANDROID_TARGET_OUT_TESTCASES.name()),
                        singleton.getEnv(EnvVariable.ANDROID_HOST_OUT_TESTCASES.name()));
        for (String testCasesDirName : testCasesDirNames) {
            if (testCasesDirName != null) {
                File dir = new File(testCasesDirName);
                if (dir.exists() && dir.isDirectory()) {
                    CLog.d("Found test case dir: %s", testCasesDirName);
                    testCasesDirs.add(dir);
                } else {
                    CLog.w(
                            "Path %s for test cases directory does not exist or it's not a "
                                    + "directory.",
                            testCasesDirName);
                }
            }
        }
        return testCasesDirs;
    }

    /**
     * Get the file associated with the env. variable.
     *
     * @param envVariable ANDROID_TARGET_OUT_TESTCASES or ANDROID_HOST_OUT_TESTCASES
     * @return The directory associated.
     */
    public static File getExternalTestCasesDir(EnvVariable envVariable) {
        String var = System.getenv(envVariable.name());
        if (var == null) {
            return null;
        }
        File dir = new File(var);
        if (dir.exists() && dir.isDirectory()) {
            CLog.d("Found test case dir: %s for %s.", dir.getAbsolutePath(), envVariable.name());
            return dir;
        }
        return null;
    }

    /**
     * Get a list of {@link File} of the test cases directories
     *
     * @param buildInfo the build artifact information. Set it to null if build info is not
     *     available or there is no need to get test cases directories from build info.
     * @return a list of {@link File} of directories of the test cases folder of build output, based
     *     on the value of environment variables and the given build info.
     */
    public static List<File> getTestCasesDirs(IBuildInfo buildInfo) {
        List<File> testCasesDirs = new ArrayList<File>();
        testCasesDirs.addAll(getExternalTestCasesDirs());

        // TODO: Remove this logic after Versioned TF V2 is implemented, in which staging build
        // artifact will be done by the parent process, and the test cases dirs will be set by
        // environment variables.
        // Add tests dir from build info.
        if (buildInfo instanceof IDeviceBuildInfo) {
            IDeviceBuildInfo deviceBuildInfo = (IDeviceBuildInfo) buildInfo;
            File testsDir = deviceBuildInfo.getTestsDir();
            // Add all possible paths to the testcases directory list.
            if (testsDir != null) {
                testCasesDirs.addAll(
                        Arrays.asList(
                                testsDir,
                                FileUtil.getFileForPath(testsDir, HOST_TESTCASES),
                                FileUtil.getFileForPath(testsDir, TARGET_TESTCASES)));
            }
        }

        return testCasesDirs;
    }

    /**
     * Gets the product specific output dir from an Android build tree. Typically this location
     * contains images for various device partitions, bootloader, radio and so on.
     *
     * <p>Note: the method does not guarantee that this path exists.
     *
     * @return the location of the output dir or <code>null</code> if the current build is not
     */
    public static File getProductOutputDir() {
        String path = singleton.getEnv(ENV_ANDROID_PRODUCT_OUT);
        if (path == null) {
            return null;
        } else {
            return new File(path);
        }
    }
}
