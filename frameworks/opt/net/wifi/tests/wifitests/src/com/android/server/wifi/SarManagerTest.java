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
 * limitations under the License.
 */

package com.android.server.wifi;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.mockito.Mockito.*;

import static android.telephony.TelephonyManager.CALL_STATE_IDLE;
import static android.telephony.TelephonyManager.CALL_STATE_OFFHOOK;

import android.app.test.MockAnswerUtil.AnswerWithArguments;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.IntentFilter;
import android.content.pm.ApplicationInfo;
import android.hardware.SensorEvent;
import android.net.wifi.WifiManager;
import android.os.Build;
import android.os.test.TestLooper;
import android.support.test.filters.SmallTest;
import android.telephony.PhoneStateListener;
import android.telephony.TelephonyManager;
import android.util.Log;

import com.android.internal.R;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.mockito.ArgumentCaptor;
import org.mockito.InOrder;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

/**
 * unit tests for {@link com.android.server.wifi.SarManager}.
 */
@SmallTest
public class SarManagerTest {
    private static final String TAG = "WifiSarManagerTest";
    private static final String OP_PACKAGE_NAME = "com.xxx";

    private void enableDebugLogs() {
        mSarMgr.enableVerboseLogging(1);
    }

    private MockResources getMockResources() {
        MockResources resources = new MockResources();
        return resources;
    }

    private SarManager mSarMgr;
    private TestLooper mLooper;
    private MockResources mResources;
    private PhoneStateListener mPhoneStateListener;

    @Mock  private Context mContext;
    @Mock TelephonyManager mTelephonyManager;
    @Mock private ApplicationInfo mMockApplInfo;
    @Mock WifiNative mWifiNative;

    @Before
    public void setUp() throws Exception {
        Log.e(TAG, "Setting Up ...");

        // Ensure Looper exists
        mLooper = new TestLooper();

        MockitoAnnotations.initMocks(this);

        /* Default behavior is to return with success */
        when(mWifiNative.selectTxPowerScenario(anyInt())).thenReturn(true);

        mResources = getMockResources();

        when(mContext.getResources()).thenReturn(mResources);
        mMockApplInfo.targetSdkVersion = Build.VERSION_CODES.P;
        when(mContext.getApplicationInfo()).thenReturn(mMockApplInfo);
        when(mContext.getOpPackageName()).thenReturn(OP_PACKAGE_NAME);
    }

    @After
    public void cleanUp() throws Exception {
        mSarMgr = null;
        mLooper = null;
        mContext = null;
        mResources = null;
    }

    /**
     * Helper function to set configuration for SAR and create the SAR Manager
     *
     */
    private void createSarManager(boolean isSarEnabled) {
        mResources.setBoolean(
                R.bool.config_wifi_framework_enable_voice_call_sar_tx_power_limit, isSarEnabled);

        mSarMgr = new SarManager(mContext, mTelephonyManager, mLooper.getLooper(),
                mWifiNative);

        if (isSarEnabled) {
            /* Capture the PhoneStateListener */
            ArgumentCaptor<PhoneStateListener> phoneStateListenerCaptor =
                    ArgumentCaptor.forClass(PhoneStateListener.class);
            verify(mTelephonyManager).listen(phoneStateListenerCaptor.capture(),
                    eq(PhoneStateListener.LISTEN_CALL_STATE));
            mPhoneStateListener = phoneStateListenerCaptor.getValue();
        }

        /* Enable logs from SarManager */
        enableDebugLogs();
    }

    /**
     * Test that we do register the telephony call state listener on devices which do support
     * setting/resetting Tx power limit.
     */
    @Test
    public void testSarMgr_enabledTxPowerScenario_registerPhone() throws Exception {
        createSarManager(true);
        verify(mTelephonyManager).listen(any(), eq(PhoneStateListener.LISTEN_CALL_STATE));
    }

    /**
     * Test that we do not register the telephony call state listener on devices which
     * do not support setting/resetting Tx power limit.
     */
    @Test
    public void testSarMgr_disabledTxPowerScenario_registerPhone() throws Exception {
        createSarManager(false);
        verify(mTelephonyManager, never()).listen(any(), anyInt());
    }

    /**
     * Test that for devices that support setting/resetting Tx Power limits, device sets the proper
     * Tx power scenario upon receiving {@link TelephonyManager#CALL_STATE_OFFHOOK} when WiFi STA
     * is enabled
     * In this case Wifi is enabled first, then off-hook is detected
     * Expectation is to get {@link WifiNative#TX_POWER_SCENARIO_NORMAL} when WiFi is turned on
     * followed by {@link WifiNative#TX_POWER_SCENARIO_VOICE_CALL} when OFFHOOK event is detected
     */
    @Test
    public void testSarMgr_enabledTxPowerScenario_wifiOn_offHook() throws Exception {
        createSarManager(true);
        assertNotNull(mPhoneStateListener);

        InOrder inOrder = inOrder(mWifiNative);

        /* Enable WiFi State */
        mSarMgr.setClientWifiState(WifiManager.WIFI_STATE_ENABLED);

        inOrder.verify(mWifiNative).selectTxPowerScenario(
                eq(WifiNative.TX_POWER_SCENARIO_NORMAL));

        /* Set phone state to OFFHOOK */
        mPhoneStateListener.onCallStateChanged(CALL_STATE_OFFHOOK, "");

        inOrder.verify(mWifiNative).selectTxPowerScenario(
                eq(WifiNative.TX_POWER_SCENARIO_VOICE_CALL));
    }

    /**
     * Test that for devices that support setting/resetting Tx Power limits, device sets the proper
     * Tx power scenario upon receiving {@link TelephonyManager#CALL_STATE_OFFHOOK} when WiFi STA
     * is enabled
     * In this case off-hook event is detected first, then wifi is turned on
     * Expectation is to get {@link WifiNative#TX_POWER_SCENARIO_VOICE_CALL} once wifi is turned on
     */
    @Test
    public void testSarMgr_enabledTxPowerScenario_offHook_wifiOn() throws Exception {
        createSarManager(true);
        assertNotNull(mPhoneStateListener);

        /* Set phone state to OFFHOOK */
        mPhoneStateListener.onCallStateChanged(CALL_STATE_OFFHOOK, "");

        /* Enable WiFi State */
        mSarMgr.setClientWifiState(WifiManager.WIFI_STATE_ENABLED);

        verify(mWifiNative).selectTxPowerScenario(
                eq(WifiNative.TX_POWER_SCENARIO_VOICE_CALL));
    }

    /**
     * Test that for devices that support setting/resetting Tx Power limits, device sets the proper
     * Tx power scenarios upon receiving {@link TelephonyManager#CALL_STATE_OFFHOOK} and
     * {@link TelephonyManager#CALL_STATE_OFFHOOK} when WiFi STA is enabled
     */
    @Test
    public void testSarMgr_enabledTxPowerScenario_wifiOn_offHook_onHook() throws Exception {
        createSarManager(true);
        assertNotNull(mPhoneStateListener);

        InOrder inOrder = inOrder(mWifiNative);

        /* Enable WiFi State */
        mSarMgr.setClientWifiState(WifiManager.WIFI_STATE_ENABLED);

        /* Now device should set tx power scenario to NORMAL */
        inOrder.verify(mWifiNative).selectTxPowerScenario(
                eq(WifiNative.TX_POWER_SCENARIO_NORMAL));

        /* Set phone state to OFFHOOK */
        mPhoneStateListener.onCallStateChanged(CALL_STATE_OFFHOOK, "");

        /* Device should set tx power scenario to Voice call */
        inOrder.verify(mWifiNative).selectTxPowerScenario(
                eq(WifiNative.TX_POWER_SCENARIO_VOICE_CALL));

        /* Set state back to ONHOOK */
        mPhoneStateListener.onCallStateChanged(CALL_STATE_IDLE, "");

        /* Device should set tx power scenario to NORMAL again */
        inOrder.verify(mWifiNative).selectTxPowerScenario(
                eq(WifiNative.TX_POWER_SCENARIO_NORMAL));
    }

    /**
     * Test that for devices that support setting/resetting Tx Power limits, device does not
     * sets the Tx power scenarios upon receiving {@link TelephonyManager#CALL_STATE_OFFHOOK} and
     * {@link TelephonyManager#CALL_STATE_OFFHOOK} when WiFi STA is disabled
     */
    @Test
    public void testSarMgr_enabledTxPowerScenario_wifiOff_offHook_onHook() throws Exception {
        createSarManager(true);
        assertNotNull(mPhoneStateListener);

        InOrder inOrder = inOrder(mWifiNative);

        /* Set phone state to OFFHOOK */
        mPhoneStateListener.onCallStateChanged(CALL_STATE_OFFHOOK, "");

        /* Set state back to ONHOOK */
        mPhoneStateListener.onCallStateChanged(CALL_STATE_IDLE, "");

        /* Device should not set tx power scenario at all */
        inOrder.verify(mWifiNative, never()).selectTxPowerScenario(anyInt());
    }
}
