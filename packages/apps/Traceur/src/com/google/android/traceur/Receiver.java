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

import com.google.android.collect.Sets;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.database.ContentObserver;
import android.net.Uri;
import android.os.Build;
import android.os.Handler;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.preference.PreferenceManager;
import android.provider.Settings;
import android.text.TextUtils;
import android.util.Log;

import com.android.internal.statusbar.IStatusBarService;

import java.util.Set;
import java.util.TreeMap;

public class Receiver extends BroadcastReceiver {

    public static final String STOP_ACTION = "com.android.traceur.STOP";
    public static final String OPEN_ACTION = "com.android.traceur.OPEN";

    public static final String NOTIFICATION_CHANNEL = "system-tracing";

    private static final Set<String> ATRACE_TAGS = Sets.newArraySet(
            "am", "binder_driver", "camera", "dalvik", "freq", "gfx", "hal",
            "idle", "input", "irq", "res", "sched", "sync", "view", "wm",
            "workq");

    /* The user list doesn't include workq, irq, or sync, because the user builds don't have
     * permissions for them. */
    private static final Set<String> ATRACE_TAGS_USER = Sets.newArraySet(
            "am", "binder_driver", "camera", "dalvik", "freq", "gfx", "hal",
            "idle", "input", "res", "sched", "view", "wm");

    private static final String TAG = "Traceur";

    private static ContentObserver mDeveloperOptionsObserver;

    @Override
    public void onReceive(Context context, Intent intent) {
        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(context);

        if (Intent.ACTION_BOOT_COMPLETED.equals(intent.getAction())) {
            createNotificationChannel(context);
            updateDeveloperOptionsWatcher(context,
                prefs.getBoolean(context.getString(R.string.pref_key_quick_setting), false));
            updateTracing(context);
        } else if (STOP_ACTION.equals(intent.getAction())) {
            prefs.edit().putBoolean(context.getString(R.string.pref_key_tracing_on), false).apply();
            updateTracing(context);
        } else if (OPEN_ACTION.equals(intent.getAction())) {
            context.sendBroadcast(new Intent(Intent.ACTION_CLOSE_SYSTEM_DIALOGS));
            context.startActivity(new Intent(context, MainActivity.class)
                    .setFlags(Intent.FLAG_ACTIVITY_NEW_TASK));
        }
    }

    /*
     * Updates the current tracing state based on the current state of preferences.
     */
    public static void updateTracing(Context context) {
        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(context);
        boolean prefsTracingOn =
                prefs.getBoolean(context.getString(R.string.pref_key_tracing_on), false);

        if (prefsTracingOn != AtraceUtils.isTracingOn()) {
            if (prefsTracingOn) {
                // Show notification if the tags in preferences are not all actually available.
                String activeAvailableTags = getActiveTags(context, prefs, true);
                String activeTags = getActiveTags(context, prefs, false);
                if (!TextUtils.equals(activeAvailableTags, activeTags)) {
                    postCategoryNotification(context, prefs);
                }

                int bufferSize = Integer.parseInt(
                    prefs.getString(context.getString(R.string.pref_key_buffer_size),
                        context.getString(R.string.default_buffer_size)));

                boolean appTracing = prefs.getBoolean(context.getString(R.string.pref_key_apps), true);

                AtraceService.startTracing(context, activeAvailableTags, bufferSize, appTracing);
            } else {
                AtraceService.stopTracing(context);
            }
        }

        // Update the main UI and the QS tile.
        context.sendBroadcast(new Intent(MainFragment.ACTION_REFRESH_TAGS));
        QsService.updateTile();
    }

    /*
     * Updates the current Quick Settings tile state based on the current state
     * of preferences.
     */
    public static void updateQuickSettings(Context context) {
        boolean quickSettingsEnabled =
            PreferenceManager.getDefaultSharedPreferences(context)
              .getBoolean(context.getString(R.string.pref_key_quick_setting), false);

        ComponentName name = new ComponentName(context, QsService.class);
        context.getPackageManager().setComponentEnabledSetting(name,
            quickSettingsEnabled
                ? PackageManager.COMPONENT_ENABLED_STATE_ENABLED
                : PackageManager.COMPONENT_ENABLED_STATE_DISABLED,
            PackageManager.DONT_KILL_APP);

        IStatusBarService statusBarService = IStatusBarService.Stub.asInterface(
            ServiceManager.checkService(Context.STATUS_BAR_SERVICE));

        try {
            if (statusBarService != null) {
                if (quickSettingsEnabled) {
                    statusBarService.addTile(name);
                } else {
                    statusBarService.remTile(name);
                }
            }
        } catch (RemoteException e) {
            Log.e(TAG, "Failed to modify QS tile for Traceur.", e);
        }

        QsService.updateTile();

        updateDeveloperOptionsWatcher(context, quickSettingsEnabled);
    }

    /*
     * When Developer Options are turned off, reset the Show Quick Settings Tile
     * preference to false to hide the tile. The user will need to re-enable the
     * preference if they decide to turn Developer Options back on again.
     */
    private static void updateDeveloperOptionsWatcher(Context context,
            boolean quickSettingsEnabled) {

        Uri settingUri = Settings.Global.getUriFor(
            Settings.Global.DEVELOPMENT_SETTINGS_ENABLED);

        if (quickSettingsEnabled) {
            mDeveloperOptionsObserver =
                new ContentObserver(new Handler()) {
                    @Override
                    public void onChange(boolean selfChange) {
                        super.onChange(selfChange);

                        boolean developerOptionsEnabled = (1 ==
                            Settings.Global.getInt(context.getContentResolver(),
                                Settings.Global.DEVELOPMENT_SETTINGS_ENABLED , 0));

                        if (!developerOptionsEnabled) {
                            SharedPreferences prefs =
                                PreferenceManager.getDefaultSharedPreferences(context);
                            prefs.edit().putBoolean(
                                context.getString(R.string.pref_key_quick_setting), false)
                                .apply();
                            updateQuickSettings(context);
                        }
                    }
                };

            context.getContentResolver().registerContentObserver(settingUri,
                false, mDeveloperOptionsObserver);

        } else if (mDeveloperOptionsObserver != null) {
            context.getContentResolver().unregisterContentObserver(
                mDeveloperOptionsObserver);
            mDeveloperOptionsObserver = null;
        }
    }

    private static void postCategoryNotification(Context context, SharedPreferences prefs) {
        Intent sendIntent = new Intent(context, MainActivity.class);

        String title = context.getString(R.string.tracing_categories_unavailable);
        String msg = getActiveUnavailableTags(context, prefs);
        final Notification.Builder builder =
            new Notification.Builder(context, NOTIFICATION_CHANNEL)
                .setSmallIcon(R.drawable.stat_sys_adb)
                .setContentTitle(title)
                .setTicker(title)
                .setContentText(msg)
                .setContentIntent(PendingIntent.getActivity(
                        context, 0, sendIntent, PendingIntent.FLAG_ONE_SHOT
                                | PendingIntent.FLAG_CANCEL_CURRENT))
                .setAutoCancel(true)
                .setLocalOnly(true)
                .setColor(context.getColor(
                        com.android.internal.R.color.system_notification_accent_color));

        context.getSystemService(NotificationManager.class)
            .notify(Receiver.class.getName(), 0, builder.build());
    }

    private static void createNotificationChannel(Context context) {
        NotificationChannel channel = new NotificationChannel(
            NOTIFICATION_CHANNEL, context.getString(R.string.system_tracing),
            NotificationManager.IMPORTANCE_HIGH);
        channel.setBypassDnd(true);
        channel.enableVibration(true);
        channel.setSound(null, null);

        NotificationManager notificationManager =
            context.getSystemService(NotificationManager.class);
        notificationManager.createNotificationChannel(channel);
    }

    public static String getActiveTags(Context context, SharedPreferences prefs, boolean onlyAvailable) {
        Set<String> tags = prefs.getStringSet(context.getString(R.string.pref_key_tags),
                getDefaultTagList());
        StringBuilder sb = new StringBuilder(10 * tags.size());
        TreeMap<String, String> available =
                onlyAvailable ? AtraceUtils.atraceListCategories() : null;

        for (String s : tags) {
            if (onlyAvailable && !available.containsKey(s)) continue;
            if (sb.length() > 0) {
                sb.append(' ');
            }
            sb.append(s);
        }
        String s = sb.toString();
        Log.v(TAG, "getActiveTags(onlyAvailable=" + onlyAvailable + ") = \"" + s + "\"");
        return s;
    }

    public static String getActiveUnavailableTags(Context context, SharedPreferences prefs) {
        Set<String> tags = prefs.getStringSet(context.getString(R.string.pref_key_tags),
                getDefaultTagList());
        StringBuilder sb = new StringBuilder(10 * tags.size());
        TreeMap<String, String> available = AtraceUtils.atraceListCategories();

        for (String s : tags) {
            if (available.containsKey(s)) continue;
            if (sb.length() > 0) {
                sb.append(' ');
            }
            sb.append(s);
        }
        String s = sb.toString();
        Log.v(TAG, "getActiveUnavailableTags() = \"" + s + "\"");
        return s;
    }

    public static Set<String> getDefaultTagList() {
        return Build.TYPE.equals("user") ? ATRACE_TAGS_USER : ATRACE_TAGS;
    }
}
