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

package com.android.server.wifi.hotspot2;

import com.android.org.conscrypt.TrustManagerImpl;

import static org.junit.Assert.*;
import static org.mockito.Mockito.any;
import static org.mockito.Mockito.atLeastOnce;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.net.Network;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiSsid;
import android.net.wifi.hotspot2.IProvisioningCallback;
import android.net.wifi.hotspot2.OsuProvider;
import android.net.wifi.hotspot2.ProvisioningCallback;
import android.os.Handler;
import android.os.RemoteException;
import android.os.test.TestLooper;
import android.support.test.filters.SmallTest;

import org.junit.Before;
import org.junit.Test;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.net.URL;
import java.security.KeyStore;

import javax.net.ssl.SSLContext;
/**
 * Unit tests for {@link com.android.server.wifi.hotspot2.PasspointProvisioner}.
 */
@SmallTest
public class PasspointProvisionerTest {
    private static final String TAG = "PasspointProvisionerTest";

    private static final int TEST_UID = 1500;

    private PasspointProvisioner mPasspointProvisioner;
    private TestLooper mLooper = new TestLooper();
    private Handler mHandler;
    private OsuNetworkConnection.Callbacks mOsuNetworkCallbacks;
    private PasspointProvisioner.OsuServerCallbacks mOsuServerCallbacks;
    private ArgumentCaptor<OsuNetworkConnection.Callbacks> mOsuNetworkCallbacksCaptor =
            ArgumentCaptor.forClass(OsuNetworkConnection.Callbacks.class);
    private ArgumentCaptor<PasspointProvisioner.OsuServerCallbacks> mOsuServerCallbacksCaptor =
            ArgumentCaptor.forClass(PasspointProvisioner.OsuServerCallbacks.class);
    private ArgumentCaptor<Handler> mHandlerCaptor = ArgumentCaptor.forClass(Handler.class);
    private OsuProvider mOsuProvider;
    private TrustManagerImpl mDelegate;


    @Mock PasspointObjectFactory mObjectFactory;
    @Mock Context mContext;
    @Mock WifiManager mWifiManager;
    @Mock IProvisioningCallback mCallback;
    @Mock OsuNetworkConnection mOsuNetworkConnection;
    @Mock OsuServerConnection mOsuServerConnection;
    @Mock Network mNetwork;
    @Mock WfaKeyStore mWfaKeyStore;
    @Mock KeyStore mKeyStore;
    @Mock SSLContext mTlsContext;

    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);
        when(mWifiManager.isWifiEnabled()).thenReturn(true);
        when(mObjectFactory.makeOsuNetworkConnection(any(Context.class)))
                .thenReturn(mOsuNetworkConnection);
        when(mObjectFactory.makeOsuServerConnection()).thenReturn(mOsuServerConnection);
        when(mWfaKeyStore.get()).thenReturn(mKeyStore);
        when(mObjectFactory.makeWfaKeyStore()).thenReturn(mWfaKeyStore);
        when(mObjectFactory.getSSLContext(any(String.class))).thenReturn(mTlsContext);
        doReturn(mWifiManager).when(mContext)
                .getSystemService(eq(Context.WIFI_SERVICE));
        mPasspointProvisioner = new PasspointProvisioner(mContext, mObjectFactory);
        when(mOsuNetworkConnection.connect(any(WifiSsid.class), any())).thenReturn(true);
        when(mOsuServerConnection.connect(any(URL.class), any(Network.class))).thenReturn(true);
        when(mOsuServerConnection.validateProvider(any(String.class))).thenReturn(true);
        when(mOsuServerConnection.canValidateServer()).thenReturn(true);
        mPasspointProvisioner.enableVerboseLogging(1);
        mOsuProvider = PasspointProvisioningTestUtil.generateOsuProvider(true);
        mDelegate = new TrustManagerImpl(PasspointProvisioningTestUtil.createFakeKeyStore());
        when(mObjectFactory.getTrustManagerImpl(any(KeyStore.class))).thenReturn(mDelegate);
    }

    private void initAndStartProvisioning() {
        mPasspointProvisioner.init(mLooper.getLooper());
        verify(mOsuNetworkConnection).init(mHandlerCaptor.capture());

        mHandler = mHandlerCaptor.getValue();
        assertEquals(mHandler.getLooper(), mLooper.getLooper());

        mLooper.dispatchAll();

        assertTrue(mPasspointProvisioner.startSubscriptionProvisioning(
                TEST_UID, mOsuProvider, mCallback));
        // Runnable posted by the provisioning start request
        assertEquals(mHandler.hasMessagesOrCallbacks(), true);
        mLooper.dispatchAll();
        assertEquals(mHandler.hasMessagesOrCallbacks(), false);

        verify(mOsuNetworkConnection, atLeastOnce())
                .setEventCallback(mOsuNetworkCallbacksCaptor.capture());
        mOsuNetworkCallbacks = mOsuNetworkCallbacksCaptor.getAllValues().get(0);
        verify(mOsuServerConnection, atLeastOnce())
                .setEventCallback(mOsuServerCallbacksCaptor.capture());
        mOsuServerCallbacks = mOsuServerCallbacksCaptor.getAllValues().get(0);
    }

    /**
     * Verifies initialization and starting subscription provisioning flow
     */
    @Test
    public void verifyInitAndStartProvisioning() throws RemoteException {
        initAndStartProvisioning();
    }

    /**
     * Verifies initialization and starting subscription provisioning flow
     */
    @Test
    public void verifyProvisioningUnavailable() throws RemoteException {
        when(mOsuServerConnection.canValidateServer()).thenReturn(false);
        initAndStartProvisioning();
        verify(mCallback).onProvisioningFailure(
                ProvisioningCallback.OSU_FAILURE_PROVISIONING_NOT_AVAILABLE);
    }

    /**
     * Verifies existing provisioning flow is aborted before starting another one.
     */
    @Test
    public void verifyProvisioningAbort() throws RemoteException {
        initAndStartProvisioning();
        verify(mCallback).onProvisioningStatus(
                ProvisioningCallback.OSU_STATUS_AP_CONNECTING);
        IProvisioningCallback mCallback2 = mock(IProvisioningCallback.class);
        assertTrue(mPasspointProvisioner.startSubscriptionProvisioning(
                TEST_UID, mOsuProvider, mCallback2));
        // Runnable posted by the provisioning start request
        assertEquals(mHandler.hasMessagesOrCallbacks(), true);
        mLooper.dispatchAll();
        assertEquals(mHandler.hasMessagesOrCallbacks(), false);
        verify(mCallback).onProvisioningFailure(
                ProvisioningCallback.OSU_FAILURE_PROVISIONING_ABORTED);
    }

    /**
     * Verifies that the right provisioning callbacks are invoked as the provisioner progresses
     * to associating with the OSU AP, connecting to the OSU Server, validating its certificates
     * and verifying the OSU provider.
     */
    @Test
    public void verifyProvisioningFlow() throws RemoteException {
        initAndStartProvisioning();
        // state exptected is WAITING_TO_CONNECT
        verify(mCallback).onProvisioningStatus(ProvisioningCallback.OSU_STATUS_AP_CONNECTING);
        // Connection to OSU AP successful
        mOsuNetworkCallbacks.onConnected(mNetwork);
        // state expected is OSU_AP_CONNECTED
        verify(mCallback).onProvisioningStatus(ProvisioningCallback.OSU_STATUS_AP_CONNECTED);
        // state expected is OSU_SERVER_CONNECTED
        verify(mCallback).onProvisioningStatus(ProvisioningCallback.OSU_STATUS_SERVER_CONNECTED);
        // Wait for server validation status
        assertFalse(mHandler.hasMessagesOrCallbacks());
        // Server validation passed
        mOsuServerCallbacks.onServerValidationStatus(mOsuServerCallbacks.getSessionId(), true);
        assertTrue(mHandler.hasMessagesOrCallbacks());
        mLooper.dispatchAll();
        // state expected is OSU_SERVER_VALIDATED
        verify(mCallback).onProvisioningStatus(ProvisioningCallback.OSU_STATUS_SERVER_VALIDATED);
        // state expected is OSU_PROVIDER_VALIDATED
        verify(mCallback).onProvisioningStatus(ProvisioningCallback.OSU_STATUS_PROVIDER_VERIFIED);
        // Osu provider verification is the last current step in the flow, no more runnables posted.
        assertFalse(mHandler.hasMessagesOrCallbacks());
    }

    /**
     * Verifies that if connection attempt to OSU AP fails, corresponding error callback is invoked.
     */
    @Test
    public void verifyConnectAttemptFailure() throws RemoteException {
        when(mOsuNetworkConnection.connect(any(WifiSsid.class), any())).thenReturn(false);
        initAndStartProvisioning();

        // Since connection attempt fails, directly move to FAILED_STATE
        verify(mCallback).onProvisioningFailure(ProvisioningCallback.OSU_FAILURE_AP_CONNECTION);
        // Failure case, no more runnables posted
        assertFalse(mHandler.hasMessagesOrCallbacks());
    }

    /**
     * Verifies that after connection attemp succeeds and a network disconnection occurs, the
     * corresponding failure callback is invoked.
     */
    @Test
    public void verifyConnectAttemptedAndConnectionFailed() throws RemoteException {
        initAndStartProvisioning();

        // state expected is WAITING_TO_CONNECT
        verify(mCallback).onProvisioningStatus(ProvisioningCallback.OSU_STATUS_AP_CONNECTING);
        // Connection to OSU AP fails
        mOsuNetworkCallbacks.onDisconnected();
        // Move to failed state
        verify(mCallback).onProvisioningFailure(ProvisioningCallback.OSU_FAILURE_AP_CONNECTION);
        // Failure case, no more runnables posted
        assertEquals(mHandler.hasMessagesOrCallbacks(), false);
    }

    /**
     * Verifies that a connection drop is reported via callback.
     */
    @Test
    public void verifyConnectionDrop() throws RemoteException {
        initAndStartProvisioning();
        // state expected WAITING_TO_CONNECT
        verify(mCallback).onProvisioningStatus(ProvisioningCallback.OSU_STATUS_AP_CONNECTING);
        // Connection to OSU AP successful
        mOsuNetworkCallbacks.onConnected(mNetwork);
        // state expected OSU_AP_CONNECTED
        verify(mCallback).onProvisioningStatus(ProvisioningCallback.OSU_STATUS_AP_CONNECTED);
        // state expected OSU_SERVER_CONNECTED
        verify(mCallback).onProvisioningStatus(ProvisioningCallback.OSU_STATUS_SERVER_CONNECTED);
        // Disconnect received
        mOsuNetworkCallbacks.onDisconnected();
        // Move to failed state
        verify(mCallback).onProvisioningFailure(ProvisioningCallback.OSU_FAILURE_AP_CONNECTION);
        // No more callbacks, Osu server validation not initiated
        assertFalse(mHandler.hasMessagesOrCallbacks());
    }

    /**
     * Verifies that Wifi Disable while provisioning is communicated as provisioning failure
     */
    @Test
    public void verifyFailureDueToWifiDisable() throws RemoteException {
        initAndStartProvisioning();
        // state expected WAITING_TO_CONNECT
        verify(mCallback).onProvisioningStatus(ProvisioningCallback.OSU_STATUS_AP_CONNECTING);
        // Connection to OSU AP successful
        mOsuNetworkCallbacks.onConnected(mNetwork);
        // state expected is OSU_AP_CONNECTED
        verify(mCallback).onProvisioningStatus(ProvisioningCallback.OSU_STATUS_AP_CONNECTED);
        // state expecte is OSU_SERVER_CONNECTED
        verify(mCallback).onProvisioningStatus(ProvisioningCallback.OSU_STATUS_SERVER_CONNECTED);
        // Wifi disabled notification
        mOsuNetworkCallbacks.onWifiDisabled();
        // Wifi Disable is processed first and move to failed state
        verify(mCallback).onProvisioningFailure(ProvisioningCallback.OSU_FAILURE_AP_CONNECTION);
        // OSU server connection event is not handled
        assertFalse(mHandler.hasMessagesOrCallbacks());
    }

   /**
     * Verifies that the right provisioning callbacks are invoked as the provisioner connects
     * to OSU AP and OSU server and that invalid server URL generates the right error callback.
     */
    @Test
    public void verifyInvalidOsuServerURL() throws RemoteException {
        mOsuProvider = PasspointProvisioningTestUtil.generateInvalidServerUrlOsuProvider();
        initAndStartProvisioning();
        // Attempting to connect to OSU server fails due to invalid server URL, move to failed state
        verify(mCallback).onProvisioningFailure(
                ProvisioningCallback.OSU_FAILURE_SERVER_URL_INVALID);
        // No further runnables posted
        assertEquals(mHandler.hasMessagesOrCallbacks(), false);
    }

    /**
     * Verifies that the right provisioning callbacks are invoked as the provisioner progresses
     * to associating with the OSU AP and connection to OSU server fails.
     */
    @Test
    public void verifyServerConnectionFailure() throws RemoteException {
        initAndStartProvisioning();
        when(mOsuServerConnection.connect(any(URL.class), any(Network.class))).thenReturn(false);
        // expected state is WAITING_TO_CONNECT
        verify(mCallback).onProvisioningStatus(ProvisioningCallback.OSU_STATUS_AP_CONNECTING);
        // Connection to OSU AP is successful
        mOsuNetworkCallbacks.onConnected(mNetwork);
        // state expected is OSU_AP_CONNECTED
        verify(mCallback).onProvisioningStatus(ProvisioningCallback.OSU_STATUS_AP_CONNECTED);
        // Connection to OSU Server fails, move to failed state
        verify(mCallback).onProvisioningFailure(ProvisioningCallback.OSU_FAILURE_SERVER_CONNECTION);
        // No further runnables posted
        assertFalse(mHandler.hasMessagesOrCallbacks());
    }

    /**
     * Verifies that the right provisioning callbacks are invoked as the provisioner is unable
     * to validate the OSU Server
     */
    @Test
    public void verifyServerValidationFailure() throws RemoteException {
        initAndStartProvisioning();
        // state expected is WAITING_TO_CONNECT
        verify(mCallback).onProvisioningStatus(ProvisioningCallback.OSU_STATUS_AP_CONNECTING);
        // Connection to OSU AP is successful
        mOsuNetworkCallbacks.onConnected(mNetwork);
        // state expected is OSU_AP_CONNECTED
        verify(mCallback).onProvisioningStatus(ProvisioningCallback.OSU_STATUS_AP_CONNECTED);
        // state expected is OSU_SERVER_CONNECTED
        verify(mCallback).onProvisioningStatus(ProvisioningCallback.OSU_STATUS_SERVER_CONNECTED);
        // OSU Server validation of certs fails
        mOsuServerCallbacks.onServerValidationStatus(mOsuServerCallbacks.getSessionId(), false);
        // Runnable posted by server callback
        assertTrue(mHandler.hasMessagesOrCallbacks());
        mLooper.dispatchAll();
        // Server validation failure, move to failed state
        verify(mCallback).onProvisioningFailure(ProvisioningCallback.OSU_FAILURE_SERVER_VALIDATION);
        // No further runnables posted
        assertFalse(mHandler.hasMessagesOrCallbacks());
    }

    /**
     * Verifies that the status for server validation from a previous session is ignored
     * by the provisioning state machine
     */
    @Test
    public void verifyServerValidationFailurePreviousSession() throws RemoteException {
        initAndStartProvisioning();
        // state expected is WAITING_TO_CONNECT
        verify(mCallback).onProvisioningStatus(ProvisioningCallback.OSU_STATUS_AP_CONNECTING);
        // Connection to OSU AP is successful
        mOsuNetworkCallbacks.onConnected(mNetwork);
        // state expected is OSU_AP_CONNECTED
        verify(mCallback).onProvisioningStatus(ProvisioningCallback.OSU_STATUS_AP_CONNECTED);
        // state expected is OSU_SERVER_CONNECTED
        verify(mCallback).onProvisioningStatus(ProvisioningCallback.OSU_STATUS_SERVER_CONNECTED);
        // OSU Server validation of certs failure but from a previous session
        mOsuServerCallbacks.onServerValidationStatus(mOsuServerCallbacks.getSessionId() - 1, false);
        // Runnable posted by server callback
        assertTrue(mHandler.hasMessagesOrCallbacks());
        mLooper.dispatchAll();
        // Server validation failure, move to failed state
        verify(mCallback, never()).onProvisioningFailure(
                ProvisioningCallback.OSU_FAILURE_SERVER_VALIDATION);
        // No further runnables posted
        assertFalse(mHandler.hasMessagesOrCallbacks());
    }

    /**
     * Verifies that the status for server validation from a previous session is ignored
     * by the provisioning state machine
     */
    @Test
    public void verifyServerValidationSuccessPreviousSession() throws RemoteException {
        initAndStartProvisioning();
        // state expected is WAITING_TO_CONNECT
        verify(mCallback).onProvisioningStatus(ProvisioningCallback.OSU_STATUS_AP_CONNECTING);
        // Connection to OSU AP is successful
        mOsuNetworkCallbacks.onConnected(mNetwork);
        // state expected is OSU_AP_CONNECTED
        verify(mCallback).onProvisioningStatus(ProvisioningCallback.OSU_STATUS_AP_CONNECTED);
        // state expected is OSU_SERVER_CONNECTED
        verify(mCallback).onProvisioningStatus(ProvisioningCallback.OSU_STATUS_SERVER_CONNECTED);
        // OSU Server validation success but from a previous session
        mOsuServerCallbacks.onServerValidationStatus(mOsuServerCallbacks.getSessionId() - 1, true);
        // Runnable posted by server callback
        assertTrue(mHandler.hasMessagesOrCallbacks());
        mLooper.dispatchAll();
        // Server validation failure, move to failed state
        verify(mCallback, never()).onProvisioningStatus(
                ProvisioningCallback.OSU_STATUS_SERVER_VALIDATED);
        // No further runnables posted
        assertFalse(mHandler.hasMessagesOrCallbacks());
    }

    /**
     * Verifies that the right provisioning callbacks are invoked when provisioner is unable
     * to validate the OSU provider
     */
    @Test
    public void verifyProviderVerificationFailure() throws RemoteException {
        initAndStartProvisioning();
        when(mOsuServerConnection.validateProvider(any(String.class))).thenReturn(false);
        // state expected is WAITING_TO_CONNECT
        verify(mCallback).onProvisioningStatus(ProvisioningCallback.OSU_STATUS_AP_CONNECTING);
        // Connection to OSU AP successful
        mOsuNetworkCallbacks.onConnected(mNetwork);
        // state expected is OSU_AP_CONNECTED
        verify(mCallback).onProvisioningStatus(ProvisioningCallback.OSU_STATUS_AP_CONNECTED);
        verify(mCallback).onProvisioningStatus(ProvisioningCallback.OSU_STATUS_SERVER_CONNECTED);
        // Wait for OSU server validation callback
        mOsuServerCallbacks.onServerValidationStatus(mOsuServerCallbacks.getSessionId(), true);
        // Runnable posted by server callback
        assertTrue(mHandler.hasMessagesOrCallbacks());
        // OSU server validation success posts another runnable to validate the provider
        mLooper.dispatchAll();
        // state expected is OSU_SERVER_VALIDATED
        verify(mCallback).onProvisioningStatus(ProvisioningCallback.OSU_STATUS_SERVER_VALIDATED);
        // Provider validation failure is processed next, move to failed state
        verify(mCallback).onProvisioningFailure(
                ProvisioningCallback.OSU_FAILURE_PROVIDER_VERIFICATION);
        // No further runnables posted
        assertFalse(mHandler.hasMessagesOrCallbacks());
    }
}
