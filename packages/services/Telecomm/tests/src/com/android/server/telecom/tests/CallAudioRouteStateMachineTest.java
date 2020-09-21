/*
 * Copyright (C) 2015 The Android Open Source Project
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

import android.bluetooth.BluetoothDevice;
import android.content.Context;
import android.media.AudioManager;
import android.media.IAudioService;
import android.telecom.CallAudioState;
import android.test.suitebuilder.annotation.MediumTest;
import android.test.suitebuilder.annotation.SmallTest;

import com.android.server.telecom.bluetooth.BluetoothRouteManager;
import com.android.server.telecom.Call;
import com.android.server.telecom.CallAudioRouteStateMachine;
import com.android.server.telecom.CallsManager;
import com.android.server.telecom.ConnectionServiceWrapper;
import com.android.server.telecom.CallAudioManager;
import com.android.server.telecom.StatusBarNotifier;
import com.android.server.telecom.TelecomSystem;
import com.android.server.telecom.WiredHeadsetManager;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.stubbing.Answer;

import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.List;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.nullable;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.same;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.timeout;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

@RunWith(JUnit4.class)
public class CallAudioRouteStateMachineTest extends TelecomSystemTest {

    private static final BluetoothDevice bluetoothDevice1 =
            BluetoothRouteManagerTest.makeBluetoothDevice("00:00:00:00:00:01");
    private static final BluetoothDevice bluetoothDevice2 =
            BluetoothRouteManagerTest.makeBluetoothDevice("00:00:00:00:00:02");
    private static final BluetoothDevice bluetoothDevice3 =
            BluetoothRouteManagerTest.makeBluetoothDevice("00:00:00:00:00:03");

    @Mock CallsManager mockCallsManager;
    @Mock BluetoothRouteManager mockBluetoothRouteManager;
    @Mock IAudioService mockAudioService;
    @Mock ConnectionServiceWrapper mockConnectionServiceWrapper;
    @Mock WiredHeadsetManager mockWiredHeadsetManager;
    @Mock StatusBarNotifier mockStatusBarNotifier;
    @Mock Call fakeCall;
    @Mock CallAudioManager mockCallAudioManager;

    private CallAudioManager.AudioServiceFactory mAudioServiceFactory;
    private static final int TEST_TIMEOUT = 500;
    private AudioManager mockAudioManager;
    private final TelecomSystem.SyncRoot mLock = new TelecomSystem.SyncRoot() { };

    @Override
    @Before
    public void setUp() throws Exception {
        super.setUp();
        MockitoAnnotations.initMocks(this);
        mContext = mComponentContextFixture.getTestDouble().getApplicationContext();
        mockAudioManager = (AudioManager) mContext.getSystemService(Context.AUDIO_SERVICE);

        mAudioServiceFactory = new CallAudioManager.AudioServiceFactory() {
            @Override
            public IAudioService getAudioService() {
                return mockAudioService;
            }
        };

        when(mockCallsManager.getForegroundCall()).thenReturn(fakeCall);
        when(mockCallsManager.getLock()).thenReturn(mLock);
        when(mockCallsManager.hasVideoCall()).thenReturn(false);
        when(fakeCall.getConnectionService()).thenReturn(mockConnectionServiceWrapper);
        when(fakeCall.isAlive()).thenReturn(true);
        when(fakeCall.getSupportedAudioRoutes()).thenReturn(CallAudioState.ROUTE_ALL);

        doNothing().when(mockConnectionServiceWrapper).onCallAudioStateChanged(any(Call.class),
                any(CallAudioState.class));
    }

    @SmallTest
    @Test
    public void testEarpieceAutodetect() {
        CallAudioRouteStateMachine stateMachine = new CallAudioRouteStateMachine(
                mContext,
                mockCallsManager,
                mockBluetoothRouteManager,
                mockWiredHeadsetManager,
                mockStatusBarNotifier,
                mAudioServiceFactory,
                CallAudioRouteStateMachine.EARPIECE_AUTO_DETECT);

        // Since we don't know if we're on a platform with an earpiece or not, all we can do
        // is ensure the stateMachine construction didn't fail.  But at least we exercised the
        // autodetection code...
        assertNotNull(stateMachine);
    }

    @MediumTest
    @Test
    public void testSpeakerPersistence() {
        CallAudioRouteStateMachine stateMachine = new CallAudioRouteStateMachine(
                mContext,
                mockCallsManager,
                mockBluetoothRouteManager,
                mockWiredHeadsetManager,
                mockStatusBarNotifier,
                mAudioServiceFactory,
                CallAudioRouteStateMachine.EARPIECE_FORCE_ENABLED);

        when(mockBluetoothRouteManager.isBluetoothAudioConnectedOrPending()).thenReturn(false);
        when(mockBluetoothRouteManager.isBluetoothAvailable()).thenReturn(true);
        when(mockAudioManager.isSpeakerphoneOn()).thenReturn(true);
        doAnswer(new Answer() {
            @Override
            public Object answer(InvocationOnMock invocation) throws Throwable {
                Object[] args = invocation.getArguments();
                when(mockAudioManager.isSpeakerphoneOn()).thenReturn((Boolean) args[0]);
                return null;
            }
        }).when(mockAudioManager).setSpeakerphoneOn(any(Boolean.class));
        CallAudioState initState = new CallAudioState(false, CallAudioState.ROUTE_SPEAKER,
                CallAudioState.ROUTE_EARPIECE | CallAudioState.ROUTE_SPEAKER);
        stateMachine.initialize(initState);

        stateMachine.sendMessageWithSessionInfo(CallAudioRouteStateMachine.SWITCH_FOCUS,
                CallAudioRouteStateMachine.ACTIVE_FOCUS);
        stateMachine.sendMessageWithSessionInfo(CallAudioRouteStateMachine.CONNECT_WIRED_HEADSET);
        CallAudioState expectedMiddleState = new CallAudioState(false,
                CallAudioState.ROUTE_WIRED_HEADSET,
                CallAudioState.ROUTE_WIRED_HEADSET | CallAudioState.ROUTE_SPEAKER);
        waitForHandlerAction(stateMachine.getHandler(), TEST_TIMEOUT);
        waitForHandlerAction(stateMachine.getHandler(), TEST_TIMEOUT);
        verifyNewSystemCallAudioState(initState, expectedMiddleState);
        resetMocks();

        stateMachine.sendMessageWithSessionInfo(
                CallAudioRouteStateMachine.DISCONNECT_WIRED_HEADSET);
        waitForHandlerAction(stateMachine.getHandler(), TEST_TIMEOUT);
        verifyNewSystemCallAudioState(expectedMiddleState, initState);
    }

    @MediumTest
    @Test
    public void testUserBluetoothSwitchOff() {
        CallAudioRouteStateMachine stateMachine = new CallAudioRouteStateMachine(
                mContext,
                mockCallsManager,
                mockBluetoothRouteManager,
                mockWiredHeadsetManager,
                mockStatusBarNotifier,
                mAudioServiceFactory,
                CallAudioRouteStateMachine.EARPIECE_FORCE_ENABLED);
        stateMachine.setCallAudioManager(mockCallAudioManager);

        when(mockBluetoothRouteManager.isBluetoothAudioConnectedOrPending()).thenReturn(false);
        when(mockBluetoothRouteManager.isBluetoothAvailable()).thenReturn(true);
        when(mockAudioManager.isSpeakerphoneOn()).thenReturn(true);

        CallAudioState initState = new CallAudioState(false, CallAudioState.ROUTE_BLUETOOTH,
                CallAudioState.ROUTE_EARPIECE | CallAudioState.ROUTE_BLUETOOTH);
        stateMachine.initialize(initState);

        stateMachine.sendMessageWithSessionInfo(CallAudioRouteStateMachine.SWITCH_FOCUS,
                CallAudioRouteStateMachine.ACTIVE_FOCUS);
        stateMachine.sendMessageWithSessionInfo(CallAudioRouteStateMachine.BT_AUDIO_CONNECTED);
        stateMachine.sendMessageWithSessionInfo(
                CallAudioRouteStateMachine.USER_SWITCH_BASELINE_ROUTE,
                CallAudioRouteStateMachine.NO_INCLUDE_BLUETOOTH_IN_BASELINE);
        CallAudioState expectedEndState = new CallAudioState(false,
                CallAudioState.ROUTE_EARPIECE,
                CallAudioState.ROUTE_EARPIECE | CallAudioState.ROUTE_BLUETOOTH);

        waitForHandlerAction(stateMachine.getHandler(), TEST_TIMEOUT);
        verifyNewSystemCallAudioState(initState, expectedEndState);
        resetMocks();
        stateMachine.sendMessageWithSessionInfo(
                CallAudioRouteStateMachine.BT_ACTIVE_DEVICE_GONE);
        stateMachine.sendMessageWithSessionInfo(
                CallAudioRouteStateMachine.BT_ACTIVE_DEVICE_PRESENT);

        waitForHandlerAction(stateMachine.getHandler(), TEST_TIMEOUT);
        assertEquals(expectedEndState, stateMachine.getCurrentCallAudioState());
    }

    @MediumTest
    @Test
    public void testUserBluetoothSwitchOffAndOnAgain() {
        CallAudioRouteStateMachine stateMachine = new CallAudioRouteStateMachine(
                mContext,
                mockCallsManager,
                mockBluetoothRouteManager,
                mockWiredHeadsetManager,
                mockStatusBarNotifier,
                mAudioServiceFactory,
                CallAudioRouteStateMachine.EARPIECE_FORCE_ENABLED);
        stateMachine.setCallAudioManager(mockCallAudioManager);
        Collection<BluetoothDevice> availableDevices = Collections.singleton(bluetoothDevice1);

        when(mockBluetoothRouteManager.isBluetoothAudioConnectedOrPending()).thenReturn(false);
        when(mockBluetoothRouteManager.isBluetoothAvailable()).thenReturn(true);
        when(mockBluetoothRouteManager.getConnectedDevices()).thenReturn(availableDevices);

        CallAudioState initState = new CallAudioState(false, CallAudioState.ROUTE_BLUETOOTH,
                CallAudioState.ROUTE_EARPIECE | CallAudioState.ROUTE_BLUETOOTH);
        stateMachine.initialize(initState);

        stateMachine.sendMessageWithSessionInfo(CallAudioRouteStateMachine.SWITCH_FOCUS,
                CallAudioRouteStateMachine.ACTIVE_FOCUS);
        stateMachine.sendMessageWithSessionInfo(CallAudioRouteStateMachine.BT_AUDIO_CONNECTED);

        // Switch off the BT route explicitly.
        stateMachine.sendMessageWithSessionInfo(
                CallAudioRouteStateMachine.USER_SWITCH_BASELINE_ROUTE,
                CallAudioRouteStateMachine.NO_INCLUDE_BLUETOOTH_IN_BASELINE);
        CallAudioState expectedMidState = new CallAudioState(false,
                CallAudioState.ROUTE_EARPIECE,
                CallAudioState.ROUTE_EARPIECE | CallAudioState.ROUTE_BLUETOOTH,
                null, availableDevices);

        waitForHandlerAction(stateMachine.getHandler(), TEST_TIMEOUT);
        verifyNewSystemCallAudioState(initState, expectedMidState);
        resetMocks();

        // Now, switch back to BT explicitly
        when(mockBluetoothRouteManager.isBluetoothAudioConnectedOrPending()).thenReturn(false);
        when(mockBluetoothRouteManager.isBluetoothAvailable()).thenReturn(true);
        when(mockBluetoothRouteManager.getConnectedDevices()).thenReturn(availableDevices);
        doAnswer(invocation -> {
            when(mockBluetoothRouteManager.getBluetoothAudioConnectedDevice())
                    .thenReturn(bluetoothDevice1);
            stateMachine.sendMessageWithSessionInfo(CallAudioRouteStateMachine.BT_AUDIO_CONNECTED);
            return null;
        }).when(mockBluetoothRouteManager).connectBluetoothAudio(nullable(String.class));
        stateMachine.sendMessageWithSessionInfo(CallAudioRouteStateMachine.USER_SWITCH_BLUETOOTH);
        CallAudioState expectedEndState = new CallAudioState(false,
                CallAudioState.ROUTE_BLUETOOTH,
                CallAudioState.ROUTE_EARPIECE | CallAudioState.ROUTE_BLUETOOTH,
                bluetoothDevice1, availableDevices);
        waitForHandlerAction(stateMachine.getHandler(), TEST_TIMEOUT);
        // second wait needed for the BT_AUDIO_CONNECTED message
        waitForHandlerAction(stateMachine.getHandler(), TEST_TIMEOUT);
        verifyNewSystemCallAudioState(expectedMidState, expectedEndState);

        stateMachine.sendMessageWithSessionInfo(
                CallAudioRouteStateMachine.BT_ACTIVE_DEVICE_GONE);
        when(mockBluetoothRouteManager.getBluetoothAudioConnectedDevice())
                .thenReturn(null);
        stateMachine.sendMessageWithSessionInfo(
                CallAudioRouteStateMachine.BT_ACTIVE_DEVICE_PRESENT);

        waitForHandlerAction(stateMachine.getHandler(), TEST_TIMEOUT);
        // second wait needed for the BT_AUDIO_CONNECTED message
        waitForHandlerAction(stateMachine.getHandler(), TEST_TIMEOUT);
        // Verify that we're still on bluetooth.
        assertEquals(expectedEndState, stateMachine.getCurrentCallAudioState());
    }

    @MediumTest
    @Test
    public void testBluetoothRinging() {
        CallAudioRouteStateMachine stateMachine = new CallAudioRouteStateMachine(
                mContext,
                mockCallsManager,
                mockBluetoothRouteManager,
                mockWiredHeadsetManager,
                mockStatusBarNotifier,
                mAudioServiceFactory,
                CallAudioRouteStateMachine.EARPIECE_FORCE_ENABLED);
        stateMachine.setCallAudioManager(mockCallAudioManager);

        when(mockBluetoothRouteManager.isBluetoothAudioConnectedOrPending()).thenReturn(false);
        when(mockBluetoothRouteManager.isBluetoothAvailable()).thenReturn(true);
        when(mockBluetoothRouteManager.getConnectedDevices())
                .thenReturn(Collections.singletonList(bluetoothDevice1));
        when(mockAudioManager.isSpeakerphoneOn()).thenReturn(false);

        CallAudioState initState = new CallAudioState(false, CallAudioState.ROUTE_BLUETOOTH,
                CallAudioState.ROUTE_EARPIECE | CallAudioState.ROUTE_BLUETOOTH);
        stateMachine.initialize(initState);

        stateMachine.sendMessageWithSessionInfo(CallAudioRouteStateMachine.SWITCH_FOCUS,
                CallAudioRouteStateMachine.RINGING_FOCUS);
        waitForHandlerAction(stateMachine.getHandler(), TEST_TIMEOUT);

        verify(mockBluetoothRouteManager, never()).connectBluetoothAudio(nullable(String.class));

        stateMachine.sendMessageWithSessionInfo(CallAudioRouteStateMachine.SWITCH_FOCUS,
                CallAudioRouteStateMachine.ACTIVE_FOCUS);
        waitForHandlerAction(stateMachine.getHandler(), TEST_TIMEOUT);
        verify(mockBluetoothRouteManager, times(1)).connectBluetoothAudio(nullable(String.class));
    }

    @MediumTest
    @Test
    public void testConnectBluetoothDuringRinging() {
        CallAudioRouteStateMachine stateMachine = new CallAudioRouteStateMachine(
                mContext,
                mockCallsManager,
                mockBluetoothRouteManager,
                mockWiredHeadsetManager,
                mockStatusBarNotifier,
                mAudioServiceFactory,
                CallAudioRouteStateMachine.EARPIECE_FORCE_ENABLED);
        stateMachine.setCallAudioManager(mockCallAudioManager);
        setInBandRing(false);
        when(mockBluetoothRouteManager.isBluetoothAudioConnectedOrPending()).thenReturn(false);
        when(mockBluetoothRouteManager.isBluetoothAvailable()).thenReturn(false);
        when(mockAudioManager.isSpeakerphoneOn()).thenReturn(false);
        CallAudioState initState = new CallAudioState(false, CallAudioState.ROUTE_EARPIECE,
                CallAudioState.ROUTE_EARPIECE);
        stateMachine.initialize(initState);

        stateMachine.sendMessageWithSessionInfo(CallAudioRouteStateMachine.SWITCH_FOCUS,
                CallAudioRouteStateMachine.RINGING_FOCUS);
        // Wait for the state machine to finish transiting to ActiveEarpiece before hooking up
        // bluetooth mocks
        waitForHandlerAction(stateMachine.getHandler(), TEST_TIMEOUT);

        when(mockBluetoothRouteManager.isBluetoothAvailable()).thenReturn(true);
        when(mockBluetoothRouteManager.getConnectedDevices())
                .thenReturn(Collections.singletonList(bluetoothDevice1));
        stateMachine.sendMessageWithSessionInfo(
                CallAudioRouteStateMachine.BLUETOOTH_DEVICE_LIST_CHANGED);
        stateMachine.sendMessageWithSessionInfo(
                CallAudioRouteStateMachine.BT_ACTIVE_DEVICE_PRESENT);
        waitForHandlerAction(stateMachine.getHandler(), TEST_TIMEOUT);

        verify(mockBluetoothRouteManager, never()).connectBluetoothAudio(null);
        CallAudioState expectedEndState = new CallAudioState(false,
                CallAudioState.ROUTE_BLUETOOTH,
                CallAudioState.ROUTE_EARPIECE | CallAudioState.ROUTE_BLUETOOTH,
                null, Collections.singletonList(bluetoothDevice1));
        verifyNewSystemCallAudioState(initState, expectedEndState);

        stateMachine.sendMessageWithSessionInfo(CallAudioRouteStateMachine.SWITCH_FOCUS,
                CallAudioRouteStateMachine.ACTIVE_FOCUS);
        waitForHandlerAction(stateMachine.getHandler(), TEST_TIMEOUT);
        verify(mockBluetoothRouteManager, times(1)).connectBluetoothAudio(null);

        when(mockBluetoothRouteManager.getBluetoothAudioConnectedDevice())
                .thenReturn(bluetoothDevice1);
        stateMachine.sendMessage(CallAudioRouteStateMachine.BT_AUDIO_CONNECTED);
        waitForHandlerAction(stateMachine.getHandler(), TEST_TIMEOUT);
        verify(mockCallAudioManager, times(1)).onRingerModeChange();
    }

    @SmallTest
    @Test
    public void testConnectSpecificBluetoothDevice() {
        CallAudioRouteStateMachine stateMachine = new CallAudioRouteStateMachine(
                mContext,
                mockCallsManager,
                mockBluetoothRouteManager,
                mockWiredHeadsetManager,
                mockStatusBarNotifier,
                mAudioServiceFactory,
                CallAudioRouteStateMachine.EARPIECE_FORCE_ENABLED);
        stateMachine.setCallAudioManager(mockCallAudioManager);
        List<BluetoothDevice> availableDevices =
                Arrays.asList(bluetoothDevice1, bluetoothDevice2, bluetoothDevice3);

        when(mockAudioManager.isSpeakerphoneOn()).thenReturn(false);
        when(mockBluetoothRouteManager.isBluetoothAudioConnectedOrPending()).thenReturn(false);
        when(mockBluetoothRouteManager.isBluetoothAvailable()).thenReturn(true);
        when(mockBluetoothRouteManager.getConnectedDevices()).thenReturn(availableDevices);
        doAnswer(invocation -> {
            when(mockBluetoothRouteManager.getBluetoothAudioConnectedDevice())
                    .thenReturn(bluetoothDevice2);
            stateMachine.sendMessageWithSessionInfo(CallAudioRouteStateMachine.BT_AUDIO_CONNECTED);
            return null;
        }).when(mockBluetoothRouteManager).connectBluetoothAudio(bluetoothDevice2.getAddress());

        CallAudioState initState = new CallAudioState(false, CallAudioState.ROUTE_EARPIECE,
                CallAudioState.ROUTE_EARPIECE | CallAudioState.ROUTE_BLUETOOTH, null,
                availableDevices);
        stateMachine.initialize(initState);

        stateMachine.sendMessageWithSessionInfo(CallAudioRouteStateMachine.SWITCH_FOCUS,
                CallAudioRouteStateMachine.ACTIVE_FOCUS);

        stateMachine.sendMessageWithSessionInfo(CallAudioRouteStateMachine.USER_SWITCH_BLUETOOTH,
                0, bluetoothDevice2.getAddress());
        waitForHandlerAction(stateMachine.getHandler(), TEST_TIMEOUT);

        verify(mockBluetoothRouteManager).connectBluetoothAudio(bluetoothDevice2.getAddress());
        waitForHandlerAction(stateMachine.getHandler(), TEST_TIMEOUT);
        CallAudioState expectedEndState = new CallAudioState(false,
                CallAudioState.ROUTE_BLUETOOTH,
                CallAudioState.ROUTE_EARPIECE | CallAudioState.ROUTE_BLUETOOTH,
                bluetoothDevice2,
                availableDevices);

        verifyNewSystemCallAudioState(initState, expectedEndState);
    }

    @SmallTest
    @Test
    public void testFocusChangeWithAlreadyActiveBtDevice() {
        CallAudioRouteStateMachine stateMachine = new CallAudioRouteStateMachine(
                mContext,
                mockCallsManager,
                mockBluetoothRouteManager,
                mockWiredHeadsetManager,
                mockStatusBarNotifier,
                mAudioServiceFactory,
                CallAudioRouteStateMachine.EARPIECE_FORCE_ENABLED);
        stateMachine.setCallAudioManager(mockCallAudioManager);
        List<BluetoothDevice> availableDevices =
                Arrays.asList(bluetoothDevice1, bluetoothDevice2);

        // Set up a state where there's an HFP connected bluetooth device already.
        when(mockAudioManager.isSpeakerphoneOn()).thenReturn(false);
        when(mockBluetoothRouteManager.isBluetoothAudioConnectedOrPending()).thenReturn(true);
        when(mockBluetoothRouteManager.isBluetoothAvailable()).thenReturn(true);
        when(mockBluetoothRouteManager.getConnectedDevices()).thenReturn(availableDevices);
        when(mockBluetoothRouteManager.getBluetoothAudioConnectedDevice())
                .thenReturn(bluetoothDevice1);

        // We want to be in the QuiescentBluetoothRoute because there's no call yet
        CallAudioState initState = new CallAudioState(false, CallAudioState.ROUTE_BLUETOOTH,
                CallAudioState.ROUTE_EARPIECE | CallAudioState.ROUTE_BLUETOOTH,
                bluetoothDevice1, availableDevices);
        stateMachine.initialize(initState);

        // Switch to active, pretending that a call came in.
        stateMachine.sendMessageWithSessionInfo(CallAudioRouteStateMachine.SWITCH_FOCUS,
                CallAudioRouteStateMachine.ACTIVE_FOCUS);
        waitForHandlerAction(stateMachine.getHandler(), TEST_TIMEOUT);

        // Make sure that we've successfully switched to the active BT route without actually
        // calling connectAudio.
        verify(mockBluetoothRouteManager, never()).connectBluetoothAudio(nullable(String.class));
        assertTrue(stateMachine.isInActiveState());
    }

    @SmallTest
    @Test
    public void testInitializationWithEarpieceNoHeadsetNoBluetooth() {
        CallAudioState expectedState = new CallAudioState(false, CallAudioState.ROUTE_EARPIECE,
                CallAudioState.ROUTE_EARPIECE | CallAudioState.ROUTE_SPEAKER);
        initializationTestHelper(expectedState, CallAudioRouteStateMachine.EARPIECE_FORCE_ENABLED);
    }

    @SmallTest
    @Test
    public void testInitializationWithEarpieceAndHeadsetNoBluetooth() {
        CallAudioState expectedState = new CallAudioState(false, CallAudioState.ROUTE_WIRED_HEADSET,
                CallAudioState.ROUTE_WIRED_HEADSET | CallAudioState.ROUTE_SPEAKER);
        initializationTestHelper(expectedState, CallAudioRouteStateMachine.EARPIECE_FORCE_ENABLED);
    }

    @SmallTest
    @Test
    public void testInitializationWithEarpieceAndHeadsetAndBluetooth() {
        CallAudioState expectedState = new CallAudioState(false, CallAudioState.ROUTE_BLUETOOTH,
                CallAudioState.ROUTE_WIRED_HEADSET | CallAudioState.ROUTE_SPEAKER
                | CallAudioState.ROUTE_BLUETOOTH);
        initializationTestHelper(expectedState, CallAudioRouteStateMachine.EARPIECE_FORCE_ENABLED);
    }

    @SmallTest
    @Test
    public void testInitializationWithEarpieceAndBluetoothNoHeadset() {
        CallAudioState expectedState = new CallAudioState(false, CallAudioState.ROUTE_BLUETOOTH,
                CallAudioState.ROUTE_EARPIECE | CallAudioState.ROUTE_SPEAKER
                        | CallAudioState.ROUTE_BLUETOOTH);
        initializationTestHelper(expectedState, CallAudioRouteStateMachine.EARPIECE_FORCE_ENABLED);
    }

    @SmallTest
    @Test
    public void testInitializationWithNoEarpieceNoHeadsetNoBluetooth() {
        CallAudioState expectedState = new CallAudioState(false, CallAudioState.ROUTE_SPEAKER,
                CallAudioState.ROUTE_SPEAKER);
        initializationTestHelper(expectedState, CallAudioRouteStateMachine.EARPIECE_FORCE_DISABLED);
    }

    @SmallTest
    @Test
    public void testInitializationWithHeadsetNoBluetoothNoEarpiece() {
        CallAudioState expectedState = new CallAudioState(false, CallAudioState.ROUTE_WIRED_HEADSET,
                CallAudioState.ROUTE_WIRED_HEADSET | CallAudioState.ROUTE_SPEAKER);
        initializationTestHelper(expectedState, CallAudioRouteStateMachine.EARPIECE_FORCE_DISABLED);
    }

    @SmallTest
    @Test
    public void testInitializationWithHeadsetAndBluetoothNoEarpiece() {
        CallAudioState expectedState = new CallAudioState(false, CallAudioState.ROUTE_BLUETOOTH,
                CallAudioState.ROUTE_WIRED_HEADSET | CallAudioState.ROUTE_SPEAKER
                | CallAudioState.ROUTE_BLUETOOTH);
        initializationTestHelper(expectedState, CallAudioRouteStateMachine.EARPIECE_FORCE_DISABLED);
    }

    @SmallTest
    @Test
    public void testInitializationWithBluetoothNoHeadsetNoEarpiece() {
        CallAudioState expectedState = new CallAudioState(false, CallAudioState.ROUTE_BLUETOOTH,
                CallAudioState.ROUTE_SPEAKER | CallAudioState.ROUTE_BLUETOOTH);
        initializationTestHelper(expectedState, CallAudioRouteStateMachine.EARPIECE_FORCE_DISABLED);
    }

    private void initializationTestHelper(CallAudioState expectedState,
            int earpieceControl) {
        when(mockWiredHeadsetManager.isPluggedIn()).thenReturn(
                (expectedState.getSupportedRouteMask() & CallAudioState.ROUTE_WIRED_HEADSET) != 0);
        when(mockBluetoothRouteManager.isBluetoothAvailable()).thenReturn(
                (expectedState.getSupportedRouteMask() & CallAudioState.ROUTE_BLUETOOTH) != 0);

        CallAudioRouteStateMachine stateMachine = new CallAudioRouteStateMachine(
                mContext,
                mockCallsManager,
                mockBluetoothRouteManager,
                mockWiredHeadsetManager,
                mockStatusBarNotifier,
                mAudioServiceFactory,
                earpieceControl);
        stateMachine.initialize();
        assertEquals(expectedState, stateMachine.getCurrentCallAudioState());
    }

    private void verifyNewSystemCallAudioState(CallAudioState expectedOldState,
            CallAudioState expectedNewState) {
        ArgumentCaptor<CallAudioState> oldStateCaptor = ArgumentCaptor.forClass(
                CallAudioState.class);
        ArgumentCaptor<CallAudioState> newStateCaptor1 = ArgumentCaptor.forClass(
                CallAudioState.class);
        ArgumentCaptor<CallAudioState> newStateCaptor2 = ArgumentCaptor.forClass(
                CallAudioState.class);
        verify(mockCallsManager, timeout(TEST_TIMEOUT).atLeastOnce()).onCallAudioStateChanged(
                oldStateCaptor.capture(), newStateCaptor1.capture());
        verify(mockConnectionServiceWrapper, timeout(TEST_TIMEOUT).atLeastOnce())
                .onCallAudioStateChanged(same(fakeCall), newStateCaptor2.capture());

        assertTrue(oldStateCaptor.getAllValues().get(0).equals(expectedOldState));
        assertTrue(newStateCaptor1.getValue().equals(expectedNewState));
        assertTrue(newStateCaptor2.getValue().equals(expectedNewState));
    }

    private void setInBandRing(boolean enabled) {
        when(mockBluetoothRouteManager.isInbandRingingEnabled()).thenReturn(enabled);
    }

    private void resetMocks() {
        reset(mockAudioManager, mockBluetoothRouteManager, mockCallsManager,
                mockConnectionServiceWrapper);
        fakeCall = mock(Call.class);
        when(mockCallsManager.getForegroundCall()).thenReturn(fakeCall);
        when(fakeCall.getConnectionService()).thenReturn(mockConnectionServiceWrapper);
        when(fakeCall.isAlive()).thenReturn(true);
        when(fakeCall.getSupportedAudioRoutes()).thenReturn(CallAudioState.ROUTE_ALL);
        when(mockCallsManager.getLock()).thenReturn(mLock);
        doNothing().when(mockConnectionServiceWrapper).onCallAudioStateChanged(any(Call.class),
                any(CallAudioState.class));
    }
}
