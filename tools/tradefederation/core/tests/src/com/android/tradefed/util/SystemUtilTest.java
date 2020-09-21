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
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.util.SystemUtil.EnvVariable;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.mockito.Mockito;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/** Unit tests for {@link SystemUtil} */
@RunWith(JUnit4.class)
public class SystemUtilTest {

    /**
     * test {@link SystemUtil#getTestCasesDirs(IBuildInfo)} to make sure it gets directories from
     * environment variables.
     */
    @Test
    public void testGetExternalTestCasesDirs() throws IOException {
        File targetOutDir = null;
        File hostOutDir = null;
        try {
            targetOutDir = FileUtil.createTempDir("target_out_dir");
            hostOutDir = FileUtil.createTempDir("host_out_dir");

            SystemUtil.singleton = Mockito.mock(SystemUtil.class);
            Mockito.when(
                            SystemUtil.singleton.getEnv(
                                    EnvVariable.ANDROID_TARGET_OUT_TESTCASES.name()))
                    .thenReturn(targetOutDir.getAbsolutePath());
            Mockito.when(SystemUtil.singleton.getEnv(EnvVariable.ANDROID_HOST_OUT_TESTCASES.name()))
                    .thenReturn(hostOutDir.getAbsolutePath());

            List<File> testCasesDirs = new ArrayList<File>(SystemUtil.getExternalTestCasesDirs());
            assertTrue(testCasesDirs.get(0).equals(targetOutDir));
            assertTrue(testCasesDirs.get(1).equals(hostOutDir));
        } finally {
            FileUtil.recursiveDelete(targetOutDir);
            FileUtil.recursiveDelete(hostOutDir);
        }
    }

    /**
     * test {@link SystemUtil#getTestCasesDirs(IBuildInfo)} to make sure no exception thrown if no
     * environment variable is set or the directory does not exist.
     */
    @Test
    public void testGetExternalTestCasesDirsNoDir() {
        File targetOutDir = new File("/path/not/exist_1");

        SystemUtil.singleton = Mockito.mock(SystemUtil.class);
        Mockito.when(SystemUtil.singleton.getEnv(EnvVariable.ANDROID_TARGET_OUT_TESTCASES.name()))
                .thenReturn(targetOutDir.getAbsolutePath());
        Mockito.when(SystemUtil.singleton.getEnv(EnvVariable.ANDROID_HOST_OUT_TESTCASES.name()))
                .thenReturn(null);
        List<File> testCasesDirs = new ArrayList<File>(SystemUtil.getExternalTestCasesDirs());
        assertEquals(0, testCasesDirs.size());
    }
}
