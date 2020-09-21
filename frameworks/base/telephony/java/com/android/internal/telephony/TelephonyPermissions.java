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

import static android.content.pm.PackageManager.PERMISSION_GRANTED;

import android.Manifest;
import android.app.AppOpsManager;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Binder;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.telephony.Rlog;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;

import com.android.internal.annotations.VisibleForTesting;

import java.util.function.Supplier;

/** Utility class for Telephony permission enforcement. */
public final class TelephonyPermissions {
    private static final String LOG_TAG = "TelephonyPermissions";

    private static final boolean DBG = false;

    private static final Supplier<ITelephony> TELEPHONY_SUPPLIER = () ->
            ITelephony.Stub.asInterface(ServiceManager.getService(Context.TELEPHONY_SERVICE));

    private TelephonyPermissions() {}

    /**
     * Check whether the caller (or self, if not processing an IPC) can read phone state.
     *
     * <p>This method behaves in one of the following ways:
     * <ul>
     *   <li>return true: if the caller has the READ_PRIVILEGED_PHONE_STATE permission, the
     *       READ_PHONE_STATE runtime permission, or carrier privileges on the given subId.
     *   <li>throw SecurityException: if the caller didn't declare any of these permissions, or, for
     *       apps which support runtime permissions, if the caller does not currently have any of
     *       these permissions.
     *   <li>return false: if the caller lacks all of these permissions and doesn't support runtime
     *       permissions. This implies that the user revoked the ability to read phone state
     *       manually (via AppOps). In this case we can't throw as it would break app compatibility,
     *       so we return false to indicate that the calling function should return dummy data.
     * </ul>
     *
     * <p>Note: for simplicity, this method always returns false for callers using legacy
     * permissions and who have had READ_PHONE_STATE revoked, even if they are carrier-privileged.
     * Such apps should migrate to runtime permissions or stop requiring READ_PHONE_STATE on P+
     * devices.
     *
     * @param subId the subId of the relevant subscription; used to check carrier privileges. May be
     *              {@link SubscriptionManager#INVALID_SUBSCRIPTION_ID} to skip this check for cases
     *              where it isn't relevant (hidden APIs, or APIs which are otherwise okay to leave
     *              inaccesible to carrier-privileged apps).
     */
    public static boolean checkCallingOrSelfReadPhoneState(
            Context context, int subId, String callingPackage, String message) {
        return checkReadPhoneState(context, subId, Binder.getCallingPid(), Binder.getCallingUid(),
                callingPackage, message);
    }

    /**
     * Check whether the app with the given pid/uid can read phone state.
     *
     * <p>This method behaves in one of the following ways:
     * <ul>
     *   <li>return true: if the caller has the READ_PRIVILEGED_PHONE_STATE permission, the
     *       READ_PHONE_STATE runtime permission, or carrier privileges on the given subId.
     *   <li>throw SecurityException: if the caller didn't declare any of these permissions, or, for
     *       apps which support runtime permissions, if the caller does not currently have any of
     *       these permissions.
     *   <li>return false: if the caller lacks all of these permissions and doesn't support runtime
     *       permissions. This implies that the user revoked the ability to read phone state
     *       manually (via AppOps). In this case we can't throw as it would break app compatibility,
     *       so we return false to indicate that the calling function should return dummy data.
     * </ul>
     *
     * <p>Note: for simplicity, this method always returns false for callers using legacy
     * permissions and who have had READ_PHONE_STATE revoked, even if they are carrier-privileged.
     * Such apps should migrate to runtime permissions or stop requiring READ_PHONE_STATE on P+
     * devices.
     */
    public static boolean checkReadPhoneState(
            Context context, int subId, int pid, int uid, String callingPackage, String message) {
        return checkReadPhoneState(
                context, TELEPHONY_SUPPLIER, subId, pid, uid, callingPackage, message);
    }

    @VisibleForTesting
    public static boolean checkReadPhoneState(
            Context context, Supplier<ITelephony> telephonySupplier, int subId, int pid, int uid,
            String callingPackage, String message) {
        try {
            context.enforcePermission(
                    android.Manifest.permission.READ_PRIVILEGED_PHONE_STATE, pid, uid, message);

            // SKIP checking for run-time permission since caller has PRIVILEGED permission
            return true;
        } catch (SecurityException privilegedPhoneStateException) {
            try {
                context.enforcePermission(
                        android.Manifest.permission.READ_PHONE_STATE, pid, uid, message);
            } catch (SecurityException phoneStateException) {
                // If we don't have the runtime permission, but do have carrier privileges, that
                // suffices for reading phone state.
                if (SubscriptionManager.isValidSubscriptionId(subId)) {
                    enforceCarrierPrivilege(telephonySupplier, subId, uid, message);
                    return true;
                }
                throw phoneStateException;
            }
        }

        // We have READ_PHONE_STATE permission, so return true as long as the AppOps bit hasn't been
        // revoked.
        AppOpsManager appOps = (AppOpsManager) context.getSystemService(Context.APP_OPS_SERVICE);
        return appOps.noteOp(AppOpsManager.OP_READ_PHONE_STATE, uid, callingPackage) ==
                AppOpsManager.MODE_ALLOWED;
    }

    /**
     * Check whether the app with the given pid/uid can read the call log.
     * @return {@code true} if the specified app has the read call log permission and AppOpp granted
     *      to it, {@code false} otherwise.
     */
    public static boolean checkReadCallLog(
            Context context, int subId, int pid, int uid, String callingPackage) {
        return checkReadCallLog(
                context, TELEPHONY_SUPPLIER, subId, pid, uid, callingPackage);
    }

    @VisibleForTesting
    public static boolean checkReadCallLog(
            Context context, Supplier<ITelephony> telephonySupplier, int subId, int pid, int uid,
            String callingPackage) {

        if (context.checkPermission(Manifest.permission.READ_CALL_LOG, pid, uid)
                != PERMISSION_GRANTED) {
            // If we don't have the runtime permission, but do have carrier privileges, that
            // suffices for being able to see the call phone numbers.
            if (SubscriptionManager.isValidSubscriptionId(subId)) {
                enforceCarrierPrivilege(telephonySupplier, subId, uid, "readCallLog");
                return true;
            }
            return false;
        }

        // We have READ_CALL_LOG permission, so return true as long as the AppOps bit hasn't been
        // revoked.
        AppOpsManager appOps = (AppOpsManager) context.getSystemService(Context.APP_OPS_SERVICE);
        return appOps.noteOp(AppOpsManager.OP_READ_CALL_LOG, uid, callingPackage) ==
                AppOpsManager.MODE_ALLOWED;
    }

    /**
     * Returns whether the caller can read phone numbers.
     *
     * <p>Besides apps with the ability to read phone state per {@link #checkReadPhoneState}, the
     * default SMS app and apps with READ_SMS or READ_PHONE_NUMBERS can also read phone numbers.
     */
    public static boolean checkCallingOrSelfReadPhoneNumber(
            Context context, int subId, String callingPackage, String message) {
        return checkReadPhoneNumber(
                context, TELEPHONY_SUPPLIER, subId, Binder.getCallingPid(), Binder.getCallingUid(),
                callingPackage, message);
    }

    @VisibleForTesting
    public static boolean checkReadPhoneNumber(
            Context context, Supplier<ITelephony> telephonySupplier, int subId, int pid, int uid,
            String callingPackage, String message) {
        // Default SMS app can always read it.
        AppOpsManager appOps = (AppOpsManager) context.getSystemService(Context.APP_OPS_SERVICE);
        if (appOps.noteOp(AppOpsManager.OP_WRITE_SMS, uid, callingPackage) ==
                AppOpsManager.MODE_ALLOWED) {
            return true;
        }

        // NOTE(b/73308711): If an app has one of the following AppOps bits explicitly revoked, they
        // will be denied access, even if they have another permission and AppOps bit if needed.

        // First, check if we can read the phone state.
        try {
            return checkReadPhoneState(
                    context, telephonySupplier, subId, pid, uid, callingPackage, message);
        } catch (SecurityException readPhoneStateSecurityException) {
        }
        // Can be read with READ_SMS too.
        try {
            context.enforcePermission(android.Manifest.permission.READ_SMS, pid, uid, message);
            int opCode = AppOpsManager.permissionToOpCode(android.Manifest.permission.READ_SMS);
            if (opCode != AppOpsManager.OP_NONE) {
                return appOps.noteOp(opCode, uid, callingPackage) == AppOpsManager.MODE_ALLOWED;
            } else {
                return true;
            }
        } catch (SecurityException readSmsSecurityException) {
        }
        // Can be read with READ_PHONE_NUMBERS too.
        try {
            context.enforcePermission(android.Manifest.permission.READ_PHONE_NUMBERS, pid, uid,
                    message);
            int opCode = AppOpsManager.permissionToOpCode(
                    android.Manifest.permission.READ_PHONE_NUMBERS);
            if (opCode != AppOpsManager.OP_NONE) {
                return appOps.noteOp(opCode, uid, callingPackage) == AppOpsManager.MODE_ALLOWED;
            } else {
                return true;
            }
        } catch (SecurityException readPhoneNumberSecurityException) {
        }

        throw new SecurityException(message + ": Neither user " + uid +
                " nor current process has " + android.Manifest.permission.READ_PHONE_STATE +
                ", " + android.Manifest.permission.READ_SMS + ", or " +
                android.Manifest.permission.READ_PHONE_NUMBERS);
    }

    /**
     * Ensure the caller (or self, if not processing an IPC) has MODIFY_PHONE_STATE (and is thus a
     * privileged app) or carrier privileges.
     *
     * @throws SecurityException if the caller does not have the required permission/privileges
     */
    public static void enforceCallingOrSelfModifyPermissionOrCarrierPrivilege(
            Context context, int subId, String message) {
        if (context.checkCallingOrSelfPermission(android.Manifest.permission.MODIFY_PHONE_STATE) ==
                PERMISSION_GRANTED) {
            return;
        }

        if (DBG) Rlog.d(LOG_TAG, "No modify permission, check carrier privilege next.");
        enforceCallingOrSelfCarrierPrivilege(subId, message);
    }

    /**
     * Make sure the caller (or self, if not processing an IPC) has carrier privileges.
     *
     * @throws SecurityException if the caller does not have the required privileges
     */
    public static void enforceCallingOrSelfCarrierPrivilege(int subId, String message) {
        // NOTE: It's critical that we explicitly pass the calling UID here rather than call
        // TelephonyManager#hasCarrierPrivileges directly, as the latter only works when called from
        // the phone process. When called from another process, it will check whether that process
        // has carrier privileges instead.
        enforceCarrierPrivilege(subId, Binder.getCallingUid(), message);
    }

    private static void enforceCarrierPrivilege(int subId, int uid, String message) {
        enforceCarrierPrivilege(TELEPHONY_SUPPLIER, subId, uid, message);
    }

    private static void enforceCarrierPrivilege(
            Supplier<ITelephony> telephonySupplier, int subId, int uid, String message) {
        if (getCarrierPrivilegeStatus(telephonySupplier, subId, uid) !=
                TelephonyManager.CARRIER_PRIVILEGE_STATUS_HAS_ACCESS) {
            if (DBG) Rlog.e(LOG_TAG, "No Carrier Privilege.");
            throw new SecurityException(message);
        }
    }

    private static int getCarrierPrivilegeStatus(
            Supplier<ITelephony> telephonySupplier, int subId, int uid) {
        ITelephony telephony = telephonySupplier.get();
        try {
            if (telephony != null) {
                return telephony.getCarrierPrivilegeStatusForUid(subId, uid);
            }
        } catch (RemoteException e) {
            // Fallback below.
        }
        Rlog.e(LOG_TAG, "Phone process is down, cannot check carrier privileges");
        return TelephonyManager.CARRIER_PRIVILEGE_STATUS_NO_ACCESS;
    }
}
