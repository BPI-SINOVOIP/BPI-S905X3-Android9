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
 * limitations under the License
 */

package com.android.server.telecom.tests;

import android.media.AudioManager;
import android.test.suitebuilder.annotation.SmallTest;

import com.android.server.telecom.CallAudioManager;
import com.android.server.telecom.CallAudioModeStateMachine;
import com.android.server.telecom.CallAudioRouteStateMachine;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.mockito.Mock;

import static org.junit.Assert.assertEquals;
import static org.mockito.Matchers.anyInt;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

@RunWith(JUnit4.class)
public class CallAudioModeStateMachineTest extends TelecomTestCase {
    private static final int TEST_TIMEOUT = 1000;

    @Mock private AudioManager mAudioManager;
    @Mock private CallAudioManager mCallAudioManager;

    @Override
    @Before
    public void setUp() throws Exception {
        super.setUp();
    }

    @SmallTest
    @Test
    public void testNoFocusWhenRingerSilenced() throws Throwable {
        CallAudioModeStateMachine sm = new CallAudioModeStateMachine(mAudioManager);
        sm.setCallAudioManager(mCallAudioManager);
        sm.sendMessage(CallAudioModeStateMachine.ABANDON_FOCUS_FOR_TESTING);
        waitForHandlerAction(sm.getHandler(), TEST_TIMEOUT);

        resetMocks();
        when(mCallAudioManager.startRinging()).thenReturn(false);

        sm.sendMessage(CallAudioModeStateMachine.NEW_RINGING_CALL,
                new CallAudioModeStateMachine.MessageArgs(
                        false, // hasActiveOrDialingCalls
                        true, // hasRingingCalls
                        false, // hasHoldingCalls
                        false, // isTonePlaying
                        false, // foregroundCallIsVoip
                        null // session
                ));
        waitForHandlerAction(sm.getHandler(), TEST_TIMEOUT);

        assertEquals(CallAudioModeStateMachine.RING_STATE_NAME, sm.getCurrentStateName());

        verify(mAudioManager, never()).requestAudioFocusForCall(anyInt(), anyInt());
        verify(mAudioManager, never()).setMode(anyInt());

        verify(mCallAudioManager, never()).stopRinging();

        verify(mCallAudioManager).stopCallWaiting();
    }

    @SmallTest
    @Test
    public void testRegainFocusWhenHfpIsConnectedSilenced() throws Throwable {
        CallAudioModeStateMachine sm = new CallAudioModeStateMachine(mAudioManager);
        sm.setCallAudioManager(mCallAudioManager);
        sm.sendMessage(CallAudioModeStateMachine.ABANDON_FOCUS_FOR_TESTING);
        waitForHandlerAction(sm.getHandler(), TEST_TIMEOUT);

        resetMocks();
        when(mCallAudioManager.startRinging()).thenReturn(false);

        sm.sendMessage(CallAudioModeStateMachine.NEW_RINGING_CALL,
                new CallAudioModeStateMachine.MessageArgs(
                        false, // hasActiveOrDialingCalls
                        true, // hasRingingCalls
                        false, // hasHoldingCalls
                        false, // isTonePlaying
                        false, // foregroundCallIsVoip
                        null // session
                ));
        waitForHandlerAction(sm.getHandler(), TEST_TIMEOUT);

        assertEquals(CallAudioModeStateMachine.RING_STATE_NAME, sm.getCurrentStateName());

        verify(mAudioManager, never()).requestAudioFocusForCall(anyInt(), anyInt());
        verify(mAudioManager, never()).setMode(anyInt());

        verify(mCallAudioManager, never()).stopRinging();

        verify(mCallAudioManager).stopCallWaiting();

        when(mCallAudioManager.startRinging()).thenReturn(true);

        sm.sendMessage(CallAudioModeStateMachine.RINGER_MODE_CHANGE);
        waitForHandlerAction(sm.getHandler(), TEST_TIMEOUT);

        verify(mCallAudioManager).startRinging();
        verify(mAudioManager).requestAudioFocusForCall(AudioManager.STREAM_RING,
                AudioManager.AUDIOFOCUS_GAIN_TRANSIENT);
        verify(mAudioManager).setMode(AudioManager.MODE_RINGTONE);
        verify(mCallAudioManager).setCallAudioRouteFocusState(
                CallAudioRouteStateMachine.RINGING_FOCUS);
    }


    private void resetMocks() {
        reset(mCallAudioManager, mAudioManager);
    }
}
