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
 * limitations under the License
 */

package com.android.settings;

import static org.mockito.Matchers.any;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.AppOpsManager;
import android.app.admin.DeviceAdminInfo;
import android.content.Context;

import com.android.internal.logging.nano.MetricsProto;
import com.android.settings.fuelgauge.BatteryUtils;
import com.android.settings.testutils.FakeFeatureFactory;
import com.android.settings.testutils.SettingsRobolectricTestRunner;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Answers;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.Robolectric;

@RunWith(SettingsRobolectricTestRunner.class)
public class DeviceAdminAddTest {
    private static final int UID = 12345;
    private static final String PACKAGE_NAME = "com.android.test.device.admin";

    @Mock(answer = Answers.RETURNS_DEEP_STUBS)
    private DeviceAdminInfo mDeviceAdmin;
    @Mock
    private BatteryUtils mBatteryUtils;
    private FakeFeatureFactory mFeatureFactory;
    private DeviceAdminAdd mDeviceAdminAdd;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);

        mFeatureFactory = FakeFeatureFactory.setupForTest();
        mDeviceAdminAdd = Robolectric.buildActivity(DeviceAdminAdd.class).get();

        doReturn(UID).when(mBatteryUtils).getPackageUid(PACKAGE_NAME);
        when(mDeviceAdmin.getComponent().getPackageName()).thenReturn(PACKAGE_NAME);
        mDeviceAdminAdd.mDeviceAdmin = mDeviceAdmin;
    }

    @Test
    public void logSpecialPermissionChange() {
        mDeviceAdminAdd.logSpecialPermissionChange(true, "app");
        verify(mFeatureFactory.metricsFeatureProvider).action(any(Context.class),
                eq(MetricsProto.MetricsEvent.APP_SPECIAL_PERMISSION_ADMIN_ALLOW), eq("app"));

        mDeviceAdminAdd.logSpecialPermissionChange(false, "app");
        verify(mFeatureFactory.metricsFeatureProvider).action(any(Context.class),
                eq(MetricsProto.MetricsEvent.APP_SPECIAL_PERMISSION_ADMIN_DENY), eq("app"));
    }

    @Test
    public void unrestrictAppIfPossible_appRestricted_unrestrictApp() {
        doReturn(true).when(mBatteryUtils).isForceAppStandbyEnabled(UID, PACKAGE_NAME);

        mDeviceAdminAdd.unrestrictAppIfPossible(mBatteryUtils);

        verify(mBatteryUtils).setForceAppStandby(UID, PACKAGE_NAME, AppOpsManager.MODE_ALLOWED);
    }

    @Test
    public void unrestrictAppIfPossible_appUnrestricted_doNothing() {
        doReturn(false).when(mBatteryUtils).isForceAppStandbyEnabled(UID, PACKAGE_NAME);

        mDeviceAdminAdd.unrestrictAppIfPossible(mBatteryUtils);

        verify(mBatteryUtils, never()).setForceAppStandby(UID, PACKAGE_NAME,
                AppOpsManager.MODE_ALLOWED);
    }
}
