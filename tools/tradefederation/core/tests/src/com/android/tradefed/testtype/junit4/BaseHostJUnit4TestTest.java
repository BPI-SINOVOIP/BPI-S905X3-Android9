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
package com.android.tradefed.testtype.junit4;

import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import com.android.ddmlib.IDevice;
import com.android.ddmlib.testrunner.IRemoteAndroidTestRunner;
import com.android.ddmlib.testrunner.RemoteAndroidTestRunner;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.command.remote.DeviceDescriptor;
import com.android.tradefed.config.ConfigurationDef;
import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.device.StubDevice;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.invoker.InvocationContext;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.CollectingTestListener;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.targetprep.TargetSetupError;
import com.android.tradefed.targetprep.suite.SuiteApkInstaller;
import com.android.tradefed.testtype.Abi;
import com.android.tradefed.testtype.DeviceJUnit4ClassRunner;
import com.android.tradefed.testtype.HostTest;
import com.android.tradefed.util.FileUtil;

import org.easymock.EasyMock;
import org.junit.Assert;
import org.junit.AssumptionViolatedException;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.mockito.Mockito;

import java.io.File;
import java.util.HashMap;
import java.util.Map;

/** Unit tests for {@link BaseHostJUnit4Test}. */
@RunWith(JUnit4.class)
public class BaseHostJUnit4TestTest {

    /** An implementation of the base class for testing purpose. */
    @RunWith(DeviceJUnit4ClassRunner.class)
    public static class TestableHostJUnit4Test extends BaseHostJUnit4Test {
        @Test
        public void testPass() {
            Assert.assertNotNull(getDevice());
            Assert.assertNotNull(getBuild());
        }

        @Override
        CollectingTestListener createListener() {
            CollectingTestListener listener = new CollectingTestListener();
            listener.testRunStarted("testRun", 1);
            TestDescription tid = new TestDescription("class", "test1");
            listener.testStarted(tid);
            listener.testEnded(tid, new HashMap<String, Metric>());
            listener.testRunEnded(500l, new HashMap<String, Metric>());
            return listener;
        }
    }

    /**
     * An implementation of the base class that simulate a crashed instrumentation from an host test
     * run.
     */
    @RunWith(DeviceJUnit4ClassRunner.class)
    public static class FailureHostJUnit4Test extends BaseHostJUnit4Test {
        @Test
        public void testOne() {
            Assert.assertNotNull(getDevice());
            Assert.assertNotNull(getBuild());
        }

        @Override
        CollectingTestListener createListener() {
            CollectingTestListener listener = new CollectingTestListener();
            listener.testRunStarted("testRun", 1);
            listener.testRunFailed("instrumentation crashed");
            listener.testRunEnded(50L, new HashMap<String, Metric>());
            return listener;
        }
    }

    private static final String CLASSNAME =
            "com.android.tradefed.testtype.junit4.BaseHostJUnit4TestTest$TestableHostJUnit4Test";

    private ITestInvocationListener mMockListener;
    private IBuildInfo mMockBuild;
    private ITestDevice mMockDevice;
    private IInvocationContext mMockContext;
    private HostTest mHostTest;

    @Before
    public void setUp() throws Exception {
        mMockListener = EasyMock.createMock(ITestInvocationListener.class);
        mMockBuild = EasyMock.createMock(IBuildInfo.class);
        mMockDevice = EasyMock.createMock(ITestDevice.class);
        mMockContext = new InvocationContext();
        mMockContext.addAllocatedDevice(ConfigurationDef.DEFAULT_DEVICE_NAME, mMockDevice);
        mMockContext.addDeviceBuildInfo(ConfigurationDef.DEFAULT_DEVICE_NAME, mMockBuild);

        mHostTest = new HostTest();
        mHostTest.setBuild(mMockBuild);
        mHostTest.setDevice(mMockDevice);
        mHostTest.setInvocationContext(mMockContext);
        OptionSetter setter = new OptionSetter(mHostTest);
        // Disable pretty logging for testing
        setter.setOptionValue("enable-pretty-logs", "false");
    }

    /** Test that we are able to run the test as a JUnit4. */
    @Test
    public void testSimpleRun() throws Exception {
        OptionSetter setter = new OptionSetter(mHostTest);
        setter.setOptionValue("class", CLASSNAME);
        mMockListener.testRunStarted(EasyMock.anyObject(), EasyMock.eq(1));
        TestDescription tid = new TestDescription(CLASSNAME, "testPass");
        mMockListener.testStarted(tid);
        mMockListener.testEnded(tid, new HashMap<String, Metric>());
        mMockListener.testRunEnded(
                EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.replay(mMockListener, mMockBuild, mMockDevice);
        mHostTest.run(mMockListener);
        EasyMock.verify(mMockListener, mMockBuild, mMockDevice);
    }

    /**
     * Test that {@link BaseHostJUnit4Test#runDeviceTests(String, String)} properly trigger an
     * instrumentation run.
     */
    @Test
    public void testRunDeviceTests() throws Exception {
        TestableHostJUnit4Test test = new TestableHostJUnit4Test();
        test.setDevice(mMockDevice);
        test.setBuild(mMockBuild);
        test.setInvocationContext(mMockContext);
        EasyMock.expect(mMockDevice.getIDevice()).andReturn(new StubDevice("serial"));
        EasyMock.expect(
                        mMockDevice.runInstrumentationTests(
                                (IRemoteAndroidTestRunner) EasyMock.anyObject(),
                                (ITestInvocationListener) EasyMock.anyObject()))
                .andReturn(true);
        EasyMock.replay(mMockBuild, mMockDevice);
        try {
            test.runDeviceTests("com.package", "testClass");
        } catch (AssumptionViolatedException e) {
            // Ensure that the Assume logic in the test does not make a false pass for the unit test
            fail("Should not have thrown an Assume exception.");
        }
        EasyMock.verify(mMockBuild, mMockDevice);
    }

    /** Test that when running an instrumentation, the abi is properly passed. */
    @Test
    public void testRunDeviceTests_abi() throws Exception {
        RemoteAndroidTestRunner runner = Mockito.mock(RemoteAndroidTestRunner.class);
        TestableHostJUnit4Test test =
                new TestableHostJUnit4Test() {
                    @Override
                    RemoteAndroidTestRunner createTestRunner(
                            String packageName, String runnerName, IDevice device) {
                        return runner;
                    }
                };
        test.setDevice(mMockDevice);
        test.setBuild(mMockBuild);
        test.setInvocationContext(mMockContext);
        test.setAbi(new Abi("arm", "32"));
        EasyMock.expect(mMockDevice.getIDevice()).andReturn(new StubDevice("serial"));
        EasyMock.expect(
                        mMockDevice.runInstrumentationTests(
                                (IRemoteAndroidTestRunner) EasyMock.anyObject(),
                                (ITestInvocationListener) EasyMock.anyObject()))
                .andReturn(true);
        EasyMock.replay(mMockBuild, mMockDevice);
        try {
            test.runDeviceTests("com.package", "testClass");
        } catch (AssumptionViolatedException e) {
            // Ensure that the Assume logic in the test does not make a false pass for the unit test
            fail("Should not have thrown an Assume exception.");
        }
        EasyMock.verify(mMockBuild, mMockDevice);
        // Verify that the runner options were properly set.
        Mockito.verify(runner).setRunOptions("--abi arm");
    }

    /**
     * Test that {@link BaseHostJUnit4Test#runDeviceTests(String, String)} properly trigger an
     * instrumentation run as a user.
     */
    @Test
    public void testRunDeviceTests_asUser() throws Exception {
        TestableHostJUnit4Test test = new TestableHostJUnit4Test();
        test.setDevice(mMockDevice);
        test.setBuild(mMockBuild);
        test.setInvocationContext(mMockContext);
        EasyMock.expect(mMockDevice.getIDevice()).andReturn(new StubDevice("serial"));
        EasyMock.expect(
                        mMockDevice.runInstrumentationTestsAsUser(
                                (IRemoteAndroidTestRunner) EasyMock.anyObject(),
                                EasyMock.eq(0),
                                (ITestInvocationListener) EasyMock.anyObject()))
                .andReturn(true);
        EasyMock.replay(mMockBuild, mMockDevice);
        try {
            test.runDeviceTests("package", "class", 0, null);
        } catch (AssumptionViolatedException e) {
            // Ensure that the Assume logic in the test does not make a false pass for the unit test
            fail("Should not have thrown an Assume exception.");
        }
        EasyMock.verify(mMockBuild, mMockDevice);
    }

    /**
     * Test that {@link BaseHostJUnit4Test#runDeviceTests(DeviceTestRunOptions)} properly trigger an
     * instrumentation run.
     */
    @Test
    public void testRunDeviceTestsWithOptions() throws Exception {
        TestableHostJUnit4Test test = new TestableHostJUnit4Test();
        test.setDevice(mMockDevice);
        test.setBuild(mMockBuild);
        test.setInvocationContext(mMockContext);
        EasyMock.expect(mMockDevice.getIDevice()).andReturn(new StubDevice("serial"));
        EasyMock.expect(
                        mMockDevice.runInstrumentationTests(
                                (IRemoteAndroidTestRunner) EasyMock.anyObject(),
                                (ITestInvocationListener) EasyMock.anyObject()))
                .andReturn(true);
        EasyMock.replay(mMockBuild, mMockDevice);
        try {
            test.runDeviceTests(
                    new DeviceTestRunOptions("com.package").setTestClassName("testClass"));
        } catch (AssumptionViolatedException e) {
            // Ensure that the Assume logic in the test does not make a false pass for the unit test
            fail("Should not have thrown an Assume exception.");
        }
        EasyMock.verify(mMockBuild, mMockDevice);
    }

    /**
     * Test that if the instrumentation crash directly we report it as a failure and not an
     * AssumptionFailure (which would improperly categorize the failure).
     */
    @Test
    public void testRunDeviceTests_crashedInstrumentation() throws Exception {
        FailureHostJUnit4Test test = new FailureHostJUnit4Test();
        test.setDevice(mMockDevice);
        test.setBuild(mMockBuild);
        test.setInvocationContext(mMockContext);
        EasyMock.expect(mMockDevice.getIDevice()).andReturn(new StubDevice("serial"));
        EasyMock.expect(
                        mMockDevice.runInstrumentationTests(
                                (IRemoteAndroidTestRunner) EasyMock.anyObject(),
                                (ITestInvocationListener) EasyMock.anyObject()))
                .andReturn(true);
        EasyMock.replay(mMockBuild, mMockDevice);
        try {
            test.runDeviceTests("com.package", "testClass");
        } catch (AssumptionViolatedException e) {
            // Ensure that the Assume logic in the test does not make a false pass for the unit test
            fail("Should not have thrown an Assume exception.");
        } catch (AssertionError expected) {
            assertTrue(expected.getMessage().contains("instrumentation crashed"));
        }
        EasyMock.verify(mMockBuild, mMockDevice);
    }

    /** An implementation of the base class for testing purpose of installation of apk. */
    @RunWith(DeviceJUnit4ClassRunner.class)
    public static class InstallApkHostJUnit4Test extends BaseHostJUnit4Test {
        @Test
        public void testInstall() throws Exception {
            installPackage("apkFileName");
        }

        @Override
        SuiteApkInstaller createSuiteApkInstaller() {
            return new SuiteApkInstaller() {
                @Override
                protected String parsePackageName(
                        File testAppFile, DeviceDescriptor deviceDescriptor)
                        throws TargetSetupError {
                    return "fakepackage";
                }
            };
        }
    }

    /**
     * Test that when running a test that use the {@link BaseHostJUnit4Test#installPackage(String,
     * String...)} the package is properly auto uninstalled.
     */
    @Test
    public void testInstallUninstall() throws Exception {
        File fakeTestsDir = FileUtil.createTempDir("fake-base-host-dir");
        try {
            File apk = new File(fakeTestsDir, "apkFileName");
            apk.createNewFile();
            HostTest test = new HostTest();
            test.setBuild(mMockBuild);
            test.setDevice(mMockDevice);
            test.setInvocationContext(mMockContext);
            OptionSetter setter = new OptionSetter(test);
            // Disable pretty logging for testing
            setter.setOptionValue("enable-pretty-logs", "false");
            setter.setOptionValue("class", InstallApkHostJUnit4Test.class.getName());
            mMockListener.testRunStarted(InstallApkHostJUnit4Test.class.getName(), 1);
            TestDescription description =
                    new TestDescription(InstallApkHostJUnit4Test.class.getName(), "testInstall");
            mMockListener.testStarted(description);
            Map<String, String> properties = new HashMap<>();
            properties.put("ROOT_DIR", fakeTestsDir.getAbsolutePath());
            EasyMock.expect(mMockBuild.getBuildAttributes()).andReturn(properties).times(2);
            EasyMock.expect(mMockDevice.getDeviceDescriptor()).andReturn(null);

            EasyMock.expect(mMockDevice.installPackage(apk, true)).andReturn(null);
            // Ensure that the auto-uninstall is triggered
            EasyMock.expect(mMockDevice.uninstallPackage("fakepackage")).andReturn(null);
            mMockListener.testEnded(description, new HashMap<String, Metric>());
            mMockListener.testRunEnded(
                    EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());

            EasyMock.replay(mMockBuild, mMockDevice, mMockListener);
            test.run(mMockListener);
            EasyMock.verify(mMockBuild, mMockDevice, mMockListener);
        } finally {
            FileUtil.recursiveDelete(fakeTestsDir);
        }
    }
}
