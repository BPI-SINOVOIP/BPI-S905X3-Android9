/*
 * Copyright (C) 2010 The Android Open Source Project
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

import static org.junit.Assert.fail;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyCollectionOf;
import static org.mockito.ArgumentMatchers.anyLong;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import com.android.ddmlib.IDevice;
import com.android.ddmlib.testrunner.IRemoteAndroidTestRunner;
import com.android.ddmlib.testrunner.InstrumentationResultParser;
import com.android.ddmlib.testrunner.RemoteAndroidTestRunner;
import com.android.tradefed.config.ConfigurationException;
import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.ITestLifeCycleReceiver;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.util.ListInstrumentationParser;
import com.android.tradefed.util.ListInstrumentationParser.InstrumentationTarget;

import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableMap;

import org.easymock.IAnswer;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.mockito.ArgumentCaptor;
import org.mockito.Captor;
import org.mockito.InOrder;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.MockitoAnnotations;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.stubbing.Answer;

import java.util.Collection;
import java.util.HashMap;
import java.util.Map;

/** Unit tests for {@link InstrumentationTest} */
@RunWith(JUnit4.class)
public class InstrumentationTestTest {

    private static final String TEST_PACKAGE_VALUE = "com.foo";
    private static final String TEST_RUNNER_VALUE = ".FooRunner";
    private static final TestDescription TEST1 = new TestDescription("Test", "test1");
    private static final TestDescription TEST2 = new TestDescription("Test", "test2");
    private static final String RUN_ERROR_MSG = "error";
    private static final HashMap<String, Metric> EMPTY_STRING_MAP = new HashMap<>();

    /** The {@link InstrumentationTest} under test, with all dependencies mocked out */
    private InstrumentationTest mInstrumentationTest;

    // The mock objects.
    @Mock IDevice mMockIDevice;
    @Mock ITestDevice mMockTestDevice;
    @Mock ITestInvocationListener mMockListener;
    @Mock ListInstrumentationParser mMockListInstrumentationParser;

    @Captor private ArgumentCaptor<Collection<TestDescription>> testCaptor;

    /**
     * Helper class for providing an {@link IAnswer} to a {@link
     * ITestDevice#runInstrumentationTests(IRemoteAndroidTestRunner, ITestInvocationListener...)}
     * call.
     */
    @FunctionalInterface
    interface RunInstrumentationTestsAnswer extends Answer<Boolean> {
        @Override
        default Boolean answer(InvocationOnMock invocation) throws Exception {
            Object[] args = invocation.getArguments();
            return answer((IRemoteAndroidTestRunner) args[0], (ITestLifeCycleReceiver) args[1]);
        }

        Boolean answer(IRemoteAndroidTestRunner runner, ITestLifeCycleReceiver listener)
                throws Exception;
    }

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);

        doReturn(mMockIDevice).when(mMockTestDevice).getIDevice();
        doReturn("serial").when(mMockTestDevice).getSerialNumber();

        InstrumentationTarget target1 =
                new InstrumentationTarget(TEST_PACKAGE_VALUE, "runner1", "target1");
        InstrumentationTarget target2 = new InstrumentationTarget("package2", "runner2", "target2");
        doReturn(ImmutableList.of(target1, target2))
                .when(mMockListInstrumentationParser)
                .getInstrumentationTargets();

        mInstrumentationTest = Mockito.spy(new InstrumentationTest());
        mInstrumentationTest.setPackageName(TEST_PACKAGE_VALUE);
        mInstrumentationTest.setRunnerName(TEST_RUNNER_VALUE);
        mInstrumentationTest.setDevice(mMockTestDevice);
        mInstrumentationTest.setListInstrumentationParser(mMockListInstrumentationParser);
    }

    /** Test normal run scenario. */
    @Test
    public void testRun() throws DeviceNotAvailableException {
        // verify the mock listener is passed through to the runner
        RunInstrumentationTestsAnswer runTests =
                (runner, listener) -> {
                    // perform call back on listener to show run of two tests
                    listener.testRunStarted(TEST_PACKAGE_VALUE, 2);
                    listener.testStarted(TEST1);
                    listener.testEnded(TEST1, EMPTY_STRING_MAP);
                    listener.testStarted(TEST2);
                    listener.testEnded(TEST2, EMPTY_STRING_MAP);
                    listener.testRunEnded(1, EMPTY_STRING_MAP);
                    return true;
                };
        doAnswer(runTests)
                .when(mMockTestDevice)
                .runInstrumentationTests(
                        any(IRemoteAndroidTestRunner.class), any(ITestInvocationListener.class));

        mInstrumentationTest.run(mMockListener);

        InOrder inOrder = Mockito.inOrder(mInstrumentationTest, mMockTestDevice, mMockListener);
        ArgumentCaptor<IRemoteAndroidTestRunner> runner =
                ArgumentCaptor.forClass(IRemoteAndroidTestRunner.class);
        inOrder.verify(mInstrumentationTest).setRunnerArgs(runner.capture());
        inOrder.verify(mMockTestDevice, times(2))
                .runInstrumentationTests(eq(runner.getValue()), any(ITestLifeCycleReceiver.class));

        inOrder.verify(mMockListener).testRunStarted(TEST_PACKAGE_VALUE, 2);
        inOrder.verify(mMockListener).testStarted(eq(TEST1), anyLong());
        inOrder.verify(mMockListener).testEnded(eq(TEST1), anyLong(), eq(EMPTY_STRING_MAP));
        inOrder.verify(mMockListener).testStarted(eq(TEST2), anyLong());
        inOrder.verify(mMockListener).testEnded(eq(TEST2), anyLong(), eq(EMPTY_STRING_MAP));
        inOrder.verify(mMockListener).testRunEnded(1, EMPTY_STRING_MAP);
    }

    @Test
    public void testRun_bothAbi() throws DeviceNotAvailableException {
        mInstrumentationTest.setAbi(mock(IAbi.class));
        mInstrumentationTest.setForceAbi("test");
        try {
            mInstrumentationTest.run(mMockListener);
            fail("Should have thrown an exception");
        } catch (IllegalArgumentException e) {
            // expected
        }
    }

    /** Test normal run scenario with --no-hidden-api-check specified */
    @Test
    public void testRun_hiddenApiCheck() throws Exception {
        doReturn(28).when(mMockTestDevice).getApiLevel();
        OptionSetter setter = new OptionSetter(mInstrumentationTest);
        setter.setOptionValue("hidden-api-checks", "false");
        RemoteAndroidTestRunner runner =
                (RemoteAndroidTestRunner)
                        mInstrumentationTest.createRemoteAndroidTestRunner("", "", mMockIDevice);
        assertThat(runner.getRunOptions()).contains("--no-hidden-api-checks");
    }

    /** Test normal run scenario with a test class specified. */
    @Test
    public void testRun_class() {
        String className = "FooTest";
        FakeTestRunner runner = new FakeTestRunner("unused", "unused");

        mInstrumentationTest.setClassName(className);
        mInstrumentationTest.setRunnerArgs(runner);

        assertThat(runner.getArgs()).containsEntry("class", "'" + className + "'");
    }

    /** Test normal run scenario with a test class and method specified. */
    @Test
    public void testRun_classMethod() {
        String className = "FooTest";
        String methodName = "testFoo";
        FakeTestRunner runner = new FakeTestRunner("unused", "unused");

        mInstrumentationTest.setClassName(className);
        mInstrumentationTest.setMethodName(methodName);
        mInstrumentationTest.setRunnerArgs(runner);

        assertThat(runner.getArgs()).containsEntry("class", "'FooTest#testFoo'");
    }

    /** Test normal run scenario with a test package specified. */
    @Test
    public void testRun_testPackage() {
        String testPackageName = "com.foo";
        FakeTestRunner runner = new FakeTestRunner("unused", "unused");

        mInstrumentationTest.setTestPackageName(testPackageName);
        mInstrumentationTest.setRunnerArgs(runner);

        assertThat(runner.getArgs()).containsEntry("package", testPackageName);
    }

    /** Verify test package name is not passed to the runner if class name is set */
    @Test
    public void testRun_testPackageAndClass() {
        String testClassName = "FooTest";
        FakeTestRunner runner = new FakeTestRunner("unused", "unused");

        mInstrumentationTest.setTestPackageName("com.foo");
        mInstrumentationTest.setClassName(testClassName);
        mInstrumentationTest.setRunnerArgs(runner);
        assertThat(runner.getArgs()).containsEntry("class", "'" + testClassName + "'");
        assertThat(runner.getArgs()).doesNotContainKey("package");
    }

    /** Test that IllegalArgumentException is thrown when attempting run without setting device. */
    @Test
    public void testRun_noDevice() throws DeviceNotAvailableException {
        mInstrumentationTest.setDevice(null);

        try {
            mInstrumentationTest.run(mMockListener);
            fail("IllegalArgumentException not thrown");
        } catch (IllegalArgumentException e) {
            // expected
        }
    }

    /** Test the rerun mode when test run has no tests. */
    @Test
    public void testRun_rerunEmpty() throws DeviceNotAvailableException {
        mInstrumentationTest.setRerunMode(true);

        // collect tests run
        RunInstrumentationTestsAnswer collectTest =
                (runner, listener) -> {
                    listener.testRunStarted(TEST_PACKAGE_VALUE, 0);
                    listener.testRunEnded(1, EMPTY_STRING_MAP);
                    return true;
                };
        doAnswer(collectTest)
                .when(mMockTestDevice)
                .runInstrumentationTests(
                        any(IRemoteAndroidTestRunner.class), any(ITestLifeCycleReceiver.class));

        mInstrumentationTest.run(mMockListener);

        // note: expect run to not be reported
        Mockito.verifyNoMoreInteractions(mMockListener);
    }

    /** Test the rerun mode when first test run fails. */
    @Test
    public void testRun_rerun() throws Exception {
        OptionSetter setter = new OptionSetter(mInstrumentationTest);
        setter.setOptionValue("bugreport-on-run-failure", "true");
        mInstrumentationTest.setRerunMode(true);

        // Mock collected tests
        RunInstrumentationTestsAnswer collected =
                (runner, listener) -> {
                    // perform call back on listener to show run of two tests
                    listener.testRunStarted(TEST_PACKAGE_VALUE, 2);
                    listener.testStarted(TEST1);
                    listener.testEnded(TEST1, EMPTY_STRING_MAP);
                    listener.testStarted(TEST2);
                    listener.testEnded(TEST2, EMPTY_STRING_MAP);
                    listener.testRunEnded(1, EMPTY_STRING_MAP);
                    return true;
                };
        RunInstrumentationTestsAnswer partialRun =
                (runner, listener) -> {
                    // perform call back on listener to show run failed - only one test
                    listener.testRunStarted(TEST_PACKAGE_VALUE, 2);
                    listener.testStarted(TEST1);
                    listener.testEnded(TEST1, EMPTY_STRING_MAP);
                    listener.testRunFailed(RUN_ERROR_MSG);
                    listener.testRunEnded(1, EMPTY_STRING_MAP);
                    return true;
                };
        RunInstrumentationTestsAnswer rerun =
                (runner, listener) -> {
                    // perform call back on listeners to show run remaining test was run
                    listener.testRunStarted(TEST_PACKAGE_VALUE, 1);
                    listener.testStarted(TEST2);
                    listener.testEnded(TEST2, EMPTY_STRING_MAP);
                    listener.testRunEnded(1, EMPTY_STRING_MAP);
                    return true;
                };

        doAnswer(collected)
                .doAnswer(partialRun)
                .doAnswer(rerun)
                .when(mMockTestDevice)
                .runInstrumentationTests(
                        any(IRemoteAndroidTestRunner.class), any(ITestLifeCycleReceiver.class));

        mInstrumentationTest.run(mMockListener);

        InOrder inOrder = Mockito.inOrder(mMockListener, mMockTestDevice);
        inOrder.verify(mMockListener).testRunStarted(TEST_PACKAGE_VALUE, 2);
        inOrder.verify(mMockListener).testStarted(eq(TEST1), anyLong());
        inOrder.verify(mMockListener).testEnded(eq(TEST1), anyLong(), eq(EMPTY_STRING_MAP));
        inOrder.verify(mMockListener).testRunFailed(RUN_ERROR_MSG);
        inOrder.verify(mMockListener).testRunEnded(1, EMPTY_STRING_MAP);
        // Expect a bugreport since there was a failure.
        inOrder.verify(mMockTestDevice)
                .logBugreport("bugreport-on-run-failure-com.foo", mMockListener);
        inOrder.verify(mMockListener).testRunStarted(TEST_PACKAGE_VALUE, 1);
        inOrder.verify(mMockListener).testStarted(eq(TEST2), anyLong());
        inOrder.verify(mMockListener).testEnded(eq(TEST2), anyLong(), eq(EMPTY_STRING_MAP));
        inOrder.verify(mMockListener).testRunEnded(1, EMPTY_STRING_MAP);
        inOrder.verifyNoMoreInteractions();
    }

    /** Verify that all tests are re-run when there is a failure during a coverage run. */
    @Test
    public void testRun_rerunCoverage() throws ConfigurationException, DeviceNotAvailableException {
        mInstrumentationTest.setRerunMode(true);
        mInstrumentationTest.setCoverage(true);

        Collection<TestDescription> expectedTests = ImmutableList.of(TEST1, TEST2);

        // Mock collected tests
        RunInstrumentationTestsAnswer collected =
                (runner, listener) -> {
                    // perform call back on listener to show run of two tests
                    listener.testRunStarted(TEST_PACKAGE_VALUE, 2);
                    listener.testStarted(TEST1);
                    listener.testEnded(TEST1, EMPTY_STRING_MAP);
                    listener.testStarted(TEST2);
                    listener.testEnded(TEST2, EMPTY_STRING_MAP);
                    listener.testRunEnded(1, EMPTY_STRING_MAP);
                    return true;
                };
        RunInstrumentationTestsAnswer partialRun =
                (runner, listener) -> {
                    // perform call back on listener to show run failed - only one test
                    listener.testRunStarted(TEST_PACKAGE_VALUE, 2);
                    listener.testStarted(TEST1);
                    listener.testEnded(TEST1, EMPTY_STRING_MAP);
                    listener.testRunFailed(RUN_ERROR_MSG);
                    listener.testRunEnded(1, EMPTY_STRING_MAP);
                    return true;
                };
        RunInstrumentationTestsAnswer rerun1 =
                (runner, listener) -> {
                    // perform call back on listeners to show run remaining test was run
                    listener.testRunStarted(TEST_PACKAGE_VALUE, 1);
                    listener.testStarted(TEST1);
                    listener.testEnded(TEST1, EMPTY_STRING_MAP);
                    listener.testRunEnded(1, EMPTY_STRING_MAP);
                    return true;
                };
        RunInstrumentationTestsAnswer rerun2 =
                (runner, listener) -> {
                    // perform call back on listeners to show run remaining test was run
                    listener.testRunStarted(TEST_PACKAGE_VALUE, 1);
                    listener.testStarted(TEST2);
                    listener.testEnded(TEST2, EMPTY_STRING_MAP);
                    listener.testRunEnded(1, EMPTY_STRING_MAP);
                    return true;
                };

        doAnswer(collected)
                .doAnswer(partialRun)
                .doAnswer(rerun1)
                .doAnswer(rerun2)
                .when(mMockTestDevice)
                .runInstrumentationTests(
                        any(IRemoteAndroidTestRunner.class), any(ITestLifeCycleReceiver.class));

        mInstrumentationTest.run(mMockListener);

        verify(mInstrumentationTest).getTestReRunner(testCaptor.capture());
        assertThat(testCaptor.getValue()).containsExactlyElementsIn(expectedTests);

        InOrder inOrder = Mockito.inOrder(mMockListener);
        inOrder.verify(mMockListener).testRunStarted(TEST_PACKAGE_VALUE, 2);
        inOrder.verify(mMockListener).testStarted(eq(TEST1), anyLong());
        inOrder.verify(mMockListener).testEnded(eq(TEST1), anyLong(), eq(EMPTY_STRING_MAP));
        inOrder.verify(mMockListener).testRunFailed(RUN_ERROR_MSG);
        inOrder.verify(mMockListener).testRunEnded(1, EMPTY_STRING_MAP);
        inOrder.verify(mMockListener).testRunStarted(TEST_PACKAGE_VALUE, 1);
        inOrder.verify(mMockListener).testStarted(eq(TEST1), anyLong());
        inOrder.verify(mMockListener).testEnded(eq(TEST1), anyLong(), eq(EMPTY_STRING_MAP));
        inOrder.verify(mMockListener).testRunEnded(1, EMPTY_STRING_MAP);
        inOrder.verify(mMockListener).testStarted(eq(TEST2), anyLong());
        inOrder.verify(mMockListener).testEnded(eq(TEST2), anyLong(), eq(EMPTY_STRING_MAP));
        inOrder.verify(mMockListener).testRunEnded(1, EMPTY_STRING_MAP);
        inOrder.verifyNoMoreInteractions();
    }

    /** Test the reboot before re-run option. */
    @Test
    public void testRun_rebootBeforeReRun() throws DeviceNotAvailableException {
        mInstrumentationTest.setRerunMode(true);
        mInstrumentationTest.setRebootBeforeReRun(true);

        RunInstrumentationTestsAnswer collected =
                (runner, listener) -> {
                    // perform call back on listener to show run of two tests
                    listener.testRunStarted(TEST_PACKAGE_VALUE, 2);
                    listener.testStarted(TEST1);
                    listener.testEnded(TEST1, EMPTY_STRING_MAP);
                    listener.testStarted(TEST2);
                    listener.testEnded(TEST2, EMPTY_STRING_MAP);
                    listener.testRunEnded(1, EMPTY_STRING_MAP);
                    return true;
                };
        RunInstrumentationTestsAnswer partialRun =
                (runner, listener) -> {
                    // perform call back on listener to show run failed - only one test
                    listener.testRunStarted(TEST_PACKAGE_VALUE, 2);
                    listener.testStarted(TEST1);
                    listener.testEnded(TEST1, EMPTY_STRING_MAP);
                    listener.testRunFailed(RUN_ERROR_MSG);
                    listener.testRunEnded(1, EMPTY_STRING_MAP);
                    return true;
                };
        RunInstrumentationTestsAnswer rerun =
                (runner, listener) -> {
                    // perform call back on listeners to show run remaining test was run
                    listener.testRunStarted(TEST_PACKAGE_VALUE, 1);
                    listener.testStarted(TEST2);
                    listener.testEnded(TEST2, EMPTY_STRING_MAP);
                    listener.testRunEnded(1, EMPTY_STRING_MAP);
                    return true;
                };

        doAnswer(collected)
                .doAnswer(partialRun)
                .doAnswer(rerun)
                .when(mMockTestDevice)
                .runInstrumentationTests(
                        any(IRemoteAndroidTestRunner.class), any(ITestInvocationListener.class));

        mInstrumentationTest.run(mMockListener);

        InOrder inOrder = Mockito.inOrder(mMockListener, mMockTestDevice);
        inOrder.verify(mMockListener).testRunStarted(TEST_PACKAGE_VALUE, 2);
        inOrder.verify(mMockListener).testStarted(eq(TEST1), anyLong());
        inOrder.verify(mMockListener).testEnded(eq(TEST1), anyLong(), eq(EMPTY_STRING_MAP));
        inOrder.verify(mMockListener).testRunFailed(RUN_ERROR_MSG);
        inOrder.verify(mMockListener).testRunEnded(1, EMPTY_STRING_MAP);
        inOrder.verify(mMockTestDevice).reboot();
        inOrder.verify(mMockListener).testRunStarted(TEST_PACKAGE_VALUE, 1);
        inOrder.verify(mMockListener).testStarted(eq(TEST2), anyLong());
        inOrder.verify(mMockListener).testEnded(eq(TEST2), anyLong(), eq(EMPTY_STRING_MAP));
        inOrder.verify(mMockListener).testRunEnded(1, EMPTY_STRING_MAP);
    }

    /**
     * Test resuming a test run when first run is aborted due to {@link DeviceNotAvailableException}
     */
    @Test
    public void testRun_resume() throws DeviceNotAvailableException {
        RunInstrumentationTestsAnswer collected =
                (runner, listener) -> {
                    // perform call back on listener to show run of two tests
                    listener.testRunStarted(TEST_PACKAGE_VALUE, 2);
                    listener.testStarted(TEST1);
                    listener.testEnded(TEST1, EMPTY_STRING_MAP);
                    listener.testStarted(TEST2);
                    listener.testEnded(TEST2, EMPTY_STRING_MAP);
                    listener.testRunEnded(1, EMPTY_STRING_MAP);
                    return true;
                };
        RunInstrumentationTestsAnswer partialRun =
                (runner, listener) -> {
                    // perform call back on listener to show run failed - only one test
                    listener.testRunStarted(TEST_PACKAGE_VALUE, 2);
                    listener.testStarted(TEST1);
                    listener.testEnded(TEST1, EMPTY_STRING_MAP);
                    listener.testRunFailed(RUN_ERROR_MSG);
                    listener.testRunEnded(1, EMPTY_STRING_MAP);
                    throw new DeviceNotAvailableException();
                };
        RunInstrumentationTestsAnswer rerun =
                (runner, listener) -> {
                    // perform call back on listeners to show run remaining test was run
                    listener.testRunStarted(TEST_PACKAGE_VALUE, 1);
                    listener.testStarted(TEST2);
                    listener.testEnded(TEST2, EMPTY_STRING_MAP);
                    listener.testRunEnded(1, EMPTY_STRING_MAP);
                    return true;
                };

        doAnswer(collected)
                .doAnswer(partialRun)
                .doAnswer(rerun)
                .when(mMockTestDevice)
                .runInstrumentationTests(
                        any(IRemoteAndroidTestRunner.class), any(ITestLifeCycleReceiver.class));

        try {
            mInstrumentationTest.run(mMockListener);
            fail("DeviceNotAvailableException not thrown");
        } catch (DeviceNotAvailableException e) {
            // expected
        }
        mInstrumentationTest.run(mMockListener);

        InOrder inOrder = Mockito.inOrder(mMockListener);
        inOrder.verify(mMockListener).testRunStarted(TEST_PACKAGE_VALUE, 2);
        inOrder.verify(mMockListener).testStarted(eq(TEST1), anyLong());
        inOrder.verify(mMockListener).testEnded(eq(TEST1), anyLong(), eq(EMPTY_STRING_MAP));
        inOrder.verify(mMockListener).testRunFailed(RUN_ERROR_MSG);
        inOrder.verify(mMockListener).testRunEnded(1, EMPTY_STRING_MAP);
        inOrder.verify(mMockListener).testRunStarted(TEST_PACKAGE_VALUE, 1);
        inOrder.verify(mMockListener).testStarted(eq(TEST2), anyLong());
        inOrder.verify(mMockListener).testEnded(eq(TEST2), anyLong(), eq(EMPTY_STRING_MAP));
        inOrder.verify(mMockListener).testRunEnded(1, EMPTY_STRING_MAP);
    }

    /**
     * Test that IllegalArgumentException is thrown when attempting run with negative timeout args.
     */
    @Test
    public void testRun_negativeTimeouts() throws DeviceNotAvailableException {
        mInstrumentationTest.setShellTimeout(-1);
        mInstrumentationTest.setTestTimeout(-2);

        try {
            mInstrumentationTest.run(mMockListener);
            fail("IllegalArgumentException not thrown");
        } catch (IllegalArgumentException e) {
            // expected
        }
    }

    /** Test that IllegalArgumentException is thrown if an invalid test size is provided. */
    @Test
    public void testRun_badTestSize() throws DeviceNotAvailableException {
        mInstrumentationTest.setTestSize("foo");

        try {
            mInstrumentationTest.run(mMockListener);
            fail("IllegalArgumentException not thrown");
        } catch (IllegalArgumentException e) {
            // expected
        }
    }

    @Test
    public void testQueryRunnerName() throws DeviceNotAvailableException {
        String queriedRunner = mInstrumentationTest.queryRunnerName();
        assertThat(queriedRunner).isEqualTo("runner1");
    }

    @Test
    public void testQueryRunnerName_noMatch() throws DeviceNotAvailableException {
        mInstrumentationTest.setPackageName("noMatchPackage");
        String queriedRunner = mInstrumentationTest.queryRunnerName();
        assertThat(queriedRunner).isNull();
    }

    @Test
    public void testRun_noMatchingRunner() throws DeviceNotAvailableException {
        mInstrumentationTest.setPackageName("noMatchPackage");
        mInstrumentationTest.setRunnerName(null);
        try {
            mInstrumentationTest.run(mMockListener);
            fail("Should have thrown an exception.");
        } catch (IllegalArgumentException e) {
            // expected
        }
    }

    /**
     * Test for {@link InstrumentationTest#collectTestsAndRetry(IRemoteAndroidTestRunner,
     * ITestInvocationListener)} when the collection fails.
     */
    @Test
    public void testCollectTestsAndRetry_Failure() throws DeviceNotAvailableException {
        RunInstrumentationTestsAnswer collected =
                (runner, listener) -> {
                    listener.testRunStarted(TEST_PACKAGE_VALUE, 0);
                    listener.testRunFailed(InstrumentationResultParser.INVALID_OUTPUT_ERR_MSG);
                    listener.testRunEnded(1, EMPTY_STRING_MAP);
                    return true;
                };

        doAnswer(collected)
                .when(mMockTestDevice)
                .runInstrumentationTests(
                        any(IRemoteAndroidTestRunner.class), any(ITestLifeCycleReceiver.class));

        mInstrumentationTest.setEnforceFormat(true);

        try {
            mInstrumentationTest.collectTestsAndRetry(mock(IRemoteAndroidTestRunner.class), null);
            fail("Should have thrown an exception");
        } catch (RuntimeException e) {
            // expected.
        }
    }

    /**
     * Test for {@link InstrumentationTest#collectTestsAndRetry(IRemoteAndroidTestRunner,
     * ITestInvocationListener)} when the tests collection fails but we do not enforce the format,
     * so we don't throw an exception.
     */
    @Test
    public void testCollectTestsAndRetry_notEnforced() throws DeviceNotAvailableException {
        RunInstrumentationTestsAnswer collected =
                (runner, listener) -> {
                    listener.testRunStarted(TEST_PACKAGE_VALUE, 0);
                    listener.testRunFailed(InstrumentationResultParser.INVALID_OUTPUT_ERR_MSG);
                    listener.testRunEnded(1, EMPTY_STRING_MAP);
                    return true;
                };

        doAnswer(collected)
                .when(mMockTestDevice)
                .runInstrumentationTests(
                        any(IRemoteAndroidTestRunner.class), any(ITestLifeCycleReceiver.class));

        mInstrumentationTest.setEnforceFormat(false);

        Collection<TestDescription> result =
                mInstrumentationTest.collectTestsAndRetry(
                        mock(IRemoteAndroidTestRunner.class), null);
        assertThat(result).isNull();
    }

    /**
     * Test that if we successfully collect a list of tests and the run crash and try to report 0 we
     * instead do report the number of collected test to get an appropriate count.
     */
    @Test
    public void testCollectWorks_RunCrash() throws Exception {
        doReturn(mock(IRemoteTest.class))
                .when(mInstrumentationTest)
                .getTestReRunner(anyCollectionOf(TestDescription.class));

        // We collect successfully 5 tests
        RunInstrumentationTestsAnswer collected =
                (runner, listener) -> {
                    listener.testRunStarted("fakeName", 5);
                    for (int i = 0; i < 5; i++) {
                        TestDescription tid = new TestDescription("fakeclass", "fakemethod" + i);
                        listener.testStarted(tid, 5);
                        listener.testEnded(tid, 15, EMPTY_STRING_MAP);
                    }
                    listener.testRunEnded(500, EMPTY_STRING_MAP);
                    return true;
                };

        // We attempt to run and crash
        RunInstrumentationTestsAnswer partialRun =
                (runner, listener) -> {
                    listener.testRunStarted("fakeName", 0);
                    listener.testRunFailed("Instrumentation run failed due to 'Process crashed.'");
                    listener.testRunEnded(1, EMPTY_STRING_MAP);
                    return true;
                };

        doAnswer(collected)
                .doAnswer(partialRun)
                .when(mMockTestDevice)
                .runInstrumentationTests(
                        any(IRemoteAndroidTestRunner.class), any(ITestLifeCycleReceiver.class));

        mInstrumentationTest.run(mMockListener);

        // The reported number of tests is the one from the collected output
        InOrder inOrder = Mockito.inOrder(mMockListener);
        inOrder.verify(mMockListener).testRunStarted("fakeName", 5);
        inOrder.verify(mMockListener)
                .testRunFailed("Instrumentation run failed due to 'Process crashed.'");
        inOrder.verify(mMockListener).testRunEnded(1, EMPTY_STRING_MAP);
    }

    @Test
    public void testAddScreenshotListener_enabled() {
        mInstrumentationTest.setScreenshotOnFailure(true);

        ITestInvocationListener listener =
                mInstrumentationTest.addScreenshotListenerIfEnabled(mMockListener);
        assertThat(listener).isInstanceOf(InstrumentationTest.FailedTestScreenshotGenerator.class);
    }

    @Test
    public void testAddScreenshotListener_disabled() {
        mInstrumentationTest.setScreenshotOnFailure(false);

        ITestInvocationListener listener =
                mInstrumentationTest.addScreenshotListenerIfEnabled(mMockListener);
        assertThat(listener).isSameAs(mMockListener);
    }

    @Test
    public void testAddLogcatListener_enabled() {
        mInstrumentationTest.setLogcatOnFailure(true);

        ITestInvocationListener listener =
                mInstrumentationTest.addLogcatListenerIfEnabled(mMockListener);
        assertThat(listener).isInstanceOf(InstrumentationTest.FailedTestLogcatGenerator.class);
    }

    @Test
    public void testAddLogcatListener_setMaxSize() {
        int maxSize = 1234;
        mInstrumentationTest.setLogcatOnFailure(true);
        mInstrumentationTest.setLogcatOnFailureSize(maxSize);

        ITestInvocationListener listener =
                mInstrumentationTest.addLogcatListenerIfEnabled(mMockListener);
        assertThat(listener).isInstanceOf(InstrumentationTest.FailedTestLogcatGenerator.class);

        InstrumentationTest.FailedTestLogcatGenerator logcatGenerator =
                (InstrumentationTest.FailedTestLogcatGenerator) listener;
        assertThat(logcatGenerator.getMaxSize()).isEqualTo(maxSize);
    }

    @Test
    public void testAddLogcatListener_disabled() {
        mInstrumentationTest.setLogcatOnFailure(false);

        ITestInvocationListener listener =
                mInstrumentationTest.addLogcatListenerIfEnabled(mMockListener);
        assertThat(listener).isSameAs(mMockListener);
    }

    @Test
    public void testAddCoverageListener_enabled() {
        mInstrumentationTest.setCoverage(true);

        ITestInvocationListener listener =
                mInstrumentationTest.addCoverageListenerIfEnabled(mMockListener);
        assertThat(listener).isInstanceOf(CodeCoverageListener.class);
    }

    @Test
    public void testAddCoverageListener_disabled() {
        mInstrumentationTest.setCoverage(false);

        ITestInvocationListener listener =
                mInstrumentationTest.addCoverageListenerIfEnabled(mMockListener);
        assertThat(listener).isSameAs(mMockListener);
    }

    private static class FakeTestRunner extends RemoteAndroidTestRunner {

        private Map<String, String> mArgs = new HashMap<>();
        private String mRunOptions;

        FakeTestRunner(String packageName, String runnerName) {
            super(packageName, runnerName, null);
        }

        @Override
        public void addInstrumentationArg(String name, String value) {
            mArgs.put(name, value);
        }

        Map<String, String> getArgs() {
            return ImmutableMap.copyOf(mArgs);
        }
    }
}
