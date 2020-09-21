/*
 * Copyright 2017 The Android Open Source Project
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

import android.os.Looper;
import android.test.suitebuilder.annotation.SmallTest;
import com.android.server.telecom.Call;
import com.android.server.telecom.CallState;
import com.android.server.telecom.CallsManager;
import com.android.server.telecom.ConnectionServiceFocusManager;
import com.android.server.telecom.ConnectionServiceFocusManager.*;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.Mockito;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.Matchers.any;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

@RunWith(JUnit4.class)
public class ConnectionServiceFocusManagerTest extends TelecomTestCase {

    @Mock CallsManagerRequester mockCallsManagerRequester;
    @Mock RequestFocusCallback mockRequestFocusCallback;

    @Mock ConnectionServiceFocus mNewConnectionService;
    @Mock ConnectionServiceFocus mActiveConnectionService;

    private static final int CHECK_HANDLER_INTERVAL_MS = 10;

    private ConnectionServiceFocusManager mFocusManagerUT;
    private CallFocus mNewCall;
    private CallFocus mActiveCall;
    private CallsManager.CallsManagerListener mCallsManagerListener;

    @Override
    @Before
    public void setUp() throws Exception {
        super.setUp();
        mFocusManagerUT = new ConnectionServiceFocusManager(
                mockCallsManagerRequester, Looper.getMainLooper());
        mNewCall = createFakeCall(mNewConnectionService, CallState.NEW);
        mActiveCall = createFakeCall(mActiveConnectionService, CallState.ACTIVE);
        ArgumentCaptor<CallsManager.CallsManagerListener> captor =
                ArgumentCaptor.forClass(CallsManager.CallsManagerListener.class);
        verify(mockCallsManagerRequester).setCallsManagerListener(captor.capture());
        mCallsManagerListener = captor.getValue();
    }

    @SmallTest
    @Test
    public void testRequestFocusWithoutActiveFocusExisted() {
        // GIVEN the ConnectionServiceFocusManager without focus ConnectionService.

        // WHEN request calling focus for the given call.
        requestFocus(mNewCall, mockRequestFocusCallback);

        // THEN the request is done and the ConnectionService of the given call has gain the focus.
        verifyRequestFocusDone(mFocusManagerUT, mNewCall, mockRequestFocusCallback, true);
    }

    @SmallTest
    @Test
    public void testRequestFocusWithActiveFocusExisted() {
        // GIVEN the ConnectionServiceFocusManager with the focus ConnectionService.
        requestFocus(mActiveCall, null);
        ConnectionServiceFocusListener connSvrFocusListener =
                getConnectionServiceFocusListener(mActiveConnectionService);

        // WHEN request calling focus for the given call.
        requestFocus(mNewCall, mockRequestFocusCallback);

        // THEN the current focus ConnectionService is informed it has lose the focus.
        verify(mActiveConnectionService).connectionServiceFocusLost();
        // and the focus request is not done.
        verify(mockRequestFocusCallback, never())
                .onRequestFocusDone(any(CallFocus.class));

        // WHEN the current focus released the call resource.
        connSvrFocusListener.onConnectionServiceReleased(mActiveConnectionService);
        waitForHandlerAction(mFocusManagerUT.getHandler(), CHECK_HANDLER_INTERVAL_MS);

        // THEN the request is done and the ConnectionService of the given call has gain the focus.
        verifyRequestFocusDone(mFocusManagerUT, mNewCall, mockRequestFocusCallback, true);

        // and the timeout event of the focus released is canceled.
        waitForHandlerActionDelayed(
                mFocusManagerUT.getHandler(),
                mFocusManagerUT.RELEASE_FOCUS_TIMEOUT_MS,
                CHECK_HANDLER_INTERVAL_MS);
        verify(mockCallsManagerRequester, never()).releaseConnectionService(
                any(ConnectionServiceFocus.class));
    }

    @SmallTest
    @Test
    public void testRequestConnectionServiceSameAsFocusConnectionService() {
        // GIVEN the ConnectionServiceFocusManager with the focus ConnectionService.
        requestFocus(mActiveCall, null);
        reset(mActiveConnectionService);

        // WHEN request calling focus for the given call that has the same ConnectionService as the
        // active call.
        when(mNewCall.getConnectionServiceWrapper()).thenReturn(mActiveConnectionService);
        requestFocus(mNewCall, mockRequestFocusCallback);

        // THEN the request is done without any change on the focus ConnectionService.
        verify(mNewConnectionService, never()).connectionServiceFocusLost();
        verifyRequestFocusDone(mFocusManagerUT, mNewCall, mockRequestFocusCallback, false);
    }

    @SmallTest
    @Test
    public void testFocusConnectionServiceDoesNotRespondToFocusLost() {
        // GIVEN the ConnectionServiceFocusManager with the focus ConnectionService.
        requestFocus(mActiveCall, null);

        // WHEN request calling focus for the given call.
        requestFocus(mNewCall, mockRequestFocusCallback);

        // THEN the current focus ConnectionService is informed it has lose the focus.
        verify(mActiveConnectionService).connectionServiceFocusLost();
        // and the focus request is not done.
        verify(mockRequestFocusCallback, never())
                .onRequestFocusDone(any(CallFocus.class));

        // but the current focus ConnectionService didn't respond to the focus lost.
        waitForHandlerActionDelayed(
                mFocusManagerUT.getHandler(),
                CHECK_HANDLER_INTERVAL_MS,
                mFocusManagerUT.RELEASE_FOCUS_TIMEOUT_MS + 100);

        // THEN the focusManager sends a request to disconnect the focus ConnectionService
        verify(mockCallsManagerRequester).releaseConnectionService(mActiveConnectionService);
        // THEN the request is done and the ConnectionService of the given call has gain the focus.
        verifyRequestFocusDone(mFocusManagerUT, mNewCall, mockRequestFocusCallback, true);
    }

    @SmallTest
    @Test
    public void testNonFocusConnectionServiceReleased() {
        // GIVEN the ConnectionServiceFocusManager with the focus ConnectionService
        requestFocus(mActiveCall, null);
        ConnectionServiceFocusListener connSvrFocusListener =
                getConnectionServiceFocusListener(mActiveConnectionService);

        // WHEN there is a released request for a non focus ConnectionService.
        connSvrFocusListener.onConnectionServiceReleased(mNewConnectionService);

        // THEN nothing changed.
        assertEquals(mActiveCall, mFocusManagerUT.getCurrentFocusCall());
        assertEquals(mActiveConnectionService, mFocusManagerUT.getCurrentFocusConnectionService());
    }

    @SmallTest
    @Test
    public void testFocusConnectionServiceReleased() {
        // GIVEN the ConnectionServiceFocusManager with the focus ConnectionService
        requestFocus(mActiveCall, null);
        ConnectionServiceFocusListener connSvrFocusListener =
                getConnectionServiceFocusListener(mActiveConnectionService);

        // WHEN the focus ConnectionService request to release.
        connSvrFocusListener.onConnectionServiceReleased(mActiveConnectionService);
        waitForHandlerAction(mFocusManagerUT.getHandler(), CHECK_HANDLER_INTERVAL_MS);

        // THEN both focus call and ConnectionService are null.
        assertNull(mFocusManagerUT.getCurrentFocusCall());
        assertNull(mFocusManagerUT.getCurrentFocusConnectionService());
    }

    @SmallTest
    @Test
    public void testCallStateChangedAffectCallFocus() {
        // GIVEN the ConnectionServiceFocusManager with the focus ConnectionService.
        CallFocus activeCall = createFakeCall(mActiveConnectionService, CallState.ACTIVE);
        CallFocus newActivateCall = createFakeCall(mActiveConnectionService, CallState.ACTIVE);
        requestFocus(activeCall, null);

        // WHEN hold the active call.
        int previousState = activeCall.getState();
        when(activeCall.getState()).thenReturn(CallState.ON_HOLD);
        mCallsManagerListener.onCallStateChanged(
                (Call) activeCall, previousState, activeCall.getState());
        waitForHandlerAction(mFocusManagerUT.getHandler(), CHECK_HANDLER_INTERVAL_MS);

        // THEN the focus call is null
        assertNull(mFocusManagerUT.getCurrentFocusCall());
        // and the focus ConnectionService is not changed.
        assertEquals(mActiveConnectionService, mFocusManagerUT.getCurrentFocusConnectionService());

        // WHEN a new active call is added.
        when(newActivateCall.getState()).thenReturn(CallState.ACTIVE);
        mCallsManagerListener.onCallAdded((Call) newActivateCall);
        waitForHandlerAction(mFocusManagerUT.getHandler(), CHECK_HANDLER_INTERVAL_MS);

        // THEN the focus call changed as excepted.
        assertEquals(newActivateCall, mFocusManagerUT.getCurrentFocusCall());
    }

    @SmallTest
    @Test
    public void testCallStateChangedDoesNotAffectCallFocusIfConnectionServiceIsDifferent() {
        // GIVEN the ConnectionServiceFocusManager with the focus ConnectionService
        requestFocus(mActiveCall, null);

        // WHEN a new active call is added (actually this should not happen).
        when(mNewCall.getState()).thenReturn(CallState.ACTIVE);
        mCallsManagerListener.onCallAdded((Call) mNewCall);

        // THEN the call focus isn't changed.
        assertEquals(mActiveCall, mFocusManagerUT.getCurrentFocusCall());

        // WHEN the hold the active call.
        when(mActiveCall.getState()).thenReturn(CallState.ON_HOLD);
        mCallsManagerListener.onCallStateChanged(
                (Call) mActiveCall, CallState.ACTIVE, CallState.ON_HOLD);
        waitForHandlerAction(mFocusManagerUT.getHandler(), CHECK_HANDLER_INTERVAL_MS);

        // THEN the focus call is null.
        assertNull(mFocusManagerUT.getCurrentFocusCall());
    }

    @SmallTest
    @Test
    public void testFocusCallIsNullWhenRemoveTheFocusCall() {
        // GIVEN the ConnectionServiceFocusManager with the focus ConnectionService
        requestFocus(mActiveCall, null);
        assertEquals(mActiveCall, mFocusManagerUT.getCurrentFocusCall());

        // WHEN remove the active call
        mCallsManagerListener.onCallRemoved((Call) mActiveCall);
        waitForHandlerAction(mFocusManagerUT.getHandler(), CHECK_HANDLER_INTERVAL_MS);

        // THEN the focus call is null
        assertNull(mFocusManagerUT.getCurrentFocusCall());
    }

    @SmallTest
    @Test
    public void testConnectionServiceFocusDeath() {
        // GIVEN the ConnectionServiceFocusManager with the focus ConnectionService
        requestFocus(mActiveCall, null);
        ConnectionServiceFocusListener connSvrFocusListener =
                getConnectionServiceFocusListener(mActiveConnectionService);

        // WHEN the active connection service focus is death
        connSvrFocusListener.onConnectionServiceDeath(mActiveConnectionService);
        waitForHandlerAction(mFocusManagerUT.getHandler(), CHECK_HANDLER_INTERVAL_MS);

        // THEN both connection service focus and call focus are null.
        assertNull(mFocusManagerUT.getCurrentFocusConnectionService());
        assertNull(mFocusManagerUT.getCurrentFocusCall());
    }

    @SmallTest
    @Test
    public void testNonExternalCallChangedToExternalCall() {
        // GIVEN the ConnectionServiceFocusManager with the focus ConnectionService.
        requestFocus(mActiveCall, null);
        assertTrue(mFocusManagerUT.getAllCall().contains(mActiveCall));

        // WHEN the non-external call changed to external call
        mCallsManagerListener.onExternalCallChanged((Call) mActiveCall, true);
        waitForHandlerAction(mFocusManagerUT.getHandler(), CHECK_HANDLER_INTERVAL_MS);

        // THEN the call should be removed as it's an external call now.
        assertFalse(mFocusManagerUT.getAllCall().contains(mActiveCall));
    }

    @SmallTest
    @Test
    public void testExternalCallChangedToNonExternalCall() {
        // GIVEN the ConnectionServiceFocusManager without focus ConnectionService

        // WHEN an external call changed to external call
        mCallsManagerListener.onExternalCallChanged((Call) mActiveCall, false);
        waitForHandlerAction(mFocusManagerUT.getHandler(), CHECK_HANDLER_INTERVAL_MS);

        // THEN the call should be added as it's a non-external call now.
        assertTrue(mFocusManagerUT.getAllCall().contains(mActiveCall));
    }

    @SmallTest
    @Test
    public void testNonFocusableDoesntChangeFocus() {
        // GIVEN the ConnectionServiceFocusManager with the focus ConnectionService
        requestFocus(mActiveCall, null);

        // WHEN a new non-focusable call is added.
        when(mNewCall.isFocusable()).thenReturn(false);
        mCallsManagerListener.onCallAdded((Call) mNewCall);

        // THEN the call focus isn't changed.
        assertEquals(mActiveCall, mFocusManagerUT.getCurrentFocusCall());
    }

    private void requestFocus(CallFocus call, RequestFocusCallback callback) {
        mCallsManagerListener.onCallAdded((Call) call);
        mFocusManagerUT.requestFocus(call, callback);
        waitForHandlerAction(mFocusManagerUT.getHandler(), CHECK_HANDLER_INTERVAL_MS);
    }

    private static void verifyRequestFocusDone(
            ConnectionServiceFocusManager focusManager,
            CallFocus call,
            RequestFocusCallback callback,
            boolean isConnectionServiceFocusChanged) {
        verify(callback).onRequestFocusDone(call);
        verify(call.getConnectionServiceWrapper(), times(isConnectionServiceFocusChanged ? 1 : 0))
                .connectionServiceFocusGained();
        assertEquals(
                call.getConnectionServiceWrapper(),
                focusManager.getCurrentFocusConnectionService());
    }

    /**
     * Returns the {@link ConnectionServiceFocusListener} of the ConnectionServiceFocusManager.
     * Make sure the given parameter {@code ConnectionServiceFocus} is a mock object and
     * {@link ConnectionServiceFocus#setConnectionServiceFocusListener(
     * ConnectionServiceFocusListener)} is called.
     */
    private static ConnectionServiceFocusListener getConnectionServiceFocusListener(
            ConnectionServiceFocus connSvrFocus) {
        ArgumentCaptor<ConnectionServiceFocusListener> captor =
                ArgumentCaptor.forClass(ConnectionServiceFocusListener.class);
        verify(connSvrFocus).setConnectionServiceFocusListener(captor.capture());
        return captor.getValue();
    }

    private static Call createFakeCall(ConnectionServiceFocus connSvr, int state) {
        Call call = Mockito.mock(Call.class);
        when(call.getConnectionServiceWrapper()).thenReturn(connSvr);
        when(call.getState()).thenReturn(state);
        when(call.isFocusable()).thenReturn(true);
        return call;
    }
}
