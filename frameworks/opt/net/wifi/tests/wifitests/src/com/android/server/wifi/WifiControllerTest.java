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
 * limitations under the License.
 */

package com.android.server.wifi;

import static com.android.server.wifi.WifiController.CMD_AP_STOPPED;
import static com.android.server.wifi.WifiController.CMD_EMERGENCY_CALL_STATE_CHANGED;
import static com.android.server.wifi.WifiController.CMD_EMERGENCY_MODE_CHANGED;
import static com.android.server.wifi.WifiController.CMD_RECOVERY_DISABLE_WIFI;
import static com.android.server.wifi.WifiController.CMD_RECOVERY_RESTART_WIFI;
import static com.android.server.wifi.WifiController.CMD_SCAN_ALWAYS_MODE_CHANGED;
import static com.android.server.wifi.WifiController.CMD_SET_AP;
import static com.android.server.wifi.WifiController.CMD_WIFI_TOGGLED;

import static org.junit.Assert.assertEquals;
import static org.mockito.Mockito.*;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.location.LocationManager;
import android.net.wifi.WifiManager;
import android.os.Handler;
import android.os.test.TestLooper;
import android.provider.Settings;
import android.support.test.filters.SmallTest;
import android.util.Log;

import com.android.internal.util.IState;
import com.android.internal.util.StateMachine;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.mockito.ArgumentCaptor;
import org.mockito.InOrder;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.io.ByteArrayOutputStream;
import java.io.PrintWriter;
import java.lang.reflect.Method;

/**
 * Test WifiController for changes in and out of ECM and SoftAP modes.
 */
@SmallTest
public class WifiControllerTest {

    private static final String TAG = "WifiControllerTest";

    private void dumpState() {
        ByteArrayOutputStream stream = new ByteArrayOutputStream();
        PrintWriter writer = new PrintWriter(stream);
        mWifiController.dump(null, writer, null);
        writer.flush();
        Log.d(TAG, "WifiStateMachine state -" + stream.toString());
    }

    private IState getCurrentState() throws Exception {
        Method method = StateMachine.class.getDeclaredMethod("getCurrentState");
        method.setAccessible(true);
        return (IState) method.invoke(mWifiController);
    }

    private void initializeSettingsStore() throws Exception {
        when(mSettingsStore.isAirplaneModeOn()).thenReturn(false);
        when(mSettingsStore.isWifiToggleEnabled()).thenReturn(false);
        when(mSettingsStore.isScanAlwaysAvailable()).thenReturn(false);
        when(mSettingsStore.getLocationModeSetting(eq(mContext)))
                .thenReturn(Settings.Secure.LOCATION_MODE_HIGH_ACCURACY);
    }

    TestLooper mLooper;
    @Mock Context mContext;
    @Mock FrameworkFacade mFacade;
    @Mock WifiSettingsStore mSettingsStore;
    @Mock WifiStateMachine mWifiStateMachine;
    @Mock WifiStateMachinePrime mWifiStateMachinePrime;

    WifiController mWifiController;
    Handler mWifiStateMachineHandler;

    private BroadcastReceiver mBroadcastReceiver;

    private ClientModeManager.Listener mClientModeCallback;
    private ScanOnlyModeManager.Listener mScanOnlyModeCallback;

    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);

        mLooper = new TestLooper();

        initializeSettingsStore();

        mWifiController = new WifiController(mContext, mWifiStateMachine, mLooper.getLooper(),
                mSettingsStore, mLooper.getLooper(), mFacade, mWifiStateMachinePrime);
        mWifiController.start();
        mLooper.dispatchAll();
        ArgumentCaptor<BroadcastReceiver> bcastRxCaptor = ArgumentCaptor.forClass(
                BroadcastReceiver.class);
        verify(mContext).registerReceiver(bcastRxCaptor.capture(), any(IntentFilter.class));

        mBroadcastReceiver = bcastRxCaptor.getValue();

        ArgumentCaptor<ClientModeManager.Listener> clientModeCallbackCaptor =
                ArgumentCaptor.forClass(ClientModeManager.Listener.class);
        verify(mWifiStateMachinePrime)
                .registerClientModeCallback(clientModeCallbackCaptor.capture());
        mClientModeCallback = clientModeCallbackCaptor.getValue();

        ArgumentCaptor<ScanOnlyModeManager.Listener> scanOnlyModeCallbackCaptor =
                ArgumentCaptor.forClass(ScanOnlyModeManager.Listener.class);
        verify(mWifiStateMachinePrime)
                .registerScanOnlyCallback(scanOnlyModeCallbackCaptor.capture());
        mScanOnlyModeCallback = scanOnlyModeCallbackCaptor.getValue();

    }

    @After
    public void cleanUp() {
        mLooper.dispatchAll();
    }

    /**
     * Verify that toggling wifi from disabled starts client mode.
     */
    @Test
    public void enableWifi() throws Exception {
        assertEquals("StaDisabledState", getCurrentState().getName());

        when(mSettingsStore.isWifiToggleEnabled()).thenReturn(true);
        mWifiController.sendMessage(CMD_WIFI_TOGGLED);
        mLooper.dispatchAll();
        assertEquals("DeviceActiveState", getCurrentState().getName());
    }

    /**
     * Test verifying that we can enter scan mode when the scan mode changes
     */
    @Test
    public void enableScanMode() throws Exception {
        when(mSettingsStore.isScanAlwaysAvailable()).thenReturn(true);
        mWifiController.sendMessage(CMD_SCAN_ALWAYS_MODE_CHANGED);
        mLooper.dispatchAll();
        verify(mWifiStateMachinePrime).enterScanOnlyMode();
    }

    /**
     * Verify that if scanning is enabled at startup, we enter scan mode
     */
    @Test
    public void testEnterScanModeAtStartWhenSet() throws Exception {
        when(mSettingsStore.isScanAlwaysAvailable()).thenReturn(true);

        // reset to avoid the default behavior
        reset(mWifiStateMachinePrime);

        WifiController wifiController =
                new WifiController(mContext, mWifiStateMachine, mLooper.getLooper(),
                                   mSettingsStore, mLooper.getLooper(), mFacade,
                                   mWifiStateMachinePrime);

        wifiController.start();
        mLooper.dispatchAll();

        verify(mWifiStateMachinePrime, never()).disableWifi();
        verify(mWifiStateMachinePrime).enterScanOnlyMode();
    }

    /**
     * Do not enter scan mode if location mode disabled.
     */
    @Test
    public void testDoesNotEnterScanModeWhenLocationModeDisabled() throws Exception {
        // Start a new WifiController with wifi disabled
        when(mSettingsStore.isAirplaneModeOn()).thenReturn(false);
        when(mSettingsStore.isWifiToggleEnabled()).thenReturn(false);
        when(mSettingsStore.isScanAlwaysAvailable()).thenReturn(false);
        when(mSettingsStore.getLocationModeSetting(eq(mContext)))
                .thenReturn(Settings.Secure.LOCATION_MODE_OFF);

        mWifiController = new WifiController(mContext, mWifiStateMachine, mLooper.getLooper(),
                mSettingsStore, mLooper.getLooper(), mFacade, mWifiStateMachinePrime);

        reset(mWifiStateMachinePrime);
        mWifiController.start();
        mLooper.dispatchAll();

        verify(mWifiStateMachinePrime).disableWifi();

        // toggling scan always available is not sufficient for scan mode
        when(mSettingsStore.isScanAlwaysAvailable()).thenReturn(true);
        mWifiController.sendMessage(CMD_SCAN_ALWAYS_MODE_CHANGED);
        mLooper.dispatchAll();

        verify(mWifiStateMachinePrime, never()).enterScanOnlyMode();

    }

    /**
     * Only enter scan mode if location mode enabled
     */
    @Test
    public void testEnterScanModeWhenLocationModeEnabled() throws Exception {
        when(mSettingsStore.isScanAlwaysAvailable()).thenReturn(true);
        when(mSettingsStore.getLocationModeSetting(eq(mContext)))
                .thenReturn(Settings.Secure.LOCATION_MODE_OFF);

        reset(mContext, mWifiStateMachinePrime);
        mWifiController = new WifiController(mContext, mWifiStateMachine, mLooper.getLooper(),
                mSettingsStore, mLooper.getLooper(), mFacade, mWifiStateMachinePrime);
        ArgumentCaptor<BroadcastReceiver> bcastRxCaptor = ArgumentCaptor.forClass(
                BroadcastReceiver.class);
        verify(mContext).registerReceiver(bcastRxCaptor.capture(), any(IntentFilter.class));

        mBroadcastReceiver = bcastRxCaptor.getValue();

        mWifiController.start();
        mLooper.dispatchAll();

        verify(mWifiStateMachinePrime).disableWifi();

        when(mSettingsStore.getLocationModeSetting(eq(mContext)))
                .thenReturn(Settings.Secure.LOCATION_MODE_HIGH_ACCURACY);
        Intent intent = new Intent(LocationManager.MODE_CHANGED_ACTION);

        mBroadcastReceiver.onReceive(mContext, intent);
        mLooper.dispatchAll();
        verify(mWifiStateMachinePrime).enterScanOnlyMode();
    }



    /**
     * Disabling location mode when in scan mode will disable wifi
     */
    @Test
    public void testExitScanModeWhenLocationModeDisabled() throws Exception {
        when(mSettingsStore.isScanAlwaysAvailable()).thenReturn(true);
        when(mSettingsStore.getLocationModeSetting(eq(mContext)))
                .thenReturn(Settings.Secure.LOCATION_MODE_HIGH_ACCURACY);

        reset(mContext, mWifiStateMachinePrime);
        mWifiController = new WifiController(mContext, mWifiStateMachine, mLooper.getLooper(),
                mSettingsStore, mLooper.getLooper(), mFacade, mWifiStateMachinePrime);
        ArgumentCaptor<BroadcastReceiver> bcastRxCaptor = ArgumentCaptor.forClass(
                BroadcastReceiver.class);
        verify(mContext).registerReceiver(bcastRxCaptor.capture(), any(IntentFilter.class));

        mBroadcastReceiver = bcastRxCaptor.getValue();

        mWifiController.start();
        mLooper.dispatchAll();

        verify(mWifiStateMachinePrime).enterScanOnlyMode();

        when(mSettingsStore.getLocationModeSetting(eq(mContext)))
                .thenReturn(Settings.Secure.LOCATION_MODE_OFF);
        Intent intent = new Intent(LocationManager.MODE_CHANGED_ACTION);

        mBroadcastReceiver.onReceive(mContext, intent);
        mLooper.dispatchAll();
        verify(mWifiStateMachinePrime).disableWifi();
    }

    /**
     * When in Client mode, make sure ECM triggers wifi shutdown.
     */
    @Test
    public void testEcmOnFromClientMode() throws Exception {
        when(mSettingsStore.isScanAlwaysAvailable()).thenReturn(false);
        enableWifi();

        // Test with WifiDisableInECBM turned on:
        when(mFacade.getConfigWiFiDisableInECBM(mContext)).thenReturn(true);

        // test ecm changed
        mWifiController.sendMessage(CMD_EMERGENCY_MODE_CHANGED, 1);
        mLooper.dispatchAll();

        verify(mWifiStateMachinePrime).shutdownWifi();
    }

    /**
     * ECM disabling messages, when in client mode (not expected) do not trigger state changes.
     */
    @Test
    public void testEcmOffInClientMode() throws Exception {
        when(mSettingsStore.isScanAlwaysAvailable()).thenReturn(false);
        enableWifi();

        // Test with WifiDisableInECBM turned off
        when(mFacade.getConfigWiFiDisableInECBM(mContext)).thenReturn(false);

        // test ecm changed
        mWifiController.sendMessage(CMD_EMERGENCY_MODE_CHANGED, 1);
        mLooper.dispatchAll();

        verify(mWifiStateMachinePrime, never()).shutdownWifi();
    }

    /**
     * When ECM activates and we are in client mode, disabling ECM should return us to client mode.
     */
    @Test
    public void testEcmDisabledReturnsToClientMode() throws Exception {
        enableWifi();
        verify(mWifiStateMachinePrime).enterClientMode();

        reset(mWifiStateMachinePrime);

        // Test with WifiDisableInECBM turned on:
        when(mFacade.getConfigWiFiDisableInECBM(mContext)).thenReturn(true);

        // test ecm changed
        mWifiController.sendMessage(CMD_EMERGENCY_MODE_CHANGED, 1);
        mLooper.dispatchAll();

        verify(mWifiStateMachinePrime).shutdownWifi();

        // test ecm changed
        mWifiController.sendMessage(CMD_EMERGENCY_MODE_CHANGED, 0);
        mLooper.dispatchAll();

        verify(mWifiStateMachinePrime).enterClientMode();
    }

    /**
     * When Ecm mode is enabled, we should shut down wifi when we get an emergency mode changed
     * update.
     */
    @Test
    public void testEcmOnFromScanMode() throws Exception {
        when(mSettingsStore.isScanAlwaysAvailable()).thenReturn(true);
        mWifiController.sendMessage(CMD_SCAN_ALWAYS_MODE_CHANGED);
        mLooper.dispatchAll();
        verify(mWifiStateMachinePrime).enterScanOnlyMode();

        // Test with WifiDisableInECBM turned on:
        when(mFacade.getConfigWiFiDisableInECBM(mContext)).thenReturn(true);
        verify(mWifiStateMachinePrime).enterScanOnlyMode();

        // test ecm changed
        mWifiController.sendMessage(CMD_EMERGENCY_MODE_CHANGED, 1);
        mLooper.dispatchAll();

        verify(mWifiStateMachinePrime).shutdownWifi();
    }

    /**
     * When Ecm mode is disabled, we should not shut down scan mode if we get an emergency mode
     * changed update
     */
    @Test
    public void testEcmOffInScanMode() throws Exception {
        when(mSettingsStore.isScanAlwaysAvailable()).thenReturn(true);
        mWifiController.sendMessage(CMD_SCAN_ALWAYS_MODE_CHANGED);
        mLooper.dispatchAll();
        verify(mWifiStateMachinePrime).enterScanOnlyMode();

        // Test with WifiDisableInECBM turned off:
        when(mFacade.getConfigWiFiDisableInECBM(mContext)).thenReturn(false);
        verify(mWifiStateMachinePrime).enterScanOnlyMode();

        // test ecm changed
        mWifiController.sendMessage(CMD_EMERGENCY_MODE_CHANGED, 1);
        mLooper.dispatchAll();

        verify(mWifiStateMachinePrime, never()).shutdownWifi();
    }

    /**
     * When ECM is disabled, we should return to scan mode
     */
    @Test
    public void testEcmDisabledReturnsToScanMode() throws Exception {
        when(mSettingsStore.isScanAlwaysAvailable()).thenReturn(true);
        mWifiController.sendMessage(CMD_SCAN_ALWAYS_MODE_CHANGED);
        mLooper.dispatchAll();
        verify(mWifiStateMachinePrime).enterScanOnlyMode();

        reset(mWifiStateMachinePrime);

        // Test with WifiDisableInECBM turned on:
        when(mFacade.getConfigWiFiDisableInECBM(mContext)).thenReturn(true);

        // test ecm changed
        mWifiController.sendMessage(CMD_EMERGENCY_MODE_CHANGED, 1);
        mLooper.dispatchAll();

        verify(mWifiStateMachinePrime).shutdownWifi();

        // test ecm changed
        mWifiController.sendMessage(CMD_EMERGENCY_MODE_CHANGED, 0);
        mLooper.dispatchAll();

        verify(mWifiStateMachinePrime).enterScanOnlyMode();
    }

    /**
     * When Ecm mode is enabled, we should shut down wifi when we get an emergency mode changed
     * update.
     */
    @Test
    public void testEcmOnFromSoftApMode() throws Exception {
        mWifiController.obtainMessage(CMD_SET_AP, 1, 0).sendToTarget();
        mLooper.dispatchAll();

        // Test with WifiDisableInECBM turned on:
        when(mFacade.getConfigWiFiDisableInECBM(mContext)).thenReturn(true);
        verify(mWifiStateMachinePrime).enterSoftAPMode(any());

        // test ecm changed
        mWifiController.sendMessage(CMD_EMERGENCY_MODE_CHANGED, 1);
        mLooper.dispatchAll();

        verify(mWifiStateMachinePrime).shutdownWifi();
    }

    /**
     * When Ecm mode is disabled, we should not shut down softap mode if we get an emergency mode
     * changed update
     */
    @Test
    public void testEcmOffInSoftApMode() throws Exception {
        mWifiController.obtainMessage(CMD_SET_AP, 1, 0).sendToTarget();
        mLooper.dispatchAll();
        verify(mWifiStateMachinePrime).enterSoftAPMode(any());

        // Test with WifiDisableInECBM turned off:
        when(mFacade.getConfigWiFiDisableInECBM(mContext)).thenReturn(false);

        // test ecm changed
        mWifiController.sendMessage(CMD_EMERGENCY_MODE_CHANGED, 1);
        mLooper.dispatchAll();

        verify(mWifiStateMachinePrime, never()).shutdownWifi();
    }

    /**
     * When ECM is activated and we were in softap mode, we should just return to wifi off when ECM
     * ends
     */
    @Test
    public void testEcmDisabledRemainsDisabledWhenSoftApHadBeenOn() throws Exception {
        verify(mWifiStateMachinePrime).disableWifi();

        mWifiController.obtainMessage(CMD_SET_AP, 1, 0).sendToTarget();
        mLooper.dispatchAll();
        verify(mWifiStateMachinePrime).enterSoftAPMode(any());

        // Test with WifiDisableInECBM turned on:
        when(mFacade.getConfigWiFiDisableInECBM(mContext)).thenReturn(true);

        // test ecm changed
        mWifiController.sendMessage(CMD_EMERGENCY_MODE_CHANGED, 1);
        mLooper.dispatchAll();
        verify(mWifiStateMachinePrime).shutdownWifi();

        reset(mWifiStateMachinePrime);

        // test ecm changed
        mWifiController.sendMessage(CMD_EMERGENCY_MODE_CHANGED, 0);
        mLooper.dispatchAll();

        verify(mWifiStateMachinePrime).disableWifi();
        // no additional calls to enable softap
        verify(mWifiStateMachinePrime, never()).enterSoftAPMode(any());
    }

    /**
     * Wifi should remain off when already disabled and we enter ECM.
     */
    @Test
    public void testEcmOnFromDisabledMode() throws Exception {
        verify(mWifiStateMachinePrime, never()).enterSoftAPMode(any());
        verify(mWifiStateMachinePrime, never()).enterClientMode();
        verify(mWifiStateMachinePrime, never()).enterScanOnlyMode();

        // Test with WifiDisableInECBM turned on:
        when(mFacade.getConfigWiFiDisableInECBM(mContext)).thenReturn(true);

        // test ecm changed
        mWifiController.sendMessage(CMD_EMERGENCY_MODE_CHANGED, 1);
        mLooper.dispatchAll();
        assertEquals("EcmState", getCurrentState().getName());

        verify(mWifiStateMachinePrime).shutdownWifi();
    }


    /**
     * Updates about call state change also trigger entry of ECM mode.
     */
    @Test
    public void testEnterEcmOnEmergencyCallStateChange() throws Exception {
        verify(mWifiStateMachinePrime).disableWifi();

        enableWifi();
        verify(mWifiStateMachinePrime).enterClientMode();

        reset(mWifiStateMachinePrime);

        // Test with WifiDisableInECBM turned on:
        when(mFacade.getConfigWiFiDisableInECBM(mContext)).thenReturn(true);

        // test call state changed
        mWifiController.sendMessage(CMD_EMERGENCY_CALL_STATE_CHANGED, 1);
        mLooper.dispatchAll();
        verify(mWifiStateMachinePrime).shutdownWifi();

        mWifiController.sendMessage(CMD_EMERGENCY_CALL_STATE_CHANGED, 0);
        mLooper.dispatchAll();
        verify(mWifiStateMachinePrime).enterClientMode();
    }

    /**
     * Verify when both ECM and call state changes arrive, we enter ECM mode
     */
    @Test
    public void testEnterEcmWithBothSignals() throws Exception {
        verify(mWifiStateMachinePrime).disableWifi();

        enableWifi();
        verify(mWifiStateMachinePrime).enterClientMode();

        reset(mWifiStateMachinePrime);

        // Test with WifiDisableInECBM turned on:
        when(mFacade.getConfigWiFiDisableInECBM(mContext)).thenReturn(true);

        mWifiController.sendMessage(CMD_EMERGENCY_CALL_STATE_CHANGED, 1);
        mLooper.dispatchAll();
        verify(mWifiStateMachinePrime).shutdownWifi();

        mWifiController.sendMessage(CMD_EMERGENCY_MODE_CHANGED, 1);
        mLooper.dispatchAll();
        // still only 1 shutdown
        verify(mWifiStateMachinePrime).shutdownWifi();

        mWifiController.sendMessage(CMD_EMERGENCY_CALL_STATE_CHANGED, 0);
        mLooper.dispatchAll();
        // stay in ecm, do not send an additional client mode trigger
        verify(mWifiStateMachinePrime, never()).enterClientMode();

        mWifiController.sendMessage(CMD_EMERGENCY_MODE_CHANGED, 0);
        mLooper.dispatchAll();
        // now we can re-enable wifi
        verify(mWifiStateMachinePrime).enterClientMode();
    }

    /**
     * Verify when both ECM and call state changes arrive but out of order, we enter ECM mode
     */
    @Test
    public void testEnterEcmWithBothSignalsOutOfOrder() throws Exception {
        verify(mWifiStateMachinePrime).disableWifi();

        enableWifi();
        verify(mWifiStateMachinePrime).enterClientMode();

        reset(mWifiStateMachinePrime);

        // Test with WifiDisableInECBM turned on:
        when(mFacade.getConfigWiFiDisableInECBM(mContext)).thenReturn(true);

        mWifiController.sendMessage(CMD_EMERGENCY_MODE_CHANGED, 1);
        mLooper.dispatchAll();
        verify(mWifiStateMachinePrime).shutdownWifi();

        mWifiController.sendMessage(CMD_EMERGENCY_CALL_STATE_CHANGED, 1);
        mLooper.dispatchAll();
        // still only 1 shutdown
        verify(mWifiStateMachinePrime).shutdownWifi();

        mWifiController.sendMessage(CMD_EMERGENCY_CALL_STATE_CHANGED, 0);
        mLooper.dispatchAll();
        // stay in ecm, do not send an additional client mode trigger
        verify(mWifiStateMachinePrime, never()).enterClientMode();

        mWifiController.sendMessage(CMD_EMERGENCY_MODE_CHANGED, 0);
        mLooper.dispatchAll();
        // now we can re-enable wifi
        verify(mWifiStateMachinePrime).enterClientMode();
    }

    /**
     * Verify when both ECM and call state changes arrive but completely out of order,
     * we still enter and properly exit ECM mode
     */
    @Test
    public void testEnterEcmWithBothSignalsOppositeOrder() throws Exception {
        verify(mWifiStateMachinePrime).disableWifi();

        enableWifi();
        verify(mWifiStateMachinePrime).enterClientMode();

        reset(mWifiStateMachinePrime);

        // Test with WifiDisableInECBM turned on:
        when(mFacade.getConfigWiFiDisableInECBM(mContext)).thenReturn(true);

        mWifiController.sendMessage(CMD_EMERGENCY_CALL_STATE_CHANGED, 1);
        mLooper.dispatchAll();
        verify(mWifiStateMachinePrime).shutdownWifi();

        mWifiController.sendMessage(CMD_EMERGENCY_MODE_CHANGED, 1);
        mLooper.dispatchAll();
        // still only 1 shutdown
        verify(mWifiStateMachinePrime).shutdownWifi();

        mWifiController.sendMessage(CMD_EMERGENCY_MODE_CHANGED, 0);
        mLooper.dispatchAll();
        // stay in ecm, do not send an additional client mode trigger
        verify(mWifiStateMachinePrime, never()).enterClientMode();

        mWifiController.sendMessage(CMD_EMERGENCY_CALL_STATE_CHANGED, 0);
        mLooper.dispatchAll();
        // now we can re-enable wifi
        verify(mWifiStateMachinePrime).enterClientMode();
    }


    /**
     * When ECM is active, we might get addition signals of ECM mode, we must not exit until they
     * are all cleared.
     */
    @Test
    public void testProperExitFromEcmModeWithMultipleMessages() throws Exception {
        verify(mWifiStateMachinePrime).disableWifi();

        enableWifi();
        verify(mWifiStateMachinePrime).enterClientMode();

        reset(mWifiStateMachinePrime);

        // Test with WifiDisableInECBM turned on:
        when(mFacade.getConfigWiFiDisableInECBM(mContext)).thenReturn(true);

        mWifiController.sendMessage(CMD_EMERGENCY_MODE_CHANGED, 1);
        mWifiController.sendMessage(CMD_EMERGENCY_CALL_STATE_CHANGED, 1);
        mWifiController.sendMessage(CMD_EMERGENCY_CALL_STATE_CHANGED, 1);
        mWifiController.sendMessage(CMD_EMERGENCY_MODE_CHANGED, 1);
        mWifiController.sendMessage(CMD_EMERGENCY_MODE_CHANGED, 1);
        mLooper.dispatchAll();
        verify(mWifiStateMachinePrime).shutdownWifi();

        mWifiController.sendMessage(CMD_EMERGENCY_MODE_CHANGED, 0);
        mLooper.dispatchAll();
        mWifiController.sendMessage(CMD_EMERGENCY_MODE_CHANGED, 0);
        mLooper.dispatchAll();
        mWifiController.sendMessage(CMD_EMERGENCY_MODE_CHANGED, 0);
        mLooper.dispatchAll();
        mWifiController.sendMessage(CMD_EMERGENCY_CALL_STATE_CHANGED, 0);
        mLooper.dispatchAll();
        verify(mWifiStateMachinePrime, never()).enterClientMode();

        // now we will exit ECM
        mWifiController.sendMessage(CMD_EMERGENCY_CALL_STATE_CHANGED, 0);
        mLooper.dispatchAll();

        // now we can re-enable wifi
        verify(mWifiStateMachinePrime).enterClientMode();
    }

    /**
     * Toggling wifi when in ECM does not exit ecm mode and enable wifi
     */
    @Test
    public void testWifiDoesNotToggleOnWhenInEcm() throws Exception {

        // Test with WifiDisableInECBM turned on:
        when(mFacade.getConfigWiFiDisableInECBM(mContext)).thenReturn(true);

        // test ecm changed
        mWifiController.sendMessage(CMD_EMERGENCY_MODE_CHANGED, 1);
        mLooper.dispatchAll();
        assertEquals("EcmState", getCurrentState().getName());

        verify(mWifiStateMachinePrime).shutdownWifi();

        // now toggle wifi and verify we do not start wifi
        when(mSettingsStore.isWifiToggleEnabled()).thenReturn(true);
        mWifiController.sendMessage(CMD_WIFI_TOGGLED);
        mLooper.dispatchAll();

        verify(mWifiStateMachinePrime, never()).enterClientMode();
    }

    /**
     * Toggling scan mode when in ECM does not exit ecm mode and enable scan mode
     */
    @Test
    public void testScanModeDoesNotToggleOnWhenInEcm() throws Exception {

        // Test with WifiDisableInECBM turned on:
        when(mFacade.getConfigWiFiDisableInECBM(mContext)).thenReturn(true);

        // test ecm changed
        mWifiController.sendMessage(CMD_EMERGENCY_MODE_CHANGED, 1);
        mLooper.dispatchAll();
        assertEquals("EcmState", getCurrentState().getName());

        verify(mWifiStateMachinePrime).shutdownWifi();

        // now enable scanning and verify we do not start wifi
        when(mSettingsStore.isWifiToggleEnabled()).thenReturn(true);
        when(mSettingsStore.isScanAlwaysAvailable()).thenReturn(true);

        mWifiController.sendMessage(CMD_SCAN_ALWAYS_MODE_CHANGED);
        mLooper.dispatchAll();

        verify(mWifiStateMachinePrime, never()).enterScanOnlyMode();
    }

    /**
     * Toggling softap mode when in ECM does not exit ecm mode and enable softap
     */
    @Test
    public void testSoftApModeDoesNotToggleOnWhenInEcm() throws Exception {

        // Test with WifiDisableInECBM turned on:
        when(mFacade.getConfigWiFiDisableInECBM(mContext)).thenReturn(true);

        // test ecm changed
        mWifiController.sendMessage(CMD_EMERGENCY_MODE_CHANGED, 1);
        mLooper.dispatchAll();
        assertEquals("EcmState", getCurrentState().getName());

        verify(mWifiStateMachinePrime).shutdownWifi();

        mWifiController.sendMessage(CMD_SET_AP);
        mLooper.dispatchAll();

        verify(mWifiStateMachinePrime, never()).enterSoftAPMode(any());
    }

    /**
     * When AP mode is enabled and wifi was previously in AP mode, we should return to
     * DeviceActiveState after the AP is disabled.
     * Enter DeviceActiveState, activate AP mode, disable AP mode.
     * <p>
     * Expected: AP should successfully start and exit, then return to DeviceActiveState.
     */
    @Test
    public void testReturnToDeviceActiveStateAfterAPModeShutdown() throws Exception {
        enableWifi();
        assertEquals("DeviceActiveState", getCurrentState().getName());

        mWifiController.obtainMessage(CMD_SET_AP, 1, 0).sendToTarget();
        // add an "unexpected" sta mode stop to simulate a single interface device
        mClientModeCallback.onStateChanged(WifiManager.WIFI_STATE_DISABLED);
        mLooper.dispatchAll();

        when(mSettingsStore.getWifiSavedState()).thenReturn(1);
        mWifiController.obtainMessage(CMD_AP_STOPPED).sendToTarget();
        mLooper.dispatchAll();

        InOrder inOrder = inOrder(mWifiStateMachinePrime);
        inOrder.verify(mWifiStateMachinePrime).enterClientMode();
        assertEquals("DeviceActiveState", getCurrentState().getName());
    }

    /**
     * When AP mode is enabled and wifi is toggled on, we should transition to
     * DeviceActiveState after the AP is disabled.
     * Enter DeviceActiveState, activate AP mode, toggle WiFi.
     * <p>
     * Expected: AP should successfully start and exit, then return to DeviceActiveState.
     */
    @Test
    public void testReturnToDeviceActiveStateAfterWifiEnabledShutdown() throws Exception {
        enableWifi();
        assertEquals("DeviceActiveState", getCurrentState().getName());

        mWifiController.obtainMessage(CMD_SET_AP, 1, 0).sendToTarget();
        mLooper.dispatchAll();

        when(mSettingsStore.isWifiToggleEnabled()).thenReturn(true);
        mWifiController.obtainMessage(CMD_WIFI_TOGGLED).sendToTarget();
        mWifiController.obtainMessage(CMD_AP_STOPPED).sendToTarget();
        mLooper.dispatchAll();

        InOrder inOrder = inOrder(mWifiStateMachinePrime);
        inOrder.verify(mWifiStateMachinePrime).enterClientMode();
        assertEquals("DeviceActiveState", getCurrentState().getName());
    }

    @Test
    public void testRestartWifiStackInStaEnabledStateTriggersBugReport() throws Exception {
        enableWifi();
        mWifiController.sendMessage(CMD_RECOVERY_RESTART_WIFI,
                                    SelfRecovery.REASON_WIFINATIVE_FAILURE);
        mLooper.dispatchAll();
        verify(mWifiStateMachine).takeBugReport(anyString(), anyString());
    }

    @Test
    public void testRestartWifiWatchdogDoesNotTriggerBugReport() throws Exception {
        enableWifi();
        mWifiController.sendMessage(CMD_RECOVERY_RESTART_WIFI,
                                    SelfRecovery.REASON_LAST_RESORT_WATCHDOG);
        mLooper.dispatchAll();
        verify(mWifiStateMachine, never()).takeBugReport(anyString(), anyString());
    }

    /**
     * When in sta mode, CMD_RECOVERY_DISABLE_WIFI messages should trigger wifi to disable.
     */
    @Test
    public void testRecoveryDisabledTurnsWifiOff() throws Exception {
        enableWifi();
        reset(mWifiStateMachinePrime);
        mWifiController.sendMessage(CMD_RECOVERY_DISABLE_WIFI);
        mLooper.dispatchAll();
        verify(mWifiStateMachinePrime).disableWifi();
    }

    /**
     * When wifi is disabled, CMD_RECOVERY_DISABLE_WIFI should not trigger a state change.
     */
    @Test
    public void testRecoveryDisabledWhenWifiAlreadyOff() throws Exception {
        assertEquals("StaDisabledState", getCurrentState().getName());
        mWifiController.sendMessage(CMD_RECOVERY_DISABLE_WIFI);
        mLooper.dispatchAll();
        verify(mWifiStateMachinePrime).shutdownWifi();
    }

    /**
     * The command to trigger a WiFi reset should not trigger any action by WifiController if we
     * are not in STA mode.
     * WiFi is not in connect mode, so any calls to reset the wifi stack due to connection failures
     * should be ignored.
     * Create and start WifiController in StaDisabledState, send command to restart WiFi
     * <p>
     * Expected: WiFiController should not call WifiStateMachinePrime.disableWifi()
     */
    @Test
    public void testRestartWifiStackInStaDisabledState() throws Exception {
        // Start a new WifiController with wifi disabled
        when(mSettingsStore.isAirplaneModeOn()).thenReturn(false);
        when(mSettingsStore.isWifiToggleEnabled()).thenReturn(false);
        when(mSettingsStore.isScanAlwaysAvailable()).thenReturn(false);

        mWifiController = new WifiController(mContext, mWifiStateMachine, mLooper.getLooper(),
                mSettingsStore, mLooper.getLooper(), mFacade, mWifiStateMachinePrime);

        mWifiController.start();
        mLooper.dispatchAll();

        reset(mWifiStateMachine);
        assertEquals("StaDisabledState", getCurrentState().getName());

        reset(mWifiStateMachinePrime);
        mWifiController.sendMessage(CMD_RECOVERY_RESTART_WIFI);
        mLooper.dispatchAll();
        verify(mWifiStateMachinePrime).disableWifi();
    }

    /**
     * The command to trigger a WiFi reset should not trigger any action by WifiController if we
     * are not in STA mode, even if scans are allowed.
     * WiFi is not in connect mode, so any calls to reset the wifi stack due to connection failures
     * should be ignored.
     * Create and start WifiController in StaDisabledState, send command to restart WiFi
     * <p>
     * Expected: WiFiController should not call WifiStateMachinePrime.disableWifi() or
     * WifiStateMachinePrime.shutdownWifi().
     */
    @Test
    public void testRestartWifiStackInStaDisabledWithScanState() throws Exception {
        when(mSettingsStore.isScanAlwaysAvailable()).thenReturn(true);
        mWifiController.sendMessage(CMD_SCAN_ALWAYS_MODE_CHANGED);
        mLooper.dispatchAll();
        verify(mWifiStateMachinePrime).enterScanOnlyMode();

        reset(mWifiStateMachinePrime);
        mWifiController.sendMessage(CMD_RECOVERY_RESTART_WIFI);
        mLooper.dispatchAll();
        InOrder inOrder = inOrder(mWifiStateMachinePrime);
        verify(mWifiStateMachinePrime).disableWifi();
        verify(mWifiStateMachinePrime).enterScanOnlyMode();
    }

    /**
     * The command to trigger a WiFi reset should trigger a wifi reset in WifiStateMachine through
     * the WifiStateMachinePrime.shutdownWifi() call when in STA mode.
     * WiFi is in connect mode, calls to reset the wifi stack due to connection failures
     * should trigger a supplicant stop, and subsequently, a driver reload.
     * Create and start WifiController in DeviceActiveState, send command to restart WiFi
     * <p>
     * Expected: WiFiController should call WifiStateMachinePrime.shutdownWifi() and
     * WifiStateMachinePrime should enter CONNECT_MODE and the wifi driver should be started.
     */
    @Test
    public void testRestartWifiStackInStaEnabledState() throws Exception {
        enableWifi();
        assertEquals("DeviceActiveState", getCurrentState().getName());

        mWifiController.sendMessage(CMD_RECOVERY_RESTART_WIFI);
        mLooper.dispatchAll();
        InOrder inOrder = inOrder(mWifiStateMachinePrime);
        inOrder.verify(mWifiStateMachinePrime).shutdownWifi();
        inOrder.verify(mWifiStateMachinePrime).enterClientMode();
        assertEquals("DeviceActiveState", getCurrentState().getName());
    }

    /**
     * The command to trigger a WiFi reset should not trigger a reset when in ECM mode.
     * Enable wifi and enter ECM state, send command to restart wifi.
     * <p>
     * Expected: The command to trigger a wifi reset should be ignored and we should remain in ECM
     * mode.
     */
    @Test
    public void testRestartWifiStackDoesNotExitECMMode() throws Exception {
        enableWifi();
        assertEquals("DeviceActiveState", getCurrentState().getName());
        when(mFacade.getConfigWiFiDisableInECBM(mContext)).thenReturn(true);

        mWifiController.sendMessage(CMD_EMERGENCY_CALL_STATE_CHANGED, 1);
        mLooper.dispatchAll();
        assertEquals("EcmState", getCurrentState().getName());
        verify(mWifiStateMachinePrime).shutdownWifi();

        reset(mWifiStateMachinePrime);
        mWifiController.sendMessage(CMD_RECOVERY_RESTART_WIFI);
        mLooper.dispatchAll();
        assertEquals("EcmState", getCurrentState().getName());

        verifyZeroInteractions(mWifiStateMachinePrime);
    }

    /**
     * The command to trigger a WiFi reset should trigger a reset when in AP mode.
     * Enter AP mode, send command to restart wifi.
     * <p>
     * Expected: The command to trigger a wifi reset should trigger wifi shutdown.
     */
    @Test
    public void testRestartWifiStackFullyStopsWifi() throws Exception {
        mWifiController.obtainMessage(CMD_SET_AP, 1).sendToTarget();
        mLooper.dispatchAll();
        verify(mWifiStateMachinePrime).enterSoftAPMode(any());

        reset(mWifiStateMachinePrime);
        mWifiController.sendMessage(CMD_RECOVERY_RESTART_WIFI);
        mLooper.dispatchAll();
        verify(mWifiStateMachinePrime).shutdownWifi();
    }
}
