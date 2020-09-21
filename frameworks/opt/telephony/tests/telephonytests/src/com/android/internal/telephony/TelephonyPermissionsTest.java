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
package com.android.internal.telephony;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.doThrow;
import static org.mockito.Mockito.when;

import android.app.AppOpsManager;
import android.content.Context;
import android.telephony.TelephonyManager;
import android.test.suitebuilder.annotation.SmallTest;

import org.junit.Before;
import org.junit.Test;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

@SmallTest
public class TelephonyPermissionsTest {

    private static final int SUB_ID = 55555;
    private static final int PID = 12345;
    private static final int UID = 54321;
    private static final String PACKAGE = "com.example";
    private static final String MSG = "message";

    @Mock
    private Context mMockContext;
    @Mock
    private AppOpsManager mMockAppOps;
    @Mock
    private ITelephony mMockTelephony;

    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);
        when(mMockContext.getSystemService(Context.APP_OPS_SERVICE)).thenReturn(mMockAppOps);

        // By default, assume we have no permissions or app-ops bits.
        doThrow(new SecurityException()).when(mMockContext)
                .enforcePermission(anyString(), eq(PID), eq(UID), eq(MSG));
        doThrow(new SecurityException()).when(mMockContext)
                .enforcePermission(anyString(), eq(PID), eq(UID), eq(MSG));
        when(mMockAppOps.noteOp(anyInt(), eq(UID), eq(PACKAGE)))
                .thenReturn(AppOpsManager.MODE_ERRORED);
        when(mMockTelephony.getCarrierPrivilegeStatusForUid(eq(SUB_ID), eq(UID)))
                .thenReturn(TelephonyManager.CARRIER_PRIVILEGE_STATUS_NO_ACCESS);
    }

    @Test
    public void testCheckReadPhoneState_noPermissions() {
        try {
            TelephonyPermissions.checkReadPhoneState(
                    mMockContext, () -> mMockTelephony, SUB_ID, PID, UID, PACKAGE, MSG);
            fail("Should have thrown SecurityException");
        } catch (SecurityException e) {
            // expected
        }
    }

    @Test
    public void testCheckReadPhoneState_hasPrivilegedPermission() {
        doNothing().when(mMockContext).enforcePermission(
                android.Manifest.permission.READ_PRIVILEGED_PHONE_STATE, PID, UID, MSG);
        assertTrue(TelephonyPermissions.checkReadPhoneState(
                mMockContext, () -> mMockTelephony, SUB_ID, PID, UID, PACKAGE, MSG));
    }

    @Test
    public void testCheckReadPhoneState_hasPermissionAndAppOp() {
        doNothing().when(mMockContext).enforcePermission(
                android.Manifest.permission.READ_PHONE_STATE, PID, UID, MSG);
        when(mMockAppOps.noteOp(AppOpsManager.OP_READ_PHONE_STATE, UID, PACKAGE))
                .thenReturn(AppOpsManager.MODE_ALLOWED);
        assertTrue(TelephonyPermissions.checkReadPhoneState(
                mMockContext, () -> mMockTelephony, SUB_ID, PID, UID, PACKAGE, MSG));
    }

    @Test
    public void testCheckReadPhoneState_hasPermissionWithoutAppOp() {
        doNothing().when(mMockContext).enforcePermission(
                android.Manifest.permission.READ_PHONE_STATE, PID, UID, MSG);
        assertFalse(TelephonyPermissions.checkReadPhoneState(
                mMockContext, () -> mMockTelephony, SUB_ID, PID, UID, PACKAGE, MSG));
    }

    @Test
    public void testCheckReadPhoneState_hasCarrierPrivileges() throws Exception {
        when(mMockTelephony.getCarrierPrivilegeStatusForUid(eq(SUB_ID), eq(UID)))
                .thenReturn(TelephonyManager.CARRIER_PRIVILEGE_STATUS_HAS_ACCESS);
        assertTrue(TelephonyPermissions.checkReadPhoneState(
                mMockContext, () -> mMockTelephony, SUB_ID, PID, UID, PACKAGE, MSG));
    }

    @Test
    public void testCheckReadPhoneNumber_noPermissions() {
        try {
            TelephonyPermissions.checkReadPhoneNumber(
                    mMockContext, () -> mMockTelephony, SUB_ID, PID, UID, PACKAGE, MSG);
            fail("Should have thrown SecurityException");
        } catch (SecurityException e) {
            // expected
        }
    }

    @Test
    public void testCheckReadPhoneNumber_defaultSmsApp() {
        when(mMockAppOps.noteOp(AppOpsManager.OP_WRITE_SMS, UID, PACKAGE))
                .thenReturn(AppOpsManager.MODE_ALLOWED);
        assertTrue(TelephonyPermissions.checkReadPhoneNumber(
                mMockContext, () -> mMockTelephony, SUB_ID, PID, UID, PACKAGE, MSG));
    }

    @Test
    public void testCheckReadPhoneNumber_hasPrivilegedPhoneStatePermission() {
        doNothing().when(mMockContext).enforcePermission(
                android.Manifest.permission.READ_PRIVILEGED_PHONE_STATE, PID, UID, MSG);
        assertTrue(TelephonyPermissions.checkReadPhoneNumber(
                mMockContext, () -> mMockTelephony, SUB_ID, PID, UID, PACKAGE, MSG));
    }

    @Test
    public void testCheckReadPhoneNumber_hasReadSms() {
        doNothing().when(mMockContext).enforcePermission(
                android.Manifest.permission.READ_SMS, PID, UID, MSG);
        when(mMockAppOps.noteOp(AppOpsManager.OP_READ_SMS, UID, PACKAGE))
                .thenReturn(AppOpsManager.MODE_ALLOWED);
        assertTrue(TelephonyPermissions.checkReadPhoneNumber(
                mMockContext, () -> mMockTelephony, SUB_ID, PID, UID, PACKAGE, MSG));
    }

    @Test
    public void testCheckReadPhoneNumber_hasReadPhoneNumbers() {
        doNothing().when(mMockContext).enforcePermission(
                android.Manifest.permission.READ_PHONE_NUMBERS, PID, UID, MSG);
        when(mMockAppOps.noteOp(AppOpsManager.OP_READ_PHONE_NUMBERS, UID, PACKAGE))
                .thenReturn(AppOpsManager.MODE_ALLOWED);
        assertTrue(TelephonyPermissions.checkReadPhoneNumber(
                mMockContext, () -> mMockTelephony, SUB_ID, PID, UID, PACKAGE, MSG));
    }
}
