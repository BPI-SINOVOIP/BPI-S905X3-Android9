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
package com.android.tradefed.device;

import static org.junit.Assert.*;

import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;
import com.android.tradefed.util.IRunUtil;

import org.easymock.EasyMock;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Unit tests for {@link AndroidDebugBridgeWrapper}. */
@RunWith(JUnit4.class)
public class AndroidDebugBridgeWrapperTest {

    private AndroidDebugBridgeWrapper mBridge;
    private IRunUtil mMockRunUtil;

    @Before
    public void setUp() {
        mMockRunUtil = EasyMock.createMock(IRunUtil.class);
        mBridge =
                new AndroidDebugBridgeWrapper() {
                    @Override
                    IRunUtil getRunUtil() {
                        return mMockRunUtil;
                    }
                };
    }

    /** Test parsing the adb version when available. */
    @Test
    public void testAdbVersionParsing() {
        CommandResult res = new CommandResult();
        res.setStatus(CommandStatus.SUCCESS);
        res.setStdout("Android Debug Bridge version 1.0.36\nRevision 0e7324e9095a-android\n");
        EasyMock.expect(
                        mMockRunUtil.runTimedCmd(
                                EasyMock.anyLong(), EasyMock.eq("fakeadb"), EasyMock.eq("version")))
                .andReturn(res);
        EasyMock.replay(mMockRunUtil);
        String version = mBridge.getAdbVersion("fakeadb");
        assertEquals("1.0.36-0e7324e9095a-android", version);
        EasyMock.verify(mMockRunUtil);
    }

    /** Test parsing the adb version when available with alternative format. */
    @Test
    public void testAdbVersionParsing_altFormat() {
        CommandResult res = new CommandResult();
        res.setStatus(CommandStatus.SUCCESS);
        res.setStdout(
                "Android Debug Bridge version 1.0.36\n"
                        + "Version 0.0.0-4407735\n"
                        + "Installed as /usr/local/adb\n");
        EasyMock.expect(
                        mMockRunUtil.runTimedCmd(
                                EasyMock.anyLong(), EasyMock.eq("fakeadb"), EasyMock.eq("version")))
                .andReturn(res);
        EasyMock.replay(mMockRunUtil);
        String version = mBridge.getAdbVersion("fakeadb");
        assertEquals("1.0.36 subVersion: 0.0.0-4407735 install path: /usr/local/adb", version);
        EasyMock.verify(mMockRunUtil);
    }

    /** Test when the version process fails. */
    @Test
    public void testAdbVersionParse_runFail() {
        CommandResult res = new CommandResult();
        res.setStatus(CommandStatus.FAILED);
        res.setStdout("");
        res.setStderr("");
        EasyMock.expect(
                        mMockRunUtil.runTimedCmd(
                                EasyMock.anyLong(), EasyMock.eq("fakeadb"), EasyMock.eq("version")))
                .andReturn(res);
        EasyMock.replay(mMockRunUtil);
        assertNull(mBridge.getAdbVersion("fakeadb"));
        EasyMock.verify(mMockRunUtil);
    }

    /** Test when the revision is not present, in that case we output a partial version. */
    @Test
    public void testAdbVersionParsing_partial() {
        CommandResult res = new CommandResult();
        res.setStatus(CommandStatus.SUCCESS);
        res.setStdout("Android Debug Bridge version 1.0.36\n");
        EasyMock.expect(
                        mMockRunUtil.runTimedCmd(
                                EasyMock.anyLong(), EasyMock.eq("fakeadb"), EasyMock.eq("version")))
                .andReturn(res);
        EasyMock.replay(mMockRunUtil);
        String version = mBridge.getAdbVersion("fakeadb");
        assertEquals("1.0.36", version);
        EasyMock.verify(mMockRunUtil);
    }

    /** Test when the output from 'adb version' is not as expected at all. */
    @Test
    public void testAdbVersionParsing_parseFail() {
        CommandResult res = new CommandResult();
        res.setStatus(CommandStatus.SUCCESS);
        res.setStdout("This is probably not the right output\n");
        EasyMock.expect(
                        mMockRunUtil.runTimedCmd(
                                EasyMock.anyLong(), EasyMock.eq("fakeadb"), EasyMock.eq("version")))
                .andReturn(res);
        EasyMock.replay(mMockRunUtil);
        assertNull(mBridge.getAdbVersion("fakeadb"));
        EasyMock.verify(mMockRunUtil);
    }
}
