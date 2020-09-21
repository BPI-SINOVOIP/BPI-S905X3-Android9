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
package com.android.tradefed.testtype.suite;

import static org.junit.Assert.assertEquals;

import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.StreamUtil;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.io.File;
import java.io.FileInputStream;
import java.util.Properties;

/** Unit tests for {@link TestSuiteInfo}. */
@RunWith(JUnit4.class)
public class TestSuiteInfoTest {

    /** Ensure that the instance default values are stubs. */
    @Test
    public void testDefaultValues() {
        TestSuiteInfo instance = TestSuiteInfo.getInstance();
        if (instance.didLoadFromProperties()) {
            // If a property file was found on the classpath, skip this test as it will probably
            // fail. This test is meant to check in isolation.
            return;
        }
        assertEquals("[stub build number]", instance.getBuildNumber());
        assertEquals("[stub target arch]", instance.getTargetArch());
        assertEquals("[stub name]", instance.getName());
        assertEquals("[stub fullname]", instance.getFullName());
        assertEquals("[stub version]", instance.getVersion());
    }

    /**
     * Check that the format of the loaded property will result in the underlying {@link Properties}
     * to be populated properly.
     */
    @Test
    public void testLoadConfig() throws Exception {
        File propertyFile = FileUtil.createTempFile("test-suite-prop", ".properties");
        FileInputStream stream = null;
        try {
            FileUtil.writeToFile(
                    "build_number = eng.build.5\n"
                            + "target_arch = arm64\n"
                            + "name = CTS\n"
                            + "fullname = Compatibility Test Suite\n"
                            + "version = 7.0",
                    propertyFile);
            TestSuiteInfo instance = TestSuiteInfo.getInstance();
            stream = new FileInputStream(propertyFile);
            Properties p = instance.loadSuiteInfo(stream);
            assertEquals("eng.build.5", p.getProperty("build_number"));
            assertEquals("arm64", p.getProperty("target_arch"));
            assertEquals("CTS", p.getProperty("name"));
            assertEquals("Compatibility Test Suite", p.getProperty("fullname"));
            assertEquals("7.0", p.getProperty("version"));
        } finally {
            FileUtil.deleteFile(propertyFile);
            StreamUtil.close(stream);
        }
    }
}
