/*
 * Copyright (C) 2016 The Android Open Source Project
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
package com.android.tradefed.config;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.fail;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Unit Tests for {@link ConfigurationDescriptor} */
@RunWith(JUnit4.class)
public class ConfigurationDescriptorTest {

    private ConfigurationFactory mFactory;

    @Before
    public void setUp() throws Exception {
        mFactory =
                new ConfigurationFactory() {
                    @Override
                    protected String getConfigPrefix() {
                        return "testconfigs/";
                    }
                };
    }

    /**
     * Test that even if an option exists in a ConfigurationDescriptor, it cannot receive value via
     * command line.
     */
    @Test
    public void testRejectCommandLine() throws Exception {
        IConfiguration config = mFactory.createConfigurationFromArgs(new String[] {"test-config"});
        OptionSetter setter = new OptionSetter(config.getConfigurationDescription());
        // Confirm that options exists for the object
        setter.setOptionValue("test-suite-tag", "Test");
        try {
            // This will still throw since it cannot be set via command line.
            mFactory.createConfigurationFromArgs(
                    new String[] {"test-config", "--test-suite-tag", "TEST"});
            fail("Should have thrown an exception.");
        } catch (OptionNotAllowedException expected) {
            assertEquals(
                    "Option test-suite-tag cannot be specified via command line. "
                            + "Only in the configuration xml.",
                    expected.getMessage());
        }
    }
}
