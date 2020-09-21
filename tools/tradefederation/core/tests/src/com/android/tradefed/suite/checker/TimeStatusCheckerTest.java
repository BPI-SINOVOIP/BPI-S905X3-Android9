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
package com.android.tradefed.suite.checker;

import static org.junit.Assert.assertEquals;

import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.suite.checker.StatusCheckerResult.CheckStatus;

import org.easymock.EasyMock;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Unit tests for {@link TimeStatusChecker}. */
@RunWith(JUnit4.class)
public class TimeStatusCheckerTest {

    private TimeStatusChecker mChecker;
    private ITestDevice mMockDevice;

    @Before
    public void setUp() {
        mMockDevice = EasyMock.createMock(ITestDevice.class);
        mChecker = new TimeStatusChecker();
    }

    /**
     * Test that the status checker is successful and does not resync the time for small
     * differences.
     */
    @Test
    public void testCheckTimeDiff_small() throws DeviceNotAvailableException {
        EasyMock.expect(mMockDevice.getDeviceTimeOffset(EasyMock.anyObject())).andReturn(2000L);
        EasyMock.replay(mMockDevice);
        assertEquals(CheckStatus.SUCCESS, mChecker.postExecutionCheck(mMockDevice).getStatus());
        EasyMock.verify(mMockDevice);
    }

    /**
     * Test that when the time difference is bigger, we fail the status checker and resync the time
     * between the host and the device.
     */
    @Test
    public void testCheckTimeDiff_large() throws DeviceNotAvailableException {
        EasyMock.expect(mMockDevice.getDeviceTimeOffset(EasyMock.anyObject())).andReturn(15000L);
        mMockDevice.setDate(EasyMock.anyObject());
        EasyMock.replay(mMockDevice);
        assertEquals(CheckStatus.FAILED, mChecker.postExecutionCheck(mMockDevice).getStatus());
        EasyMock.verify(mMockDevice);
    }
}
