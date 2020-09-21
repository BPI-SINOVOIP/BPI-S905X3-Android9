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
package com.android.tradefed.suite.checker;

import static org.junit.Assert.assertEquals;

import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.ITestLogger;
import com.android.tradefed.result.ByteArrayInputStreamSource;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.suite.checker.StatusCheckerResult.CheckStatus;

import org.easymock.EasyMock;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Unit tests for {@link ActivityStatusChecker}. */
@RunWith(JUnit4.class)
public class ActivityStatusCheckerTest {
    private ActivityStatusChecker mChecker;
    private ITestLogger mMockLogger;
    private ITestDevice mMockDevice;

    @Before
    public void setUp() {
        mMockLogger = EasyMock.createStrictMock(ITestLogger.class);
        mChecker = new ActivityStatusChecker();
        mChecker.setTestLogger(mMockLogger);
        mMockDevice = EasyMock.createStrictMock(ITestDevice.class);
    }

    /** Test that the status checker is a success if the home/launcher activity is on top. */
    @Test
    public void testCheckerLauncherHomeScreen() throws Exception {
        EasyMock.expect(mMockDevice.executeShellCommand(EasyMock.anyObject()))
                .andReturn(
                        "  mCurrentFocus=Window{46dd15 u0 com.google.android.apps.nexuslauncher/"
                                + "com.google.android.apps.nexuslauncher.NexusLauncherActivity}\n"
                                + "  mFocusedApp=AppWindowToken{37e3c39 token=Token{312ce85 ActivityRecord{a9437fc "
                                + "u0 com.google.android.apps.nexuslauncher/.NexusLauncherActivity t2}}}");
        EasyMock.replay(mMockLogger, mMockDevice);
        assertEquals(CheckStatus.SUCCESS, mChecker.postExecutionCheck(mMockDevice).getStatus());
        EasyMock.verify(mMockLogger, mMockDevice);
    }

    /** Test that if another activity is on top, then we fail the checker and take a screenshot. */
    @Test
    public void testCheckerOtherActivity() throws Exception {
        EasyMock.expect(mMockDevice.executeShellCommand(EasyMock.anyObject()))
                .andReturn(
                        "mCurrentFocus=Window{52b89df u0 com.android.chrome/org.chromium.chrome."
                                + "browser.ChromeTabbedActivity}\n"
                                + "  mFocusedApp=AppWindowToken{955b485 token=Token{6bebd1b ActivityRecord{fd30b2a "
                                + "u0 com.android.chrome/org.chromium.chrome.browser.ChromeTabbedActivity t7}}}");
        InputStreamSource fake = new ByteArrayInputStreamSource("fakedata".getBytes());
        EasyMock.expect(mMockDevice.getScreenshot(EasyMock.anyObject())).andReturn(fake);
        mMockLogger.testLog("status_checker_front_activity", LogDataType.JPEG, fake);
        EasyMock.replay(mMockLogger, mMockDevice);
        assertEquals(CheckStatus.FAILED, mChecker.postExecutionCheck(mMockDevice).getStatus());
        EasyMock.verify(mMockLogger, mMockDevice);
    }
}
