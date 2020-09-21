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

package com.android.server.wifi.aware;

import static org.hamcrest.core.IsEqual.equalTo;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyShort;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.hardware.wifi.V1_0.IWifiNanIface;
import android.hardware.wifi.V1_0.NanBandIndex;
import android.hardware.wifi.V1_0.NanConfigRequest;
import android.hardware.wifi.V1_0.NanEnableRequest;
import android.hardware.wifi.V1_0.NanPublishRequest;
import android.hardware.wifi.V1_0.NanRangingIndication;
import android.hardware.wifi.V1_0.NanSubscribeRequest;
import android.hardware.wifi.V1_0.WifiStatus;
import android.hardware.wifi.V1_0.WifiStatusCode;
import android.hardware.wifi.V1_2.NanConfigRequestSupplemental;
import android.net.wifi.aware.ConfigRequest;
import android.net.wifi.aware.PublishConfig;
import android.net.wifi.aware.SubscribeConfig;
import android.os.RemoteException;
import android.util.Pair;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.ErrorCollector;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.io.PrintWriter;

/**
 * Unit test harness for WifiAwareNativeApi
 */
public class WifiAwareNativeApiTest {
    @Mock WifiAwareNativeManager mWifiAwareNativeManagerMock;
    @Mock IWifiNanIface mIWifiNanIfaceMock;
    @Mock android.hardware.wifi.V1_2.IWifiNanIface mIWifiNanIface12Mock;

    @Rule public ErrorCollector collector = new ErrorCollector();

    private class MockableWifiAwareNativeApi extends WifiAwareNativeApi {
        MockableWifiAwareNativeApi(WifiAwareNativeManager wifiAwareNativeManager) {
            super(wifiAwareNativeManager);
        }

        @Override
        public android.hardware.wifi.V1_2.IWifiNanIface mockableCastTo_1_2(IWifiNanIface iface) {
            return mIsInterface12 ? mIWifiNanIface12Mock : null;
        }
    }

    private MockableWifiAwareNativeApi mDut;
    private boolean mIsInterface12;

    /**
     * Initializes mocks.
     */
    @Before
    public void setup() throws Exception {
        MockitoAnnotations.initMocks(this);

        when(mWifiAwareNativeManagerMock.getWifiNanIface()).thenReturn(mIWifiNanIfaceMock);

        WifiStatus status = new WifiStatus();
        status.code = WifiStatusCode.SUCCESS;
        when(mIWifiNanIfaceMock.enableRequest(anyShort(), any())).thenReturn(status);
        when(mIWifiNanIfaceMock.configRequest(anyShort(), any())).thenReturn(status);
        when(mIWifiNanIfaceMock.startPublishRequest(anyShort(), any())).thenReturn(status);
        when(mIWifiNanIfaceMock.startSubscribeRequest(anyShort(), any())).thenReturn(status);
        when(mIWifiNanIface12Mock.enableRequest_1_2(anyShort(), any(), any())).thenReturn(status);
        when(mIWifiNanIface12Mock.configRequest_1_2(anyShort(), any(), any())).thenReturn(status);
        when(mIWifiNanIface12Mock.startPublishRequest(anyShort(), any())).thenReturn(status);
        when(mIWifiNanIface12Mock.startSubscribeRequest(anyShort(), any())).thenReturn(status);

        mIsInterface12 = false;

        mDut = new MockableWifiAwareNativeApi(mWifiAwareNativeManagerMock);
    }

    /**
     * Test that the set parameter shell command executor works when parameters are valid.
     */
    @Test
    public void testSetParameterShellCommandSuccess() {
        setSettableParam(WifiAwareNativeApi.PARAM_MAC_RANDOM_INTERVAL_SEC, Integer.toString(1),
                true);
    }

    /**
     * Test that the set parameter shell command executor fails on incorrect name.
     */
    @Test
    public void testSetParameterShellCommandInvalidParameterName() {
        setSettableParam("XXX", Integer.toString(1), false);
    }

    /**
     * Test that the set parameter shell command executor fails on invalid value (not convertible
     * to an int).
     */
    @Test
    public void testSetParameterShellCommandInvalidValue() {
        setSettableParam(WifiAwareNativeApi.PARAM_MAC_RANDOM_INTERVAL_SEC, "garbage", false);
    }

    /**
     * Validate that the configuration parameters used to manage power state behavior is
     * using default values at the default power state.
     */
    @Test
    public void testEnableAndConfigPowerSettingsDefaults() throws RemoteException {
        Pair<NanConfigRequest, NanConfigRequestSupplemental> configs =
                validateEnableAndConfigure((short) 10, new ConfigRequest.Builder().build(), true,
                        true, true, false, false);

        collector.checkThat("validDiscoveryWindowIntervalVal-5", false,
                equalTo(configs.first.bandSpecificConfig[NanBandIndex.NAN_BAND_5GHZ]
                        .validDiscoveryWindowIntervalVal));
        collector.checkThat("validDiscoveryWindowIntervalVal-24", false,
                equalTo(configs.first.bandSpecificConfig[NanBandIndex.NAN_BAND_24GHZ]
                        .validDiscoveryWindowIntervalVal));
    }

    /**
     * Validate that the configuration parameters used to manage power state behavior is
     * using the specified non-interactive values when in that power state.
     */
    @Test
    public void testEnableAndConfigPowerSettingsNoneInteractive() throws RemoteException {
        byte interactive5 = 2;
        byte interactive24 = 3;

        setPowerConfigurationParams(interactive5, interactive24, (byte) -1, (byte) -1);
        Pair<NanConfigRequest, NanConfigRequestSupplemental> configs =
                validateEnableAndConfigure((short) 10, new ConfigRequest.Builder().build(), false,
                        false, false, false, false);

        collector.checkThat("validDiscoveryWindowIntervalVal-5", true,
                equalTo(configs.first.bandSpecificConfig[NanBandIndex.NAN_BAND_5GHZ]
                        .validDiscoveryWindowIntervalVal));
        collector.checkThat("discoveryWindowIntervalVal-5", interactive5,
                equalTo(configs.first.bandSpecificConfig[NanBandIndex.NAN_BAND_5GHZ]
                        .discoveryWindowIntervalVal));
        collector.checkThat("validDiscoveryWindowIntervalVal-24", true,
                equalTo(configs.first.bandSpecificConfig[NanBandIndex.NAN_BAND_24GHZ]
                        .validDiscoveryWindowIntervalVal));
        collector.checkThat("discoveryWindowIntervalVal-24", interactive24,
                equalTo(configs.first.bandSpecificConfig[NanBandIndex.NAN_BAND_24GHZ]
                        .discoveryWindowIntervalVal));
    }

    /**
     * Validate that the configuration parameters used to manage power state behavior is
     * using the specified idle (doze) values when in that power state.
     */
    @Test
    public void testEnableAndConfigPowerSettingsIdle() throws RemoteException {
        byte idle5 = 2;
        byte idle24 = -1;

        setPowerConfigurationParams((byte) -1, (byte) -1, idle5, idle24);
        Pair<NanConfigRequest, NanConfigRequestSupplemental> configs =
                validateEnableAndConfigure((short) 10, new ConfigRequest.Builder().build(), false,
                        true, false, true, false);

        collector.checkThat("validDiscoveryWindowIntervalVal-5", true,
                equalTo(configs.first.bandSpecificConfig[NanBandIndex.NAN_BAND_5GHZ]
                        .validDiscoveryWindowIntervalVal));
        collector.checkThat("discoveryWindowIntervalVal-5", idle5,
                equalTo(configs.first.bandSpecificConfig[NanBandIndex.NAN_BAND_5GHZ]
                        .discoveryWindowIntervalVal));
        collector.checkThat("validDiscoveryWindowIntervalVal-24", false,
                equalTo(configs.first.bandSpecificConfig[NanBandIndex.NAN_BAND_24GHZ]
                        .validDiscoveryWindowIntervalVal));
    }

    /**
     * Validate that the configuration parameters used to manage power state behavior is
     * using default values at the default power state.
     *
     * Using HAL 1.2: additional power configurations.
     */
    @Test
    public void testEnableAndConfigPowerSettingsDefaults_1_2() throws RemoteException {
        Pair<NanConfigRequest, NanConfigRequestSupplemental> configs =
                validateEnableAndConfigure((short) 10, new ConfigRequest.Builder().build(), true,
                        true, true, false, true);

        collector.checkThat("validDiscoveryWindowIntervalVal-5", false,
                equalTo(configs.first.bandSpecificConfig[NanBandIndex.NAN_BAND_5GHZ]
                        .validDiscoveryWindowIntervalVal));
        collector.checkThat("validDiscoveryWindowIntervalVal-24", false,
                equalTo(configs.first.bandSpecificConfig[NanBandIndex.NAN_BAND_24GHZ]
                        .validDiscoveryWindowIntervalVal));

        collector.checkThat("discoveryBeaconIntervalMs", 0,
                equalTo(configs.second.discoveryBeaconIntervalMs));
        collector.checkThat("numberOfSpatialStreamsInDiscovery", 0,
                equalTo(configs.second.numberOfSpatialStreamsInDiscovery));
        collector.checkThat("enableDiscoveryWindowEarlyTermination", false,
                equalTo(configs.second.enableDiscoveryWindowEarlyTermination));
    }

    /**
     * Validate that the configuration parameters used to manage power state behavior is
     * using the specified non-interactive values when in that power state.
     *
     * Using HAL 1.2: additional power configurations.
     */
    @Test
    public void testEnableAndConfigPowerSettingsNoneInteractive_1_2() throws RemoteException {
        byte interactive5 = 2;
        byte interactive24 = 3;

        setPowerConfigurationParams(interactive5, interactive24, (byte) -1, (byte) -1);
        Pair<NanConfigRequest, NanConfigRequestSupplemental> configs =
                validateEnableAndConfigure((short) 10, new ConfigRequest.Builder().build(), false,
                        false, false, false, true);

        collector.checkThat("validDiscoveryWindowIntervalVal-5", true,
                equalTo(configs.first.bandSpecificConfig[NanBandIndex.NAN_BAND_5GHZ]
                        .validDiscoveryWindowIntervalVal));
        collector.checkThat("discoveryWindowIntervalVal-5", interactive5,
                equalTo(configs.first.bandSpecificConfig[NanBandIndex.NAN_BAND_5GHZ]
                        .discoveryWindowIntervalVal));
        collector.checkThat("validDiscoveryWindowIntervalVal-24", true,
                equalTo(configs.first.bandSpecificConfig[NanBandIndex.NAN_BAND_24GHZ]
                        .validDiscoveryWindowIntervalVal));
        collector.checkThat("discoveryWindowIntervalVal-24", interactive24,
                equalTo(configs.first.bandSpecificConfig[NanBandIndex.NAN_BAND_24GHZ]
                        .discoveryWindowIntervalVal));

        // Note: still defaults (i.e. disabled) - will be tweaked for low power
        collector.checkThat("discoveryBeaconIntervalMs", 0,
                equalTo(configs.second.discoveryBeaconIntervalMs));
        collector.checkThat("numberOfSpatialStreamsInDiscovery", 0,
                equalTo(configs.second.numberOfSpatialStreamsInDiscovery));
        collector.checkThat("enableDiscoveryWindowEarlyTermination", false,
                equalTo(configs.second.enableDiscoveryWindowEarlyTermination));
    }

    /**
     * Validate that the configuration parameters used to manage power state behavior is
     * using the specified idle (doze) values when in that power state.
     *
     * Using HAL 1.2: additional power configurations.
     */
    @Test
    public void testEnableAndConfigPowerSettingsIdle_1_2() throws RemoteException {
        byte idle5 = 2;
        byte idle24 = -1;

        setPowerConfigurationParams((byte) -1, (byte) -1, idle5, idle24);
        Pair<NanConfigRequest, NanConfigRequestSupplemental> configs =
                validateEnableAndConfigure((short) 10, new ConfigRequest.Builder().build(), false,
                        true, false, true, true);

        collector.checkThat("validDiscoveryWindowIntervalVal-5", true,
                equalTo(configs.first.bandSpecificConfig[NanBandIndex.NAN_BAND_5GHZ]
                        .validDiscoveryWindowIntervalVal));
        collector.checkThat("discoveryWindowIntervalVal-5", idle5,
                equalTo(configs.first.bandSpecificConfig[NanBandIndex.NAN_BAND_5GHZ]
                        .discoveryWindowIntervalVal));
        collector.checkThat("validDiscoveryWindowIntervalVal-24", false,
                equalTo(configs.first.bandSpecificConfig[NanBandIndex.NAN_BAND_24GHZ]
                        .validDiscoveryWindowIntervalVal));

        // Note: still defaults (i.e. disabled) - will be tweaked for low power
        collector.checkThat("discoveryBeaconIntervalMs", 0,
                equalTo(configs.second.discoveryBeaconIntervalMs));
        collector.checkThat("numberOfSpatialStreamsInDiscovery", 0,
                equalTo(configs.second.numberOfSpatialStreamsInDiscovery));
        collector.checkThat("enableDiscoveryWindowEarlyTermination", false,
                equalTo(configs.second.enableDiscoveryWindowEarlyTermination));
    }

    @Test
    public void testDiscoveryRangingSettings() throws RemoteException {
        short tid = 666;
        byte pid = 34;
        int minDistanceMm = 100;
        int maxDistanceMm = 555;
        // TODO: b/69428593 remove once HAL is converted from CM to MM
        short minDistanceCm = (short) (minDistanceMm / 10);
        short maxDistanceCm = (short) (maxDistanceMm / 10);

        ArgumentCaptor<NanPublishRequest> pubCaptor = ArgumentCaptor.forClass(
                NanPublishRequest.class);
        ArgumentCaptor<NanSubscribeRequest> subCaptor = ArgumentCaptor.forClass(
                NanSubscribeRequest.class);

        PublishConfig pubDefault = new PublishConfig.Builder().setServiceName("XXX").build();
        PublishConfig pubWithRanging = new PublishConfig.Builder().setServiceName(
                "XXX").setRangingEnabled(true).build();
        SubscribeConfig subDefault = new SubscribeConfig.Builder().setServiceName("XXX").build();
        SubscribeConfig subWithMin = new SubscribeConfig.Builder().setServiceName(
                "XXX").setMinDistanceMm(minDistanceMm).build();
        SubscribeConfig subWithMax = new SubscribeConfig.Builder().setServiceName(
                "XXX").setMaxDistanceMm(maxDistanceMm).build();
        SubscribeConfig subWithMinMax = new SubscribeConfig.Builder().setServiceName(
                "XXX").setMinDistanceMm(minDistanceMm).setMaxDistanceMm(maxDistanceMm).build();

        mDut.publish(tid, pid, pubDefault);
        mDut.publish(tid, pid, pubWithRanging);
        mDut.subscribe(tid, pid, subDefault);
        mDut.subscribe(tid, pid, subWithMin);
        mDut.subscribe(tid, pid, subWithMax);
        mDut.subscribe(tid, pid, subWithMinMax);

        verify(mIWifiNanIfaceMock, times(2)).startPublishRequest(eq(tid), pubCaptor.capture());
        verify(mIWifiNanIfaceMock, times(4)).startSubscribeRequest(eq(tid), subCaptor.capture());

        NanPublishRequest halPubReq;
        NanSubscribeRequest halSubReq;

        // pubDefault
        halPubReq = pubCaptor.getAllValues().get(0);
        collector.checkThat("pubDefault.baseConfigs.sessionId", pid,
                equalTo(halPubReq.baseConfigs.sessionId));
        collector.checkThat("pubDefault.baseConfigs.rangingRequired", false,
                equalTo(halPubReq.baseConfigs.rangingRequired));

        // pubWithRanging
        halPubReq = pubCaptor.getAllValues().get(1);
        collector.checkThat("pubWithRanging.baseConfigs.sessionId", pid,
                equalTo(halPubReq.baseConfigs.sessionId));
        collector.checkThat("pubWithRanging.baseConfigs.rangingRequired", true,
                equalTo(halPubReq.baseConfigs.rangingRequired));

        // subDefault
        halSubReq = subCaptor.getAllValues().get(0);
        collector.checkThat("subDefault.baseConfigs.sessionId", pid,
                equalTo(halSubReq.baseConfigs.sessionId));
        collector.checkThat("subDefault.baseConfigs.rangingRequired", false,
                equalTo(halSubReq.baseConfigs.rangingRequired));

        // subWithMin
        halSubReq = subCaptor.getAllValues().get(1);
        collector.checkThat("subWithMin.baseConfigs.sessionId", pid,
                equalTo(halSubReq.baseConfigs.sessionId));
        collector.checkThat("subWithMin.baseConfigs.rangingRequired", true,
                equalTo(halSubReq.baseConfigs.rangingRequired));
        collector.checkThat("subWithMin.baseConfigs.configRangingIndications",
                NanRangingIndication.EGRESS_MET_MASK,
                equalTo(halSubReq.baseConfigs.configRangingIndications));
        collector.checkThat("subWithMin.baseConfigs.distanceEgressCm", minDistanceCm,
                equalTo(halSubReq.baseConfigs.distanceEgressCm));

        // subWithMax
        halSubReq = subCaptor.getAllValues().get(2);
        collector.checkThat("subWithMax.baseConfigs.sessionId", pid,
                equalTo(halSubReq.baseConfigs.sessionId));
        collector.checkThat("subWithMax.baseConfigs.rangingRequired", true,
                equalTo(halSubReq.baseConfigs.rangingRequired));
        collector.checkThat("subWithMax.baseConfigs.configRangingIndications",
                NanRangingIndication.INGRESS_MET_MASK,
                equalTo(halSubReq.baseConfigs.configRangingIndications));
        collector.checkThat("subWithMin.baseConfigs.distanceIngressCm", maxDistanceCm,
                equalTo(halSubReq.baseConfigs.distanceIngressCm));

        // subWithMinMax
        halSubReq = subCaptor.getAllValues().get(3);
        collector.checkThat("subWithMinMax.baseConfigs.sessionId", pid,
                equalTo(halSubReq.baseConfigs.sessionId));
        collector.checkThat("subWithMinMax.baseConfigs.rangingRequired", true,
                equalTo(halSubReq.baseConfigs.rangingRequired));
        collector.checkThat("subWithMinMax.baseConfigs.configRangingIndications",
                NanRangingIndication.INGRESS_MET_MASK | NanRangingIndication.EGRESS_MET_MASK,
                equalTo(halSubReq.baseConfigs.configRangingIndications));
        collector.checkThat("subWithMin.baseConfigs.distanceEgressCm", minDistanceCm,
                equalTo(halSubReq.baseConfigs.distanceEgressCm));
        collector.checkThat("subWithMin.baseConfigs.distanceIngressCm", maxDistanceCm,
                equalTo(halSubReq.baseConfigs.distanceIngressCm));
    }

    // utilities

    private void setPowerConfigurationParams(byte interactive5, byte interactive24, byte idle5,
            byte idle24) {
        setSettablePowerParam(WifiAwareNativeApi.POWER_PARAM_INACTIVE_KEY,
                WifiAwareNativeApi.PARAM_DW_5GHZ, Integer.toString(interactive5), true);
        setSettablePowerParam(WifiAwareNativeApi.POWER_PARAM_INACTIVE_KEY,
                WifiAwareNativeApi.PARAM_DW_24GHZ, Integer.toString(interactive24), true);
        setSettablePowerParam(WifiAwareNativeApi.POWER_PARAM_IDLE_KEY,
                WifiAwareNativeApi.PARAM_DW_5GHZ, Integer.toString(idle5), true);
        setSettablePowerParam(WifiAwareNativeApi.POWER_PARAM_IDLE_KEY,
                WifiAwareNativeApi.PARAM_DW_24GHZ, Integer.toString(idle24), true);
    }

    private void setSettablePowerParam(String mode, String name, String value,
            boolean expectSuccess) {
        PrintWriter pwMock = mock(PrintWriter.class);
        WifiAwareShellCommand parentShellMock = mock(WifiAwareShellCommand.class);
        when(parentShellMock.getNextArgRequired()).thenReturn("set-power").thenReturn(
                mode).thenReturn(name).thenReturn(value);
        when(parentShellMock.getErrPrintWriter()).thenReturn(pwMock);

        collector.checkThat(mDut.onCommand(parentShellMock), equalTo(expectSuccess ? 0 : -1));
    }

    private void setSettableParam(String name, String value, boolean expectSuccess) {
        PrintWriter pwMock = mock(PrintWriter.class);
        WifiAwareShellCommand parentShellMock = mock(WifiAwareShellCommand.class);
        when(parentShellMock.getNextArgRequired()).thenReturn("set").thenReturn(name).thenReturn(
                value);
        when(parentShellMock.getErrPrintWriter()).thenReturn(pwMock);

        collector.checkThat(mDut.onCommand(parentShellMock), equalTo(expectSuccess ? 0 : -1));
    }

    private Pair<NanConfigRequest, NanConfigRequestSupplemental> validateEnableAndConfigure(
            short transactionId, ConfigRequest configRequest, boolean notifyIdentityChange,
            boolean initialConfiguration, boolean isInteractive, boolean isIdle, boolean isHal12)
            throws RemoteException {
        mIsInterface12 = isHal12;

        mDut.enableAndConfigure(transactionId, configRequest, notifyIdentityChange,
                initialConfiguration, isInteractive, isIdle);

        ArgumentCaptor<NanEnableRequest> enableReqCaptor = ArgumentCaptor.forClass(
                NanEnableRequest.class);
        ArgumentCaptor<NanConfigRequest> configReqCaptor = ArgumentCaptor.forClass(
                NanConfigRequest.class);
        ArgumentCaptor<NanConfigRequestSupplemental> configSuppCaptor = ArgumentCaptor.forClass(
                NanConfigRequestSupplemental.class);
        NanConfigRequest config;
        NanConfigRequestSupplemental configSupp = null;

        if (initialConfiguration) {
            if (isHal12) {
                verify(mIWifiNanIface12Mock).enableRequest_1_2(eq(transactionId),
                        enableReqCaptor.capture(), configSuppCaptor.capture());
                configSupp = configSuppCaptor.getValue();
            } else {
                verify(mIWifiNanIfaceMock).enableRequest(eq(transactionId),
                        enableReqCaptor.capture());
            }
            config = enableReqCaptor.getValue().configParams;
        } else {
            if (isHal12) {
                verify(mIWifiNanIface12Mock).configRequest_1_2(eq(transactionId),
                        configReqCaptor.capture(), configSuppCaptor.capture());
                configSupp = configSuppCaptor.getValue();
            } else {
                verify(mIWifiNanIfaceMock).configRequest(eq(transactionId),
                        configReqCaptor.capture());
            }
            config = configReqCaptor.getValue();
        }

        collector.checkThat("disableDiscoveryAddressChangeIndication", !notifyIdentityChange,
                equalTo(config.disableDiscoveryAddressChangeIndication));
        collector.checkThat("disableStartedClusterIndication", !notifyIdentityChange,
                equalTo(config.disableStartedClusterIndication));
        collector.checkThat("disableJoinedClusterIndication", !notifyIdentityChange,
                equalTo(config.disableJoinedClusterIndication));

        return new Pair<>(config, configSupp);
    }
}
