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

package com.android.tradefed.device;

import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;
import com.android.tradefed.util.IRunUtil;

import junit.framework.TestCase;

import org.easymock.EasyMock;

import java.util.Collection;

/**
 * Unit tests for {@link FastbootHelper}.
 */
public class FastbootHelperTest extends TestCase {

    private IRunUtil mMockRunUtil;
    private FastbootHelper mFastbootHelper;

    /**
     * {@inheritDoc}
     */
    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mMockRunUtil = EasyMock.createMock(IRunUtil.class);
        mFastbootHelper = new FastbootHelper(mMockRunUtil, "fastboot");
    }

    /**
     * Verify the 'fastboot devices' output parsing
     */
    public void testParseDevicesOnFastboot() {
        Collection<String> deviceSerials = mFastbootHelper.parseDevices(
                "04035EEB0B01F01C        fastboot\n" +
                "HT99PP800024    fastboot\n" +
                "????????????    fastboot");
        assertEquals(2, deviceSerials.size());
        assertTrue(deviceSerials.contains("04035EEB0B01F01C"));
        assertTrue(deviceSerials.contains("HT99PP800024"));
    }

    /**
     * Verify 'fastboot devices' parsing with hyphenated serial number.
     */
    public void testParseDevicesOnFastboot_hyphen() {
        Collection<String> deviceSerials = mFastbootHelper.parseDevices(
                "Foo-Bar123    fastboot");
        assertEquals(1, deviceSerials.size());
        assertTrue(deviceSerials.contains("Foo-Bar123"));
    }

    /**
     * Verify the 'fastboot devices' output parsing when empty
     */
    public void testParseDevicesOnFastboot_empty() {
        Collection<String> deviceSerials = mFastbootHelper.parseDevices("");
        assertEquals(0, deviceSerials.size());
    }

    /**
     * Ensure that FastbootHelper cannot be created with incorrect params
     */
    public void testConstructor_noRunUtil() {
        try {
            new FastbootHelper(null, "/fakepath/");
            fail("Should have thrown an exception");
        } catch (IllegalArgumentException expected) {
            assertEquals("runUtil cannot be null", expected.getMessage());
        }
    }

    /**
     * Ensure that FastbootHelper cannot be created with incorrect path params
     */
    public void testConstructor_badPath() {
        try {
            new FastbootHelper(mMockRunUtil, null);
            fail("Should have thrown an exception");
        } catch (IllegalArgumentException expected) {
            assertEquals("fastboot cannot be null or empty", expected.getMessage());
        }
        try {
            new FastbootHelper(mMockRunUtil, "");
            fail("Should have thrown an exception");
        } catch (IllegalArgumentException expected) {
            assertEquals("fastboot cannot be null or empty", expected.getMessage());
        }
    }

    /**
     * Test that an older version of fastboot is still accepted
     */
    public void testIsFastbootAvailable_oldVersion() {
        CommandResult fakeRes = new CommandResult(CommandStatus.FAILED);
        fakeRes.setStderr("Help doesn't exists. usage: fastboot");
        EasyMock.expect(mMockRunUtil.runTimedCmdSilently(EasyMock.anyLong(),
                EasyMock.eq("fastboot"), EasyMock.eq("help"))).andReturn(fakeRes);
        EasyMock.replay(mMockRunUtil);
        assertTrue(mFastbootHelper.isFastbootAvailable());
        EasyMock.verify(mMockRunUtil);
    }

    /**
     * Test that an older version of fastboot is still accepted
     */
    public void testIsFastbootAvailable_noBinary() {
        CommandResult fakeRes = new CommandResult(CommandStatus.FAILED);
        fakeRes.setStderr("No command 'fastboot' found");
        EasyMock.expect(mMockRunUtil.runTimedCmdSilently(EasyMock.anyLong(),
                EasyMock.eq("fastboot"), EasyMock.eq("help"))).andReturn(fakeRes);
        EasyMock.replay(mMockRunUtil);
        assertFalse(mFastbootHelper.isFastbootAvailable());
        EasyMock.verify(mMockRunUtil);
    }

    /**
     * Test that get device returns null if command fails.
     */
    public void testGetDevice_fail() {
        CommandResult fakeRes = new CommandResult(CommandStatus.FAILED);
        fakeRes.setStdout("");
        fakeRes.setStderr("");
        EasyMock.expect(
                        mMockRunUtil.runTimedCmdSilently(
                                EasyMock.anyLong(),
                                EasyMock.eq("fastboot"),
                                EasyMock.eq("devices")))
                .andReturn(fakeRes);
        EasyMock.replay(mMockRunUtil);
        assertTrue(mFastbootHelper.getDevices().isEmpty());
        EasyMock.verify(mMockRunUtil);
    }

    /**
     * Test {@link FastbootHelper#executeCommand(String, String)} return the stdout on success.
     */
    public void testExecuteCommand() {
        final String expectedStdout = "stdout";
        CommandResult fakeRes = new CommandResult(CommandStatus.SUCCESS);
        fakeRes.setStdout(expectedStdout);
        EasyMock.expect(mMockRunUtil.runTimedCmd(EasyMock.anyLong(), EasyMock.eq("fastboot"),
                EasyMock.eq("-s"), EasyMock.eq("SERIAL"), EasyMock.eq("wipe"))).andReturn(fakeRes);
        EasyMock.replay(mMockRunUtil);
        assertEquals(expectedStdout, mFastbootHelper.executeCommand("SERIAL", "wipe"));
        EasyMock.verify(mMockRunUtil);
    }

    /**
     * Test {@link FastbootHelper#executeCommand(String, String)} return null on failure
     */
    public void testExecuteCommand_fail() {
        CommandResult fakeRes = new CommandResult(CommandStatus.FAILED);
        fakeRes.setStderr("error");
        EasyMock.expect(mMockRunUtil.runTimedCmd(EasyMock.anyLong(), EasyMock.eq("fastboot"),
                EasyMock.eq("-s"), EasyMock.eq("SERIAL"), EasyMock.eq("wipe"))).andReturn(fakeRes);
        EasyMock.replay(mMockRunUtil);
        assertNull(mFastbootHelper.executeCommand("SERIAL", "wipe"));
        EasyMock.verify(mMockRunUtil);
    }
}
