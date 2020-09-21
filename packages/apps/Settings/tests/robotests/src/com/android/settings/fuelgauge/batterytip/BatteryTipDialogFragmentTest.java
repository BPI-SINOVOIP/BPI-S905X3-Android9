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

package com.android.settings.fuelgauge.batterytip;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.spy;
import static org.robolectric.Shadows.shadowOf;

import android.app.AlertDialog;
import android.content.Context;
import android.text.format.DateUtils;

import com.android.settings.R;
import com.android.settings.fuelgauge.Estimate;
import com.android.settings.fuelgauge.batterytip.tips.BatteryTip;
import com.android.settings.fuelgauge.batterytip.tips.HighUsageTip;
import com.android.settings.fuelgauge.batterytip.tips.RestrictAppTip;
import com.android.settings.fuelgauge.batterytip.tips.SummaryTip;
import com.android.settings.fuelgauge.batterytip.tips.UnrestrictAppTip;
import com.android.settings.testutils.FakeFeatureFactory;
import com.android.settings.testutils.SettingsRobolectricTestRunner;
import com.android.settings.testutils.shadow.ShadowUtils;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.MockitoAnnotations;
import org.robolectric.Robolectric;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;
import org.robolectric.shadows.ShadowAlertDialog;
import org.robolectric.shadows.ShadowDialog;
import org.robolectric.util.FragmentTestUtil;

import java.util.ArrayList;
import java.util.List;

@RunWith(SettingsRobolectricTestRunner.class)
@Config(shadows = ShadowUtils.class)
public class BatteryTipDialogFragmentTest {

    private static final String PACKAGE_NAME = "com.android.app";
    private static final String DISPLAY_NAME = "app";
    private static final long SCREEN_TIME_MS = DateUtils.HOUR_IN_MILLIS;
    private static final long AVERAGE_TIME_MS = DateUtils.HOUR_IN_MILLIS;
    private static final int METRICS_KEY = 1;

    private BatteryTipDialogFragment mDialogFragment;
    private Context mContext;
    private HighUsageTip mHighUsageTip;
    private RestrictAppTip mRestrictedOneAppTip;
    private RestrictAppTip mRestrictTwoAppsTip;
    private UnrestrictAppTip mUnrestrictAppTip;
    private SummaryTip mSummaryTip;
    private AppInfo mAppInfo;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);

        mContext = spy(RuntimeEnvironment.application);
        FakeFeatureFactory.setupForTest();
        ShadowUtils.setApplicationLabel(PACKAGE_NAME, DISPLAY_NAME);

        List<AppInfo> highUsageTips = new ArrayList<>();
        mAppInfo = new AppInfo.Builder()
                .setScreenOnTimeMs(SCREEN_TIME_MS)
                .setPackageName(PACKAGE_NAME)
                .build();
        highUsageTips.add(mAppInfo);
        mHighUsageTip = new HighUsageTip(SCREEN_TIME_MS, highUsageTips);

        final List<AppInfo> restrictApps = new ArrayList<>();
        restrictApps.add(mAppInfo);
        mRestrictedOneAppTip = new RestrictAppTip(BatteryTip.StateType.NEW,
                new ArrayList<>(restrictApps));
        restrictApps.add(mAppInfo);
        mRestrictTwoAppsTip = new RestrictAppTip(BatteryTip.StateType.NEW,
                new ArrayList<>(restrictApps));

        mUnrestrictAppTip = new UnrestrictAppTip(BatteryTip.StateType.NEW, mAppInfo);
        mSummaryTip = spy(new SummaryTip(BatteryTip.StateType.NEW,
                Estimate.AVERAGE_TIME_TO_DISCHARGE_UNKNOWN));
    }

    @Test
    public void testOnCreateDialog_highUsageTip_fireHighUsageDialog() {
        Robolectric.getForegroundThreadScheduler().pause();

        mDialogFragment = BatteryTipDialogFragment.newInstance(mHighUsageTip, METRICS_KEY);

        FragmentTestUtil.startFragment(mDialogFragment);

        Robolectric.getForegroundThreadScheduler().advanceToLastPostedRunnable();

        final AlertDialog dialog = (AlertDialog) ShadowDialog.getLatestDialog();
        ShadowAlertDialog shadowDialog = shadowOf(dialog);

        assertThat(shadowDialog.getMessage()).isEqualTo(
                mContext.getString(R.string.battery_tip_dialog_message, 1));
    }

    @Test
    public void testOnCreateDialog_restrictOneAppTip_fireRestrictOneAppDialog() {
        mDialogFragment = BatteryTipDialogFragment.newInstance(mRestrictedOneAppTip, METRICS_KEY);

        FragmentTestUtil.startFragment(mDialogFragment);

        final AlertDialog dialog = (AlertDialog) ShadowDialog.getLatestDialog();
        ShadowAlertDialog shadowDialog = shadowOf(dialog);

        assertThat(shadowDialog.getTitle()).isEqualTo("Restrict app?");
        assertThat(shadowDialog.getMessage())
                .isEqualTo(
                        "To save battery, stop app from using battery in the background. This app"
                                + " may not work properly and notifications may be delayed.");
    }

    @Test
    public void testOnCreateDialog_restrictTwoAppsTip_fireRestrictTwoAppsDialog() {
        Robolectric.getForegroundThreadScheduler().pause();

        mDialogFragment = BatteryTipDialogFragment.newInstance(mRestrictTwoAppsTip, METRICS_KEY);


        FragmentTestUtil.startFragment(mDialogFragment);

        Robolectric.getForegroundThreadScheduler().advanceToLastPostedRunnable();

        final AlertDialog dialog = (AlertDialog) ShadowDialog.getLatestDialog();
        ShadowAlertDialog shadowDialog = shadowOf(dialog);

        assertThat(shadowDialog.getTitle()).isEqualTo("Restrict 2 apps?");
        assertThat(shadowDialog.getMessage())
                .isEqualTo(
                        "To save battery, stop these apps from using battery in the background. "
                                + "Restricted apps may not work properly and notifications may be"
                                + " delayed.\n\nApps:");
        assertThat(shadowDialog.getView()).isNotNull();
    }

    @Test
    public void testOnCreateDialog_restrictSixAppsTip_fireRestrictSixAppsDialog() {
        Robolectric.getForegroundThreadScheduler().pause();

        final List<AppInfo> appInfos = new ArrayList<>();
        for (int i = 0; i < 6; i++) {
            appInfos.add(mAppInfo);
        }
        final RestrictAppTip restrictSixAppsTip = new RestrictAppTip(BatteryTip.StateType.NEW,
                appInfos);

        mDialogFragment = BatteryTipDialogFragment.newInstance(restrictSixAppsTip, METRICS_KEY);

        FragmentTestUtil.startFragment(mDialogFragment);

        Robolectric.getForegroundThreadScheduler().advanceToLastPostedRunnable();

        final AlertDialog dialog = (AlertDialog) ShadowDialog.getLatestDialog();
        ShadowAlertDialog shadowDialog = shadowOf(dialog);

        assertThat(shadowDialog.getTitle()).isEqualTo("Restrict 6 apps?");
        assertThat(shadowDialog.getMessage())
                .isEqualTo(
                        "To save battery, stop these apps from using battery in the background. "
                                + "Restricted apps may not work properly and notifications may be"
                                + " delayed.\n\nApps:\napp, app, app, app, app, and app.");
    }

    @Test
    public void testOnCreateDialog_unRestrictAppTip_fireUnRestrictDialog() {
        mDialogFragment = BatteryTipDialogFragment.newInstance(mUnrestrictAppTip, METRICS_KEY);
        ShadowUtils.setApplicationLabel(PACKAGE_NAME, DISPLAY_NAME);

        FragmentTestUtil.startFragment(mDialogFragment);

        final AlertDialog dialog = (AlertDialog) ShadowDialog.getLatestDialog();
        ShadowAlertDialog shadowDialog = shadowOf(dialog);

        assertThat(shadowDialog.getTitle()).isEqualTo("Remove restriction?");
        assertThat(shadowDialog.getMessage())
                .isEqualTo(mContext.getString(R.string.battery_tip_unrestrict_app_dialog_message));
    }

    @Test
    public void testOnCreateDialog_summaryTip_fireDialog() {
        doReturn(AVERAGE_TIME_MS).when(mSummaryTip).getAverageTimeMs();
        mDialogFragment = BatteryTipDialogFragment.newInstance(mSummaryTip, METRICS_KEY);

        FragmentTestUtil.startFragment(mDialogFragment);

        final AlertDialog dialog = (AlertDialog) ShadowDialog.getLatestDialog();
        ShadowAlertDialog shadowDialog = shadowOf(dialog);

        assertThat(shadowDialog.getMessage()).isEqualTo(
                "Your apps are using a normal amount of battery. If apps use too much battery, "
                        + "your phone will suggest actions you can take.\n\nYou can always turn"
                        + " on Battery Saver if you’re running low on battery.");
    }
}
