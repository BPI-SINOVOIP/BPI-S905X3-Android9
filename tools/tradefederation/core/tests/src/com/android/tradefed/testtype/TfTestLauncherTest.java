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

import com.android.tradefed.build.IFolderBuildInfo;
import com.android.tradefed.config.IConfiguration;
import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.FileInputStreamSource;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.IRunUtil;
import com.android.tradefed.util.IRunUtil.EnvPriority;
import com.android.tradefed.util.SystemUtil.EnvVariable;

import org.easymock.EasyMock;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.mockito.Mockito;

import java.io.File;
import java.io.FileOutputStream;
import java.util.HashMap;

/** Unit tests for {@link TfTestLauncher} */
@RunWith(JUnit4.class)
public class TfTestLauncherTest {

    private static final String CONFIG_NAME = "FAKE_CONFIG";
    private static final String TEST_TAG = "FAKE_TAG";
    private static final String BUILD_BRANCH = "FAKE_BRANCH";
    private static final String BUILD_ID = "FAKE_BUILD_ID";
    private static final String BUILD_FLAVOR = "FAKE_FLAVOR";
    private static final String SUB_GLOBAL_CONFIG = "FAKE_GLOBAL_CONFIG";

    private TfTestLauncher mTfTestLauncher;
    private ITestInvocationListener mMockListener;
    private IRunUtil mMockRunUtil;
    private IFolderBuildInfo mMockBuildInfo;
    private IConfiguration mMockConfig;

    @Before
    public void setUp() throws Exception {
        mMockListener = EasyMock.createMock(ITestInvocationListener.class);
        mMockRunUtil = EasyMock.createMock(IRunUtil.class);
        mMockBuildInfo = EasyMock.createMock(IFolderBuildInfo.class);
        mMockConfig = EasyMock.createMock(IConfiguration.class);

        mTfTestLauncher = new TfTestLauncher();
        mTfTestLauncher.setRunUtil(mMockRunUtil);
        mTfTestLauncher.setBuild(mMockBuildInfo);
        mTfTestLauncher.setEventStreaming(false);
        mTfTestLauncher.setConfiguration(mMockConfig);

        OptionSetter setter = new OptionSetter(mTfTestLauncher);
        setter.setOptionValue("config-name", CONFIG_NAME);
        setter.setOptionValue("sub-global-config", SUB_GLOBAL_CONFIG);
    }

    /** Test {@link TfTestLauncher#run(ITestInvocationListener)} */
    @Test
    public void testRun() {
        CommandResult cr = new CommandResult(CommandStatus.SUCCESS);
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
                                EasyMock.eq("-n"),
                                EasyMock.eq("--test-tag"),
                                EasyMock.eq(TEST_TAG),
                                EasyMock.eq("--build-id"),
                                EasyMock.eq(BUILD_ID),
                                EasyMock.eq("--branch"),
                                EasyMock.eq(BUILD_BRANCH),
                                EasyMock.eq("--build-flavor"),
                                EasyMock.eq(BUILD_FLAVOR),
                                EasyMock.eq("--subprocess-report-file"),
                                (String) EasyMock.anyObject()))
                .andReturn(cr);

        mMockRunUtil.unsetEnvVariable(SubprocessTfLauncher.TF_GLOBAL_CONFIG);
        mMockRunUtil.unsetEnvVariable(EnvVariable.ANDROID_HOST_OUT_TESTCASES.name());
        mMockRunUtil.unsetEnvVariable(EnvVariable.ANDROID_TARGET_OUT_TESTCASES.name());
        mMockRunUtil.setEnvVariablePriority(EnvPriority.SET);
        mMockRunUtil.setEnvVariable(SubprocessTfLauncher.TF_GLOBAL_CONFIG, SUB_GLOBAL_CONFIG);

        EasyMock.expect(mMockBuildInfo.getTestTag()).andReturn(TEST_TAG);
        EasyMock.expect(mMockBuildInfo.getBuildBranch()).andReturn(BUILD_BRANCH).times(2);
        EasyMock.expect(mMockBuildInfo.getBuildFlavor()).andReturn(BUILD_FLAVOR).times(2);

        EasyMock.expect(mMockBuildInfo.getRootDir()).andReturn(new File(""));
        EasyMock.expect(mMockBuildInfo.getBuildId()).andReturn(BUILD_ID).times(3);
        mMockListener.testLog((String)EasyMock.anyObject(), (LogDataType)EasyMock.anyObject(),
                (FileInputStreamSource)EasyMock.anyObject());
        EasyMock.expectLastCall().times(3);

        mMockListener.testRunStarted("temporaryFiles", 1);
        mMockListener.testRunStarted("StdErr", 1);
        for (int i = 0; i < 2; i++) {
            mMockListener.testStarted((TestDescription) EasyMock.anyObject());
            mMockListener.testEnded(
                    (TestDescription) EasyMock.anyObject(),
                    EasyMock.eq(new HashMap<String, Metric>()));
            mMockListener.testRunEnded(0, new HashMap<String, Metric>());
        }
        mMockListener.testRunStarted("elapsed-time", 1);
        mMockListener.testStarted(EasyMock.anyObject());
        mMockListener.testEnded(
                EasyMock.anyObject(), (HashMap<String, Metric>) EasyMock.anyObject());
        mMockListener.testRunEnded(
                EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());

        EasyMock.replay(mMockBuildInfo, mMockRunUtil, mMockListener, mMockConfig);
        mTfTestLauncher.run(mMockListener);
        EasyMock.verify(mMockBuildInfo, mMockRunUtil, mMockListener, mMockConfig);
    }

    /**
     * Test {@link TfTestLauncher#testTmpDirClean(File, ITestInvocationListener)}
     */
    @Test
    public void testTestTmpDirClean_success() {
        mMockListener.testRunStarted("temporaryFiles", 1);
        mMockListener.testStarted((TestDescription) EasyMock.anyObject());
        mMockListener.testEnded(
                (TestDescription) EasyMock.anyObject(), EasyMock.eq(new HashMap<String, Metric>()));
        mMockListener.testRunEnded(0, new HashMap<String, Metric>());
        File tmpDir = Mockito.mock(File.class);
        Mockito.when(tmpDir.list())
                .thenReturn(new String[] {"inv_123", "tradefed_global_log_123", "lc_cache",
                        "stage-android-build-api"});
        EasyMock.replay(mMockListener);
        mTfTestLauncher.testTmpDirClean(tmpDir, mMockListener);
        EasyMock.verify(mMockListener);
    }

    /**
     * Test {@link TfTestLauncher#testTmpDirClean(File, ITestInvocationListener)}
     *
     * Test should fail if there are extra files do not match expected pattern.
     */
    @Test
    public void testTestTmpDirClean_failExtraFile() {
        mMockListener.testRunStarted("temporaryFiles", 1);
        mMockListener.testStarted((TestDescription) EasyMock.anyObject());
        mMockListener.testFailed(
                (TestDescription) EasyMock.anyObject(), (String) EasyMock.anyObject());
        mMockListener.testEnded(
                (TestDescription) EasyMock.anyObject(), EasyMock.eq(new HashMap<String, Metric>()));
        mMockListener.testRunEnded(0, new HashMap<String, Metric>());
        File tmpDir = Mockito.mock(File.class);
        Mockito.when(tmpDir.list()).thenReturn(new String[] {"extra_file"});
        EasyMock.replay(mMockListener);
        mTfTestLauncher.testTmpDirClean(tmpDir, mMockListener);
        EasyMock.verify(mMockListener);
    }

    /**
     * Test {@link TfTestLauncher#testTmpDirClean(File, ITestInvocationListener)}
     *
     * Test should fail if there are multiple files matching an expected pattern.
     */
    @Test
    public void testTestTmpDirClean_failMultipleFiles() {
        mMockListener.testRunStarted("temporaryFiles", 1);
        mMockListener.testStarted((TestDescription) EasyMock.anyObject());
        mMockListener.testFailed(
                (TestDescription) EasyMock.anyObject(), (String) EasyMock.anyObject());
        mMockListener.testEnded(
                (TestDescription) EasyMock.anyObject(), EasyMock.eq(new HashMap<String, Metric>()));
        mMockListener.testRunEnded(0, new HashMap<String, Metric>());
        File tmpDir = Mockito.mock(File.class);
        Mockito.when(tmpDir.list()).thenReturn(new String[] {"inv_1", "inv_2"});
        EasyMock.replay(mMockListener);
        mTfTestLauncher.testTmpDirClean(tmpDir, mMockListener);
        EasyMock.verify(mMockListener);
    }

    /** Test that when code coverage option is on, we add the javaagent to the java arguments. */
    @Test
    public void testRunCoverage() throws Exception {
        OptionSetter setter = new OptionSetter(mTfTestLauncher);
        setter.setOptionValue("jacoco-code-coverage", "true");
        EasyMock.expect(mMockBuildInfo.getRootDir()).andReturn(new File(""));
        EasyMock.expect(mMockBuildInfo.getTestTag()).andReturn(TEST_TAG);
        EasyMock.expect(mMockBuildInfo.getBuildBranch()).andReturn(BUILD_BRANCH).times(2);
        EasyMock.expect(mMockBuildInfo.getBuildFlavor()).andReturn(BUILD_FLAVOR).times(2);
        EasyMock.expect(mMockBuildInfo.getBuildId()).andReturn(BUILD_ID).times(2);
        mMockRunUtil.unsetEnvVariable(TfTestLauncher.TF_GLOBAL_CONFIG);
        mMockRunUtil.unsetEnvVariable(EnvVariable.ANDROID_HOST_OUT_TESTCASES.name());
        mMockRunUtil.unsetEnvVariable(EnvVariable.ANDROID_TARGET_OUT_TESTCASES.name());
        mMockRunUtil.setEnvVariablePriority(EnvPriority.SET);
        mMockRunUtil.setEnvVariable(SubprocessTfLauncher.TF_GLOBAL_CONFIG, SUB_GLOBAL_CONFIG);
        EasyMock.replay(mMockBuildInfo, mMockRunUtil, mMockListener);
        try {
            mTfTestLauncher.preRun();
            EasyMock.verify(mMockBuildInfo, mMockRunUtil, mMockListener);
            assertTrue(mTfTestLauncher.mCmdArgs.get(2).startsWith("-javaagent:"));
        } finally {
            FileUtil.recursiveDelete(mTfTestLauncher.mTmpDir);
            mTfTestLauncher.cleanTmpFile();
        }
    }
}
