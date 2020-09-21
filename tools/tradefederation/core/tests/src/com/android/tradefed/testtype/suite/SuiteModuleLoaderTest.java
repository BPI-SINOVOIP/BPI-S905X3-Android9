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
package com.android.tradefed.testtype.suite;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import com.android.tradefed.config.IConfiguration;
import com.android.tradefed.config.Option;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.testtype.Abi;
import com.android.tradefed.testtype.IAbi;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.util.FileUtil;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;

/** Unit tests for {@link SuiteModuleLoader}. */
@RunWith(JUnit4.class)
public class SuiteModuleLoaderTest {

    private static final String TEST_CONFIG =
            "<configuration description=\"Runs a stub tests part of some suite\">\n"
                    + "    <test class=\"com.android.tradefed.testtype.suite.SuiteModuleLoaderTest"
                    + "$TestInject\" />\n"
                    + "</configuration>";

    private SuiteModuleLoader mRepo;
    private File mTestsDir;
    private Set<IAbi> mAbis;

    @Before
    public void setUp() throws Exception {
        mRepo =
                new SuiteModuleLoader(
                        new LinkedHashMap<String, List<SuiteTestFilter>>(),
                        new LinkedHashMap<String, List<SuiteTestFilter>>(),
                        new ArrayList<>(),
                        new ArrayList<>());
        mTestsDir = FileUtil.createTempDir("suite-module-loader-tests");
        mAbis = new HashSet<>();
        mAbis.add(new Abi("armeabi-v7a", "32"));
    }

    @After
    public void tearDown() {
        FileUtil.recursiveDelete(mTestsDir);
    }

    private void createModuleConfig(String moduleName) throws IOException {
        File module = new File(mTestsDir, moduleName + SuiteModuleLoader.CONFIG_EXT);
        FileUtil.writeToFile(TEST_CONFIG, module);
    }

    public static class TestInject implements IRemoteTest {
        @Option(name = "simple-string")
        public String test = null;

        @Option(name = "list-string")
        public List<String> testList = new ArrayList<>();

        @Option(name = "map-string")
        public Map<String, String> testMap = new HashMap<>();

        @Override
        public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {}
    }

    /** Test an end-to-end injection of --module-arg. */
    @Test
    public void testInjectConfigOptions_moduleArgs() throws Exception {
        List<String> moduleArgs = new ArrayList<>();
        moduleArgs.add("module1:simple-string:value1");

        moduleArgs.add("module1:list-string:value2");
        moduleArgs.add("module1:list-string:value3");
        moduleArgs.add("module1:list-string:set-option:moreoption");
        moduleArgs.add("module1:map-string:set-option:=moreoption");

        createModuleConfig("module1");

        mRepo =
                new SuiteModuleLoader(
                        new LinkedHashMap<String, List<SuiteTestFilter>>(),
                        new LinkedHashMap<String, List<SuiteTestFilter>>(),
                        new ArrayList<>(),
                        moduleArgs);
        List<String> patterns = new ArrayList<>();
        patterns.add(".*.config");
        patterns.add(".*.xml");
        LinkedHashMap<String, IConfiguration> res =
                mRepo.loadConfigsFromDirectory(mTestsDir, mAbis, null, null, patterns);
        assertNotNull(res.get("armeabi-v7a module1"));
        IConfiguration config = res.get("armeabi-v7a module1");

        TestInject checker = (TestInject) config.getTests().get(0);
        assertEquals("value1", checker.test);
        // Check list
        assertTrue(checker.testList.size() == 3);
        assertTrue(checker.testList.contains("value2"));
        assertTrue(checker.testList.contains("value3"));
        assertTrue(checker.testList.contains("set-option:moreoption"));
        // Chech map
        assertTrue(checker.testMap.size() == 1);
        assertEquals("moreoption", checker.testMap.get("set-option"));
    }

    /** Test an end-to-end injection of --test-arg. */
    @Test
    public void testInjectConfigOptions_testArgs() throws Exception {
        List<String> testArgs = new ArrayList<>();
        testArgs.add(
                "com.android.tradefed.testtype.suite.SuiteModuleLoaderTest$TestInject:"
                        + "simple-string:value1");
        testArgs.add(
                "com.android.tradefed.testtype.suite.SuiteModuleLoaderTest$TestInject:"
                        + "list-string:value2");
        testArgs.add(
                "com.android.tradefed.testtype.suite.SuiteModuleLoaderTest$TestInject:"
                        + "list-string:value3");
        testArgs.add(
                "com.android.tradefed.testtype.suite.SuiteModuleLoaderTest$TestInject:"
                        + "list-string:set-option:moreoption");
        testArgs.add(
                "com.android.tradefed.testtype.suite.SuiteModuleLoaderTest$TestInject:"
                        + "map-string:set-option:=moreoption");

        createModuleConfig("module1");

        mRepo =
                new SuiteModuleLoader(
                        new LinkedHashMap<String, List<SuiteTestFilter>>(),
                        new LinkedHashMap<String, List<SuiteTestFilter>>(),
                        testArgs,
                        new ArrayList<>());
        List<String> patterns = new ArrayList<>();
        patterns.add(".*.config");
        patterns.add(".*.xml");
        LinkedHashMap<String, IConfiguration> res =
                mRepo.loadConfigsFromDirectory(mTestsDir, mAbis, null, null, patterns);
        assertNotNull(res.get("armeabi-v7a module1"));
        IConfiguration config = res.get("armeabi-v7a module1");

        TestInject checker = (TestInject) config.getTests().get(0);
        assertEquals("value1", checker.test);
        // Check list
        assertTrue(checker.testList.size() == 3);
        assertTrue(checker.testList.contains("value2"));
        assertTrue(checker.testList.contains("value3"));
        assertTrue(checker.testList.contains("set-option:moreoption"));
        // Chech map
        assertTrue(checker.testMap.size() == 1);
        assertEquals("moreoption", checker.testMap.get("set-option"));
    }
}
