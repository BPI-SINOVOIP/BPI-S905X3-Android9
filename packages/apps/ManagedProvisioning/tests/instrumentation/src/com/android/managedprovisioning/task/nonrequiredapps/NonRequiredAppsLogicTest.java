/*
 * Copyright 2016, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.managedprovisioning.task.nonrequiredapps;

import static android.app.admin.DevicePolicyManager.ACTION_PROVISION_MANAGED_DEVICE;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.ArgumentMatchers.nullable;
import static org.mockito.Matchers.anyInt;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.admin.DevicePolicyManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.pm.IPackageManager;
import android.support.test.filters.SmallTest;

import com.android.managedprovisioning.common.Utils;
import com.android.managedprovisioning.model.ProvisioningParams;

import org.junit.Before;
import org.junit.Test;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.Set;
import java.util.stream.Collectors;

/**
 * Unit tests for {@link NonRequiredAppsLogic}.
 */
@SmallTest
public class NonRequiredAppsLogicTest {
    private static final String TEST_DPC_PACKAGE_NAME = "dpc.package.name";
    private static final ComponentName TEST_MDM_COMPONENT_NAME = new ComponentName(
            TEST_DPC_PACKAGE_NAME, "pc.package.name.DeviceAdmin");

    private static final int TEST_USER_ID = 123;
    private static final List<String> APPS = Arrays.asList("app.a", "app.b", "app.c", "app.d",
            "app.e", "app.f", "app.g", "app.h");
    private static final List<Integer> SNAPSHOT_APPS = Arrays.asList(0, 1, 2, 3);
    private static final List<Integer> SYSTEM_APPS = Arrays.asList(0, 1, 4, 5);
    private static final List<Integer> BLACKLIST_APPS = Arrays.asList(0, 2, 4, 6);

    @Mock
    private DevicePolicyManager mDevicePolicyManager;
    @Mock
    private IPackageManager mIPackageManager;
    @Mock
    private SystemAppsSnapshot mSnapshot;
    @Mock
    private Utils mUtils;
    @Mock
    private Context mContext;

    private ProvisioningParams.Builder mParamsBuilder;

    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);
        mParamsBuilder = createParamsBuilder();
        when(mUtils.findDeviceAdmin(nullable(String.class), nullable(ComponentName.class),
                eq(mContext), eq(TEST_USER_ID))).thenReturn(TEST_MDM_COMPONENT_NAME);
    }

    @Test
    public void testGetSystemAppsToRemove_NewLeave() throws Exception {
        // GIVEN that a new profile is being created and that system apps should not be deleted
        mParamsBuilder.setLeaveAllSystemAppsEnabled(true);
        final NonRequiredAppsLogic logic = createLogic(true);
        // GIVEN that a combination of apps is present
        initializeApps();

        // THEN getSystemAppsToRemove should be empty
        assertTrue(logic.getSystemAppsToRemove(TEST_USER_ID).isEmpty());
    }

    @Test
    public void testGetSystemAppsToRemove_NewDelete() throws Exception {
        // GIVEN that a new profile is being created and that system apps should be deleted
        mParamsBuilder.setLeaveAllSystemAppsEnabled(false);
        final NonRequiredAppsLogic logic = createLogic(true);
        // GIVEN that a combination of apps is present
        initializeApps();

        // THEN getSystemAppsToRemove should return a non-empty list with the only app to removed
        // being the one that is a current system app and non required
        assertEquals(getAppsSet(Arrays.asList(0, 4)),
                logic.getSystemAppsToRemove(TEST_USER_ID));
    }

    @Test
    public void testGetSystemAppsToRemove_deviceAdminComponentIsNotGiven() throws Exception {
        // GIVEN that only device admin package name is given.
        mParamsBuilder.setDeviceAdminComponentName(null);
        mParamsBuilder.setDeviceAdminPackageName(TEST_DPC_PACKAGE_NAME);
        // GIVEN that a new profile is being created and that system apps should be deleted
        mParamsBuilder.setLeaveAllSystemAppsEnabled(false);
        final NonRequiredAppsLogic logic = createLogic(true);
        // GIVEN that a combination of apps is present
        initializeApps();

        // THEN getSystemAppsToRemove should return a non-empty list with the only app to removed
        // being the one that is a current system app and non required
        assertEquals(getAppsSet(Arrays.asList(0, 4)),
                logic.getSystemAppsToRemove(TEST_USER_ID));
    }

    @Test
    public void testGetSystemAppsToRemove_OtaLeave() throws Exception {
        // GIVEN that an OTA occurs and that system apps should not be deleted (indicated by the
        // fact that no snapshot currently exists)
        mParamsBuilder.setLeaveAllSystemAppsEnabled(false);
        final NonRequiredAppsLogic logic = createLogic(false);
        // GIVEN that a combination of apps is present
        initializeApps();
        // GIVEN that no snapshot currently exists
        when(mSnapshot.hasSnapshot(TEST_USER_ID)).thenReturn(false);

        // THEN getSystemAppsToRemove should be empty
        assertTrue(logic.getSystemAppsToRemove(TEST_USER_ID).isEmpty());
    }

    @Test
    public void testGetSystemAppsToRemove_OtaDelete() throws Exception {
        // GIVEN that an OTA occurs and that system apps should be deleted (indicated by the fact
        // that a snapshot currently exists)
        mParamsBuilder.setLeaveAllSystemAppsEnabled(false);
        final NonRequiredAppsLogic logic = createLogic(false);
        // GIVEN that a combination of apps is present
        initializeApps();

        // THEN getSystemAppsToRemove should return a non-empty list with the only app to removed
        // being the one that is a current system app, non required and not in the last
        // snapshot.
        assertEquals(Collections.singleton(APPS.get(4)), logic.getSystemAppsToRemove(TEST_USER_ID));
    }

    @Test
    public void testMaybeTakeSnapshot_NewLeave() {
        // GIVEN that a new profile is being created and that system apps should not be deleted
        mParamsBuilder.setLeaveAllSystemAppsEnabled(true);
        final NonRequiredAppsLogic logic = createLogic(true);

        // WHEN calling maybeTakeSystemAppsSnapshot
        logic.maybeTakeSystemAppsSnapshot(TEST_USER_ID);

        // THEN no snapshot should be taken
        verify(mSnapshot, never()).takeNewSnapshot(anyInt());
    }

    @Test
    public void testMaybeTakeSnapshot_NewDelete() {
        // GIVEN that a new profile is being created and that system apps should be deleted
        mParamsBuilder.setLeaveAllSystemAppsEnabled(false);
        final NonRequiredAppsLogic logic = createLogic(true);

        // WHEN calling maybeTakeSystemAppsSnapshot
        logic.maybeTakeSystemAppsSnapshot(TEST_USER_ID);

        // THEN a snapshot should be taken
        verify(mSnapshot).takeNewSnapshot(TEST_USER_ID);
    }

    @Test
    public void testMaybeTakeSnapshot_OtaLeave() {
        // GIVEN that an OTA occurs and that system apps should not be deleted (indicated by the
        // fact that no snapshot currently exists)
        mParamsBuilder.setLeaveAllSystemAppsEnabled(false);
        final NonRequiredAppsLogic logic = createLogic(false);
        when(mSnapshot.hasSnapshot(TEST_USER_ID)).thenReturn(false);

        // WHEN calling maybeTakeSystemAppsSnapshot
        logic.maybeTakeSystemAppsSnapshot(TEST_USER_ID);

        // THEN no snapshot should be taken
        verify(mSnapshot, never()).takeNewSnapshot(anyInt());
    }

    @Test
    public void testMaybeTakeSnapshot_OtaDelete() {
        // GIVEN that an OTA occurs and that system apps should be deleted (indicated by the fact
        // that a snapshot currently exists)
        mParamsBuilder.setLeaveAllSystemAppsEnabled(false);
        final NonRequiredAppsLogic logic = createLogic(false);
        when(mSnapshot.hasSnapshot(TEST_USER_ID)).thenReturn(true);

        // WHEN calling maybeTakeSystemAppsSnapshot
        logic.maybeTakeSystemAppsSnapshot(TEST_USER_ID);

        // THEN a snapshot should be taken
        verify(mSnapshot).takeNewSnapshot(TEST_USER_ID);
    }

    private void initializeApps() throws Exception {
        setCurrentSystemApps(getAppsSet(SYSTEM_APPS));
        setLastSnapshot(getAppsSet(SNAPSHOT_APPS));
        setNonRequiredApps(getAppsSet(BLACKLIST_APPS));
    }

    private void setCurrentSystemApps(Set<String> set) {
        when(mUtils.getCurrentSystemApps(mIPackageManager, TEST_USER_ID)).thenReturn(set);
    }

    private void setLastSnapshot(Set<String> set) {
        when(mSnapshot.getSnapshot(TEST_USER_ID)).thenReturn(set);
        when(mSnapshot.hasSnapshot(TEST_USER_ID)).thenReturn(true);
    }

    private void setNonRequiredApps(Set<String> set) {
        when(mDevicePolicyManager.getDisallowedSystemApps(TEST_MDM_COMPONENT_NAME, TEST_USER_ID,
                ACTION_PROVISION_MANAGED_DEVICE)).thenReturn(set);
    }

    private Set<String> getAppsSet(List<Integer> ids) {
        return ids.stream().map(APPS::get).collect(Collectors.toSet());
    }

    private NonRequiredAppsLogic createLogic(boolean newProfile) {
        return new NonRequiredAppsLogic(
                mContext,
                mIPackageManager,
                mDevicePolicyManager,
                newProfile,
                mParamsBuilder.build(),
                mSnapshot,
                mUtils);
    }

    private ProvisioningParams.Builder createParamsBuilder() {
        return new ProvisioningParams.Builder()
                .setProvisioningAction(ACTION_PROVISION_MANAGED_DEVICE)
                .setDeviceAdminComponentName(TEST_MDM_COMPONENT_NAME);
    }
}
