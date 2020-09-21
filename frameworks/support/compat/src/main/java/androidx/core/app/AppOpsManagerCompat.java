/*
 * Copyright (C) 2015 The Android Open Source Project
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

package androidx.core.app;

import static android.os.Build.VERSION.SDK_INT;

import android.app.AppOpsManager;
import android.content.Context;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

/**
 * Helper for accessing features in {@link android.app.AppOpsManager}.
 */
public final class AppOpsManagerCompat {

    /**
     * Result from {@link #noteOp}: the given caller is allowed to
     * perform the given operation.
     */
    public static final int MODE_ALLOWED = AppOpsManager.MODE_ALLOWED;

    /**
     * Result from {@link #noteOp}: the given caller is not allowed to perform
     * the given operation, and this attempt should <em>silently fail</em> (it
     * should not cause the app to crash).
     */
    public static final int MODE_IGNORED = AppOpsManager.MODE_IGNORED;

    /**
     * Result from {@link #noteOpNoThrow}: the
     * given caller is not allowed to perform the given operation, and this attempt should
     * cause it to have a fatal error, typically a {@link SecurityException}.
     */
    public static final int MODE_ERRORED = AppOpsManager.MODE_ERRORED;

    /**
     * Result from {@link #noteOp}: the given caller should use its default
     * security check.  This mode is not normally used; it should only be used
     * with appop permissions, and callers must explicitly check for it and
     * deal with it.
     */
    public static final int MODE_DEFAULT = AppOpsManager.MODE_DEFAULT;

    private AppOpsManagerCompat() {}

    /**
     * Gets the app op name associated with a given permission.
     * <p>
     * <strong>Compatibility</strong>
     * <ul>
     * <li>On API 22 and lower, this method always returns {@code null}
     * </ul>
     *
     * @param permission The permission.
     * @return The app op associated with the permission or null.
     */
    @Nullable
    public static String permissionToOp(@NonNull String permission) {
        if (SDK_INT >= 23) {
            return AppOpsManager.permissionToOp(permission);
        } else {
            return null;
        }
    }

    /**
     * Make note of an application performing an operation.  Note that you must pass
     * in both the uid and name of the application to be checked; this function will verify
     * that these two match, and if not, return {@link #MODE_IGNORED}.  If this call
     * succeeds, the last execution time of the operation for this app will be updated to
     * the current time.
     * <p>
     * <strong>Compatibility</strong>
     * <ul>
     * <li>On API 18 and lower, this method always returns {@link #MODE_IGNORED}
     * </ul>
     * @param context Your context.
     * @param op The operation to note.  One of the OPSTR_* constants.
     * @param uid The user id of the application attempting to perform the operation.
     * @param packageName The name of the application attempting to perform the operation.
     * @return Returns {@link #MODE_ALLOWED} if the operation is allowed, or
     * {@link #MODE_IGNORED} if it is not allowed and should be silently ignored (without
     * causing the app to crash).
     * @throws SecurityException If the app has been configured to crash on this op.
     */
    public static int noteOp(@NonNull Context context, @NonNull String op, int uid,
            @NonNull String packageName) {
        if (SDK_INT >= 19) {
            AppOpsManager appOpsManager =
                    (AppOpsManager) context.getSystemService(Context.APP_OPS_SERVICE);
            return appOpsManager.noteOp(op, uid, packageName);
        } else {
            return MODE_IGNORED;
        }
    }

    /**
     * Like {@link #noteOp} but instead of throwing a {@link SecurityException} it
     * returns {@link #MODE_ERRORED}.
     * <p>
     * <strong>Compatibility</strong>
     * <ul>
     * <li>On API 18 and lower, this method always returns {@link #MODE_IGNORED}
     * </ul>
     */
    public static int noteOpNoThrow(@NonNull Context context, @NonNull String op, int uid,
            @NonNull String packageName) {
        if (SDK_INT >= 19) {
            AppOpsManager appOpsManager =
                    (AppOpsManager) context.getSystemService(Context.APP_OPS_SERVICE);
            return appOpsManager.noteOpNoThrow(op, uid, packageName);
        } else {
            return MODE_IGNORED;
        }
    }

    /**
     * Make note of an application performing an operation on behalf of another
     * application when handling an IPC. Note that you must pass the package name
     * of the application that is being proxied while its UID will be inferred from
     * the IPC state; this function will verify that the calling uid and proxied
     * package name match, and if not, return {@link #MODE_IGNORED}. If this call
     * succeeds, the last execution time of the operation for the proxied app and
     * your app will be updated to the current time.
     * <p>
     * <strong>Compatibility</strong>
     * <ul>
     * <li>On API 22 and lower, this method always returns {@link #MODE_IGNORED}
     * </ul>
     * @param context Your context.
     * @param op The operation to note.  One of the OPSTR_* constants.
     * @param proxiedPackageName The name of the application calling into the proxy application.
     * @return Returns {@link #MODE_ALLOWED} if the operation is allowed, or
     * {@link #MODE_IGNORED} if it is not allowed and should be silently ignored (without
     * causing the app to crash).
     * @throws SecurityException If the app has been configured to crash on this op.
     */
    public static int noteProxyOp(@NonNull Context context, @NonNull String op,
            @NonNull String proxiedPackageName) {
        if (SDK_INT >= 23) {
            AppOpsManager appOpsManager = context.getSystemService(AppOpsManager.class);
            return appOpsManager.noteProxyOp(op, proxiedPackageName);
        } else {
            return MODE_IGNORED;
        }
    }

    /**
     * Like {@link #noteProxyOp(Context, String, String)} but instead
     * of throwing a {@link SecurityException} it returns {@link #MODE_ERRORED}.
     * <p>
     * <strong>Compatibility</strong>
     * <ul>
     * <li>On API 22 and lower, this method always returns {@link #MODE_IGNORED}
     * </ul>
     */
    public static int noteProxyOpNoThrow(@NonNull Context context, @NonNull String op,
            @NonNull String proxiedPackageName) {
        if (SDK_INT >= 23) {
            AppOpsManager appOpsManager = context.getSystemService(AppOpsManager.class);
            return appOpsManager.noteProxyOpNoThrow(op, proxiedPackageName);
        } else {
            return MODE_IGNORED;
        }
    }
}
