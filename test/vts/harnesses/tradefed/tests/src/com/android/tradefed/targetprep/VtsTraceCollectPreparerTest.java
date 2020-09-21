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

import com.android.compatibility.common.tradefed.build.CompatibilityBuildHelper;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.targetprep.TargetSetupError;

import junit.framework.AssertionFailedError;

import org.easymock.EasyMock;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.io.File;
import java.io.FileNotFoundException;
import java.util.NoSuchElementException;

/**
 * Unit tests for {@link VtsTraceCollectPreparer}.
 */
@RunWith(JUnit4.class)
public final class VtsTraceCollectPreparerTest {
    private String SELINUX_PERMISSIVE = VtsTraceCollectPreparer.SELINUX_PERMISSIVE;
    private String VTS_LIB_DIR_32 = VtsTraceCollectPreparer.VTS_LIB_DIR_32;
    private String VTS_LIB_DIR_64 = VtsTraceCollectPreparer.VTS_LIB_DIR_64;
    private String VTS_BINARY_DIR = VtsTraceCollectPreparer.VTS_BINARY_DIR;
    private String VTS_TMP_LIB_DIR_32 = VtsTraceCollectPreparer.VTS_TMP_LIB_DIR_32;
    private String VTS_TMP_LIB_DIR_64 = VtsTraceCollectPreparer.VTS_TMP_LIB_DIR_64;
    private String VTS_TMP_DIR = VtsTraceCollectPreparer.VTS_TMP_DIR;
    private String PROFILING_CONFIGURE_BINARY = VtsTraceCollectPreparer.PROFILING_CONFIGURE_BINARY;
    private String TRACE_PATH = VtsTraceCollectPreparer.TRACE_PATH;
    private String LOCAL_TRACE_DIR = VtsTraceCollectPreparer.LOCAL_TRACE_DIR;

    private static final String TEST_VTS_PROFILER = "test-vts.profiler.so";
    private static final String TEST_VTS_LIB = "libvts-for-test.so";
    private static final String UNRELATED_LIB = "somelib.so";

    private VtsTraceCollectPreparer mPreparer;
    private IBuildInfo mMockBuildInfo;
    private ITestDevice mMockDevice;
    private CompatibilityBuildHelper mMockHelper;
    private File mTestDir;

    @Before
    public void setUp() throws Exception {
        mMockDevice = EasyMock.createNiceMock(ITestDevice.class);
        mMockBuildInfo = EasyMock.createNiceMock(IBuildInfo.class);
        mTestDir = FileUtil.createTempDir("vts-trace-collect-unit-tests");
        mMockHelper = new CompatibilityBuildHelper(mMockBuildInfo) {
            @Override
            public File getTestsDir() throws FileNotFoundException {
                return mTestDir;
            }
            public File getResultDir() throws FileNotFoundException {
                return mTestDir;
            }
        };
        mPreparer = new VtsTraceCollectPreparer() {
            @Override
            CompatibilityBuildHelper createBuildHelper(IBuildInfo buildInfo) {
                return mMockHelper;
            }
        };
    }

    @After
    public void tearDown() throws Exception {
        // Cleanup test files.
        FileUtil.recursiveDelete(mTestDir);
    }

    @Test
    public void testOnSetUp() throws Exception {
        // Create the base dirs
        new File(mTestDir, VTS_LIB_DIR_32).mkdirs();
        new File(mTestDir, VTS_LIB_DIR_64).mkdirs();
        // Files that should be pushed.
        File testProfilerlib32 = createTestFile(TEST_VTS_PROFILER, "32");
        File testProfilerlib64 = createTestFile(TEST_VTS_PROFILER, "64");
        File testVtslib32 = createTestFile(TEST_VTS_LIB, "32");
        File testVtslib64 = createTestFile(TEST_VTS_LIB, "64");
        // Files that should not be pushed.
        File testUnrelatedlib32 = createTestFile(UNRELATED_LIB, "32");
        File testUnrelatedlib64 = createTestFile(UNRELATED_LIB, "64");

        EasyMock.expect(mMockDevice.pushFile(EasyMock.eq(testProfilerlib32),
                                EasyMock.eq(VTS_TMP_LIB_DIR_32 + TEST_VTS_PROFILER)))
                .andReturn(true)
                .times(1);
        EasyMock.expect(mMockDevice.pushFile(EasyMock.eq(testProfilerlib64),
                                EasyMock.eq(VTS_TMP_LIB_DIR_64 + TEST_VTS_PROFILER)))
                .andReturn(true)
                .times(1);
        EasyMock.expect(mMockDevice.pushFile(EasyMock.eq(testVtslib32),
                                EasyMock.eq(VTS_TMP_LIB_DIR_32 + TEST_VTS_LIB)))
                .andReturn(true)
                .times(1);
        EasyMock.expect(mMockDevice.pushFile(EasyMock.eq(testVtslib64),
                                EasyMock.eq(VTS_TMP_LIB_DIR_64 + TEST_VTS_LIB)))
                .andReturn(true)
                .times(1);
        EasyMock.expect(mMockDevice.pushFile(EasyMock.eq(testUnrelatedlib32),
                                EasyMock.eq(VTS_TMP_LIB_DIR_32 + UNRELATED_LIB)))
                .andThrow(new AssertionFailedError())
                .anyTimes();
        EasyMock.expect(mMockDevice.pushFile(EasyMock.eq(testUnrelatedlib64),
                                EasyMock.eq(VTS_TMP_LIB_DIR_64 + UNRELATED_LIB)))
                .andThrow(new AssertionFailedError())
                .anyTimes();
        EasyMock.expect(mMockDevice.pushFile(EasyMock.eq(new File(mTestDir,
                                                     VTS_BINARY_DIR + PROFILING_CONFIGURE_BINARY)),
                                EasyMock.eq(VTS_TMP_DIR + PROFILING_CONFIGURE_BINARY)))
                .andReturn(true)
                .times(1);
        EasyMock.expect(mMockDevice.executeShellCommand(EasyMock.eq("getenforce")))
                .andReturn("")
                .times(1);
        EasyMock.expect(mMockDevice.executeShellCommand(
                                EasyMock.eq("setenforce " + SELINUX_PERMISSIVE)))
                .andReturn("")
                .times(1);

        // Configure the trace directory path.
        File traceDir = new File(mTestDir, LOCAL_TRACE_DIR);
        mMockBuildInfo.addBuildAttribute(TRACE_PATH, traceDir.getAbsolutePath());
        EasyMock.expectLastCall().times(1);
        EasyMock.replay(mMockDevice, mMockBuildInfo);

        // Run setUp.
        mPreparer.setUp(mMockDevice, mMockBuildInfo);
        EasyMock.verify(mMockDevice, mMockBuildInfo);
    }

    @Test
    public void testOnSetUpPushFileException() throws Exception {
        EasyMock.expect(mMockDevice.pushFile(EasyMock.anyObject(), EasyMock.anyObject()))
                .andThrow(new NoSuchElementException("file not found."));
        EasyMock.replay(mMockDevice);
        try {
            mPreparer.setUp(mMockDevice, mMockBuildInfo);
            EasyMock.verify(mMockDevice);
        } catch (TargetSetupError e) {
            // Expected.
            assertEquals("Could not push profiler.", e.getMessage());
            return;
        }
        fail();
    }

    @Test
    public void testOnTearDown() throws Exception {
        EasyMock.expect(mMockDevice.executeShellCommand(EasyMock.eq("getenforce")))
                .andReturn(SELINUX_PERMISSIVE);
        EasyMock.expect(mMockDevice.executeShellCommand(
                                EasyMock.eq("setenforce " + SELINUX_PERMISSIVE)))
                .andReturn("")
                .times(1);
        EasyMock.replay(mMockDevice, mMockBuildInfo);
        mPreparer.setUp(mMockDevice, mMockBuildInfo);
        mPreparer.tearDown(mMockDevice, mMockBuildInfo, null);
        EasyMock.verify(mMockDevice, mMockBuildInfo);
    }

    /**
     * Helper method to create a test file under mTestDir.
     *
     * @param fileName test file name.
     * @param bitness bitness for the test file. Only supports "32" or "64".
     * @return created test file, null for unsupported bitness.
     */
    private File createTestFile(String fileName, String bitness) throws Exception {
        if (bitness == "32") {
            File testFile32 = new File(mTestDir, VTS_LIB_DIR_32 + fileName);
            testFile32.createNewFile();
            return testFile32;
        } else if (bitness == "64") {
            File testFile64 = new File(mTestDir, VTS_LIB_DIR_64 + fileName);
            testFile64.createNewFile();
            return testFile64;
        }
        return null;
    }
}
