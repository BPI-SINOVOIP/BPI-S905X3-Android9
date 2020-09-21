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
 * limitations under the License.
 */

package android.telecom.cts;

import static android.telecom.cts.TestUtils.*;

import android.content.ComponentName;
import android.content.Context;
import android.media.AudioManager;
import android.net.Uri;
import android.telecom.Call;
import android.telecom.Connection;
import android.telecom.ConnectionService;
import android.telecom.PhoneAccountHandle;

import java.util.Collection;

/**
 * Test some additional {@link ConnectionService} and {@link Connection} APIs not already covered
 * by other tests.
 */
public class ConnectionServiceTest extends BaseTelecomTestWithMockServices {

    private static final Uri SELF_MANAGED_TEST_ADDRESS =
            Uri.fromParts("sip", "call1@test.com", null);

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mContext = getInstrumentation().getContext();
        if (mShouldTestTelecom) {
            setupConnectionService(null, FLAG_REGISTER | FLAG_ENABLE);
            mTelecomManager.registerPhoneAccount(TestUtils.TEST_SELF_MANAGED_PHONE_ACCOUNT_1);
        }
    }

    public void testAddExistingConnection() {
        if (!mShouldTestTelecom) {
            return;
        }

        placeAndVerifyCall();
        verifyConnectionForOutgoingCall();

        // Add second connection (add existing connection)
        final MockConnection connection = new MockConnection();
        connection.setOnHold();
        CtsConnectionService.addExistingConnectionToTelecom(TEST_PHONE_ACCOUNT_HANDLE, connection);
        assertNumCalls(mInCallCallbacks.getService(), 2);
        mInCallCallbacks.lock.drainPermits();
        final Call call = mInCallCallbacks.getService().getLastCall();
        assertCallState(call, Call.STATE_HOLDING);
    }

    public void testAddExistingConnection_invalidPhoneAccountPackageName() {
        if (!mShouldTestTelecom) {
            return;
        }

        placeAndVerifyCall();
        verifyConnectionForOutgoingCall();

        // Add second connection (add existing connection)
        final MockConnection connection = new MockConnection();
        connection.setOnHold();
        ComponentName invalidName = new ComponentName("com.android.phone",
                "com.android.services.telephony.TelephonyConnectionService");
        // This command will fail and a SecurityException will be thrown by Telecom. The Exception
        // will then be absorbed by the ConnectionServiceAdapter.
        CtsConnectionService.addExistingConnectionToTelecom(new PhoneAccountHandle(invalidName,
                "Test"), connection);
        // Make sure that only the original Call exists.
        assertNumCalls(mInCallCallbacks.getService(), 1);
        mInCallCallbacks.lock.drainPermits();
        final Call call = mInCallCallbacks.getService().getLastCall();
        assertCallState(call, Call.STATE_DIALING);
    }

    public void testAddExistingConnection_invalidPhoneAccountAccountId() {
        if (!mShouldTestTelecom) {
            return;
        }

        placeAndVerifyCall();
        verifyConnectionForOutgoingCall();

        // Add second connection (add existing connection)
        final MockConnection connection = new MockConnection();
        connection.setOnHold();
        ComponentName validName = new ComponentName(PACKAGE, COMPONENT);
        // This command will fail because the PhoneAccount is not registered to Telecom currently.
        CtsConnectionService.addExistingConnectionToTelecom(new PhoneAccountHandle(validName,
                "Invalid Account Id"), connection);
        // Make sure that only the original Call exists.
        assertNumCalls(mInCallCallbacks.getService(), 1);
        mInCallCallbacks.lock.drainPermits();
        final Call call = mInCallCallbacks.getService().getLastCall();
        assertCallState(call, Call.STATE_DIALING);
    }

    public void testVoipAudioModePropagation() throws Exception {
        if (!mShouldTestTelecom) {
            return;
        }

        placeAndVerifyCall();
        MockConnection connection = verifyConnectionForOutgoingCall();
        connection.setAudioModeIsVoip(true);
        waitOnAllHandlers(getInstrumentation());

        AudioManager audioManager = (AudioManager) mContext.getSystemService(Context.AUDIO_SERVICE);
        assertEquals(AudioManager.MODE_IN_COMMUNICATION, audioManager.getMode());
        connection.setAudioModeIsVoip(false);
        waitOnAllHandlers(getInstrumentation());
        assertEquals(AudioManager.MODE_IN_CALL, audioManager.getMode());
    }

    public void testConnectionServiceFocusGainedWithNoConnectionService() {
        if (!mShouldTestTelecom) {
            return;
        }

        // WHEN place a managed call
        placeAndVerifyCall();

        // THEN managed connection service has gained the focus
        assertTrue(connectionService.waitForEvent(
                MockConnectionService.EVENT_CONNECTION_SERVICE_FOCUS_GAINED));
    }

    public void testConnectionServiceFocusGainedWithSameConnectionService() {
        if (!mShouldTestTelecom) {
            return;
        }

        // GIVEN a managed call
        placeAndVerifyCall();
        verifyConnectionForOutgoingCall().setActive();
        assertTrue(connectionService.waitForEvent(
                MockConnectionService.EVENT_CONNECTION_SERVICE_FOCUS_GAINED));

        // WHEN place another call has the same ConnectionService as the existing call
        placeAndVerifyCall();
        verifyConnectionForOutgoingCall();

        // THEN the ConnectionService has not gained the focus again
        assertFalse(connectionService.waitForEvent(
                MockConnectionService.EVENT_CONNECTION_SERVICE_FOCUS_GAINED));
        // and the ConnectionService didn't lose the focus
        assertFalse(connectionService.waitForEvent(
                MockConnectionService.EVENT_CONNECTION_SERVICE_FOCUS_LOST));
    }

    public void testConnectionServiceFocusGainedWithDifferentConnectionService() {
        if (!mShouldTestTelecom) {
            return;
        }

        // GIVEN an existing managed call
        placeAndVerifyCall();
        verifyConnectionForOutgoingCall().setActive();
        assertTrue(connectionService.waitForEvent(
                MockConnectionService.EVENT_CONNECTION_SERVICE_FOCUS_GAINED));

        // WHEN a self-managed call is coming
        SelfManagedConnection selfManagedConnection =
                addIncomingSelfManagedCall(TEST_SELF_MANAGED_HANDLE_1, SELF_MANAGED_TEST_ADDRESS);

        // THEN the managed ConnectionService has lost the focus
        assertTrue(connectionService.waitForEvent(
                MockConnectionService.EVENT_CONNECTION_SERVICE_FOCUS_LOST));
        // and the self-managed ConnectionService has gained the focus
        assertTrue(CtsSelfManagedConnectionService.getConnectionService().waitForUpdate(
                        CtsSelfManagedConnectionService.FOCUS_GAINED_LOCK));

        // Disconnected the self-managed call
        selfManagedConnection.disconnectAndDestroy();
    }

    private SelfManagedConnection addIncomingSelfManagedCall(
            PhoneAccountHandle pah, Uri address) {

        TestUtils.addIncomingCall(getInstrumentation(), mTelecomManager, pah, address);

        // Ensure Telecom bound to the self managed CS
        if (!CtsSelfManagedConnectionService.waitForBinding()) {
            fail("Could not bind to Self-Managed ConnectionService");
        }

        SelfManagedConnection connection = TestUtils.waitForAndGetConnection(address);

        // Active the call
        connection.setActive();

        return connection;
    }

    public void testGetAllConnections() {
        if (!mShouldTestTelecom) {
            return;
        }

        // Add first connection (outgoing call)
        placeAndVerifyCall();
        final Connection connection1 = verifyConnectionForOutgoingCall();

        Collection<Connection> connections = CtsConnectionService.getAllConnectionsFromTelecom();
        assertEquals(1, connections.size());
        assertTrue(connections.contains(connection1));
        // Need to move this to active since we reject the 3rd incoming call below if this is in
        // dialing state (b/23428950).
        connection1.setActive();
        assertCallState(mInCallCallbacks.getService().getLastCall(), Call.STATE_ACTIVE);

        // Add second connection (add existing connection)
        final Connection connection2 = new MockConnection();
        connection2.setActive();
        CtsConnectionService.addExistingConnectionToTelecom(TEST_PHONE_ACCOUNT_HANDLE, connection2);
        assertNumCalls(mInCallCallbacks.getService(), 2);
        mInCallCallbacks.lock.drainPermits();
        connections = CtsConnectionService.getAllConnectionsFromTelecom();
        assertEquals(2, connections.size());
        assertTrue(connections.contains(connection2));

        // Add third connection (incoming call)
        addAndVerifyNewIncomingCall(createTestNumber(), null);
        final Connection connection3 = verifyConnectionForIncomingCall();
        connections = CtsConnectionService.getAllConnectionsFromTelecom();
        assertEquals(3, connections.size());
        assertTrue(connections.contains(connection3));
    }
}
