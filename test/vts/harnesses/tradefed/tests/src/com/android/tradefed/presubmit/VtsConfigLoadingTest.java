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
package com.android.tradefed.presubmit;

import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import com.android.tradefed.config.ConfigurationDescriptor;
import com.android.tradefed.config.ConfigurationException;
import com.android.tradefed.config.ConfigurationFactory;
import com.android.tradefed.config.IConfiguration;
import com.android.tradefed.testtype.IRemoteTest;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.io.File;
import java.io.FilenameFilter;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;

/**
 * Test that configuration in VTS can load and have expected properties.
 */
@RunWith(JUnit4.class)
public class VtsConfigLoadingTest {
    /**
     * List of the officially supported runners in VTS.
     */
    private static final Set<String> SUPPORTED_VTS_TEST_TYPE = new HashSet<>(Arrays.asList(
            "com.android.compatibility.common.tradefed.testtype.JarHostTest",
            "com.android.tradefed.testtype.AndroidJUnitTest",
            "com.android.tradefed.testtype.GTest",
            "com.android.tradefed.testtype.VtsMultiDeviceTest"));

    /**
     * Test that configuration shipped in Tradefed can be parsed.
     */
    @Test
    public void testConfigurationLoad() throws Exception {
        String vtsRoot = System.getProperty("VTS_ROOT");
        File testcases = new File(vtsRoot, "/android-vts/testcases/");
        if (!testcases.exists()) {
            fail(String.format("%s does not exists", testcases));
            return;
        }
        File[] listVtsConfig = testcases.listFiles(new FilenameFilter() {
            @Override
            public boolean accept(File dir, String name) {
                // Only check the VTS test config
                if (name.startsWith("Vts") && name.endsWith(".config")) {
                    return true;
                }
                return false;
            }
        });
        assertTrue(listVtsConfig.length > 0);
        for (File config : listVtsConfig) {
            IConfiguration c = ConfigurationFactory.getInstance().createConfigurationFromArgs(
                    new String[] {config.getAbsolutePath()});
            for (IRemoteTest test : c.getTests()) {
                // Check that all the tests runners are well supported.
                if (!SUPPORTED_VTS_TEST_TYPE.contains(test.getClass().getCanonicalName())) {
                    throw new ConfigurationException(
                            String.format("testtype %s is not officially supported by VTS.",
                                    test.getClass().getCanonicalName()));
                }
            }
        }
    }
}
