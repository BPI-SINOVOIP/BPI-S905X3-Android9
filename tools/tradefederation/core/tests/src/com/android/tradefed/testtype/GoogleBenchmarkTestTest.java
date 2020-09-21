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

import com.android.tradefed.device.CollectingOutputReceiver;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.device.MockFileUtil;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ITestInvocationListener;

import junit.framework.TestCase;

import org.easymock.EasyMock;

import java.util.Collections;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.TimeUnit;

/**
 * Unit tests for {@link GoogleBenchmarkTest}.
 */
public class GoogleBenchmarkTestTest extends TestCase {

    private ITestInvocationListener mMockInvocationListener = null;
    private CollectingOutputReceiver mMockReceiver = null;
    private ITestDevice mMockITestDevice = null;
    private GoogleBenchmarkTest mGoogleBenchmarkTest;

    /**
     * Helper to initialize the various EasyMocks we'll need.
     */
    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mMockInvocationListener = EasyMock.createMock(ITestInvocationListener.class);
        mMockReceiver = new CollectingOutputReceiver();
        mMockITestDevice = EasyMock.createMock(ITestDevice.class);
        EasyMock.expect(mMockITestDevice.getSerialNumber()).andStubReturn("serial");
        mGoogleBenchmarkTest = new GoogleBenchmarkTest() {
            @Override
            CollectingOutputReceiver createOutputCollector() {
                return mMockReceiver;
            }
            @Override
            GoogleBenchmarkResultParser createResultParser(String runName,
                    ITestInvocationListener listener) {
                return new GoogleBenchmarkResultParser(runName, listener) {
                    @Override
                    public Map<String, String> parse(CollectingOutputReceiver output) {
                        return Collections.emptyMap();
                    }
                };
            }
        };
        mGoogleBenchmarkTest.setDevice(mMockITestDevice);
    }

    /**
     * Helper that replays all mocks.
     */
    private void replayMocks() {
      EasyMock.replay(mMockInvocationListener, mMockITestDevice);
    }

    /**
     * Helper that verifies all mocks.
     */
    private void verifyMocks() {
      EasyMock.verify(mMockInvocationListener, mMockITestDevice);
    }

    /**
     * Test the run method for a couple tests
     */
    public void testRun() throws DeviceNotAvailableException {
        final String nativeTestPath = GoogleBenchmarkTest.DEFAULT_TEST_PATH;
        final String test1 = "test1";
        final String test2 = "test2";
        MockFileUtil.setMockDirContents(mMockITestDevice, nativeTestPath, test1, test2);
        EasyMock.expect(mMockITestDevice.doesFileExist(nativeTestPath)).andReturn(true);
        EasyMock.expect(mMockITestDevice.isDirectory(nativeTestPath)).andReturn(true);
        EasyMock.expect(mMockITestDevice.isDirectory(nativeTestPath + "/test1")).andReturn(false);
        EasyMock.expect(mMockITestDevice.isDirectory(nativeTestPath + "/test2")).andReturn(false);
        String[] files = new String[] {"test1", "test2"};
        EasyMock.expect(mMockITestDevice.getChildren(nativeTestPath)).andReturn(files);
        EasyMock.expect(mMockITestDevice.executeShellCommand(EasyMock.contains("chmod")))
                .andReturn("").times(2);
        mMockITestDevice.executeShellCommand(EasyMock.contains(test1), EasyMock.same(mMockReceiver),
                EasyMock.anyLong(), (TimeUnit)EasyMock.anyObject(), EasyMock.anyInt());
        mMockITestDevice.executeShellCommand(EasyMock.contains(test2), EasyMock.same(mMockReceiver),
                EasyMock.anyLong(), (TimeUnit)EasyMock.anyObject(), EasyMock.anyInt());
        EasyMock.expect(
                        mMockITestDevice.executeShellCommand(
                                String.format("file %s/test1", nativeTestPath)))
                .andReturn("ELF whatever, BuildID=blabla\n");
        EasyMock.expect(
                        mMockITestDevice.executeShellCommand(
                                String.format(
                                        "%s/test1 --benchmark_list_tests=true", nativeTestPath)))
                .andReturn("method1\nmethod2\nmethod3");
        EasyMock.expect(
                        mMockITestDevice.executeShellCommand(
                                String.format("file %s/test2", nativeTestPath)))
                .andReturn("ELF whatever, BuildID=blabla\n");
        EasyMock.expect(
                        mMockITestDevice.executeShellCommand(
                                String.format(
                                        "%s/test2 --benchmark_list_tests=true", nativeTestPath)))
                .andReturn("method1\nmethod2\n");
        mMockInvocationListener.testRunStarted(test1, 3);
        mMockInvocationListener.testRunStarted(test2, 2);
        mMockInvocationListener.testRunEnded(
                EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.expectLastCall().times(2);
        replayMocks();

        mGoogleBenchmarkTest.run(mMockInvocationListener);
        verifyMocks();
    }

    /**
     * Test the run method when no device is set.
     */
    public void testRun_noDevice() throws DeviceNotAvailableException {
        mGoogleBenchmarkTest.setDevice(null);
        try {
            mGoogleBenchmarkTest.run(mMockInvocationListener);
        } catch (IllegalArgumentException e) {
            assertEquals("Device has not been set", e.getMessage());
            return;
        }
        fail();
    }

    /**
     * Test the run method for a couple tests
     */
    public void testRun_noBenchmarkDir() throws DeviceNotAvailableException {
        EasyMock.expect(mMockITestDevice.doesFileExist(GoogleBenchmarkTest.DEFAULT_TEST_PATH))
                .andReturn(false);
        replayMocks();
        try {
            mGoogleBenchmarkTest.run(mMockInvocationListener);
            fail("Should have thrown an exception.");
        } catch (RuntimeException e) {
            // expected
        }
        verifyMocks();
    }

    /**
     * Test the run method for a couple tests with a module name
     */
    public void testRun_withModuleName() throws DeviceNotAvailableException {
        final String moduleName = "module";
        final String nativeTestPath =
                String.format("%s/%s", GoogleBenchmarkTest.DEFAULT_TEST_PATH, moduleName);
        mGoogleBenchmarkTest.setModuleName(moduleName);
        final String test1 = "test1";
        final String test2 = "test2";
        MockFileUtil.setMockDirContents(mMockITestDevice, nativeTestPath, test1, test2);
        EasyMock.expect(mMockITestDevice.doesFileExist(nativeTestPath)).andReturn(true);
        EasyMock.expect(mMockITestDevice.isDirectory(nativeTestPath)).andReturn(true);
        EasyMock.expect(mMockITestDevice.isDirectory(nativeTestPath + "/test1")).andReturn(false);
        EasyMock.expect(mMockITestDevice.isDirectory(nativeTestPath + "/test2")).andReturn(false);
        String[] files = new String[] {"test1", "test2"};
        EasyMock.expect(mMockITestDevice.getChildren(nativeTestPath)).andReturn(files);
        EasyMock.expect(mMockITestDevice.executeShellCommand(EasyMock.contains("chmod")))
                .andReturn("").times(2);
        mMockITestDevice.executeShellCommand(EasyMock.contains(test1), EasyMock.same(mMockReceiver),
                EasyMock.anyLong(), (TimeUnit)EasyMock.anyObject(), EasyMock.anyInt());
        mMockITestDevice.executeShellCommand(EasyMock.contains(test2), EasyMock.same(mMockReceiver),
                EasyMock.anyLong(), (TimeUnit)EasyMock.anyObject(), EasyMock.anyInt());
        EasyMock.expect(
                        mMockITestDevice.executeShellCommand(
                                String.format("file %s/test1", nativeTestPath)))
                .andReturn("ELF whatever, BuildID=blabla\n");
        EasyMock.expect(
                        mMockITestDevice.executeShellCommand(
                                String.format(
                                        "%s/test1 --benchmark_list_tests=true", nativeTestPath)))
                .andReturn("\nmethod1\nmethod2\nmethod3\n\n");
        EasyMock.expect(
                        mMockITestDevice.executeShellCommand(
                                String.format("file %s/test2", nativeTestPath)))
                .andReturn("ELF whatever, BuildID=blabla\n");
        EasyMock.expect(
                        mMockITestDevice.executeShellCommand(
                                String.format(
                                        "%s/test2 --benchmark_list_tests=true", nativeTestPath)))
                .andReturn("method1\nmethod2\n");
        mMockInvocationListener.testRunStarted(test1, 3);
        mMockInvocationListener.testRunStarted(test2, 2);
        mMockInvocationListener.testRunEnded(
                EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.expectLastCall().times(2);
        replayMocks();

        mGoogleBenchmarkTest.run(mMockInvocationListener);
        verifyMocks();
    }

    /**
     * Test the run method for a couple tests with a module name
     */
    public void testRun_withRunReportName() throws DeviceNotAvailableException {
        final String nativeTestPath = GoogleBenchmarkTest.DEFAULT_TEST_PATH;
        final String test1 = "test1";
        final String reportName = "reportName";
        mGoogleBenchmarkTest.setReportRunName(reportName);
        MockFileUtil.setMockDirContents(mMockITestDevice, nativeTestPath, test1);
        EasyMock.expect(mMockITestDevice.doesFileExist(nativeTestPath)).andReturn(true);
        EasyMock.expect(mMockITestDevice.isDirectory(nativeTestPath)).andReturn(true);
        EasyMock.expect(mMockITestDevice.isDirectory(nativeTestPath + "/test1")).andReturn(false);
        String[] files = new String[] {"test1"};
        EasyMock.expect(mMockITestDevice.getChildren(nativeTestPath)).andReturn(files);
        EasyMock.expect(mMockITestDevice.executeShellCommand(EasyMock.contains("chmod")))
                .andReturn("");
        mMockITestDevice.executeShellCommand(EasyMock.contains(test1), EasyMock.same(mMockReceiver),
                EasyMock.anyLong(), (TimeUnit)EasyMock.anyObject(), EasyMock.anyInt());
        EasyMock.expect(
                        mMockITestDevice.executeShellCommand(
                                String.format("file %s/test1", nativeTestPath)))
                .andReturn("ELF whatever, BuildID=blabla\n");
        EasyMock.expect(
                        mMockITestDevice.executeShellCommand(
                                String.format(
                                        "%s/test1 --benchmark_list_tests=true", nativeTestPath)))
                .andReturn("method1\nmethod2\nmethod3");
        // Expect reportName instead of test name
        mMockInvocationListener.testRunStarted(reportName, 3);
        mMockInvocationListener.testRunEnded(
                EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.expectLastCall();
        replayMocks();

        mGoogleBenchmarkTest.run(mMockInvocationListener);
        verifyMocks();
    }

    /**
     * Test the run method when exec shell throw exeception.
     */
    public void testRun_exceptionDuringExecShell() throws DeviceNotAvailableException {
        final String nativeTestPath = GoogleBenchmarkTest.DEFAULT_TEST_PATH;
        final String test1 = "test1";
        MockFileUtil.setMockDirContents(mMockITestDevice, nativeTestPath, test1);
        EasyMock.expect(mMockITestDevice.doesFileExist(nativeTestPath)).andReturn(true);
        EasyMock.expect(mMockITestDevice.isDirectory(nativeTestPath)).andReturn(true);
        EasyMock.expect(mMockITestDevice.isDirectory(nativeTestPath + "/test1")).andReturn(false);
        String[] files = new String[] {"test1"};
        EasyMock.expect(mMockITestDevice.getChildren(nativeTestPath)).andReturn(files);
        EasyMock.expect(mMockITestDevice.executeShellCommand(EasyMock.contains("chmod")))
                .andReturn("");
        mMockITestDevice.executeShellCommand(EasyMock.contains(test1), EasyMock.same(mMockReceiver),
                EasyMock.anyLong(), (TimeUnit)EasyMock.anyObject(), EasyMock.anyInt());
        EasyMock.expectLastCall().andThrow(new DeviceNotAvailableException("dnae", "serial"));
        EasyMock.expect(
                        mMockITestDevice.executeShellCommand(
                                String.format("file %s/test1", nativeTestPath)))
                .andReturn("ELF whatever, BuildID=blabla\n");
        EasyMock.expect(
                        mMockITestDevice.executeShellCommand(
                                String.format(
                                        "%s/test1 --benchmark_list_tests=true", nativeTestPath)))
                .andReturn("method1\nmethod2\nmethod3");
        mMockInvocationListener.testRunStarted(test1, 3);
        mMockInvocationListener.testRunFailed(EasyMock.anyObject());
        // Even with exception testrunEnded is expected.
        mMockInvocationListener.testRunEnded(
                EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.expectLastCall();
        replayMocks();

        try {
            mGoogleBenchmarkTest.run(mMockInvocationListener);
            fail();
        } catch (DeviceNotAvailableException e) {
            // expected
        }
        verifyMocks();
    }

    /**
     * File exclusion regex filter should skip matched filepaths.
     */
    public void testFileExclusionRegexFilter_skipMatched() {
        // Skip files ending in .txt
        mGoogleBenchmarkTest.addFileExclusionFilterRegex(".*\\.txt");
        assertFalse(mGoogleBenchmarkTest.shouldSkipFile("/some/path/file/binary"));
        assertFalse(mGoogleBenchmarkTest.shouldSkipFile("/some/path/file/random.dat"));
        assertTrue(mGoogleBenchmarkTest.shouldSkipFile("/some/path/file/test.txt"));
    }

    /**
     * File exclusion regex filter for multi filters.
     */
    public void testFileExclusionRegexFilter_skipMultiMatched() {
        // Skip files ending in .txt
        mGoogleBenchmarkTest.addFileExclusionFilterRegex(".*\\.txt");
        // Also skip files ending in .dat
        mGoogleBenchmarkTest.addFileExclusionFilterRegex(".*\\.dat");
        assertFalse(mGoogleBenchmarkTest.shouldSkipFile("/some/path/file/binary"));
        assertTrue(mGoogleBenchmarkTest.shouldSkipFile("/some/path/file/random.dat"));
        assertTrue(mGoogleBenchmarkTest.shouldSkipFile("/some/path/file/test.txt"));
    }
}
