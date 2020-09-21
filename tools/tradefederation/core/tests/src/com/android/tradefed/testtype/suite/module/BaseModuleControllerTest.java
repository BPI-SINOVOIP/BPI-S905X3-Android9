/*
 * Copyright (C) 2018 The Android Open Source Project
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
package com.android.tradefed.testtype.suite.module;

import com.android.tradefed.config.ConfigurationFactory;
import com.android.tradefed.util.FileUtil;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.io.File;
import java.io.IOException;

/** Unit tests for {@link BaseModuleController}. */
@RunWith(JUnit4.class)
public class BaseModuleControllerTest {

    private static final String TEST_CONFIG =
            "<configuration>"
                    + "  <test class=\"com.android.tradefed.testtype.AndroidJUnitTest\" />"
                    + "  <test class=\"com.android.tradefed.testtype.HostTest\" />"
                    + "  <test class=\"com.android.tradefed.testtype.GoogleBenchmarkTest\" />"
                    + "  <test class=\"com.android.tradefed.testtype.GTest\" />"
                    + "  <object type=\"module_controller\" class=\"com.android.tradefed.testtype.suite.module.TestFailureModuleController\" />"
                    + "</configuration>";

    private File mTestConfig;

    @Before
    public void setUp() throws IOException {
        mTestConfig = FileUtil.createTempFile("base-module-controller-test", ".config");
        FileUtil.writeToFile(TEST_CONFIG, mTestConfig);
    }

    @After
    public void tearDown() {
        FileUtil.deleteFile(mTestConfig);
    }

    /** Ensure that BaseModuleController options do not conflict with our usual runner. */
    @Test
    public void testCheckCompatibility() throws Exception {
        ConfigurationFactory.getInstance()
                .createConfigurationFromArgs(new String[] {mTestConfig.getAbsolutePath()});
    }
}
