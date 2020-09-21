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

import static org.junit.Assert.assertTrue;

import com.android.ddmlib.IDevice;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.build.IFolderBuildInfo;
import com.android.tradefed.command.CommandOptions;
import com.android.tradefed.config.GlobalConfiguration;
import com.android.tradefed.config.IConfiguration;
import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.device.NullDevice;
import com.android.tradefed.device.StubDevice;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.FileInputStreamSource;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;
import com.android.tradefed.util.IRunUtil;
import com.android.tradefed.util.IRunUtil.EnvPriority;

import org.easymock.EasyMock;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.io.File;
import java.io.FileOutputStream;
import java.util.HashMap;
import java.util.Map;

/** Unit tests for {@link VersionedTfLauncher}. */
@RunWith(JUnit4.class)
public class VersionedTfLauncherTest {

    private static final String FAKE_SERIAL = "FAKE_SERIAL";
    private static final String CONFIG_NAME = "FAKE_CONFIG";
    private static final String TF_COMMAND_LINE_TEMPLATE = "--template:map";
    private static final String TF_COMMAND_LINE_TEST = "test=tf/fake";
    // Test option value with empty spaces should be parsed correctly.
    private static final String TF_COMMAND_LINE_OPTION = "--option";
    private static final String TF_COMMAND_LINE_OPTION_VALUE = "value1 value2";
    private static final String TF_COMMAND_LINE_OPTION_VALUE_QUOTED =
            ("\\\"" + TF_COMMAND_LINE_OPTION_VALUE + "\\\"");
    private static final String TF_COMMAND_LINE =
            (TF_COMMAND_LINE_TEMPLATE + " " + TF_COMMAND_LINE_TEST + " " + TF_COMMAND_LINE_OPTION +
             " " + TF_COMMAND_LINE_OPTION_VALUE_QUOTED);
    private static final String ADDITIONAL_TEST_ZIP = "/tmp/tests.zip";

    private VersionedTfLauncher mVersionedTfLauncher;
    private ITestInvocationListener mMockListener;
    private IRunUtil mMockRunUtil;
    private ITestDevice mMockTestDevice;
    private IDevice mMockIDevice;
    private IFolderBuildInfo mMockBuildInfo;
    private IConfiguration mMockConfig;

    @Before
    public void setUp() throws Exception {
        mMockListener = EasyMock.createMock(ITestInvocationListener.class);
        mMockRunUtil = EasyMock.createMock(IRunUtil.class);
        mMockBuildInfo = EasyMock.createMock(IFolderBuildInfo.class);
        mMockTestDevice = EasyMock.createMock(ITestDevice.class);
        mMockConfig = EasyMock.createMock(IConfiguration.class);

        mVersionedTfLauncher = new VersionedTfLauncher();
        mVersionedTfLauncher.setRunUtil(mMockRunUtil);
        mVersionedTfLauncher.setBuild(mMockBuildInfo);
        mVersionedTfLauncher.setEventStreaming(false);
        mVersionedTfLauncher.setConfiguration(mMockConfig);

        OptionSetter setter = new OptionSetter(mVersionedTfLauncher);
        setter.setOptionValue("config-name", CONFIG_NAME);
        setter.setOptionValue("tf-command-line", TF_COMMAND_LINE);
        setter.setOptionValue("inject-invocation-data", "true");

        try {
            GlobalConfiguration.createGlobalConfiguration(new String[] {});
        } catch (IllegalStateException e) {
            // ignore re-init.
        }
    }

    /**
     * Test {@link VersionedTfLauncher#run(ITestInvocationListener)} for test with a single device
     */
    @Test
    public void testRun_singleDevice() {
        mMockIDevice = EasyMock.createMock(IDevice.class);

        CommandResult cr = new CommandResult(CommandStatus.SUCCESS);
        mMockRunUtil.unsetEnvVariable(SubprocessTfLauncher.TF_GLOBAL_CONFIG);
        mMockRunUtil.setEnvVariablePriority(EnvPriority.SET);
        mMockRunUtil.setEnvVariable(
                EasyMock.eq(SubprocessTfLauncher.TF_GLOBAL_CONFIG), (String) EasyMock.anyObject());

        EasyMock.expect(
                        mMockRunUtil.runTimedCmd(
                                EasyMock.anyLong(),
                                (FileOutputStream) EasyMock.anyObject(),
                                (FileOutputStream) EasyMock.anyObject(),
                                EasyMock.eq("java"),
                                (String) EasyMock.anyObject(),
                                EasyMock.eq("--add-opens=java.base/java.nio=ALL-UNNAMED"),
                                EasyMock.eq("-cp"),
                                (String) EasyMock.anyObject(),
                                EasyMock.eq("com.android.tradefed.command.CommandRunner"),
                                EasyMock.eq(CONFIG_NAME),
                                EasyMock.eq(TF_COMMAND_LINE_TEMPLATE),
                                EasyMock.eq(TF_COMMAND_LINE_TEST),
                                EasyMock.eq(TF_COMMAND_LINE_OPTION),
                                EasyMock.eq(TF_COMMAND_LINE_OPTION_VALUE),
                                EasyMock.eq("--serial"),
                                EasyMock.eq(FAKE_SERIAL),
                                EasyMock.eq("--additional-tests-zip"),
                                EasyMock.eq(ADDITIONAL_TEST_ZIP),
                                EasyMock.eq("--subprocess-report-file"),
                                (String) EasyMock.anyObject()))
                .andReturn(cr);
        Map<ITestDevice, IBuildInfo> deviceInfos = new HashMap<ITestDevice, IBuildInfo>();
        deviceInfos.put(mMockTestDevice, null);
        mVersionedTfLauncher.setDeviceInfos(deviceInfos);
        EasyMock.expect(mMockBuildInfo.getRootDir()).andReturn(new File(""));
        EasyMock.expect(mMockBuildInfo.getBuildId()).andReturn("FAKEID").times(2);
        EasyMock.expect(mMockBuildInfo.getFile("general-tests.zip"))
                .andReturn(new File(ADDITIONAL_TEST_ZIP));
        EasyMock.expect(mMockTestDevice.getIDevice()).andReturn(mMockIDevice).times(2);
        EasyMock.expect(mMockTestDevice.getSerialNumber()).andReturn(FAKE_SERIAL).times(1);
        mMockListener.testLog((String)EasyMock.anyObject(), (LogDataType)EasyMock.anyObject(),
                (FileInputStreamSource)EasyMock.anyObject());
        EasyMock.expectLastCall().times(3);
        mMockListener.testRunStarted("StdErr", 1);
        mMockListener.testStarted((TestDescription) EasyMock.anyObject());
        mMockListener.testEnded(
                (TestDescription) EasyMock.anyObject(), EasyMock.eq(new HashMap<String, Metric>()));
        mMockListener.testRunEnded(0, new HashMap<String, Metric>());

        EasyMock.expect(mMockConfig.getCommandOptions()).andReturn(new CommandOptions());
        EasyMock.replay(mMockTestDevice, mMockBuildInfo, mMockRunUtil, mMockListener, mMockConfig);
        mVersionedTfLauncher.run(mMockListener);
        EasyMock.verify(mMockTestDevice, mMockBuildInfo, mMockRunUtil, mMockListener, mMockConfig);
    }

    /**
     * Test {@link VersionedTfLauncher#run(ITestInvocationListener)} for test with a null device
     */
    @Test
    public void testRun_nullDevice() {
        mMockIDevice = new NullDevice("null-device-1");

        CommandResult cr = new CommandResult(CommandStatus.SUCCESS);
        mMockRunUtil.unsetEnvVariable(SubprocessTfLauncher.TF_GLOBAL_CONFIG);
        mMockRunUtil.setEnvVariablePriority(EnvPriority.SET);
        mMockRunUtil.setEnvVariable(
                EasyMock.eq(SubprocessTfLauncher.TF_GLOBAL_CONFIG), (String) EasyMock.anyObject());

        EasyMock.expect(
                        mMockRunUtil.runTimedCmd(
                                EasyMock.anyLong(),
                                (FileOutputStream) EasyMock.anyObject(),
                                (FileOutputStream) EasyMock.anyObject(),
                                EasyMock.eq("java"),
                                (String) EasyMock.anyObject(),
                                EasyMock.eq("--add-opens=java.base/java.nio=ALL-UNNAMED"),
                                EasyMock.eq("-cp"),
                                (String) EasyMock.anyObject(),
                                EasyMock.eq("com.android.tradefed.command.CommandRunner"),
                                EasyMock.eq(CONFIG_NAME),
                                EasyMock.eq(TF_COMMAND_LINE_TEMPLATE),
                                EasyMock.eq(TF_COMMAND_LINE_TEST),
                                EasyMock.eq(TF_COMMAND_LINE_OPTION),
                                EasyMock.eq(TF_COMMAND_LINE_OPTION_VALUE),
                                EasyMock.eq("--null-device"),
                                EasyMock.eq("--subprocess-report-file"),
                                (String) EasyMock.anyObject()))
                .andReturn(cr);
        Map<ITestDevice, IBuildInfo> deviceInfos = new HashMap<ITestDevice, IBuildInfo>();
        deviceInfos.put(mMockTestDevice, null);
        mVersionedTfLauncher.setDeviceInfos(deviceInfos);
        EasyMock.expect(mMockBuildInfo.getRootDir()).andReturn(new File(""));
        EasyMock.expect(mMockBuildInfo.getBuildId()).andReturn("FAKEID").times(2);
        EasyMock.expect(mMockBuildInfo.getFile("general-tests.zip")).andReturn(null);
        EasyMock.expect(mMockTestDevice.getIDevice()).andReturn(mMockIDevice).times(1);
        mMockListener.testLog((String)EasyMock.anyObject(), (LogDataType)EasyMock.anyObject(),
                (FileInputStreamSource)EasyMock.anyObject());
        EasyMock.expectLastCall().times(3);
        mMockListener.testRunStarted("StdErr", 1);
        mMockListener.testStarted((TestDescription) EasyMock.anyObject());
        mMockListener.testEnded(
                (TestDescription) EasyMock.anyObject(), EasyMock.eq(new HashMap<String, Metric>()));
        mMockListener.testRunEnded(0, new HashMap<String, Metric>());

        EasyMock.expect(mMockConfig.getCommandOptions()).andReturn(new CommandOptions());
        EasyMock.replay(mMockTestDevice, mMockBuildInfo, mMockRunUtil, mMockListener, mMockConfig);
        mVersionedTfLauncher.run(mMockListener);
        EasyMock.verify(mMockTestDevice, mMockBuildInfo, mMockRunUtil, mMockListener, mMockConfig);
    }

    /** Test {@link VersionedTfLauncher#run(ITestInvocationListener)} for test with a StubDevice. */
    @Test
    public void testRun_DeviceNoPreSetup() {
        CommandResult cr = new CommandResult(CommandStatus.SUCCESS);
        mMockRunUtil.unsetEnvVariable(SubprocessTfLauncher.TF_GLOBAL_CONFIG);
        mMockRunUtil.setEnvVariablePriority(EnvPriority.SET);
        mMockRunUtil.setEnvVariable(
                EasyMock.eq(SubprocessTfLauncher.TF_GLOBAL_CONFIG), (String) EasyMock.anyObject());

        EasyMock.expect(
                        mMockRunUtil.runTimedCmd(
                                EasyMock.anyLong(),
                                (FileOutputStream) EasyMock.anyObject(),
                                (FileOutputStream) EasyMock.anyObject(),
                                EasyMock.eq("java"),
                                (String) EasyMock.anyObject(),
                                EasyMock.eq("--add-opens=java.base/java.nio=ALL-UNNAMED"),
                                EasyMock.eq("-cp"),
                                (String) EasyMock.anyObject(),
                                EasyMock.eq("com.android.tradefed.command.CommandRunner"),
                                EasyMock.eq(CONFIG_NAME),
                                EasyMock.eq(TF_COMMAND_LINE_TEMPLATE),
                                EasyMock.eq(TF_COMMAND_LINE_TEST),
                                EasyMock.eq(TF_COMMAND_LINE_OPTION),
                                EasyMock.eq(TF_COMMAND_LINE_OPTION_VALUE),
                                EasyMock.eq("--additional-tests-zip"),
                                EasyMock.eq(ADDITIONAL_TEST_ZIP),
                                EasyMock.eq("--subprocess-report-file"),
                                (String) EasyMock.anyObject()))
                .andReturn(cr);
        Map<ITestDevice, IBuildInfo> deviceInfos = new HashMap<ITestDevice, IBuildInfo>();
        deviceInfos.put(mMockTestDevice, null);
        mVersionedTfLauncher.setDeviceInfos(deviceInfos);
        EasyMock.expect(mMockBuildInfo.getRootDir()).andReturn(new File(""));
        EasyMock.expect(mMockBuildInfo.getBuildId()).andReturn("FAKEID").times(2);
        EasyMock.expect(mMockBuildInfo.getFile("general-tests.zip"))
                .andReturn(new File(ADDITIONAL_TEST_ZIP));
        EasyMock.expect(mMockTestDevice.getIDevice()).andReturn(new StubDevice("serial1")).times(2);
        mMockListener.testLog(
                (String) EasyMock.anyObject(),
                (LogDataType) EasyMock.anyObject(),
                (FileInputStreamSource) EasyMock.anyObject());
        EasyMock.expectLastCall().times(3);
        mMockListener.testRunStarted("StdErr", 1);
        mMockListener.testStarted((TestDescription) EasyMock.anyObject());
        mMockListener.testEnded(
                (TestDescription) EasyMock.anyObject(), EasyMock.eq(new HashMap<String, Metric>()));
        mMockListener.testRunEnded(0, new HashMap<String, Metric>());

        EasyMock.expect(mMockConfig.getCommandOptions()).andReturn(new CommandOptions());
        EasyMock.replay(mMockTestDevice, mMockBuildInfo, mMockRunUtil, mMockListener, mMockConfig);
        mVersionedTfLauncher.run(mMockListener);
        EasyMock.verify(mMockTestDevice, mMockBuildInfo, mMockRunUtil, mMockListener, mMockConfig);
    }

    /**
     * Test that when a test is sharded, the instance of the implementation is used and options are
     * passed to the shard test.
     */
    @Test
    public void testGetTestShard() {
        IRemoteTest test = mVersionedTfLauncher.getTestShard(2, 1);
        assertTrue(test instanceof VersionedTfLauncher);

        VersionedTfLauncher shardedTest = (VersionedTfLauncher) test;

        shardedTest.setRunUtil(mMockRunUtil);
        shardedTest.setBuild(mMockBuildInfo);
        shardedTest.setEventStreaming(false);
        shardedTest.setConfiguration(mMockConfig);

        mMockIDevice = EasyMock.createMock(IDevice.class);

        CommandResult cr = new CommandResult(CommandStatus.SUCCESS);
        mMockRunUtil.unsetEnvVariable(SubprocessTfLauncher.TF_GLOBAL_CONFIG);
        mMockRunUtil.setEnvVariablePriority(EnvPriority.SET);
        mMockRunUtil.setEnvVariable(
                EasyMock.eq(SubprocessTfLauncher.TF_GLOBAL_CONFIG), (String) EasyMock.anyObject());

        EasyMock.expect(
                        mMockRunUtil.runTimedCmd(
                                EasyMock.anyLong(),
                                (FileOutputStream) EasyMock.anyObject(),
                                (FileOutputStream) EasyMock.anyObject(),
                                EasyMock.eq("java"),
                                (String) EasyMock.anyObject(),
                                EasyMock.eq("--add-opens=java.base/java.nio=ALL-UNNAMED"),
                                EasyMock.eq("-cp"),
                                (String) EasyMock.anyObject(),
                                EasyMock.eq("com.android.tradefed.command.CommandRunner"),
                                EasyMock.eq(CONFIG_NAME),
                                EasyMock.eq(TF_COMMAND_LINE_TEMPLATE),
                                EasyMock.eq(TF_COMMAND_LINE_TEST),
                                EasyMock.eq(TF_COMMAND_LINE_OPTION),
                                EasyMock.eq(TF_COMMAND_LINE_OPTION_VALUE),
                                EasyMock.eq("--serial"),
                                EasyMock.eq(FAKE_SERIAL),
                                EasyMock.eq("--shard-count"),
                                EasyMock.eq("2"),
                                EasyMock.eq("--shard-index"),
                                EasyMock.eq("1"),
                                EasyMock.eq("--subprocess-report-file"),
                                (String) EasyMock.anyObject()))
                .andReturn(cr);
        Map<ITestDevice, IBuildInfo> deviceInfos = new HashMap<ITestDevice, IBuildInfo>();
        deviceInfos.put(mMockTestDevice, null);
        shardedTest.setDeviceInfos(deviceInfos);
        EasyMock.expect(mMockBuildInfo.getRootDir()).andReturn(new File(""));
        EasyMock.expect(mMockBuildInfo.getBuildId()).andReturn("FAKEID").times(2);
        EasyMock.expect(mMockBuildInfo.getFile("general-tests.zip")).andReturn(null);
        EasyMock.expect(mMockTestDevice.getIDevice()).andReturn(mMockIDevice).times(2);
        EasyMock.expect(mMockTestDevice.getSerialNumber()).andReturn(FAKE_SERIAL).times(1);
        mMockListener.testLog(
                (String) EasyMock.anyObject(),
                (LogDataType) EasyMock.anyObject(),
                (FileInputStreamSource) EasyMock.anyObject());
        EasyMock.expectLastCall().times(3);
        mMockListener.testRunStarted("StdErr", 1);
        mMockListener.testStarted((TestDescription) EasyMock.anyObject());
        mMockListener.testEnded(
                (TestDescription) EasyMock.anyObject(), EasyMock.eq(new HashMap<String, Metric>()));
        mMockListener.testRunEnded(0, new HashMap<String, Metric>());
        EasyMock.expect(mMockConfig.getCommandOptions()).andReturn(new CommandOptions());
        EasyMock.replay(mMockTestDevice, mMockBuildInfo, mMockRunUtil, mMockListener, mMockConfig);
        shardedTest.run(mMockListener);
        EasyMock.verify(mMockTestDevice, mMockBuildInfo, mMockRunUtil, mMockListener, mMockConfig);
    }
}
