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
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.spy;

import com.android.tradefed.build.DeviceBuildInfo;
import com.android.tradefed.build.IDeviceBuildInfo;
import com.android.tradefed.config.IConfiguration;
import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.targetprep.ITargetPreparer;
import com.android.tradefed.testtype.Abi;
import com.android.tradefed.testtype.IAbi;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.testtype.InstrumentationTest;
import com.android.tradefed.testtype.UiAutomatorTest;
import com.android.tradefed.util.AbiUtils;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import java.io.File;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Set;

/** Unit tests for {@link AtestRunner}. */
@RunWith(JUnit4.class)
public class AtestRunnerTest {

    private static final String ABI = "armeabi-v7a";
    private static final String TEST_NAME_FMT = ABI + " %s";
    private static final String INSTRUMENTATION_TEST_NAME =
            String.format(TEST_NAME_FMT, "tf/instrumentation");

    private AtestRunner mSpyRunner;
    private OptionSetter setter;
    private IDeviceBuildInfo mBuildInfo;
    private ITestDevice mMockDevice;
    private String classA = "fully.qualified.classA";
    private String classB = "fully.qualified.classB";
    private String method1 = "method1";

    @Before
    public void setUp() throws Exception {
        mSpyRunner = spy(new AtestRunner());
        mBuildInfo = spy(new DeviceBuildInfo());
        mMockDevice = mock(ITestDevice.class);
        mSpyRunner.setBuild(mBuildInfo);
        mSpyRunner.setDevice(mMockDevice);

        // Hardcode the abis to avoid failures related to running the tests against a particular
        // abi build of tradefed.
        Set<IAbi> abis = new HashSet<>();
        abis.add(new Abi(ABI, AbiUtils.getBitness(ABI)));
        doReturn(abis).when(mSpyRunner).getAbis(mMockDevice);
        doReturn(new File("some-dir")).when(mBuildInfo).getTestsDir();
    }

    @Test
    public void testLoadTests_one() throws Exception {
        setter = new OptionSetter(mSpyRunner);
        setter.setOptionValue("include-filter", "tf/fake");
        LinkedHashMap<String, IConfiguration> configMap = mSpyRunner.loadTests();
        assertEquals(1, configMap.size());
        assertTrue(configMap.containsKey(String.format(TEST_NAME_FMT, "tf/fake")));
    }

    @Test
    public void testLoadTests_two() throws Exception {
        setter = new OptionSetter(mSpyRunner);
        setter.setOptionValue("include-filter", "tf/fake");
        setter.setOptionValue("include-filter", "tf/func");
        LinkedHashMap<String, IConfiguration> configMap = mSpyRunner.loadTests();
        assertEquals(2, configMap.size());
        assertTrue(configMap.containsKey(String.format(TEST_NAME_FMT, "tf/fake")));
        assertTrue(configMap.containsKey(String.format(TEST_NAME_FMT, "tf/func")));
    }

    @Test
    public void testLoadTests_filter() throws Exception {
        setter = new OptionSetter(mSpyRunner);
        setter.setOptionValue("include-filter", "tf/uiautomator");
        setter.setOptionValue("atest-include-filter", "tf/uiautomator:" + classA);
        setter.setOptionValue("atest-include-filter", "tf/uiautomator:" + classB + "#" + method1);
        LinkedHashMap<String, IConfiguration> configMap = mSpyRunner.loadTests();
        assertEquals(1, configMap.size());
        String testName = String.format(TEST_NAME_FMT, "tf/uiautomator");
        assertTrue(configMap.containsKey(testName));
        IConfiguration config = configMap.get(testName);
        List<IRemoteTest> tests = config.getTests();
        assertEquals(1, tests.size());
        UiAutomatorTest test = (UiAutomatorTest) tests.get(0);
        List<String> classFilters = new ArrayList<String>();
        classFilters.add(classA);
        classFilters.add(classB + "#" + method1);
        assertEquals(classFilters, test.getClassNames());
    }

    @Test
    public void testLoadTests_ignoreFilter() throws Exception {
        setter = new OptionSetter(mSpyRunner);
        setter.setOptionValue("include-filter", "suite/base-suite1");
        setter.setOptionValue("atest-include-filter", "suite/base-suite1:" + classA);
        LinkedHashMap<String, IConfiguration> configMap = mSpyRunner.loadTests();
        assertEquals(1, configMap.size());
        String testName = String.format(TEST_NAME_FMT, "suite/base-suite1");
        assertTrue(configMap.containsKey(testName));
        IConfiguration config = configMap.get(testName);
        List<IRemoteTest> tests = config.getTests();
        assertEquals(1, tests.size());
        BaseTestSuite test = (BaseTestSuite) tests.get(0);
        assertEquals(new HashSet<String>(), test.getIncludeFilter());
    }

    @Test
    public void testWaitForDebugger() throws Exception {
        OptionSetter setter = new OptionSetter(mSpyRunner);
        setter.setOptionValue("wait-for-debugger", "true");
        setter.setOptionValue("include-filter", "tf/instrumentation");
        LinkedHashMap<String, IConfiguration> configMap = mSpyRunner.loadTests();
        assertEquals(1, configMap.size());
        IConfiguration config = configMap.get(INSTRUMENTATION_TEST_NAME);
        IRemoteTest test = config.getTests().get(0);
        assertTrue(((InstrumentationTest) test).getDebug());
    }

    @Test
    public void testdisableTargetPreparers() throws Exception {
        OptionSetter setter = new OptionSetter(mSpyRunner);
        setter.setOptionValue("disable-target-preparers", "true");
        setter.setOptionValue("include-filter", "tf/instrumentation");
        LinkedHashMap<String, IConfiguration> configMap = mSpyRunner.loadTests();
        assertEquals(1, configMap.size());
        IConfiguration config = configMap.get(INSTRUMENTATION_TEST_NAME);
        for (ITargetPreparer targetPreparer : config.getTargetPreparers()) {
            assertTrue(targetPreparer.isDisabled());
        }
    }

    @Test
    public void testdisableTargetPreparersUnset() throws Exception {
        OptionSetter setter = new OptionSetter(mSpyRunner);
        setter.setOptionValue("include-filter", "tf/instrumentation");
        LinkedHashMap<String, IConfiguration> configMap = mSpyRunner.loadTests();
        assertEquals(1, configMap.size());
        IConfiguration config = configMap.get(INSTRUMENTATION_TEST_NAME);
        for (ITargetPreparer targetPreparer : config.getTargetPreparers()) {
            assertTrue(!targetPreparer.isDisabled());
        }
    }

    @Test
    public void testDisableTearDown() throws Exception {
        OptionSetter setter = new OptionSetter(mSpyRunner);
        setter.setOptionValue("disable-teardown", "true");
        setter.setOptionValue("include-filter", "tf/instrumentation");
        LinkedHashMap<String, IConfiguration> configMap = mSpyRunner.loadTests();
        IConfiguration config = configMap.get(INSTRUMENTATION_TEST_NAME);
        for (ITargetPreparer targetPreparer : config.getTargetPreparers()) {
            assertTrue(targetPreparer.isTearDownDisabled());
        }
    }

    @Test
    public void testDisableTearDownUnset() throws Exception {
        OptionSetter setter = new OptionSetter(mSpyRunner);
        setter.setOptionValue("include-filter", "tf/instrumentation");
        LinkedHashMap<String, IConfiguration> configMap = mSpyRunner.loadTests();
        assertEquals(1, configMap.size());
        IConfiguration config = configMap.get(INSTRUMENTATION_TEST_NAME);
        for (ITargetPreparer targetPreparer : config.getTargetPreparers()) {
            assertTrue(!targetPreparer.isTearDownDisabled());
        }
    }

    @Test
    public void testCreateModuleListener() throws Exception {
        OptionSetter setter = new OptionSetter(mSpyRunner);
        setter.setOptionValue("subprocess-report-port", "55555");
        List<ITestInvocationListener> listeners = mSpyRunner.createModuleListeners();
        assertEquals(1, listeners.size());
    }
}
