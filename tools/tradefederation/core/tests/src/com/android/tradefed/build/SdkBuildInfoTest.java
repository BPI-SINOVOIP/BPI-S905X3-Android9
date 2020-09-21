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
package com.android.tradefed.build;

import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;
import com.android.tradefed.util.IRunUtil;

import junit.framework.TestCase;

import org.easymock.EasyMock;

import java.io.File;

/**
 * Unit tests for {@link SdkBuildInfo}.
 */
public class SdkBuildInfoTest extends TestCase {

    private IRunUtil mMockRunUtil;
    private SdkBuildInfo mSdkBuild;

    @Override
    protected void setUp() throws Exception {
        mMockRunUtil = EasyMock.createMock(IRunUtil.class);
        mSdkBuild =
                new SdkBuildInfo() {
                    private static final long serialVersionUID = BuildSerializedVersion.VERSION;

                    @Override
                    IRunUtil getRunUtil() {
                        return mMockRunUtil;
                    }
                };
        mSdkBuild.setSdkDir(new File("tmp"));
    }
    /**
     * Test success case for {@link SdkBuildInfo#getSdkTargets()}.
     */
    public void testGetSdkTargets() {
        CommandResult result = new CommandResult();
        result.setStdout("android-1\nandroid-2\n");
        result.setStatus(CommandStatus.SUCCESS);
        // expect the 'android list targets --compact' call
        EasyMock.expect(mMockRunUtil.runTimedCmd(EasyMock.anyLong(),
                (String)EasyMock.anyObject() /* android */,
                (String)EasyMock.anyObject(), /* list */
                (String)EasyMock.anyObject(), /* targets */
                (String)EasyMock.anyObject() /* --compact */)).andReturn(result);
        EasyMock.replay(mMockRunUtil);
        String[] targets = mSdkBuild.getSdkTargets();
        assertEquals("android-1", targets[0]);
        assertEquals("android-2", targets[1]);
    }
}
