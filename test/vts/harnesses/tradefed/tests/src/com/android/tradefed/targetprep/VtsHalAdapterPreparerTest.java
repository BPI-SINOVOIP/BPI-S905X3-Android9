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

package com.android.tradefed.targetprep;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.fail;
import static org.mockito.Mockito.*;

import com.android.compatibility.common.tradefed.build.VtsCompatibilityInvocationHelper;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.testtype.IAbi;
import com.android.tradefed.util.CmdUtil;
import com.android.tradefed.util.FileUtil;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.mockito.InjectMocks;
import org.mockito.Mockito;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Vector;
import java.util.function.Predicate;
/**
 * Unit tests for {@link VtsHalAdapterPreparer}.
 */
@RunWith(JUnit4.class)
public final class VtsHalAdapterPreparerTest {
    private int THREAD_COUNT_DEFAULT = VtsHalAdapterPreparer.THREAD_COUNT_DEFAULT;
    private String SCRIPT_PATH = VtsHalAdapterPreparer.SCRIPT_PATH;
    private String LIST_HAL_CMD = VtsHalAdapterPreparer.LIST_HAL_CMD;

    private String VTS_NATIVE_TEST_DIR = "DATA/nativetest64/";
    private String TARGET_NATIVE_TEST_DIR = "/data/nativetest64/";
    private String TEST_HAL_ADAPTER_BINARY = "android.hardware.foo@1.0-adapter";
    private String TEST_HAL_PACKAGE = "android.hardware.foo@1.1";

    private class TestCmdUtil extends CmdUtil {
        public boolean mCmdSuccess = true;
        @Override
        public boolean waitCmdResultWithDelay(ITestDevice device, String cmd,
                Predicate<String> predicate) throws DeviceNotAvailableException {
            return mCmdSuccess;
        }

        @Override
        public boolean retry(ITestDevice device, String cmd, String validation_cmd,
                Predicate<String> predicate, int retry_count) throws DeviceNotAvailableException {
            device.executeShellCommand(cmd);
            return mCmdSuccess;
        }

        @Override
        public boolean retry(ITestDevice device, Vector<String> cmds, String validation_cmd,
                Predicate<String> predicate) throws DeviceNotAvailableException {
            for (String cmd : cmds) {
                device.executeShellCommand(cmd);
            }
            return mCmdSuccess;
        }

        @Override
        public void restartFramework(ITestDevice device) throws DeviceNotAvailableException {}

        @Override
        public void setSystemProperty(ITestDevice device, String name, String value)
                throws DeviceNotAvailableException {}
    }
    private TestCmdUtil mCmdUtil = new TestCmdUtil();
    VtsCompatibilityInvocationHelper mMockHelper = null;
    private class TestPreparer extends VtsHalAdapterPreparer {
        @Override
        VtsCompatibilityInvocationHelper createVtsHelper() {
            return mMockHelper;
        }
    }
    File mTestDir = null;

    @Mock private IBuildInfo mBuildInfo;
    @Mock private ITestDevice mDevice;
    @Mock private IAbi mAbi;
    @InjectMocks private TestPreparer mPreparer = new TestPreparer();

    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);
        // Create the base dirs
        mTestDir = FileUtil.createTempDir("vts-hal-adapter-preparer-unit-tests");
        new File(mTestDir, VTS_NATIVE_TEST_DIR).mkdirs();
        mMockHelper = new VtsCompatibilityInvocationHelper() {
            @Override
            public File getTestsDir() throws FileNotFoundException {
                return mTestDir;
            }
        };
        mPreparer.setCmdUtil(mCmdUtil);
        OptionSetter setter = new OptionSetter(mPreparer);
        setter.setOptionValue("adapter-binary-name", TEST_HAL_ADAPTER_BINARY);
        setter.setOptionValue("hal-package-name", TEST_HAL_PACKAGE);
        doReturn("64").when(mAbi).getBitness();
    }

    @After
    public void tearDown() throws Exception {
        FileUtil.recursiveDelete(mTestDir);
    }

    @Test
    public void testOnSetUpAdapterSingleInstance() throws Exception {
        File testAdapter = createTestAdapter();
        String output = "android.hardware.foo@1.1::IFoo/default";
        doReturn(output).doReturn("").when(mDevice).executeShellCommand(
                String.format(LIST_HAL_CMD, TEST_HAL_PACKAGE));

        mPreparer.setUp(mDevice, mBuildInfo);
        verify(mDevice, times(1))
                .pushFile(eq(testAdapter), eq(TARGET_NATIVE_TEST_DIR + TEST_HAL_ADAPTER_BINARY));
        String adapterCmd = String.format("%s /data/nativetest64/%s %s %s %d", SCRIPT_PATH,
                TEST_HAL_ADAPTER_BINARY, "IFoo", "default", THREAD_COUNT_DEFAULT);
        verify(mDevice, times(1)).executeShellCommand(eq(adapterCmd));
    }

    @Test
    public void testOnSetUpAdapterMultipleInstance() throws Exception {
        File testAdapter = createTestAdapter();
        String output = "android.hardware.foo@1.1::IFoo/default\n"
                + "android.hardware.foo@1.1::IFoo/test\n"
                + "android.hardware.foo@1.1::IFooSecond/default\n"
                + "android.hardware.foo@1.1::IFooSecond/slot/1\n";
        doReturn(output).doReturn("").when(mDevice).executeShellCommand(
                String.format(LIST_HAL_CMD, TEST_HAL_PACKAGE));

        mPreparer.setUp(mDevice, mBuildInfo);

        List<String> adapterCmds = new ArrayList<String>();
        adapterCmds.add(String.format("%s /data/nativetest64/%s %s %s %d", SCRIPT_PATH,
                TEST_HAL_ADAPTER_BINARY, "IFoo", "default", THREAD_COUNT_DEFAULT));
        adapterCmds.add(String.format("%s /data/nativetest64/%s %s %s %d", SCRIPT_PATH,
                TEST_HAL_ADAPTER_BINARY, "IFoo", "test", THREAD_COUNT_DEFAULT));
        adapterCmds.add(String.format("%s /data/nativetest64/%s %s %s %d", SCRIPT_PATH,
                TEST_HAL_ADAPTER_BINARY, "IFooSecond", "default", THREAD_COUNT_DEFAULT));
        adapterCmds.add(String.format("%s /data/nativetest64/%s %s %s %d", SCRIPT_PATH,
                TEST_HAL_ADAPTER_BINARY, "IFooSecond", "slot/1", THREAD_COUNT_DEFAULT));

        for (String cmd : adapterCmds) {
            verify(mDevice, times(1)).executeShellCommand(eq(cmd));
        }
    }

    @Test
    public void testOnSetupAdapterNotFound() throws Exception {
        try {
            mPreparer.setUp(mDevice, mBuildInfo);
        } catch (TargetSetupError e) {
            assertEquals("Could not push adapter.", e.getMessage());
            return;
        }
        fail();
    }

    @Test
    public void testOnSetupServiceNotAvailable() throws Exception {
        File testAdapter = createTestAdapter();
        doReturn("").when(mDevice).executeShellCommand(
                String.format(LIST_HAL_CMD, TEST_HAL_PACKAGE));
        mPreparer.setUp(mDevice, mBuildInfo);
    }

    @Test
    public void testOnSetUpAdapterFailed() throws Exception {
        File testAdapter = createTestAdapter();
        String output = "android.hardware.foo@1.1::IFoo/default";
        doReturn(output).when(mDevice).executeShellCommand(
                String.format(LIST_HAL_CMD, TEST_HAL_PACKAGE));
        mCmdUtil.mCmdSuccess = false;
        try {
            mPreparer.setUp(mDevice, mBuildInfo);
        } catch (TargetSetupError e) {
            assertEquals("HAL adapter setup failed.", e.getMessage());
            return;
        }
        fail();
    }

    @Test
    public void testOnTearDownRestoreFailed() throws Exception {
        mCmdUtil.mCmdSuccess = false;
        try {
            mPreparer.setCmdUtil(mCmdUtil);
            mPreparer.addCommand("one");
            mPreparer.tearDown(mDevice, mBuildInfo, null);
        } catch (AssertionError | DeviceNotAvailableException e) {
            assertEquals("HAL restore failed.", e.getMessage());
            return;
        }
        fail();
    }

    /**
     * Helper method to create a test adapter under mTestDir.
     *
     * @return created test file.
     */
    private File createTestAdapter() throws IOException {
        File testAdapter = new File(mTestDir, VTS_NATIVE_TEST_DIR + TEST_HAL_ADAPTER_BINARY);
        testAdapter.createNewFile();
        return testAdapter;
    }
}
