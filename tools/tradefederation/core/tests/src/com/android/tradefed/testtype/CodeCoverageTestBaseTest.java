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

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.AdditionalMatchers.gt;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyCollection;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;

import com.android.ddmlib.IDevice;
import com.android.ddmlib.testrunner.IRemoteAndroidTestRunner;
import com.android.ddmlib.testrunner.ITestRunListener;
import com.android.ddmlib.testrunner.RemoteAndroidTestRunner;
import com.android.tradefed.build.BuildInfo;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.ITestLogger;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.ITestLifeCycleReceiver;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.result.TestRunResult;
import com.android.tradefed.result.ddmlib.TestRunToTestInvocationForwarder;
import com.android.tradefed.util.ICompressionStrategy;
import com.android.tradefed.util.ListInstrumentationParser;
import com.android.tradefed.util.ListInstrumentationParser.InstrumentationTarget;

import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableMap;
import com.google.common.collect.Lists;
import com.google.protobuf.ByteString;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.MockitoAnnotations;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.stubbing.Answer;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/** Unit tests for {@link CodeCoverageTestBase}. */
@RunWith(JUnit4.class)
public class CodeCoverageTestBaseTest {

    private static final long TEST_RUN_TIME = 1000;
    private static final String COVERAGE_PATH = "/data/user/0/%s/files/coverage.ec";

    private static final String PACKAGE_NAME1 = "com.example.foo.test";
    private static final String PACKAGE_NAME2 = "com.example.bar.test";
    private static final String PACKAGE_NAME3 = "com.example.baz.test";

    private static final String RUNNER_NAME1 = "android.support.test.runner.AndroidJUnitRunner";
    private static final String RUNNER_NAME2 = "android.test.InstrumentationTestRunner";
    private static final String RUNNER_NAME3 = "com.example.custom.Runner";

    private static final TestDescription FOO_TEST1 = new TestDescription(".FooTest", "test1");
    private static final TestDescription FOO_TEST2 = new TestDescription(".FooTest", "test2");
    private static final TestDescription FOO_TEST3 = new TestDescription(".FooTest", "test3");
    private static final List<TestDescription> FOO_TESTS =
            ImmutableList.of(FOO_TEST1, FOO_TEST2, FOO_TEST3);

    private static final TestDescription BAR_TEST1 = new TestDescription(".BarTest", "test1");
    private static final TestDescription BAR_TEST2 = new TestDescription(".BarTest", "test2");
    private static final List<TestDescription> BAR_TESTS = ImmutableList.of(BAR_TEST1, BAR_TEST2);

    private static final TestDescription BAZ_TEST1 = new TestDescription(".BazTest", "test1");
    private static final TestDescription BAZ_TEST2 = new TestDescription(".BazTest", "test2");
    private static final List<TestDescription> BAZ_TESTS = ImmutableList.of(BAZ_TEST1, BAZ_TEST2);

    private static final ByteString FAKE_REPORT_CONTENTS =
            ByteString.copyFromUtf8("Mi estas kovrado raporto");

    private static final ByteString FAKE_MEASUREMENT1 =
            ByteString.copyFromUtf8("Mi estas kovrado mezurado");
    private static final ByteString FAKE_MEASUREMENT2 =
            ByteString.copyFromUtf8("Mi estas ankau kovrado mezurado");
    private static final ByteString FAKE_MEASUREMENT3 =
            ByteString.copyFromUtf8("Mi estas ankorau alia priraportado mezurado");

    private static final IBuildInfo BUILD_INFO = new BuildInfo("123456", "device-userdebug");

    @Rule public TemporaryFolder mFolder = new TemporaryFolder();

    // Mocks
    @Mock ITestDevice mDevice;

    @Mock ITestInvocationListener mListener;

    @Mock ListInstrumentationParser mInstrumentationParser;

    // Fake test data
    @Mock TestDataRegistry<List<TestDescription>> mTests;

    @Mock TestDataRegistry<ByteString> mMeasurements;

    interface TestDataRegistry<T> {
        T get(String packageName, String runnerName, int shardIndex, int numShards);
    }

    /** Object under test */
    CodeCoverageTestStub mCoverageTest;

    @Before
    public void setUp() throws DeviceNotAvailableException {
        MockitoAnnotations.initMocks(this);
        doAnswer(CALL_RUNNER)
                .when(mDevice)
                .runInstrumentationTests(
                        any(IRemoteAndroidTestRunner.class), any(ITestLifeCycleReceiver.class));
        mCoverageTest = new CodeCoverageTestStub();
    }

    static enum FakeReportFormat implements CodeCoverageReportFormat {
        CSV(LogDataType.JACOCO_CSV),
        XML(LogDataType.JACOCO_XML),
        HTML(LogDataType.HTML);

        private final LogDataType mLogDataType;

        private FakeReportFormat(LogDataType logDataType) {
            mLogDataType = logDataType;
        }

        @Override
        public LogDataType getLogDataType() { return mLogDataType; }
    }

    /** A subclass of {@link CodeCoverageTest} with certain methods stubbed out for testing. */
    private class CodeCoverageTestStub extends CodeCoverageTestBase<FakeReportFormat> {

        // Captured data
        private ImmutableList.Builder<ByteString> mCapturedMeasurements =
                new ImmutableList.Builder<>();

        public CodeCoverageTestStub() {
            setDevice(mDevice);
        }

        @Override
        public IBuildInfo getBuild() { return BUILD_INFO; }

        @Override
        ICompressionStrategy getCompressionStrategy() {
            return mock(ICompressionStrategy.class);
        }

        ImmutableList<ByteString> getMeasurements() {
            return mCapturedMeasurements.build();
        }

        @Override
        protected File generateCoverageReport(
                Collection<File> measurementFiles, FakeReportFormat format) throws IOException {
            // Capture the measurements for verification later
            for (File measurementFile : measurementFiles) {
                try (FileInputStream inputStream = new FileInputStream(measurementFile)) {
                    mCapturedMeasurements.add(ByteString.readFrom(inputStream));
                }
            }

            // Write the fake report
            File ret = mFolder.newFile();
            FAKE_REPORT_CONTENTS.writeTo(new FileOutputStream(ret));
            return ret;
        }

        @Override
        protected List<FakeReportFormat> getReportFormat() {
            return ImmutableList.of(FakeReportFormat.HTML);
        }

        @Override
        InstrumentationTest internalCreateTest() {
            return new InstrumentationTest() {
                @Override
                IRemoteAndroidTestRunner createRemoteAndroidTestRunner(
                        String packageName, String runnerName, IDevice device) {
                    return new FakeTestRunner(packageName, runnerName);
                }
            };
        }

        @Override
        IRemoteAndroidTestRunner internalCreateTestRunner(String packageName, String runnerName) {
            return new FakeTestRunner(packageName, runnerName);
        }

        @Override
        ListInstrumentationParser internalCreateListInstrumentationParser() {
            return mInstrumentationParser;
        }
    }

    private static final class LogCaptorAnswer implements Answer<Void> {
        private ByteString mValue;

        @Override
        public Void answer(InvocationOnMock invocation) throws Throwable {
            Object[] args = invocation.getArguments();
            InputStream reportStream = ((InputStreamSource) args[2]).createInputStream();
            mValue = ByteString.readFrom(reportStream);
            return null;
        }

        ByteString getValue() {
            return mValue;
        }
    }

    @Test
    public void testRun() throws DeviceNotAvailableException {
        // Prepare some test data
        doReturn(ImmutableList.of(new InstrumentationTarget(PACKAGE_NAME1, RUNNER_NAME1, "")))
                .when(mInstrumentationParser)
                .getInstrumentationTargets();

        // Mocking boilerplate
        doReturn(FOO_TESTS)
                .when(mTests)
                .get(eq(PACKAGE_NAME1), eq(RUNNER_NAME1), anyInt(), anyInt());
        doReturn(FAKE_MEASUREMENT1)
                .when(mMeasurements)
                .get(eq(PACKAGE_NAME1), eq(RUNNER_NAME1), anyInt(), anyInt());

        // Validate the report when it gets log, since the file will get cleaned up later
        LogCaptorAnswer logCaptor = new LogCaptorAnswer();
        doAnswer(logCaptor)
                .when(mListener)
                .testLog(
                        eq("coverage"),
                        eq(FakeReportFormat.HTML.getLogDataType()),
                        any(InputStreamSource.class));

        // Run the test
        mCoverageTest.run(mListener);

        // Verify that the measurements were collected and the report was logged
        assertThat(mCoverageTest.getMeasurements()).containsExactly(FAKE_MEASUREMENT1);
        assertThat(logCaptor.getValue()).isEqualTo(FAKE_REPORT_CONTENTS);
    }

    @Test
    public void testRun_multipleInstrumentationTargets() throws DeviceNotAvailableException {
        // Prepare some test data
        ImmutableList<InstrumentationTarget> targets =
                ImmutableList.of(
                        new InstrumentationTarget(PACKAGE_NAME1, RUNNER_NAME1, ""),
                        new InstrumentationTarget(PACKAGE_NAME2, RUNNER_NAME1, ""),
                        new InstrumentationTarget(PACKAGE_NAME3, RUNNER_NAME1, ""));
        doReturn(targets).when(mInstrumentationParser).getInstrumentationTargets();

        // Mocking boilerplate
        doReturn(FOO_TESTS)
                .when(mTests)
                .get(eq(PACKAGE_NAME1), eq(RUNNER_NAME1), anyInt(), anyInt());
        doReturn(FAKE_MEASUREMENT1)
                .when(mMeasurements)
                .get(eq(PACKAGE_NAME1), eq(RUNNER_NAME1), anyInt(), anyInt());

        doReturn(BAR_TESTS)
                .when(mTests)
                .get(eq(PACKAGE_NAME2), eq(RUNNER_NAME1), anyInt(), anyInt());
        doReturn(FAKE_MEASUREMENT2)
                .when(mMeasurements)
                .get(eq(PACKAGE_NAME2), eq(RUNNER_NAME1), anyInt(), anyInt());

        doReturn(BAZ_TESTS)
                .when(mTests)
                .get(eq(PACKAGE_NAME3), eq(RUNNER_NAME1), anyInt(), anyInt());
        doReturn(FAKE_MEASUREMENT3)
                .when(mMeasurements)
                .get(eq(PACKAGE_NAME3), eq(RUNNER_NAME1), anyInt(), anyInt());

        // Run the test
        mCoverageTest.run(mListener);

        // Verify that all targets were run by checking that we recieved measurements from each
        assertThat(mCoverageTest.getMeasurements())
                .containsExactly(FAKE_MEASUREMENT1, FAKE_MEASUREMENT2, FAKE_MEASUREMENT3);
    }

    @Test
    public void testRun_multipleShards() throws DeviceNotAvailableException {
        // Prepare some test data
        doReturn(ImmutableList.of(new InstrumentationTarget(PACKAGE_NAME1, RUNNER_NAME1, "")))
                .when(mInstrumentationParser)
                .getInstrumentationTargets();
        List<List<TestDescription>> shards = Lists.partition(FOO_TESTS, 1);

        // Indicate that the test should be split into 3 shards
        doReturn(FOO_TESTS).when(mTests).get(PACKAGE_NAME1, RUNNER_NAME1, 0, 1);
        doReturn(FOO_TESTS.subList(0, FOO_TESTS.size() / 2))
                .when(mTests)
                .get(eq(PACKAGE_NAME1), eq(RUNNER_NAME1), eq(0), eq(2));
        mCoverageTest.setMaxTestsPerChunk(1);

        // Return subsets of FOO_TESTS when running shards
        doReturn(shards.get(0)).when(mTests).get(eq(PACKAGE_NAME1), eq(RUNNER_NAME1), eq(0), gt(1));
        doReturn(FAKE_MEASUREMENT1)
                .when(mMeasurements)
                .get(eq(PACKAGE_NAME1), eq(RUNNER_NAME1), eq(0), gt(1));

        doReturn(shards.get(1)).when(mTests).get(eq(PACKAGE_NAME1), eq(RUNNER_NAME1), eq(1), gt(1));
        doReturn(FAKE_MEASUREMENT2)
                .when(mMeasurements)
                .get(eq(PACKAGE_NAME1), eq(RUNNER_NAME1), eq(1), gt(1));

        doReturn(shards.get(2)).when(mTests).get(eq(PACKAGE_NAME1), eq(RUNNER_NAME1), eq(2), gt(1));
        doReturn(FAKE_MEASUREMENT3)
                .when(mMeasurements)
                .get(eq(PACKAGE_NAME1), eq(RUNNER_NAME1), eq(2), gt(1));

        // Run the test
        mCoverageTest.run(mListener);

        // Verify that all shards were run by checking that we recieved measurements from each
        assertThat(mCoverageTest.getMeasurements())
                .containsExactly(FAKE_MEASUREMENT1, FAKE_MEASUREMENT2, FAKE_MEASUREMENT3);
    }

    @Test
    public void testRun_rerunIndividualTests_failedRun() throws DeviceNotAvailableException {
        // Prepare some test data
        InstrumentationTarget target = new InstrumentationTarget(PACKAGE_NAME1, RUNNER_NAME1, "");
        doReturn(ImmutableList.of(target)).when(mInstrumentationParser).getInstrumentationTargets();
        doReturn(FOO_TESTS)
                .when(mTests)
                .get(eq(PACKAGE_NAME1), eq(RUNNER_NAME1), anyInt(), anyInt());

        TestRunResult success = mock(TestRunResult.class);
        doReturn(false).when(success).isRunFailure();
        TestRunResult failure = mock(TestRunResult.class);
        doReturn(true).when(failure).isRunFailure();

        // Mocking boilerplate
        ITestInvocationListener mockListener = mock(ITestInvocationListener.class);
        CodeCoverageTestStub coverageTest = Mockito.spy(new CodeCoverageTestStub());
        doReturn(FOO_TESTS).when(mTests).get(PACKAGE_NAME1, RUNNER_NAME1, 0, 1);

        doReturn(failure).when(coverageTest).runTest(eq(target), eq(0), eq(1),
                any(ITestInvocationListener.class));
        doReturn(success).when(coverageTest).runTest(eq(target), eq(FOO_TEST1),
                any(ITestInvocationListener.class));
        doReturn(failure).when(coverageTest).runTest(eq(target), eq(FOO_TEST2),
                any(ITestInvocationListener.class));
        doReturn(success).when(coverageTest).runTest(eq(target), eq(FOO_TEST3),
                any(ITestInvocationListener.class));

        // Run the test
        coverageTest.run(mockListener);

        // Verify that individual tests are rerun
        verify(coverageTest).runTest(eq(target), eq(0), eq(1), any(ITestInvocationListener.class));
        verify(coverageTest).runTest(eq(target), eq(FOO_TEST1), any(ITestInvocationListener.class));
        verify(coverageTest).runTest(eq(target), eq(FOO_TEST2), any(ITestInvocationListener.class));
        verify(coverageTest).runTest(eq(target), eq(FOO_TEST3), any(ITestInvocationListener.class));
    }

    @Test
    public void testRun_rerunIndividualTests_missingCoverageFile()
            throws DeviceNotAvailableException {
        // Prepare some test data
        InstrumentationTarget target = new InstrumentationTarget(PACKAGE_NAME1, RUNNER_NAME1, "");
        doReturn(ImmutableList.of(target)).when(mInstrumentationParser).getInstrumentationTargets();
        doReturn(FOO_TESTS)
                .when(mTests)
                .get(eq(PACKAGE_NAME1), eq(RUNNER_NAME1), anyInt(), anyInt());

        TestRunResult success = mock(TestRunResult.class);
        doReturn(false).when(success).isRunFailure();

        // Mocking boilerplate
        ITestInvocationListener mockListener = mock(ITestInvocationListener.class);
        CodeCoverageTestStub coverageTest = Mockito.spy(new CodeCoverageTestStub());
        doReturn(FOO_TESTS).when(mTests).get(PACKAGE_NAME1, RUNNER_NAME1, 0, 1);

        doReturn(success)
                .when(coverageTest)
                .runTest(any(InstrumentationTest.class), any(ITestInvocationListener.class));
        ITestDevice mockDevice = coverageTest.getDevice();
        doReturn(false).doReturn(true).when(mockDevice).doesFileExist(anyString());

        // Run the test
        coverageTest.run(mockListener);

        // Verify that individual tests are rerun
        verify(coverageTest).runTest(eq(target), eq(0), eq(1), any(ITestInvocationListener.class));
        verify(coverageTest).runTest(eq(target), eq(FOO_TEST1), any(ITestInvocationListener.class));
        verify(coverageTest).runTest(eq(target), eq(FOO_TEST2), any(ITestInvocationListener.class));
        verify(coverageTest).runTest(eq(target), eq(FOO_TEST3), any(ITestInvocationListener.class));
    }

    @Test
    public void testRun_multipleFormats() throws DeviceNotAvailableException, IOException {
        // Prepare some test data
        InstrumentationTarget target = new InstrumentationTarget(PACKAGE_NAME1, RUNNER_NAME1, "");
        doReturn(ImmutableList.of(target)).when(mInstrumentationParser).getInstrumentationTargets();
        doReturn(FOO_TESTS)
                .when(mTests)
                .get(eq(PACKAGE_NAME1), eq(RUNNER_NAME1), anyInt(), anyInt());
        doReturn(FAKE_MEASUREMENT1)
                .when(mMeasurements)
                .get(eq(PACKAGE_NAME1), eq(RUNNER_NAME1), anyInt(), anyInt());

        File fakeHtmlReport = new File("/some/fake/xml/report/");
        File fakeXmlReport = new File("/some/fake/xml/report.xml");

        // Mocking boilerplate
        ITestInvocationListener mockListener = mock(ITestInvocationListener.class);
        CodeCoverageTestStub coverageTest = Mockito.spy(new CodeCoverageTestStub());
        doReturn(FOO_TESTS).when(mTests).get(PACKAGE_NAME1, RUNNER_NAME1, 0, 1);
        doReturn(ImmutableList.of(FakeReportFormat.XML, FakeReportFormat.HTML))
                .when(coverageTest)
                .getReportFormat();
        doReturn(fakeHtmlReport)
                .when(coverageTest)
                .generateCoverageReport(anyCollection(), eq(FakeReportFormat.HTML));
        doReturn(fakeXmlReport)
                .when(coverageTest)
                .generateCoverageReport(anyCollection(), eq(FakeReportFormat.XML));

        // Run the test
        coverageTest.run(mockListener);

        // Verify that the test was run, and that the reports were logged
        verify(coverageTest)
                .runTest(any(InstrumentationTest.class), any(ITestInvocationListener.class));
        verify(coverageTest).generateCoverageReport(anyCollection(), eq(FakeReportFormat.HTML));
        verify(coverageTest).doLogReport(anyString(), eq(FakeReportFormat.HTML.getLogDataType()),
                eq(fakeHtmlReport), any(ITestLogger.class));
        verify(coverageTest).generateCoverageReport(anyCollection(), eq(FakeReportFormat.XML));
        verify(coverageTest).doLogReport(anyString(), eq(FakeReportFormat.XML.getLogDataType()),
                eq(fakeXmlReport), any(ITestLogger.class));
    }

    @Test
    public void testGetInstrumentationTargets() throws DeviceNotAvailableException {
        // Prepare some test data
        InstrumentationTarget target1 = new InstrumentationTarget(PACKAGE_NAME1, RUNNER_NAME1, "");
        InstrumentationTarget target2 = new InstrumentationTarget(PACKAGE_NAME2, RUNNER_NAME2, "");
        InstrumentationTarget target3 = new InstrumentationTarget(PACKAGE_NAME3, RUNNER_NAME1, "");
        InstrumentationTarget target4 = new InstrumentationTarget(PACKAGE_NAME3, RUNNER_NAME3, "");

        // Set up mocks
        doReturn(ImmutableList.of(target1, target2, target3, target4))
                .when(mInstrumentationParser)
                .getInstrumentationTargets();

        // Get the instrumentation targets
        Collection<InstrumentationTarget> targets = mCoverageTest.getInstrumentationTargets();

        // Verify that all of the instrumentation targets were found
        assertThat(targets).containsExactly(target1, target2, target3, target4);
    }

    @Test
    public void testGetInstrumentationTargets_packageFilterSingleFilterSingleResult()
            throws DeviceNotAvailableException {
        // Prepare some test data
        InstrumentationTarget target1 = new InstrumentationTarget(PACKAGE_NAME1, RUNNER_NAME1, "");
        InstrumentationTarget target2 = new InstrumentationTarget(PACKAGE_NAME2, RUNNER_NAME2, "");
        InstrumentationTarget target3 = new InstrumentationTarget(PACKAGE_NAME3, RUNNER_NAME1, "");
        InstrumentationTarget target4 = new InstrumentationTarget(PACKAGE_NAME3, RUNNER_NAME3, "");

        // Set up mocks
        doReturn(ImmutableList.of(target1, target2, target3, target4))
                .when(mInstrumentationParser)
                .getInstrumentationTargets();
        mCoverageTest.setPackageFilter(ImmutableList.of(PACKAGE_NAME1));

        // Get the instrumentation targets
        Collection<InstrumentationTarget> targets = mCoverageTest.getInstrumentationTargets();

        // Verify that only the PACKAGE_NAME1 target was returned
        assertThat(targets).containsExactly(target1);
    }

    @Test
    public void testGetInstrumentationTargets_packageFilterSingleFilterMultipleResults()
            throws DeviceNotAvailableException {
        // Prepare some test data
        InstrumentationTarget target1 = new InstrumentationTarget(PACKAGE_NAME1, RUNNER_NAME1, "");
        InstrumentationTarget target2 = new InstrumentationTarget(PACKAGE_NAME2, RUNNER_NAME2, "");
        InstrumentationTarget target3 = new InstrumentationTarget(PACKAGE_NAME3, RUNNER_NAME1, "");
        InstrumentationTarget target4 = new InstrumentationTarget(PACKAGE_NAME3, RUNNER_NAME3, "");

        // Set up mocks
        doReturn(ImmutableList.of(target1, target2, target3, target4))
                .when(mInstrumentationParser)
                .getInstrumentationTargets();
        mCoverageTest.setPackageFilter(ImmutableList.of(PACKAGE_NAME3));

        // Get the instrumentation targets
        Collection<InstrumentationTarget> targets = mCoverageTest.getInstrumentationTargets();

        // Verify that both PACKAGE_NAME3 targets were returned
        assertThat(targets).containsExactly(target3, target4);
    }

    @Test
    public void testGetInstrumentationTargets_packageFilterMultipleFilters()
            throws DeviceNotAvailableException {
        // Prepare some test data
        InstrumentationTarget target1 = new InstrumentationTarget(PACKAGE_NAME1, RUNNER_NAME1, "");
        InstrumentationTarget target2 = new InstrumentationTarget(PACKAGE_NAME2, RUNNER_NAME2, "");
        InstrumentationTarget target3 = new InstrumentationTarget(PACKAGE_NAME3, RUNNER_NAME1, "");
        InstrumentationTarget target4 = new InstrumentationTarget(PACKAGE_NAME3, RUNNER_NAME3, "");

        // Set up mocks
        doReturn(ImmutableList.of(target1, target2, target3, target4))
                .when(mInstrumentationParser)
                .getInstrumentationTargets();
        mCoverageTest.setPackageFilter(ImmutableList.of(PACKAGE_NAME1, PACKAGE_NAME2));

        // Get the instrumentation targets
        Collection<InstrumentationTarget> targets = mCoverageTest.getInstrumentationTargets();

        // Verify that the PACKAGE_NAME1 and PACKAGE_NAME2 targets were returned
        assertThat(targets).containsExactly(target1, target2);
    }

    @Test
    public void testGetInstrumentationTargets_runnerFilterSingleFilterSingleResult()
            throws DeviceNotAvailableException {
        // Prepare some test data
        InstrumentationTarget target1 = new InstrumentationTarget(PACKAGE_NAME1, RUNNER_NAME1, "");
        InstrumentationTarget target2 = new InstrumentationTarget(PACKAGE_NAME2, RUNNER_NAME2, "");
        InstrumentationTarget target3 = new InstrumentationTarget(PACKAGE_NAME3, RUNNER_NAME1, "");
        InstrumentationTarget target4 = new InstrumentationTarget(PACKAGE_NAME3, RUNNER_NAME3, "");

        // Set up mocks
        doReturn(ImmutableList.of(target1, target2, target3, target4))
                .when(mInstrumentationParser)
                .getInstrumentationTargets();
        mCoverageTest.setRunnerFilter(ImmutableList.of(RUNNER_NAME2));

        // Get the instrumentation targets
        Collection<InstrumentationTarget> targets = mCoverageTest.getInstrumentationTargets();

        // Verify that only the RUNNER_NAME2 target was returned
        assertThat(targets).containsExactly(target2);
    }

    @Test
    public void testGetInstrumentationTargets_runnerFilterSingleFilterMultipleResults()
            throws DeviceNotAvailableException {
        // Prepare some test data
        InstrumentationTarget target1 = new InstrumentationTarget(PACKAGE_NAME1, RUNNER_NAME1, "");
        InstrumentationTarget target2 = new InstrumentationTarget(PACKAGE_NAME2, RUNNER_NAME2, "");
        InstrumentationTarget target3 = new InstrumentationTarget(PACKAGE_NAME3, RUNNER_NAME1, "");
        InstrumentationTarget target4 = new InstrumentationTarget(PACKAGE_NAME3, RUNNER_NAME3, "");

        // Set up mocks
        doReturn(ImmutableList.of(target1, target2, target3, target4))
                .when(mInstrumentationParser)
                .getInstrumentationTargets();
        mCoverageTest.setRunnerFilter(ImmutableList.of(RUNNER_NAME1));

        // Get the instrumentation targets
        Collection<InstrumentationTarget> targets = mCoverageTest.getInstrumentationTargets();

        // Verify that both RUNNER_NAME1 targets were returned
        assertThat(targets).containsExactly(target1, target3);
    }

    @Test
    public void testGetInstrumentationTargets_runnerFilterMultipleFilters()
            throws DeviceNotAvailableException {
        // Prepare some test data
        InstrumentationTarget target1 = new InstrumentationTarget(PACKAGE_NAME1, RUNNER_NAME1, "");
        InstrumentationTarget target2 = new InstrumentationTarget(PACKAGE_NAME2, RUNNER_NAME2, "");
        InstrumentationTarget target3 = new InstrumentationTarget(PACKAGE_NAME3, RUNNER_NAME1, "");
        InstrumentationTarget target4 = new InstrumentationTarget(PACKAGE_NAME3, RUNNER_NAME3, "");

        // Set up mocks
        doReturn(ImmutableList.of(target1, target2, target3, target4))
                .when(mInstrumentationParser)
                .getInstrumentationTargets();
        mCoverageTest.setRunnerFilter(ImmutableList.of(RUNNER_NAME2, RUNNER_NAME3));

        // Get the instrumentation targets
        Collection<InstrumentationTarget> targets = mCoverageTest.getInstrumentationTargets();

        // Verify that the RUNNER_NAME2 and RUNNER_NAME3 targets were returned
        assertThat(targets).containsExactly(target2, target4);
    }

    @Test
    public void testDoesRunnerSupportSharding_true() throws DeviceNotAvailableException {
        // Prepare some test data
        InstrumentationTarget target = new InstrumentationTarget(PACKAGE_NAME1, RUNNER_NAME1, "");

        // Set up mocks. Return fewer tests when sharding is enabled.
        doReturn(ImmutableList.of(target)).when(mInstrumentationParser).getInstrumentationTargets();
        doReturn(FOO_TESTS).when(mTests).get(eq(PACKAGE_NAME1), eq(RUNNER_NAME1), eq(0), eq(1));
        doReturn(Lists.partition(FOO_TESTS, 2).get(0))
                .when(mTests)
                .get(eq(PACKAGE_NAME1), eq(RUNNER_NAME1), eq(0), gt(1));

        // Verify that the method returns true
        assertThat(mCoverageTest.doesRunnerSupportSharding(target)).isTrue();
    }

    @Test
    public void testDoesRunnerSupportSharding_false() throws DeviceNotAvailableException {
        // Prepare some test data
        InstrumentationTarget target = new InstrumentationTarget(PACKAGE_NAME1, RUNNER_NAME1, "");

        // Set up mocks. Return the same number of tests for any number of shards.
        doReturn(ImmutableList.of(target)).when(mInstrumentationParser).getInstrumentationTargets();
        doReturn(FOO_TESTS)
                .when(mTests)
                .get(eq(PACKAGE_NAME1), eq(RUNNER_NAME1), anyInt(), anyInt());

        // Verify that the method returns false
        assertThat(mCoverageTest.doesRunnerSupportSharding(target)).isFalse();
    }

    @Test
    public void testGetNumberOfShards() throws DeviceNotAvailableException {
        // Prepare some test data
        InstrumentationTarget target = new InstrumentationTarget(PACKAGE_NAME1, RUNNER_NAME1, "");
        List<TestDescription> tests = new ArrayList<>();
        for (int i = 0; i < 10; i++) {
            tests.add(new TestDescription(PACKAGE_NAME1, String.format("test%d", i)));
        }

        // Set up mocks
        doReturn(tests).when(mTests).get(eq(PACKAGE_NAME1), eq(RUNNER_NAME1), anyInt(), anyInt());
        mCoverageTest.setMaxTestsPerChunk(1);

        // Verify that each test will run in a separate shard
        assertThat(mCoverageTest.getNumberOfShards(target)).isEqualTo(tests.size());
    }

    @Test
    public void testGetNumberOfShards_allTestsInSingleShard() throws DeviceNotAvailableException {
        // Prepare some test data
        InstrumentationTarget target = new InstrumentationTarget(PACKAGE_NAME1, RUNNER_NAME1, "");
        List<TestDescription> tests = new ArrayList<>();
        for (int i = 0; i < 10; i++) {
            tests.add(new TestDescription(PACKAGE_NAME1, String.format("test%d", i)));
        }

        // Set up mocks
        doReturn(tests).when(mTests).get(eq(PACKAGE_NAME1), eq(RUNNER_NAME1), anyInt(), anyInt());
        mCoverageTest.setMaxTestsPerChunk(10);

        // Verify that all tests will run in a single shard
        assertThat(mCoverageTest.getNumberOfShards(target)).isEqualTo(1);
    }

    @Test
    public void testCollectTests() throws DeviceNotAvailableException {
        // Prepare some test data
        InstrumentationTarget target = new InstrumentationTarget(PACKAGE_NAME1, RUNNER_NAME1, "");
        List<TestDescription> tests = new ArrayList<>();
        for (int i = 0; i < 10; i++) {
            tests.add(new TestDescription(PACKAGE_NAME1, String.format("test%d", i)));
        }

        // Set up mocks
        doReturn(tests).when(mTests).get(eq(PACKAGE_NAME1), eq(RUNNER_NAME1), anyInt(), anyInt());

        // Collect the tests
        Collection<TestDescription> collectedTests = mCoverageTest.collectTests(target, 0, 1);

        // Verify that all of the tests were returned
        assertThat(collectedTests).containsExactlyElementsIn(tests);
    }

    @Test
    public void testCollectTests_withShards() throws DeviceNotAvailableException {
        // Prepare some test data
        InstrumentationTarget target = new InstrumentationTarget(PACKAGE_NAME1, RUNNER_NAME1, "");
        int numShards = 3;
        List<TestDescription> tests = new ArrayList<>();
        for (int i = 0; i < 10; i++) {
            tests.add(new TestDescription(PACKAGE_NAME1, String.format("test%d", i)));
        }

        // Set up mocks
        mCoverageTest.setMaxTestsPerChunk((int) Math.ceil((double) tests.size() / numShards));
        doReturn(tests).when(mTests).get(PACKAGE_NAME1, RUNNER_NAME1, 0, 1);
        doReturn(tests.subList(0, tests.size() / 2))
                .when(mTests)
                .get(eq(PACKAGE_NAME1), eq(RUNNER_NAME1), eq(0), eq(2));

        List<List<TestDescription>> shards =
                Lists.partition(tests, (int) Math.ceil((double) tests.size() / numShards));
        int currentIndex = 0;
        for (List<TestDescription> shard : shards) {
            doReturn(shard)
                    .when(mTests)
                    .get(PACKAGE_NAME1, RUNNER_NAME1, currentIndex, shards.size());
            currentIndex++;
        }

        // Collect the tests in shards
        ArrayList<TestDescription> allCollectedTests = new ArrayList<>();
        for (int shardIndex = 0; shardIndex < numShards; shardIndex++) {
            // Verify that each shard contains some tests
            Collection<TestDescription> collectedTests =
                    mCoverageTest.collectTests(target, shardIndex, numShards);
            assertThat(collectedTests).containsExactlyElementsIn(shards.get(shardIndex));

            allCollectedTests.addAll(collectedTests);
        }
        // Verify that all of the tests were returned in the end
        assertThat(allCollectedTests).containsExactlyElementsIn(tests);
    }

    @Test
    public void testCreateTestRunner() {
        // Prepare some test data
        InstrumentationTarget target = new InstrumentationTarget(PACKAGE_NAME1, RUNNER_NAME1, "");

        // Create a test runner
        IRemoteAndroidTestRunner runner = mCoverageTest.createTestRunner(target, 0, 1);

        // Verify that the runner has the correct values
        assertThat(runner.getPackageName()).isEqualTo(PACKAGE_NAME1);
        assertThat(runner.getRunnerName()).isEqualTo(RUNNER_NAME1);
    }

    @Test
    public void testCreateTestRunner_withArgs() {
        // Prepare some test data
        InstrumentationTarget target = new InstrumentationTarget(PACKAGE_NAME1, RUNNER_NAME1, "");
        Map<String, String> args = ImmutableMap.of("arg1", "value1", "arg2", "value2");

        // Set up mocks
        mCoverageTest.setInstrumentationArgs(args);

        // Create a test runner
        FakeTestRunner runner = (FakeTestRunner) mCoverageTest.createTestRunner(target, 0, 1);

        // Verify that the addInstrumentationArg(..) method was called with each argument
        assertThat(runner.getArgs()).containsExactlyEntriesIn(args);
    }

    @Test
    public void testCreateTestRunner_withShards() {
        // Prepare some test data
        InstrumentationTarget target = new InstrumentationTarget(PACKAGE_NAME1, RUNNER_NAME1, "");
        int shardIndex = 3;
        int numShards = 5;

        // Create a test runner
        FakeTestRunner runner =
                (FakeTestRunner) mCoverageTest.createTestRunner(target, shardIndex, numShards);

        // Verify that the addInstrumentationArg(..) method was called to configure the shards
        assertThat(runner.getArgs()).containsEntry("shardIndex", Integer.toString(shardIndex));
        assertThat(runner.getArgs()).containsEntry("numShards", Integer.toString(numShards));
    }

    @Test
    public void testCreateCoverageTest() {
        // Prepare some test data
        InstrumentationTarget target = new InstrumentationTarget(PACKAGE_NAME1, RUNNER_NAME1, "");

        // Create a CodeCoverageTest instance
        InstrumentationTest test = mCoverageTest.createTest(target);

        // Verify that the test has the correct values
        assertThat(test.getPackageName()).isEqualTo(PACKAGE_NAME1);
        assertThat(test.getRunnerName()).isEqualTo(RUNNER_NAME1);
        assertThat(test.getInstrumentationArg("coverage")).isEqualTo("true");
    }

    @Test
    public void testCreateCoverageTest_withArgs() {
        // Prepare some test data
        InstrumentationTarget target = new InstrumentationTarget(PACKAGE_NAME1, RUNNER_NAME1, "");
        Map<String, String> args = ImmutableMap.of("arg1", "value1", "arg2", "value2");

        // Set up mocks
        mCoverageTest.setInstrumentationArgs(args);

        // Create a CodeCoverageTest instance
        InstrumentationTest test = mCoverageTest.createTest(target, 0, 3);

        // Verify that the test has the correct values
        for (Map.Entry<String, String> entry : args.entrySet()) {
            assertThat(test.getInstrumentationArg(entry.getKey())).isEqualTo(entry.getValue());
        }
    }

    @Test
    public void testCreateCoverageTest_withShards() {
        // Prepare some test data
        InstrumentationTarget target = new InstrumentationTarget(PACKAGE_NAME1, RUNNER_NAME1, "");
        int shardIndex = 3;
        int numShards = 5;

        // Create a CodeCoverageTest instance
        InstrumentationTest test = mCoverageTest.createTest(target, shardIndex, numShards);

        // Verify that the addInstrumentationArg(..) method was called to configure the shards
        assertThat(test.getInstrumentationArg("shardIndex"))
                .isEqualTo(Integer.toString(shardIndex));
        assertThat(test.getInstrumentationArg("numShards")).isEqualTo(Integer.toString(numShards));
        assertThat(test.getInstrumentationArg("coverage")).isEqualTo("true");
    }

    private static final Answer<Void> CALL_RUNNER =
            invocation -> {
                Object[] args = invocation.getArguments();
                ((IRemoteAndroidTestRunner) args[0])
                        .run(
                                new TestRunToTestInvocationForwarder(
                                        (ITestLifeCycleReceiver) args[1]));
                return null;
            };

    /**
     * A fake {@link IRemoteAndroidTestRunner} which simulates a test run by notifying the {@link
     * ITestRunListener}s but does not actually run anything.
     */
    private class FakeTestRunner extends RemoteAndroidTestRunner {
        private Map<String, String> mArgs = new HashMap<>();

        FakeTestRunner(String packageName, String runnerName) {
            super(packageName, runnerName, null);
        }

        @Override
        public void addInstrumentationArg(String name, String value) {
            super.addInstrumentationArg(name, value);
            mArgs.put(name, value);
        }

        Map<String, String> getArgs() {
            return mArgs;
        }

        @Override
        public void run(Collection<ITestRunListener> listeners) {
            int shardIndex = Integer.parseInt(getArgs().getOrDefault("shardIndex", "0"));
            int numShards = Integer.parseInt(getArgs().getOrDefault("numShards", "1"));
            List<TestDescription> tests =
                    mTests.get(getPackageName(), getRunnerName(), shardIndex, numShards);

            // Start the test run
            listeners.stream().forEach(l -> l.testRunStarted(getPackageName(), tests.size()));

            // Run each of the tests
            for (TestDescription test : tests) {
                listeners
                        .stream()
                        .forEach(l -> l.testStarted(TestDescription.convertToIdentifier(test)));
                listeners
                        .stream()
                        .forEach(
                                l ->
                                        l.testEnded(
                                                TestDescription.convertToIdentifier(test),
                                                ImmutableMap.of()));
            }

            // Mock out the coverage measurement if necessary
            Map<String, String> metrics = new HashMap<>();
            if (getArgs().getOrDefault("coverage", "false").equals("true")) {
                String devicePath = String.format(COVERAGE_PATH, getPackageName());
                ByteString measurement =
                        mMeasurements.get(getPackageName(), getRunnerName(), shardIndex, numShards);
                mockDeviceFile(devicePath, measurement);
                metrics.put(CodeCoverageTest.COVERAGE_REMOTE_FILE_LABEL, devicePath);
            }

            // End the test run
            listeners.stream().forEach(l -> l.testRunEnded(TEST_RUN_TIME, metrics));
        }
    }

    private void mockDeviceFile(String devicePath, ByteString contents) {
        Answer<File> pullFile =
                unused -> {
                    File ret = mFolder.newFile();
                    contents.writeTo(new FileOutputStream(ret));
                    return ret;
                };
        try {
            doReturn(true).when(mDevice).doesFileExist(devicePath);
            doAnswer(pullFile).when(mDevice).pullFile(devicePath);
        } catch (DeviceNotAvailableException impossible) {
            // Mocks won't actually throw.
            throw new AssertionError(impossible);
        }
    }
}
