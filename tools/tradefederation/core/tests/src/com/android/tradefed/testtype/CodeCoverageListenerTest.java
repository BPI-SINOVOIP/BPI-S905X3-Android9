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

package com.android.tradefed.testtype;

import static org.junit.Assert.fail;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;

import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.util.proto.TfMetricProtoUtil;

import com.google.common.base.VerifyException;
import com.google.protobuf.ByteString;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.mockito.Spy;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.HashMap;
import java.util.Map;

/** Unit tests for {@link CodeCoverageListener}. */
@RunWith(JUnit4.class)
public class CodeCoverageListenerTest {

    private static final String RUN_NAME = "SomeTest";
    private static final int TEST_COUNT = 5;
    private static final long ELAPSED_TIME = 1000;

    private static final String DEVICE_PATH = "/some/path/on/the/device.ec";
    private static final ByteString COVERAGE_MEASUREMENT =
            ByteString.copyFromUtf8("Mi estas kovrado mezurado");

    @Rule public TemporaryFolder folder = new TemporaryFolder();

    @Mock ITestDevice mMockDevice;

    @Spy LogFileReader mFakeListener = new LogFileReader();

    /** Object under test. */
    CodeCoverageListener mCodeCoverageListener;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);

        mCodeCoverageListener = new CodeCoverageListener(mMockDevice, mFakeListener);
    }

    @Test
    public void test() throws DeviceNotAvailableException, IOException {
        // Setup mocks.
        File coverageFile = folder.newFile("coverage.ec");
        try (OutputStream out = new FileOutputStream(coverageFile)) {
            COVERAGE_MEASUREMENT.writeTo(out);
        }
        doReturn(coverageFile).when(mMockDevice).pullFile(DEVICE_PATH);

        // Simulate a test run.
        mCodeCoverageListener.testRunStarted(RUN_NAME, TEST_COUNT);
        Map<String, String> metric = new HashMap<>();
        metric.put("coverageFilePath", DEVICE_PATH);
        mCodeCoverageListener.testRunEnded(ELAPSED_TIME, TfMetricProtoUtil.upgradeConvert(metric));

        // Verify testLog(..) was called with the coverage file.
        verify(mFakeListener)
                .testLog(anyString(), eq(LogDataType.COVERAGE), eq(COVERAGE_MEASUREMENT));
    }

    @Test
    public void testFailure_noCoverageMetric() {
        // Simulate a test run.
        mCodeCoverageListener.testRunStarted(RUN_NAME, TEST_COUNT);
        mCodeCoverageListener.testRunEnded(ELAPSED_TIME, new HashMap<String, Metric>());

        // Verify that the test run is marked as a failure.
        verify(mFakeListener).testRunFailed(anyString());

        // Verify testLog(..) was not called.
        verify(mFakeListener, never())
                .testLog(anyString(), eq(LogDataType.COVERAGE), any(InputStreamSource.class));
    }

    @Test
    public void testFailure_unableToPullFile() throws DeviceNotAvailableException {
        // Setup mocks.
        doReturn(null).when(mMockDevice).pullFile(DEVICE_PATH);

        // Simulate a test run.
        mCodeCoverageListener.testRunStarted(RUN_NAME, TEST_COUNT);

        try {
            Map<String, String> metric = new HashMap<>();
            metric.put("coverageFilePath", DEVICE_PATH);
            mCodeCoverageListener.testRunEnded(
                    ELAPSED_TIME, TfMetricProtoUtil.upgradeConvert(metric));
            fail("Exception not thrown");
        } catch (VerifyException expected) {
        }

        // Verify testLog(..) was not called.
        verify(mFakeListener, never())
                .testLog(anyString(), eq(LogDataType.COVERAGE), any(InputStreamSource.class));
    }

    /** An {@link ITestInvocationListener} which reads test log data streams for verification. */
    private static class LogFileReader implements ITestInvocationListener {
        /**
         * Reads the contents of the {@code dataStream} and forwards it to the {@link
         * #testLog(String, LogDataType, ByteString)} method.
         */
        @Override
        public void testLog(String dataName, LogDataType dataType, InputStreamSource dataStream) {
            try (InputStream input = dataStream.createInputStream()) {
                testLog(dataName, dataType, ByteString.readFrom(input));
            } catch (IOException e) {
                throw new RuntimeException(e);
            }
        }

        /** No-op method for {@link Spy} verification. */
        public void testLog(String dataName, LogDataType dataType, ByteString data) {}
    }
}
