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
 * limitations under the License
 */

package com.android.server.telecom.tests;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyBoolean;
import static org.mockito.ArgumentMatchers.anyChar;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.content.ComponentName;
import android.net.Uri;
import android.os.SystemClock;
import android.telecom.Connection;
import android.telecom.PhoneAccount;
import android.telecom.PhoneAccountHandle;
import android.telecom.VideoProfile;
import android.telephony.TelephonyManager;
import android.test.suitebuilder.annotation.MediumTest;

import android.test.suitebuilder.annotation.SmallTest;
import com.android.server.telecom.AsyncRingtonePlayer;
import com.android.server.telecom.Call;
import com.android.server.telecom.CallAudioManager;
import com.android.server.telecom.CallState;
import com.android.server.telecom.CallerInfoAsyncQueryFactory;
import com.android.server.telecom.CallsManager;
import com.android.server.telecom.ClockProxy;
import com.android.server.telecom.ConnectionServiceFocusManager;
import com.android.server.telecom.ConnectionServiceFocusManager.ConnectionServiceFocusManagerFactory;
import com.android.server.telecom.ConnectionServiceWrapper;
import com.android.server.telecom.ContactsAsyncHelper;
import com.android.server.telecom.DefaultDialerCache;
import com.android.server.telecom.EmergencyCallHelper;
import com.android.server.telecom.HeadsetMediaButton;
import com.android.server.telecom.HeadsetMediaButtonFactory;
import com.android.server.telecom.InCallController;
import com.android.server.telecom.InCallControllerFactory;
import com.android.server.telecom.InCallTonePlayer;
import com.android.server.telecom.InCallWakeLockController;
import com.android.server.telecom.InCallWakeLockControllerFactory;
import com.android.server.telecom.MissedCallNotifier;
import com.android.server.telecom.PhoneAccountRegistrar;
import com.android.server.telecom.PhoneNumberUtilsAdapter;
import com.android.server.telecom.ProximitySensorManager;
import com.android.server.telecom.ProximitySensorManagerFactory;
import com.android.server.telecom.SystemStateProvider;
import com.android.server.telecom.TelecomSystem;
import com.android.server.telecom.Timeouts;
import com.android.server.telecom.WiredHeadsetManager;
import com.android.server.telecom.bluetooth.BluetoothRouteManager;
import com.android.server.telecom.bluetooth.BluetoothStateReceiver;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.mockito.ArgumentCaptor;
import org.mockito.Matchers;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.MockitoAnnotations;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

@RunWith(JUnit4.class)
public class CallsManagerTest extends TelecomTestCase {
    private static final PhoneAccountHandle SIM_1_HANDLE = new PhoneAccountHandle(
            ComponentName.unflattenFromString("com.foo/.Blah"), "Sim1");
    private static final PhoneAccountHandle SIM_2_HANDLE = new PhoneAccountHandle(
            ComponentName.unflattenFromString("com.foo/.Blah"), "Sim2");
    private static final PhoneAccountHandle SELF_MANAGED_HANDLE = new PhoneAccountHandle(
            ComponentName.unflattenFromString("com.foo/.Self"), "Self");
    private static final PhoneAccount SIM_1_ACCOUNT = new PhoneAccount.Builder(SIM_1_HANDLE, "Sim1")
            .setCapabilities(PhoneAccount.CAPABILITY_SIM_SUBSCRIPTION
                    | PhoneAccount.CAPABILITY_CALL_PROVIDER)
            .setIsEnabled(true)
            .build();
    private static final PhoneAccount SIM_2_ACCOUNT = new PhoneAccount.Builder(SIM_2_HANDLE, "Sim2")
            .setCapabilities(PhoneAccount.CAPABILITY_SIM_SUBSCRIPTION
                    | PhoneAccount.CAPABILITY_CALL_PROVIDER
                    | PhoneAccount.CAPABILITY_SUPPORTS_VIDEO_CALLING)
            .setIsEnabled(true)
            .build();
    private static final PhoneAccount SELF_MANAGED_ACCOUNT = new PhoneAccount.Builder(
            SELF_MANAGED_HANDLE, "Self")
            .setCapabilities(PhoneAccount.CAPABILITY_SELF_MANAGED)
            .setIsEnabled(true)
            .build();
    private static final Uri TEST_ADDRESS = Uri.parse("tel:555-1212");

    private final TelecomSystem.SyncRoot mLock = new TelecomSystem.SyncRoot() { };
    @Mock private ContactsAsyncHelper mContactsAsyncHelper;
    @Mock private CallerInfoAsyncQueryFactory mCallerInfoAsyncQueryFactory;
    @Mock private MissedCallNotifier mMissedCallNotifier;
    @Mock private PhoneAccountRegistrar mPhoneAccountRegistrar;
    @Mock private HeadsetMediaButton mHeadsetMediaButton;
    @Mock private HeadsetMediaButtonFactory mHeadsetMediaButtonFactory;
    @Mock private ProximitySensorManager mProximitySensorManager;
    @Mock private ProximitySensorManagerFactory mProximitySensorManagerFactory;
    @Mock private InCallWakeLockController mInCallWakeLockController;
    @Mock private ConnectionServiceFocusManagerFactory mConnSvrFocusManagerFactory;
    @Mock private InCallWakeLockControllerFactory mInCallWakeLockControllerFactory;
    @Mock private CallAudioManager.AudioServiceFactory mAudioServiceFactory;
    @Mock private BluetoothRouteManager mBluetoothRouteManager;
    @Mock private WiredHeadsetManager mWiredHeadsetManager;
    @Mock private SystemStateProvider mSystemStateProvider;
    @Mock private DefaultDialerCache mDefaultDialerCache;
    @Mock private Timeouts.Adapter mTimeoutsAdapter;
    @Mock private AsyncRingtonePlayer mAsyncRingtonePlayer;
    @Mock private PhoneNumberUtilsAdapter mPhoneNumberUtilsAdapter;
    @Mock private EmergencyCallHelper mEmergencyCallHelper;
    @Mock private InCallTonePlayer.ToneGeneratorFactory mToneGeneratorFactory;
    @Mock private ClockProxy mClockProxy;
    @Mock private InCallControllerFactory mInCallControllerFactory;
    @Mock private InCallController mInCallController;
    @Mock private ConnectionServiceFocusManager mConnectionSvrFocusMgr;
    @Mock private BluetoothStateReceiver mBluetoothStateReceiver;

    private CallsManager mCallsManager;

    @Override
    @Before
    public void setUp() throws Exception {
        super.setUp();
        MockitoAnnotations.initMocks(this);
        when(mInCallWakeLockControllerFactory.create(any(), any())).thenReturn(
                mInCallWakeLockController);
        when(mHeadsetMediaButtonFactory.create(any(), any(), any())).thenReturn(
                mHeadsetMediaButton);
        when(mProximitySensorManagerFactory.create(any(), any())).thenReturn(
                mProximitySensorManager);
        when(mInCallControllerFactory.create(any(), any(), any(), any(), any(), any(),
                any())).thenReturn(mInCallController);
        when(mClockProxy.currentTimeMillis()).thenReturn(System.currentTimeMillis());
        when(mClockProxy.elapsedRealtime()).thenReturn(SystemClock.elapsedRealtime());
        when(mConnSvrFocusManagerFactory.create(any(), any())).thenReturn(mConnectionSvrFocusMgr);
        mCallsManager = new CallsManager(
                mComponentContextFixture.getTestDouble().getApplicationContext(),
                mLock,
                mContactsAsyncHelper,
                mCallerInfoAsyncQueryFactory,
                mMissedCallNotifier,
                mPhoneAccountRegistrar,
                mHeadsetMediaButtonFactory,
                mProximitySensorManagerFactory,
                mInCallWakeLockControllerFactory,
                mConnSvrFocusManagerFactory,
                mAudioServiceFactory,
                mBluetoothRouteManager,
                mWiredHeadsetManager,
                mSystemStateProvider,
                mDefaultDialerCache,
                mTimeoutsAdapter,
                mAsyncRingtonePlayer,
                mPhoneNumberUtilsAdapter,
                mEmergencyCallHelper,
                mToneGeneratorFactory,
                mClockProxy,
                mBluetoothStateReceiver,
                mInCallControllerFactory);

        when(mPhoneAccountRegistrar.getPhoneAccount(
                eq(SELF_MANAGED_HANDLE), any())).thenReturn(SELF_MANAGED_ACCOUNT);
        when(mPhoneAccountRegistrar.getPhoneAccount(
                eq(SIM_1_HANDLE), any())).thenReturn(SIM_1_ACCOUNT);
        when(mPhoneAccountRegistrar.getPhoneAccount(
                eq(SIM_2_HANDLE), any())).thenReturn(SIM_2_ACCOUNT);
    }

    @MediumTest
    @Test
    public void testConstructPossiblePhoneAccounts() throws Exception {
        // Should be empty since the URI is null.
        assertEquals(0, mCallsManager.constructPossiblePhoneAccounts(null, null, false).size());
    }

    /**
     * Verify behavior for multisim devices where we want to ensure that the active sim is used for
     * placing a new call.
     * @throws Exception
     */
    @MediumTest
    @Test
    public void testConstructPossiblePhoneAccountsMultiSimActive() throws Exception {
        setupMsimAccounts();

        Call ongoingCall = new Call(
                "1", /* callId */
                mComponentContextFixture.getTestDouble(),
                mCallsManager,
                mLock,
                null /* ConnectionServiceRepository */,
                mContactsAsyncHelper,
                mCallerInfoAsyncQueryFactory,
                mPhoneNumberUtilsAdapter,
                TEST_ADDRESS,
                null /* GatewayInfo */,
                null /* connectionManagerPhoneAccountHandle */,
                SIM_2_HANDLE,
                Call.CALL_DIRECTION_INCOMING,
                false /* shouldAttachToExistingConnection*/,
                false /* isConference */,
                mClockProxy);
        ongoingCall.setState(CallState.ACTIVE, "just cuz");
        mCallsManager.addCall(ongoingCall);

        List<PhoneAccountHandle> phoneAccountHandles = mCallsManager.constructPossiblePhoneAccounts(
                TEST_ADDRESS, null, false);
        assertEquals(1, phoneAccountHandles.size());
        assertEquals(SIM_2_HANDLE, phoneAccountHandles.get(0));
    }

    /**
     * Verify behavior for multisim devices when there are no calls active; expect both accounts.
     * @throws Exception
     */
    @MediumTest
    @Test
    public void testConstructPossiblePhoneAccountsMultiSimIdle() throws Exception {
        setupMsimAccounts();

        List<PhoneAccountHandle> phoneAccountHandles = mCallsManager.constructPossiblePhoneAccounts(
                TEST_ADDRESS, null, false);
        assertEquals(2, phoneAccountHandles.size());
    }

    /**
     * Tests finding the outgoing call phone account where the call is being placed on a
     * self-managed ConnectionService.
     * @throws Exception
     */
    @MediumTest
    @Test
    public void testFindOutgoingCallPhoneAccountSelfManaged() throws Exception {
        List<PhoneAccountHandle> accounts = mCallsManager.findOutgoingCallPhoneAccount(
                SELF_MANAGED_HANDLE, TEST_ADDRESS, false /* isVideo */, null /* userHandle */);
        assertEquals(1, accounts.size());
        assertEquals(SELF_MANAGED_HANDLE, accounts.get(0));
    }

    /**
     * Tests finding the outgoing calling account where the call has no associated phone account,
     * but there is a user specified default which can be used.
     * @throws Exception
     */
    @MediumTest
    @Test
    public void testFindOutgoingCallAccountDefault() throws Exception {
        when(mPhoneAccountRegistrar.getOutgoingPhoneAccountForScheme(any(), any())).thenReturn(
                SIM_1_HANDLE);
        when(mPhoneAccountRegistrar.getCallCapablePhoneAccounts(any(), anyBoolean(),
                any(), anyInt())).thenReturn(
                new ArrayList<>(Arrays.asList(SIM_1_HANDLE, SIM_2_HANDLE)));

        List<PhoneAccountHandle> accounts = mCallsManager.findOutgoingCallPhoneAccount(
                null /* phoneAcct */, TEST_ADDRESS, false /* isVideo */, null /* userHandle */);

        // Should have found just the default.
        assertEquals(1, accounts.size());
        assertEquals(SIM_1_HANDLE, accounts.get(0));
    }

    /**
     * Tests finding the outgoing calling account where the call has no associated phone account,
     * but there is no user specified default which can be used.
     * @throws Exception
     */
    @MediumTest
    @Test
    public void testFindOutgoingCallAccountNoDefault() throws Exception {
        when(mPhoneAccountRegistrar.getOutgoingPhoneAccountForScheme(any(), any())).thenReturn(
                null);
        when(mPhoneAccountRegistrar.getCallCapablePhoneAccounts(any(), anyBoolean(),
                any(), anyInt())).thenReturn(
                new ArrayList<>(Arrays.asList(SIM_1_HANDLE, SIM_2_HANDLE)));

        List<PhoneAccountHandle> accounts = mCallsManager.findOutgoingCallPhoneAccount(
                null /* phoneAcct */, TEST_ADDRESS, false /* isVideo */, null /* userHandle */);

        assertEquals(2, accounts.size());
        assertTrue(accounts.contains(SIM_1_HANDLE));
        assertTrue(accounts.contains(SIM_2_HANDLE));
    }

    /**
     * Tests that we will default to a video capable phone account if one is available for a video
     * call.
     * @throws Exception
     */
    @MediumTest
    @Test
    public void testFindOutgoingCallAccountVideo() throws Exception {
        when(mPhoneAccountRegistrar.getOutgoingPhoneAccountForScheme(any(), any())).thenReturn(
                null);
        when(mPhoneAccountRegistrar.getCallCapablePhoneAccounts(any(), anyBoolean(),
                any(), eq(PhoneAccount.CAPABILITY_VIDEO_CALLING))).thenReturn(
                new ArrayList<>(Arrays.asList(SIM_2_HANDLE)));

        List<PhoneAccountHandle> accounts = mCallsManager.findOutgoingCallPhoneAccount(
                null /* phoneAcct */, TEST_ADDRESS, true /* isVideo */, null /* userHandle */);

        assertEquals(1, accounts.size());
        assertTrue(accounts.contains(SIM_2_HANDLE));
    }

    /**
     * Tests that we will default to a non-video capable phone account for a video call if no video
     * capable phone accounts are available.
     * @throws Exception
     */
    @MediumTest
    @Test
    public void testFindOutgoingCallAccountVideoNotAvailable() throws Exception {
        when(mPhoneAccountRegistrar.getOutgoingPhoneAccountForScheme(any(), any())).thenReturn(
                null);
        // When querying for video capable accounts, return nothing.
        when(mPhoneAccountRegistrar.getCallCapablePhoneAccounts(any(), anyBoolean(),
                any(), eq(PhoneAccount.CAPABILITY_VIDEO_CALLING))).thenReturn(
                Collections.emptyList());
        // When querying for non-video capable accounts, return one.
        when(mPhoneAccountRegistrar.getCallCapablePhoneAccounts(any(), anyBoolean(),
                any(), eq(0 /* none specified */))).thenReturn(
                new ArrayList<>(Arrays.asList(SIM_1_HANDLE)));
        List<PhoneAccountHandle> accounts = mCallsManager.findOutgoingCallPhoneAccount(
                null /* phoneAcct */, TEST_ADDRESS, true /* isVideo */, null /* userHandle */);

        // Should have found one.
        assertEquals(1, accounts.size());
        assertTrue(accounts.contains(SIM_1_HANDLE));
    }

    /**
     * Tests that we will use the provided target phone account if it exists.
     * @throws Exception
     */
    @MediumTest
    @Test
    public void testUseSpecifiedAccount() throws Exception {
        when(mPhoneAccountRegistrar.getOutgoingPhoneAccountForScheme(any(), any())).thenReturn(
                null);
        when(mPhoneAccountRegistrar.getCallCapablePhoneAccounts(any(), anyBoolean(),
                any(), anyInt())).thenReturn(
                new ArrayList<>(Arrays.asList(SIM_1_HANDLE, SIM_2_HANDLE)));

        List<PhoneAccountHandle> accounts = mCallsManager.findOutgoingCallPhoneAccount(
                SIM_2_HANDLE, TEST_ADDRESS, false /* isVideo */, null /* userHandle */);

        assertEquals(1, accounts.size());
        assertTrue(accounts.contains(SIM_2_HANDLE));
    }

    /**
     * Verifies that an active call will result in playing a DTMF tone when requested.
     * @throws Exception
     */
    @MediumTest
    @Test
    public void testPlayDtmfWhenActive() throws Exception {
        Call callSpy = addSpyCall();
        mCallsManager.playDtmfTone(callSpy, '1');
        verify(callSpy).playDtmfTone(anyChar());
    }

    /**
     * Verifies that DTMF requests are suppressed when a call is held.
     * @throws Exception
     */
    @MediumTest
    @Test
    public void testSuppessDtmfWhenHeld() throws Exception {
        Call callSpy = addSpyCall();
        callSpy.setState(CallState.ON_HOLD, "test");

        mCallsManager.playDtmfTone(callSpy, '1');
        verify(callSpy, never()).playDtmfTone(anyChar());
    }

    /**
     * Verifies that DTMF requests are suppressed when a call is held.
     * @throws Exception
     */
    @MediumTest
    @Test
    public void testCancelDtmfWhenHeld() throws Exception {
        Call callSpy = addSpyCall();
        mCallsManager.playDtmfTone(callSpy, '1');
        mCallsManager.markCallAsOnHold(callSpy);
        verify(callSpy).stopDtmfTone();
    }

    @SmallTest
    @Test
    public void testUnholdCallWhenOngoingCallCanBeHeld() {
        // GIVEN a CallsManager with ongoing call, and this call can be held
        Call ongoingCall = addSpyCall();
        doReturn(true).when(ongoingCall).can(Connection.CAPABILITY_HOLD);
        doReturn(true).when(ongoingCall).can(Connection.CAPABILITY_SUPPORT_HOLD);
        when(mConnectionSvrFocusMgr.getCurrentFocusCall()).thenReturn(ongoingCall);

        // and a held call
        Call heldCall = addSpyCall();

        // WHEN unhold the held call
        mCallsManager.unholdCall(heldCall);

        // THEN the ongoing call is held, and the focus request for incoming call is sent
        verify(ongoingCall).hold(any());
        verifyFocusRequestAndExecuteCallback(heldCall);

        // and held call is unhold now
        verify(heldCall).unhold(any());
    }

    @SmallTest
    @Test
    public void testUnholdCallWhenOngoingCallCanNotBeHeldAndFromDifferentConnectionService() {
        ConnectionServiceWrapper connSvr1 = Mockito.mock(ConnectionServiceWrapper.class);
        ConnectionServiceWrapper connSvr2 = Mockito.mock(ConnectionServiceWrapper.class);

        // GIVEN a CallsManager with ongoing call, and this call can not be held
        Call ongoingCall = addSpyCallWithConnectionService(connSvr1);
        doReturn(false).when(ongoingCall).can(Connection.CAPABILITY_HOLD);
        doReturn(false).when(ongoingCall).can(Connection.CAPABILITY_SUPPORT_HOLD);
        when(mConnectionSvrFocusMgr.getCurrentFocusCall()).thenReturn(ongoingCall);

        // and a held call which has different ConnectionService
        Call heldCall = addSpyCallWithConnectionService(connSvr2);

        // WHEN unhold the held call
        mCallsManager.unholdCall(heldCall);

        // THEN the ongoing call is disconnected, and the focus request for incoming call is sent
        verify(ongoingCall).disconnect(any());
        verifyFocusRequestAndExecuteCallback(heldCall);

        // and held call is unhold now
        verify(heldCall).unhold(any());
    }

    @SmallTest
    @Test
    public void testUnholdCallWhenOngoingCallCanNotBeHeldAndHasSameConnectionService() {
        ConnectionServiceWrapper connSvr = Mockito.mock(ConnectionServiceWrapper.class);

        // GIVEN a CallsManager with ongoing call, and this call can not be held
        Call ongoingCall = addSpyCallWithConnectionService(connSvr);
        doReturn(false).when(ongoingCall).can(Connection.CAPABILITY_HOLD);
        doReturn(true).when(ongoingCall).can(Connection.CAPABILITY_SUPPORT_HOLD);
        when(mConnectionSvrFocusMgr.getCurrentFocusCall()).thenReturn(ongoingCall);

        // and a held call which has different ConnectionService
        Call heldCall = addSpyCallWithConnectionService(connSvr);

        // WHEN unhold the held call
        mCallsManager.unholdCall(heldCall);

        // THEN the ongoing call is held
        verify(ongoingCall).hold(any());
        verifyFocusRequestAndExecuteCallback(heldCall);

        // and held call is unhold now
        verify(heldCall).unhold(any());
    }

    @SmallTest
    @Test
    public void testAnswerCallWhenOngoingCallCanBeHeld() {
        // GIVEN a CallsManager with ongoing call, and this call can be held
        Call ongoingCall = addSpyCall();
        doReturn(true).when(ongoingCall).can(Connection.CAPABILITY_HOLD);
        doReturn(true).when(ongoingCall).can(Connection.CAPABILITY_SUPPORT_HOLD);
        when(mConnectionSvrFocusMgr.getCurrentFocusCall()).thenReturn(ongoingCall);

        // WHEN answer an incoming call
        Call incomingCall = addSpyCall();
        mCallsManager.answerCall(incomingCall, VideoProfile.STATE_AUDIO_ONLY);

        // THEN the ongoing call is held and the focus request for incoming call is sent
        verify(ongoingCall).hold();
        verifyFocusRequestAndExecuteCallback(incomingCall);

        // and the incoming call is answered.
        verify(incomingCall).answer(VideoProfile.STATE_AUDIO_ONLY);
    }

    @SmallTest
    @Test
    public void testAnswerCallWhenOngoingHasSameConnectionService() {
        ConnectionServiceWrapper connSvr = Mockito.mock(ConnectionServiceWrapper.class);

        // GIVEN a CallsManager with ongoing call, and this call can not be held
        Call ongoingCall = addSpyCallWithConnectionService(connSvr);
        doReturn(false).when(ongoingCall).can(Connection.CAPABILITY_HOLD);
        when(mConnectionSvrFocusMgr.getCurrentFocusCall()).thenReturn(ongoingCall);

        // WHEN answer an incoming call
        Call incomingCall = addSpyCallWithConnectionService(connSvr);
        mCallsManager.answerCall(incomingCall, VideoProfile.STATE_AUDIO_ONLY);

        // THEN nothing happened on the ongoing call and the focus request for incoming call is sent
        verifyFocusRequestAndExecuteCallback(incomingCall);

        // and the incoming call is answered.
        verify(incomingCall).answer(VideoProfile.STATE_AUDIO_ONLY);
    }

    @SmallTest
    @Test
    public void testAnswerCallWhenOngoingHasDifferentConnectionService() {
        ConnectionServiceWrapper connSvr1 = Mockito.mock(ConnectionServiceWrapper.class);
        ConnectionServiceWrapper connSvr2 = Mockito.mock(ConnectionServiceWrapper.class);

        // GIVEN a CallsManager with ongoing call, and this call can not be held
        Call ongoingCall = addSpyCallWithConnectionService(connSvr1);
        doReturn(false).when(ongoingCall).can(Connection.CAPABILITY_HOLD);
        doReturn(true).when(ongoingCall).can(Connection.CAPABILITY_SUPPORT_HOLD);
        when(mConnectionSvrFocusMgr.getCurrentFocusCall()).thenReturn(ongoingCall);

        // WHEN answer an incoming call
        Call incomingCall = addSpyCallWithConnectionService(connSvr2);
        mCallsManager.answerCall(incomingCall, VideoProfile.STATE_AUDIO_ONLY);

        // THEN the ongoing call is disconnected and the focus request for incoming call is sent
        verify(ongoingCall).disconnect();
        verifyFocusRequestAndExecuteCallback(incomingCall);

        // and the incoming call is answered.
        verify(incomingCall).answer(VideoProfile.STATE_AUDIO_ONLY);
    }

    @SmallTest
    @Test
    public void testAnswerCallWhenMultipleHeldCallsExisted() {
        ConnectionServiceWrapper connSvr1 = Mockito.mock(ConnectionServiceWrapper.class);
        ConnectionServiceWrapper connSvr2 = Mockito.mock(ConnectionServiceWrapper.class);

        // Given an ongoing call and held call with the ConnectionService connSvr1. The
        // ConnectionService connSvr1 can handle one held call
        Call ongoingCall = addSpyCallWithConnectionService(connSvr1);
        doReturn(false).when(ongoingCall).can(Connection.CAPABILITY_HOLD);
        doReturn(true).when(ongoingCall).can(Connection.CAPABILITY_SUPPORT_HOLD);
        Call heldCall = addSpyCallWithConnectionService(connSvr1);
        doReturn(CallState.ON_HOLD).when(heldCall).getState();

        // and other held call has difference ConnectionService
        Call heldCall2 = addSpyCallWithConnectionService(connSvr2);
        doReturn(CallState.ON_HOLD).when(heldCall2).getState();

        // WHEN answer an incoming call which ConnectionService is connSvr1
        Call incomingCall = addSpyCallWithConnectionService(connSvr1);
        mCallsManager.answerCall(incomingCall, VideoProfile.STATE_AUDIO_ONLY);

        // THEN the previous held call is disconnected
        verify(heldCall).disconnect();

        // and the ongoing call is held
        verify(ongoingCall).hold();

        // and the heldCall2 is not disconnected
        verify(heldCall2, never()).disconnect();

        // and the focus request is sent
        verifyFocusRequestAndExecuteCallback(incomingCall);

        // and the incoming call is answered
        verify(incomingCall).answer(VideoProfile.STATE_AUDIO_ONLY);
    }

    @SmallTest
    @Test
    public void testAnswerCallWhenNoOngoingCallExisted() {
        // GIVEN a CallsManager with no ongoing call.

        // WHEN answer an incoming call
        Call incomingCall = addSpyCall();
        mCallsManager.answerCall(incomingCall, VideoProfile.STATE_AUDIO_ONLY);

        // THEN the focus request for incoming call is sent
        verifyFocusRequestAndExecuteCallback(incomingCall);

        // and the incoming call is answered.
        verify(incomingCall).answer(VideoProfile.STATE_AUDIO_ONLY);
    }

    @SmallTest
    @Test
    public void testSetActiveCallWhenOngoingCallCanNotBeHeldAndFromDifferentConnectionService() {
        ConnectionServiceWrapper connSvr1 = Mockito.mock(ConnectionServiceWrapper.class);
        ConnectionServiceWrapper connSvr2 = Mockito.mock(ConnectionServiceWrapper.class);

        // GIVEN a CallsManager with ongoing call, and this call can not be held
        Call ongoingCall = addSpyCallWithConnectionService(connSvr1);
        doReturn(false).when(ongoingCall).can(Connection.CAPABILITY_HOLD);
        doReturn(false).when(ongoingCall).can(Connection.CAPABILITY_SUPPORT_HOLD);
        doReturn(ongoingCall).when(mConnectionSvrFocusMgr).getCurrentFocusCall();

        // and a new self-managed call which has different ConnectionService
        Call newCall = addSpyCallWithConnectionService(connSvr2);
        doReturn(true).when(newCall).isSelfManaged();

        // WHEN active the new call
        mCallsManager.markCallAsActive(newCall);

        // THEN the ongoing call is disconnected, and the focus request for the new call is sent
        verify(ongoingCall).disconnect();
        verifyFocusRequestAndExecuteCallback(newCall);

        // and the new call is active
        assertEquals(CallState.ACTIVE, newCall.getState());
    }

    @SmallTest
    @Test
    public void testSetActiveCallWhenOngoingCallCanNotBeHeldAndHasSameConnectionService() {
        ConnectionServiceWrapper connSvr = Mockito.mock(ConnectionServiceWrapper.class);

        // GIVEN a CallsManager with ongoing call, and this call can not be held
        Call ongoingCall = addSpyCallWithConnectionService(connSvr);
        doReturn(false).when(ongoingCall).can(Connection.CAPABILITY_HOLD);
        doReturn(false).when(ongoingCall).can(Connection.CAPABILITY_SUPPORT_HOLD);
        when(mConnectionSvrFocusMgr.getCurrentFocusCall()).thenReturn(ongoingCall);

        // and a new self-managed call which has the same ConnectionService
        Call newCall = addSpyCallWithConnectionService(connSvr);
        doReturn(true).when(newCall).isSelfManaged();

        // WHEN active the new call
        mCallsManager.markCallAsActive(newCall);

        // THEN the ongoing call isn't disconnected
        verify(ongoingCall, never()).disconnect();
        verifyFocusRequestAndExecuteCallback(newCall);

        // and the new call is active
        assertEquals(CallState.ACTIVE, newCall.getState());
    }

    @SmallTest
    @Test
    public void testSetActiveCallWhenOngoingCallCanBeHeld() {
        // GIVEN a CallsManager with ongoing call, and this call can be held
        Call ongoingCall = addSpyCall();
        doReturn(true).when(ongoingCall).can(Connection.CAPABILITY_HOLD);
        doReturn(true).when(ongoingCall).can(Connection.CAPABILITY_SUPPORT_HOLD);
        doReturn(ongoingCall).when(mConnectionSvrFocusMgr).getCurrentFocusCall();

        // and a new self-managed call
        Call newCall = addSpyCall();
        doReturn(true).when(newCall).isSelfManaged();

        // WHEN active the new call
        mCallsManager.markCallAsActive(newCall);

        // THEN the ongoing call is held
        verify(ongoingCall).hold();
        verifyFocusRequestAndExecuteCallback(newCall);

        // and the new call is active
        assertEquals(CallState.ACTIVE, newCall.getState());
    }

    private Call addSpyCallWithConnectionService(ConnectionServiceWrapper connSvr) {
        Call call = addSpyCall();
        doReturn(connSvr).when(call).getConnectionService();
        return call;
    }

    private Call addSpyCall() {
        Call ongoingCall = new Call("1", /* callId */
                mComponentContextFixture.getTestDouble(),
                mCallsManager,
                mLock, /* ConnectionServiceRepository */
                null,
                mContactsAsyncHelper,
                mCallerInfoAsyncQueryFactory,
                mPhoneNumberUtilsAdapter,
                TEST_ADDRESS,
                null /* GatewayInfo */,
                null /* connectionManagerPhoneAccountHandle */,
                SIM_2_HANDLE,
                Call.CALL_DIRECTION_INCOMING,
                false /* shouldAttachToExistingConnection*/,
                false /* isConference */,
                mClockProxy);
        ongoingCall.setState(CallState.ACTIVE, "just cuz");
        Call callSpy = Mockito.spy(ongoingCall);

        // Mocks some methods to not call the real method.
        doNothing().when(callSpy).unhold();
        doNothing().when(callSpy).hold();
        doNothing().when(callSpy).disconnect();
        doNothing().when(callSpy).answer(Matchers.anyInt());
        doNothing().when(callSpy).setStartWithSpeakerphoneOn(Matchers.anyBoolean());

        mCallsManager.addCall(callSpy);
        return callSpy;
    }

    private void verifyFocusRequestAndExecuteCallback(Call call) {
        ArgumentCaptor<CallsManager.RequestCallback> captor =
                ArgumentCaptor.forClass(CallsManager.RequestCallback.class);
        verify(mConnectionSvrFocusMgr).requestFocus(eq(call), captor.capture());
        CallsManager.RequestCallback callback = captor.getValue();
        callback.onRequestFocusDone(call);
    }

    private void setupMsimAccounts() {
        TelephonyManager mockTelephonyManager = mComponentContextFixture.getTelephonyManager();
        when(mockTelephonyManager.getMultiSimConfiguration()).thenReturn(
                TelephonyManager.MultiSimVariants.DSDS);
        when(mPhoneAccountRegistrar.getCallCapablePhoneAccounts(any(), anyBoolean(),
                any(), anyInt())).thenReturn(
                new ArrayList<>(Arrays.asList(SIM_1_HANDLE, SIM_2_HANDLE)));
        when(mPhoneAccountRegistrar.getSimPhoneAccountsOfCurrentUser()).thenReturn(
                new ArrayList<>(Arrays.asList(SIM_1_HANDLE, SIM_2_HANDLE)));
    }
}
