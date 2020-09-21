/*
 * Copyright (C) 2014 The Android Open Source Project
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

import com.android.ddmlib.IDevice;
import com.android.ddmlib.testrunner.IRemoteAndroidTestRunner;
import com.android.ddmlib.testrunner.ITestRunListener;
import com.android.tradefed.config.ConfigurationException;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.ITestLifeCycleReceiver;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.result.ddmlib.TestRunToTestInvocationForwarder;

import junit.framework.TestCase;

import org.easymock.EasyMock;
import org.easymock.IAnswer;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.HashMap;

/**
 * Unit tests for {@link InstrumentationFileTest}.
 */
public class InstrumentationFileTestTest extends TestCase {

    private static final String TEST_PACKAGE_VALUE = "com.foo";

    /** The {@link InstrumentationFileTest} under test, with all dependencies mocked out */
    private InstrumentationFileTest mInstrumentationFileTest;

    private ITestDevice mMockTestDevice;
    private ITestInvocationListener mMockListener;
    private InstrumentationTest mMockITest;

    private File mTestFile;

    @Override
    protected void setUp() throws Exception {
        super.setUp();

        mTestFile = null;

        IDevice mockIDevice = EasyMock.createMock(IDevice.class);
        mMockTestDevice = EasyMock.createMock(ITestDevice.class);
        mMockListener = EasyMock.createMock(ITestInvocationListener.class);

        EasyMock.expect(mMockTestDevice.getIDevice()).andStubReturn(mockIDevice);
        EasyMock.expect(mMockTestDevice.getSerialNumber()).andStubReturn("serial");

        // mock out InstrumentationTest that will be used to create InstrumentationFileTest
        mMockITest = new InstrumentationTest() {
            @Override
            protected String queryRunnerName() {
                return "runner";
            }
        };
        mMockITest.setDevice(mMockTestDevice);
        mMockITest.setPackageName(TEST_PACKAGE_VALUE);
    }

    /**
     * Test normal run scenario with a single test.
     */
    public void testRun_singleSuccessfulTest() throws DeviceNotAvailableException,
            ConfigurationException {
        final Collection<TestDescription> testsList = new ArrayList<>(1);
        final TestDescription test = new TestDescription("ClassFoo", "methodBar");
        testsList.add(test);

        // verify the mock listener is passed through to the runner
        RunTestAnswer runTestResponse =
                new RunTestAnswer() {
                    @Override
                    public Boolean answer(
                            IRemoteAndroidTestRunner runner, ITestRunListener listener) {
                        listener.testRunStarted(TEST_PACKAGE_VALUE, 1);
                        listener.testStarted(TestDescription.convertToIdentifier(test));
                        listener.testEnded(
                                TestDescription.convertToIdentifier(test), Collections.emptyMap());
                        listener.testRunEnded(0, Collections.emptyMap());
                        return true;
                    }
                };
        setRunTestExpectations(runTestResponse);
        mInstrumentationFileTest = new InstrumentationFileTest(mMockITest, testsList, true, -1) {
            @Override
            InstrumentationTest createInstrumentationTest() {
                return mMockITest;
            }
            @Override
            boolean pushFileToTestDevice(File file, String destinationPath)
                    throws DeviceNotAvailableException {
                // simulate successful push and store created file
                mTestFile = file;
                // verify that the content of the testFile contains all expected tests
                verifyTestFile(testsList);
                return true;
            }
            @Override
            void deleteTestFileFromDevice(String pathToFile) throws DeviceNotAvailableException {
                //ignore
            }
        };

        // mock successful test run lifecycle
        mMockListener.testRunStarted(TEST_PACKAGE_VALUE, 1);
        mMockListener.testStarted(EasyMock.eq(test), EasyMock.anyLong());
        mMockListener.testEnded(
                EasyMock.eq(test), EasyMock.anyLong(), EasyMock.eq(new HashMap<String, Metric>()));
        mMockListener.testRunEnded(0, new HashMap<String, Metric>());

        EasyMock.replay(mMockListener, mMockTestDevice);
        mInstrumentationFileTest.run(mMockListener);
        assertEquals(mMockTestDevice, mMockITest.getDevice());
    }

    /**
     * Test re-run scenario when 1 out of 3 tests fails to complete but is successful after re-run
     */
    public void testRun_reRunOneFailedToCompleteTest()
            throws DeviceNotAvailableException, ConfigurationException {
        final Collection<TestDescription> testsList = new ArrayList<>(1);
        final TestDescription test1 = new TestDescription("ClassFoo1", "methodBar1");
        final TestDescription test2 = new TestDescription("ClassFoo2", "methodBar2");
        final TestDescription test3 = new TestDescription("ClassFoo3", "methodBar3");
        testsList.add(test1);
        testsList.add(test2);
        testsList.add(test3);

        // verify the test1 is completed and test2 was started but never finished
        RunTestAnswer firstRunAnswer =
                new RunTestAnswer() {
                    @Override
                    public Boolean answer(
                            IRemoteAndroidTestRunner runner, ITestRunListener listener) {
                        // first test started and ended successfully
                        listener.testRunStarted(TEST_PACKAGE_VALUE, 2);
                        listener.testStarted(TestDescription.convertToIdentifier(test1));
                        listener.testEnded(
                                TestDescription.convertToIdentifier(test1), Collections.emptyMap());
                        listener.testRunEnded(1, Collections.emptyMap());
                        // second test started but never finished
                        listener.testStarted(TestDescription.convertToIdentifier(test2));
                        return true;
                    }
                };
        setRunTestExpectations(firstRunAnswer);

        // now expect second run to rerun remaining test3 and test2
        RunTestAnswer secondRunAnswer =
                new RunTestAnswer() {
                    @Override
                    public Boolean answer(
                            IRemoteAndroidTestRunner runner, ITestRunListener listener) {
                        // third test started and ended successfully
                        listener.testRunStarted(TEST_PACKAGE_VALUE, 2);
                        listener.testStarted(TestDescription.convertToIdentifier(test3));
                        listener.testEnded(
                                TestDescription.convertToIdentifier(test3), Collections.emptyMap());
                        listener.testRunEnded(1, Collections.emptyMap());
                        // second test is rerun but completed successfully this time
                        listener.testStarted(TestDescription.convertToIdentifier(test2));
                        listener.testEnded(
                                TestDescription.convertToIdentifier(test2), Collections.emptyMap());
                        listener.testRunEnded(1, Collections.emptyMap());
                        return true;
                    }
                };
        setRunTestExpectations(secondRunAnswer);
        mInstrumentationFileTest = new InstrumentationFileTest(mMockITest, testsList, true, -1) {
            @Override
            InstrumentationTest createInstrumentationTest() {
                return mMockITest;
            }
            @Override
            boolean pushFileToTestDevice(File file, String destinationPath)
                    throws DeviceNotAvailableException {
                // simulate successful push and store created file
                mTestFile = file;
                // verify that the content of the testFile contains all expected tests
                verifyTestFile(testsList);
                return true;
            }
            @Override
            void deleteTestFileFromDevice(String pathToFile) throws DeviceNotAvailableException {
                //ignore
            }
        };

        // First run:
        mMockListener.testRunStarted(TEST_PACKAGE_VALUE, 2);
        // expect test1 to start and finish successfully
        mMockListener.testStarted(EasyMock.eq(test1), EasyMock.anyLong());
        mMockListener.testEnded(
                EasyMock.eq(test1), EasyMock.anyLong(), EasyMock.eq(new HashMap<String, Metric>()));
        mMockListener.testRunEnded(1, new HashMap<String, Metric>());
        // expect test2 to start but never finish
        mMockListener.testStarted(EasyMock.eq(test2), EasyMock.anyLong());
        // Second run:
        mMockListener.testRunStarted(TEST_PACKAGE_VALUE, 2);
        // expect test3 to start and finish successfully
        mMockListener.testStarted(EasyMock.eq(test3), EasyMock.anyLong());
        mMockListener.testEnded(
                EasyMock.eq(test3), EasyMock.anyLong(), EasyMock.eq(new HashMap<String, Metric>()));
        mMockListener.testRunEnded(1, new HashMap<String, Metric>());
        // expect to rerun test2 successfully
        mMockListener.testStarted(EasyMock.eq(test2), EasyMock.anyLong());
        mMockListener.testEnded(
                EasyMock.eq(test2), EasyMock.anyLong(), EasyMock.eq(new HashMap<String, Metric>()));
        mMockListener.testRunEnded(1, new HashMap<String, Metric>());

        EasyMock.replay(mMockListener, mMockTestDevice);
        mInstrumentationFileTest.run(mMockListener);
        assertEquals(mMockTestDevice, mMockITest.getDevice());
    }

    /**
     * Test re-run scenario when 2 remaining tests fail to complete and need to be run serially
     */
    public void testRun_serialReRunOfTwoFailedToCompleteTests()
            throws DeviceNotAvailableException, ConfigurationException {
        final Collection<TestDescription> testsList = new ArrayList<>(1);
        final TestDescription test1 = new TestDescription("ClassFoo1", "methodBar1");
        final TestDescription test2 = new TestDescription("ClassFoo2", "methodBar2");
        testsList.add(test1);
        testsList.add(test2);

        // verify the test1 and test2 started but never completed
        RunTestAnswer firstRunAnswer =
                new RunTestAnswer() {
                    @Override
                    public Boolean answer(
                            IRemoteAndroidTestRunner runner, ITestRunListener listener) {
                        // first and second tests started but never completed
                        listener.testRunStarted(TEST_PACKAGE_VALUE, 2);
                        listener.testStarted(TestDescription.convertToIdentifier(test1));
                        listener.testStarted(TestDescription.convertToIdentifier(test2));
                        // verify that the content of the testFile contains all expected tests
                        verifyTestFile(testsList);
                        return true;
                    }
                };
        setRunTestExpectations(firstRunAnswer);

        // verify successful serial execution of test1
        RunTestAnswer firstSerialRunAnswer =
                new RunTestAnswer() {
                    @Override
                    public Boolean answer(
                            IRemoteAndroidTestRunner runner, ITestRunListener listener) {
                        // first test started and ended successfully in serial mode
                        listener.testRunStarted(TEST_PACKAGE_VALUE, 1);
                        listener.testStarted(TestDescription.convertToIdentifier(test1));
                        listener.testEnded(
                                TestDescription.convertToIdentifier(test1), Collections.emptyMap());
                        listener.testRunEnded(1, Collections.emptyMap());
                        return true;
                    }
                };
        setRunTestExpectations(firstSerialRunAnswer);

        // verify successful serial execution of test2
        RunTestAnswer secdondSerialRunAnswer =
                new RunTestAnswer() {
                    @Override
                    public Boolean answer(
                            IRemoteAndroidTestRunner runner, ITestRunListener listener) {
                        // Second test started and ended successfully in serial mode
                        listener.testRunStarted(TEST_PACKAGE_VALUE, 1);
                        listener.testStarted(TestDescription.convertToIdentifier(test2));
                        listener.testEnded(
                                TestDescription.convertToIdentifier(test2), Collections.emptyMap());
                        listener.testRunEnded(1, Collections.emptyMap());
                        return true;
                    }
                };
        setRunTestExpectations(secdondSerialRunAnswer);

        mInstrumentationFileTest = new InstrumentationFileTest(mMockITest, testsList, true, -1) {
            @Override
            InstrumentationTest createInstrumentationTest() {
                return mMockITest;
            }
            @Override
            boolean pushFileToTestDevice(File file, String destinationPath)
                    throws DeviceNotAvailableException {
                // simulate successful push and store created file
                mTestFile = file;
                return true;
            }
            @Override
            void deleteTestFileFromDevice(String pathToFile) throws DeviceNotAvailableException {
                //ignore
            }
        };

        // First run:
        mMockListener.testRunStarted(TEST_PACKAGE_VALUE, 2);
        // expect test1 and test 2 to start but never finish
        mMockListener.testStarted(EasyMock.eq(test1), EasyMock.anyLong());
        mMockListener.testStarted(EasyMock.eq(test2), EasyMock.anyLong());

        // re-run test1 and test2 serially
        // first serial re-run:
        mMockListener.testRunStarted(TEST_PACKAGE_VALUE, 1);
        // expect test1 to start and finish successfully
        mMockListener.testStarted(EasyMock.eq(test1), EasyMock.anyLong());
        mMockListener.testEnded(
                EasyMock.eq(test1), EasyMock.anyLong(), EasyMock.eq(new HashMap<String, Metric>()));
        mMockListener.testRunEnded(1, new HashMap<String, Metric>());
        // first serial re-run:
        mMockListener.testRunStarted(TEST_PACKAGE_VALUE, 1);
        // expect test2 to start and finish successfully
        mMockListener.testStarted(EasyMock.eq(test2), EasyMock.anyLong());
        mMockListener.testEnded(
                EasyMock.eq(test2), EasyMock.anyLong(), EasyMock.eq(new HashMap<String, Metric>()));
        mMockListener.testRunEnded(1, new HashMap<String, Metric>());

        EasyMock.replay(mMockListener, mMockTestDevice);
        mInstrumentationFileTest.run(mMockListener);
        assertEquals(mMockTestDevice, mMockITest.getDevice());
        // test file is expected to be null since we defaulted to serial test execution
        assertEquals(null, mMockITest.getTestFilePathOnDevice());
    }

    /**
     * Test no serial re-run tests fail to complete.
     */
    public void testRun_noSerialReRun()
            throws DeviceNotAvailableException, ConfigurationException {
        final Collection<TestDescription> testsList = new ArrayList<>(1);
        final TestDescription test1 = new TestDescription("ClassFoo1", "methodBar1");
        final TestDescription test2 = new TestDescription("ClassFoo2", "methodBar2");
        testsList.add(test1);
        testsList.add(test2);

        // verify the test1 and test2 started but never completed
        RunTestAnswer firstRunAnswer =
                new RunTestAnswer() {
                    @Override
                    public Boolean answer(
                            IRemoteAndroidTestRunner runner, ITestRunListener listener) {
                        // first and second tests started but never completed
                        listener.testRunStarted(TEST_PACKAGE_VALUE, 2);
                        listener.testStarted(TestDescription.convertToIdentifier(test1));
                        listener.testStarted(TestDescription.convertToIdentifier(test2));
                        // verify that the content of the testFile contains all expected tests
                        verifyTestFile(testsList);
                        return true;
                    }
                };
        setRunTestExpectations(firstRunAnswer);

        mInstrumentationFileTest = new InstrumentationFileTest(mMockITest, testsList, false, -1) {
            @Override
            InstrumentationTest createInstrumentationTest() {
                return mMockITest;
            }
            @Override
            boolean pushFileToTestDevice(File file, String destinationPath)
                    throws DeviceNotAvailableException {
                // simulate successful push and store created file
                mTestFile = file;
                return true;
            }
            @Override
            void deleteTestFileFromDevice(String pathToFile) throws DeviceNotAvailableException {
                //ignore
            }
        };

        // First run:
        mMockListener.testRunStarted(TEST_PACKAGE_VALUE, 2);
        // expect test1 and test 2 to start but never finish
        mMockListener.testStarted(EasyMock.eq(test1), EasyMock.anyLong());
        mMockListener.testStarted(EasyMock.eq(test2), EasyMock.anyLong());

        EasyMock.replay(mMockListener, mMockTestDevice);
        mInstrumentationFileTest.run(mMockListener);
        assertEquals(mMockTestDevice, mMockITest.getDevice());
    }

    /**
     * Test attempting times exceed max attempts.
     */
    public void testRun_exceedMaxAttempts()
            throws DeviceNotAvailableException, ConfigurationException {
        final ArrayList<TestDescription> testsList = new ArrayList<>(1);
        final TestDescription test1 = new TestDescription("ClassFoo1", "methodBar1");
        final TestDescription test2 = new TestDescription("ClassFoo2", "methodBar2");
        final TestDescription test3 = new TestDescription("ClassFoo3", "methodBar3");
        final TestDescription test4 = new TestDescription("ClassFoo4", "methodBar4");
        final TestDescription test5 = new TestDescription("ClassFoo5", "methodBar5");
        final TestDescription test6 = new TestDescription("ClassFoo6", "methodBar6");

        testsList.add(test1);
        testsList.add(test2);
        testsList.add(test3);
        testsList.add(test4);
        testsList.add(test5);
        testsList.add(test6);

        final ArrayList<TestDescription> expectedTestsList = new ArrayList<>(testsList);

        // test1 fininshed, test2 started but not finished.
        RunTestAnswer firstRunAnswer =
                new RunTestAnswer() {
                    @Override
                    public Boolean answer(
                            IRemoteAndroidTestRunner runner, ITestRunListener listener) {
                        listener.testRunStarted(TEST_PACKAGE_VALUE, 6);
                        // first test started and ended successfully
                        listener.testStarted(TestDescription.convertToIdentifier(test1));
                        listener.testEnded(
                                TestDescription.convertToIdentifier(test1), Collections.emptyMap());
                        listener.testRunEnded(1, Collections.emptyMap());
                        // second test started but never finished
                        listener.testStarted(TestDescription.convertToIdentifier(test2));
                        // verify that the content of the testFile contains all expected tests
                        verifyTestFile(expectedTestsList);
                        return true;
                    }
                };
        setRunTestExpectations(firstRunAnswer);

        // test2 finished, test3 started but not finished.
        RunTestAnswer secondRunAnswer =
                new RunTestAnswer() {
                    @Override
                    public Boolean answer(
                            IRemoteAndroidTestRunner runner, ITestRunListener listener) {
                        // test2 started and ended successfully
                        listener.testRunStarted(TEST_PACKAGE_VALUE, 5);
                        listener.testStarted(TestDescription.convertToIdentifier(test2));
                        listener.testEnded(
                                TestDescription.convertToIdentifier(test2), Collections.emptyMap());
                        listener.testRunEnded(1, Collections.emptyMap());
                        // test3 started but never finished
                        listener.testStarted(TestDescription.convertToIdentifier(test3));
                        // verify that the content of the testFile contains all expected tests
                        verifyTestFile(expectedTestsList.subList(1, expectedTestsList.size()));
                        return true;
                    }
                };
        setRunTestExpectations(secondRunAnswer);

        // test3 finished, test4 started but not finished.
        RunTestAnswer thirdRunAnswer =
                new RunTestAnswer() {
                    @Override
                    public Boolean answer(
                            IRemoteAndroidTestRunner runner, ITestRunListener listener) {
                        // test3 started and ended successfully
                        listener.testRunStarted(TEST_PACKAGE_VALUE, 4);
                        listener.testStarted(TestDescription.convertToIdentifier(test3));
                        listener.testEnded(
                                TestDescription.convertToIdentifier(test3), Collections.emptyMap());
                        listener.testRunEnded(1, Collections.emptyMap());
                        // test4 started but never finished
                        listener.testStarted(TestDescription.convertToIdentifier(test4));
                        // verify that the content of the testFile contains all expected tests
                        verifyTestFile(expectedTestsList.subList(2, expectedTestsList.size()));
                        return true;
                    }
                };
        setRunTestExpectations(thirdRunAnswer);

        mInstrumentationFileTest = new InstrumentationFileTest(mMockITest, testsList, false, 3) {
            @Override
            InstrumentationTest createInstrumentationTest() {
                return mMockITest;
            }
            @Override
            boolean pushFileToTestDevice(File file, String destinationPath)
                    throws DeviceNotAvailableException {
                // simulate successful push and store created file
                mTestFile = file;
                return true;
            }
            @Override
            void deleteTestFileFromDevice(String pathToFile) throws DeviceNotAvailableException {
                //ignore
            }
        };

        // First run:
        mMockListener.testRunStarted(TEST_PACKAGE_VALUE, 6);
        mMockListener.testStarted(EasyMock.eq(test1), EasyMock.anyLong());
        mMockListener.testEnded(
                EasyMock.eq(test1), EasyMock.anyLong(), EasyMock.eq(new HashMap<String, Metric>()));
        mMockListener.testRunEnded(1, new HashMap<String, Metric>());
        mMockListener.testStarted(EasyMock.eq(test2), EasyMock.anyLong());

        // Second run:
        mMockListener.testRunStarted(TEST_PACKAGE_VALUE, 5);
        mMockListener.testStarted(EasyMock.eq(test2), EasyMock.anyLong());
        mMockListener.testEnded(
                EasyMock.eq(test2), EasyMock.anyLong(), EasyMock.eq(new HashMap<String, Metric>()));
        mMockListener.testRunEnded(1, new HashMap<String, Metric>());
        mMockListener.testStarted(EasyMock.eq(test3), EasyMock.anyLong());

        // Third run:
        mMockListener.testRunStarted(TEST_PACKAGE_VALUE, 4);
        mMockListener.testStarted(EasyMock.eq(test3), EasyMock.anyLong());
        mMockListener.testEnded(
                EasyMock.eq(test3), EasyMock.anyLong(), EasyMock.eq(new HashMap<String, Metric>()));
        mMockListener.testRunEnded(1, new HashMap<String, Metric>());
        mMockListener.testStarted(EasyMock.eq(test4), EasyMock.anyLong());

        // MAX_ATTEMPTS is 3, so there will be no forth run.

        EasyMock.replay(mMockListener, mMockTestDevice);
        mInstrumentationFileTest.run(mMockListener);
        assertEquals(mMockTestDevice, mMockITest.getDevice());
    }

    /** Test re-run a test instrumentation when some methods are parameterized. */
    public void testRun_parameterized() throws DeviceNotAvailableException, ConfigurationException {
        final Collection<TestDescription> testsList = new ArrayList<>();
        final TestDescription test = new TestDescription("ClassFoo", "methodBar");
        final TestDescription test1 = new TestDescription("ClassFoo", "paramMethod[0]");
        final TestDescription test2 = new TestDescription("ClassFoo", "paramMethod[1]");
        testsList.add(test);
        testsList.add(test1);
        testsList.add(test2);

        // verify the mock listener is passed through to the runner, the first test pass
        RunTestAnswer runTestResponse =
                new RunTestAnswer() {
                    @Override
                    public Boolean answer(
                            IRemoteAndroidTestRunner runner, ITestRunListener listener) {
                        listener.testRunStarted(TEST_PACKAGE_VALUE, 3);
                        listener.testStarted(TestDescription.convertToIdentifier(test));
                        listener.testEnded(
                                TestDescription.convertToIdentifier(test), Collections.emptyMap());
                        listener.testStarted(TestDescription.convertToIdentifier(test1));
                        listener.testRunEnded(0, Collections.emptyMap());
                        return true;
                    }
                };
        setRunTestExpectations(runTestResponse);

        RunTestAnswer secondRunAnswer =
                new RunTestAnswer() {
                    @Override
                    public Boolean answer(
                            IRemoteAndroidTestRunner runner, ITestRunListener listener) {
                        // test2 started and ended successfully
                        listener.testRunStarted(TEST_PACKAGE_VALUE, 2);
                        listener.testStarted(TestDescription.convertToIdentifier(test1));
                        listener.testEnded(
                                TestDescription.convertToIdentifier(test1), Collections.emptyMap());
                        listener.testStarted(TestDescription.convertToIdentifier(test2));
                        listener.testEnded(
                                TestDescription.convertToIdentifier(test2), Collections.emptyMap());
                        listener.testRunEnded(1, Collections.emptyMap());
                        return true;
                    }
                };
        setRunTestExpectations(secondRunAnswer);

        mInstrumentationFileTest =
                new InstrumentationFileTest(mMockITest, testsList, true, -1) {
                    @Override
                    InstrumentationTest createInstrumentationTest() {
                        return mMockITest;
                    }

                    @Override
                    boolean pushFileToTestDevice(File file, String destinationPath)
                            throws DeviceNotAvailableException {
                        // simulate successful push and store created file
                        mTestFile = file;
                        // verify that the content of the testFile contains all expected tests
                        Collection<TestDescription> updatedList = new ArrayList<>();
                        updatedList.add(test);
                        updatedList.add(new TestDescription("ClassFoo", "paramMethod"));
                        verifyTestFile(updatedList);
                        return true;
                    }

                    @Override
                    void deleteTestFileFromDevice(String pathToFile)
                            throws DeviceNotAvailableException {
                        //ignore
                    }
                };

        // mock successful test run lifecycle for the first test
        mMockListener.testRunStarted(TEST_PACKAGE_VALUE, 3);
        mMockListener.testStarted(EasyMock.eq(test), EasyMock.anyLong());
        mMockListener.testEnded(
                EasyMock.eq(test), EasyMock.anyLong(), EasyMock.eq(new HashMap<String, Metric>()));
        mMockListener.testStarted(EasyMock.eq(test1), EasyMock.anyLong());
        mMockListener.testRunEnded(0, new HashMap<String, Metric>());

        // Second run:
        mMockListener.testRunStarted(TEST_PACKAGE_VALUE, 2);
        mMockListener.testStarted(EasyMock.eq(test1), EasyMock.anyLong());
        mMockListener.testEnded(
                EasyMock.eq(test1), EasyMock.anyLong(), EasyMock.eq(new HashMap<String, Metric>()));
        mMockListener.testStarted(EasyMock.eq(test2), EasyMock.anyLong());
        mMockListener.testEnded(
                EasyMock.eq(test2), EasyMock.anyLong(), EasyMock.eq(new HashMap<String, Metric>()));
        mMockListener.testRunEnded(1, new HashMap<String, Metric>());

        EasyMock.replay(mMockListener, mMockTestDevice);
        mInstrumentationFileTest.run(mMockListener);
        assertEquals(mMockTestDevice, mMockITest.getDevice());
    }

    /**
     * Helper class that verifies tetFile's content match the expected list of test to be run
     *
     * @param testsList list of test to be executed
     */
    private void verifyTestFile(Collection<TestDescription> testsList) {
        // fail if the file was never created
        assertNotNull(mTestFile);

        try (BufferedReader br = new BufferedReader(new FileReader(mTestFile))) {
            String line;
            while ((line = br.readLine()) != null) {
                String[] str = line.split("#");
                TestDescription test = new TestDescription(str[0], str[1]);
                assertTrue(String.format(
                        "Test with class name: %s and method name: %s does not exists",
                        test.getClassName(), test.getTestName()), testsList.contains(test));
            }
        } catch (IOException e) {
            // fail if the file is corrupt in any way
            fail("failed reading test file");
        }
    }

    /**
     * Helper class for providing an EasyMock {@link IAnswer} to a
     * {@link ITestDevice#runInstrumentationTests(IRemoteAndroidTestRunner, Collection)} call.
     */
    private static abstract class RunTestAnswer implements IAnswer<Boolean> {
        @Override
        public Boolean answer() throws Throwable {
            Object[] args = EasyMock.getCurrentArguments();
            return answer(
                    (IRemoteAndroidTestRunner) args[0],
                    new TestRunToTestInvocationForwarder((ITestLifeCycleReceiver) args[1]));
        }

        public abstract Boolean answer(IRemoteAndroidTestRunner runner,
                ITestRunListener listener) throws DeviceNotAvailableException;
    }

    private void setRunTestExpectations(RunTestAnswer runTestResponse)
            throws DeviceNotAvailableException {

        EasyMock.expect(
                        mMockTestDevice.runInstrumentationTests(
                                (IRemoteAndroidTestRunner) EasyMock.anyObject(),
                                (ITestLifeCycleReceiver) EasyMock.anyObject()))
                .andAnswer(runTestResponse);
    }
}
