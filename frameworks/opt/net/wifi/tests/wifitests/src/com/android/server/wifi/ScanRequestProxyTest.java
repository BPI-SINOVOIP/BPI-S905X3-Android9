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

import static android.app.ActivityManager.RunningAppProcessInfo.IMPORTANCE_FOREGROUND_SERVICE;

import static com.android.server.wifi.ScanRequestProxy.SCAN_REQUEST_THROTTLE_MAX_IN_TIME_WINDOW_FG_APPS;
import static com.android.server.wifi.ScanRequestProxy.SCAN_REQUEST_THROTTLE_TIME_WINDOW_FG_APPS_MS;

import static org.junit.Assert.*;
import static org.mockito.Mockito.*;

import android.app.ActivityManager;
import android.app.AppOpsManager;
import android.content.Context;
import android.content.Intent;
import android.net.wifi.ScanResult;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiScanner;
import android.os.UserHandle;
import android.os.WorkSource;
import android.support.test.filters.SmallTest;

import com.android.server.wifi.util.WifiPermissionsUtil;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.mockito.ArgumentCaptor;
import org.mockito.InOrder;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.util.ArrayList;
import java.util.List;

/**
 * Unit tests for {@link com.android.server.wifi.ScanRequestProxy}.
 */
@SmallTest
public class ScanRequestProxyTest {
    private static final int TEST_UID = 5;
    private static final String TEST_PACKAGE_NAME_1 = "com.test.1";
    private static final String TEST_PACKAGE_NAME_2 = "com.test.2";
    private static final List<WifiScanner.ScanSettings.HiddenNetwork> TEST_HIDDEN_NETWORKS_LIST =
            new ArrayList<WifiScanner.ScanSettings.HiddenNetwork>() {{
                add(new WifiScanner.ScanSettings.HiddenNetwork("test_ssid_1"));
                add(new WifiScanner.ScanSettings.HiddenNetwork("test_ssid_2"));

            }};

    @Mock private Context mContext;
    @Mock private AppOpsManager mAppOps;
    @Mock private ActivityManager mActivityManager;
    @Mock private WifiInjector mWifiInjector;
    @Mock private WifiConfigManager mWifiConfigManager;
    @Mock private WifiScanner mWifiScanner;
    @Mock private WifiPermissionsUtil mWifiPermissionsUtil;
    @Mock private WifiMetrics mWifiMetrics;
    @Mock private Clock mClock;
    private ArgumentCaptor<WorkSource> mWorkSourceArgumentCaptor =
            ArgumentCaptor.forClass(WorkSource.class);
    private ArgumentCaptor<WifiScanner.ScanSettings> mScanSettingsArgumentCaptor =
            ArgumentCaptor.forClass(WifiScanner.ScanSettings.class);
    private ArgumentCaptor<WifiScanner.ScanListener> mScanListenerArgumentCaptor =
            ArgumentCaptor.forClass(WifiScanner.ScanListener.class);
    private WifiScanner.ScanData[] mTestScanDatas1;
    private WifiScanner.ScanData[] mTestScanDatas2;
    private InOrder mInOrder;

    private ScanRequestProxy mScanRequestProxy;

    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);

        when(mWifiInjector.getWifiScanner()).thenReturn(mWifiScanner);
        when(mWifiConfigManager.retrieveHiddenNetworkList()).thenReturn(TEST_HIDDEN_NETWORKS_LIST);
        doNothing().when(mWifiScanner).startScan(
                mScanSettingsArgumentCaptor.capture(),
                mScanListenerArgumentCaptor.capture(),
                mWorkSourceArgumentCaptor.capture());

        mInOrder = inOrder(mWifiScanner, mWifiConfigManager, mContext);
        mTestScanDatas1 = ScanTestUtil.createScanDatas(new int[][]{ { 2417, 2427, 5180, 5170 } });
        mTestScanDatas2 = ScanTestUtil.createScanDatas(new int[][]{ { 2412, 2422, 5200, 5210 } });

        mScanRequestProxy =
            new ScanRequestProxy(mContext, mAppOps, mActivityManager, mWifiInjector,
                    mWifiConfigManager, mWifiPermissionsUtil, mWifiMetrics, mClock);
    }

    @After
    public void cleanUp() throws Exception {
        verifyNoMoreInteractions(mWifiScanner, mWifiConfigManager, mContext, mWifiMetrics);
        validateMockitoUsage();
    }

    /**
     * Verify scan request will be rejected if we cannot get a handle to wifiscanner.
     */
    @Test
    public void testStartScanFailWithoutScanner() {
        when(mWifiInjector.getWifiScanner()).thenReturn(null);
        assertFalse(mScanRequestProxy.startScan(TEST_UID, TEST_PACKAGE_NAME_1));
        validateScanResultsFailureBroadcastSent(TEST_PACKAGE_NAME_1);
    }

    /**
     * Verify scan request will forwarded to wifiscanner if wifiscanner is present.
     */
    @Test
    public void testStartScanSuccess() {
        assertTrue(mScanRequestProxy.startScan(TEST_UID, TEST_PACKAGE_NAME_1));
        mInOrder.verify(mWifiScanner).startScan(any(), any(), any());

        assertTrue(mWorkSourceArgumentCaptor.getValue().equals(new WorkSource(TEST_UID)));
        validateScanSettings(mScanSettingsArgumentCaptor.getValue(), false);

        verify(mWifiMetrics).incrementExternalAppOneshotScanRequestsCount();
    }

    /**
     * Verify scan request will forwarded to wifiscanner if wifiscanner is present.
     */
    @Test
    public void testStartScanSuccessFromAppWithNetworkSettings() {
        when(mWifiPermissionsUtil.checkNetworkSettingsPermission(TEST_UID)).thenReturn(true);
        assertTrue(mScanRequestProxy.startScan(TEST_UID, TEST_PACKAGE_NAME_1));
        mInOrder.verify(mWifiScanner).startScan(any(), any(), any());

        assertTrue(mWorkSourceArgumentCaptor.getValue().equals(new WorkSource(TEST_UID)));
        validateScanSettings(mScanSettingsArgumentCaptor.getValue(), false, true);
    }

    /**
     * Verify scan request will forwarded to wifiscanner if wifiscanner is present.
     */
    @Test
    public void testStartScanSuccessFromAppWithNetworkSetupWizard() {
        when(mWifiPermissionsUtil.checkNetworkSetupWizardPermission(TEST_UID)).thenReturn(true);
        assertTrue(mScanRequestProxy.startScan(TEST_UID, TEST_PACKAGE_NAME_1));
        mInOrder.verify(mWifiScanner).startScan(any(), any(), any());

        assertTrue(mWorkSourceArgumentCaptor.getValue().equals(new WorkSource(TEST_UID)));
        validateScanSettings(mScanSettingsArgumentCaptor.getValue(), false, true);
    }

    /**
     * Verify that hidden network list is not retrieved when hidden network scanning is disabled.
     */
    @Test
    public void testStartScanWithHiddenNetworkScanningDisabled() {
        mScanRequestProxy.enableScanningForHiddenNetworks(false);
        assertTrue(mScanRequestProxy.startScan(TEST_UID, TEST_PACKAGE_NAME_1));
        mInOrder.verify(mWifiConfigManager, never()).retrieveHiddenNetworkList();
        mInOrder.verify(mWifiScanner).startScan(any(), any(), any());

        assertTrue(mWorkSourceArgumentCaptor.getValue().equals(new WorkSource(TEST_UID)));
        validateScanSettings(mScanSettingsArgumentCaptor.getValue(), false);

        verify(mWifiMetrics).incrementExternalAppOneshotScanRequestsCount();
    }

    /**
     * Verify that hidden network list is retrieved when hidden network scanning is enabled.
     */
    @Test
    public void testStartScanWithHiddenNetworkScanningEnabled() {
        mScanRequestProxy.enableScanningForHiddenNetworks(true);
        assertTrue(mScanRequestProxy.startScan(TEST_UID, TEST_PACKAGE_NAME_1));
        mInOrder.verify(mWifiConfigManager).retrieveHiddenNetworkList();
        mInOrder.verify(mWifiScanner).startScan(any(), any(), any());

        assertTrue(mWorkSourceArgumentCaptor.getValue().equals(new WorkSource(TEST_UID)));
        validateScanSettings(mScanSettingsArgumentCaptor.getValue(), true);

        verify(mWifiMetrics).incrementExternalAppOneshotScanRequestsCount();
    }

    /**
     * Verify a successful scan request and processing of scan results.
     */
    @Test
    public void testScanSuccess() {
        // Make a scan request.
        testStartScanSuccess();

        // Verify the scan results processing.
        mScanListenerArgumentCaptor.getValue().onResults(mTestScanDatas1);
        validateScanResultsAvailableBroadcastSent(true);

        // Validate the scan results in the cache.
        ScanTestUtil.assertScanResultsEquals(
                mTestScanDatas1[0].getResults(),
                mScanRequestProxy.getScanResults().stream().toArray(ScanResult[]::new));

        verify(mWifiMetrics).incrementExternalAppOneshotScanRequestsCount();
    }

    /**
     * Verify a successful scan request and processing of scan failure.
     */
    @Test
    public void testScanFailure() {
        // Make a scan request.
        testStartScanSuccess();

        // Verify the scan failure processing.
        mScanListenerArgumentCaptor.getValue().onFailure(0, "failed");
        validateScanResultsAvailableBroadcastSent(false);

        // Ensure scan results in the cache is empty.
        assertTrue(mScanRequestProxy.getScanResults().isEmpty());

        verify(mWifiMetrics).incrementExternalAppOneshotScanRequestsCount();
    }

    /**
     * Verify processing of successive successful scans.
     */
    @Test
    public void testScanSuccessOverwritesPreviousResults() {
        // Make scan request 1.
        assertTrue(mScanRequestProxy.startScan(TEST_UID, TEST_PACKAGE_NAME_1));
        mInOrder.verify(mWifiScanner).startScan(any(), any(), any());
        // Verify the scan results processing for request 1.
        mScanListenerArgumentCaptor.getValue().onResults(mTestScanDatas1);
        validateScanResultsAvailableBroadcastSent(true);
        // Validate the scan results in the cache.
        ScanTestUtil.assertScanResultsEquals(
                mTestScanDatas1[0].getResults(),
                mScanRequestProxy.getScanResults().stream().toArray(ScanResult[]::new));

        // Make scan request 2.
        assertTrue(mScanRequestProxy.startScan(TEST_UID, TEST_PACKAGE_NAME_2));
        mInOrder.verify(mWifiScanner).startScan(any(), any(), any());
        // Verify the scan results processing for request 2.
        mScanListenerArgumentCaptor.getValue().onResults(mTestScanDatas2);
        validateScanResultsAvailableBroadcastSent(true);
        // Validate the scan results in the cache.
        ScanTestUtil.assertScanResultsEquals(
                mTestScanDatas2[0].getResults(),
                mScanRequestProxy.getScanResults().stream().toArray(ScanResult[]::new));

        verify(mWifiMetrics, times(2)).incrementExternalAppOneshotScanRequestsCount();
    }

    /**
     * Verify processing of a successful scan followed by a failure.
     */
    @Test
    public void testScanFailureDoesNotOverwritePreviousResults() {
        // Make scan request 1.
        assertTrue(mScanRequestProxy.startScan(TEST_UID, TEST_PACKAGE_NAME_1));
        mInOrder.verify(mWifiScanner).startScan(any(), any(), any());
        // Verify the scan results processing for request 1.
        mScanListenerArgumentCaptor.getValue().onResults(mTestScanDatas1);
        validateScanResultsAvailableBroadcastSent(true);
        // Validate the scan results in the cache.
        ScanTestUtil.assertScanResultsEquals(
                mTestScanDatas1[0].getResults(),
                mScanRequestProxy.getScanResults().stream().toArray(ScanResult[]::new));

        // Make scan request 2.
        assertTrue(mScanRequestProxy.startScan(TEST_UID, TEST_PACKAGE_NAME_2));
        mInOrder.verify(mWifiScanner).startScan(any(), any(), any());
        // Verify the scan failure processing.
        mScanListenerArgumentCaptor.getValue().onFailure(0, "failed");
        validateScanResultsAvailableBroadcastSent(false);
        // Validate the scan results from a previous successful scan in the cache.
        ScanTestUtil.assertScanResultsEquals(
                mTestScanDatas1[0].getResults(),
                mScanRequestProxy.getScanResults().stream().toArray(ScanResult[]::new));

        verify(mWifiMetrics, times(2)).incrementExternalAppOneshotScanRequestsCount();
    }

    /**
     * Verify processing of a new scan request while there was a previous scan request being
     * processed.
     * Verify that we don't send a second broadcast.
     */
    @Test
    public void testScanRequestWhilePeviousScanRunning() {
        WifiScanner.ScanListener listener1;
        WifiScanner.ScanListener listener2;
        // Make scan request 1.
        assertTrue(mScanRequestProxy.startScan(TEST_UID, TEST_PACKAGE_NAME_1));
        mInOrder.verify(mWifiScanner).startScan(any(), any(), any());
        listener1 = mScanListenerArgumentCaptor.getValue();

        // Make scan request 2.
        assertTrue(mScanRequestProxy.startScan(TEST_UID, TEST_PACKAGE_NAME_2));
        // Ensure that we did send a second scan request to scanner.
        mInOrder.verify(mWifiScanner).startScan(any(), any(), any());
        listener2 = mScanListenerArgumentCaptor.getValue();

        // Now send the scan results for request 1.
        listener1.onResults(mTestScanDatas1);
        validateScanResultsAvailableBroadcastSent(true);
        // Validate the scan results in the cache.
        ScanTestUtil.assertScanResultsEquals(
                mTestScanDatas1[0].getResults(),
                mScanRequestProxy.getScanResults().stream().toArray(ScanResult[]::new));

        // Now send the scan results for request 2.
        listener2.onResults(mTestScanDatas2);
        // Ensure that we did not send out another broadcast.

        // Validate the scan results in the cache.
        ScanTestUtil.assertScanResultsEquals(
                mTestScanDatas2[0].getResults(),
                mScanRequestProxy.getScanResults().stream().toArray(ScanResult[]::new));

        verify(mWifiMetrics, times(2)).incrementExternalAppOneshotScanRequestsCount();
    }


    /**
     * Verify processing of a new scan request after a previous scan success.
     * Verify that we send out two broadcasts (two successes).
     */
    @Test
    public void testNewScanRequestAfterPreviousScanSucceeds() {
        // Make scan request 1.
        assertTrue(mScanRequestProxy.startScan(TEST_UID, TEST_PACKAGE_NAME_1));
        mInOrder.verify(mWifiScanner).startScan(any(), any(), any());
        // Now send the scan results for request 1.
        mScanListenerArgumentCaptor.getValue().onResults(mTestScanDatas1);
        validateScanResultsAvailableBroadcastSent(true);
        // Validate the scan results in the cache.
        ScanTestUtil.assertScanResultsEquals(
                mTestScanDatas1[0].getResults(),
                mScanRequestProxy.getScanResults().stream().toArray(ScanResult[]::new));

        // Make scan request 2.
        assertTrue(mScanRequestProxy.startScan(TEST_UID, TEST_PACKAGE_NAME_2));
        // Ensure that we did send a second scan request to scanner.
        mInOrder.verify(mWifiScanner).startScan(any(), any(), any());
        // Now send the scan results for request 2.
        mScanListenerArgumentCaptor.getValue().onResults(mTestScanDatas2);
        validateScanResultsAvailableBroadcastSent(true);
        // Validate the scan results in the cache.
        ScanTestUtil.assertScanResultsEquals(
                mTestScanDatas2[0].getResults(),
                mScanRequestProxy.getScanResults().stream().toArray(ScanResult[]::new));

        verify(mWifiMetrics, times(2)).incrementExternalAppOneshotScanRequestsCount();
    }

    /**
     * Verify processing of a new scan request after a previous scan success, but with bad scan
     * data.
     * Verify that we send out two broadcasts (one failure & one success).
     */
    @Test
    public void testNewScanRequestAfterPreviousScanSucceedsWithInvalidScanDatas() {
        // Make scan request 1.
        assertTrue(mScanRequestProxy.startScan(TEST_UID, TEST_PACKAGE_NAME_1));
        mInOrder.verify(mWifiScanner).startScan(any(), any(), any());

        // Now send scan success for request 1, but with invalid scan datas.
        mScanListenerArgumentCaptor.getValue().onResults(
                new WifiScanner.ScanData[] {mTestScanDatas1[0], mTestScanDatas2[0]});
        validateScanResultsAvailableBroadcastSent(false);
        // Validate the scan results in the cache.
        assertTrue(mScanRequestProxy.getScanResults().isEmpty());

        // Make scan request 2.
        assertTrue(mScanRequestProxy.startScan(TEST_UID, TEST_PACKAGE_NAME_2));
        // Ensure that we did send a second scan request to scanner.
        mInOrder.verify(mWifiScanner).startScan(any(), any(), any());
        // Now send the scan results for request 2.
        mScanListenerArgumentCaptor.getValue().onResults(mTestScanDatas2);
        validateScanResultsAvailableBroadcastSent(true);
        // Validate the scan results in the cache.
        ScanTestUtil.assertScanResultsEquals(
                mTestScanDatas2[0].getResults(),
                mScanRequestProxy.getScanResults().stream().toArray(ScanResult[]::new));

        verify(mWifiMetrics, times(2)).incrementExternalAppOneshotScanRequestsCount();
    }


    /**
     * Verify processing of a new scan request after a previous scan failure.
     * Verify that we send out two broadcasts (one failure & one success).
     */
    @Test
    public void testNewScanRequestAfterPreviousScanFailure() {
        // Make scan request 1.
        assertTrue(mScanRequestProxy.startScan(TEST_UID, TEST_PACKAGE_NAME_1));
        mInOrder.verify(mWifiScanner).startScan(any(), any(), any());

        // Now send scan failure for request 1.
        mScanListenerArgumentCaptor.getValue().onFailure(0, "failed");
        validateScanResultsAvailableBroadcastSent(false);
        // Validate the scan results in the cache.
        assertTrue(mScanRequestProxy.getScanResults().isEmpty());

        // Make scan request 2.
        assertTrue(mScanRequestProxy.startScan(TEST_UID, TEST_PACKAGE_NAME_2));
        // Ensure that we did send a second scan request to scanner.
        mInOrder.verify(mWifiScanner).startScan(any(), any(), any());
        // Now send the scan results for request 2.
        mScanListenerArgumentCaptor.getValue().onResults(mTestScanDatas2);
        validateScanResultsAvailableBroadcastSent(true);
        // Validate the scan results in the cache.
        ScanTestUtil.assertScanResultsEquals(
                mTestScanDatas2[0].getResults(),
                mScanRequestProxy.getScanResults().stream().toArray(ScanResult[]::new));

        verify(mWifiMetrics, times(2)).incrementExternalAppOneshotScanRequestsCount();
    }

    /**
     * Verify that clear scan results invocation clears all stored scan results.
     */
    @Test
    public void testClearScanResults() {
        // Make scan request 1.
        assertTrue(mScanRequestProxy.startScan(TEST_UID, TEST_PACKAGE_NAME_1));
        mInOrder.verify(mWifiScanner).startScan(any(), any(), any());
        // Verify the scan results processing for request 1.
        mScanListenerArgumentCaptor.getValue().onResults(mTestScanDatas1);
        validateScanResultsAvailableBroadcastSent(true);
        // Validate the scan results in the cache.
        ScanTestUtil.assertScanResultsEquals(
                mTestScanDatas1[0].getResults(),
                mScanRequestProxy.getScanResults().stream().toArray(ScanResult[]::new));

        mScanRequestProxy.clearScanResults();
        assertTrue(mScanRequestProxy.getScanResults().isEmpty());

        verify(mWifiMetrics).incrementExternalAppOneshotScanRequestsCount();
    }

    /**
     * Verify that we don't use the same listener for multiple scan requests.
     */
    @Test
    public void testSuccessiveScanRequestsDontUseSameListener() {
        WifiScanner.ScanListener listener1;
        WifiScanner.ScanListener listener2;
        // Make scan request 1.
        assertTrue(mScanRequestProxy.startScan(TEST_UID, TEST_PACKAGE_NAME_1));
        mInOrder.verify(mWifiScanner).startScan(any(), any(), any());
        listener1 = mScanListenerArgumentCaptor.getValue();

        // Make scan request 2.
        assertTrue(mScanRequestProxy.startScan(TEST_UID, TEST_PACKAGE_NAME_2));
        // Ensure that we did send a second scan request to scanner.
        mInOrder.verify(mWifiScanner).startScan(any(), any(), any());
        listener2 = mScanListenerArgumentCaptor.getValue();

        assertNotEquals(listener1, listener2);

        verify(mWifiMetrics, times(2)).incrementExternalAppOneshotScanRequestsCount();
    }

    /**
     * Ensure new scan requests from the same app are rejected if there are more than
     * {@link ScanRequestProxy#SCAN_REQUEST_THROTTLE_MAX_IN_TIME_WINDOW_FG_APPS} requests in
     * {@link ScanRequestProxy#SCAN_REQUEST_THROTTLE_TIME_WINDOW_FG_APPS_MS}
     */
    @Test
    public void testSuccessiveScanRequestFromSameFgAppThrottled() {
        long firstRequestMs = 782;
        when(mClock.getElapsedSinceBootMillis()).thenReturn(firstRequestMs);
        for (int i = 0; i < SCAN_REQUEST_THROTTLE_MAX_IN_TIME_WINDOW_FG_APPS; i++) {
            when(mClock.getElapsedSinceBootMillis()).thenReturn(firstRequestMs + i);
            assertTrue(mScanRequestProxy.startScan(TEST_UID, TEST_PACKAGE_NAME_1));
            mInOrder.verify(mWifiScanner).startScan(any(), any(), any());
        }
        // Make next scan request from the same package name & ensure that it is throttled.
        assertFalse(mScanRequestProxy.startScan(TEST_UID, TEST_PACKAGE_NAME_1));
        validateScanResultsFailureBroadcastSent(TEST_PACKAGE_NAME_1);

        verify(mWifiMetrics, times(SCAN_REQUEST_THROTTLE_MAX_IN_TIME_WINDOW_FG_APPS + 1))
                .incrementExternalAppOneshotScanRequestsCount();
        verify(mWifiMetrics).incrementExternalForegroundAppOneshotScanRequestsThrottledCount();
    }

    /**
     * Ensure new scan requests from the same app are rejected if there are more than
     * {@link ScanRequestProxy#SCAN_REQUEST_THROTTLE_MAX_IN_TIME_WINDOW_FG_APPS} requests after
     * {@link ScanRequestProxy#SCAN_REQUEST_THROTTLE_TIME_WINDOW_FG_APPS_MS}
     */
    @Test
    public void testSuccessiveScanRequestFromSameFgAppNotThrottled() {
        long firstRequestMs = 782;
        when(mClock.getElapsedSinceBootMillis()).thenReturn(firstRequestMs);
        for (int i = 0; i < SCAN_REQUEST_THROTTLE_MAX_IN_TIME_WINDOW_FG_APPS; i++) {
            when(mClock.getElapsedSinceBootMillis()).thenReturn(firstRequestMs + i);
            assertTrue(mScanRequestProxy.startScan(TEST_UID, TEST_PACKAGE_NAME_1));
            mInOrder.verify(mWifiScanner).startScan(any(), any(), any());
        }
        long lastRequestMs = firstRequestMs + SCAN_REQUEST_THROTTLE_TIME_WINDOW_FG_APPS_MS + 1;
        when(mClock.getElapsedSinceBootMillis()).thenReturn(lastRequestMs);
        // Make next scan request from the same package name & ensure that it is not throttled.
        assertTrue(mScanRequestProxy.startScan(TEST_UID, TEST_PACKAGE_NAME_1));
        mInOrder.verify(mWifiScanner).startScan(any(), any(), any());

        verify(mWifiMetrics, times(SCAN_REQUEST_THROTTLE_MAX_IN_TIME_WINDOW_FG_APPS + 1))
                .incrementExternalAppOneshotScanRequestsCount();
    }

    /**
     * Ensure new scan requests from the same app with NETWORK_SETTINGS permission are not
     * throttled.
     */
    @Test
    public void testSuccessiveScanRequestFromSameAppWithNetworkSettingsPermissionNotThrottled() {
        when(mWifiPermissionsUtil.checkNetworkSettingsPermission(TEST_UID)).thenReturn(true);

        long firstRequestMs = 782;
        when(mClock.getElapsedSinceBootMillis()).thenReturn(firstRequestMs);
        for (int i = 0; i < SCAN_REQUEST_THROTTLE_MAX_IN_TIME_WINDOW_FG_APPS; i++) {
            when(mClock.getElapsedSinceBootMillis()).thenReturn(firstRequestMs + i);
            assertTrue(mScanRequestProxy.startScan(TEST_UID, TEST_PACKAGE_NAME_1));
            mInOrder.verify(mWifiScanner).startScan(any(), any(), any());
        }
        // Make next scan request from the same package name & ensure that it is not throttled.
        assertTrue(mScanRequestProxy.startScan(TEST_UID, TEST_PACKAGE_NAME_1));
        mInOrder.verify(mWifiScanner).startScan(any(), any(), any());
    }

    /**
     * Ensure new scan requests from the same app with NETWORK_SETUP_WIZARD permission are not
     * throttled.
     */
    @Test
    public void testSuccessiveScanRequestFromSameAppWithNetworkSetupWizardPermissionNotThrottled() {
        when(mWifiPermissionsUtil.checkNetworkSetupWizardPermission(TEST_UID)).thenReturn(true);

        long firstRequestMs = 782;
        when(mClock.getElapsedSinceBootMillis()).thenReturn(firstRequestMs);
        for (int i = 0; i < SCAN_REQUEST_THROTTLE_MAX_IN_TIME_WINDOW_FG_APPS; i++) {
            when(mClock.getElapsedSinceBootMillis()).thenReturn(firstRequestMs + i);
            assertTrue(mScanRequestProxy.startScan(TEST_UID, TEST_PACKAGE_NAME_1));
            mInOrder.verify(mWifiScanner).startScan(any(), any(), any());
        }
        // Make next scan request from the same package name & ensure that it is not throttled.
        assertTrue(mScanRequestProxy.startScan(TEST_UID, TEST_PACKAGE_NAME_1));
        mInOrder.verify(mWifiScanner).startScan(any(), any(), any());
    }

    /**
     * Ensure new scan requests from different apps are not throttled.
     */
    @Test
    public void testSuccessiveScanRequestFromDifferentFgAppsNotThrottled() {
        long firstRequestMs = 782;
        when(mClock.getElapsedSinceBootMillis()).thenReturn(firstRequestMs);
        for (int i = 0; i < SCAN_REQUEST_THROTTLE_MAX_IN_TIME_WINDOW_FG_APPS / 2; i++) {
            when(mClock.getElapsedSinceBootMillis()).thenReturn(firstRequestMs + i);
            assertTrue(mScanRequestProxy.startScan(TEST_UID, TEST_PACKAGE_NAME_1));
            mInOrder.verify(mWifiScanner).startScan(any(), any(), any());
        }
        for (int i = 0; i < SCAN_REQUEST_THROTTLE_MAX_IN_TIME_WINDOW_FG_APPS / 2; i++) {
            when(mClock.getElapsedSinceBootMillis()).thenReturn(firstRequestMs + i);
            assertTrue(mScanRequestProxy.startScan(TEST_UID, TEST_PACKAGE_NAME_2));
            mInOrder.verify(mWifiScanner).startScan(any(), any(), any());
        }
        // Make next scan request from both the package name & ensure that it is not throttled.
        assertTrue(mScanRequestProxy.startScan(TEST_UID, TEST_PACKAGE_NAME_1));
        mInOrder.verify(mWifiScanner).startScan(any(), any(), any());
        assertTrue(mScanRequestProxy.startScan(TEST_UID, TEST_PACKAGE_NAME_2));
        mInOrder.verify(mWifiScanner).startScan(any(), any(), any());

        verify(mWifiMetrics, times(SCAN_REQUEST_THROTTLE_MAX_IN_TIME_WINDOW_FG_APPS + 2))
                .incrementExternalAppOneshotScanRequestsCount();
    }

    /**
     * Ensure new scan requests from the same app after removal & re-install is not
     * throttled.
     * Verifies that we clear the scan timestamps for apps that were removed.
     */
    @Test
    public void testSuccessiveScanRequestFromSameAppAfterRemovalAndReinstallNotThrottled() {
        long firstRequestMs = 782;
        when(mClock.getElapsedSinceBootMillis()).thenReturn(firstRequestMs);
        for (int i = 0; i < SCAN_REQUEST_THROTTLE_MAX_IN_TIME_WINDOW_FG_APPS; i++) {
            when(mClock.getElapsedSinceBootMillis()).thenReturn(firstRequestMs + i);
            assertTrue(mScanRequestProxy.startScan(TEST_UID, TEST_PACKAGE_NAME_1));
            mInOrder.verify(mWifiScanner).startScan(any(), any(), any());
        }
        // Now simulate removing the app.
        mScanRequestProxy.clearScanRequestTimestampsForApp(TEST_PACKAGE_NAME_1, TEST_UID);

        // Make next scan request from the same package name (simulating a reinstall) & ensure that
        // it is not throttled.
        assertTrue(mScanRequestProxy.startScan(TEST_UID, TEST_PACKAGE_NAME_1));
        mInOrder.verify(mWifiScanner).startScan(any(), any(), any());

        verify(mWifiMetrics, times(SCAN_REQUEST_THROTTLE_MAX_IN_TIME_WINDOW_FG_APPS + 1))
                .incrementExternalAppOneshotScanRequestsCount();
    }

    /**
     * Ensure new scan requests after removal of the app from a different user is throttled.
     * The app has same the package name across users, but different UID's. Verifies that
     * the cache is cleared only for the specific app for a specific user when an app is removed.
     */
    @Test
    public void testSuccessiveScanRequestFromSameAppAfterRemovalOnAnotherUserThrottled() {
        long firstRequestMs = 782;
        when(mClock.getElapsedSinceBootMillis()).thenReturn(firstRequestMs);
        for (int i = 0; i < SCAN_REQUEST_THROTTLE_MAX_IN_TIME_WINDOW_FG_APPS; i++) {
            when(mClock.getElapsedSinceBootMillis()).thenReturn(firstRequestMs + i);
            assertTrue(mScanRequestProxy.startScan(TEST_UID, TEST_PACKAGE_NAME_1));
            mInOrder.verify(mWifiScanner).startScan(any(), any(), any());
        }
        // Now simulate removing the app for another user (User 1).
        mScanRequestProxy.clearScanRequestTimestampsForApp(
                TEST_PACKAGE_NAME_1,
                UserHandle.getUid(UserHandle.USER_SYSTEM + 1, TEST_UID));

        // Make next scan request from the same package name & ensure that is throttled.
        assertFalse(mScanRequestProxy.startScan(TEST_UID, TEST_PACKAGE_NAME_1));
        validateScanResultsFailureBroadcastSent(TEST_PACKAGE_NAME_1);

        verify(mWifiMetrics, times(SCAN_REQUEST_THROTTLE_MAX_IN_TIME_WINDOW_FG_APPS + 1))
                .incrementExternalAppOneshotScanRequestsCount();
        verify(mWifiMetrics).incrementExternalForegroundAppOneshotScanRequestsThrottledCount();
    }

    /**
     * Ensure scan requests from different background apps are throttled if it's before
     * {@link ScanRequestProxy#SCAN_REQUEST_THROTTLE_INTERVAL_BG_APPS_MS}.
     */
    @Test
    public void testSuccessiveScanRequestFromBgAppsThrottled() {
        when(mActivityManager.getPackageImportance(TEST_PACKAGE_NAME_1))
                .thenReturn(IMPORTANCE_FOREGROUND_SERVICE + 1);
        when(mActivityManager.getPackageImportance(TEST_PACKAGE_NAME_2))
                .thenReturn(IMPORTANCE_FOREGROUND_SERVICE + 1);

        long firstRequestMs = 782;
        when(mClock.getElapsedSinceBootMillis()).thenReturn(firstRequestMs);
        // Make scan request 1.
        assertTrue(mScanRequestProxy.startScan(TEST_UID, TEST_PACKAGE_NAME_1));
        mInOrder.verify(mWifiScanner).startScan(any(), any(), any());

        // Make scan request 2 from the different package name & ensure that it is throttled.
        assertFalse(mScanRequestProxy.startScan(TEST_UID, TEST_PACKAGE_NAME_2));
        validateScanResultsFailureBroadcastSent(TEST_PACKAGE_NAME_2);

        verify(mWifiMetrics, times(2)).incrementExternalAppOneshotScanRequestsCount();
        verify(mWifiMetrics).incrementExternalBackgroundAppOneshotScanRequestsThrottledCount();
    }

    /**
     * Ensure scan requests from different background apps are not throttled if it's after
     * {@link ScanRequestProxy#SCAN_REQUEST_THROTTLE_INTERVAL_BG_APPS_MS}.
     */
    @Test
    public void testSuccessiveScanRequestFromBgAppsNotThrottled() {
        when(mActivityManager.getPackageImportance(TEST_PACKAGE_NAME_1))
                .thenReturn(ActivityManager.RunningAppProcessInfo.IMPORTANCE_FOREGROUND + 1);
        when(mActivityManager.getPackageImportance(TEST_PACKAGE_NAME_2))
                .thenReturn(ActivityManager.RunningAppProcessInfo.IMPORTANCE_FOREGROUND + 1);

        long firstRequestMs = 782;
        when(mClock.getElapsedSinceBootMillis()).thenReturn(firstRequestMs);
        // Make scan request 1.
        assertTrue(mScanRequestProxy.startScan(TEST_UID, TEST_PACKAGE_NAME_1));
        mInOrder.verify(mWifiScanner).startScan(any(), any(), any());

        long secondRequestMs =
                firstRequestMs + ScanRequestProxy.SCAN_REQUEST_THROTTLE_INTERVAL_BG_APPS_MS + 1;
        when(mClock.getElapsedSinceBootMillis()).thenReturn(secondRequestMs);
        // Make scan request 2 from the different package name & ensure that it is throttled.
        assertTrue(mScanRequestProxy.startScan(TEST_UID, TEST_PACKAGE_NAME_2));
        mInOrder.verify(mWifiScanner).startScan(any(), any(), any());

        verify(mWifiMetrics, times(2)).incrementExternalAppOneshotScanRequestsCount();
    }

    private void validateScanSettings(WifiScanner.ScanSettings scanSettings,
                                      boolean expectHiddenNetworks) {
        validateScanSettings(scanSettings, expectHiddenNetworks, false);
    }

    private void validateScanSettings(WifiScanner.ScanSettings scanSettings,
                                      boolean expectHiddenNetworks,
                                      boolean expectHighAccuracyType) {
        assertNotNull(scanSettings);
        assertEquals(WifiScanner.WIFI_BAND_BOTH_WITH_DFS, scanSettings.band);
        if (expectHighAccuracyType) {
            assertEquals(WifiScanner.TYPE_HIGH_ACCURACY, scanSettings.type);
        } else {
            assertEquals(WifiScanner.TYPE_LOW_LATENCY, scanSettings.type);
        }
        assertEquals(WifiScanner.REPORT_EVENT_AFTER_EACH_SCAN
                | WifiScanner.REPORT_EVENT_FULL_SCAN_RESULT, scanSettings.reportEvents);
        if (expectHiddenNetworks) {
            assertNotNull(scanSettings.hiddenNetworks);
            assertEquals(TEST_HIDDEN_NETWORKS_LIST.size(), scanSettings.hiddenNetworks.length);
            for (int i = 0; i < scanSettings.hiddenNetworks.length; i++) {
                validateHiddenNetworkInList(scanSettings.hiddenNetworks[i],
                        TEST_HIDDEN_NETWORKS_LIST);
            }
        } else {
            assertNull(scanSettings.hiddenNetworks);
        }
    }

    private void validateHiddenNetworkInList(
            WifiScanner.ScanSettings.HiddenNetwork expectedHiddenNetwork,
            List<WifiScanner.ScanSettings.HiddenNetwork> hiddenNetworkList) {
        for (WifiScanner.ScanSettings.HiddenNetwork hiddenNetwork : hiddenNetworkList) {
            if (hiddenNetwork.ssid.equals(expectedHiddenNetwork.ssid)) {
                return;
            }
        }
        fail();
    }

    private void validateScanResultsAvailableBroadcastSent(boolean expectScanSuceeded) {
        ArgumentCaptor<Intent> intentCaptor = ArgumentCaptor.forClass(Intent.class);
        ArgumentCaptor<UserHandle> userHandleCaptor = ArgumentCaptor.forClass(UserHandle.class);
        mInOrder.verify(mContext).sendBroadcastAsUser(
                intentCaptor.capture(), userHandleCaptor.capture());

        assertEquals(userHandleCaptor.getValue(), UserHandle.ALL);

        Intent intent = intentCaptor.getValue();
        assertEquals(WifiManager.SCAN_RESULTS_AVAILABLE_ACTION, intent.getAction());
        assertEquals(Intent.FLAG_RECEIVER_REGISTERED_ONLY_BEFORE_BOOT, intent.getFlags());
        boolean scanSucceeded = intent.getBooleanExtra(WifiManager.EXTRA_RESULTS_UPDATED, false);
        assertEquals(expectScanSuceeded, scanSucceeded);
    }

    private void validateScanResultsFailureBroadcastSent(String expectedPackageName) {
        ArgumentCaptor<Intent> intentCaptor = ArgumentCaptor.forClass(Intent.class);
        ArgumentCaptor<UserHandle> userHandleCaptor = ArgumentCaptor.forClass(UserHandle.class);
        mInOrder.verify(mContext).sendBroadcastAsUser(
                intentCaptor.capture(), userHandleCaptor.capture());

        assertEquals(userHandleCaptor.getValue(), UserHandle.ALL);

        Intent intent = intentCaptor.getValue();
        assertEquals(WifiManager.SCAN_RESULTS_AVAILABLE_ACTION, intent.getAction());
        assertEquals(Intent.FLAG_RECEIVER_REGISTERED_ONLY_BEFORE_BOOT, intent.getFlags());
        boolean scanSucceeded = intent.getBooleanExtra(WifiManager.EXTRA_RESULTS_UPDATED, false);
        assertFalse(scanSucceeded);
        String packageName = intent.getPackage();
        assertEquals(expectedPackageName, packageName);
    }
}
