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

import static android.net.wifi.WifiConfiguration.KeyMgmt.WPA_PSK;
import static android.net.wifi.WifiManager.EXTRA_PREVIOUS_WIFI_AP_STATE;
import static android.net.wifi.WifiManager.EXTRA_WIFI_AP_FAILURE_REASON;
import static android.net.wifi.WifiManager.EXTRA_WIFI_AP_INTERFACE_NAME;
import static android.net.wifi.WifiManager.EXTRA_WIFI_AP_MODE;
import static android.net.wifi.WifiManager.EXTRA_WIFI_AP_STATE;
import static android.net.wifi.WifiManager.WIFI_AP_STATE_DISABLED;
import static android.net.wifi.WifiManager.WIFI_AP_STATE_DISABLING;
import static android.net.wifi.WifiManager.WIFI_AP_STATE_ENABLED;
import static android.net.wifi.WifiManager.WIFI_AP_STATE_ENABLING;
import static android.net.wifi.WifiManager.WIFI_AP_STATE_FAILED;

import static com.android.server.wifi.LocalOnlyHotspotRequestInfo.HOTSPOT_NO_ERROR;

import static org.junit.Assert.assertEquals;
import static org.mockito.Mockito.*;

import android.app.test.TestAlarmManager;
import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.database.ContentObserver;
import android.net.Uri;
import android.net.wifi.IApInterfaceEventCallback;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiScanner;
import android.os.UserHandle;
import android.os.test.TestLooper;
import android.provider.Settings;
import android.support.test.filters.SmallTest;

import com.android.internal.R;
import com.android.internal.util.WakeupMessage;

import org.junit.Before;
import org.junit.Test;
import org.mockito.ArgumentCaptor;
import org.mockito.InOrder;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.util.List;
import java.util.Locale;

/** Unit tests for {@link SoftApManager}. */
@SmallTest
public class SoftApManagerTest {

    private static final String TAG = "SoftApManagerTest";

    private static final String DEFAULT_SSID = "DefaultTestSSID";
    private static final String TEST_SSID = "TestSSID";
    private static final String TEST_PASSWORD = "TestPassword";
    private static final String TEST_COUNTRY_CODE = "TestCountry";
    private static final String TEST_INTERFACE_NAME = "testif0";
    private static final String OTHER_INTERFACE_NAME = "otherif";
    private static final int TEST_NUM_CONNECTED_CLIENTS = 4;
    private static final int[] ALLOWED_2G_CHANNELS = {2412, 2417, 2437};
    private static final int[] ALLOWED_5G_CHANNELS = {5180, 5190, 5240};

    private final WifiConfiguration mDefaultApConfig = createDefaultApConfig();

    private ContentObserver mContentObserver;
    private TestLooper mLooper;
    private TestAlarmManager mAlarmManager;

    @Mock Context mContext;
    @Mock Resources mResources;
    @Mock WifiNative mWifiNative;
    @Mock WifiManager.SoftApCallback mCallback;
    @Mock FrameworkFacade mFrameworkFacade;
    @Mock WifiApConfigStore mWifiApConfigStore;
    @Mock WifiMetrics mWifiMetrics;
    final ArgumentCaptor<WifiNative.InterfaceCallback> mWifiNativeInterfaceCallbackCaptor =
            ArgumentCaptor.forClass(WifiNative.InterfaceCallback.class);
    final ArgumentCaptor<WifiNative.SoftApListener> mSoftApListenerCaptor =
            ArgumentCaptor.forClass(WifiNative.SoftApListener.class);

    SoftApManager mSoftApManager;

    /** Sets up test. */
    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);
        mLooper = new TestLooper();

        when(mWifiNative.startSoftAp(eq(TEST_INTERFACE_NAME), any(), any())).thenReturn(true);

        when(mFrameworkFacade.getIntegerSetting(
                mContext, Settings.Global.SOFT_AP_TIMEOUT_ENABLED, 1)).thenReturn(1);
        mAlarmManager = new TestAlarmManager();
        when(mContext.getSystemService(Context.ALARM_SERVICE))
                .thenReturn(mAlarmManager.getAlarmManager());
        when(mContext.getResources()).thenReturn(mResources);
        when(mResources.getInteger(R.integer.config_wifi_framework_soft_ap_timeout_delay))
                .thenReturn(600000);
    }

    private WifiConfiguration createDefaultApConfig() {
        WifiConfiguration defaultConfig = new WifiConfiguration();
        defaultConfig.SSID = DEFAULT_SSID;
        return defaultConfig;
    }

    private SoftApManager createSoftApManager(SoftApModeConfiguration config) throws Exception {
        if (config.getWifiConfiguration() == null) {
            when(mWifiApConfigStore.getApConfiguration()).thenReturn(mDefaultApConfig);
        }
        SoftApManager newSoftApManager = new SoftApManager(mContext,
                                                           mLooper.getLooper(),
                                                           mFrameworkFacade,
                                                           mWifiNative,
                                                           TEST_COUNTRY_CODE,
                                                           mCallback,
                                                           mWifiApConfigStore,
                                                           config,
                                                           mWifiMetrics);
        mLooper.dispatchAll();

        return newSoftApManager;
    }

    /** Verifies startSoftAp will use default config if AP configuration is not provided. */
    @Test
    public void startSoftApWithoutConfig() throws Exception {
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null);
        startSoftApAndVerifyEnabled(apConfig);
    }

    /** Verifies startSoftAp will use provided config and start AP. */
    @Test
    public void startSoftApWithConfig() throws Exception {
        WifiConfiguration config = new WifiConfiguration();
        config.apBand = WifiConfiguration.AP_BAND_2GHZ;
        config.SSID = TEST_SSID;
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, config);
        startSoftApAndVerifyEnabled(apConfig);
    }

    /**
     * Verifies startSoftAp will start with the hiddenSSID param set when it is set to true in the
     * supplied config.
     */
    @Test
    public void startSoftApWithHiddenSsidTrueInConfig() throws Exception {
        WifiConfiguration config = new WifiConfiguration();
        config.apBand = WifiConfiguration.AP_BAND_2GHZ;
        config.SSID = TEST_SSID;
        config.hiddenSSID = true;
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, config);
        startSoftApAndVerifyEnabled(apConfig);
    }

    /**
     * Verifies startSoftAp will start with the password param set in the
     * supplied config.
     */
    @Test
    public void startSoftApWithPassphraseInConfig() throws Exception {
        WifiConfiguration config = new WifiConfiguration();
        config.apBand = WifiConfiguration.AP_BAND_5GHZ;
        config.SSID = TEST_SSID;
        config.allowedKeyManagement.set(WPA_PSK);
        config.preSharedKey = TEST_PASSWORD;
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, config);
        startSoftApAndVerifyEnabled(apConfig);
    }

    /** Tests softap startup if default config fails to load. **/
    @Test
    public void startSoftApDefaultConfigFailedToLoad() throws Exception {
        when(mWifiNative.setupInterfaceForSoftApMode(any())).thenReturn(TEST_INTERFACE_NAME);

        when(mWifiApConfigStore.getApConfiguration()).thenReturn(null);
        SoftApModeConfiguration nullApConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null);
        SoftApManager newSoftApManager = new SoftApManager(mContext,
                                                           mLooper.getLooper(),
                                                           mFrameworkFacade,
                                                           mWifiNative,
                                                           TEST_COUNTRY_CODE,
                                                           mCallback,
                                                           mWifiApConfigStore,
                                                           nullApConfig,
                                                           mWifiMetrics);
        mLooper.dispatchAll();
        newSoftApManager.start();
        mLooper.dispatchAll();
        verify(mCallback).onStateChanged(WifiManager.WIFI_AP_STATE_FAILED,
                WifiManager.SAP_START_FAILURE_GENERAL);
        ArgumentCaptor<Intent> intentCaptor = ArgumentCaptor.forClass(Intent.class);
        verify(mContext, times(2)).sendStickyBroadcastAsUser(intentCaptor.capture(),
                eq(UserHandle.ALL));

        List<Intent> capturedIntents = intentCaptor.getAllValues();
        checkApStateChangedBroadcast(capturedIntents.get(0), WIFI_AP_STATE_ENABLING,
                WIFI_AP_STATE_DISABLED, HOTSPOT_NO_ERROR, TEST_INTERFACE_NAME,
                nullApConfig.getTargetMode());
        checkApStateChangedBroadcast(capturedIntents.get(1), WIFI_AP_STATE_FAILED,
                WIFI_AP_STATE_ENABLING, WifiManager.SAP_START_FAILURE_GENERAL, TEST_INTERFACE_NAME,
                nullApConfig.getTargetMode());
    }

    /**
     * Test that failure to retrieve the SoftApInterface name increments the corresponding metrics
     * and proper state updates are sent out.
     */
    @Test
    public void testSetupForSoftApModeNullApInterfaceNameFailureIncrementsMetrics()
            throws Exception {
        when(mWifiNative.setupInterfaceForSoftApMode(any())).thenReturn(null);

        SoftApModeConfiguration config = new SoftApModeConfiguration(
                WifiManager.IFACE_IP_MODE_TETHERED, new WifiConfiguration());

        when(mWifiApConfigStore.getApConfiguration()).thenReturn(null);
        SoftApModeConfiguration nullApConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null);
        SoftApManager newSoftApManager = new SoftApManager(mContext,
                                                           mLooper.getLooper(),
                                                           mFrameworkFacade,
                                                           mWifiNative,
                                                           TEST_COUNTRY_CODE,
                                                           mCallback,
                                                           mWifiApConfigStore,
                                                           nullApConfig,
                                                           mWifiMetrics);
        mLooper.dispatchAll();
        newSoftApManager.start();
        mLooper.dispatchAll();
        verify(mCallback).onStateChanged(WifiManager.WIFI_AP_STATE_FAILED,
                WifiManager.SAP_START_FAILURE_GENERAL);
        ArgumentCaptor<Intent> intentCaptor = ArgumentCaptor.forClass(Intent.class);
        verify(mContext).sendStickyBroadcastAsUser(intentCaptor.capture(),
                eq(UserHandle.ALL));

        checkApStateChangedBroadcast(intentCaptor.getValue(), WIFI_AP_STATE_FAILED,
                WIFI_AP_STATE_DISABLED, WifiManager.SAP_START_FAILURE_GENERAL, null,
                nullApConfig.getTargetMode());

        verify(mWifiMetrics).incrementSoftApStartResult(false,
                WifiManager.SAP_START_FAILURE_GENERAL);
    }

    /**
     * Test that an empty SoftApInterface name is detected as a failure and increments the
     * corresponding metrics and proper state updates are sent out.
     */
    @Test
    public void testSetupForSoftApModeEmptyInterfaceNameFailureIncrementsMetrics()
            throws Exception {
        when(mWifiNative.setupInterfaceForSoftApMode(any())).thenReturn("");

        SoftApModeConfiguration config = new SoftApModeConfiguration(
                WifiManager.IFACE_IP_MODE_TETHERED, new WifiConfiguration());

        when(mWifiApConfigStore.getApConfiguration()).thenReturn(null);
        SoftApModeConfiguration nullApConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null);
        SoftApManager newSoftApManager = new SoftApManager(mContext,
                                                           mLooper.getLooper(),
                                                           mFrameworkFacade,
                                                           mWifiNative,
                                                           TEST_COUNTRY_CODE,
                                                           mCallback,
                                                           mWifiApConfigStore,
                                                           nullApConfig,
                                                           mWifiMetrics);
        mLooper.dispatchAll();
        newSoftApManager.start();
        mLooper.dispatchAll();
        verify(mCallback).onStateChanged(WifiManager.WIFI_AP_STATE_FAILED,
                WifiManager.SAP_START_FAILURE_GENERAL);
        ArgumentCaptor<Intent> intentCaptor = ArgumentCaptor.forClass(Intent.class);
        verify(mContext).sendStickyBroadcastAsUser(intentCaptor.capture(),
                eq(UserHandle.ALL));

        checkApStateChangedBroadcast(intentCaptor.getValue(), WIFI_AP_STATE_FAILED,
                WIFI_AP_STATE_DISABLED, WifiManager.SAP_START_FAILURE_GENERAL, "",
                nullApConfig.getTargetMode());

        verify(mWifiMetrics).incrementSoftApStartResult(false,
                WifiManager.SAP_START_FAILURE_GENERAL);
    }

    /**
     * Tests that the generic error is propagated and properly reported when starting softap and the
     * specified channel cannot be used.
     */
    @Test
    public void startSoftApFailGeneralErrorForConfigChannel() throws Exception {
        WifiConfiguration config = new WifiConfiguration();
        config.apBand = WifiConfiguration.AP_BAND_5GHZ;
        config.SSID = TEST_SSID;
        SoftApModeConfiguration softApConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, config);

        when(mWifiNative.setupInterfaceForSoftApMode(any())).thenReturn(TEST_INTERFACE_NAME);
        when(mWifiNative.setCountryCodeHal(eq(TEST_INTERFACE_NAME), any())).thenReturn(false);

        SoftApManager newSoftApManager = new SoftApManager(mContext,
                                                           mLooper.getLooper(),
                                                           mFrameworkFacade,
                                                           mWifiNative,
                                                           "US",
                                                           mCallback,
                                                           mWifiApConfigStore,
                                                           softApConfig,
                                                           mWifiMetrics);
        mLooper.dispatchAll();
        newSoftApManager.start();
        mLooper.dispatchAll();

        ArgumentCaptor<Intent> intentCaptor = ArgumentCaptor.forClass(Intent.class);
        verify(mContext, times(2)).sendStickyBroadcastAsUser(intentCaptor.capture(),
                eq(UserHandle.ALL));

        List<Intent> capturedIntents = intentCaptor.getAllValues();
        checkApStateChangedBroadcast(capturedIntents.get(0), WIFI_AP_STATE_ENABLING,
                WIFI_AP_STATE_DISABLED, HOTSPOT_NO_ERROR, TEST_INTERFACE_NAME,
                softApConfig.getTargetMode());
        checkApStateChangedBroadcast(capturedIntents.get(1), WIFI_AP_STATE_FAILED,
                WIFI_AP_STATE_ENABLING, WifiManager.SAP_START_FAILURE_GENERAL, TEST_INTERFACE_NAME,
                softApConfig.getTargetMode());
    }

    /**
     * Tests that the NO_CHANNEL error is propagated and properly reported when starting softap and
     * a valid channel cannot be determined.
     */
    @Test
    public void startSoftApFailNoChannel() throws Exception {
        WifiConfiguration config = new WifiConfiguration();
        config.apBand = -2;
        config.apChannel = 0;
        config.SSID = TEST_SSID;
        SoftApModeConfiguration softApConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, config);

        when(mWifiNative.setupInterfaceForSoftApMode(any())).thenReturn(TEST_INTERFACE_NAME);
        when(mWifiNative.isHalStarted()).thenReturn(true);

        SoftApManager newSoftApManager = new SoftApManager(mContext,
                                                           mLooper.getLooper(),
                                                           mFrameworkFacade,
                                                           mWifiNative,
                                                           TEST_COUNTRY_CODE,
                                                           mCallback,
                                                           mWifiApConfigStore,
                                                           softApConfig,
                                                           mWifiMetrics);
        mLooper.dispatchAll();
        newSoftApManager.start();
        mLooper.dispatchAll();

        ArgumentCaptor<Intent> intentCaptor = ArgumentCaptor.forClass(Intent.class);
        verify(mContext, times(2)).sendStickyBroadcastAsUser(intentCaptor.capture(),
                eq(UserHandle.ALL));

        List<Intent> capturedIntents = intentCaptor.getAllValues();
        checkApStateChangedBroadcast(capturedIntents.get(0), WIFI_AP_STATE_ENABLING,
                WIFI_AP_STATE_DISABLED, HOTSPOT_NO_ERROR, TEST_INTERFACE_NAME,
                softApConfig.getTargetMode());
        checkApStateChangedBroadcast(capturedIntents.get(1), WIFI_AP_STATE_FAILED,
                WIFI_AP_STATE_ENABLING, WifiManager.SAP_START_FAILURE_NO_CHANNEL,
                TEST_INTERFACE_NAME, softApConfig.getTargetMode());
    }

    /**
     * Tests startup when Ap Interface fails to start successfully.
     */
    @Test
    public void startSoftApApInterfaceFailedToStart() throws Exception {
        when(mWifiNative.setupInterfaceForSoftApMode(any())).thenReturn(TEST_INTERFACE_NAME);
        when(mWifiNative.startSoftAp(eq(TEST_INTERFACE_NAME), any(), any())).thenReturn(false);

        SoftApModeConfiguration softApModeConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, mDefaultApConfig);

        SoftApManager newSoftApManager = new SoftApManager(mContext,
                                                           mLooper.getLooper(),
                                                           mFrameworkFacade,
                                                           mWifiNative,
                                                           TEST_COUNTRY_CODE,
                                                           mCallback,
                                                           mWifiApConfigStore,
                                                           softApModeConfig,
                                                           mWifiMetrics);

        mLooper.dispatchAll();
        newSoftApManager.start();
        mLooper.dispatchAll();
        verify(mCallback).onStateChanged(WifiManager.WIFI_AP_STATE_FAILED,
                WifiManager.SAP_START_FAILURE_GENERAL);
        verify(mWifiNative).teardownInterface(TEST_INTERFACE_NAME);
    }

    /**
     * Tests the handling of stop command when soft AP is not started.
     */
    @Test
    public void stopWhenNotStarted() throws Exception {
        mSoftApManager = createSoftApManager(
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null));
        mSoftApManager.stop();
        mLooper.dispatchAll();
        /* Verify no state changes. */
        verify(mCallback, never()).onStateChanged(anyInt(), anyInt());
        verify(mContext, never()).sendStickyBroadcastAsUser(any(), any());
        verify(mWifiNative, never()).teardownInterface(anyString());
    }

    /**
     * Tests the handling of stop command when soft AP is started.
     */
    @Test
    public void stopWhenStarted() throws Exception {
        SoftApModeConfiguration softApModeConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null);
        startSoftApAndVerifyEnabled(softApModeConfig);

        // reset to clear verified Intents for ap state change updates
        reset(mContext);

        InOrder order = inOrder(mCallback, mContext);

        mSoftApManager.stop();
        mLooper.dispatchAll();

        ArgumentCaptor<Intent> intentCaptor = ArgumentCaptor.forClass(Intent.class);
        order.verify(mCallback).onStateChanged(WifiManager.WIFI_AP_STATE_DISABLING, 0);
        order.verify(mContext).sendStickyBroadcastAsUser(intentCaptor.capture(),
                eq(UserHandle.ALL));
        checkApStateChangedBroadcast(intentCaptor.getValue(), WIFI_AP_STATE_DISABLING,
                WIFI_AP_STATE_ENABLED, HOTSPOT_NO_ERROR, TEST_INTERFACE_NAME,
                softApModeConfig.getTargetMode());

        order.verify(mCallback).onStateChanged(WifiManager.WIFI_AP_STATE_DISABLED, 0);
        order.verify(mContext).sendStickyBroadcastAsUser(intentCaptor.capture(),
                eq(UserHandle.ALL));
        checkApStateChangedBroadcast(intentCaptor.getValue(), WIFI_AP_STATE_DISABLED,
                WIFI_AP_STATE_DISABLING, HOTSPOT_NO_ERROR, TEST_INTERFACE_NAME,
                softApModeConfig.getTargetMode());
    }

    /**
     * Verify that onDestroyed properly reports softap stop.
     */
    @Test
    public void cleanStopOnInterfaceDestroyed() throws Exception {
        SoftApModeConfiguration softApModeConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null);
        startSoftApAndVerifyEnabled(softApModeConfig);

        // reset to clear verified Intents for ap state change updates
        reset(mContext);

        InOrder order = inOrder(mCallback, mContext);

        mWifiNativeInterfaceCallbackCaptor.getValue().onDestroyed(TEST_INTERFACE_NAME);

        mLooper.dispatchAll();
        ArgumentCaptor<Intent> intentCaptor = ArgumentCaptor.forClass(Intent.class);
        order.verify(mCallback).onStateChanged(WifiManager.WIFI_AP_STATE_DISABLING, 0);
        order.verify(mContext).sendStickyBroadcastAsUser(intentCaptor.capture(),
                eq(UserHandle.ALL));
        checkApStateChangedBroadcast(intentCaptor.getValue(), WIFI_AP_STATE_DISABLING,
                WIFI_AP_STATE_ENABLED, HOTSPOT_NO_ERROR, TEST_INTERFACE_NAME,
                softApModeConfig.getTargetMode());

        order.verify(mCallback).onStateChanged(WifiManager.WIFI_AP_STATE_DISABLED, 0);
        order.verify(mContext).sendStickyBroadcastAsUser(intentCaptor.capture(),
                eq(UserHandle.ALL));
        checkApStateChangedBroadcast(intentCaptor.getValue(), WIFI_AP_STATE_DISABLED,
                WIFI_AP_STATE_DISABLING, HOTSPOT_NO_ERROR, null,
                softApModeConfig.getTargetMode());
    }

    /**
     * Verify that onDestroyed after softap is stopped doesn't trigger a callback.
     */
    @Test
    public void noCallbackOnInterfaceDestroyedWhenAlreadyStopped() throws Exception {
        SoftApModeConfiguration softApModeConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null);
        startSoftApAndVerifyEnabled(softApModeConfig);

        mSoftApManager.stop();
        mLooper.dispatchAll();

        verify(mCallback).onStateChanged(WifiManager.WIFI_AP_STATE_DISABLING, 0);
        verify(mCallback).onStateChanged(WifiManager.WIFI_AP_STATE_DISABLED, 0);

        reset(mCallback);

        // now trigger interface destroyed and make sure callback doesn't get called
        mWifiNativeInterfaceCallbackCaptor.getValue().onDestroyed(TEST_INTERFACE_NAME);
        mLooper.dispatchAll();

        verifyNoMoreInteractions(mCallback);
    }

    /**
     * Verify that onDown is handled by SoftApManager.
     */
    @Test
    public void testInterfaceOnDownHandled() throws Exception {
        SoftApModeConfiguration softApModeConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null);
        startSoftApAndVerifyEnabled(softApModeConfig);

        // reset to clear verified Intents for ap state change updates
        reset(mContext, mCallback, mWifiNative);

        InOrder order = inOrder(mCallback, mContext);

        mWifiNativeInterfaceCallbackCaptor.getValue().onDown(TEST_INTERFACE_NAME);

        mLooper.dispatchAll();

        order.verify(mCallback).onStateChanged(WifiManager.WIFI_AP_STATE_FAILED,
                WifiManager.SAP_START_FAILURE_GENERAL);
        ArgumentCaptor<Intent> intentCaptor = ArgumentCaptor.forClass(Intent.class);
        verify(mContext, times(3)).sendStickyBroadcastAsUser(intentCaptor.capture(),
                eq(UserHandle.ALL));

        List<Intent> capturedIntents = intentCaptor.getAllValues();
        checkApStateChangedBroadcast(capturedIntents.get(0), WIFI_AP_STATE_FAILED,
                WIFI_AP_STATE_ENABLED, WifiManager.SAP_START_FAILURE_GENERAL, TEST_INTERFACE_NAME,
                softApModeConfig.getTargetMode());
        checkApStateChangedBroadcast(capturedIntents.get(1), WIFI_AP_STATE_DISABLING,
                WIFI_AP_STATE_FAILED, HOTSPOT_NO_ERROR, TEST_INTERFACE_NAME,
                softApModeConfig.getTargetMode());
        checkApStateChangedBroadcast(capturedIntents.get(2), WIFI_AP_STATE_DISABLED,
                WIFI_AP_STATE_DISABLING, HOTSPOT_NO_ERROR, TEST_INTERFACE_NAME,
                softApModeConfig.getTargetMode());
    }

    /**
     * Verify that onDown for a different interface name does not stop SoftApManager.
     */
    @Test
    public void testInterfaceOnDownForDifferentInterfaceDoesNotTriggerStop() throws Exception {
        SoftApModeConfiguration softApModeConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null);
        startSoftApAndVerifyEnabled(softApModeConfig);

        // reset to clear verified Intents for ap state change updates
        reset(mContext, mCallback, mWifiNative);

        InOrder order = inOrder(mCallback, mContext);

        mWifiNativeInterfaceCallbackCaptor.getValue().onDown(OTHER_INTERFACE_NAME);

        mLooper.dispatchAll();

        verifyNoMoreInteractions(mContext, mCallback, mWifiNative);
    }

    @Test
    public void updatesMetricsOnChannelSwitchedEvent() throws Exception {
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null);
        startSoftApAndVerifyEnabled(apConfig);

        final int channelFrequency = 2437;
        final int channelBandwidth = IApInterfaceEventCallback.BANDWIDTH_20;
        mSoftApListenerCaptor.getValue().onSoftApChannelSwitched(channelFrequency,
                channelBandwidth);
        mLooper.dispatchAll();

        verify(mWifiMetrics).addSoftApChannelSwitchedEvent(channelFrequency, channelBandwidth,
                apConfig.getTargetMode());
    }

    @Test
    public void updatesMetricsOnChannelSwitchedEventDetectsBandUnsatisfiedOnBand2Ghz()
            throws Exception {
        WifiConfiguration config = createDefaultApConfig();
        config.apBand = WifiConfiguration.AP_BAND_2GHZ;

        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, config);
        startSoftApAndVerifyEnabled(apConfig);

        final int channelFrequency = 2437;
        final int channelBandwidth = IApInterfaceEventCallback.BANDWIDTH_20;
        when(mWifiNative.getChannelsForBand(WifiScanner.WIFI_BAND_24_GHZ))
                .thenReturn(ALLOWED_5G_CHANNELS);
        mSoftApListenerCaptor.getValue().onSoftApChannelSwitched(channelFrequency,
                channelBandwidth);
        mLooper.dispatchAll();

        verify(mWifiNative).getChannelsForBand(WifiScanner.WIFI_BAND_24_GHZ);
        verify(mWifiMetrics).addSoftApChannelSwitchedEvent(channelFrequency, channelBandwidth,
                apConfig.getTargetMode());
        verify(mWifiMetrics).incrementNumSoftApUserBandPreferenceUnsatisfied();
    }

    @Test
    public void updatesMetricsOnChannelSwitchedEventDetectsBandUnsatisfiedOnBand5Ghz()
            throws Exception {
        WifiConfiguration config = createDefaultApConfig();
        config.apBand = WifiConfiguration.AP_BAND_5GHZ;

        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, config);
        startSoftApAndVerifyEnabled(apConfig);

        final int channelFrequency = 5180;
        final int channelBandwidth = IApInterfaceEventCallback.BANDWIDTH_20;
        when(mWifiNative.getChannelsForBand(WifiScanner.WIFI_BAND_5_GHZ))
                .thenReturn(ALLOWED_2G_CHANNELS);
        mSoftApListenerCaptor.getValue().onSoftApChannelSwitched(channelFrequency,
                channelBandwidth);
        mLooper.dispatchAll();

        verify(mWifiNative).getChannelsForBand(WifiScanner.WIFI_BAND_5_GHZ);
        verify(mWifiMetrics).addSoftApChannelSwitchedEvent(channelFrequency, channelBandwidth,
                apConfig.getTargetMode());
        verify(mWifiMetrics).incrementNumSoftApUserBandPreferenceUnsatisfied();
    }

    @Test
    public void updatesMetricsOnChannelSwitchedEventDetectsBandUnsatisfiedOnBandAny()
            throws Exception {
        WifiConfiguration config = createDefaultApConfig();
        config.apBand = WifiConfiguration.AP_BAND_ANY;

        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, config);
        startSoftApAndVerifyEnabled(apConfig);

        final int channelFrequency = 2437;
        final int channelBandwidth = IApInterfaceEventCallback.BANDWIDTH_20;
        when(mWifiNative.getChannelsForBand(WifiScanner.WIFI_BAND_24_GHZ))
                .thenReturn(new int[0]);
        when(mWifiNative.getChannelsForBand(WifiScanner.WIFI_BAND_5_GHZ))
                .thenReturn(new int[0]);
        mSoftApListenerCaptor.getValue().onSoftApChannelSwitched(channelFrequency,
                channelBandwidth);
        mLooper.dispatchAll();

        verify(mWifiNative).getChannelsForBand(WifiScanner.WIFI_BAND_24_GHZ);
        verify(mWifiNative).getChannelsForBand(WifiScanner.WIFI_BAND_5_GHZ);
        verify(mWifiMetrics).addSoftApChannelSwitchedEvent(channelFrequency, channelBandwidth,
                apConfig.getTargetMode());
        verify(mWifiMetrics).incrementNumSoftApUserBandPreferenceUnsatisfied();
    }

    @Test
    public void updatesMetricsOnChannelSwitchedEventDetectsBandUnsatisfiedOnlyWhenRequired()
            throws Exception {
        WifiConfiguration config = createDefaultApConfig();
        config.apBand = WifiConfiguration.AP_BAND_2GHZ;

        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, config);
        startSoftApAndVerifyEnabled(apConfig);

        final int channelFrequency = 2437;
        final int channelBandwidth = IApInterfaceEventCallback.BANDWIDTH_20;
        when(mWifiNative.getChannelsForBand(WifiScanner.WIFI_BAND_24_GHZ))
                .thenReturn(ALLOWED_2G_CHANNELS);
        mSoftApListenerCaptor.getValue().onSoftApChannelSwitched(channelFrequency,
                channelBandwidth);
        mLooper.dispatchAll();

        verify(mWifiNative).getChannelsForBand(WifiScanner.WIFI_BAND_24_GHZ);
        verify(mWifiMetrics).addSoftApChannelSwitchedEvent(channelFrequency, channelBandwidth,
                apConfig.getTargetMode());
        verify(mWifiMetrics, never()).incrementNumSoftApUserBandPreferenceUnsatisfied();
    }

    @Test
    public void updatesNumAssociatedStations() throws Exception {
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null);
        startSoftApAndVerifyEnabled(apConfig);

        mSoftApListenerCaptor.getValue().onNumAssociatedStationsChanged(
                TEST_NUM_CONNECTED_CLIENTS);
        mLooper.dispatchAll();

        verify(mCallback).onNumClientsChanged(TEST_NUM_CONNECTED_CLIENTS);
        verify(mWifiMetrics).addSoftApNumAssociatedStationsChangedEvent(TEST_NUM_CONNECTED_CLIENTS,
                apConfig.getTargetMode());
    }

    /**
     * If SoftApManager gets an update for the number of connected clients that is the same, do not
     * trigger callbacks a second time.
     */
    @Test
    public void testDoesNotTriggerCallbackForSameNumberClientUpdate() throws Exception {
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null);
        startSoftApAndVerifyEnabled(apConfig);

        mSoftApListenerCaptor.getValue().onNumAssociatedStationsChanged(
                TEST_NUM_CONNECTED_CLIENTS);
        mLooper.dispatchAll();

        // now trigger callback again, but we should have each method only called once
        mSoftApListenerCaptor.getValue().onNumAssociatedStationsChanged(
                TEST_NUM_CONNECTED_CLIENTS);
        mLooper.dispatchAll();

        verify(mCallback).onNumClientsChanged(TEST_NUM_CONNECTED_CLIENTS);
        verify(mWifiMetrics).addSoftApNumAssociatedStationsChangedEvent(TEST_NUM_CONNECTED_CLIENTS,
                apConfig.getTargetMode());
    }

    @Test
    public void handlesInvalidNumAssociatedStations() throws Exception {
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null);
        startSoftApAndVerifyEnabled(apConfig);

        /* Invalid values should be ignored */
        final int mInvalidNumClients = -1;
        mSoftApListenerCaptor.getValue().onNumAssociatedStationsChanged(mInvalidNumClients);
        mLooper.dispatchAll();
        verify(mCallback, never()).onNumClientsChanged(mInvalidNumClients);
        verify(mWifiMetrics, never()).addSoftApNumAssociatedStationsChangedEvent(anyInt(),
                anyInt());
    }

    @Test
    public void schedulesTimeoutTimerOnStart() throws Exception {
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null);
        startSoftApAndVerifyEnabled(apConfig);

        // Verify timer is scheduled
        verify(mAlarmManager.getAlarmManager()).setExact(anyInt(), anyLong(),
                eq(mSoftApManager.SOFT_AP_SEND_MESSAGE_TIMEOUT_TAG), any(), any());
    }

    @Test
    public void cancelsTimeoutTimerOnStop() throws Exception {
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null);
        startSoftApAndVerifyEnabled(apConfig);
        mSoftApManager.stop();
        mLooper.dispatchAll();

        // Verify timer is canceled
        verify(mAlarmManager.getAlarmManager()).cancel(any(WakeupMessage.class));
    }

    @Test
    public void cancelsTimeoutTimerOnNewClientsConnect() throws Exception {
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null);
        startSoftApAndVerifyEnabled(apConfig);
        mSoftApListenerCaptor.getValue().onNumAssociatedStationsChanged(
                TEST_NUM_CONNECTED_CLIENTS);
        mLooper.dispatchAll();

        // Verify timer is canceled
        verify(mAlarmManager.getAlarmManager()).cancel(any(WakeupMessage.class));
    }

    @Test
    public void schedulesTimeoutTimerWhenAllClientsDisconnect() throws Exception {
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null);
        startSoftApAndVerifyEnabled(apConfig);

        mSoftApListenerCaptor.getValue().onNumAssociatedStationsChanged(
                TEST_NUM_CONNECTED_CLIENTS);
        mLooper.dispatchAll();
        verify(mCallback).onNumClientsChanged(TEST_NUM_CONNECTED_CLIENTS);
        // Verify timer is canceled at this point
        verify(mAlarmManager.getAlarmManager()).cancel(any(WakeupMessage.class));

        mSoftApListenerCaptor.getValue().onNumAssociatedStationsChanged(0);
        mLooper.dispatchAll();
        verify(mCallback, times(2)).onNumClientsChanged(0);
        // Verify timer is scheduled again
        verify(mAlarmManager.getAlarmManager(), times(2)).setExact(anyInt(), anyLong(),
                eq(mSoftApManager.SOFT_AP_SEND_MESSAGE_TIMEOUT_TAG), any(), any());
    }

    @Test
    public void stopsSoftApOnTimeoutMessage() throws Exception {
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null);
        startSoftApAndVerifyEnabled(apConfig);

        mAlarmManager.dispatch(mSoftApManager.SOFT_AP_SEND_MESSAGE_TIMEOUT_TAG);
        mLooper.dispatchAll();

        verify(mWifiNative).teardownInterface(TEST_INTERFACE_NAME);
    }

    @Test
    public void cancelsTimeoutTimerOnTimeoutToggleChangeWhenNoClients() throws Exception {
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null);
        startSoftApAndVerifyEnabled(apConfig);

        when(mFrameworkFacade.getIntegerSetting(
                mContext, Settings.Global.SOFT_AP_TIMEOUT_ENABLED, 1)).thenReturn(0);
        mContentObserver.onChange(false);
        mLooper.dispatchAll();

        // Verify timer is canceled
        verify(mAlarmManager.getAlarmManager()).cancel(any(WakeupMessage.class));
    }

    @Test
    public void schedulesTimeoutTimerOnTimeoutToggleChangeWhenNoClients() throws Exception {
        // start with timeout toggle disabled
        when(mFrameworkFacade.getIntegerSetting(
                mContext, Settings.Global.SOFT_AP_TIMEOUT_ENABLED, 1)).thenReturn(0);
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null);
        startSoftApAndVerifyEnabled(apConfig);

        when(mFrameworkFacade.getIntegerSetting(
                mContext, Settings.Global.SOFT_AP_TIMEOUT_ENABLED, 1)).thenReturn(1);
        mContentObserver.onChange(false);
        mLooper.dispatchAll();

        // Verify timer is scheduled
        verify(mAlarmManager.getAlarmManager()).setExact(anyInt(), anyLong(),
                eq(mSoftApManager.SOFT_AP_SEND_MESSAGE_TIMEOUT_TAG), any(), any());
    }

    @Test
    public void doesNotScheduleTimeoutTimerOnStartWhenTimeoutIsDisabled() throws Exception {
        // start with timeout toggle disabled
        when(mFrameworkFacade.getIntegerSetting(
                mContext, Settings.Global.SOFT_AP_TIMEOUT_ENABLED, 1)).thenReturn(0);
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null);
        startSoftApAndVerifyEnabled(apConfig);

        // Verify timer is not scheduled
        verify(mAlarmManager.getAlarmManager(), never()).setExact(anyInt(), anyLong(),
                eq(mSoftApManager.SOFT_AP_SEND_MESSAGE_TIMEOUT_TAG), any(), any());
    }

    @Test
    public void doesNotScheduleTimeoutTimerWhenAllClientsDisconnectButTimeoutIsDisabled()
            throws Exception {
        // start with timeout toggle disabled
        when(mFrameworkFacade.getIntegerSetting(
                mContext, Settings.Global.SOFT_AP_TIMEOUT_ENABLED, 1)).thenReturn(0);
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null);
        startSoftApAndVerifyEnabled(apConfig);
        // add some clients
        mSoftApListenerCaptor.getValue().onNumAssociatedStationsChanged(
                TEST_NUM_CONNECTED_CLIENTS);
        mLooper.dispatchAll();
        // remove all clients
        mSoftApListenerCaptor.getValue().onNumAssociatedStationsChanged(0);
        mLooper.dispatchAll();
        // Verify timer is not scheduled
        verify(mAlarmManager.getAlarmManager(), never()).setExact(anyInt(), anyLong(),
                eq(mSoftApManager.SOFT_AP_SEND_MESSAGE_TIMEOUT_TAG), any(), any());
    }

    @Test
    public void unregistersSettingsObserverOnStop() throws Exception {
        SoftApModeConfiguration apConfig =
                new SoftApModeConfiguration(WifiManager.IFACE_IP_MODE_TETHERED, null);
        startSoftApAndVerifyEnabled(apConfig);
        mSoftApManager.stop();
        mLooper.dispatchAll();

        verify(mFrameworkFacade).unregisterContentObserver(eq(mContext), eq(mContentObserver));
    }

    /** Starts soft AP and verifies that it is enabled successfully. */
    protected void startSoftApAndVerifyEnabled(
            SoftApModeConfiguration softApConfig) throws Exception {
        WifiConfiguration expectedConfig;
        InOrder order = inOrder(mCallback, mWifiNative);

        when(mWifiNative.setCountryCodeHal(
                TEST_INTERFACE_NAME, TEST_COUNTRY_CODE.toUpperCase(Locale.ROOT)))
                .thenReturn(true);

        mSoftApManager = createSoftApManager(softApConfig);
        WifiConfiguration config = softApConfig.getWifiConfiguration();
        if (config == null) {
            when(mWifiApConfigStore.getApConfiguration()).thenReturn(mDefaultApConfig);
            expectedConfig = new WifiConfiguration(mDefaultApConfig);
        } else {
            expectedConfig = new WifiConfiguration(config);
        }

        ArgumentCaptor<ContentObserver> observerCaptor = ArgumentCaptor.forClass(
                ContentObserver.class);
        ArgumentCaptor<Intent> intentCaptor = ArgumentCaptor.forClass(Intent.class);

        when(mWifiNative.setupInterfaceForSoftApMode(any()))
                .thenReturn(TEST_INTERFACE_NAME);

        mSoftApManager.start();
        mLooper.dispatchAll();
        order.verify(mWifiNative).setupInterfaceForSoftApMode(
                mWifiNativeInterfaceCallbackCaptor.capture());
        ArgumentCaptor<WifiConfiguration> configCaptor =
                ArgumentCaptor.forClass(WifiConfiguration.class);
        order.verify(mCallback).onStateChanged(WifiManager.WIFI_AP_STATE_ENABLING, 0);
        order.verify(mWifiNative).startSoftAp(eq(TEST_INTERFACE_NAME),
                configCaptor.capture(), mSoftApListenerCaptor.capture());
        WifiConfigurationTestUtil.assertConfigurationEqual(expectedConfig, configCaptor.getValue());
        mWifiNativeInterfaceCallbackCaptor.getValue().onUp(TEST_INTERFACE_NAME);
        mLooper.dispatchAll();
        order.verify(mCallback).onStateChanged(WifiManager.WIFI_AP_STATE_ENABLED, 0);
        order.verify(mCallback).onNumClientsChanged(0);
        verify(mContext, times(2)).sendStickyBroadcastAsUser(intentCaptor.capture(),
                                                             eq(UserHandle.ALL));
        List<Intent> capturedIntents = intentCaptor.getAllValues();
        checkApStateChangedBroadcast(capturedIntents.get(0), WIFI_AP_STATE_ENABLING,
                WIFI_AP_STATE_DISABLED, HOTSPOT_NO_ERROR, TEST_INTERFACE_NAME,
                softApConfig.getTargetMode());
        checkApStateChangedBroadcast(capturedIntents.get(1), WIFI_AP_STATE_ENABLED,
                WIFI_AP_STATE_ENABLING, HOTSPOT_NO_ERROR, TEST_INTERFACE_NAME,
                softApConfig.getTargetMode());
        verify(mWifiMetrics).addSoftApUpChangedEvent(true, softApConfig.mTargetMode);
        verify(mFrameworkFacade).registerContentObserver(eq(mContext), any(Uri.class), eq(true),
                observerCaptor.capture());
        mContentObserver = observerCaptor.getValue();
    }

    private void checkApStateChangedBroadcast(Intent intent, int expectedCurrentState,
                                              int expectedPrevState, int expectedErrorCode,
                                              String expectedIfaceName, int expectedMode) {
        int currentState = intent.getIntExtra(EXTRA_WIFI_AP_STATE, WIFI_AP_STATE_DISABLED);
        int prevState = intent.getIntExtra(EXTRA_PREVIOUS_WIFI_AP_STATE, WIFI_AP_STATE_DISABLED);
        int errorCode = intent.getIntExtra(EXTRA_WIFI_AP_FAILURE_REASON, HOTSPOT_NO_ERROR);
        String ifaceName = intent.getStringExtra(EXTRA_WIFI_AP_INTERFACE_NAME);
        int mode = intent.getIntExtra(EXTRA_WIFI_AP_MODE, WifiManager.IFACE_IP_MODE_UNSPECIFIED);
        assertEquals(expectedCurrentState, currentState);
        assertEquals(expectedPrevState, prevState);
        assertEquals(expectedErrorCode, errorCode);
        assertEquals(expectedIfaceName, ifaceName);
        assertEquals(expectedMode, mode);
    }
}
