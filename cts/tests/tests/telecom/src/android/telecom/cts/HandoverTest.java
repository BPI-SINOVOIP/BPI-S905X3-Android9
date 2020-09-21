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
 * limitations under the License
 */

package android.telecom.cts;

import static android.telecom.cts.TestUtils.TEST_HANDOVER_DEST_PHONE_ACCOUNT_HANDLE;
import static android.telecom.cts.TestUtils.TEST_HANDOVER_SRC_PHONE_ACCOUNT_HANDLE;
import static android.telecom.cts.TestUtils.WAIT_FOR_STATE_CHANGE_TIMEOUT_MS;

import android.net.Uri;
import android.os.Bundle;
import android.telecom.Call;
import android.telecom.ConnectionRequest;
import android.telecom.PhoneAccountHandle;
import android.telecom.TelecomManager;
import android.telecom.VideoProfile;

/**
 * Tests the Telecom handover APIs.
 */
public class HandoverTest extends BaseTelecomTestWithMockServices {
    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mContext = getInstrumentation().getContext();
        if (mShouldTestTelecom) {
            setupConnectionService(null, FLAG_REGISTER | FLAG_ENABLE);

            // Test handover source is a managed ConnectionService
            mTelecomManager.registerPhoneAccount(TestUtils.TEST_PHONE_ACCOUNT_HANDOVER_SRC);
            TestUtils.enablePhoneAccount(getInstrumentation(),
                    TestUtils.TEST_HANDOVER_SRC_PHONE_ACCOUNT_HANDLE);
            assertPhoneAccountEnabled(TestUtils.TEST_HANDOVER_SRC_PHONE_ACCOUNT_HANDLE);

            // Test handover destination is a self-managed ConnectionService.
            mTelecomManager.registerPhoneAccount(TestUtils.TEST_PHONE_ACCOUNT_HANDOVER_DEST);
        }

    }

    @Override
    protected void tearDown() throws Exception {
        CtsSelfManagedConnectionService connectionService =
                CtsSelfManagedConnectionService.getConnectionService();
        if (connectionService != null) {
            connectionService.tearDown();
            mTelecomManager.unregisterPhoneAccount(
                    TestUtils.TEST_HANDOVER_SRC_PHONE_ACCOUNT_HANDLE);
            mTelecomManager.unregisterPhoneAccount(
                    TestUtils.TEST_HANDOVER_DEST_PHONE_ACCOUNT_HANDLE);
        }

        super.tearDown();
    }

    /**
     * Ensures a call handover cannot be initiated for a {@link android.telecom.PhoneAccount} which
     * does not declare {@link android.telecom.PhoneAccount#EXTRA_SUPPORTS_HANDOVER_FROM}.
     */
    public void testHandoverSourceFailed() {
        if (!mShouldTestTelecom) {
            return;
        }

        placeAndVerifyCall();
        Call call = mInCallCallbacks.getService().getLastCall();

        call.handoverTo(TestUtils.TEST_SELF_MANAGED_HANDLE_1, VideoProfile.STATE_BIDIRECTIONAL,
                null);

        // Expect the handover failed callback to be called.
        mOnHandoverFailedCounter.waitForCount(WAIT_FOR_STATE_CHANGE_TIMEOUT_MS);
        Call callbackCall = (Call) mOnHandoverFailedCounter.getArgs(0)[0];
        int failureReason = (int) mOnHandoverFailedCounter.getArgs(0)[1];
        assertEquals(call, callbackCall);
        assertEquals(Call.Callback.HANDOVER_FAILURE_NOT_SUPPORTED, failureReason);

        call.disconnect();
    }

    /**
     * Ensures a call handover cannot be initiated to a {@link android.telecom.PhoneAccount} which
     * does not declare {@link android.telecom.PhoneAccount#EXTRA_SUPPORTS_HANDOVER_TO}.
     */
    public void testHandoverDestinationFailed() {
        if (!mShouldTestTelecom) {
            return;
        }
        startSourceCall();
        Call call = mInCallCallbacks.getService().getLastCall();

        // Now try to handover to an account which does not support handover.
        call.handoverTo(TestUtils.TEST_SELF_MANAGED_HANDLE_1, VideoProfile.STATE_BIDIRECTIONAL,
                null);

        // Expect the handover failed callback to be called.
        mOnHandoverFailedCounter.waitForCount(WAIT_FOR_STATE_CHANGE_TIMEOUT_MS);
        Call callbackCall = (Call) mOnHandoverFailedCounter.getArgs(0)[0];
        int failureReason = (int) mOnHandoverFailedCounter.getArgs(0)[1];
        assertEquals(call, callbackCall);
        assertEquals(Call.Callback.HANDOVER_FAILURE_NOT_SUPPORTED, failureReason);

        call.disconnect();
    }

    /**
     * Ensures that when the source and destination both support handover that an outgoing handover
     * request will be successfully relayed.
     */
    public void testOutgoingHandoverRequestValid() {
        if (!mShouldTestTelecom) {
            return;
        }

        // Begin our source call on the CS which supports handover from it.
        startSourceCall();
        final Call call = mInCallCallbacks.getService().getLastCall();

        // Now try to handover to an account which does support handover to it.
        call.handoverTo(TestUtils.TEST_HANDOVER_DEST_PHONE_ACCOUNT_HANDLE,
                VideoProfile.STATE_BIDIRECTIONAL, null);

        // Ensure Telecom bound to the self managed CS
        if (!CtsSelfManagedConnectionService.waitForBinding()) {
            fail("Could not bind to Self-Managed ConnectionService");
        }

        // Wait for binding to self managed CS and invocation of outgoing handover method.
        TestUtils.InvokeCounter counter =
                CtsSelfManagedConnectionService.getConnectionService()
                        .getOnCreateOutgoingHandoverConnectionCounter();
        counter.waitForCount(WAIT_FOR_STATE_CHANGE_TIMEOUT_MS);

        SelfManagedConnection connection = TestUtils.waitForAndGetConnection(
                call.getDetails().getHandle());

        // Verify the from handle is as expected.
        PhoneAccountHandle fromHandle = (PhoneAccountHandle) counter.getArgs(0)[0];
        assertEquals(TEST_HANDOVER_SRC_PHONE_ACCOUNT_HANDLE, fromHandle);
        // Verify the to handle is as expected.
        ConnectionRequest request = (ConnectionRequest) counter.getArgs(0)[1];
        assertEquals(TEST_HANDOVER_DEST_PHONE_ACCOUNT_HANDLE, request.getAccountHandle());

        completeHandoverAndVerify(call, connection);
    }

    /**
     * Tests use of the
     * {@link android.telecom.TelecomManager#acceptHandover(Uri, int, PhoneAccountHandle)} API on
     * the receiving side of the handover.
     */
    public void testIncomingHandoverRequestValid() {
        if (!mShouldTestTelecom) {
            return;
        }

        // Begin our source call on the CS which supports handover from it.
        startSourceCall();
        final Call call = mInCallCallbacks.getService().getLastCall();

        // Request to accept handover of that call to another app.
        mTelecomManager.acceptHandover(call.getDetails().getHandle(),
                VideoProfile.STATE_BIDIRECTIONAL, TEST_HANDOVER_DEST_PHONE_ACCOUNT_HANDLE);

        // Ensure Telecom bound to the self managed CS
        if (!CtsSelfManagedConnectionService.waitForBinding()) {
            fail("Could not bind to Self-Managed ConnectionService");
        }

        // Wait for binding to self managed CS and invocation of incoming handover method.
        TestUtils.InvokeCounter counter =
                CtsSelfManagedConnectionService.getConnectionService()
                        .getOnCreateIncomingHandoverConnectionCounter();
        counter.waitForCount(WAIT_FOR_STATE_CHANGE_TIMEOUT_MS);

        SelfManagedConnection connection = TestUtils.waitForAndGetConnection(
                call.getDetails().getHandle());

        // Verify the from handle is as expected.
        PhoneAccountHandle fromHandle = (PhoneAccountHandle) counter.getArgs(0)[0];
        assertEquals(TEST_HANDOVER_SRC_PHONE_ACCOUNT_HANDLE, fromHandle);
        // Verify the to handle is as expected.
        ConnectionRequest request = (ConnectionRequest) counter.getArgs(0)[1];
        assertEquals(TEST_HANDOVER_DEST_PHONE_ACCOUNT_HANDLE, request.getAccountHandle());
        // The original call's address should match the address of the handover request.
        assertEquals(call.getDetails().getHandle(), request.getAddress());

        completeHandoverAndVerify(call, connection);
    }

    /**
     * Begins a call which will be the source of a handover.
     */
    private void startSourceCall() {
        // Ensure the ongoing account is on a PhoneAccount which supports handover from.
        Bundle extras = new Bundle();
        extras.putParcelable(TelecomManager.EXTRA_PHONE_ACCOUNT_HANDLE,
                TEST_HANDOVER_SRC_PHONE_ACCOUNT_HANDLE);
        placeAndVerifyCall(extras);
    }

    /**
     * Complete the a call handover and verify that it was successfully reported.
     * @param call
     * @param connection
     */
    private void completeHandoverAndVerify(final Call call, SelfManagedConnection connection) {
        // Make the connection active, indicating that the user has accepted the handover.
        connection.setActive();

        // Expect the original call to have been informed of handover completion.
        mOnHandoverCompleteCounter.waitForCount(1, WAIT_FOR_STATE_CHANGE_TIMEOUT_MS);
        assertEquals(1, mOnHandoverCompleteCounter.getInvokeCount());

        // Also expect the connection to be informed of handover completion.
        connection.getHandoverCompleteCounter().waitForCount(1, WAIT_FOR_STATE_CHANGE_TIMEOUT_MS);
        assertEquals(1, connection.getHandoverCompleteCounter().getInvokeCount());

        // Now, we expect that the original connection will get disconnected.
        waitUntilConditionIsTrueOrTimeout(new Condition() {
                                              @Override
                                              public Object expected() {
                                                  return Call.STATE_DISCONNECTED;
                                              }
                                              @Override
                                              public Object actual() {
                                                  return call.getState();
                                              }
                                          },
                WAIT_FOR_STATE_CHANGE_TIMEOUT_MS,
                "Expected original call to be disconnected."
        );
    }
}
