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

package com.android.settings.fuelgauge;

import static com.google.common.truth.Truth.assertThat;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.support.v7.preference.PreferenceGroup;
import android.text.TextUtils;
import android.text.format.DateUtils;
import android.util.FeatureFlagUtils;
import android.util.SparseArray;

import com.android.internal.os.BatterySipper;
import com.android.internal.os.BatteryStatsImpl;
import com.android.settings.R;
import com.android.settings.SettingsActivity;
import com.android.settings.core.FeatureFlags;
import com.android.settings.core.InstrumentedPreferenceFragment;
import com.android.settings.fuelgauge.anomaly.Anomaly;
import com.android.settings.testutils.FakeFeatureFactory;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;

import java.util.List;

@RunWith(RobolectricTestRunner.class)
public class BatteryAppListPreferenceControllerTest {

    private static final String[] PACKAGE_NAMES = {"com.app1", "com.app2"};
    private static final String KEY_APP_LIST = "app_list";
    private static final int UID = 123;

    @Mock
    private BatterySipper mNormalBatterySipper;
    @Mock
    private SettingsActivity mSettingsActivity;
    @Mock
    private PreferenceGroup mAppListGroup;
    @Mock
    private InstrumentedPreferenceFragment mFragment;
    @Mock
    private BatteryUtils mBatteryUtils;

    private Context mContext;
    private PowerGaugePreference mPreference;
    private BatteryAppListPreferenceController mPreferenceController;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);

        mContext = spy(RuntimeEnvironment.application);
        FakeFeatureFactory.setupForTest();

        mPreference = new PowerGaugePreference(mContext);
        when(mNormalBatterySipper.getPackages()).thenReturn(PACKAGE_NAMES);
        when(mNormalBatterySipper.getUid()).thenReturn(UID);
        mNormalBatterySipper.drainType = BatterySipper.DrainType.APP;

        mPreferenceController = new BatteryAppListPreferenceController(mContext, KEY_APP_LIST, null,
                mSettingsActivity, mFragment);
        mPreferenceController.mBatteryUtils = mBatteryUtils;
        mPreferenceController.mAppListGroup = mAppListGroup;
    }

    @Test
    public void testExtractKeyFromSipper_typeAPPUidObjectNull_returnPackageNames() {
        mNormalBatterySipper.uidObj = null;
        mNormalBatterySipper.drainType = BatterySipper.DrainType.APP;

        final String key = mPreferenceController.extractKeyFromSipper(mNormalBatterySipper);
        assertThat(key).isEqualTo(TextUtils.concat(mNormalBatterySipper.getPackages()).toString());
    }

    @Test
    public void testExtractKeyFromSipper_typeOther_returnDrainType() {
        mNormalBatterySipper.uidObj = null;
        mNormalBatterySipper.drainType = BatterySipper.DrainType.BLUETOOTH;

        final String key = mPreferenceController.extractKeyFromSipper(mNormalBatterySipper);
        assertThat(key).isEqualTo(mNormalBatterySipper.drainType.toString());
    }

    @Test
    public void testExtractKeyFromSipper_typeUser_returnDrainTypeWithUserId() {
        mNormalBatterySipper.uidObj = null;
        mNormalBatterySipper.drainType = BatterySipper.DrainType.USER;
        mNormalBatterySipper.userId = 2;

        final String key = mPreferenceController.extractKeyFromSipper(mNormalBatterySipper);
        assertThat(key).isEqualTo("USER2");
    }

    @Test
    public void testExtractKeyFromSipper_typeAPPUidObjectNotNull_returnUid() {
        mNormalBatterySipper.uidObj = new BatteryStatsImpl.Uid(new BatteryStatsImpl(), UID);
        mNormalBatterySipper.drainType = BatterySipper.DrainType.APP;

        final String key = mPreferenceController.extractKeyFromSipper(mNormalBatterySipper);
        assertThat(key).isEqualTo(Integer.toString(mNormalBatterySipper.getUid()));
    }

    @Test
    public void testSetUsageSummary_timeLessThanOneMinute_DoNotSetSummary() {
        mNormalBatterySipper.usageTimeMs = 59 * DateUtils.SECOND_IN_MILLIS;

        mPreferenceController.setUsageSummary(mPreference, mNormalBatterySipper);
        assertThat(mPreference.getSummary()).isNull();
    }

    @Test
    public void testSetUsageSummary_timeMoreThanOneMinute_normalApp_setScreenSummary() {
        mNormalBatterySipper.usageTimeMs = 2 * DateUtils.MINUTE_IN_MILLIS;
        doReturn(mContext.getText(R.string.battery_used_for)).when(mFragment).getText(
                R.string.battery_used_for);
        doReturn(mContext).when(mFragment).getContext();

        mPreferenceController.setUsageSummary(mPreference, mNormalBatterySipper);

        assertThat(mPreference.getSummary().toString()).isEqualTo("Used for 2 min");
    }

    @Test
    public void testSetUsageSummary_timeMoreThanOneMinute_hiddenApp_setUsedSummary() {
        mNormalBatterySipper.usageTimeMs = 2 * DateUtils.MINUTE_IN_MILLIS;
        doReturn(true).when(mBatteryUtils).shouldHideSipper(mNormalBatterySipper);
        doReturn(mContext).when(mFragment).getContext();

        mPreferenceController.setUsageSummary(mPreference, mNormalBatterySipper);

        assertThat(mPreference.getSummary().toString()).isEqualTo("2 min");
    }

    @Test
    public void testSetUsageSummary_timeMoreThanOneMinute_notApp_setUsedSummary() {
        mNormalBatterySipper.usageTimeMs = 2 * DateUtils.MINUTE_IN_MILLIS;
        mNormalBatterySipper.drainType = BatterySipper.DrainType.PHONE;
        doReturn(mContext).when(mFragment).getContext();

        mPreferenceController.setUsageSummary(mPreference, mNormalBatterySipper);

        assertThat(mPreference.getSummary().toString()).isEqualTo("2 min");
    }

    @Test
    public void testRefreshAnomalyIcon_containsAnomaly_showAnomalyIcon() {
        FeatureFlagUtils.setEnabled(mContext, FeatureFlags.BATTERY_DISPLAY_APP_LIST, true);
        PowerGaugePreference preference = new PowerGaugePreference(mContext);
        final String key = mPreferenceController.extractKeyFromUid(UID);
        final SparseArray<List<Anomaly>> anomalySparseArray = new SparseArray<>();
        anomalySparseArray.append(UID, null);
        preference.setKey(key);
        doReturn(preference).when(mAppListGroup).findPreference(key);

        mPreferenceController.refreshAnomalyIcon(anomalySparseArray);

        assertThat(preference.showAnomalyIcon()).isTrue();
    }

    @Test
    public void testShouldHideSipper_typeOvercounted_returnTrue() {
        mNormalBatterySipper.drainType = BatterySipper.DrainType.OVERCOUNTED;

        assertThat(mPreferenceController.shouldHideSipper(mNormalBatterySipper)).isTrue();
    }

    @Test
    public void testShouldHideSipper_typeUnaccounted_returnTrue() {
        mNormalBatterySipper.drainType = BatterySipper.DrainType.UNACCOUNTED;

        assertThat(mPreferenceController.shouldHideSipper(mNormalBatterySipper)).isTrue();
    }

    @Test
    public void testShouldHideSipper_typeNormal_returnFalse() {
        mNormalBatterySipper.drainType = BatterySipper.DrainType.APP;

        assertThat(mPreferenceController.shouldHideSipper(mNormalBatterySipper)).isFalse();
    }

    @Test
    public void testNeverUseFakeData() {
        assertThat(BatteryAppListPreferenceController.USE_FAKE_DATA).isFalse();
    }
}
