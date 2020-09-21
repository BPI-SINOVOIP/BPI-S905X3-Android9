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
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.fail;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyLong;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import com.android.compatibility.common.tradefed.build.CompatibilityBuildHelper;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.targetprep.TargetSetupError;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.IRunUtil;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.mockito.InjectMocks;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
/**
 * Unit tests for {@link VtsCoveragePreparer}.
 */
@RunWith(JUnit4.class)
public final class VtsCoveragePreparerTest {
    private String BUILD_INFO_ARTIFACT = VtsCoveragePreparer.BUILD_INFO_ARTIFACT;
    private String GCOV_PROPERTY = VtsCoveragePreparer.GCOV_PROPERTY;
    private String GCOV_FILE_NAME = VtsCoveragePreparer.GCOV_FILE_NAME;
    private String SYMBOLS_FILE_NAME = VtsCoveragePreparer.SYMBOLS_FILE_NAME;
    private String COVERAGE_REPORT_PATH = VtsCoveragePreparer.COVERAGE_REPORT_PATH;

    // Path to store coverage report files.
    private static final String TEST_COVERAGE_REPORT_PATH = "/tmp/test-coverage";

    private File mTestDir;
    private CompatibilityBuildHelper mMockHelper;

    private class TestCoveragePreparer extends VtsCoveragePreparer {
        @Override
        CompatibilityBuildHelper createBuildHelper(IBuildInfo buildInfo) {
            return mMockHelper;
        }

        @Override
        File createTempDir(ITestDevice device) {
            return mTestDir;
        }

        @Override
        String getArtifactFetcher(IBuildInfo buildInfo) {
            return "fetcher --bid %s --target %s %s %s";
        }
    }

    @Mock private IBuildInfo mBuildInfo;
    @Mock private ITestDevice mDevice;
    @Mock private IRunUtil mRunUtil;
    @InjectMocks private TestCoveragePreparer mPreparer = new TestCoveragePreparer();

    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);
        mTestDir = FileUtil.createTempDir("vts-coverage-preparer-unit-tests");
        mMockHelper = new CompatibilityBuildHelper(mBuildInfo) {
            @Override
            public File getTestsDir() throws FileNotFoundException {
                return mTestDir;
            }
            public File getResultDir() throws FileNotFoundException {
                return mTestDir;
            }
        };
        doReturn("build_id").when(mDevice).getBuildId();
        doReturn("1234").when(mDevice).getSerialNumber();
        doReturn("enforcing").when(mDevice).executeShellCommand("getenforce");
        doReturn("build_id").when(mBuildInfo).getBuildId();
        CommandResult commandResult = new CommandResult();
        commandResult.setStatus(CommandStatus.SUCCESS);
        doReturn(commandResult).when(mRunUtil).runTimedCmd(anyLong(), any());
    }

    @After
    public void tearDown() throws Exception {
        FileUtil.recursiveDelete(mTestDir);
    }

    @Test
    public void testOnSetUpCoverageDisabled() throws Exception {
        doReturn("UnknowFlaver").when(mDevice).getBuildFlavor();
        doReturn("None").when(mDevice).getProperty(GCOV_PROPERTY);

        mPreparer.setUp(mDevice, mBuildInfo);
        verify(mBuildInfo, never()).setFile((String) any(), any(), any());
    }

    @Test
    public void testOnSetUpSancovEnabled() throws Exception {
        doReturn("walleye_asan_coverage-userdebug").when(mDevice).getBuildFlavor();
        createTestFile(SYMBOLS_FILE_NAME);
        createTestFile(BUILD_INFO_ARTIFACT);

        mPreparer.setUp(mDevice, mBuildInfo);
        verify(mBuildInfo, times(1))
                .setFile(eq(VtsCoveragePreparer.getSancovResourceDirKey(mDevice)), eq(mTestDir),
                        eq("build_id"));
    }

    @Test
    public void testOnSetUpGcovEnabled() throws Exception {
        doReturn("walleye_coverage-userdebug").when(mDevice).getBuildFlavor();
        doReturn("1").when(mDevice).getProperty(GCOV_PROPERTY);
        createTestFile(GCOV_FILE_NAME);
        createTestFile(BUILD_INFO_ARTIFACT);
        mPreparer.setUp(mDevice, mBuildInfo);
        verify(mBuildInfo, times(1))
                .setFile(eq(VtsCoveragePreparer.getGcovResourceDirKey(mDevice)), eq(mTestDir),
                        eq("build_id"));
    }

    @Test
    public void testOnSetUpLocalArtifectsNormal() throws Exception {
        OptionSetter setter = new OptionSetter(mPreparer);
        setter.setOptionValue("use-local-artifects", "true");
        setter.setOptionValue("local-coverage-resource-path", mTestDir.getAbsolutePath());
        doReturn("1").when(mDevice).getProperty(GCOV_PROPERTY);
        createTestFile(GCOV_FILE_NAME);
        createTestFile(BUILD_INFO_ARTIFACT);

        mPreparer.setUp(mDevice, mBuildInfo);
        verify(mBuildInfo, times(1))
                .setFile(eq(VtsCoveragePreparer.getGcovResourceDirKey(mDevice)), eq(mTestDir),
                        eq("build_id"));
    }

    @Test
    public void testOnSetUpLocalArtifectsNotExists() throws Exception {
        OptionSetter setter = new OptionSetter(mPreparer);
        setter.setOptionValue("use-local-artifects", "true");
        setter.setOptionValue("local-coverage-resource-path", mTestDir.getAbsolutePath());
        doReturn("1").when(mDevice).getProperty(GCOV_PROPERTY);

        try {
            mPreparer.setUp(mDevice, mBuildInfo);
        } catch (TargetSetupError e) {
            // Expected.
            assertEquals(String.format("Could not find %s under %s.", GCOV_FILE_NAME,
                                 mTestDir.getAbsolutePath()),
                    e.getMessage());
            verify(mBuildInfo, never()).setFile((String) any(), any(), any());
            return;
        }
        fail();
    }

    @Test
    public void testOnSetUpOutputCoverageReport() throws Exception {
        OptionSetter setter = new OptionSetter(mPreparer);
        setter.setOptionValue("coverage-report-dir", TEST_COVERAGE_REPORT_PATH);
        doReturn("walleye_coverage-userdebug").when(mDevice).getBuildFlavor();
        doReturn("1").when(mDevice).getProperty(GCOV_PROPERTY);
        createTestFile(GCOV_FILE_NAME);
        createTestFile(BUILD_INFO_ARTIFACT);

        mPreparer.setUp(mDevice, mBuildInfo);
        verify(mBuildInfo, times(1))
                .addBuildAttribute(eq(COVERAGE_REPORT_PATH),
                        eq(mTestDir.getAbsolutePath() + TEST_COVERAGE_REPORT_PATH));
    }

    @Test
    public void testOnTearDown() throws Exception {
        doReturn("walleye_coverage-userdebug").when(mDevice).getBuildFlavor();
        doReturn("1").when(mDevice).getProperty(GCOV_PROPERTY);
        File artifectsFile = createTestFile(GCOV_FILE_NAME);
        File buildInfoFile = createTestFile(BUILD_INFO_ARTIFACT);
        mPreparer.setUp(mDevice, mBuildInfo);
        mPreparer.tearDown(mDevice, mBuildInfo, null);
        verify(mDevice, times(1)).executeShellCommand("setenforce enforcing");
        assertFalse(artifectsFile.exists());
        assertFalse(buildInfoFile.exists());
    }

    /**
     * Helper method to create a test file under mTestDir.
     *
     * @param fileName test file name.
     * @return created test file.
     */
    private File createTestFile(String fileName) throws IOException {
        File testFile = new File(mTestDir, fileName);
        testFile.createNewFile();
        return testFile;
    }
}
