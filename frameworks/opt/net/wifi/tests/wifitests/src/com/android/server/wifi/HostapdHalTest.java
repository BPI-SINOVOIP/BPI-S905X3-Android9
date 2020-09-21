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
package com.android.server.wifi;

import static org.junit.Assert.*;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anyLong;
import static org.mockito.Matchers.anyString;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.*;

import android.content.Context;
import android.hardware.wifi.hostapd.V1_0.HostapdStatus;
import android.hardware.wifi.hostapd.V1_0.HostapdStatusCode;
import android.hardware.wifi.hostapd.V1_0.IHostapd;
import android.hidl.manager.V1_0.IServiceManager;
import android.hidl.manager.V1_0.IServiceNotification;
import android.net.wifi.WifiConfiguration;
import android.os.IHwBinder;
import android.os.RemoteException;

import com.android.internal.R;
import com.android.server.wifi.util.NativeUtil;

import org.junit.Before;
import org.junit.Test;
import org.mockito.ArgumentCaptor;
import org.mockito.InOrder;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

/**
 * Unit tests for HostapdHal
 */
public class HostapdHalTest {
    private static final String IFACE_NAME = "mock-wlan0";
    private static final String NETWORK_SSID = "test-ssid";
    private static final String NETWORK_PSK = "test-psk";

    private @Mock Context mContext;
    private @Mock IServiceManager mServiceManagerMock;
    private @Mock IHostapd mIHostapdMock;
    private @Mock WifiNative.HostapdDeathEventHandler mHostapdHalDeathHandler;
    private MockResources mResources;
    HostapdStatus mStatusSuccess;
    HostapdStatus mStatusFailure;
    private HostapdHal mHostapdHal;
    private ArgumentCaptor<IHwBinder.DeathRecipient> mServiceManagerDeathCaptor =
            ArgumentCaptor.forClass(IHwBinder.DeathRecipient.class);
    private ArgumentCaptor<IHwBinder.DeathRecipient> mHostapdDeathCaptor =
            ArgumentCaptor.forClass(IHwBinder.DeathRecipient.class);
    private ArgumentCaptor<IServiceNotification.Stub> mServiceNotificationCaptor =
            ArgumentCaptor.forClass(IServiceNotification.Stub.class);
    private ArgumentCaptor<IHostapd.IfaceParams> mIfaceParamsCaptor =
            ArgumentCaptor.forClass(IHostapd.IfaceParams.class);
    private ArgumentCaptor<IHostapd.NetworkParams> mNetworkParamsCaptor =
            ArgumentCaptor.forClass(IHostapd.NetworkParams.class);
    private InOrder mInOrder;

    private class HostapdHalSpy extends HostapdHal {
        HostapdHalSpy() {
            super(mContext);
        }

        @Override
        protected IServiceManager getServiceManagerMockable() throws RemoteException {
            return mServiceManagerMock;
        }

        @Override
        protected IHostapd getHostapdMockable() throws RemoteException {
            return mIHostapdMock;
        }
    }

    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);
        mResources = new MockResources();
        mResources.setBoolean(R.bool.config_wifi_softap_acs_supported, false);
        mResources.setBoolean(R.bool.config_wifi_softap_ieee80211ac_supported, false);

        mStatusSuccess = createHostapdStatus(HostapdStatusCode.SUCCESS);
        mStatusFailure = createHostapdStatus(HostapdStatusCode.FAILURE_UNKNOWN);

        when(mContext.getResources()).thenReturn(mResources);
        when(mServiceManagerMock.linkToDeath(any(IHwBinder.DeathRecipient.class),
                anyLong())).thenReturn(true);
        when(mServiceManagerMock.registerForNotifications(anyString(), anyString(),
                any(IServiceNotification.Stub.class))).thenReturn(true);
        when(mIHostapdMock.linkToDeath(any(IHwBinder.DeathRecipient.class),
                anyLong())).thenReturn(true);
        when(mIHostapdMock.linkToDeath(any(IHwBinder.DeathRecipient.class),
                anyLong())).thenReturn(true);
        when(mIHostapdMock.addAccessPoint(
                mIfaceParamsCaptor.capture(), mNetworkParamsCaptor.capture()))
                .thenReturn(mStatusSuccess);
        when(mIHostapdMock.removeAccessPoint(any())).thenReturn(mStatusSuccess);
        mHostapdHal = new HostapdHalSpy();
    }

    /**
     * Sunny day scenario for HostapdHal initialization
     * Asserts successful initialization
     */
    @Test
    public void testInitialize_success() throws Exception {
        executeAndValidateInitializationSequence(false, false);
    }

    /**
     * Failure scenario for HostapdHal initialization
     */
    @Test
    public void testInitialize_registerException() throws Exception {
        executeAndValidateInitializationSequence(true, false);
    }

    /**
     * Failure scenario for HostapdHal initialization
     */
    @Test
    public void testInitialize_registerFailure() throws Exception {
        executeAndValidateInitializationSequence(false, true);
    }

    /**
     * Verifies the hostapd death handling.
     */
    @Test
    public void testDeathHandling() throws Exception {
        executeAndValidateInitializationSequence();

        mHostapdHal.registerDeathHandler(mHostapdHalDeathHandler);
        mHostapdDeathCaptor.getValue().serviceDied(0);
        verify(mHostapdHalDeathHandler).onDeath();
    }

    /**
     * Verifies the successful addition of access point.
     */
    @Test
    public void testAddAccessPointSuccess_Psk_Band2G() throws Exception {
        executeAndValidateInitializationSequence();
        final int apChannel = 6;

        WifiConfiguration configuration = new WifiConfiguration();
        configuration.SSID = NETWORK_SSID;
        configuration.hiddenSSID = false;
        configuration.allowedKeyManagement.set(WifiConfiguration.KeyMgmt.WPA2_PSK);
        configuration.preSharedKey = NETWORK_PSK;
        configuration.apChannel = apChannel;
        configuration.apBand = WifiConfiguration.AP_BAND_2GHZ;

        assertTrue(mHostapdHal.addAccessPoint(IFACE_NAME, configuration));
        verify(mIHostapdMock).addAccessPoint(any(), any());

        assertEquals(IFACE_NAME, mIfaceParamsCaptor.getValue().ifaceName);
        assertTrue(mIfaceParamsCaptor.getValue().hwModeParams.enable80211N);
        assertFalse(mIfaceParamsCaptor.getValue().hwModeParams.enable80211AC);
        assertEquals(IHostapd.Band.BAND_2_4_GHZ, mIfaceParamsCaptor.getValue().channelParams.band);
        assertFalse(mIfaceParamsCaptor.getValue().channelParams.enableAcs);
        assertFalse(mIfaceParamsCaptor.getValue().channelParams.acsShouldExcludeDfs);
        assertEquals(apChannel, mIfaceParamsCaptor.getValue().channelParams.channel);

        assertEquals(NativeUtil.stringToByteArrayList(NETWORK_SSID),
                mNetworkParamsCaptor.getValue().ssid);
        assertFalse(mNetworkParamsCaptor.getValue().isHidden);
        assertEquals(IHostapd.EncryptionType.WPA2, mNetworkParamsCaptor.getValue().encryptionType);
        assertEquals(NETWORK_PSK, mNetworkParamsCaptor.getValue().pskPassphrase);
    }

    /**
     * Verifies the successful addition of access point.
     */
    @Test
    public void testAddAccessPointSuccess_Open_Band5G() throws Exception {
        executeAndValidateInitializationSequence();
        final int apChannel = 18;

        WifiConfiguration configuration = new WifiConfiguration();
        configuration.SSID = NETWORK_SSID;
        configuration.hiddenSSID = true;
        configuration.allowedKeyManagement.set(WifiConfiguration.KeyMgmt.NONE);
        configuration.apChannel = apChannel;
        configuration.apBand = WifiConfiguration.AP_BAND_5GHZ;

        assertTrue(mHostapdHal.addAccessPoint(IFACE_NAME, configuration));
        verify(mIHostapdMock).addAccessPoint(any(), any());

        assertEquals(IFACE_NAME, mIfaceParamsCaptor.getValue().ifaceName);
        assertTrue(mIfaceParamsCaptor.getValue().hwModeParams.enable80211N);
        assertFalse(mIfaceParamsCaptor.getValue().hwModeParams.enable80211AC);
        assertEquals(IHostapd.Band.BAND_5_GHZ, mIfaceParamsCaptor.getValue().channelParams.band);
        assertFalse(mIfaceParamsCaptor.getValue().channelParams.enableAcs);
        assertFalse(mIfaceParamsCaptor.getValue().channelParams.acsShouldExcludeDfs);
        assertEquals(apChannel, mIfaceParamsCaptor.getValue().channelParams.channel);

        assertEquals(NativeUtil.stringToByteArrayList(NETWORK_SSID),
                mNetworkParamsCaptor.getValue().ssid);
        assertTrue(mNetworkParamsCaptor.getValue().isHidden);
        assertEquals(IHostapd.EncryptionType.NONE, mNetworkParamsCaptor.getValue().encryptionType);
        assertNotNull(mNetworkParamsCaptor.getValue().pskPassphrase);
    }

    /**
     * Verifies the successful addition of access point.
     */
    @Test
    public void testAddAccessPointSuccess_Psk_Band5G_Hidden() throws Exception {
        executeAndValidateInitializationSequence();
        final int apChannel = 18;

        WifiConfiguration configuration = new WifiConfiguration();
        configuration.SSID = NETWORK_SSID;
        configuration.hiddenSSID = true;
        configuration.allowedKeyManagement.set(WifiConfiguration.KeyMgmt.WPA2_PSK);
        configuration.preSharedKey = NETWORK_PSK;
        configuration.apChannel = apChannel;
        configuration.apBand = WifiConfiguration.AP_BAND_5GHZ;

        assertTrue(mHostapdHal.addAccessPoint(IFACE_NAME, configuration));
        verify(mIHostapdMock).addAccessPoint(any(), any());

        assertEquals(IFACE_NAME, mIfaceParamsCaptor.getValue().ifaceName);
        assertTrue(mIfaceParamsCaptor.getValue().hwModeParams.enable80211N);
        assertFalse(mIfaceParamsCaptor.getValue().hwModeParams.enable80211AC);
        assertEquals(IHostapd.Band.BAND_5_GHZ, mIfaceParamsCaptor.getValue().channelParams.band);
        assertFalse(mIfaceParamsCaptor.getValue().channelParams.enableAcs);
        assertFalse(mIfaceParamsCaptor.getValue().channelParams.acsShouldExcludeDfs);
        assertEquals(apChannel, mIfaceParamsCaptor.getValue().channelParams.channel);

        assertEquals(NativeUtil.stringToByteArrayList(NETWORK_SSID),
                mNetworkParamsCaptor.getValue().ssid);
        assertTrue(mNetworkParamsCaptor.getValue().isHidden);
        assertEquals(IHostapd.EncryptionType.WPA2, mNetworkParamsCaptor.getValue().encryptionType);
        assertEquals(NETWORK_PSK, mNetworkParamsCaptor.getValue().pskPassphrase);
    }

    /**
     * Verifies the successful addition of access point.
     */
    @Test
    public void testAddAccessPointSuccess_Psk_Band2G_WithACS() throws Exception {
        // Enable ACS in the config.
        mResources.setBoolean(R.bool.config_wifi_softap_acs_supported, true);
        mHostapdHal = new HostapdHalSpy();

        executeAndValidateInitializationSequence();
        final int apChannel = 6;

        WifiConfiguration configuration = new WifiConfiguration();
        configuration.SSID = NETWORK_SSID;
        configuration.hiddenSSID = false;
        configuration.allowedKeyManagement.set(WifiConfiguration.KeyMgmt.WPA2_PSK);
        configuration.preSharedKey = NETWORK_PSK;
        configuration.apChannel = apChannel;
        configuration.apBand = WifiConfiguration.AP_BAND_2GHZ;

        assertTrue(mHostapdHal.addAccessPoint(IFACE_NAME, configuration));
        verify(mIHostapdMock).addAccessPoint(any(), any());

        assertEquals(IFACE_NAME, mIfaceParamsCaptor.getValue().ifaceName);
        assertTrue(mIfaceParamsCaptor.getValue().hwModeParams.enable80211N);
        assertFalse(mIfaceParamsCaptor.getValue().hwModeParams.enable80211AC);
        assertEquals(IHostapd.Band.BAND_2_4_GHZ, mIfaceParamsCaptor.getValue().channelParams.band);
        assertTrue(mIfaceParamsCaptor.getValue().channelParams.enableAcs);
        assertTrue(mIfaceParamsCaptor.getValue().channelParams.acsShouldExcludeDfs);

        assertEquals(NativeUtil.stringToByteArrayList(NETWORK_SSID),
                mNetworkParamsCaptor.getValue().ssid);
        assertFalse(mNetworkParamsCaptor.getValue().isHidden);
        assertEquals(IHostapd.EncryptionType.WPA2, mNetworkParamsCaptor.getValue().encryptionType);
        assertEquals(NETWORK_PSK, mNetworkParamsCaptor.getValue().pskPassphrase);
    }

    /**
     * Verifies the successful addition of access point.
     */
    @Test
    public void testAddAccessPointSuccess_Psk_Band2G_WithIeee80211AC() throws Exception {
        // Enable ACS & 80211AC in the config.
        mResources.setBoolean(R.bool.config_wifi_softap_acs_supported, true);
        mResources.setBoolean(R.bool.config_wifi_softap_ieee80211ac_supported, true);
        mHostapdHal = new HostapdHalSpy();

        executeAndValidateInitializationSequence();
        final int apChannel = 6;

        WifiConfiguration configuration = new WifiConfiguration();
        configuration.SSID = NETWORK_SSID;
        configuration.hiddenSSID = false;
        configuration.allowedKeyManagement.set(WifiConfiguration.KeyMgmt.WPA2_PSK);
        configuration.preSharedKey = NETWORK_PSK;
        configuration.apChannel = apChannel;
        configuration.apBand = WifiConfiguration.AP_BAND_2GHZ;

        assertTrue(mHostapdHal.addAccessPoint(IFACE_NAME, configuration));
        verify(mIHostapdMock).addAccessPoint(any(), any());

        assertEquals(IFACE_NAME, mIfaceParamsCaptor.getValue().ifaceName);
        assertTrue(mIfaceParamsCaptor.getValue().hwModeParams.enable80211N);
        assertTrue(mIfaceParamsCaptor.getValue().hwModeParams.enable80211AC);
        assertEquals(IHostapd.Band.BAND_2_4_GHZ, mIfaceParamsCaptor.getValue().channelParams.band);
        assertTrue(mIfaceParamsCaptor.getValue().channelParams.enableAcs);
        assertTrue(mIfaceParamsCaptor.getValue().channelParams.acsShouldExcludeDfs);

        assertEquals(NativeUtil.stringToByteArrayList(NETWORK_SSID),
                mNetworkParamsCaptor.getValue().ssid);
        assertFalse(mNetworkParamsCaptor.getValue().isHidden);
        assertEquals(IHostapd.EncryptionType.WPA2, mNetworkParamsCaptor.getValue().encryptionType);
        assertEquals(NETWORK_PSK, mNetworkParamsCaptor.getValue().pskPassphrase);
    }

    /**
     * Verifies the successful addition of access point.
     */
    @Test
    public void testAddAccessPointSuccess_Psk_BandAny_WithACS() throws Exception {
        // Enable ACS in the config.
        mResources.setBoolean(R.bool.config_wifi_softap_acs_supported, true);
        mHostapdHal = new HostapdHalSpy();

        executeAndValidateInitializationSequence();
        final int apChannel = 6;

        WifiConfiguration configuration = new WifiConfiguration();
        configuration.SSID = NETWORK_SSID;
        configuration.hiddenSSID = false;
        configuration.allowedKeyManagement.set(WifiConfiguration.KeyMgmt.WPA2_PSK);
        configuration.preSharedKey = NETWORK_PSK;
        configuration.apChannel = apChannel;
        configuration.apBand = WifiConfiguration.AP_BAND_ANY;

        assertTrue(mHostapdHal.addAccessPoint(IFACE_NAME, configuration));
        verify(mIHostapdMock).addAccessPoint(any(), any());

        assertEquals(IFACE_NAME, mIfaceParamsCaptor.getValue().ifaceName);
        assertTrue(mIfaceParamsCaptor.getValue().hwModeParams.enable80211N);
        assertFalse(mIfaceParamsCaptor.getValue().hwModeParams.enable80211AC);
        assertEquals(IHostapd.Band.BAND_ANY, mIfaceParamsCaptor.getValue().channelParams.band);
        assertTrue(mIfaceParamsCaptor.getValue().channelParams.enableAcs);
        assertTrue(mIfaceParamsCaptor.getValue().channelParams.acsShouldExcludeDfs);

        assertEquals(NativeUtil.stringToByteArrayList(NETWORK_SSID),
                mNetworkParamsCaptor.getValue().ssid);
        assertFalse(mNetworkParamsCaptor.getValue().isHidden);
        assertEquals(IHostapd.EncryptionType.WPA2, mNetworkParamsCaptor.getValue().encryptionType);
        assertEquals(NETWORK_PSK, mNetworkParamsCaptor.getValue().pskPassphrase);
    }

    /**
     * Verifies the successful addition of access point.
     * Verifies that BAND_ANY is downgraded to 2.4GHz if ACS is disabled.
     */
    @Test
    public void testAddAccessPointSuccess_Psk_BandAny_Downgraded_WithoutACS() throws Exception {
        // Disable ACS in the config.
        mResources.setBoolean(R.bool.config_wifi_softap_acs_supported, false);
        mHostapdHal = new HostapdHalSpy();

        executeAndValidateInitializationSequence();
        final int apChannel = 6;

        WifiConfiguration configuration = new WifiConfiguration();
        configuration.SSID = NETWORK_SSID;
        configuration.hiddenSSID = false;
        configuration.allowedKeyManagement.set(WifiConfiguration.KeyMgmt.WPA2_PSK);
        configuration.preSharedKey = NETWORK_PSK;
        configuration.apChannel = apChannel;
        configuration.apBand = WifiConfiguration.AP_BAND_ANY;

        assertTrue(mHostapdHal.addAccessPoint(IFACE_NAME, configuration));
        verify(mIHostapdMock).addAccessPoint(any(), any());

        assertEquals(IFACE_NAME, mIfaceParamsCaptor.getValue().ifaceName);
        assertTrue(mIfaceParamsCaptor.getValue().hwModeParams.enable80211N);
        assertFalse(mIfaceParamsCaptor.getValue().hwModeParams.enable80211AC);
        // Verify the band is downgraded to 2.4GHz.
        assertEquals(IHostapd.Band.BAND_2_4_GHZ,
                mIfaceParamsCaptor.getValue().channelParams.band);
        assertFalse(mIfaceParamsCaptor.getValue().channelParams.enableAcs);

        assertEquals(NativeUtil.stringToByteArrayList(NETWORK_SSID),
                mNetworkParamsCaptor.getValue().ssid);
        assertFalse(mNetworkParamsCaptor.getValue().isHidden);
        assertEquals(IHostapd.EncryptionType.WPA2, mNetworkParamsCaptor.getValue().encryptionType);
        assertEquals(NETWORK_PSK, mNetworkParamsCaptor.getValue().pskPassphrase);
    }

    /**
     * Verifies the failure handling in addition of access point with an invalid band.
     */
    @Test
    public void testAddAccessPointInvalidBandFailure() throws Exception {
        executeAndValidateInitializationSequence();

        WifiConfiguration configuration = new WifiConfiguration();
        configuration.SSID = NETWORK_SSID;
        configuration.hiddenSSID = true;
        configuration.allowedKeyManagement.set(WifiConfiguration.KeyMgmt.NONE);
        configuration.apBand = WifiConfiguration.AP_BAND_5GHZ + 1;

        assertFalse(mHostapdHal.addAccessPoint(IFACE_NAME, configuration));
        verify(mIHostapdMock, never()).addAccessPoint(any(), any());
    }

    /**
     * Verifies the failure handling in addition of access point.
     */
    @Test
    public void testAddAccessPointFailure() throws Exception {
        executeAndValidateInitializationSequence();
        when(mIHostapdMock.addAccessPoint(any(), any())).thenReturn(mStatusFailure);

        WifiConfiguration configuration = new WifiConfiguration();
        configuration.SSID = NETWORK_SSID;
        configuration.hiddenSSID = true;
        configuration.allowedKeyManagement.set(WifiConfiguration.KeyMgmt.NONE);
        configuration.apChannel = 6;
        configuration.apBand = WifiConfiguration.AP_BAND_2GHZ;

        assertFalse(mHostapdHal.addAccessPoint(IFACE_NAME, configuration));
        verify(mIHostapdMock).addAccessPoint(any(), any());
    }

    /**
     * Verifies the failure handling in addition of access point.
     */
    @Test
    public void testAddAccessPointRemoteException() throws Exception {
        executeAndValidateInitializationSequence();
        doThrow(new RemoteException()).when(mIHostapdMock).addAccessPoint(any(), any());

        WifiConfiguration configuration = new WifiConfiguration();
        configuration.SSID = NETWORK_SSID;
        configuration.hiddenSSID = true;
        configuration.allowedKeyManagement.set(WifiConfiguration.KeyMgmt.NONE);
        configuration.apChannel = 6;

        assertFalse(mHostapdHal.addAccessPoint(IFACE_NAME, configuration));
        verify(mIHostapdMock).addAccessPoint(any(), any());
    }

    /**
     * Verifies the successful removal of access point.
     */
    @Test
    public void testRemoveAccessPointSuccess() throws Exception {
        executeAndValidateInitializationSequence();

        assertTrue(mHostapdHal.removeAccessPoint(IFACE_NAME));
        verify(mIHostapdMock).removeAccessPoint(any());
    }

    /**
     * Verifies the failure handling in removal of access point.
     */
    @Test
    public void testRemoveAccessPointFailure() throws Exception {
        executeAndValidateInitializationSequence();
        when(mIHostapdMock.removeAccessPoint(any())).thenReturn(mStatusFailure);

        assertFalse(mHostapdHal.removeAccessPoint(IFACE_NAME));
        verify(mIHostapdMock).removeAccessPoint(any());
    }

    /**
     * Verifies the failure handling in addition of access point.
     */
    @Test
    public void testRemoveAccessPointRemoteException() throws Exception {
        executeAndValidateInitializationSequence();
        doThrow(new RemoteException()).when(mIHostapdMock).removeAccessPoint(any());

        assertFalse(mHostapdHal.removeAccessPoint(IFACE_NAME));
        verify(mIHostapdMock).removeAccessPoint(any());
    }

    private void executeAndValidateInitializationSequence() throws Exception {
        executeAndValidateInitializationSequence(false, false);
    }

    /**
     * Calls.initialize(), mocking various callback answers and verifying flow, asserting for the
     * expected result. Verifies if IHostapd manager is initialized or reset.
     */
    private void executeAndValidateInitializationSequence(
            boolean causeRegisterRemoteException, boolean causeRegisterFailure) throws Exception {
        boolean shouldSucceed = !causeRegisterRemoteException && !causeRegisterFailure;
        mInOrder = inOrder(mServiceManagerMock, mIHostapdMock);
        if (causeRegisterFailure) {
            when(mServiceManagerMock.registerForNotifications(anyString(), anyString(),
                    any(IServiceNotification.Stub.class))).thenReturn(false);
        } else if (causeRegisterRemoteException) {
            doThrow(new RemoteException()).when(mServiceManagerMock)
                    .registerForNotifications(
                            anyString(), anyString(), any(IServiceNotification.Stub.class));
        }
        // Initialize HostapdHal, should call serviceManager.registerForNotifications
        assertEquals(shouldSucceed, mHostapdHal.initialize());
        // verify: service manager initialization sequence
        mInOrder.verify(mServiceManagerMock).linkToDeath(mServiceManagerDeathCaptor.capture(),
                anyLong());
        mInOrder.verify(mServiceManagerMock).registerForNotifications(
                eq(IHostapd.kInterfaceName), eq(""), mServiceNotificationCaptor.capture());
        if (shouldSucceed) {
            // act: cause the onRegistration(...) callback to execute
            mServiceNotificationCaptor.getValue().onRegistration(IHostapd.kInterfaceName, "", true);
            assertTrue(mHostapdHal.isInitializationComplete());
            mInOrder.verify(mIHostapdMock).linkToDeath(mHostapdDeathCaptor.capture(), anyLong());
        } else {
            assertFalse(mHostapdHal.isInitializationComplete());
            mInOrder.verify(mIHostapdMock, never()).linkToDeath(
                    mHostapdDeathCaptor.capture(), anyLong());
        }
    }

    private HostapdStatus createHostapdStatus(int code) {
        HostapdStatus status = new HostapdStatus();
        status.code = code;
        return status;
    }
}

