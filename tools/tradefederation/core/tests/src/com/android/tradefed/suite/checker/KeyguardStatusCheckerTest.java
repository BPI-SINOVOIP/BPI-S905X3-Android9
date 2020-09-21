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

import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.suite.checker.StatusCheckerResult.CheckStatus;
import com.android.tradefed.util.KeyguardControllerState;

import org.easymock.EasyMock;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.util.ArrayList;
import java.util.List;

/** Unit tests for {@link KeyguardStatusChecker} */
@RunWith(JUnit4.class)
public class KeyguardStatusCheckerTest {

    private KeyguardStatusChecker mKsc;
    private ITestDevice mMockDevice;

    @Before
    public void setUp() {
        mMockDevice = EasyMock.createMock(ITestDevice.class);
        mKsc = new KeyguardStatusChecker();
    }

    /**
     * Test that {@link KeyguardStatusChecker#postExecutionCheck(ITestDevice)} is passing when
     * keyguard is not showing.
     */
    @Test
    public void testPostExecutionCheck() throws DeviceNotAvailableException {
        EasyMock.expect(mMockDevice.getKeyguardState())
                .andReturn(createKeyguardState(false, false));
        EasyMock.replay(mMockDevice);
        assertEquals(CheckStatus.SUCCESS, mKsc.postExecutionCheck(mMockDevice).getStatus());
        EasyMock.verify(mMockDevice);
    }

    /**
     * Test that {@link KeyguardStatusChecker#postExecutionCheck(ITestDevice)} is failing when the
     * keyguard is showing.
     */
    @Test
    public void testPostExecutionCheck_showingAfter() throws DeviceNotAvailableException {
        EasyMock.expect(mMockDevice.getKeyguardState()).andReturn(createKeyguardState(true, false));
        mMockDevice.disableKeyguard();
        EasyMock.expectLastCall();
        EasyMock.replay(mMockDevice);
        assertEquals(CheckStatus.FAILED, mKsc.postExecutionCheck(mMockDevice).getStatus());
        EasyMock.verify(mMockDevice);
    }

    /**
     * Test that {@link KeyguardStatusChecker#postExecutionCheck(ITestDevice)} is skipping when the
     * keyguard controller is not supported.
     */
    @Test
    public void testPostExecutionCheck_notSupported() throws DeviceNotAvailableException {
        EasyMock.expect(mMockDevice.getKeyguardState()).andReturn(null);
        EasyMock.replay(mMockDevice);
        assertEquals(CheckStatus.SUCCESS, mKsc.postExecutionCheck(mMockDevice).getStatus());
        EasyMock.verify(mMockDevice);
    }

    /** helper to create a response keyguard state from a fake device. */
    private KeyguardControllerState createKeyguardState(boolean showing, boolean occluded) {
        List<String> testOutput = new ArrayList<String>();
        testOutput.add("KeyguardController:");
        testOutput.add(String.format("  mKeyguardShowing=%s", showing));
        testOutput.add("  mKeyguardGoingAway=false");
        testOutput.add(String.format("  mOccluded=%s", occluded));
        return KeyguardControllerState.create(testOutput);
    }
}
