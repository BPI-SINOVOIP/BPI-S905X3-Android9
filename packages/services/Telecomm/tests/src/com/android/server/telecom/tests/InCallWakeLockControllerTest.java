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

package com.android.server.telecom.tests;

import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static org.mockito.Mockito.never;

import android.os.PowerManager;
import android.test.suitebuilder.annotation.SmallTest;

import com.android.server.telecom.Call;
import com.android.server.telecom.CallState;
import com.android.server.telecom.CallsManager;
import com.android.server.telecom.InCallWakeLockController;
import com.android.server.telecom.TelecomWakeLock;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.mockito.Mock;

@RunWith(JUnit4.class)
public class InCallWakeLockControllerTest extends TelecomTestCase {

    @Mock CallsManager mCallsManager;
    @Mock Call mCall;
    @Mock TelecomWakeLock.WakeLockAdapter mWakeLockAdapter;
    private InCallWakeLockController mInCallWakeLockController;

    @Override
    @Before
    public void setUp() throws Exception {
        super.setUp();
        TelecomWakeLock telecomWakeLock = new TelecomWakeLock(
                null, //context never used due to mock WakeLockAdapter
                mWakeLockAdapter, PowerManager.FULL_WAKE_LOCK,
                InCallWakeLockControllerTest.class.getSimpleName());
        mInCallWakeLockController = new InCallWakeLockController(telecomWakeLock, mCallsManager);
    }

    @Override
    @After
    public void tearDown() throws Exception {
        mInCallWakeLockController = null;
        super.tearDown();
    }

    @SmallTest
    @Test
    public void testRingingCallAdded() throws Exception {
        when(mCallsManager.getRingingCall()).thenReturn(mCall);
        when(mWakeLockAdapter.isHeld()).thenReturn(false);

        mInCallWakeLockController.onCallAdded(mCall);

        verify(mWakeLockAdapter).acquire();
    }

    @SmallTest
    @Test
    public void testNonRingingCallAdded() throws Exception {
        when(mCallsManager.getRingingCall()).thenReturn(null);
        when(mWakeLockAdapter.isHeld()).thenReturn(false);

        mInCallWakeLockController.onCallAdded(mCall);

        verify(mWakeLockAdapter, never()).acquire();
    }

    @SmallTest
    @Test
    public void testRingingCallTransition() throws Exception {
        when(mCallsManager.getRingingCall()).thenReturn(mCall);
        when(mWakeLockAdapter.isHeld()).thenReturn(false);

        mInCallWakeLockController.onCallStateChanged(mCall, CallState.NEW, CallState.RINGING);

        verify(mWakeLockAdapter).acquire();
    }

    @SmallTest
    @Test
    public void testRingingCallRemoved() throws Exception {
        when(mCallsManager.getRingingCall()).thenReturn(null);
        when(mWakeLockAdapter.isHeld()).thenReturn(false);

        mInCallWakeLockController.onCallRemoved(mCall);

        verify(mWakeLockAdapter, never()).acquire();
    }

    @SmallTest
    @Test
    public void testWakeLockReleased() throws Exception {
        when(mCallsManager.getRingingCall()).thenReturn(null);
        when(mWakeLockAdapter.isHeld()).thenReturn(true);

        mInCallWakeLockController.onCallRemoved(mCall);

        verify(mWakeLockAdapter).release(0);
    }

    @SmallTest
    @Test
    public void testAcquireWakeLockWhenHeld() throws Exception {
        when(mCallsManager.getRingingCall()).thenReturn(mCall);
        when(mWakeLockAdapter.isHeld()).thenReturn(true);

        mInCallWakeLockController.onCallAdded(mock(Call.class));

        verify(mWakeLockAdapter, never()).acquire();
    }
}
