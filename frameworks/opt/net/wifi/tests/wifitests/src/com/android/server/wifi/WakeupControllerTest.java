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

package com.android.server.wifi;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyBoolean;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.database.ContentObserver;
import android.net.wifi.ScanResult;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiScanner;
import android.os.test.TestLooper;
import android.provider.Settings;

import com.android.server.wifi.util.ScanResultUtil;

import org.junit.Before;
import org.junit.Test;
import org.mockito.ArgumentCaptor;
import org.mockito.InOrder;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.MockitoAnnotations;
import org.xmlpull.v1.XmlPullParserException;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.Arrays;
import java.util.Collections;
import java.util.Set;

/**
 * Unit tests for {@link WakeupController}.
 */
public class WakeupControllerTest {

    private static final String SAVED_SSID = "test scan ssid";
    private static final int DFS_CHANNEL_FREQ = 5540;

    @Mock private Context mContext;
    @Mock private WakeupLock mWakeupLock;
    @Mock private WakeupEvaluator mWakeupEvaluator;
    @Mock private WakeupOnboarding mWakeupOnboarding;
    @Mock private WifiConfigStore mWifiConfigStore;
    @Mock private WifiInjector mWifiInjector;
    @Mock private WifiScanner mWifiScanner;
    @Mock private WifiConfigManager mWifiConfigManager;
    @Mock private FrameworkFacade mFrameworkFacade;
    @Mock private WifiSettingsStore mWifiSettingsStore;
    @Mock private WifiWakeMetrics mWifiWakeMetrics;
    @Mock private WifiController mWifiController;
    @Mock private WifiNative mWifiNative;

    private TestLooper mLooper;
    private WakeupController mWakeupController;
    private WakeupConfigStoreData mWakeupConfigStoreData;
    private WifiScanner.ScanData[] mTestScanDatas;
    private ScanResult mTestScanResult;

    /** Initialize objects before each test run. */
    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);

        when(mWifiInjector.getWifiScanner()).thenReturn(mWifiScanner);
        when(mWifiInjector.getWifiSettingsStore()).thenReturn(mWifiSettingsStore);
        when(mWifiInjector.getWifiController()).thenReturn(mWifiController);
        when(mWifiInjector.getWifiNative()).thenReturn(mWifiNative);
        when(mWifiNative.getChannelsForBand(WifiScanner.WIFI_BAND_5_GHZ_DFS_ONLY))
                .thenReturn(new int[]{DFS_CHANNEL_FREQ});

        when(mWifiSettingsStore.handleWifiToggled(anyBoolean())).thenReturn(true);

        mLooper = new TestLooper();

        // scanlistener input
        mTestScanResult = new ScanResult();
        mTestScanResult.SSID = SAVED_SSID;
        mTestScanResult.capabilities = "";
        mTestScanResult.frequency = 2412;
        ScanResult[] scanResults = new ScanResult[1];
        scanResults[0] = mTestScanResult;
        mTestScanDatas = new WifiScanner.ScanData[1];
        mTestScanDatas[0] = new WifiScanner.ScanData(0 /* id */, 0 /* flags */,
                0 /* bucketsScanned */, true /* allChannelsScanned */, scanResults);
    }

    /** Initializes the wakeupcontroller in the given {@code enabled} state. */
    private void initializeWakeupController(boolean enabled) {
        initializeWakeupController(enabled, true /* isRead */);
    }

    private void initializeWakeupController(boolean enabled, boolean isRead) {
        int settingsValue = enabled ? 1 : 0;
        when(mFrameworkFacade.getIntegerSetting(mContext,
                Settings.Global.WIFI_WAKEUP_ENABLED, 0)).thenReturn(settingsValue);
        when(mWakeupOnboarding.isOnboarded()).thenReturn(true);
        mWakeupController = new WakeupController(mContext,
                mLooper.getLooper(),
                mWakeupLock,
                mWakeupEvaluator,
                mWakeupOnboarding,
                mWifiConfigManager,
                mWifiConfigStore,
                mWifiWakeMetrics,
                mWifiInjector,
                mFrameworkFacade);

        ArgumentCaptor<WakeupConfigStoreData> captor =
                ArgumentCaptor.forClass(WakeupConfigStoreData.class);
        verify(mWifiConfigStore).registerStoreData(captor.capture());
        mWakeupConfigStoreData = captor.getValue();
        if (isRead) {
            readUserStore();
        }
    }

    private void readUserStore() {
        try {
            mWakeupConfigStoreData.deserializeData(null, 0, false);
        } catch (XmlPullParserException | IOException e) {
            // unreachable
        }
    }

    private ScanResult createOpenScanResult(String ssid, int frequency) {
        ScanResult scanResult = new ScanResult();
        scanResult.SSID = ssid;
        scanResult.capabilities = "";
        scanResult.frequency = frequency;
        return scanResult;
    }

    private void verifyDoesNotEnableWifi() {
        verify(mWifiSettingsStore, never()).handleWifiToggled(true /* wifiEnabled */);
    }

    /**
     * Verify WakeupController is enabled when the settings toggle is true.
     */
    @Test
    public void verifyEnabledWhenToggledOn() {
        initializeWakeupController(true /* enabled */);

        assertTrue(mWakeupController.isEnabled());
    }

    /**
     * Verify WakeupController is disabled when the settings toggle is false.
     */
    @Test
    public void verifyDisabledWhenToggledOff() {
        initializeWakeupController(false /* enabled */);

        assertFalse(mWakeupController.isEnabled());
    }

    /**
     * Verify WakeupController registers its store data with the WifiConfigStore on construction.
     */
    @Test
    public void registersWakeupConfigStoreData() {
        initializeWakeupController(true /* enabled */);
        verify(mWifiConfigStore).registerStoreData(any(WakeupConfigStoreData.class));
    }

    /**
     * Verify that dump calls also dump the state of the WakeupLock.
     */
    @Test
    public void dumpIncludesWakeupLock() {
        initializeWakeupController(true /* enabled */);
        ByteArrayOutputStream stream = new ByteArrayOutputStream();
        PrintWriter writer = new PrintWriter(stream);
        mWakeupController.dump(null, writer, null);

        verify(mWakeupLock).dump(null, writer, null);
    }

    /**
     * Verify that start sets the wakeup lock.
     */
    @Test
    public void startSetsWakeupLock() {
        initializeWakeupController(true /* enabled */);
        mWakeupController.start();
        verify(mWakeupLock).setLock(any());
        verify(mWifiWakeMetrics).recordStartEvent(anyInt());
    }

    /**
     * Verify that start does not record an ignored start call if the controller is not yet active.
     */
    @Test
    public void startDoesNotRecordIgnoredStart() {
        initializeWakeupController(true /* enabled */);
        mWakeupController.start();
        verify(mWifiWakeMetrics, never()).recordIgnoredStart();
    }

    /**
     * Verify that start does not set the wakeup lock when feature is disabled.
     */
    @Test
    public void startDoesNotSetWakeupLockWhenDisabled() {
        initializeWakeupController(false /* enabled */);
        mWakeupController.start();
        verify(mWakeupLock, never()).setLock(any());
        verify(mWifiWakeMetrics, never()).recordStartEvent(anyInt());
    }

    /**
     * If the controller is already active, verify that start() is ignored and no setup is done.
     */
    @Test
    public void startIsIgnoredIfAlreadyActive() {
        initializeWakeupController(true /* enabled */);
        InOrder lockInOrder = Mockito.inOrder(mWakeupLock);
        InOrder metricsInOrder = Mockito.inOrder(mWifiWakeMetrics);

        mWakeupController.start();
        lockInOrder.verify(mWakeupLock).setLock(any());
        metricsInOrder.verify(mWifiWakeMetrics).recordStartEvent(anyInt());

        mWakeupController.stop();
        mWakeupController.start();
        metricsInOrder.verify(mWifiWakeMetrics).recordIgnoredStart();
        metricsInOrder.verify(mWifiWakeMetrics, never()).recordStartEvent(anyInt());
        lockInOrder.verify(mWakeupLock, never()).setLock(any());
    }

    /**
     * Verify that start registers the scan listener on the wifi scanner.
     */
    @Test
    public void startRegistersScanListener() {
        initializeWakeupController(true /* enabled */);
        mWakeupController.start();
        verify(mWifiScanner).registerScanListener(any());
    }

    /**
     * Verify that stop deregisters the scan listener from the wifi scanner.
     */
    @Test
    public void stopDeresgistersScanListener() {
        initializeWakeupController(true /* enabled */);
        mWakeupController.start();
        mWakeupController.stop();
        verify(mWifiScanner).deregisterScanListener(any());
    }

    /**
     * Verify that reset sets active to false.
     *
     * <p>This is accomplished by initiating another call to start and verifying that the wakeup
     * lock is re-set.
     */
    @Test
    public void resetSetsActiveToFalse() {
        initializeWakeupController(true /* enabled */);
        InOrder lockInOrder = Mockito.inOrder(mWakeupLock);
        InOrder metricsInOrder = Mockito.inOrder(mWifiWakeMetrics);

        mWakeupController.start();
        lockInOrder.verify(mWakeupLock).setLock(any());
        metricsInOrder.verify(mWifiWakeMetrics).recordStartEvent(anyInt());

        mWakeupController.stop();
        mWakeupController.reset();
        metricsInOrder.verify(mWifiWakeMetrics).recordResetEvent(0 /* numScans */);

        mWakeupController.start();
        lockInOrder.verify(mWakeupLock).setLock(any());
        metricsInOrder.verify(mWifiWakeMetrics).recordStartEvent(anyInt());
    }

    /**
     * Verify that the wakeup lock is initialized with the intersection of ScanResults and saved
     * networks.
     */
    @Test
    public void startInitializesWakeupLockWithSavedScanResults() {
        String ssid1 = "ssid 1";
        String ssid2 = "ssid 2";
        String quotedSsid = ScanResultUtil.createQuotedSSID(ssid1);

        // saved configs
        WifiConfiguration openNetwork = WifiConfigurationTestUtil.createOpenNetwork(quotedSsid);
        openNetwork.getNetworkSelectionStatus().setHasEverConnected(true);
        WifiConfiguration wepNetwork = WifiConfigurationTestUtil.createWepNetwork();
        wepNetwork.getNetworkSelectionStatus().setHasEverConnected(true);
        when(mWifiConfigManager.getSavedNetworks())
                .thenReturn(Arrays.asList(openNetwork, wepNetwork));

        // scan results from most recent scan
        ScanResult savedScanResult = createOpenScanResult(ssid1, 2412 /* frequency */);
        ScanResult unsavedScanResult = createOpenScanResult(ssid2, 2412 /* frequency */);

        when(mWifiScanner.getSingleScanResults())
                .thenReturn(Arrays.asList(savedScanResult, unsavedScanResult));

        // intersection of most recent scan + saved configs
        Set<ScanResultMatchInfo> expectedMatchInfos =
                Collections.singleton(ScanResultMatchInfo.fromScanResult(savedScanResult));

        initializeWakeupController(true /* enabled */);
        mWakeupController.start();
        verify(mWakeupLock).setLock(eq(expectedMatchInfos));
        verify(mWifiWakeMetrics).recordStartEvent(expectedMatchInfos.size());
    }

    /**
     * Verify that start filters out DFS channels.
     */
    @Test
    public void startFiltersOutDfsScanResults() {
        String ssidDfs = "DFS scan";
        String ssid24 = "2.4 scan";

        // saved configs
        WifiConfiguration openNetworkDfs = WifiConfigurationTestUtil
                .createOpenNetwork(ScanResultUtil.createQuotedSSID(ssidDfs));
        openNetworkDfs.getNetworkSelectionStatus().setHasEverConnected(true);
        WifiConfiguration openNetwork24 = WifiConfigurationTestUtil
                .createOpenNetwork(ScanResultUtil.createQuotedSSID(ssid24));
        openNetwork24.getNetworkSelectionStatus().setHasEverConnected(true);

        when(mWifiConfigManager.getSavedNetworks())
                .thenReturn(Arrays.asList(openNetworkDfs, openNetwork24));

        // scan results from most recent scan
        ScanResult scanResultDfs = createOpenScanResult(ssidDfs, DFS_CHANNEL_FREQ);
        ScanResult scanResult24 = createOpenScanResult(ssid24, 2412 /* frequency */);

        when(mWifiScanner.getSingleScanResults())
                .thenReturn(Arrays.asList(scanResultDfs, scanResult24));

        // should filter out scanResultDfs
        Set<ScanResultMatchInfo> expectedMatchInfos =
                Collections.singleton(ScanResultMatchInfo.fromScanResult(scanResult24));

        initializeWakeupController(true /* enabled */);
        mWakeupController.start();
        verify(mWakeupLock).setLock(eq(expectedMatchInfos));
        verify(mWifiWakeMetrics).recordStartEvent(expectedMatchInfos.size());
    }

    /**
     * Verify that onResults updates the WakeupLock.
     */
    @Test
    public void onResultsUpdatesWakeupLock() {
        // saved config
        WifiConfiguration openNetwork = WifiConfigurationTestUtil
                .createOpenNetwork(ScanResultUtil.createQuotedSSID(SAVED_SSID));
        openNetwork.getNetworkSelectionStatus().setHasEverConnected(true);
        when(mWifiConfigManager.getSavedNetworks())
                .thenReturn(Collections.singletonList(openNetwork));

        initializeWakeupController(true /* enabled */);
        mWakeupController.start();

        ArgumentCaptor<WifiScanner.ScanListener> scanListenerArgumentCaptor =
                ArgumentCaptor.forClass(WifiScanner.ScanListener.class);

        verify(mWifiScanner).registerScanListener(scanListenerArgumentCaptor.capture());
        WifiScanner.ScanListener scanListener = scanListenerArgumentCaptor.getValue();

        // incoming scan results
        scanListener.onResults(mTestScanDatas);

        ScanResultMatchInfo expectedMatchInfo = ScanResultMatchInfo.fromScanResult(mTestScanResult);
        verify(mWakeupLock).update(eq(Collections.singleton(expectedMatchInfo)));
    }


    /**
     * Verify that onResults filters out unsaved networks when updating the WakeupLock.
     */
    @Test
    public void onResultsUpdatesWakeupLockWithOnlySavedNetworks() {
        // no saved configs
        when(mWifiConfigManager.getSavedNetworks()).thenReturn(Collections.emptyList());

        initializeWakeupController(true /* enabled */);
        mWakeupController.start();

        ArgumentCaptor<WifiScanner.ScanListener> scanListenerArgumentCaptor =
                ArgumentCaptor.forClass(WifiScanner.ScanListener.class);

        verify(mWifiScanner).registerScanListener(scanListenerArgumentCaptor.capture());
        WifiScanner.ScanListener scanListener = scanListenerArgumentCaptor.getValue();

        // incoming scan results
        scanListener.onResults(mTestScanDatas);

        verify(mWakeupLock).update(eq(Collections.emptySet()));
    }

    /**
     * Verify that onResults filters out DFS channels.
     */
    @Test
    public void onResultsFiltersOutDfsScanResults() {
        initializeWakeupController(true /* enabled */);
        mWakeupController.start();

        ArgumentCaptor<WifiScanner.ScanListener> scanListenerArgumentCaptor =
                ArgumentCaptor.forClass(WifiScanner.ScanListener.class);

        verify(mWifiScanner).registerScanListener(scanListenerArgumentCaptor.capture());
        WifiScanner.ScanListener scanListener = scanListenerArgumentCaptor.getValue();

        // incoming scan results
        mTestScanResult.frequency = DFS_CHANNEL_FREQ;
        scanListener.onResults(mTestScanDatas);

        verify(mWakeupLock).update(eq(Collections.emptySet()));
    }

    /**
     * Verify that the controller searches for viable networks during onResults when WakeupLock is
     * unlocked.
     */
    @Test
    public void onResultsSearchesForViableNetworkWhenWakeupLockIsUnlocked() {
        // unlock wakeup lock
        when(mWakeupLock.isUnlocked()).thenReturn(true);
        // do not find viable network
        when(mWakeupEvaluator.findViableNetwork(any(), any())).thenReturn(null);

        initializeWakeupController(true /* enabled */);
        mWakeupController.start();

        ArgumentCaptor<WifiScanner.ScanListener> scanListenerArgumentCaptor =
                ArgumentCaptor.forClass(WifiScanner.ScanListener.class);

        verify(mWifiScanner).registerScanListener(scanListenerArgumentCaptor.capture());
        WifiScanner.ScanListener scanListener = scanListenerArgumentCaptor.getValue();

        // incoming scan results
        scanListener.onResults(mTestScanDatas);

        verify(mWakeupEvaluator).findViableNetwork(any(), any());
        verifyDoesNotEnableWifi();
    }

    /**
     * Verify that the controller updates the WakeupLock even if the user is not onboarded.
     */
    @Test
    public void onResultsUpdatesIfNotOnboarded() {
        initializeWakeupController(true /* enabled */);
        when(mWakeupOnboarding.isOnboarded()).thenReturn(false);
        when(mWakeupLock.isUnlocked()).thenReturn(false);
        mWakeupController.start();

        ArgumentCaptor<WifiScanner.ScanListener> scanListenerArgumentCaptor =
                ArgumentCaptor.forClass(WifiScanner.ScanListener.class);

        verify(mWifiScanner).registerScanListener(scanListenerArgumentCaptor.capture());
        WifiScanner.ScanListener scanListener = scanListenerArgumentCaptor.getValue();

        // incoming scan results
        scanListener.onResults(mTestScanDatas);

        verify(mWakeupLock).update(any());
        verify(mWakeupLock).isUnlocked();
        verifyDoesNotEnableWifi();
    }

    /**
     * Verify that the controller enables wifi and notifies user when all criteria are met.
     */
    @Test
    public void onResultsEnablesWifi() {
        // unlock wakeup lock
        when(mWakeupLock.isUnlocked()).thenReturn(true);
        // find viable network
        when(mWakeupEvaluator.findViableNetwork(any(), any())).thenReturn(mTestScanResult);

        initializeWakeupController(true /* enabled */);
        mWakeupController.start();

        ArgumentCaptor<WifiScanner.ScanListener> scanListenerArgumentCaptor =
                ArgumentCaptor.forClass(WifiScanner.ScanListener.class);

        verify(mWifiScanner).registerScanListener(scanListenerArgumentCaptor.capture());
        WifiScanner.ScanListener scanListener = scanListenerArgumentCaptor.getValue();

        // incoming scan results
        scanListener.onResults(mTestScanDatas);

        verify(mWakeupEvaluator).findViableNetwork(any(), any());
        verify(mWifiSettingsStore).handleWifiToggled(true /* wifiEnabled */);
        verify(mWifiWakeMetrics).recordWakeupEvent(1 /* numScans */);
    }

    /**
     * Verify that the controller will not do any work if the user store has not been read.
     */
    @Test
    public void controllerDoesNoWorkIfUserStoreIsNotRead() {
        initializeWakeupController(true /* enabled */, false /* isRead */);
        mWakeupController.start();

        ArgumentCaptor<WifiScanner.ScanListener> scanListenerArgumentCaptor =
                ArgumentCaptor.forClass(WifiScanner.ScanListener.class);

        verify(mWifiScanner).registerScanListener(scanListenerArgumentCaptor.capture());
        WifiScanner.ScanListener scanListener = scanListenerArgumentCaptor.getValue();

        // incoming scan results
        scanListener.onResults(mTestScanDatas);

        verify(mWakeupLock, never()).setLock(any());
        verify(mWakeupLock, never()).update(any());
        verify(mWakeupLock, never()).isUnlocked();
        verify(mWakeupOnboarding, never()).maybeShowNotification();
        verify(mWakeupEvaluator, never()).findViableNetwork(any(), any());
    }

    @Test
    public void userIsNotOnboardedByInitialization() {
        initializeWakeupController(true /* enabled */);
        verify(mWakeupOnboarding, never()).setOnboarded();
    }

    @Test
    public void userIsOnboardedBySettingChange() {
        initializeWakeupController(true /* enabled */);
        ArgumentCaptor<ContentObserver> argumentCaptor =
                ArgumentCaptor.forClass(ContentObserver.class);
        verify(mFrameworkFacade).registerContentObserver(any(), any(), eq(true),
                argumentCaptor.capture());
        ContentObserver contentObserver = argumentCaptor.getValue();
        contentObserver.onChange(false /* selfChange */);
        verify(mWakeupOnboarding).setOnboarded();
    }
}
