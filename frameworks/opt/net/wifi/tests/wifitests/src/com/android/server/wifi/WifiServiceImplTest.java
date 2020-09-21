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

import static android.net.wifi.WifiManager.HOTSPOT_FAILED;
import static android.net.wifi.WifiManager.HOTSPOT_STARTED;
import static android.net.wifi.WifiManager.HOTSPOT_STOPPED;
import static android.net.wifi.WifiManager.IFACE_IP_MODE_CONFIGURATION_ERROR;
import static android.net.wifi.WifiManager.IFACE_IP_MODE_LOCAL_ONLY;
import static android.net.wifi.WifiManager.IFACE_IP_MODE_TETHERED;
import static android.net.wifi.WifiManager.LocalOnlyHotspotCallback.ERROR_GENERIC;
import static android.net.wifi.WifiManager.LocalOnlyHotspotCallback.ERROR_INCOMPATIBLE_MODE;
import static android.net.wifi.WifiManager.LocalOnlyHotspotCallback.ERROR_NO_CHANNEL;
import static android.net.wifi.WifiManager.LocalOnlyHotspotCallback.ERROR_TETHERING_DISALLOWED;
import static android.net.wifi.WifiManager.SAP_START_FAILURE_GENERAL;
import static android.net.wifi.WifiManager.SAP_START_FAILURE_NO_CHANNEL;
import static android.net.wifi.WifiManager.WIFI_AP_STATE_DISABLED;
import static android.net.wifi.WifiManager.WIFI_AP_STATE_DISABLING;
import static android.net.wifi.WifiManager.WIFI_AP_STATE_ENABLED;
import static android.net.wifi.WifiManager.WIFI_AP_STATE_ENABLING;
import static android.net.wifi.WifiManager.WIFI_AP_STATE_FAILED;
import static android.net.wifi.WifiManager.WIFI_STATE_DISABLED;
import static android.net.wifi.WifiManager.WIFI_STATE_ENABLED;
import static android.provider.Settings.Secure.LOCATION_MODE_HIGH_ACCURACY;
import static android.provider.Settings.Secure.LOCATION_MODE_OFF;

import static com.android.server.wifi.LocalOnlyHotspotRequestInfo.HOTSPOT_NO_ERROR;
import static com.android.server.wifi.WifiController.CMD_SET_AP;
import static com.android.server.wifi.WifiController.CMD_WIFI_TOGGLED;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anyString;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.*;
import static org.mockito.Mockito.atLeastOnce;

import android.app.ActivityManager;
import android.app.AppOpsManager;
import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.net.Uri;
import android.net.wifi.ISoftApCallback;
import android.net.wifi.ScanResult;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiConfiguration.KeyMgmt;
import android.net.wifi.WifiEnterpriseConfig;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiManager.LocalOnlyHotspotCallback;
import android.net.wifi.WifiManager.SoftApCallback;
import android.net.wifi.WifiSsid;
import android.net.wifi.hotspot2.IProvisioningCallback;
import android.net.wifi.hotspot2.OsuProvider;
import android.net.wifi.hotspot2.PasspointConfiguration;
import android.os.Binder;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.IBinder;
import android.os.IPowerManager;
import android.os.Looper;
import android.os.Message;
import android.os.Messenger;
import android.os.PowerManager;
import android.os.Process;
import android.os.RemoteException;
import android.os.UserManager;
import android.os.test.TestLooper;
import android.support.test.filters.SmallTest;

import com.android.internal.os.PowerProfile;
import com.android.internal.telephony.TelephonyIntents;
import com.android.internal.util.AsyncChannel;
import com.android.server.wifi.WifiServiceImpl.LocalOnlyRequestorCallback;
import com.android.server.wifi.hotspot2.PasspointProvisioningTestUtil;
import com.android.server.wifi.util.WifiAsyncChannel;
import com.android.server.wifi.util.WifiPermissionsUtil;
import com.android.server.wifi.util.WifiPermissionsWrapper;

import org.junit.Before;
import org.junit.Ignore;
import org.junit.Test;
import org.mockito.ArgumentCaptor;
import org.mockito.ArgumentMatcher;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.mockito.Spy;

import java.io.FileDescriptor;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * Unit tests for {@link WifiServiceImpl}.
 *
 * Note: this is intended to build up over time and will not immediately cover the entire file.
 */
@SmallTest
public class WifiServiceImplTest {

    private static final String TAG = "WifiServiceImplTest";
    private static final String SCAN_PACKAGE_NAME = "scanPackage";
    private static final int DEFAULT_VERBOSE_LOGGING = 0;
    private static final String ANDROID_SYSTEM_PACKAGE = "android";
    private static final String TEST_PACKAGE_NAME = "TestPackage";
    private static final String SYSUI_PACKAGE_NAME = "com.android.systemui";
    private static final int TEST_PID = 6789;
    private static final int TEST_PID2 = 9876;
    private static final int TEST_UID = 1200000;
    private static final int OTHER_TEST_UID = 1300000;
    private static final int TEST_USER_HANDLE = 13;
    private static final String WIFI_IFACE_NAME = "wlan0";
    private static final String TEST_COUNTRY_CODE = "US";

    private WifiServiceImpl mWifiServiceImpl;
    private TestLooper mLooper;
    private PowerManager mPowerManager;
    private Handler mHandler;
    private Handler mHandlerSpyForWsmRunWithScissors;
    private Messenger mAppMessenger;
    private int mPid;
    private int mPid2 = Process.myPid();
    private OsuProvider mOsuProvider;
    private SoftApCallback mStateMachineSoftApCallback;

    final ArgumentCaptor<BroadcastReceiver> mBroadcastReceiverCaptor =
            ArgumentCaptor.forClass(BroadcastReceiver.class);
    final ArgumentCaptor<IntentFilter> mIntentFilterCaptor =
            ArgumentCaptor.forClass(IntentFilter.class);

    final ArgumentCaptor<Message> mMessageCaptor = ArgumentCaptor.forClass(Message.class);
    final ArgumentCaptor<SoftApModeConfiguration> mSoftApModeConfigCaptor =
            ArgumentCaptor.forClass(SoftApModeConfiguration.class);

    @Mock Context mContext;
    @Mock WifiInjector mWifiInjector;
    @Mock WifiCountryCode mWifiCountryCode;
    @Mock Clock mClock;
    @Mock WifiController mWifiController;
    @Mock WifiTrafficPoller mWifiTrafficPoller;
    @Mock WifiStateMachine mWifiStateMachine;
    @Mock WifiStateMachinePrime mWifiStateMachinePrime;
    @Mock HandlerThread mHandlerThread;
    @Mock AsyncChannel mAsyncChannel;
    @Mock Resources mResources;
    @Mock ApplicationInfo mApplicationInfo;
    @Mock FrameworkFacade mFrameworkFacade;
    @Mock WifiLockManager mLockManager;
    @Mock WifiMulticastLockManager mWifiMulticastLockManager;
    @Mock WifiLastResortWatchdog mWifiLastResortWatchdog;
    @Mock WifiBackupRestore mWifiBackupRestore;
    @Mock WifiMetrics mWifiMetrics;
    @Mock WifiPermissionsUtil mWifiPermissionsUtil;
    @Mock WifiPermissionsWrapper mWifiPermissionsWrapper;
    @Mock WifiSettingsStore mSettingsStore;
    @Mock ContentResolver mContentResolver;
    @Mock PackageManager mPackageManager;
    @Mock UserManager mUserManager;
    @Mock WifiApConfigStore mWifiApConfigStore;
    @Mock WifiConfiguration mApConfig;
    @Mock ActivityManager mActivityManager;
    @Mock AppOpsManager mAppOpsManager;
    @Mock IBinder mAppBinder;
    @Mock LocalOnlyHotspotRequestInfo mRequestInfo;
    @Mock LocalOnlyHotspotRequestInfo mRequestInfo2;
    @Mock IProvisioningCallback mProvisioningCallback;
    @Mock ISoftApCallback mClientSoftApCallback;
    @Mock ISoftApCallback mAnotherSoftApCallback;
    @Mock PowerProfile mPowerProfile;
    @Mock WifiTrafficPoller mWifiTrafficPolller;
    @Mock ScanRequestProxy mScanRequestProxy;

    @Spy FakeWifiLog mLog;

    private class WifiAsyncChannelTester {
        private static final String TAG = "WifiAsyncChannelTester";
        public static final int CHANNEL_STATE_FAILURE = -1;
        public static final int CHANNEL_STATE_DISCONNECTED = 0;
        public static final int CHANNEL_STATE_HALF_CONNECTED = 1;
        public static final int CHANNEL_STATE_FULLY_CONNECTED = 2;

        private int mState = CHANNEL_STATE_DISCONNECTED;
        private WifiAsyncChannel mChannel;
        private WifiLog mAsyncTestLog;

        WifiAsyncChannelTester(WifiInjector wifiInjector) {
            mAsyncTestLog = wifiInjector.makeLog(TAG);
        }

        public int getChannelState() {
            return mState;
        }

        public void connect(final Looper looper, final Messenger messenger,
                final Handler incomingMessageHandler) {
            assertEquals("AsyncChannel must be in disconnected state",
                    CHANNEL_STATE_DISCONNECTED, mState);
            mChannel = new WifiAsyncChannel(TAG);
            mChannel.setWifiLog(mLog);
            Handler handler = new Handler(mLooper.getLooper()) {
                @Override
                public void handleMessage(Message msg) {
                    switch (msg.what) {
                        case AsyncChannel.CMD_CHANNEL_HALF_CONNECTED:
                            if (msg.arg1 == AsyncChannel.STATUS_SUCCESSFUL) {
                                mChannel.sendMessage(AsyncChannel.CMD_CHANNEL_FULL_CONNECTION);
                                mState = CHANNEL_STATE_HALF_CONNECTED;
                            } else {
                                mState = CHANNEL_STATE_FAILURE;
                            }
                            break;
                        case AsyncChannel.CMD_CHANNEL_FULLY_CONNECTED:
                            mState = CHANNEL_STATE_FULLY_CONNECTED;
                            break;
                        case AsyncChannel.CMD_CHANNEL_DISCONNECTED:
                            mState = CHANNEL_STATE_DISCONNECTED;
                            break;
                        default:
                            incomingMessageHandler.handleMessage(msg);
                            break;
                    }
                }
            };
            mChannel.connect(null, handler, messenger);
        }

        private Message sendMessageSynchronously(Message request) {
            return mChannel.sendMessageSynchronously(request);
        }

        private void sendMessage(Message request) {
            mChannel.sendMessage(request);
        }
    }

    @Before public void setUp() {
        MockitoAnnotations.initMocks(this);
        mLooper = new TestLooper();
        mHandler = spy(new Handler(mLooper.getLooper()));
        mAppMessenger = new Messenger(mHandler);

        when(mRequestInfo.getPid()).thenReturn(mPid);
        when(mRequestInfo2.getPid()).thenReturn(mPid2);
        when(mWifiInjector.getUserManager()).thenReturn(mUserManager);
        when(mWifiInjector.getWifiCountryCode()).thenReturn(mWifiCountryCode);
        when(mWifiInjector.getWifiController()).thenReturn(mWifiController);
        when(mWifiInjector.getWifiMetrics()).thenReturn(mWifiMetrics);
        when(mWifiInjector.getWifiStateMachine()).thenReturn(mWifiStateMachine);
        when(mWifiStateMachine.syncInitialize(any())).thenReturn(true);
        when(mWifiInjector.getWifiStateMachinePrime()).thenReturn(mWifiStateMachinePrime);
        when(mWifiInjector.getWifiServiceHandlerThread()).thenReturn(mHandlerThread);
        when(mWifiInjector.getPowerProfile()).thenReturn(mPowerProfile);
        when(mHandlerThread.getLooper()).thenReturn(mLooper.getLooper());
        when(mContext.getResources()).thenReturn(mResources);
        when(mContext.getContentResolver()).thenReturn(mContentResolver);
        when(mContext.getPackageManager()).thenReturn(mPackageManager);
        when(mWifiInjector.getWifiApConfigStore()).thenReturn(mWifiApConfigStore);
        doNothing().when(mFrameworkFacade).registerContentObserver(eq(mContext), any(),
                anyBoolean(), any());
        when(mContext.getSystemService(Context.ACTIVITY_SERVICE)).thenReturn(mActivityManager);
        when(mContext.getSystemService(Context.APP_OPS_SERVICE)).thenReturn(mAppOpsManager);
        IPowerManager powerManagerService = mock(IPowerManager.class);
        mPowerManager = new PowerManager(mContext, powerManagerService, new Handler());
        when(mContext.getSystemServiceName(PowerManager.class)).thenReturn(Context.POWER_SERVICE);
        when(mContext.getSystemService(PowerManager.class)).thenReturn(mPowerManager);
        WifiAsyncChannel wifiAsyncChannel = new WifiAsyncChannel("WifiServiceImplTest");
        wifiAsyncChannel.setWifiLog(mLog);
        when(mFrameworkFacade.makeWifiAsyncChannel(anyString())).thenReturn(wifiAsyncChannel);
        when(mWifiInjector.getFrameworkFacade()).thenReturn(mFrameworkFacade);
        when(mWifiInjector.getWifiLockManager()).thenReturn(mLockManager);
        when(mWifiInjector.getWifiMulticastLockManager()).thenReturn(mWifiMulticastLockManager);
        when(mWifiInjector.getWifiLastResortWatchdog()).thenReturn(mWifiLastResortWatchdog);
        when(mWifiInjector.getWifiBackupRestore()).thenReturn(mWifiBackupRestore);
        when(mWifiInjector.makeLog(anyString())).thenReturn(mLog);
        when(mWifiInjector.getWifiTrafficPoller()).thenReturn(mWifiTrafficPoller);
        when(mWifiInjector.getWifiPermissionsUtil()).thenReturn(mWifiPermissionsUtil);
        when(mWifiInjector.getWifiPermissionsWrapper()).thenReturn(mWifiPermissionsWrapper);
        when(mWifiInjector.getWifiSettingsStore()).thenReturn(mSettingsStore);
        when(mWifiInjector.getClock()).thenReturn(mClock);
        when(mWifiInjector.getScanRequestProxy()).thenReturn(mScanRequestProxy);
        when(mWifiStateMachine.syncStartSubscriptionProvisioning(anyInt(),
                any(OsuProvider.class), any(IProvisioningCallback.class), any())).thenReturn(true);
        when(mPackageManager.hasSystemFeature(
                PackageManager.FEATURE_WIFI_PASSPOINT)).thenReturn(true);
        // Create an OSU provider that can be provisioned via an open OSU AP
        mOsuProvider = PasspointProvisioningTestUtil.generateOsuProvider(true);
        when(mContext.getOpPackageName()).thenReturn(TEST_PACKAGE_NAME);
        when(mContext.checkPermission(eq(android.Manifest.permission.NETWORK_SETTINGS),
                anyInt(), anyInt())).thenReturn(PackageManager.PERMISSION_DENIED);
        when(mScanRequestProxy.startScan(anyInt(), anyString())).thenReturn(true);

        ArgumentCaptor<SoftApCallback> softApCallbackCaptor =
                ArgumentCaptor.forClass(SoftApCallback.class);
        mWifiServiceImpl = new WifiServiceImpl(mContext, mWifiInjector, mAsyncChannel);
        verify(mWifiStateMachinePrime).registerSoftApCallback(softApCallbackCaptor.capture());
        mStateMachineSoftApCallback = softApCallbackCaptor.getValue();
        mWifiServiceImpl.setWifiHandlerLogForTest(mLog);
    }

    private WifiAsyncChannelTester verifyAsyncChannelHalfConnected() throws RemoteException {
        WifiAsyncChannelTester channelTester = new WifiAsyncChannelTester(mWifiInjector);
        Handler handler = mock(Handler.class);
        TestLooper looper = new TestLooper();
        channelTester.connect(looper.getLooper(),
                mWifiServiceImpl.getWifiServiceMessenger(TEST_PACKAGE_NAME), handler);
        mLooper.dispatchAll();
        assertEquals("AsyncChannel must be half connected",
                WifiAsyncChannelTester.CHANNEL_STATE_HALF_CONNECTED,
                channelTester.getChannelState());
        return channelTester;
    }

    /**
     * Verifies that any operations on WifiServiceImpl without setting up the WifiStateMachine
     * channel would fail.
     */
    @Test
    public void testRemoveNetworkUnknown() {
        assertFalse(mWifiServiceImpl.removeNetwork(-1, TEST_PACKAGE_NAME));
        verify(mWifiStateMachine, never()).syncRemoveNetwork(any(), anyInt());
    }

    /**
     * Tests whether we're able to set up an async channel connection with WifiServiceImpl.
     * This is the path used by some WifiManager public API calls.
     */
    @Test
    public void testAsyncChannelHalfConnected() throws RemoteException {
        verifyAsyncChannelHalfConnected();
    }

    /**
     * Ensure WifiMetrics.dump() is the only dump called when 'dumpsys wifi WifiMetricsProto' is
     * called. This is required to support simple metrics collection via dumpsys
     */
    @Test
    public void testWifiMetricsDump() {
        mWifiServiceImpl.dump(new FileDescriptor(), new PrintWriter(new StringWriter()),
                new String[]{mWifiMetrics.PROTO_DUMP_ARG});
        verify(mWifiMetrics)
                .dump(any(FileDescriptor.class), any(PrintWriter.class), any(String[].class));
        verify(mWifiStateMachine, never())
                .dump(any(FileDescriptor.class), any(PrintWriter.class), any(String[].class));
    }

    /**
     * Ensure WifiServiceImpl.dump() doesn't throw an NPE when executed with null args
     */
    @Test
    public void testDumpNullArgs() {
        mWifiServiceImpl.dump(new FileDescriptor(), new PrintWriter(new StringWriter()), null);
    }

    /**
     * Verify that wifi can be enabled by a caller with WIFI_STATE_CHANGE permission when wifi is
     * off (no hotspot, no airplane mode).
     *
     * Note: hotspot is disabled by default
     */
    @Test
    public void testSetWifiEnabledSuccess() throws Exception {
        when(mSettingsStore.handleWifiToggled(eq(true))).thenReturn(true);
        when(mSettingsStore.isAirplaneModeOn()).thenReturn(false);
        assertTrue(mWifiServiceImpl.setWifiEnabled(TEST_PACKAGE_NAME, true));
        verify(mWifiController).sendMessage(eq(CMD_WIFI_TOGGLED));
    }

    /**
     * Verify that the CMD_TOGGLE_WIFI message won't be sent if wifi is already on.
     */
    @Test
    public void testSetWifiEnabledNoToggle() throws Exception {
        when(mSettingsStore.handleWifiToggled(eq(true))).thenReturn(false);
        assertTrue(mWifiServiceImpl.setWifiEnabled(TEST_PACKAGE_NAME, true));
        verify(mWifiController, never()).sendMessage(eq(CMD_WIFI_TOGGLED));
    }

    /**
     * Verify a SecurityException is thrown if a caller does not have the correct permission to
     * toggle wifi.
     */
    @Test
    public void testSetWifiEnableWithoutPermission() throws Exception {
        doThrow(new SecurityException()).when(mContext)
                .enforceCallingOrSelfPermission(eq(android.Manifest.permission.CHANGE_WIFI_STATE),
                                                eq("WifiService"));
        when(mSettingsStore.isAirplaneModeOn()).thenReturn(false);
        try {
            mWifiServiceImpl.setWifiEnabled(TEST_PACKAGE_NAME, true);
            fail();
        } catch (SecurityException e) {

        }

    }

    /**
     * Verify a SecurityException is thrown if OPSTR_CHANGE_WIFI_STATE is disabled for the app.
     */
    @Test
    public void testSetWifiEnableAppOpsRejected() throws Exception {
        when(mSettingsStore.handleWifiToggled(eq(true))).thenReturn(true);
        doThrow(new SecurityException()).when(mAppOpsManager)
                .noteOp(AppOpsManager.OPSTR_CHANGE_WIFI_STATE, Process.myUid(), TEST_PACKAGE_NAME);
        when(mSettingsStore.isAirplaneModeOn()).thenReturn(false);
        try {
            mWifiServiceImpl.setWifiEnabled(TEST_PACKAGE_NAME, true);
            fail();
        } catch (SecurityException e) {

        }
        verify(mWifiController, never()).sendMessage(eq(CMD_WIFI_TOGGLED));
    }

    /**
     * Verify a SecurityException is thrown if OP_CHANGE_WIFI_STATE is set to MODE_IGNORED
     * for the app.
     */
    @Test // No exception expected, but the operation should not be done
    public void testSetWifiEnableAppOpsIgnored() throws Exception {
        when(mSettingsStore.handleWifiToggled(eq(true))).thenReturn(true);
        doReturn(AppOpsManager.MODE_IGNORED).when(mAppOpsManager)
                .noteOp(AppOpsManager.OPSTR_CHANGE_WIFI_STATE, Process.myUid(), TEST_PACKAGE_NAME);

        when(mSettingsStore.isAirplaneModeOn()).thenReturn(false);
        mWifiServiceImpl.setWifiEnabled(TEST_PACKAGE_NAME, true);
        verify(mWifiController, never()).sendMessage(eq(CMD_WIFI_TOGGLED));
    }

    /**
     * Verify that a call from an app with the NETWORK_SETTINGS permission can enable wifi if we
     * are in airplane mode.
     */
    @Test
    public void testSetWifiEnabledFromNetworkSettingsHolderWhenInAirplaneMode() throws Exception {
        when(mSettingsStore.handleWifiToggled(eq(true))).thenReturn(true);
        when(mSettingsStore.isAirplaneModeOn()).thenReturn(true);
        when(mContext.checkPermission(
                eq(android.Manifest.permission.NETWORK_SETTINGS), anyInt(), anyInt()))
                        .thenReturn(PackageManager.PERMISSION_GRANTED);
        assertTrue(mWifiServiceImpl.setWifiEnabled(SYSUI_PACKAGE_NAME, true));
        verify(mWifiController).sendMessage(eq(CMD_WIFI_TOGGLED));
    }

    /**
     * Verify that a caller without the NETWORK_SETTINGS permission can't enable wifi
     * if we are in airplane mode.
     */
    @Test
    public void testSetWifiEnabledFromAppFailsWhenInAirplaneMode() throws Exception {
        when(mSettingsStore.handleWifiToggled(eq(true))).thenReturn(true);
        when(mSettingsStore.isAirplaneModeOn()).thenReturn(true);
        when(mContext.checkPermission(
                eq(android.Manifest.permission.NETWORK_SETTINGS), anyInt(), anyInt()))
                        .thenReturn(PackageManager.PERMISSION_DENIED);
        assertFalse(mWifiServiceImpl.setWifiEnabled(TEST_PACKAGE_NAME, true));
        verify(mWifiController, never()).sendMessage(eq(CMD_WIFI_TOGGLED));
    }

    /**
     * Verify that a call from an app with the NETWORK_SETTINGS permission can enable wifi if we
     * are in softap mode.
     */
    @Test
    public void testSetWifiEnabledFromNetworkSettingsHolderWhenApEnabled() throws Exception {
        when(mFrameworkFacade.inStorageManagerCryptKeeperBounce()).thenReturn(false);
        when(mSettingsStore.isWifiToggleEnabled()).thenReturn(false);
        mWifiServiceImpl.checkAndStartWifi();

        verify(mContext).registerReceiver(mBroadcastReceiverCaptor.capture(),
                (IntentFilter) argThat(new IntentFilterMatcher()));

        TestUtil.sendWifiApStateChanged(mBroadcastReceiverCaptor.getValue(), mContext,
                WIFI_AP_STATE_ENABLED, WIFI_AP_STATE_ENABLING, SAP_START_FAILURE_GENERAL,
                WIFI_IFACE_NAME, IFACE_IP_MODE_LOCAL_ONLY);

        when(mSettingsStore.handleWifiToggled(eq(true))).thenReturn(true);
        when(mContext.checkPermission(
                eq(android.Manifest.permission.NETWORK_SETTINGS), anyInt(), anyInt()))
                        .thenReturn(PackageManager.PERMISSION_GRANTED);
        when(mSettingsStore.isAirplaneModeOn()).thenReturn(false);
        assertTrue(mWifiServiceImpl.setWifiEnabled(SYSUI_PACKAGE_NAME, true));
        verify(mWifiController).sendMessage(eq(CMD_WIFI_TOGGLED));
    }

    /**
     * Verify that a call from an app cannot enable wifi if we are in softap mode.
     */
    @Test
    public void testSetWifiEnabledFromAppFailsWhenApEnabled() throws Exception {
        when(mFrameworkFacade.inStorageManagerCryptKeeperBounce()).thenReturn(false);
        when(mSettingsStore.isWifiToggleEnabled()).thenReturn(false);
        mWifiServiceImpl.checkAndStartWifi();

        verify(mContext).registerReceiver(mBroadcastReceiverCaptor.capture(),
                (IntentFilter) argThat(new IntentFilterMatcher()));

        TestUtil.sendWifiApStateChanged(mBroadcastReceiverCaptor.getValue(), mContext,
                WIFI_AP_STATE_ENABLED, WIFI_AP_STATE_ENABLING, SAP_START_FAILURE_GENERAL,
                WIFI_IFACE_NAME, IFACE_IP_MODE_LOCAL_ONLY);

        when(mContext.checkPermission(
                eq(android.Manifest.permission.NETWORK_SETTINGS), anyInt(), anyInt()))
                        .thenReturn(PackageManager.PERMISSION_DENIED);
        when(mSettingsStore.isAirplaneModeOn()).thenReturn(false);
        assertFalse(mWifiServiceImpl.setWifiEnabled(TEST_PACKAGE_NAME, true));
        verify(mSettingsStore, never()).handleWifiToggled(anyBoolean());
        verify(mWifiController, never()).sendMessage(eq(CMD_WIFI_TOGGLED));
    }

    /**
     * Verify that wifi can be enabled without consent UI popup when permission
     * review is required but got permission granted.
     */
    @Test
    public void testSetWifiEnabledSuccessWhenPermissionReviewRequiredAndPermissionGranted()
            throws Exception {
        // Set PermissionReviewRequired to true explicitly
        when(mResources.getBoolean(anyInt())).thenReturn(true);
        mWifiServiceImpl = new WifiServiceImpl(mContext, mWifiInjector, mAsyncChannel);
        when(mSettingsStore.handleWifiToggled(eq(true))).thenReturn(true);
        when(mSettingsStore.isAirplaneModeOn()).thenReturn(false);
        when(mContext.checkCallingPermission(
                eq(android.Manifest.permission.MANAGE_WIFI_WHEN_PERMISSION_REVIEW_REQUIRED)))
                        .thenReturn(PackageManager.PERMISSION_GRANTED);

        assertTrue(mWifiServiceImpl.setWifiEnabled(TEST_PACKAGE_NAME, true));
        verify(mWifiController).sendMessage(eq(CMD_WIFI_TOGGLED));
    }

    /**
     * Verify that wifi is not enabled but got consent UI popup when permission
     * review is required but do not have permission.
     */
    @Test
    public void testSetWifiEnabledConsentUiWhenPermissionReviewRequiredAndPermissionDenied()
            throws Exception {
        // Set PermissionReviewRequired to true explicitly
        when(mResources.getBoolean(anyInt())).thenReturn(true);
        mWifiServiceImpl = new WifiServiceImpl(mContext, mWifiInjector, mAsyncChannel);
        when(mSettingsStore.handleWifiToggled(eq(true))).thenReturn(true);
        when(mSettingsStore.isAirplaneModeOn()).thenReturn(false);
        when(mContext.checkCallingPermission(
                eq(android.Manifest.permission.MANAGE_WIFI_WHEN_PERMISSION_REVIEW_REQUIRED)))
                        .thenReturn(PackageManager.PERMISSION_DENIED);
        when(mPackageManager.getApplicationInfoAsUser(
                anyString(), anyInt(), anyInt()))
                        .thenReturn(mApplicationInfo);
        mApplicationInfo.uid = TEST_UID;
        int uid = Binder.getCallingUid();
        BinderUtil.setUid(TEST_UID);

        try {
            assertTrue(mWifiServiceImpl.setWifiEnabled(TEST_PACKAGE_NAME, true));
        } finally {
            BinderUtil.setUid(uid);
        }

        verify(mContext).startActivity(any());
        verify(mWifiController, never()).sendMessage(eq(CMD_WIFI_TOGGLED));
    }

    /**
     * Verify that wifi can be enabled when wifi is off and permission review is
     * not required.
     */
    @Test
    public void testSetWifiEnabledSuccessWhenPermissionReviewNotRequired() throws Exception {
        // Set PermissionReviewRequired to false explicitly
        when(mResources.getBoolean(anyInt())).thenReturn(false);
        mWifiServiceImpl = new WifiServiceImpl(mContext, mWifiInjector, mAsyncChannel);
        when(mSettingsStore.handleWifiToggled(eq(true))).thenReturn(true);
        when(mSettingsStore.isAirplaneModeOn()).thenReturn(false);

        assertTrue(mWifiServiceImpl.setWifiEnabled(TEST_PACKAGE_NAME, true));
        verify(mWifiController).sendMessage(eq(CMD_WIFI_TOGGLED));
    }

    /**
     * Verify a SecurityException is thrown when bringing up Consent UI but caller
     * uid does not match application uid.
     *
     * @throws SecurityException
     */
    @Test
    public void testSetWifiEnabledThrowsSecurityExceptionForConsentUiIfUidNotMatch()
            throws Exception {
        // Set PermissionReviewRequired to true explicitly
        when(mResources.getBoolean(anyInt())).thenReturn(true);
        mWifiServiceImpl = new WifiServiceImpl(mContext, mWifiInjector, mAsyncChannel);
        when(mSettingsStore.handleWifiToggled(eq(true))).thenReturn(true);
        when(mSettingsStore.isAirplaneModeOn()).thenReturn(false);
        when(mContext.checkCallingPermission(
                eq(android.Manifest.permission.MANAGE_WIFI_WHEN_PERMISSION_REVIEW_REQUIRED)))
                        .thenReturn(PackageManager.PERMISSION_DENIED);
        when(mPackageManager.getApplicationInfoAsUser(
                anyString(), anyInt(), anyInt()))
                        .thenReturn(mApplicationInfo);
        mApplicationInfo.uid = TEST_UID;
        int uid = Binder.getCallingUid();
        BinderUtil.setUid(OTHER_TEST_UID);

        try {
            mWifiServiceImpl.setWifiEnabled(TEST_PACKAGE_NAME, true);
            fail();
        } catch (SecurityException e) {
        } finally {
            BinderUtil.setUid(uid);
        }
    }

    /**
     * Verify that wifi can be disabled by a caller with WIFI_STATE_CHANGE permission when wifi is
     * on.
     */
    @Test
    public void testSetWifiDisabledSuccess() throws Exception {
        when(mSettingsStore.handleWifiToggled(eq(false))).thenReturn(true);
        assertTrue(mWifiServiceImpl.setWifiEnabled(TEST_PACKAGE_NAME, false));
        verifyCheckChangePermission(TEST_PACKAGE_NAME);
        verify(mWifiController).sendMessage(eq(CMD_WIFI_TOGGLED));
    }

    /**
     * Verify that CMD_TOGGLE_WIFI message won't be sent if wifi is already off.
     */
    @Test
    public void testSetWifiDisabledNoToggle() throws Exception {
        when(mSettingsStore.handleWifiToggled(eq(false))).thenReturn(false);
        assertTrue(mWifiServiceImpl.setWifiEnabled(TEST_PACKAGE_NAME, false));
        verify(mWifiController, never()).sendMessage(eq(CMD_WIFI_TOGGLED));
    }

    /**
     * Verify a SecurityException is thrown if a caller does not have the correct permission to
     * toggle wifi.
     */
    @Test
    public void testSetWifiDisabledWithoutPermission() throws Exception {
        doThrow(new SecurityException()).when(mContext)
                .enforceCallingOrSelfPermission(eq(android.Manifest.permission.CHANGE_WIFI_STATE),
                                                eq("WifiService"));
        try {
            mWifiServiceImpl.setWifiEnabled(TEST_PACKAGE_NAME, false);
            fail();
        } catch (SecurityException e) { }
    }

    /**
     * Verify that wifi can be disabled without consent UI popup when permission
     * review is required but got permission granted.
     */
    @Test
    public void testSetWifiDisabledSuccessWhenPermissionReviewRequiredAndPermissionGranted()
            throws Exception {
        // Set PermissionReviewRequired to true explicitly
        when(mResources.getBoolean(anyInt())).thenReturn(true);
        mWifiServiceImpl = new WifiServiceImpl(mContext, mWifiInjector, mAsyncChannel);
        when(mSettingsStore.handleWifiToggled(eq(false))).thenReturn(true);
        when(mSettingsStore.isAirplaneModeOn()).thenReturn(false);
        when(mWifiStateMachine.syncGetWifiState()).thenReturn(WIFI_STATE_ENABLED);
        when(mContext.checkCallingPermission(
                eq(android.Manifest.permission.MANAGE_WIFI_WHEN_PERMISSION_REVIEW_REQUIRED)))
                        .thenReturn(PackageManager.PERMISSION_GRANTED);

        assertTrue(mWifiServiceImpl.setWifiEnabled(TEST_PACKAGE_NAME, false));
        verify(mWifiController).sendMessage(eq(CMD_WIFI_TOGGLED));
    }

    /**
     * Verify that wifi is not disabled but got consent UI popup when permission
     * review is required but do not have permission.
     */
    @Test
    public void testSetWifiDisabledConsentUiWhenPermissionReviewRequiredAndPermissionDenied()
            throws Exception {
        // Set PermissionReviewRequired to true explicitly
        when(mResources.getBoolean(anyInt())).thenReturn(true);
        mWifiServiceImpl = new WifiServiceImpl(mContext, mWifiInjector, mAsyncChannel);
        when(mSettingsStore.handleWifiToggled(eq(false))).thenReturn(true);
        when(mSettingsStore.isAirplaneModeOn()).thenReturn(false);
        when(mWifiStateMachine.syncGetWifiState()).thenReturn(WIFI_STATE_ENABLED);
        when(mContext.checkCallingPermission(
                eq(android.Manifest.permission.MANAGE_WIFI_WHEN_PERMISSION_REVIEW_REQUIRED)))
                        .thenReturn(PackageManager.PERMISSION_DENIED);
        when(mPackageManager.getApplicationInfoAsUser(
                anyString(), anyInt(), anyInt()))
                        .thenReturn(mApplicationInfo);
        mApplicationInfo.uid = TEST_UID;
        int uid = Binder.getCallingUid();
        BinderUtil.setUid(TEST_UID);

        try {
            assertTrue(mWifiServiceImpl.setWifiEnabled(TEST_PACKAGE_NAME, false));
        } finally {
            BinderUtil.setUid(uid);
        }
        verify(mContext).startActivity(any());
        verify(mWifiController, never()).sendMessage(eq(CMD_WIFI_TOGGLED));
    }

    /**
     * Verify that wifi can be disabled when wifi is on and permission review is
     * not required.
     */
    @Test
    public void testSetWifiDisabledSuccessWhenPermissionReviewNotRequired() throws Exception {
        // Set PermissionReviewRequired to false explicitly
        when(mResources.getBoolean(anyInt())).thenReturn(false);
        mWifiServiceImpl = new WifiServiceImpl(mContext, mWifiInjector, mAsyncChannel);
        when(mSettingsStore.handleWifiToggled(eq(false))).thenReturn(true);
        when(mSettingsStore.isAirplaneModeOn()).thenReturn(false);
        when(mWifiStateMachine.syncGetWifiState()).thenReturn(WIFI_STATE_ENABLED);
        when(mContext.checkCallingPermission(
                eq(android.Manifest.permission.MANAGE_WIFI_WHEN_PERMISSION_REVIEW_REQUIRED)))
                        .thenReturn(PackageManager.PERMISSION_GRANTED);

        assertTrue(mWifiServiceImpl.setWifiEnabled(TEST_PACKAGE_NAME, false));
        verify(mWifiController).sendMessage(eq(CMD_WIFI_TOGGLED));
    }

    /**
     * Verify a SecurityException is thrown when bringing up Consent UI but caller
     * uid does not match application uid.
     *
     * @throws SecurityException
     */
    @Test
    public void testSetWifiDisabledThrowsSecurityExceptionForConsentUiIfUidNotMatch()
            throws Exception {
        // Set PermissionReviewRequired to true explicitly
        when(mResources.getBoolean(anyInt())).thenReturn(true);
        mWifiServiceImpl = new WifiServiceImpl(mContext, mWifiInjector, mAsyncChannel);
        when(mSettingsStore.handleWifiToggled(eq(false))).thenReturn(true);
        when(mSettingsStore.isAirplaneModeOn()).thenReturn(false);
        when(mWifiStateMachine.syncGetWifiState()).thenReturn(WIFI_STATE_ENABLED);
        when(mContext.checkCallingPermission(
                eq(android.Manifest.permission.MANAGE_WIFI_WHEN_PERMISSION_REVIEW_REQUIRED)))
                        .thenReturn(PackageManager.PERMISSION_DENIED);
        when(mPackageManager.getApplicationInfoAsUser(
                anyString(), anyInt(), anyInt()))
                        .thenReturn(mApplicationInfo);
        mApplicationInfo.uid = TEST_UID;
        int uid = Binder.getCallingUid();
        BinderUtil.setUid(OTHER_TEST_UID);

        try {
            mWifiServiceImpl.setWifiEnabled(TEST_PACKAGE_NAME, false);
            fail();
        } catch (SecurityException e) {

        } finally {
            // reset Binder uid so we do not mess up other tests
            BinderUtil.setUid(uid);
        }
    }

    /**
     * Ensure unpermitted callers cannot write the SoftApConfiguration.
     *
     * @throws SecurityException
     */
    @Test
    public void testSetWifiApConfigurationNotSavedWithoutPermission() {
        when(mWifiPermissionsUtil.checkConfigOverridePermission(anyInt())).thenReturn(false);
        WifiConfiguration apConfig = new WifiConfiguration();
        try {
            mWifiServiceImpl.setWifiApConfiguration(apConfig, TEST_PACKAGE_NAME);
            fail("Expected SecurityException");
        } catch (SecurityException e) { }
    }

    /**
     * Ensure softap config is written when the caller has the correct permission.
     */
    @Test
    public void testSetWifiApConfigurationSuccess() {
        when(mWifiPermissionsUtil.checkConfigOverridePermission(anyInt())).thenReturn(true);
        WifiConfiguration apConfig = createValidSoftApConfiguration();

        assertTrue(mWifiServiceImpl.setWifiApConfiguration(apConfig, TEST_PACKAGE_NAME));
        mLooper.dispatchAll();
        verifyCheckChangePermission(TEST_PACKAGE_NAME);
        verify(mWifiApConfigStore).setApConfiguration(eq(apConfig));
    }

    /**
     * Ensure that a null config does not overwrite the saved ap config.
     */
    @Test
    public void testSetWifiApConfigurationNullConfigNotSaved() {
        when(mWifiPermissionsUtil.checkConfigOverridePermission(anyInt())).thenReturn(true);
        assertFalse(mWifiServiceImpl.setWifiApConfiguration(null, TEST_PACKAGE_NAME));
        verify(mWifiApConfigStore, never()).setApConfiguration(isNull(WifiConfiguration.class));
    }

    /**
     * Ensure that an invalid config does not overwrite the saved ap config.
     */
    @Test
    public void testSetWifiApConfigurationWithInvalidConfigNotSaved() {
        when(mWifiPermissionsUtil.checkConfigOverridePermission(anyInt())).thenReturn(true);
        assertFalse(mWifiServiceImpl.setWifiApConfiguration(new WifiConfiguration(),
                                                            TEST_PACKAGE_NAME));
        verify(mWifiApConfigStore, never()).setApConfiguration(any());
    }

    /**
     * Ensure unpermitted callers are not able to retrieve the softap config.
     *
     * @throws SecurityException
     */
    @Test
    public void testGetWifiApConfigurationNotReturnedWithoutPermission() {
        when(mWifiPermissionsUtil.checkConfigOverridePermission(anyInt())).thenReturn(false);
        try {
            mWifiServiceImpl.getWifiApConfiguration();
            fail("Expected a SecurityException");
        } catch (SecurityException e) {
        }
    }

    /**
     * Ensure permitted callers are able to retrieve the softap config.
     */
    @Test
    public void testGetWifiApConfigurationSuccess() {
        setupWifiStateMachineHandlerForRunWithScissors();

        mWifiServiceImpl = new WifiServiceImpl(mContext, mWifiInjector, mAsyncChannel);
        mWifiServiceImpl.setWifiHandlerLogForTest(mLog);

        when(mFrameworkFacade.inStorageManagerCryptKeeperBounce()).thenReturn(false);
        when(mSettingsStore.isWifiToggleEnabled()).thenReturn(false);

        when(mWifiPermissionsUtil.checkConfigOverridePermission(anyInt())).thenReturn(true);
        WifiConfiguration apConfig = new WifiConfiguration();
        when(mWifiApConfigStore.getApConfiguration()).thenReturn(apConfig);
        assertEquals(apConfig, mWifiServiceImpl.getWifiApConfiguration());
    }

    /**
     * Ensure we return the proper variable for the softap state after getting an AP state change
     * broadcast.
     */
    @Test
    public void testGetWifiApEnabled() {
        // set up WifiServiceImpl with a live thread for testing
        HandlerThread serviceHandlerThread = createAndStartHandlerThreadForRunWithScissors();
        when(mWifiInjector.getWifiServiceHandlerThread()).thenReturn(serviceHandlerThread);
        mWifiServiceImpl = new WifiServiceImpl(mContext, mWifiInjector, mAsyncChannel);
        mWifiServiceImpl.setWifiHandlerLogForTest(mLog);

        // ap should be disabled when wifi hasn't been started
        assertEquals(WifiManager.WIFI_AP_STATE_DISABLED, mWifiServiceImpl.getWifiApEnabledState());

        when(mFrameworkFacade.inStorageManagerCryptKeeperBounce()).thenReturn(false);
        when(mSettingsStore.isWifiToggleEnabled()).thenReturn(false);
        mWifiServiceImpl.checkAndStartWifi();
        mLooper.dispatchAll();

        // ap should be disabled initially
        assertEquals(WifiManager.WIFI_AP_STATE_DISABLED, mWifiServiceImpl.getWifiApEnabledState());

        // send an ap state change to verify WifiServiceImpl is updated
        verify(mContext).registerReceiver(mBroadcastReceiverCaptor.capture(),
                (IntentFilter) argThat(new IntentFilterMatcher()));

        TestUtil.sendWifiApStateChanged(mBroadcastReceiverCaptor.getValue(), mContext,
                WIFI_AP_STATE_FAILED, WIFI_AP_STATE_DISABLED, SAP_START_FAILURE_GENERAL,
                WIFI_IFACE_NAME, IFACE_IP_MODE_LOCAL_ONLY);
        mLooper.dispatchAll();

        assertEquals(WifiManager.WIFI_AP_STATE_FAILED, mWifiServiceImpl.getWifiApEnabledState());
    }

    /**
     * Ensure we do not allow unpermitted callers to get the wifi ap state.
     */
    @Ignore
    @Test
    public void testGetWifiApEnabledPermissionDenied() {
        // we should not be able to get the state
        doThrow(new SecurityException()).when(mContext)
                .enforceCallingOrSelfPermission(eq(android.Manifest.permission.ACCESS_WIFI_STATE),
                                                eq("WifiService"));

        try {
            mWifiServiceImpl.getWifiApEnabledState();
            fail("expected SecurityException");
        } catch (SecurityException expected) { }
    }

    /**
     * Make sure we do not start wifi if System services have to be restarted to decrypt the device.
     */
    @Test
    public void testWifiControllerDoesNotStartWhenDeviceTriggerResetMainAtBoot() {
        when(mFrameworkFacade.inStorageManagerCryptKeeperBounce()).thenReturn(true);
        when(mSettingsStore.isWifiToggleEnabled()).thenReturn(false);
        mWifiServiceImpl.checkAndStartWifi();
        verify(mWifiController, never()).start();
    }

    /**
     * Make sure we do start WifiController (wifi disabled) if the device is already decrypted.
     */
    @Test
    public void testWifiControllerStartsWhenDeviceIsDecryptedAtBootWithWifiDisabled() {
        when(mFrameworkFacade.inStorageManagerCryptKeeperBounce()).thenReturn(false);
        when(mSettingsStore.isWifiToggleEnabled()).thenReturn(false);
        mWifiServiceImpl.checkAndStartWifi();
        verify(mWifiController).start();
        verify(mWifiController, never()).sendMessage(CMD_WIFI_TOGGLED);
    }

    /**
     * Make sure we do start WifiController (wifi enabled) if the device is already decrypted.
     */
    @Test
    public void testWifiFullyStartsWhenDeviceIsDecryptedAtBootWithWifiEnabled() {
        when(mFrameworkFacade.inStorageManagerCryptKeeperBounce()).thenReturn(false);
        when(mSettingsStore.handleWifiToggled(true)).thenReturn(true);
        when(mSettingsStore.isWifiToggleEnabled()).thenReturn(true);
        when(mWifiStateMachine.syncGetWifiState()).thenReturn(WIFI_STATE_DISABLED);
        when(mContext.getPackageName()).thenReturn(ANDROID_SYSTEM_PACKAGE);
        mWifiServiceImpl.checkAndStartWifi();
        verify(mWifiController).start();
        verify(mWifiController).sendMessage(CMD_WIFI_TOGGLED);
    }

    /**
     * Verify caller with proper permission can call startSoftAp.
     */
    @Test
    public void testStartSoftApWithPermissionsAndNullConfig() {
        boolean result = mWifiServiceImpl.startSoftAp(null);
        assertTrue(result);
        verify(mWifiController)
                .sendMessage(eq(CMD_SET_AP), eq(1), eq(0), mSoftApModeConfigCaptor.capture());
        assertNull(mSoftApModeConfigCaptor.getValue().getWifiConfiguration());
    }

    /**
     * Verify caller with proper permissions but an invalid config does not start softap.
     */
    @Test
    public void testStartSoftApWithPermissionsAndInvalidConfig() {
        boolean result = mWifiServiceImpl.startSoftAp(mApConfig);
        assertFalse(result);
        verifyZeroInteractions(mWifiController);
    }

    /**
     * Verify caller with proper permission and valid config does start softap.
     */
    @Test
    public void testStartSoftApWithPermissionsAndValidConfig() {
        WifiConfiguration config = createValidSoftApConfiguration();
        boolean result = mWifiServiceImpl.startSoftAp(config);
        assertTrue(result);
        verify(mWifiController)
                .sendMessage(eq(CMD_SET_AP), eq(1), eq(0), mSoftApModeConfigCaptor.capture());
        assertEquals(config, mSoftApModeConfigCaptor.getValue().getWifiConfiguration());
    }

    /**
     * Verify a SecurityException is thrown when a caller without the correct permission attempts to
     * start softap.
     */
    @Test(expected = SecurityException.class)
    public void testStartSoftApWithoutPermissionThrowsException() throws Exception {
        doThrow(new SecurityException()).when(mContext)
                .enforceCallingOrSelfPermission(eq(android.Manifest.permission.NETWORK_STACK),
                                                eq("WifiService"));
        mWifiServiceImpl.startSoftAp(null);
    }

    /**
     * Verify caller with proper permission can call stopSoftAp.
     */
    @Test
    public void testStopSoftApWithPermissions() {
        boolean result = mWifiServiceImpl.stopSoftAp();
        assertTrue(result);
        verify(mWifiController).sendMessage(eq(CMD_SET_AP), eq(0), eq(0));
    }

    /**
     * Verify SecurityException is thrown when a caller without the correct permission attempts to
     * stop softap.
     */
    @Test(expected = SecurityException.class)
    public void testStopSoftApWithoutPermissionThrowsException() throws Exception {
        doThrow(new SecurityException()).when(mContext)
                .enforceCallingOrSelfPermission(eq(android.Manifest.permission.NETWORK_STACK),
                                                eq("WifiService"));
        mWifiServiceImpl.stopSoftAp();
    }

    /**
     * Ensure that we handle app ops check failure when handling scan request.
     */
    @Test
    public void testStartScanFailureAppOpsIgnored() {
        setupWifiStateMachineHandlerForRunWithScissors();
        doReturn(AppOpsManager.MODE_IGNORED).when(mAppOpsManager)
                .noteOp(AppOpsManager.OPSTR_CHANGE_WIFI_STATE, Process.myUid(), SCAN_PACKAGE_NAME);
        assertFalse(mWifiServiceImpl.startScan(SCAN_PACKAGE_NAME));
        verify(mScanRequestProxy, never()).startScan(Process.myUid(), SCAN_PACKAGE_NAME);
    }

    /**
     * Ensure that we handle scan access permission check failure when handling scan request.
     */
    @Test
    public void testStartScanFailureInCanAccessScanResultsPermission() {
        setupWifiStateMachineHandlerForRunWithScissors();
        doThrow(new SecurityException()).when(mWifiPermissionsUtil)
                .enforceCanAccessScanResults(SCAN_PACKAGE_NAME, Process.myUid());
        assertFalse(mWifiServiceImpl.startScan(SCAN_PACKAGE_NAME));
        verify(mScanRequestProxy, never()).startScan(Process.myUid(), SCAN_PACKAGE_NAME);
    }

    /**
     * Ensure that we handle scan request failure when posting the runnable to handler fails.
     */
    @Ignore
    @Test
    public void testStartScanFailureInRunWithScissors() {
        setupWifiStateMachineHandlerForRunWithScissors();
        doReturn(false).when(mHandlerSpyForWsmRunWithScissors)
                .runWithScissors(any(), anyLong());
        assertFalse(mWifiServiceImpl.startScan(SCAN_PACKAGE_NAME));
        verify(mScanRequestProxy, never()).startScan(Process.myUid(), SCAN_PACKAGE_NAME);
    }

    /**
     * Ensure that we handle scan request failure from ScanRequestProxy fails.
     */
    @Test
    public void testStartScanFailureFromScanRequestProxy() {
        setupWifiStateMachineHandlerForRunWithScissors();
        when(mScanRequestProxy.startScan(anyInt(), anyString())).thenReturn(false);
        assertFalse(mWifiServiceImpl.startScan(SCAN_PACKAGE_NAME));
        verify(mScanRequestProxy).startScan(Process.myUid(), SCAN_PACKAGE_NAME);
    }

    static final String TEST_SSID = "Sid's Place";
    static final String TEST_SSID_WITH_QUOTES = "\"" + TEST_SSID + "\"";
    static final String TEST_BSSID = "01:02:03:04:05:06";
    static final String TEST_PACKAGE = "package";

    private void setupForGetConnectionInfo() {
        WifiInfo wifiInfo = new WifiInfo();
        wifiInfo.setSSID(WifiSsid.createFromAsciiEncoded(TEST_SSID));
        wifiInfo.setBSSID(TEST_BSSID);
        when(mWifiStateMachine.syncRequestConnectionInfo()).thenReturn(wifiInfo);
    }

    /**
     * Test that connected SSID and BSSID are not exposed to an app that does not have the
     * appropriate permissions.
     */
    @Test
    public void testConnectedIdsAreHiddenFromAppWithoutPermission() throws Exception {
        setupForGetConnectionInfo();

        doThrow(new SecurityException()).when(mWifiPermissionsUtil).enforceCanAccessScanResults(
                anyString(), anyInt());

        WifiInfo connectionInfo = mWifiServiceImpl.getConnectionInfo(TEST_PACKAGE);

        assertEquals(WifiSsid.NONE, connectionInfo.getSSID());
        assertEquals(WifiInfo.DEFAULT_MAC_ADDRESS, connectionInfo.getBSSID());
    }

    /**
     * Test that connected SSID and BSSID are not exposed to an app that does not have the
     * appropriate permissions, when enforceCanAccessScanResults raises a SecurityException.
     */
    @Test
    public void testConnectedIdsAreHiddenOnSecurityException() throws Exception {
        setupForGetConnectionInfo();

        doThrow(new SecurityException()).when(mWifiPermissionsUtil).enforceCanAccessScanResults(
                anyString(), anyInt());

        WifiInfo connectionInfo = mWifiServiceImpl.getConnectionInfo(TEST_PACKAGE);

        assertEquals(WifiSsid.NONE, connectionInfo.getSSID());
        assertEquals(WifiInfo.DEFAULT_MAC_ADDRESS, connectionInfo.getBSSID());
    }

    /**
     * Test that connected SSID and BSSID are exposed to an app that does have the
     * appropriate permissions.
     */
    @Test
    public void testConnectedIdsAreVisibleFromPermittedApp() throws Exception {
        setupForGetConnectionInfo();

        WifiInfo connectionInfo = mWifiServiceImpl.getConnectionInfo(TEST_PACKAGE);

        assertEquals(TEST_SSID_WITH_QUOTES, connectionInfo.getSSID());
        assertEquals(TEST_BSSID, connectionInfo.getBSSID());
    }

    /**
     * Test fetching of scan results.
     */
    @Test
    public void testGetScanResults() {
        setupWifiStateMachineHandlerForRunWithScissors();

        ScanResult[] scanResults =
                ScanTestUtil.createScanDatas(new int[][]{{2417, 2427, 5180, 5170}})[0]
                        .getResults();
        List<ScanResult> scanResultList =
                new ArrayList<>(Arrays.asList(scanResults));
        when(mScanRequestProxy.getScanResults()).thenReturn(scanResultList);

        String packageName = "test.com";
        List<ScanResult> retrievedScanResultList = mWifiServiceImpl.getScanResults(packageName);
        verify(mScanRequestProxy).getScanResults();

        ScanTestUtil.assertScanResultsEquals(scanResults,
                retrievedScanResultList.toArray(new ScanResult[retrievedScanResultList.size()]));
    }

    /**
     * Ensure that we handle scan results failure when posting the runnable to handler fails.
     */
    @Ignore
    @Test
    public void testGetScanResultsFailureInRunWithScissors() {
        setupWifiStateMachineHandlerForRunWithScissors();
        doReturn(false).when(mHandlerSpyForWsmRunWithScissors)
                .runWithScissors(any(), anyLong());

        ScanResult[] scanResults =
                ScanTestUtil.createScanDatas(new int[][]{{2417, 2427, 5180, 5170}})[0]
                        .getResults();
        List<ScanResult> scanResultList =
                new ArrayList<>(Arrays.asList(scanResults));
        when(mScanRequestProxy.getScanResults()).thenReturn(scanResultList);

        String packageName = "test.com";
        List<ScanResult> retrievedScanResultList = mWifiServiceImpl.getScanResults(packageName);
        verify(mScanRequestProxy, never()).getScanResults();

        assertTrue(retrievedScanResultList.isEmpty());
    }

    private void registerLOHSRequestFull() {
        // allow test to proceed without a permission check failure
        when(mSettingsStore.getLocationModeSetting(mContext))
                .thenReturn(LOCATION_MODE_HIGH_ACCURACY);
        try {
            when(mFrameworkFacade.isAppForeground(anyInt())).thenReturn(true);
        } catch (RemoteException e) { }
        when(mUserManager.hasUserRestriction(UserManager.DISALLOW_CONFIG_TETHERING))
                .thenReturn(false);
        int result = mWifiServiceImpl.startLocalOnlyHotspot(mAppMessenger, mAppBinder,
                TEST_PACKAGE_NAME);
        assertEquals(LocalOnlyHotspotCallback.REQUEST_REGISTERED, result);
        verifyCheckChangePermission(TEST_PACKAGE_NAME);
    }

    /**
     * Verify that the call to startLocalOnlyHotspot returns REQUEST_REGISTERED when successfully
     * called.
     */
    @Test
    public void testStartLocalOnlyHotspotSingleRegistrationReturnsRequestRegistered() {
        registerLOHSRequestFull();
    }

    /**
     * Verify that a call to startLocalOnlyHotspot throws a SecurityException if the caller does not
     * have the CHANGE_WIFI_STATE permission.
     */
    @Test(expected = SecurityException.class)
    public void testStartLocalOnlyHotspotThrowsSecurityExceptionWithoutCorrectPermission() {
        doThrow(new SecurityException()).when(mContext)
                .enforceCallingOrSelfPermission(eq(android.Manifest.permission.CHANGE_WIFI_STATE),
                                                eq("WifiService"));
        mWifiServiceImpl.startLocalOnlyHotspot(mAppMessenger, mAppBinder, TEST_PACKAGE_NAME);
    }

    /**
     * Verify that a call to startLocalOnlyHotspot throws a SecurityException if the caller does not
     * have Location permission.
     */
    @Test(expected = SecurityException.class)
    public void testStartLocalOnlyHotspotThrowsSecurityExceptionWithoutLocationPermission() {
        doThrow(new SecurityException())
                .when(mWifiPermissionsUtil).enforceLocationPermission(eq(TEST_PACKAGE_NAME),
                                                                      anyInt());
        mWifiServiceImpl.startLocalOnlyHotspot(mAppMessenger, mAppBinder, TEST_PACKAGE_NAME);
    }

    /**
     * Verify that a call to startLocalOnlyHotspot throws a SecurityException if Location mode is
     * disabled.
     */
    @Test(expected = SecurityException.class)
    public void testStartLocalOnlyHotspotThrowsSecurityExceptionWithoutLocationEnabled() {
        when(mSettingsStore.getLocationModeSetting(mContext)).thenReturn(LOCATION_MODE_OFF);
        mWifiServiceImpl.startLocalOnlyHotspot(mAppMessenger, mAppBinder, TEST_PACKAGE_NAME);
    }

    /**
     * Only start LocalOnlyHotspot if the caller is the foreground app at the time of the request.
     */
    @Test
    public void testStartLocalOnlyHotspotFailsIfRequestorNotForegroundApp() throws Exception {
        when(mSettingsStore.getLocationModeSetting(mContext))
                .thenReturn(LOCATION_MODE_HIGH_ACCURACY);

        when(mFrameworkFacade.isAppForeground(anyInt())).thenReturn(false);
        int result = mWifiServiceImpl.startLocalOnlyHotspot(mAppMessenger, mAppBinder,
                TEST_PACKAGE_NAME);
        assertEquals(LocalOnlyHotspotCallback.ERROR_INCOMPATIBLE_MODE, result);
    }

    /**
     * Do not register the LocalOnlyHotspot request if the caller app cannot be verified as the
     * foreground app at the time of the request (ie, throws an exception in the check).
     */
    @Test
    public void testStartLocalOnlyHotspotFailsIfForegroundAppCheckThrowsRemoteException()
            throws Exception {
        when(mSettingsStore.getLocationModeSetting(mContext))
                .thenReturn(LOCATION_MODE_HIGH_ACCURACY);

        when(mFrameworkFacade.isAppForeground(anyInt())).thenThrow(new RemoteException());
        int result = mWifiServiceImpl.startLocalOnlyHotspot(mAppMessenger, mAppBinder,
                TEST_PACKAGE_NAME);
        assertEquals(LocalOnlyHotspotCallback.ERROR_INCOMPATIBLE_MODE, result);
    }

    /**
     * Only start LocalOnlyHotspot if we are not tethering.
     */
    @Test
    public void testHotspotDoesNotStartWhenAlreadyTethering() throws Exception {
        when(mSettingsStore.getLocationModeSetting(mContext))
                            .thenReturn(LOCATION_MODE_HIGH_ACCURACY);
        when(mFrameworkFacade.isAppForeground(anyInt())).thenReturn(true);
        mWifiServiceImpl.updateInterfaceIpState(WIFI_IFACE_NAME, IFACE_IP_MODE_TETHERED);
        mLooper.dispatchAll();
        int returnCode = mWifiServiceImpl.startLocalOnlyHotspot(
                mAppMessenger, mAppBinder, TEST_PACKAGE_NAME);
        assertEquals(ERROR_INCOMPATIBLE_MODE, returnCode);
    }

    /**
     * Only start LocalOnlyHotspot if admin setting does not disallow tethering.
     */
    @Test
    public void testHotspotDoesNotStartWhenTetheringDisallowed() throws Exception {
        when(mSettingsStore.getLocationModeSetting(mContext))
                .thenReturn(LOCATION_MODE_HIGH_ACCURACY);
        when(mFrameworkFacade.isAppForeground(anyInt())).thenReturn(true);
        when(mUserManager.hasUserRestriction(UserManager.DISALLOW_CONFIG_TETHERING))
                .thenReturn(true);
        int returnCode = mWifiServiceImpl.startLocalOnlyHotspot(
                mAppMessenger, mAppBinder, TEST_PACKAGE_NAME);
        assertEquals(ERROR_TETHERING_DISALLOWED, returnCode);
    }

    /**
     * Verify that callers can only have one registered LOHS request.
     */
    @Test(expected = IllegalStateException.class)
    public void testStartLocalOnlyHotspotThrowsExceptionWhenCallerAlreadyRegistered() {
        registerLOHSRequestFull();

        // now do the second request that will fail
        mWifiServiceImpl.startLocalOnlyHotspot(mAppMessenger, mAppBinder, TEST_PACKAGE_NAME);
    }

    /**
     * Verify that the call to stopLocalOnlyHotspot does not do anything when there aren't any
     * registered callers.
     */
    @Test
    public void testStopLocalOnlyHotspotDoesNothingWithoutRegisteredRequests() {
        // allow test to proceed without a permission check failure
        mWifiServiceImpl.stopLocalOnlyHotspot();
        // there is nothing registered, so this shouldn't do anything
        verify(mWifiController, never()).sendMessage(eq(CMD_SET_AP), anyInt(), anyInt());
    }

    /**
     * Verify that the call to stopLocalOnlyHotspot does not do anything when one caller unregisters
     * but there is still an active request
     */
    @Test
    public void testStopLocalOnlyHotspotDoesNothingWithARemainingRegisteredRequest() {
        // register a request that will remain after the stopLOHS call
        mWifiServiceImpl.registerLOHSForTest(mPid, mRequestInfo);

        registerLOHSRequestFull();

        // Since we are calling with the same pid, the second register call will be removed
        mWifiServiceImpl.stopLocalOnlyHotspot();
        // there is still a valid registered request - do not tear down LOHS
        verify(mWifiController, never()).sendMessage(eq(CMD_SET_AP), anyInt(), anyInt());
    }

    /**
     * Verify that the call to stopLocalOnlyHotspot sends a message to WifiController to stop
     * the softAp when there is one registered caller when that caller is removed.
     */
    @Test
    public void testStopLocalOnlyHotspotTriggersSoftApStopWithOneRegisteredRequest() {
        registerLOHSRequestFull();
        verify(mWifiController)
                .sendMessage(eq(CMD_SET_AP), eq(1), eq(0), any(SoftApModeConfiguration.class));

        // No permission check required for change_wifi_state.
        verify(mContext, never()).enforceCallingOrSelfPermission(
                eq("android.Manifest.permission.CHANGE_WIFI_STATE"), anyString());

        mWifiServiceImpl.stopLocalOnlyHotspot();
        // there is was only one request registered, we should tear down softap
        verify(mWifiController).sendMessage(eq(CMD_SET_AP), eq(0), eq(0));
    }

    /**
     * Verify that WifiServiceImpl does not send the stop ap message if there were no
     * pending LOHS requests upon a binder death callback.
     */
    @Test
    public void testServiceImplNotCalledWhenBinderDeathTriggeredNoRequests() {
        LocalOnlyRequestorCallback binderDeathCallback =
                mWifiServiceImpl.new LocalOnlyRequestorCallback();

        binderDeathCallback.onLocalOnlyHotspotRequestorDeath(mRequestInfo);
        verify(mWifiController, never()).sendMessage(eq(CMD_SET_AP), eq(0), eq(0));
    }

    /**
     * Verify that WifiServiceImpl does not send the stop ap message if there are remaining
     * registered LOHS requests upon a binder death callback.  Additionally verify that softap mode
     * will be stopped if that remaining request is removed (to verify the binder death properly
     * cleared the requestor that died).
     */
    @Test
    public void testServiceImplNotCalledWhenBinderDeathTriggeredWithRegisteredRequests() {
        LocalOnlyRequestorCallback binderDeathCallback =
                mWifiServiceImpl.new LocalOnlyRequestorCallback();

        // registering a request directly from the test will not trigger a message to start
        // softap mode
        mWifiServiceImpl.registerLOHSForTest(mPid, mRequestInfo);

        registerLOHSRequestFull();

        binderDeathCallback.onLocalOnlyHotspotRequestorDeath(mRequestInfo);
        verify(mWifiController, never()).sendMessage(eq(CMD_SET_AP), anyInt(), anyInt());

        reset(mWifiController);

        // now stop as the second request and confirm CMD_SET_AP will be sent to make sure binder
        // death requestor was removed
        mWifiServiceImpl.stopLocalOnlyHotspot();
        verify(mWifiController).sendMessage(eq(CMD_SET_AP), eq(0), eq(0));
    }

    /**
     * Verify that a call to registerSoftApCallback throws a SecurityException if the caller does
     * not have NETWORK_SETTINGS permission.
     */
    @Test
    public void registerSoftApCallbackThrowsSecurityExceptionOnMissingPermissions() {
        doThrow(new SecurityException()).when(mContext)
                .enforceCallingOrSelfPermission(eq(android.Manifest.permission.NETWORK_SETTINGS),
                                                eq("WifiService"));
        try {
            final int callbackIdentifier = 1;
            mWifiServiceImpl.registerSoftApCallback(mAppBinder, mClientSoftApCallback,
                    callbackIdentifier);
            fail("expected SecurityException");
        } catch (SecurityException expected) {
        }
    }

    /**
     * Verify that a call to registerSoftApCallback throws an IllegalArgumentException if the
     * parameters are not provided.
     */
    @Test
    public void registerSoftApCallbackThrowsIllegalArgumentExceptionOnInvalidArguments() {
        try {
            final int callbackIdentifier = 1;
            mWifiServiceImpl.registerSoftApCallback(mAppBinder, null, callbackIdentifier);
            fail("expected IllegalArgumentException");
        } catch (IllegalArgumentException expected) {
        }
    }

    /**
     * Verify that a call to unregisterSoftApCallback throws a SecurityException if the caller does
     * not have NETWORK_SETTINGS permission.
     */
    @Test
    public void unregisterSoftApCallbackThrowsSecurityExceptionOnMissingPermissions() {
        doThrow(new SecurityException()).when(mContext)
                .enforceCallingOrSelfPermission(eq(android.Manifest.permission.NETWORK_SETTINGS),
                                                eq("WifiService"));
        try {
            final int callbackIdentifier = 1;
            mWifiServiceImpl.unregisterSoftApCallback(callbackIdentifier);
            fail("expected SecurityException");
        } catch (SecurityException expected) {
        }
    }

    /**
     * Registers a soft AP callback, then verifies that the current soft AP state and num clients
     * are sent to caller immediately after callback is registered.
     */
    private void registerSoftApCallbackAndVerify(ISoftApCallback callback, int callbackIdentifier)
            throws Exception {
        mWifiServiceImpl.registerSoftApCallback(mAppBinder, callback, callbackIdentifier);
        mLooper.dispatchAll();
        verify(callback).onStateChanged(WIFI_AP_STATE_DISABLED, 0);
        verify(callback).onNumClientsChanged(0);
    }

    /**
     * Verify that registering twice with same callbackIdentifier will replace the first callback.
     */
    @Test
    public void replacesOldCallbackWithNewCallbackWhenRegisteringTwice() throws Exception {
        final int callbackIdentifier = 1;
        registerSoftApCallbackAndVerify(mClientSoftApCallback, callbackIdentifier);
        registerSoftApCallbackAndVerify(mAnotherSoftApCallback, callbackIdentifier);

        final int testNumClients = 4;
        mStateMachineSoftApCallback.onNumClientsChanged(testNumClients);
        mLooper.dispatchAll();
        // Verify only the second callback is being called
        verify(mClientSoftApCallback, never()).onNumClientsChanged(testNumClients);
        verify(mAnotherSoftApCallback).onNumClientsChanged(testNumClients);
    }

    /**
     * Verify that unregisterSoftApCallback removes callback from registered callbacks list
     */
    @Test
    public void unregisterSoftApCallbackRemovesCallback() throws Exception {
        final int callbackIdentifier = 1;
        registerSoftApCallbackAndVerify(mClientSoftApCallback, callbackIdentifier);

        mWifiServiceImpl.unregisterSoftApCallback(callbackIdentifier);
        mLooper.dispatchAll();

        final int testNumClients = 4;
        mStateMachineSoftApCallback.onNumClientsChanged(testNumClients);
        mLooper.dispatchAll();
        verify(mClientSoftApCallback, never()).onNumClientsChanged(testNumClients);
    }

    /**
     * Verify that unregisterSoftApCallback is no-op if callbackIdentifier not registered.
     */
    @Test
    public void unregisterSoftApCallbackDoesNotRemoveCallbackIfCallbackIdentifierNotMatching()
            throws Exception {
        final int callbackIdentifier = 1;
        registerSoftApCallbackAndVerify(mClientSoftApCallback, callbackIdentifier);

        final int differentCallbackIdentifier = 2;
        mWifiServiceImpl.unregisterSoftApCallback(differentCallbackIdentifier);
        mLooper.dispatchAll();

        final int testNumClients = 4;
        mStateMachineSoftApCallback.onNumClientsChanged(testNumClients);
        mLooper.dispatchAll();
        verify(mClientSoftApCallback).onNumClientsChanged(testNumClients);
    }

    /**
     * Registers two callbacks, remove one then verify the right callback is being called on events.
     */
    @Test
    public void correctCallbackIsCalledAfterAddingTwoCallbacksAndRemovingOne() throws Exception {
        final int callbackIdentifier = 1;
        mWifiServiceImpl.registerSoftApCallback(mAppBinder, mClientSoftApCallback,
                callbackIdentifier);

        // Change state from default before registering the second callback
        final int testNumClients = 4;
        mStateMachineSoftApCallback.onStateChanged(WIFI_AP_STATE_ENABLED, 0);
        mStateMachineSoftApCallback.onNumClientsChanged(testNumClients);

        // Register another callback and verify the new state is returned in the immediate callback
        final int anotherUid = 2;
        mWifiServiceImpl.registerSoftApCallback(mAppBinder, mAnotherSoftApCallback, anotherUid);
        mLooper.dispatchAll();
        verify(mAnotherSoftApCallback).onStateChanged(WIFI_AP_STATE_ENABLED, 0);
        verify(mAnotherSoftApCallback).onNumClientsChanged(testNumClients);

        // unregister the fisrt callback
        mWifiServiceImpl.unregisterSoftApCallback(callbackIdentifier);
        mLooper.dispatchAll();

        // Update soft AP state and verify the remaining callback receives the event
        mStateMachineSoftApCallback.onStateChanged(WIFI_AP_STATE_FAILED,
                SAP_START_FAILURE_NO_CHANNEL);
        mLooper.dispatchAll();
        verify(mClientSoftApCallback, never()).onStateChanged(WIFI_AP_STATE_FAILED,
                SAP_START_FAILURE_NO_CHANNEL);
        verify(mAnotherSoftApCallback).onStateChanged(WIFI_AP_STATE_FAILED,
                SAP_START_FAILURE_NO_CHANNEL);
    }

    /**
     * Verify that wifi service registers for callers BinderDeath event
     */
    @Ignore
    @Test
    public void registersForBinderDeathOnRegisterSoftApCallback() throws Exception {
        final int callbackIdentifier = 1;
        registerSoftApCallbackAndVerify(mClientSoftApCallback, callbackIdentifier);
        verify(mAppBinder).linkToDeath(any(IBinder.DeathRecipient.class), anyInt());
    }

    /**
     * Verify that we un-register the soft AP callback on receiving BinderDied event.
     */
    @Test
    public void unregistersSoftApCallbackOnBinderDied() throws Exception {
        ArgumentCaptor<IBinder.DeathRecipient> drCaptor =
                ArgumentCaptor.forClass(IBinder.DeathRecipient.class);
        final int callbackIdentifier = 1;
        registerSoftApCallbackAndVerify(mClientSoftApCallback, callbackIdentifier);
        verify(mAppBinder).linkToDeath(drCaptor.capture(), anyInt());

        drCaptor.getValue().binderDied();
        mLooper.dispatchAll();
        verify(mAppBinder).unlinkToDeath(drCaptor.getValue(), 0);

        // Verify callback is removed from the list as well
        final int testNumClients = 4;
        mStateMachineSoftApCallback.onNumClientsChanged(testNumClients);
        mLooper.dispatchAll();
        verify(mClientSoftApCallback, never()).onNumClientsChanged(testNumClients);
    }

    /**
     * Verify that soft AP callback is called on NumClientsChanged event
     */
    @Test
    public void callsRegisteredCallbacksOnNumClientsChangedEvent() throws Exception {
        final int callbackIdentifier = 1;
        registerSoftApCallbackAndVerify(mClientSoftApCallback, callbackIdentifier);

        final int testNumClients = 4;
        mStateMachineSoftApCallback.onNumClientsChanged(testNumClients);
        mLooper.dispatchAll();
        verify(mClientSoftApCallback).onNumClientsChanged(testNumClients);
    }

    /**
     * Verify that soft AP callback is called on SoftApStateChanged event
     */
    @Test
    public void callsRegisteredCallbacksOnSoftApStateChangedEvent() throws Exception {
        final int callbackIdentifier = 1;
        registerSoftApCallbackAndVerify(mClientSoftApCallback, callbackIdentifier);

        mStateMachineSoftApCallback.onStateChanged(WIFI_AP_STATE_ENABLED, 0);
        mLooper.dispatchAll();
        verify(mClientSoftApCallback).onStateChanged(WIFI_AP_STATE_ENABLED, 0);
    }

    /**
     * Verify that mSoftApState and mSoftApNumClients in WifiServiceImpl are being updated on soft
     * Ap events, even when no callbacks are registered.
     */
    @Test
    public void updatesSoftApStateAndNumClientsOnSoftApEvents() throws Exception {
        final int testNumClients = 4;
        mStateMachineSoftApCallback.onStateChanged(WIFI_AP_STATE_ENABLED, 0);
        mStateMachineSoftApCallback.onNumClientsChanged(testNumClients);

        // Register callback after num clients and soft AP are changed.
        final int callbackIdentifier = 1;
        mWifiServiceImpl.registerSoftApCallback(mAppBinder, mClientSoftApCallback,
                callbackIdentifier);
        mLooper.dispatchAll();
        verify(mClientSoftApCallback).onStateChanged(WIFI_AP_STATE_ENABLED, 0);
        verify(mClientSoftApCallback).onNumClientsChanged(testNumClients);
    }

    private class IntentFilterMatcher implements ArgumentMatcher<IntentFilter> {
        @Override
        public boolean matches(IntentFilter filter) {
            return filter.hasAction(WifiManager.WIFI_AP_STATE_CHANGED_ACTION);
        }
    }

    /**
     * Verify that onFailed is called for registered LOHS callers when a WIFI_AP_STATE_CHANGE
     * broadcast is received.
     */
    @Test
    public void testRegisteredCallbacksTriggeredOnSoftApFailureGeneric() throws Exception {
        when(mFrameworkFacade.inStorageManagerCryptKeeperBounce()).thenReturn(false);
        when(mSettingsStore.isWifiToggleEnabled()).thenReturn(false);
        mWifiServiceImpl.checkAndStartWifi();

        verify(mContext).registerReceiver(mBroadcastReceiverCaptor.capture(),
                (IntentFilter) argThat(new IntentFilterMatcher()));

        registerLOHSRequestFull();

        TestUtil.sendWifiApStateChanged(mBroadcastReceiverCaptor.getValue(), mContext,
                WIFI_AP_STATE_FAILED, WIFI_AP_STATE_DISABLED, SAP_START_FAILURE_GENERAL,
                WIFI_IFACE_NAME, IFACE_IP_MODE_LOCAL_ONLY);
        mLooper.dispatchAll();
        verify(mHandler).handleMessage(mMessageCaptor.capture());
        Message message = mMessageCaptor.getValue();
        assertEquals(HOTSPOT_FAILED, message.what);
        assertEquals(ERROR_GENERIC, message.arg1);
    }

    /**
     * Verify that onFailed is called for registered LOHS callers when a WIFI_AP_STATE_CHANGE
     * broadcast is received with the SAP_START_FAILURE_NO_CHANNEL error.
     */
    @Test
    public void testRegisteredCallbacksTriggeredOnSoftApFailureNoChannel() throws Exception {
        when(mFrameworkFacade.inStorageManagerCryptKeeperBounce()).thenReturn(false);
        when(mSettingsStore.isWifiToggleEnabled()).thenReturn(false);
        mWifiServiceImpl.checkAndStartWifi();

        verify(mContext).registerReceiver(mBroadcastReceiverCaptor.capture(),
                (IntentFilter) argThat(new IntentFilterMatcher()));

        registerLOHSRequestFull();

        TestUtil.sendWifiApStateChanged(mBroadcastReceiverCaptor.getValue(), mContext,
                WIFI_AP_STATE_FAILED, WIFI_AP_STATE_DISABLED, SAP_START_FAILURE_NO_CHANNEL,
                WIFI_IFACE_NAME, IFACE_IP_MODE_LOCAL_ONLY);

        mLooper.dispatchAll();
        verify(mHandler).handleMessage(mMessageCaptor.capture());
        Message message = mMessageCaptor.getValue();
        assertEquals(HOTSPOT_FAILED, message.what);
        assertEquals(ERROR_NO_CHANNEL, message.arg1);
    }

    /**
     * Verify that onStopped is called for registered LOHS callers when a WIFI_AP_STATE_CHANGE
     * broadcast is received with WIFI_AP_STATE_DISABLING and LOHS was active.
     */
    @Test
    public void testRegisteredCallbacksTriggeredOnSoftApDisabling() throws Exception {
        when(mFrameworkFacade.inStorageManagerCryptKeeperBounce()).thenReturn(false);
        when(mSettingsStore.isWifiToggleEnabled()).thenReturn(false);
        mWifiServiceImpl.checkAndStartWifi();

        verify(mContext).registerReceiver(mBroadcastReceiverCaptor.capture(),
                (IntentFilter) argThat(new IntentFilterMatcher()));

        registerLOHSRequestFull();

        mWifiServiceImpl.updateInterfaceIpState(WIFI_IFACE_NAME, IFACE_IP_MODE_LOCAL_ONLY);
        mLooper.dispatchAll();
        verify(mHandler).handleMessage(mMessageCaptor.capture());
        Message message = mMessageCaptor.getValue();
        assertEquals(HOTSPOT_STARTED, message.what);
        reset(mHandler);

        TestUtil.sendWifiApStateChanged(mBroadcastReceiverCaptor.getValue(), mContext,
                WIFI_AP_STATE_DISABLING, WIFI_AP_STATE_ENABLED, HOTSPOT_NO_ERROR,
                WIFI_IFACE_NAME, IFACE_IP_MODE_LOCAL_ONLY);

        mLooper.dispatchAll();
        verify(mHandler).handleMessage(mMessageCaptor.capture());
        message = mMessageCaptor.getValue();
        assertEquals(HOTSPOT_STOPPED, message.what);
    }


    /**
     * Verify that onStopped is called for registered LOHS callers when a WIFI_AP_STATE_CHANGE
     * broadcast is received with WIFI_AP_STATE_DISABLED and LOHS was enabled.
     */
    @Test
    public void testRegisteredCallbacksTriggeredOnSoftApDisabled() throws Exception {
        when(mFrameworkFacade.inStorageManagerCryptKeeperBounce()).thenReturn(false);
        when(mSettingsStore.isWifiToggleEnabled()).thenReturn(false);
        mWifiServiceImpl.checkAndStartWifi();

        verify(mContext).registerReceiver(mBroadcastReceiverCaptor.capture(),
                (IntentFilter) argThat(new IntentFilterMatcher()));

        registerLOHSRequestFull();

        mWifiServiceImpl.updateInterfaceIpState(WIFI_IFACE_NAME, IFACE_IP_MODE_LOCAL_ONLY);
        mLooper.dispatchAll();
        verify(mHandler).handleMessage(mMessageCaptor.capture());
        Message message = mMessageCaptor.getValue();
        assertEquals(HOTSPOT_STARTED, message.what);
        reset(mHandler);

        TestUtil.sendWifiApStateChanged(mBroadcastReceiverCaptor.getValue(), mContext,
                WIFI_AP_STATE_DISABLED, WIFI_AP_STATE_DISABLING, HOTSPOT_NO_ERROR,
                WIFI_IFACE_NAME, IFACE_IP_MODE_LOCAL_ONLY);

        mLooper.dispatchAll();
        verify(mHandler).handleMessage(mMessageCaptor.capture());
        message = mMessageCaptor.getValue();
        assertEquals(HOTSPOT_STOPPED, message.what);
    }

    /**
     * Verify that no callbacks are called for registered LOHS callers when a WIFI_AP_STATE_CHANGE
     * broadcast is received and the softap started.
     */
    @Test
    public void testRegisteredCallbacksNotTriggeredOnSoftApStart() throws Exception {
        when(mFrameworkFacade.inStorageManagerCryptKeeperBounce()).thenReturn(false);
        when(mSettingsStore.isWifiToggleEnabled()).thenReturn(false);
        mWifiServiceImpl.checkAndStartWifi();

        verify(mContext).registerReceiver(mBroadcastReceiverCaptor.capture(),
                (IntentFilter) argThat(new IntentFilterMatcher()));

        registerLOHSRequestFull();

        TestUtil.sendWifiApStateChanged(mBroadcastReceiverCaptor.getValue(), mContext,
                WIFI_AP_STATE_ENABLED, WIFI_AP_STATE_DISABLED, HOTSPOT_NO_ERROR, WIFI_IFACE_NAME,
                IFACE_IP_MODE_LOCAL_ONLY);

        mLooper.dispatchAll();
        verify(mHandler, never()).handleMessage(any(Message.class));
    }

    /**
     * Verify that onStopped is called only once for registered LOHS callers when
     * WIFI_AP_STATE_CHANGE broadcasts are received with WIFI_AP_STATE_DISABLING and
     * WIFI_AP_STATE_DISABLED when LOHS was enabled.
     */
    @Test
    public void testRegisteredCallbacksTriggeredOnlyOnceWhenSoftApDisabling() throws Exception {
        when(mFrameworkFacade.inStorageManagerCryptKeeperBounce()).thenReturn(false);
        when(mSettingsStore.isWifiToggleEnabled()).thenReturn(false);
        mWifiServiceImpl.checkAndStartWifi();

        verify(mContext).registerReceiver(mBroadcastReceiverCaptor.capture(),
                (IntentFilter) argThat(new IntentFilterMatcher()));

        registerLOHSRequestFull();

        mWifiServiceImpl.updateInterfaceIpState(WIFI_IFACE_NAME, IFACE_IP_MODE_LOCAL_ONLY);
        mLooper.dispatchAll();
        verify(mHandler).handleMessage(mMessageCaptor.capture());
        Message message = mMessageCaptor.getValue();
        assertEquals(HOTSPOT_STARTED, message.what);
        reset(mHandler);

        TestUtil.sendWifiApStateChanged(mBroadcastReceiverCaptor.getValue(), mContext,
                WIFI_AP_STATE_DISABLING, WIFI_AP_STATE_ENABLED, HOTSPOT_NO_ERROR,
                WIFI_IFACE_NAME, IFACE_IP_MODE_LOCAL_ONLY);
        TestUtil.sendWifiApStateChanged(mBroadcastReceiverCaptor.getValue(), mContext,
                WIFI_AP_STATE_DISABLED, WIFI_AP_STATE_DISABLING, HOTSPOT_NO_ERROR,
                WIFI_IFACE_NAME, IFACE_IP_MODE_LOCAL_ONLY);

        mLooper.dispatchAll();
        verify(mHandler).handleMessage(mMessageCaptor.capture());
        message = mMessageCaptor.getValue();
        assertEquals(HOTSPOT_STOPPED, message.what);
    }

    /**
     * Verify that onFailed is called only once for registered LOHS callers when
     * WIFI_AP_STATE_CHANGE broadcasts are received with WIFI_AP_STATE_FAILED twice.
     */
    @Test
    public void testRegisteredCallbacksTriggeredOnlyOnceWhenSoftApFailsTwice() throws Exception {
        when(mFrameworkFacade.inStorageManagerCryptKeeperBounce()).thenReturn(false);
        when(mSettingsStore.isWifiToggleEnabled()).thenReturn(false);
        mWifiServiceImpl.checkAndStartWifi();

        verify(mContext).registerReceiver(mBroadcastReceiverCaptor.capture(),
                (IntentFilter) argThat(new IntentFilterMatcher()));

        registerLOHSRequestFull();

        TestUtil.sendWifiApStateChanged(mBroadcastReceiverCaptor.getValue(), mContext,
                WIFI_AP_STATE_FAILED, WIFI_AP_STATE_FAILED, ERROR_GENERIC,
                WIFI_IFACE_NAME, IFACE_IP_MODE_LOCAL_ONLY);
        TestUtil.sendWifiApStateChanged(mBroadcastReceiverCaptor.getValue(), mContext,
                WIFI_AP_STATE_FAILED, WIFI_AP_STATE_FAILED, ERROR_GENERIC,
                WIFI_IFACE_NAME, IFACE_IP_MODE_LOCAL_ONLY);

        mLooper.dispatchAll();
        verify(mHandler).handleMessage(mMessageCaptor.capture());
        Message message = mMessageCaptor.getValue();
        assertEquals(HOTSPOT_FAILED, message.what);
        assertEquals(ERROR_GENERIC, message.arg1);
    }

    /**
     * Verify that onFailed is called for all registered LOHS callers when
     * WIFI_AP_STATE_CHANGE broadcasts are received with WIFI_AP_STATE_FAILED.
     */
    @Test
    public void testAllRegisteredCallbacksTriggeredWhenSoftApFails() throws Exception {
        when(mFrameworkFacade.inStorageManagerCryptKeeperBounce()).thenReturn(false);
        when(mSettingsStore.isWifiToggleEnabled()).thenReturn(false);
        mWifiServiceImpl.checkAndStartWifi();

        verify(mContext).registerReceiver(mBroadcastReceiverCaptor.capture(),
                (IntentFilter) argThat(new IntentFilterMatcher()));

        // make an additional request for this test
        mWifiServiceImpl.registerLOHSForTest(TEST_PID, mRequestInfo);

        registerLOHSRequestFull();

        TestUtil.sendWifiApStateChanged(mBroadcastReceiverCaptor.getValue(), mContext,
                WIFI_AP_STATE_FAILED, WIFI_AP_STATE_FAILED, ERROR_GENERIC,
                WIFI_IFACE_NAME, IFACE_IP_MODE_LOCAL_ONLY);
        TestUtil.sendWifiApStateChanged(mBroadcastReceiverCaptor.getValue(), mContext,
                WIFI_AP_STATE_FAILED, WIFI_AP_STATE_FAILED, ERROR_GENERIC,
                WIFI_IFACE_NAME, IFACE_IP_MODE_LOCAL_ONLY);

        verify(mRequestInfo).sendHotspotFailedMessage(ERROR_GENERIC);
        mLooper.dispatchAll();
        verify(mHandler).handleMessage(mMessageCaptor.capture());
        Message message = mMessageCaptor.getValue();
        assertEquals(HOTSPOT_FAILED, message.what);
        assertEquals(ERROR_GENERIC, message.arg1);
    }

    /**
     * Verify that onStopped is called for all registered LOHS callers when
     * WIFI_AP_STATE_CHANGE broadcasts are received with WIFI_AP_STATE_DISABLED when LOHS was
     * active.
     */
    @Test
    public void testAllRegisteredCallbacksTriggeredWhenSoftApStops() throws Exception {
        when(mFrameworkFacade.inStorageManagerCryptKeeperBounce()).thenReturn(false);
        when(mSettingsStore.isWifiToggleEnabled()).thenReturn(false);
        mWifiServiceImpl.checkAndStartWifi();

        verify(mContext).registerReceiver(mBroadcastReceiverCaptor.capture(),
                (IntentFilter) argThat(new IntentFilterMatcher()));

        mWifiServiceImpl.registerLOHSForTest(TEST_PID, mRequestInfo);

        registerLOHSRequestFull();

        mWifiServiceImpl.updateInterfaceIpState(WIFI_IFACE_NAME, IFACE_IP_MODE_LOCAL_ONLY);
        mLooper.dispatchAll();
        verify(mRequestInfo).sendHotspotStartedMessage(any());
        verify(mHandler).handleMessage(mMessageCaptor.capture());
        Message message = mMessageCaptor.getValue();
        assertEquals(HOTSPOT_STARTED, message.what);
        reset(mHandler);

        TestUtil.sendWifiApStateChanged(mBroadcastReceiverCaptor.getValue(), mContext,
                WIFI_AP_STATE_DISABLING, WIFI_AP_STATE_ENABLED, HOTSPOT_NO_ERROR,
                WIFI_IFACE_NAME, IFACE_IP_MODE_LOCAL_ONLY);
        TestUtil.sendWifiApStateChanged(mBroadcastReceiverCaptor.getValue(), mContext,
                WIFI_AP_STATE_DISABLED, WIFI_AP_STATE_DISABLING, HOTSPOT_NO_ERROR,
                WIFI_IFACE_NAME, IFACE_IP_MODE_LOCAL_ONLY);

        verify(mRequestInfo).sendHotspotStoppedMessage();
        mLooper.dispatchAll();
        verify(mHandler).handleMessage(mMessageCaptor.capture());
        message = mMessageCaptor.getValue();
        assertEquals(HOTSPOT_STOPPED, message.what);
    }

    /**
     * Verify that onFailed is called for all registered LOHS callers when
     * WIFI_AP_STATE_CHANGE broadcasts are received with WIFI_AP_STATE_DISABLED when LOHS was
     * not active.
     */
    @Test
    public void testAllRegisteredCallbacksTriggeredWhenSoftApStopsLOHSNotActive() throws Exception {
        when(mFrameworkFacade.inStorageManagerCryptKeeperBounce()).thenReturn(false);
        when(mSettingsStore.isWifiToggleEnabled()).thenReturn(false);
        mWifiServiceImpl.checkAndStartWifi();

        verify(mContext).registerReceiver(mBroadcastReceiverCaptor.capture(),
                (IntentFilter) argThat(new IntentFilterMatcher()));

        mWifiServiceImpl.registerLOHSForTest(TEST_PID, mRequestInfo);
        mWifiServiceImpl.registerLOHSForTest(TEST_PID2, mRequestInfo2);

        TestUtil.sendWifiApStateChanged(mBroadcastReceiverCaptor.getValue(), mContext,
                WIFI_AP_STATE_DISABLING, WIFI_AP_STATE_ENABLED, HOTSPOT_NO_ERROR,
                WIFI_IFACE_NAME, IFACE_IP_MODE_LOCAL_ONLY);
        TestUtil.sendWifiApStateChanged(mBroadcastReceiverCaptor.getValue(), mContext,
                WIFI_AP_STATE_DISABLED, WIFI_AP_STATE_DISABLING, HOTSPOT_NO_ERROR,
                WIFI_IFACE_NAME, IFACE_IP_MODE_LOCAL_ONLY);

        verify(mRequestInfo).sendHotspotFailedMessage(ERROR_GENERIC);
        verify(mRequestInfo2).sendHotspotFailedMessage(ERROR_GENERIC);
    }

    /**
     * Verify that if we do not have registered LOHS requestors and we receive an update that LOHS
     * is up and ready for use, we tell WifiController to tear it down.  This can happen if softap
     * mode fails to come up properly and we get an onFailed message for a tethering call and we
     * had registered callers for LOHS.
     */
    @Test
    public void testLOHSReadyWithoutRegisteredRequestsStopsSoftApMode() {
        mWifiServiceImpl.updateInterfaceIpState(WIFI_IFACE_NAME, IFACE_IP_MODE_LOCAL_ONLY);
        mLooper.dispatchAll();

        verify(mWifiController).sendMessage(eq(CMD_SET_AP), eq(0), eq(0));
    }

    /**
     * Verify that all registered LOHS requestors are notified via a HOTSPOT_STARTED message that
     * the hotspot is up and ready to use.
     */
    @Test
    public void testRegisteredLocalOnlyHotspotRequestorsGetOnStartedCallbackWhenReady()
            throws Exception {
        registerLOHSRequestFull();

        mWifiServiceImpl.registerLOHSForTest(TEST_PID, mRequestInfo);

        mWifiServiceImpl.updateInterfaceIpState(WIFI_IFACE_NAME, IFACE_IP_MODE_LOCAL_ONLY);
        mLooper.dispatchAll();
        verify(mRequestInfo).sendHotspotStartedMessage(any(WifiConfiguration.class));

        mLooper.dispatchAll();
        verify(mHandler).handleMessage(mMessageCaptor.capture());
        Message message = mMessageCaptor.getValue();
        assertEquals(HOTSPOT_STARTED, message.what);
        assertNotNull((WifiConfiguration) message.obj);
    }

    /**
     * Verify that if a LOHS is already active, a new call to register a request will trigger the
     * onStarted callback.
     */
    @Test
    public void testRegisterLocalOnlyHotspotRequestAfterAlreadyStartedGetsOnStartedCallback()
            throws Exception {
        mWifiServiceImpl.registerLOHSForTest(TEST_PID, mRequestInfo);

        mWifiServiceImpl.updateInterfaceIpState(WIFI_IFACE_NAME, IFACE_IP_MODE_LOCAL_ONLY);
        mLooper.dispatchAll();

        registerLOHSRequestFull();

        mLooper.dispatchAll();

        verify(mHandler).handleMessage(mMessageCaptor.capture());
        Message message = mMessageCaptor.getValue();
        assertEquals(HOTSPOT_STARTED, message.what);
        // since the first request was registered out of band, the config will be null
        assertNull((WifiConfiguration) message.obj);
    }

    /**
     * Verify that if a LOHS request is active and we receive an update with an ip mode
     * configuration error, callers are notified via the onFailed callback with the generic
     * error and are unregistered.
     */
    @Test
    public void testCallOnFailedLocalOnlyHotspotRequestWhenIpConfigFails() throws Exception {
        registerLOHSRequestFull();

        mWifiServiceImpl.updateInterfaceIpState(WIFI_IFACE_NAME, IFACE_IP_MODE_CONFIGURATION_ERROR);
        mLooper.dispatchAll();

        verify(mHandler).handleMessage(mMessageCaptor.capture());
        Message message = mMessageCaptor.getValue();
        assertEquals(HOTSPOT_FAILED, message.what);
        assertEquals(ERROR_GENERIC, message.arg1);

        verify(mWifiController, never()).sendMessage(eq(CMD_SET_AP), eq(0), eq(0));

        // sendMessage should only happen once since the requestor should be unregistered
        reset(mHandler);

        // send HOTSPOT_FAILED message should only happen once since the requestor should be
        // unregistered
        mWifiServiceImpl.updateInterfaceIpState(WIFI_IFACE_NAME, IFACE_IP_MODE_CONFIGURATION_ERROR);
        mLooper.dispatchAll();
        verify(mHandler, never()).handleMessage(any(Message.class));
    }

    /**
     * Verify that softap mode is stopped for tethering if we receive an update with an ip mode
     * configuration error.
     */
    @Test
    public void testStopSoftApWhenIpConfigFails() throws Exception {
        mWifiServiceImpl.updateInterfaceIpState(WIFI_IFACE_NAME, IFACE_IP_MODE_CONFIGURATION_ERROR);
        mLooper.dispatchAll();

        verify(mWifiController).sendMessage(eq(CMD_SET_AP), eq(0), eq(0));
    }

    /**
     * Verify that if a LOHS request is active and tethering starts, callers are notified on the
     * incompatible mode and are unregistered.
     */
    @Test
    public void testCallOnFailedLocalOnlyHotspotRequestWhenTetheringStarts() throws Exception {
        registerLOHSRequestFull();

        mWifiServiceImpl.updateInterfaceIpState(WIFI_IFACE_NAME, IFACE_IP_MODE_TETHERED);
        mLooper.dispatchAll();

        verify(mHandler).handleMessage(mMessageCaptor.capture());
        Message message = mMessageCaptor.getValue();
        assertEquals(HOTSPOT_FAILED, message.what);
        assertEquals(ERROR_INCOMPATIBLE_MODE, message.arg1);

        // sendMessage should only happen once since the requestor should be unregistered
        reset(mHandler);

        mWifiServiceImpl.updateInterfaceIpState(WIFI_IFACE_NAME, IFACE_IP_MODE_TETHERED);
        mLooper.dispatchAll();
        verify(mHandler, never()).handleMessage(any(Message.class));
    }

    /**
     * Verify that if LOHS is disabled, a new call to register a request will not trigger the
     * onStopped callback.
     */
    @Test
    public void testRegisterLocalOnlyHotspotRequestWhenStoppedDoesNotGetOnStoppedCallback()
            throws Exception {
        registerLOHSRequestFull();
        mLooper.dispatchAll();

        verify(mHandler, never()).handleMessage(any(Message.class));
    }

    /**
     * Verify that if a LOHS was active and then stopped, a new call to register a request will
     * not trigger the onStarted callback.
     */
    @Test
    public void testRegisterLocalOnlyHotspotRequestAfterStoppedNoOnStartedCallback()
            throws Exception {
        when(mFrameworkFacade.inStorageManagerCryptKeeperBounce()).thenReturn(false);
        when(mSettingsStore.isWifiToggleEnabled()).thenReturn(false);
        mWifiServiceImpl.checkAndStartWifi();
        verify(mContext).registerReceiver(mBroadcastReceiverCaptor.capture(),
                (IntentFilter) argThat(new IntentFilterMatcher()));

        // register a request so we don't drop the LOHS interface ip update
        mWifiServiceImpl.registerLOHSForTest(TEST_PID, mRequestInfo);

        mWifiServiceImpl.updateInterfaceIpState(WIFI_IFACE_NAME, IFACE_IP_MODE_LOCAL_ONLY);
        mLooper.dispatchAll();

        registerLOHSRequestFull();
        mLooper.dispatchAll();

        verify(mHandler).handleMessage(mMessageCaptor.capture());
        assertEquals(HOTSPOT_STARTED, mMessageCaptor.getValue().what);

        reset(mHandler);

        // now stop the hotspot
        TestUtil.sendWifiApStateChanged(mBroadcastReceiverCaptor.getValue(), mContext,
                WIFI_AP_STATE_DISABLING, WIFI_AP_STATE_ENABLED, HOTSPOT_NO_ERROR,
                WIFI_IFACE_NAME, IFACE_IP_MODE_LOCAL_ONLY);
        TestUtil.sendWifiApStateChanged(mBroadcastReceiverCaptor.getValue(), mContext,
                WIFI_AP_STATE_DISABLED, WIFI_AP_STATE_DISABLING, HOTSPOT_NO_ERROR,
                WIFI_IFACE_NAME, IFACE_IP_MODE_LOCAL_ONLY);
        mLooper.dispatchAll();
        verify(mHandler).handleMessage(mMessageCaptor.capture());
        assertEquals(HOTSPOT_STOPPED, mMessageCaptor.getValue().what);

        reset(mHandler);

        // now register a new caller - they should not get the onStarted callback
        Messenger messenger2 = new Messenger(mHandler);
        IBinder binder2 = mock(IBinder.class);

        int result = mWifiServiceImpl.startLocalOnlyHotspot(messenger2, binder2, TEST_PACKAGE_NAME);
        assertEquals(LocalOnlyHotspotCallback.REQUEST_REGISTERED, result);
        mLooper.dispatchAll();

        verify(mHandler, never()).handleMessage(any(Message.class));
    }

    /**
     * Verify that a call to startWatchLocalOnlyHotspot is only allowed from callers with the
     * signature only NETWORK_SETTINGS permission.
     *
     * This test is expecting the permission check to enforce the permission and throw a
     * SecurityException for callers without the permission.  This exception should be bubbled up to
     * the caller of startLocalOnlyHotspot.
     */
    @Test(expected = SecurityException.class)
    public void testStartWatchLocalOnlyHotspotNotApprovedCaller() {
        doThrow(new SecurityException()).when(mContext)
                .enforceCallingOrSelfPermission(eq(android.Manifest.permission.NETWORK_SETTINGS),
                                                eq("WifiService"));
        mWifiServiceImpl.startWatchLocalOnlyHotspot(mAppMessenger, mAppBinder);
    }

    /**
     * Verify that the call to startWatchLocalOnlyHotspot throws the UnsupportedOperationException
     * when called until the implementation is complete.
     */
    @Test(expected = UnsupportedOperationException.class)
    public void testStartWatchLocalOnlyHotspotNotSupported() {
        mWifiServiceImpl.startWatchLocalOnlyHotspot(mAppMessenger, mAppBinder);
    }

    /**
     * Verify that a call to stopWatchLocalOnlyHotspot is only allowed from callers with the
     * signature only NETWORK_SETTINGS permission.
     */
    @Test(expected = SecurityException.class)
    public void testStopWatchLocalOnlyHotspotNotApprovedCaller() {
        doThrow(new SecurityException()).when(mContext)
                .enforceCallingOrSelfPermission(eq(android.Manifest.permission.NETWORK_SETTINGS),
                                                eq("WifiService"));
        mWifiServiceImpl.stopWatchLocalOnlyHotspot();
    }

    /**
     * Verify that the call to stopWatchLocalOnlyHotspot throws the UnsupportedOperationException
     * until the implementation is complete.
     */
    @Test(expected = UnsupportedOperationException.class)
    public void testStopWatchLocalOnlyHotspotNotSupported() {
        mWifiServiceImpl.stopWatchLocalOnlyHotspot();
    }

    /**
     * Verify that the call to addOrUpdateNetwork for installing Passpoint profile is redirected
     * to the Passpoint specific API addOrUpdatePasspointConfiguration.
     */
    @Test
    public void testAddPasspointProfileViaAddNetwork() throws Exception {
        WifiConfiguration config = WifiConfigurationTestUtil.createPasspointNetwork();
        config.enterpriseConfig.setEapMethod(WifiEnterpriseConfig.Eap.TLS);

        PackageManager pm = mock(PackageManager.class);
        when(pm.hasSystemFeature(PackageManager.FEATURE_WIFI_PASSPOINT)).thenReturn(true);
        when(mContext.getPackageManager()).thenReturn(pm);

        when(mWifiStateMachine.syncAddOrUpdatePasspointConfig(any(),
                any(PasspointConfiguration.class), anyInt())).thenReturn(true);
        assertEquals(0, mWifiServiceImpl.addOrUpdateNetwork(config, TEST_PACKAGE_NAME));
        verifyCheckChangePermission(TEST_PACKAGE_NAME);
        verify(mWifiStateMachine).syncAddOrUpdatePasspointConfig(any(),
                any(PasspointConfiguration.class), anyInt());
        reset(mWifiStateMachine);

        when(mWifiStateMachine.syncAddOrUpdatePasspointConfig(any(),
                any(PasspointConfiguration.class), anyInt())).thenReturn(false);
        assertEquals(-1, mWifiServiceImpl.addOrUpdateNetwork(config, TEST_PACKAGE_NAME));
        verify(mWifiStateMachine).syncAddOrUpdatePasspointConfig(any(),
                any(PasspointConfiguration.class), anyInt());
    }

    /**
     * Verify that the call to startSubscriptionProvisioning is redirected to the Passpoint
     * specific API startSubscriptionProvisioning when the caller has the right permissions.
     */
    @Test
    public void testStartSubscriptionProvisioningWithPermission() throws Exception {
        mWifiServiceImpl.startSubscriptionProvisioning(mOsuProvider, mProvisioningCallback);
        verify(mWifiStateMachine).syncStartSubscriptionProvisioning(anyInt(),
                eq(mOsuProvider), eq(mProvisioningCallback), any());
    }

    /**
     * Verify that the call to startSubscriptionProvisioning is not directed to the Passpoint
     * specific API startSubscriptionProvisioning when the feature is not supported.
     */
    @Test(expected = UnsupportedOperationException.class)
    public void testStartSubscriptionProvisioniningPasspointUnsupported() throws Exception {
        when(mPackageManager.hasSystemFeature(
                PackageManager.FEATURE_WIFI_PASSPOINT)).thenReturn(false);
        mWifiServiceImpl.startSubscriptionProvisioning(mOsuProvider, mProvisioningCallback);
    }

    /**
     * Verify that the call to startSubscriptionProvisioning is not redirected to the Passpoint
     * specific API startSubscriptionProvisioning when the caller provides invalid arguments
     */
    @Test(expected = IllegalArgumentException.class)
    public void testStartSubscriptionProvisioningWithInvalidProvider() throws Exception {
        mWifiServiceImpl.startSubscriptionProvisioning(null, mProvisioningCallback);
    }


    /**
     * Verify that the call to startSubscriptionProvisioning is not redirected to the Passpoint
     * specific API startSubscriptionProvisioning when the caller provides invalid callback
     */
    @Test(expected = IllegalArgumentException.class)
    public void testStartSubscriptionProvisioningWithInvalidCallback() throws Exception {
        mWifiServiceImpl.startSubscriptionProvisioning(mOsuProvider, null);
    }

    /**
     * Verify that the call to startSubscriptionProvisioning is not redirected to the Passpoint
     * specific API startSubscriptionProvisioning when the caller doesn't have NETWORK_SETTINGS
     * permissions.
     */
    @Test(expected = SecurityException.class)
    public void testStartSubscriptionProvisioningWithoutPermission() throws Exception {
        doThrow(new SecurityException()).when(mContext)
                .enforceCallingOrSelfPermission(eq(android.Manifest.permission.NETWORK_SETTINGS),
                        eq("WifiService"));
        mWifiServiceImpl.startSubscriptionProvisioning(mOsuProvider, mProvisioningCallback);
    }

    /**
     * Verify that a call to {@link WifiServiceImpl#restoreBackupData(byte[])} is only allowed from
     * callers with the signature only NETWORK_SETTINGS permission.
     */
    @Test(expected = SecurityException.class)
    public void testRestoreBackupDataNotApprovedCaller() {
        doThrow(new SecurityException()).when(mContext)
                .enforceCallingOrSelfPermission(eq(android.Manifest.permission.NETWORK_SETTINGS),
                        eq("WifiService"));
        mWifiServiceImpl.restoreBackupData(null);
        verify(mWifiBackupRestore, never()).retrieveConfigurationsFromBackupData(any(byte[].class));
    }

    /**
     * Verify that a call to {@link WifiServiceImpl#restoreSupplicantBackupData(byte[], byte[])} is
     * only allowed from callers with the signature only NETWORK_SETTINGS permission.
     */
    @Ignore
    @Test(expected = SecurityException.class)
    public void testRestoreSupplicantBackupDataNotApprovedCaller() {
        doThrow(new SecurityException()).when(mContext)
                .enforceCallingOrSelfPermission(eq(android.Manifest.permission.NETWORK_SETTINGS),
                        eq("WifiService"));
        mWifiServiceImpl.restoreSupplicantBackupData(null, null);
        verify(mWifiBackupRestore, never()).retrieveConfigurationsFromSupplicantBackupData(
                any(byte[].class), any(byte[].class));
    }

    /**
     * Verify that a call to {@link WifiServiceImpl#retrieveBackupData()} is only allowed from
     * callers with the signature only NETWORK_SETTINGS permission.
     */
    @Test(expected = SecurityException.class)
    public void testRetrieveBackupDataNotApprovedCaller() {
        doThrow(new SecurityException()).when(mContext)
                .enforceCallingOrSelfPermission(eq(android.Manifest.permission.NETWORK_SETTINGS),
                        eq("WifiService"));
        mWifiServiceImpl.retrieveBackupData();
        verify(mWifiBackupRestore, never()).retrieveBackupDataFromConfigurations(any(List.class));
    }

    /**
     * Verify that a call to {@link WifiServiceImpl#enableVerboseLogging(int)} is allowed from
     * callers with the signature only NETWORK_SETTINGS permission.
     */
    @Ignore("TODO: Investigate failure")
    @Test
    public void testEnableVerboseLoggingWithNetworkSettingsPermission() {
        doNothing().when(mContext)
                .enforceCallingOrSelfPermission(eq(android.Manifest.permission.NETWORK_SETTINGS),
                        eq("WifiService"));
        mWifiServiceImpl.enableVerboseLogging(1);
        verify(mWifiStateMachine).enableVerboseLogging(anyInt());
    }

    /**
     * Verify that a call to {@link WifiServiceImpl#enableVerboseLogging(int)} is not allowed from
     * callers without the signature only NETWORK_SETTINGS permission.
     */
    @Test(expected = SecurityException.class)
    public void testEnableVerboseLoggingWithNoNetworkSettingsPermission() {
        doThrow(new SecurityException()).when(mContext)
                .enforceCallingOrSelfPermission(eq(android.Manifest.permission.NETWORK_SETTINGS),
                        eq("WifiService"));
        mWifiServiceImpl.enableVerboseLogging(1);
        verify(mWifiStateMachine, never()).enableVerboseLogging(anyInt());
    }

    /**
     * Helper to test handling of async messages by wifi service when the message comes from an
     * app without {@link android.Manifest.permission#CHANGE_WIFI_STATE} permission.
     */
    private void verifyAsyncChannelMessageHandlingWithoutChangePermisson(
            int requestMsgWhat, int expectedReplyMsgwhat) throws RemoteException {
        WifiAsyncChannelTester tester = verifyAsyncChannelHalfConnected();

        int uidWithoutPermission = 5;
        when(mWifiPermissionsUtil.checkChangePermission(eq(uidWithoutPermission)))
                .thenReturn(false);

        Message request = Message.obtain();
        request.what = requestMsgWhat;
        request.sendingUid = uidWithoutPermission;

        mLooper.startAutoDispatch();
        Message reply = tester.sendMessageSynchronously(request);
        mLooper.stopAutoDispatch();

        verify(mWifiStateMachine, never()).sendMessage(any(Message.class));
        assertEquals(expectedReplyMsgwhat, reply.what);
        assertEquals(WifiManager.NOT_AUTHORIZED, reply.arg1);
    }

    /**
     * Verify that the CONNECT_NETWORK message received from an app without
     * {@link android.Manifest.permission#CHANGE_WIFI_STATE} permission is rejected with the correct
     * error code.
     */
    @Test
    public void testConnectNetworkWithoutChangePermission() throws Exception {
        verifyAsyncChannelMessageHandlingWithoutChangePermisson(
                WifiManager.CONNECT_NETWORK, WifiManager.CONNECT_NETWORK_FAILED);
    }

    /**
     * Verify that the FORGET_NETWORK message received from an app without
     * {@link android.Manifest.permission#CHANGE_WIFI_STATE} permission is rejected with the correct
     * error code.
     */
    @Test
    public void testForgetNetworkWithoutChangePermission() throws Exception {
        verifyAsyncChannelMessageHandlingWithoutChangePermisson(
                WifiManager.SAVE_NETWORK, WifiManager.SAVE_NETWORK_FAILED);
    }

    /**
     * Verify that the START_WPS message received from an app without
     * {@link android.Manifest.permission#CHANGE_WIFI_STATE} permission is rejected with the correct
     * error code.
     */
    @Test
    public void testStartWpsWithoutChangePermission() throws Exception {
        verifyAsyncChannelMessageHandlingWithoutChangePermisson(
                WifiManager.START_WPS, WifiManager.WPS_FAILED);
    }

    /**
     * Verify that the CANCEL_WPS message received from an app without
     * {@link android.Manifest.permission#CHANGE_WIFI_STATE} permission is rejected with the correct
     * error code.
     */
    @Test
    public void testCancelWpsWithoutChangePermission() throws Exception {
        verifyAsyncChannelMessageHandlingWithoutChangePermisson(
                WifiManager.CANCEL_WPS, WifiManager.CANCEL_WPS_FAILED);
    }

    /**
     * Verify that the DISABLE_NETWORK message received from an app without
     * {@link android.Manifest.permission#CHANGE_WIFI_STATE} permission is rejected with the correct
     * error code.
     */
    @Test
    public void testDisableNetworkWithoutChangePermission() throws Exception {
        verifyAsyncChannelMessageHandlingWithoutChangePermisson(
                WifiManager.DISABLE_NETWORK, WifiManager.DISABLE_NETWORK_FAILED);
    }

    /**
     * Verify that the RSSI_PKTCNT_FETCH message received from an app without
     * {@link android.Manifest.permission#CHANGE_WIFI_STATE} permission is rejected with the correct
     * error code.
     */
    @Test
    public void testRssiPktcntFetchWithoutChangePermission() throws Exception {
        verifyAsyncChannelMessageHandlingWithoutChangePermisson(
                WifiManager.RSSI_PKTCNT_FETCH, WifiManager.RSSI_PKTCNT_FETCH_FAILED);
    }

    /**
     * Helper to test handling of async messages by wifi service when the message comes from an
     * app with {@link android.Manifest.permission#CHANGE_WIFI_STATE} permission.
     */
    private void verifyAsyncChannelMessageHandlingWithChangePermisson(
            int requestMsgWhat, Object requestMsgObj) throws RemoteException {
        WifiAsyncChannelTester tester = verifyAsyncChannelHalfConnected();

        when(mWifiPermissionsUtil.checkChangePermission(anyInt())).thenReturn(true);

        Message request = Message.obtain();
        request.what = requestMsgWhat;
        request.obj = requestMsgObj;

        tester.sendMessage(request);
        mLooper.dispatchAll();

        ArgumentCaptor<Message> messageArgumentCaptor = ArgumentCaptor.forClass(Message.class);
        verify(mWifiStateMachine).sendMessage(messageArgumentCaptor.capture());
        assertEquals(requestMsgWhat, messageArgumentCaptor.getValue().what);
    }

     /**
     * Helper to test handling of async messages by wifi service when the message comes from an
     * app with {@link android.Manifest.permission#CHANGE_WIFI_STATE} permission where we
     * immediately return an error for deprecated functionality.
     */
    private void verifyAsyncChannelDeprecatedMessageHandlingNotSentToWSMWithChangePermisson(
            int requestMsgWhat, Object requestMsgObj) throws Exception {
        WifiAsyncChannelTester tester = verifyAsyncChannelHalfConnected();

        when(mWifiPermissionsUtil.checkChangePermission(anyInt())).thenReturn(true);

        Message request = Message.obtain();
        request.what = requestMsgWhat;
        request.obj = requestMsgObj;

        tester.sendMessage(request);
        mLooper.dispatchAll();

        verify(mWifiStateMachine, never()).sendMessage(any());
    }

    /**
     * Verify that the CONNECT_NETWORK message received from an app with
     * {@link android.Manifest.permission#CHANGE_WIFI_STATE} permission is forwarded to
     * WifiStateMachine.
     */
    @Test
    public void testConnectNetworkWithChangePermission() throws Exception {
        verifyAsyncChannelMessageHandlingWithChangePermisson(
                WifiManager.CONNECT_NETWORK, new WifiConfiguration());
    }

    /**
     * Verify that the SAVE_NETWORK message received from an app with
     * {@link android.Manifest.permission#CHANGE_WIFI_STATE} permission is forwarded to
     * WifiStateMachine.
     */
    @Test
    public void testSaveNetworkWithChangePermission() throws Exception {
        verifyAsyncChannelMessageHandlingWithChangePermisson(
                WifiManager.SAVE_NETWORK, new WifiConfiguration());
    }

    /**
     * Verify that the START_WPS message received from an app with
     * {@link android.Manifest.permission#CHANGE_WIFI_STATE} permission is forwarded to
     * WifiStateMachine.
     */
    @Test
    public void testStartWpsWithChangePermission() throws Exception {
        verifyAsyncChannelDeprecatedMessageHandlingNotSentToWSMWithChangePermisson(
                WifiManager.START_WPS, new Object());
    }

    /**
     * Verify that the CANCEL_WPS message received from an app with
     * {@link android.Manifest.permission#CHANGE_WIFI_STATE} permission is forwarded to
     * WifiStateMachine.
     */
    @Test
    public void testCancelWpsWithChangePermission() throws Exception {
        verifyAsyncChannelDeprecatedMessageHandlingNotSentToWSMWithChangePermisson(
                WifiManager.CANCEL_WPS, new Object());
    }

    /**
     * Verify that the DISABLE_NETWORK message received from an app with
     * {@link android.Manifest.permission#CHANGE_WIFI_STATE} permission is forwarded to
     * WifiStateMachine.
     */
    @Test
    public void testDisableNetworkWithChangePermission() throws Exception {
        verifyAsyncChannelMessageHandlingWithChangePermisson(
                WifiManager.DISABLE_NETWORK, new Object());
    }

    /**
     * Verify that the RSSI_PKTCNT_FETCH message received from an app with
     * {@link android.Manifest.permission#CHANGE_WIFI_STATE} permission is forwarded to
     * WifiStateMachine.
     */
    @Test
    public void testRssiPktcntFetchWithChangePermission() throws Exception {
        verifyAsyncChannelMessageHandlingWithChangePermisson(
                WifiManager.RSSI_PKTCNT_FETCH, new Object());
    }

    /**
     * Verify that setCountryCode() calls WifiCountryCode object on succeess.
     */
    @Test
    public void testSetCountryCode() throws Exception {
        mWifiServiceImpl.setCountryCode(TEST_COUNTRY_CODE);
        verify(mWifiCountryCode).setCountryCode(TEST_COUNTRY_CODE);
    }

    /**
     * Verify that setCountryCode() fails and doesn't call WifiCountryCode object
     * if the caller doesn't have CONNECTIVITY_INTERNAL permission.
     */
    @Test(expected = SecurityException.class)
    public void testSetCountryCodeFailsWithoutConnectivityInternalPermission() throws Exception {
        doThrow(new SecurityException()).when(mContext)
                .enforceCallingOrSelfPermission(
                        eq(android.Manifest.permission.CONNECTIVITY_INTERNAL),
                        eq("ConnectivityService"));
        mWifiServiceImpl.setCountryCode(TEST_COUNTRY_CODE);
        verify(mWifiCountryCode, never()).setCountryCode(TEST_COUNTRY_CODE);
    }

    /**
     * Set the wifi state machine mock to return a handler created on test thread.
     */
    private void setupWifiStateMachineHandlerForRunWithScissors() {
        HandlerThread handlerThread = createAndStartHandlerThreadForRunWithScissors();
        mHandlerSpyForWsmRunWithScissors = spy(handlerThread.getThreadHandler());
        when(mWifiInjector.getWifiStateMachineHandler())
                .thenReturn(mHandlerSpyForWsmRunWithScissors);
    }

    private HandlerThread createAndStartHandlerThreadForRunWithScissors() {
        HandlerThread handlerThread = new HandlerThread("ServiceHandlerThreadForTest");
        handlerThread.start();
        return handlerThread;
    }

    /**
     * Tests the scenario when a scan request arrives while the device is idle. In this case
     * the scan is done when idle mode ends.
     */
    @Test
    public void testHandleDelayedScanAfterIdleMode() throws Exception {
        setupWifiStateMachineHandlerForRunWithScissors();
        when(mFrameworkFacade.inStorageManagerCryptKeeperBounce()).thenReturn(false);
        when(mSettingsStore.isWifiToggleEnabled()).thenReturn(false);
        mWifiServiceImpl.checkAndStartWifi();
        verify(mContext).registerReceiver(mBroadcastReceiverCaptor.capture(),
                (IntentFilter) argThat(new IdleModeIntentMatcher()));

        // Tell the wifi service that the device became idle.
        when(mPowerManager.isDeviceIdleMode()).thenReturn(true);
        TestUtil.sendIdleModeChanged(mBroadcastReceiverCaptor.getValue(), mContext);

        // Send a scan request while the device is idle.
        assertFalse(mWifiServiceImpl.startScan(SCAN_PACKAGE_NAME));
        // No scans must be made yet as the device is idle.
        verify(mScanRequestProxy, never()).startScan(Process.myUid(), SCAN_PACKAGE_NAME);

        // Tell the wifi service that idle mode ended.
        when(mPowerManager.isDeviceIdleMode()).thenReturn(false);
        TestUtil.sendIdleModeChanged(mBroadcastReceiverCaptor.getValue(), mContext);

        // Must scan now.
        verify(mScanRequestProxy).startScan(Process.myUid(), TEST_PACKAGE_NAME);
        // The app ops check is executed with this package's identity (not the identity of the
        // original remote caller who requested the scan while idle).
        verify(mAppOpsManager).noteOp(
                AppOpsManager.OPSTR_CHANGE_WIFI_STATE, Process.myUid(), TEST_PACKAGE_NAME);

        // Send another scan request. The device is not idle anymore, so it must be executed
        // immediately.
        assertTrue(mWifiServiceImpl.startScan(SCAN_PACKAGE_NAME));
        verify(mScanRequestProxy).startScan(Process.myUid(), SCAN_PACKAGE_NAME);
    }

    /**
     * Verify that if the caller has NETWORK_SETTINGS permission, then it doesn't need
     * CHANGE_WIFI_STATE permission.
     * @throws Exception
     */
    @Test
    public void testDisconnectWithNetworkSettingsPerm() throws Exception {
        when(mContext.checkPermission(eq(android.Manifest.permission.NETWORK_SETTINGS),
                anyInt(), anyInt())).thenReturn(PackageManager.PERMISSION_GRANTED);
        doThrow(new SecurityException()).when(mContext).enforceCallingOrSelfPermission(
                android.Manifest.permission.CHANGE_WIFI_STATE, "WifiService");
        doThrow(new SecurityException()).when(mAppOpsManager)
                .noteOp(AppOpsManager.OPSTR_CHANGE_WIFI_STATE, Process.myUid(), TEST_PACKAGE_NAME);
        mWifiServiceImpl.disconnect(TEST_PACKAGE_NAME);
        verify(mWifiStateMachine).disconnectCommand();
    }

    /**
     * Verify that if the caller doesn't have NETWORK_SETTINGS permission, it could still
     * get access with the CHANGE_WIFI_STATE permission.
     * @throws Exception
     */
    @Test
    public void testDisconnectWithChangeWifiStatePerm() throws Exception {
        mWifiServiceImpl.disconnect(TEST_PACKAGE_NAME);
        verifyCheckChangePermission(TEST_PACKAGE_NAME);
        verify(mWifiStateMachine).disconnectCommand();
    }

    /**
     * Verify that the operation fails if the caller has neither NETWORK_SETTINGS or
     * CHANGE_WIFI_STATE permissions.
     * @throws Exception
     */
    @Test
    public void testDisconnectRejected() throws Exception {
        doThrow(new SecurityException()).when(mAppOpsManager)
                .noteOp(AppOpsManager.OPSTR_CHANGE_WIFI_STATE, Process.myUid(), TEST_PACKAGE_NAME);
        try {
            mWifiServiceImpl.disconnect(TEST_PACKAGE_NAME);
            fail();
        } catch (SecurityException e) {

        }
        verifyCheckChangePermission(TEST_PACKAGE_NAME);
        verify(mWifiStateMachine, never()).disconnectCommand();
    }

    @Test
    public void testPackageRemovedBroadcastHandling() {
        mWifiServiceImpl.checkAndStartWifi();
        verify(mContext).registerReceiver(mBroadcastReceiverCaptor.capture(),
                (IntentFilter) argThat((IntentFilter filter) ->
                        filter.hasAction(Intent.ACTION_PACKAGE_FULLY_REMOVED)));

        int uid = TEST_UID;
        String packageName = TEST_PACKAGE_NAME;
        // Send the broadcast
        Intent intent = new Intent(Intent.ACTION_PACKAGE_FULLY_REMOVED);
        intent.putExtra(Intent.EXTRA_UID, uid);
        intent.setData(Uri.fromParts("package", packageName, ""));
        mBroadcastReceiverCaptor.getValue().onReceive(mContext, intent);

        verify(mWifiStateMachine).removeAppConfigs(packageName, uid);

        mLooper.dispatchAll();
        verify(mScanRequestProxy).clearScanRequestTimestampsForApp(packageName, uid);
    }

    @Test
    public void testPackageRemovedBroadcastHandlingWithNoUid() {
        mWifiServiceImpl.checkAndStartWifi();
        verify(mContext).registerReceiver(mBroadcastReceiverCaptor.capture(),
                (IntentFilter) argThat((IntentFilter filter) ->
                        filter.hasAction(Intent.ACTION_PACKAGE_FULLY_REMOVED)));

        String packageName = TEST_PACKAGE_NAME;
        // Send the broadcast
        Intent intent = new Intent(Intent.ACTION_PACKAGE_FULLY_REMOVED);
        intent.setData(Uri.fromParts("package", packageName, ""));
        mBroadcastReceiverCaptor.getValue().onReceive(mContext, intent);

        verify(mWifiStateMachine, never()).removeAppConfigs(anyString(), anyInt());

        mLooper.dispatchAll();
        verify(mScanRequestProxy, never()).clearScanRequestTimestampsForApp(anyString(), anyInt());
    }

    @Test
    public void testPackageRemovedBroadcastHandlingWithNoPackageName() {
        mWifiServiceImpl.checkAndStartWifi();
        verify(mContext).registerReceiver(mBroadcastReceiverCaptor.capture(),
                (IntentFilter) argThat((IntentFilter filter) ->
                        filter.hasAction(Intent.ACTION_PACKAGE_FULLY_REMOVED)));

        int uid = TEST_UID;
        // Send the broadcast
        Intent intent = new Intent(Intent.ACTION_PACKAGE_FULLY_REMOVED);
        intent.putExtra(Intent.EXTRA_UID, uid);
        mBroadcastReceiverCaptor.getValue().onReceive(mContext, intent);

        verify(mWifiStateMachine, never()).removeAppConfigs(anyString(), anyInt());

        mLooper.dispatchAll();
        verify(mScanRequestProxy, never()).clearScanRequestTimestampsForApp(anyString(), anyInt());
    }

    @Test
    public void testUserRemovedBroadcastHandling() {
        mWifiServiceImpl.checkAndStartWifi();
        verify(mContext).registerReceiver(mBroadcastReceiverCaptor.capture(),
                (IntentFilter) argThat((IntentFilter filter) ->
                        filter.hasAction(Intent.ACTION_USER_REMOVED)));

        int userHandle = TEST_USER_HANDLE;
        // Send the broadcast
        Intent intent = new Intent(Intent.ACTION_USER_REMOVED);
        intent.putExtra(Intent.EXTRA_USER_HANDLE, userHandle);
        mBroadcastReceiverCaptor.getValue().onReceive(mContext, intent);

        verify(mWifiStateMachine).removeUserConfigs(userHandle);
    }

    @Test
    public void testUserRemovedBroadcastHandlingWithWrongIntentAction() {
        mWifiServiceImpl.checkAndStartWifi();
        verify(mContext).registerReceiver(mBroadcastReceiverCaptor.capture(),
                (IntentFilter) argThat((IntentFilter filter) ->
                        filter.hasAction(Intent.ACTION_USER_REMOVED)));

        int userHandle = TEST_USER_HANDLE;
        // Send the broadcast with wrong action
        Intent intent = new Intent(Intent.ACTION_USER_FOREGROUND);
        intent.putExtra(Intent.EXTRA_USER_HANDLE, userHandle);
        mBroadcastReceiverCaptor.getValue().onReceive(mContext, intent);

        verify(mWifiStateMachine, never()).removeUserConfigs(userHandle);
    }

    /**
     * Test for needs5GHzToAnyApBandConversion returns true.  Requires the NETWORK_SETTINGS
     * permission.
     */
    @Test
    public void testNeeds5GHzToAnyApBandConversionReturnedTrue() {
        when(mResources.getBoolean(
                eq(com.android.internal.R.bool.config_wifi_convert_apband_5ghz_to_any)))
                .thenReturn(true);
        assertTrue(mWifiServiceImpl.needs5GHzToAnyApBandConversion());

        verify(mContext).enforceCallingOrSelfPermission(
                eq(android.Manifest.permission.NETWORK_SETTINGS), eq("WifiService"));
    }

    /**
     * Test for needs5GHzToAnyApBandConversion returns false.  Requires the NETWORK_SETTINGS
     * permission.
     */
    @Test
    public void testNeeds5GHzToAnyApBandConversionReturnedFalse() {
        when(mResources.getBoolean(
                eq(com.android.internal.R.bool.config_wifi_convert_apband_5ghz_to_any)))
                .thenReturn(false);

        assertFalse(mWifiServiceImpl.needs5GHzToAnyApBandConversion());

        verify(mContext).enforceCallingOrSelfPermission(
                eq(android.Manifest.permission.NETWORK_SETTINGS), eq("WifiService"));
    }

    /**
     * The API impl for needs5GHzToAnyApBandConversion requires the NETWORK_SETTINGS permission,
     * verify an exception is thrown without holding the permission.
     */
    @Test
    public void testNeeds5GHzToAnyApBandConversionThrowsWithoutProperPermissions() {
        doThrow(new SecurityException()).when(mContext)
                .enforceCallingOrSelfPermission(eq(android.Manifest.permission.NETWORK_SETTINGS),
                                                eq("WifiService"));

        try {
            mWifiServiceImpl.needs5GHzToAnyApBandConversion();
            // should have thrown an exception - fail test
            fail();
        } catch (SecurityException e) {
            // expected
        }
    }


    private class IdleModeIntentMatcher implements ArgumentMatcher<IntentFilter> {
        @Override
        public boolean matches(IntentFilter filter) {
            return filter.hasAction(PowerManager.ACTION_DEVICE_IDLE_MODE_CHANGED);
        }
    }

    /**
     * Verifies that enforceChangePermission(String package) is called and the caller doesn't
     * have NETWORK_SETTINGS permission
     */
    private void verifyCheckChangePermission(String callingPackageName) {
        verify(mContext, atLeastOnce())
                .checkPermission(eq(android.Manifest.permission.NETWORK_SETTINGS),
                        anyInt(), anyInt());
        verify(mContext, atLeastOnce()).enforceCallingOrSelfPermission(
                android.Manifest.permission.CHANGE_WIFI_STATE, "WifiService");
        verify(mAppOpsManager, atLeastOnce()).noteOp(
                AppOpsManager.OPSTR_CHANGE_WIFI_STATE, Process.myUid(), callingPackageName);
    }

    private WifiConfiguration createValidSoftApConfiguration() {
        WifiConfiguration apConfig = new WifiConfiguration();
        apConfig.SSID = "TestAp";
        apConfig.preSharedKey = "thisIsABadPassword";
        apConfig.allowedKeyManagement.set(KeyMgmt.WPA2_PSK);
        apConfig.apBand = WifiConfiguration.AP_BAND_2GHZ;

        return apConfig;
    }

    /**
     * Verifies that sim state change does not set or reset the country code
     */
    @Test
    public void testSimStateChangeDoesNotResetCountryCode() {
        mWifiServiceImpl.checkAndStartWifi();
        verify(mContext).registerReceiver(mBroadcastReceiverCaptor.capture(),
                (IntentFilter) argThat((IntentFilter filter) ->
                        filter.hasAction(TelephonyIntents.ACTION_SIM_STATE_CHANGED)));

        int userHandle = TEST_USER_HANDLE;
        // Send the broadcast
        Intent intent = new Intent(TelephonyIntents.ACTION_SIM_STATE_CHANGED);
        intent.putExtra(Intent.EXTRA_USER_HANDLE, userHandle);
        mBroadcastReceiverCaptor.getValue().onReceive(mContext, intent);
        verifyNoMoreInteractions(mWifiCountryCode);
    }
}
