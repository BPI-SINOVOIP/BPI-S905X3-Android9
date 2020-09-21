/*
 * Copyright (C) 2010 The Android Open Source Project
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

package com.android.cts.permissiondeclareapp;

import android.content.ClipboardManager;
import android.content.ContentProvider;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.content.UriPermission;
import android.database.Cursor;
import android.net.Uri;
import android.os.Binder;
import android.os.Bundle;

import java.util.List;

public class UtilsProvider extends ContentProvider {
    public static final Uri URI = Uri.parse("content://com.android.cts.permissiondeclareapp/");

    public static final String ACTION_GRANT_URI = "grantUri";
    public static final String ACTION_REVOKE_URI = "revokeUri";
    public static final String ACTION_START_ACTIVITY = "startActivity";
    public static final String ACTION_START_SERVICE = "startService";
    public static final String ACTION_VERIFY_OUTGOING_PERSISTED = "verifyOutgoingPersisted";
    public static final String ACTION_SET_PRIMARY_CLIP = "setPrimaryClip";
    public static final String ACTION_CLEAR_PRIMARY_CLIP = "clearPrimaryClip";
    public static final String ACTION_SET_INSTALLER_PACKAGE_NAME = "setInstallerPackageName";

    public static final String EXTRA_PACKAGE_NAME = "packageName";
    public static final String EXTRA_INSTALLER_PACKAGE_NAME = "installerPackageName";
    public static final String EXTRA_INTENT = Intent.EXTRA_INTENT;
    public static final String EXTRA_URI = "uri";
    public static final String EXTRA_MODE = "mode";

    @Override
    public Bundle call(String method, String arg, Bundle extras) {
        final Context context = getContext();
        final Intent intent = extras.getParcelable(Intent.EXTRA_INTENT);
        final String action = intent.getAction();

        final long token = Binder.clearCallingIdentity();
        try {
            if (ACTION_GRANT_URI.equals(action)) {
                final Uri uri = intent.getParcelableExtra(EXTRA_URI);
                context.grantUriPermission(intent.getStringExtra(EXTRA_PACKAGE_NAME), uri,
                        intent.getIntExtra(EXTRA_MODE, 0));

            } else if (ACTION_REVOKE_URI.equals(action)) {
                final Uri uri = intent.getParcelableExtra(EXTRA_URI);
                context.revokeUriPermission(uri, intent.getIntExtra(EXTRA_MODE, 0));

            } else if (ACTION_START_ACTIVITY.equals(action)) {
                final Intent newIntent = intent.getParcelableExtra(EXTRA_INTENT);
                newIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                context.startActivity(newIntent);

            } else if (ACTION_START_SERVICE.equals(action)) {
                final Intent newIntent = intent.getParcelableExtra(EXTRA_INTENT);
                context.startService(newIntent);

            } else if (ACTION_VERIFY_OUTGOING_PERSISTED.equals(action)) {
                verifyOutgoingPersisted(context, intent);

            } else if (ACTION_SET_PRIMARY_CLIP.equals(action)) {
                context.getSystemService(ClipboardManager.class)
                        .setPrimaryClip(intent.getClipData());

            } else if (ACTION_CLEAR_PRIMARY_CLIP.equals(action)) {
                context.getSystemService(ClipboardManager.class).clearPrimaryClip();

            } else if (ACTION_SET_INSTALLER_PACKAGE_NAME.equals(action)) {
                context.getPackageManager().setInstallerPackageName(
                        intent.getStringExtra(EXTRA_PACKAGE_NAME),
                        intent.getStringExtra(EXTRA_INSTALLER_PACKAGE_NAME));
            }
        } finally {
            Binder.restoreCallingIdentity(token);
        }
        return null;
    }

    private void verifyOutgoingPersisted(Context context, Intent intent) {
        final Uri uri = intent.getParcelableExtra(EXTRA_URI);
        final List<UriPermission> perms = context.getContentResolver()
                .getOutgoingPersistedUriPermissions();
        if (uri != null) {
            // Should have a single persisted perm
            if (perms.size() != 1) {
                throw new SecurityException("Missing grant");
            }
            final UriPermission perm = perms.get(0);
            if (!perm.getUri().equals(uri)) {
                throw new SecurityException(
                        "Expected " + uri + " but found " + perm.getUri());
            }
        } else {
            // Should have zero persisted perms
            if (perms.size() != 0) {
                throw new SecurityException("Unexpected grant");
            }
        }
    }

    @Override
    public boolean onCreate() {
        return true;
    }

    @Override
    public Cursor query(Uri uri, String[] projection, String selection, String[] selectionArgs,
            String sortOrder) {
        throw new UnsupportedOperationException();
    }

    @Override
    public String getType(Uri uri) {
        throw new UnsupportedOperationException();
    }

    @Override
    public Uri insert(Uri uri, ContentValues values) {
        throw new UnsupportedOperationException();
    }

    @Override
    public int delete(Uri uri, String selection, String[] selectionArgs) {
        throw new UnsupportedOperationException();
    }

    @Override
    public int update(Uri uri, ContentValues values, String selection, String[] selectionArgs) {
        throw new UnsupportedOperationException();
    }
}
