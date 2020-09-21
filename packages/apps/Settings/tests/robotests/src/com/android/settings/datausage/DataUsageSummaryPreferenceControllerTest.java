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

package com.android.settings.datausage;

import static android.net.ConnectivityManager.TYPE_WIFI;
import static com.android.settings.core.BasePreferenceController.AVAILABLE;
import static com.android.settings.core.BasePreferenceController.CONDITIONALLY_UNAVAILABLE;

import static com.google.common.truth.Truth.assertThat;
import static org.mockito.Matchers.any;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.net.ConnectivityManager;
import android.net.NetworkTemplate;
import android.support.v7.widget.RecyclerView;
import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;

import com.android.internal.logging.nano.MetricsProto;
import com.android.settings.R;
import com.android.settings.testutils.FakeFeatureFactory;
import com.android.settings.testutils.SettingsRobolectricTestRunner;
import com.android.settings.testutils.shadow.ShadowEntityHeaderController;
import com.android.settings.widget.EntityHeaderController;
import com.android.settingslib.NetworkPolicyEditor;
import com.android.settingslib.core.lifecycle.Lifecycle;
import com.android.settingslib.net.DataUsageController;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Answers;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.Robolectric;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;

import java.util.concurrent.TimeUnit;

@RunWith(SettingsRobolectricTestRunner.class)
@Config(shadows = ShadowEntityHeaderController.class)
public class DataUsageSummaryPreferenceControllerTest {

    private static final long UPDATE_BACKOFF_MS = TimeUnit.MINUTES.toMillis(13);
    private static final long CYCLE_BACKOFF_MS = TimeUnit.DAYS.toMillis(6);
    private static final long CYCLE_LENGTH_MS = TimeUnit.DAYS.toMillis(30);
    private static final long USAGE1 =  373 * BillingCycleSettings.MIB_IN_BYTES;
    private static final long LIMIT1 = BillingCycleSettings.GIB_IN_BYTES;
    private static final String CARRIER_NAME = "z-mobile";
    private static final String PERIOD = "Feb";

    @Mock
    private DataUsageController mDataUsageController;
    @Mock
    private DataUsageSummaryPreference mSummaryPreference;
    @Mock
    private NetworkPolicyEditor mPolicyEditor;
    @Mock
    private NetworkTemplate mNetworkTemplate;
    @Mock
    private SubscriptionManager mSubscriptionManager;
    @Mock
    private Lifecycle mLifecycle;
    @Mock(answer = Answers.RETURNS_DEEP_STUBS)
    private EntityHeaderController mHeaderController;
    @Mock
    private DataUsageSummary mDataUsageSummary;
    @Mock
    private TelephonyManager mTelephonyManager;
    @Mock
    private ConnectivityManager mConnectivityManager;

    private DataUsageInfoController mDataInfoController;

    private FakeFeatureFactory mFactory;
    private Activity mActivity;
    private Context mContext;
    private DataUsageSummaryPreferenceController mController;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mContext = spy(RuntimeEnvironment.application);

        doReturn("%1$s %2%s").when(mContext)
            .getString(com.android.internal.R.string.fileSizeSuffix);

        mFactory = FakeFeatureFactory.setupForTest();
        when(mFactory.metricsFeatureProvider.getMetricsCategory(any(Object.class)))
                .thenReturn(MetricsProto.MetricsEvent.SETTINGS_APP_NOTIF_CATEGORY);
        ShadowEntityHeaderController.setUseMock(mHeaderController);
        mDataInfoController = new DataUsageInfoController();

        mActivity = spy(Robolectric.buildActivity(Activity.class).get());
        when(mActivity.getSystemService(TelephonyManager.class)).thenReturn(mTelephonyManager);
        when(mActivity.getSystemService(ConnectivityManager.class))
                .thenReturn(mConnectivityManager);
        when(mTelephonyManager.getSimState()).thenReturn(TelephonyManager.SIM_STATE_READY);
        when(mConnectivityManager.isNetworkSupported(TYPE_WIFI)).thenReturn(false);
        mController = new DataUsageSummaryPreferenceController(
                mDataUsageController,
                mDataInfoController,
                mNetworkTemplate,
                mPolicyEditor,
                R.string.cell_data_template,
                true,
                null,
                mActivity, null, null, null);
    }

    @After
    public void tearDown() {
        ShadowEntityHeaderController.reset();
    }

    @Test
    public void testSummaryUpdate_onePlan_basic() {
        final long now = System.currentTimeMillis();
        final DataUsageController.DataUsageInfo info = createTestDataUsageInfo(now);

        final Intent intent = new Intent();

        when(mDataUsageController.getDataUsageInfo(any())).thenReturn(info);
        mController.setPlanValues(1 /* dataPlanCount */, LIMIT1, USAGE1);
        mController.setCarrierValues(CARRIER_NAME, now - UPDATE_BACKOFF_MS, info.cycleEnd, intent);

        mController.updateState(mSummaryPreference);
        verify(mSummaryPreference).setLimitInfo("512 MB data warning / 1.00 GB data limit");
        verify(mSummaryPreference).setUsageInfo(info.cycleEnd, now - UPDATE_BACKOFF_MS,
                CARRIER_NAME, 1 /* numPlans */, intent);
        verify(mSummaryPreference).setChartEnabled(true);
        verify(mSummaryPreference).setWifiMode(false, null);
    }

    @Test
    public void testSummaryUpdate_noPlan_basic() {
        final long now = System.currentTimeMillis();
        final DataUsageController.DataUsageInfo info = createTestDataUsageInfo(now);

        final Intent intent = new Intent();

        when(mDataUsageController.getDataUsageInfo(any())).thenReturn(info);
        mController.setPlanValues(0 /* dataPlanCount */, LIMIT1, USAGE1);
        mController.setCarrierValues(CARRIER_NAME, now - UPDATE_BACKOFF_MS, info.cycleEnd, intent);

        mController.updateState(mSummaryPreference);
        verify(mSummaryPreference).setLimitInfo("512 MB data warning / 1.00 GB data limit");
        verify(mSummaryPreference).setUsageInfo(info.cycleEnd, now - UPDATE_BACKOFF_MS,
                CARRIER_NAME, 0 /* numPlans */, intent);
        verify(mSummaryPreference).setChartEnabled(true);
        verify(mSummaryPreference).setWifiMode(false, null);
    }

    @Test
    public void testSummaryUpdate_noCarrier_basic() {
        final long now = System.currentTimeMillis();
        final DataUsageController.DataUsageInfo info = createTestDataUsageInfo(now);

        when(mDataUsageController.getDataUsageInfo(any())).thenReturn(info);
        mController.setPlanValues(0 /* dataPlanCount */, LIMIT1, USAGE1);
        mController.setCarrierValues(null /* carrierName */, -1L /* snapshotTime */,
                info.cycleEnd, null /* intent */);
        mController.updateState(mSummaryPreference);

        verify(mSummaryPreference).setLimitInfo("512 MB data warning / 1.00 GB data limit");
        verify(mSummaryPreference).setUsageInfo(
                info.cycleEnd,
                -1L /* snapshotTime */,
                null /* carrierName */,
                0 /* numPlans */,
                null /* launchIntent */);
        verify(mSummaryPreference).setChartEnabled(true);
        verify(mSummaryPreference).setWifiMode(false, null);
    }

    @Test
    public void testSummaryUpdate_noPlanData_basic() {
        final long now = System.currentTimeMillis();

        final DataUsageController.DataUsageInfo info = createTestDataUsageInfo(now);

        when(mDataUsageController.getDataUsageInfo(any())).thenReturn(info);
        mController.setPlanValues(0 /* dataPlanCount */, -1L /* dataPlanSize */, USAGE1);
        mController.setCarrierValues(null /* carrierName */, -1L /* snapshotTime */,
                info.cycleEnd, null /* intent */);
        mController.updateState(mSummaryPreference);

        verify(mSummaryPreference).setLimitInfo("512 MB data warning / 1.00 GB data limit");
        verify(mSummaryPreference).setUsageInfo(
                info.cycleEnd,
                -1L /* snapshotTime */,
                null /* carrierName */,
                0 /* numPlans */,
                null /* launchIntent */);
        verify(mSummaryPreference).setChartEnabled(false);
        verify(mSummaryPreference).setWifiMode(false, null);
    }

    @Test
    public void testSummaryUpdate_noLimitNoWarning() {
        final long now = System.currentTimeMillis();
        final DataUsageController.DataUsageInfo info = createTestDataUsageInfo(now);
        info.warningLevel = 0L;
        info.limitLevel = 0L;

        final Intent intent = new Intent();

        when(mDataUsageController.getDataUsageInfo(any())).thenReturn(info);
        mController.setPlanValues(0 /* dataPlanCount */, LIMIT1, USAGE1);
        mController.setCarrierValues(CARRIER_NAME, now - UPDATE_BACKOFF_MS, info.cycleEnd, intent);

        mController.updateState(mSummaryPreference);
        verify(mSummaryPreference).setLimitInfo(null);
    }

    @Test
    public void testSummaryUpdate_warningOnly() {
        final long now = System.currentTimeMillis();
        final DataUsageController.DataUsageInfo info = createTestDataUsageInfo(now);
        info.warningLevel = BillingCycleSettings.MIB_IN_BYTES;
        info.limitLevel = 0L;

        final Intent intent = new Intent();

        when(mDataUsageController.getDataUsageInfo(any())).thenReturn(info);
        mController.setPlanValues(0 /* dataPlanCount */, LIMIT1, USAGE1);
        mController.setCarrierValues(CARRIER_NAME, now - UPDATE_BACKOFF_MS, info.cycleEnd, intent);

        mController.updateState(mSummaryPreference);
        verify(mSummaryPreference).setLimitInfo("1.00 MB data warning");
    }

    @Test
    public void testSummaryUpdate_limitOnly() {
        final long now = System.currentTimeMillis();
        final DataUsageController.DataUsageInfo info = createTestDataUsageInfo(now);
        info.warningLevel = 0L;
        info.limitLevel = BillingCycleSettings.MIB_IN_BYTES;

        final Intent intent = new Intent();

        when(mDataUsageController.getDataUsageInfo(any())).thenReturn(info);
        mController.setPlanValues(0 /* dataPlanCount */, LIMIT1, USAGE1);
        mController.setCarrierValues(CARRIER_NAME, now - UPDATE_BACKOFF_MS, info.cycleEnd, intent);

        mController.updateState(mSummaryPreference);
        verify(mSummaryPreference).setLimitInfo("1.00 MB data limit");
    }

    @Test
    public void testSummaryUpdate_limitAndWarning() {
        final long now = System.currentTimeMillis();
        final DataUsageController.DataUsageInfo info = createTestDataUsageInfo(now);
        info.warningLevel = BillingCycleSettings.MIB_IN_BYTES;
        info.limitLevel = BillingCycleSettings.MIB_IN_BYTES;

        final Intent intent = new Intent();

        when(mDataUsageController.getDataUsageInfo(any())).thenReturn(info);
        mController.setPlanValues(0 /* dataPlanCount */, LIMIT1, USAGE1);
        mController.setCarrierValues(CARRIER_NAME, now - UPDATE_BACKOFF_MS, info.cycleEnd, intent);

        mController.updateState(mSummaryPreference);
        verify(mSummaryPreference).setLimitInfo("1.00 MB data warning / 1.00 MB data limit");
        verify(mSummaryPreference).setWifiMode(false, null);
    }

    @Test
    public void testSummaryUpdate_noSim_shouldSetWifiMode() {
        final long now = System.currentTimeMillis();
        final DataUsageController.DataUsageInfo info = createTestDataUsageInfo(now);
        info.warningLevel = BillingCycleSettings.MIB_IN_BYTES;
        info.limitLevel = BillingCycleSettings.MIB_IN_BYTES;

        final Intent intent = new Intent();

        when(mDataUsageController.getDataUsageInfo(any())).thenReturn(info);
        mController.setPlanValues(0 /* dataPlanCount */, LIMIT1, USAGE1);
        mController.setCarrierValues(CARRIER_NAME, now - UPDATE_BACKOFF_MS, info.cycleEnd, intent);

        when(mTelephonyManager.getSimState()).thenReturn(TelephonyManager.SIM_STATE_ABSENT);
        mController.updateState(mSummaryPreference);

        verify(mSummaryPreference).setWifiMode(true, info.period);
        verify(mSummaryPreference).setLimitInfo(null);
        verify(mSummaryPreference).setUsageNumbers(info.usageLevel, -1L, true);
        verify(mSummaryPreference).setChartEnabled(false);
        verify(mSummaryPreference).setUsageInfo(info.cycleEnd, -1L, null, 0, null);
    }

    @Test
    public void testMobileData_preferenceAvailable() {
        mController = new DataUsageSummaryPreferenceController(
                mDataUsageController,
                mDataInfoController,
                mNetworkTemplate,
                mPolicyEditor,
                R.string.cell_data_template,
                true,
                mSubscriptionManager,
                mActivity, null, null, null);

        final SubscriptionInfo subInfo = new SubscriptionInfo(0, "123456", 0, "name", "carrier",
                0, 0, "number", 0, null, 123, 456, "ZX");
        when(mSubscriptionManager.getDefaultDataSubscriptionInfo()).thenReturn(subInfo);
        assertThat(mController.getAvailabilityStatus()).isEqualTo(AVAILABLE);
    }

    @Test
    public void testMobileData_noSimNoWifi_preferenceDisabled() {
        mController = new DataUsageSummaryPreferenceController(
                mDataUsageController,
                mDataInfoController,
                mNetworkTemplate,
                mPolicyEditor,
                R.string.cell_data_template,
                true,
                mSubscriptionManager,
                mActivity, null, null, null);

        when(mTelephonyManager.getSimState()).thenReturn(TelephonyManager.SIM_STATE_ABSENT);
        when(mConnectivityManager.isNetworkSupported(TYPE_WIFI)).thenReturn(false);
        assertThat(mController.getAvailabilityStatus()).isEqualTo(CONDITIONALLY_UNAVAILABLE);
    }

    @Test
    public void testMobileData_noSimWifi_preferenceDisabled() {
        mController = new DataUsageSummaryPreferenceController(
                mDataUsageController,
                mDataInfoController,
                mNetworkTemplate,
                mPolicyEditor,
                R.string.cell_data_template,
                true,
                mSubscriptionManager,
                mActivity, null, null, null);

        when(mTelephonyManager.getSimState()).thenReturn(TelephonyManager.SIM_STATE_ABSENT);
        when(mConnectivityManager.isNetworkSupported(TYPE_WIFI)).thenReturn(true);
        assertThat(mController.getAvailabilityStatus()).isEqualTo(AVAILABLE);
    }

    @Test
    public void testMobileData_entityHeaderSet() {
        final RecyclerView recyclerView = new RecyclerView(mActivity);

        mController = new DataUsageSummaryPreferenceController(
                mDataUsageController,
                mDataInfoController,
                mNetworkTemplate,
                mPolicyEditor,
                R.string.cell_data_template,
                true,
                mSubscriptionManager,
                mActivity, mLifecycle, mHeaderController, mDataUsageSummary);

        when(mDataUsageSummary.getListView()).thenReturn(recyclerView);

        mController.onStart();

        verify(mHeaderController)
                .setRecyclerView(any(RecyclerView.class), any(Lifecycle.class));
        verify(mHeaderController).styleActionBar(any(Activity.class));
    }

    private DataUsageController.DataUsageInfo createTestDataUsageInfo(long now) {
        DataUsageController.DataUsageInfo info = new DataUsageController.DataUsageInfo();
        info.carrier = CARRIER_NAME;
        info.period = PERIOD;
        info.startDate = now;
        info.limitLevel = LIMIT1;
        info.warningLevel = LIMIT1 >> 1;
        info.usageLevel = USAGE1;
        info.cycleStart = now - CYCLE_BACKOFF_MS;
        info.cycleEnd = info.cycleStart + CYCLE_LENGTH_MS;
        return info;
    }
}
