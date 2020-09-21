/*
 * Copyright (C) 2011 The Android Open Source Project
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
import static org.junit.Assert.assertNull;
import static org.junit.Assert.fail;

import com.android.tradefed.build.BuildInfoKey.BuildInfoFileKey;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.build.IDeviceBuildInfo;
import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.util.FileUtil;

import org.easymock.EasyMock;
import org.junit.Before;
import org.junit.Test;

import java.io.File;

/** Unit tests for {@link PushFilePreparer} */
public class PushFilePreparerTest {

    private static final String HOST_TESTCASES = "host/testcases";

    private PushFilePreparer mPreparer = null;
    private ITestDevice mMockDevice = null;
    private OptionSetter mOptionSetter = null;

    @Before
    public void setUp() throws Exception {
        mMockDevice = EasyMock.createStrictMock(ITestDevice.class);
        EasyMock.expect(mMockDevice.getDeviceDescriptor()).andStubReturn(null);
        EasyMock.expect(mMockDevice.getSerialNumber()).andStubReturn("SERIAL");
        mPreparer = new PushFilePreparer();
        mOptionSetter = new OptionSetter(mPreparer);
    }

    /** When there's nothing to be done, expect no exception to be thrown */
    @Test
    public void testNoop() throws Exception {
        EasyMock.replay(mMockDevice);
        mPreparer.setUp(mMockDevice, null);
        EasyMock.verify(mMockDevice);
    }

    @Test
    public void testLocalNoExist() throws Exception {
        mOptionSetter.setOptionValue("push", "/noexist->/data/");
        mOptionSetter.setOptionValue("post-push", "ls /");
        EasyMock.replay(mMockDevice);
        try {
            // Should throw TargetSetupError and _not_ run any post-push command
            mPreparer.setUp(mMockDevice, null);
            fail("TargetSetupError not thrown");
        } catch (TargetSetupError e) {
            // expected
        }
        EasyMock.verify(mMockDevice);
    }

    @Test
    public void testRemoteNoExist() throws Exception {
        mOptionSetter.setOptionValue("push", "/bin/sh->/noexist/");
        mOptionSetter.setOptionValue("post-push", "ls /");
        // expect a pushFile() call and return false (failed)
        EasyMock.expect(
                mMockDevice.pushFile((File)EasyMock.anyObject(), EasyMock.eq("/noexist/")))
                .andReturn(Boolean.FALSE);
        EasyMock.replay(mMockDevice);
        try {
            // Should throw TargetSetupError and _not_ run any post-push command
            mPreparer.setUp(mMockDevice, null);
            fail("TargetSetupError not thrown");
        } catch (TargetSetupError e) {
            // expected
        }
        EasyMock.verify(mMockDevice);
    }

    @Test
    public void testWarnOnFailure() throws Exception {
        mOptionSetter.setOptionValue("push", "/bin/sh->/noexist/");
        mOptionSetter.setOptionValue("push", "/noexist->/data/");
        mOptionSetter.setOptionValue("post-push", "ls /");
        mOptionSetter.setOptionValue("abort-on-push-failure", "false");

        // expect a pushFile() call and return false (failed)
        EasyMock.expect(
                mMockDevice.pushFile((File)EasyMock.anyObject(), EasyMock.eq("/noexist/")))
                .andReturn(Boolean.FALSE);
        // Because we're only warning, the post-push command should be run despite the push failures
        EasyMock.expect(mMockDevice.executeShellCommand(EasyMock.eq("ls /"))).andReturn("");
        EasyMock.replay(mMockDevice);

        // Don't expect any exceptions to be thrown
        mPreparer.setUp(mMockDevice, null);
        EasyMock.verify(mMockDevice);
    }

    /**
     * Test {@link PushFilePreparer#resolveRelativeFilePath(IBuildInfo, String)} do not search
     * additional tests directory if the given build if is not of IBuildInfo type.
     */
    @Test
    public void testResolveRelativeFilePath_noDeviceBuildInfo() {
        IBuildInfo buildInfo = EasyMock.createStrictMock(IBuildInfo.class);
        String fileName = "source_file";
        EasyMock.expect(buildInfo.getFile(fileName)).andReturn(null);
        EasyMock.replay(buildInfo);

        assertNull(mPreparer.resolveRelativeFilePath(buildInfo, fileName));
        EasyMock.verify(buildInfo);
    }

    /**
     * Test {@link PushFilePreparer#resolveRelativeFilePath(IBuildInfo, String)} can locate a source
     * file existed in tests directory of a device build.
     */
    @Test
    public void testResolveRelativeFilePath_withDeviceBuildInfo() throws Exception {
        IDeviceBuildInfo buildInfo = EasyMock.createStrictMock(IDeviceBuildInfo.class);
        String fileName = "source_file";

        File testsDir = null;
        try {
            testsDir = FileUtil.createTempDir("tests_dir");
            File hostTestCasesDir = FileUtil.getFileForPath(testsDir, HOST_TESTCASES);
            FileUtil.mkdirsRWX(hostTestCasesDir);
            File sourceFile = FileUtil.createTempFile(fileName, null, hostTestCasesDir);

            fileName = sourceFile.getName();
            EasyMock.expect(buildInfo.getFile(fileName)).andReturn(null);
            EasyMock.expect(buildInfo.getFile(BuildInfoFileKey.TARGET_LINKED_DIR)).andReturn(null);
            EasyMock.expect(buildInfo.getTestsDir()).andReturn(testsDir);
            EasyMock.replay(buildInfo);

            assertEquals(
                    sourceFile.getAbsolutePath(),
                    mPreparer.resolveRelativeFilePath(buildInfo, fileName).getAbsolutePath());
            EasyMock.verify(buildInfo);
        } finally {
            FileUtil.recursiveDelete(testsDir);
        }
    }
}

