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

package com.android.tradefed.targetprep;

import static org.easymock.EasyMock.anyLong;
import static org.easymock.EasyMock.expect;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import com.android.tradefed.build.BuildInfo;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.IRunUtil;

import org.easymock.EasyMock;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.io.File;

/**
 * Unit tests for {@link VtsPythonVirtualenvPreparer}.</p>
 * TODO: add tests to cover a full end-to-end scenario.
 */
@RunWith(JUnit4.class)
public class VtsPythonVirtualenvPreparerTest {
    private VtsPythonVirtualenvPreparer mPreparer;
    private IRunUtil mMockRunUtil;

    @Before
    public void setUp() throws Exception {
        mMockRunUtil = EasyMock.createMock(IRunUtil.class);
        mPreparer = new VtsPythonVirtualenvPreparer() {
            @Override
            IRunUtil getRunUtil() {
                return mMockRunUtil;
            }
        };
        mPreparer.mDepModules.add("enum");
    }

    /**
     * Test that the installation of dependencies and requirements file is as expected.
     */
    @Test
    public void testInstallDeps_reqFile_success() throws Exception {
        File requirementFile = FileUtil.createTempFile("reqfile", ".txt");
        try {
            mPreparer.setRequirementsFile(requirementFile);
            CommandResult result = new CommandResult(CommandStatus.SUCCESS);
            result.setStdout("output");
            result.setStderr("std err");
            // First check that the install requirements was attempted.
            expect(mMockRunUtil.runTimedCmd(anyLong(), EasyMock.eq("pip"), EasyMock.eq("install"),
                           EasyMock.eq("-r"), EasyMock.eq(requirementFile.getAbsolutePath())))
                    .andReturn(result);
            // Check that all default modules are installed
            addDefaultModuleExpectations(mMockRunUtil, result);
            EasyMock.replay(mMockRunUtil);
            mPreparer.installDeps();
            EasyMock.verify(mMockRunUtil);
        } finally {
            FileUtil.deleteFile(requirementFile);
        }
    }

    /**
     * Test that if an extra dependency module is required, we install it too.
     */
    @Test
    public void testInstallDeps_depModule_success() throws Exception {
        mPreparer.addDepModule("blahblah");
        CommandResult result = new CommandResult(CommandStatus.SUCCESS);
        result.setStdout("output");
        result.setStderr("std err");
        addDefaultModuleExpectations(mMockRunUtil, result);
        // The non default module provided is also attempted to be installed.
        expect(mMockRunUtil.runTimedCmd(anyLong(), EasyMock.eq("pip"), EasyMock.eq("install"),
                       EasyMock.eq("blahblah")))
                .andReturn(result);

        EasyMock.replay(mMockRunUtil);
        mPreparer.installDeps();
        EasyMock.verify(mMockRunUtil);
    }

    /**
     * Test that an installation failure of the requirements file throws a {@link TargetSetupError}.
     */
    @Test
    public void testInstallDeps_reqFile_failure() throws Exception {
        File requirementFile = FileUtil.createTempFile("reqfile", ".txt");
        try {
            mPreparer.setRequirementsFile(requirementFile);
            CommandResult result = new CommandResult(CommandStatus.TIMED_OUT);
            result.setStdout("output");
            result.setStderr("std err");
            expect(mMockRunUtil.runTimedCmd(anyLong(), EasyMock.eq("pip"), EasyMock.eq("install"),
                           EasyMock.eq("-r"), EasyMock.eq(requirementFile.getAbsolutePath())))
                    .andReturn(result);
            EasyMock.replay(mMockRunUtil);
            IBuildInfo buildInfo = new BuildInfo();
            try {
                mPreparer.installDeps();
                fail("installDeps succeeded despite a failed command");
            } catch (TargetSetupError e) {
                assertTrue(buildInfo.getFile("PYTHONPATH") == null);
            }
            EasyMock.verify(mMockRunUtil);
        } finally {
            FileUtil.deleteFile(requirementFile);
        }
    }

    /**
     * Test that an installation failure of the dep module throws a {@link TargetSetupError}.
     */
    @Test
    public void testInstallDeps_depModule_failure() throws Exception {
        CommandResult result = new CommandResult(CommandStatus.TIMED_OUT);
        result.setStdout("output");
        result.setStderr("std err");
        expect(mMockRunUtil.runTimedCmd(
                       anyLong(), EasyMock.eq("pip"), EasyMock.eq("install"), EasyMock.eq("enum")))
                .andReturn(result);
        // If installing the dependency failed, an upgrade is attempted:
        expect(mMockRunUtil.runTimedCmd(anyLong(), EasyMock.eq("pip"), EasyMock.eq("install"),
                       EasyMock.eq("--upgrade"), EasyMock.eq("enum")))
                .andReturn(result);
        EasyMock.replay(mMockRunUtil);
        IBuildInfo buildInfo = new BuildInfo();
        try {
            mPreparer.installDeps();
            mPreparer.addPathToBuild(buildInfo);
            fail("installDeps succeeded despite a failed command");
        } catch (TargetSetupError e) {
            assertTrue(buildInfo.getFile("PYTHONPATH") == null);
        }
        EasyMock.verify(mMockRunUtil);
    }

    private void addDefaultModuleExpectations(IRunUtil mockRunUtil, CommandResult result) {
        expect(mockRunUtil.runTimedCmd(
                       anyLong(), EasyMock.eq("pip"), EasyMock.eq("install"), EasyMock.eq("enum")))
                .andReturn(result);
    }
}