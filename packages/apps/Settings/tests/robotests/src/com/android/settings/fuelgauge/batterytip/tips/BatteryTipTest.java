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
package com.android.settings.fuelgauge.batterytip.tips;

import static com.google.common.truth.Truth.assertThat;

import android.content.Context;
import android.os.Parcel;
import android.os.Parcelable;
import android.support.annotation.IdRes;
import android.support.v7.preference.Preference;

import com.android.settings.R;
import com.android.settings.testutils.SettingsRobolectricTestRunner;
import com.android.settingslib.core.instrumentation.MetricsFeatureProvider;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RuntimeEnvironment;

import java.util.ArrayList;
import java.util.List;

@RunWith(SettingsRobolectricTestRunner.class)
public class BatteryTipTest {

    private static final String TITLE = "title";
    private static final String SUMMARY = "summary";
    @IdRes
    private static final int ICON_ID = R.drawable.ic_fingerprint;

    private Context mContext;
    private TestBatteryTip mBatteryTip;

    @Before
    public void setUp() {
        mContext = RuntimeEnvironment.application;
        mBatteryTip = new TestBatteryTip();
    }

    @Test
    public void buildPreference() {
        final Preference preference = mBatteryTip.buildPreference(mContext);

        assertThat(preference.getTitle()).isEqualTo(TITLE);
        assertThat(preference.getSummary()).isEqualTo(SUMMARY);
        assertThat(preference.getIcon()).isEqualTo(mContext.getDrawable(ICON_ID));
    }

    @Test
    public void parcelable() {
        final BatteryTip batteryTip = new TestBatteryTip();

        Parcel parcel = Parcel.obtain();
        batteryTip.writeToParcel(parcel, batteryTip.describeContents());
        parcel.setDataPosition(0);

        final BatteryTip parcelTip = new TestBatteryTip(parcel);

        assertThat(parcelTip.getTitle(mContext)).isEqualTo(TITLE);
        assertThat(parcelTip.getSummary(mContext)).isEqualTo(SUMMARY);
        assertThat(parcelTip.getIconId()).isEqualTo(ICON_ID);
        assertThat(parcelTip.needUpdate()).isTrue();
    }

    @Test
    public void tipOrder_orderUnique() {
        final List<Integer> orders = new ArrayList<>();
        for (int i = 0, size = BatteryTip.TIP_ORDER.size(); i < size; i++) {
            orders.add(BatteryTip.TIP_ORDER.valueAt(i));
        }

        assertThat(orders).containsNoDuplicates();
    }

    @Test
    public void toString_containBatteryTipData() {
        assertThat(mBatteryTip.toString()).isEqualTo("type=6 state=0");
    }

    /**
     * Used to test the non abstract methods in {@link TestBatteryTip}
     */
    public static class TestBatteryTip extends BatteryTip {
        TestBatteryTip() {
            super(TipType.SUMMARY, StateType.NEW, true);
        }

        TestBatteryTip(Parcel in) {
            super(in);
        }

        @Override
        public String getTitle(Context context) {
            return TITLE;
        }

        @Override
        public String getSummary(Context context) {
            return SUMMARY;
        }

        @Override
        public int getIconId() {
            return ICON_ID;
        }

        @Override
        public void updateState(BatteryTip tip) {
            // do nothing
        }

        @Override
        public void log(Context context, MetricsFeatureProvider metricsFeatureProvider) {
            // do nothing
        }

        public final Parcelable.Creator CREATOR = new Parcelable.Creator() {
            public BatteryTip createFromParcel(Parcel in) {
                return new TestBatteryTip(in);
            }

            public BatteryTip[] newArray(int size) {
                return new TestBatteryTip[size];
            }
        };
    }
}
