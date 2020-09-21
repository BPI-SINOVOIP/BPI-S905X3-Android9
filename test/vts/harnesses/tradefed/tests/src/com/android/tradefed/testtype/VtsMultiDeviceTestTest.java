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
package com.android.tradefed.testtype;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.build.IFolderBuildInfo;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;
import com.android.tradefed.util.IRunUtil;
import com.android.tradefed.util.StreamUtil;
import com.android.tradefed.util.VtsPythonRunnerHelper;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.util.HashMap;
import java.util.Map;
import org.json.JSONObject;
import org.easymock.EasyMock;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/**
 * Unit tests for {@link VtsMultiDeviceTest}.
 * This class requires testcase config files.
 * The working directory is assumed to be
 * test/
 * which contains the same config as the build output
 * out/host/linux-x86/vts/android-vts/testcases/
 */
@RunWith(JUnit4.class)
public class VtsMultiDeviceTestTest {
    private static final String PYTHON_BINARY = "python";
    private static final String PYTHON_PATH = "python/path";
    private static final String PYTHON_DIR = "mock/";
    private static final String TEST_CASE_PATH =
        "vts/testcases/host/sample/SampleLightTest";

    private ITestInvocationListener mMockInvocationListener = null;
    private VtsMultiDeviceTest mTest = null;
    private VtsPythonRunnerHelper mVtsPythonRunnerHelper = null;
    /**
     * Helper to initialize the various EasyMocks we'll need.
     */
    @Before
    public void setUp() throws Exception {
        mMockInvocationListener = EasyMock.createMock(ITestInvocationListener.class);
        mTest = new VtsMultiDeviceTest() {
            // TODO: Test this method.
            @Override
            protected void updateVtsRunnerTestConfig(JSONObject jsonObject) {
                return;
            }
            @Override
            protected VtsPythonRunnerHelper createVtsPythonRunnerHelper() {
                return mVtsPythonRunnerHelper;
            }
        };
        mTest.setBuild(createMockBuildInfo());
        mTest.setTestCasePath(TEST_CASE_PATH);
        mTest.setTestConfigPath(VtsMultiDeviceTest.DEFAULT_TESTCASE_CONFIG_PATH);
    }

    /**
     * Check VTS Python command strings.
     */
    private void assertCommand(String[] cmd) {
        assertEquals(cmd[0], PYTHON_DIR + PYTHON_BINARY);
        assertEquals(cmd[1], "-m");
        assertEquals(cmd[2], TEST_CASE_PATH.replace("/", "."));
        assertTrue(cmd[3].endsWith(".json"));
        assertEquals(cmd.length, 4);
    }

    /**
     * Create files in log directory.
     */
    private void createResult(String jsonPath) throws Exception {
        String logPath = null;
        try (FileInputStream fi = new FileInputStream(jsonPath)) {
            JSONObject configJson = new JSONObject(StreamUtil.getStringFromStream(fi));
            logPath = (String) configJson.get(VtsMultiDeviceTest.LOG_PATH);
        }
        // create a test result on log path
        try (FileWriter fw = new FileWriter(new File(logPath, "test_run_summary.json"))) {
            JSONObject outJson = new JSONObject();
            fw.write(outJson.toString());
        }
        new File(logPath, VtsMultiDeviceTest.REPORT_MESSAGE_FILE_NAME).createNewFile();
    }


    /**
     * Create a mock IBuildInfo with necessary getter methods.
     */
    private static IBuildInfo createMockBuildInfo() {
        Map<String, String> buildAttributes = new HashMap<String, String>();
        buildAttributes.put("ROOT_DIR", "DIR_NOT_EXIST");
        buildAttributes.put("ROOT_DIR2", "DIR_NOT_EXIST");
        buildAttributes.put("SUITE_NAME", "JUNIT_TEST_SUITE");
        IFolderBuildInfo buildInfo = EasyMock.createNiceMock(IFolderBuildInfo.class);
        EasyMock.expect(buildInfo.getBuildId()).andReturn("BUILD_ID").anyTimes();
        EasyMock.expect(buildInfo.getBuildTargetName()).andReturn("BUILD_TARGET_NAME").anyTimes();
        EasyMock.expect(buildInfo.getTestTag()).andReturn("TEST_TAG").anyTimes();
        EasyMock.expect(buildInfo.getDeviceSerial()).andReturn("1234567890ABCXYZ").anyTimes();
        EasyMock.expect(buildInfo.getRootDir()).andReturn(new File("DIR_NOT_EXIST")).anyTimes();
        EasyMock.expect(buildInfo.getBuildAttributes()).andReturn(buildAttributes).anyTimes();
        EasyMock.expect(buildInfo.getFile(EasyMock.eq("PYTHONPATH")))
                .andReturn(new File("DIR_NOT_EXIST"))
                .anyTimes();
        EasyMock.expect(buildInfo.getFile(EasyMock.eq("VIRTUALENVPATH")))
                .andReturn(new File("DIR_NOT_EXIST"))
                .anyTimes();
        EasyMock.replay(buildInfo);
        return buildInfo;
    }

    /**
     * Create a mock ITestDevice with necessary getter methods.
     */
    private static ITestDevice createMockDevice() {
        // TestDevice
        ITestDevice device = EasyMock.createNiceMock(ITestDevice.class);
        try {
            EasyMock.expect(device.getSerialNumber()).andReturn("1234567890ABCXYZ").anyTimes();
            EasyMock.expect(device.getBuildAlias()).andReturn("BUILD_ALIAS").anyTimes();
            EasyMock.expect(device.getBuildFlavor()).andReturn("BUILD_FLAVOR").anyTimes();
            EasyMock.expect(device.getBuildId()).andReturn("BUILD_ID").anyTimes();
            EasyMock.expect(device.getProductType()).andReturn("PRODUCT_TYPE").anyTimes();
            EasyMock.expect(device.getProductVariant()).andReturn("PRODUCT_VARIANT").anyTimes();
        } catch (DeviceNotAvailableException e) {
            fail();
        }
        EasyMock.replay(device);
        return device;
    }

    /**
     * Create a process helper which mocks status of a running process.
     */
    private VtsPythonRunnerHelper createMockVtsPythonRunnerHelper(CommandStatus status) {
        IBuildInfo buildInfo = createMockBuildInfo();
        return new VtsPythonRunnerHelper(buildInfo) {
            @Override
            public String runPythonRunner(
                    String[] cmd, CommandResult commandResult, long testTimeout) {
                assertCommand(cmd);
                try {
                    createResult(cmd[3]);
                } catch (Exception e) {
                    throw new RuntimeException(e);
                }
                commandResult.setStatus(status);
                return null;
            }
            @Override
            public String getPythonBinary() {
                return PYTHON_DIR + PYTHON_BINARY;
            }
            @Override
            public String getPythonPath() {
                return PYTHON_PATH;
            }
        };
    }

    /**
     * Test the run method with a normal input.
     */
    @Test
    public void testRunNormalInput() {
        mVtsPythonRunnerHelper = createMockVtsPythonRunnerHelper(CommandStatus.SUCCESS);
        mTest.setDevice(createMockDevice());
        try {
            mTest.run(mMockInvocationListener);
        } catch (IllegalArgumentException e) {
            // not expected
            fail();
            e.printStackTrace();
        } catch (DeviceNotAvailableException e) {
            // not expected
            fail();
            e.printStackTrace();
        }
    }

    /**
     * Test the run method when the device is set null.
     */
    @Test
    public void testRunDeviceNotAvailable() {
        mTest.setDevice(null);
        try {
            mTest.run(mMockInvocationListener);
            fail();
       } catch (IllegalArgumentException e) {
            // not expected
            fail();
       } catch (DeviceNotAvailableException e) {
            // expected
       }
    }
}
