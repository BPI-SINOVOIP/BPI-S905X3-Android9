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
 * limitations under the License
 */

package com.android.traceur;

import android.accounts.Account;
import android.accounts.AccountManager;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.support.v4.content.FileProvider;
import android.content.Intent;
import android.net.Uri;
import android.os.SystemProperties;
import android.util.Patterns;

import java.io.File;

/**
 * Sends bugreport-y files, adapted from fw/base/packages/Shell's BugreportReceiver.
 */
public class FileSender {

    private static final String AUTHORITY = "com.android.traceur.files";

    public static void postNotification(Context context, File file) {
        // Files are kept on private storage, so turn into Uris that we can
        // grant temporary permissions for.
        final Uri traceUri = getUriForFile(context, file);

        Intent sendIntent = buildSendIntent(context, traceUri);
        sendIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);

        final Notification.Builder builder =
            new Notification.Builder(context, Receiver.NOTIFICATION_CHANNEL)
                .setSmallIcon(R.drawable.stat_sys_adb)
                .setContentTitle(context.getString(R.string.trace_saved))
                .setTicker(context.getString(R.string.trace_saved))
                .setContentText(context.getString(R.string.tap_to_share))
                .setContentIntent(PendingIntent.getActivity(
                        context, traceUri.hashCode(), sendIntent, PendingIntent.FLAG_ONE_SHOT
                                | PendingIntent.FLAG_CANCEL_CURRENT))
                .setAutoCancel(true)
                .setLocalOnly(true)
                .setColor(context.getColor(
                        com.android.internal.R.color.system_notification_accent_color));

        NotificationManager.from(context).notify(file.getName(), 0, builder.build());
    }

    public static void send(Context context, File file) {
        // Files are kept on private storage, so turn into Uris that we can
        // grant temporary permissions for.
        final Uri traceUri = getUriForFile(context, file);

        Intent sendIntent = buildSendIntent(context, traceUri);
        sendIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);

        context.startActivity(sendIntent);
    }

    private static Uri getUriForFile(Context context, File file) {
        return FileProvider.getUriForFile(context, AUTHORITY, file);
    }

    /**
     * Build {@link Intent} that can be used to share the given bugreport.
     */
    private static Intent buildSendIntent(Context context, Uri traceUri) {
        final Intent intent = new Intent(Intent.ACTION_SEND);
        intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
        intent.addCategory(Intent.CATEGORY_DEFAULT);
        intent.setType("application/vnd.android.systrace");

        intent.putExtra(Intent.EXTRA_SUBJECT, traceUri.getLastPathSegment());
        intent.putExtra(Intent.EXTRA_TEXT, SystemProperties.get("ro.build.description"));
        intent.putExtra(Intent.EXTRA_STREAM, traceUri);

        final Account sendToAccount = findSendToAccount(context);
        if (sendToAccount != null) {
            intent.putExtra(Intent.EXTRA_EMAIL, new String[] { sendToAccount.name });
        }

        return intent;
    }

    /**
     * Find the best matching {@link Account} based on build properties.
     */
    private static Account findSendToAccount(Context context) {
        final AccountManager am = (AccountManager) context.getSystemService(
                Context.ACCOUNT_SERVICE);

        String preferredDomain = SystemProperties.get("sendbug.preferred.domain");
        if (!preferredDomain.startsWith("@")) {
            preferredDomain = "@" + preferredDomain;
        }

        final Account[] accounts = am.getAccounts();
        Account foundAccount = null;
        for (Account account : accounts) {
            if (Patterns.EMAIL_ADDRESS.matcher(account.name).matches()) {
                if (!preferredDomain.isEmpty()) {
                    // if we have a preferred domain and it matches, return; otherwise keep
                    // looking
                    if (account.name.endsWith(preferredDomain)) {
                        return account;
                    } else {
                        foundAccount = account;
                    }
                    // if we don't have a preferred domain, just return since it looks like
                    // an email address
                } else {
                    return account;
                }
            }
        }
        return foundAccount;
    }
}
