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

package com.android.tv.receiver;

import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.text.TextUtils;
import android.os.Build;
import android.util.Log;
import com.android.tv.Starter;
import com.android.tv.TvActivity;
import com.android.tv.TvFeatures;
import com.android.tv.TvSingletons;
import com.android.tv.dvr.recorder.DvrRecordingService;
import com.android.tv.dvr.recorder.RecordingScheduler;
import com.android.tv.recommendation.ChannelPreviewUpdater;
import com.android.tv.recommendation.NotificationService;
import com.android.tv.util.OnboardingUtils;
import com.android.tv.util.SetupUtils;
import com.android.tv.common.util.SystemPropertiesProxy;

/**
 * Boot completed receiver for TV app.
 *
 * <p>It's used to
 *
 * <ul>
 *   <li>start the {@link NotificationService} for recommendation
 *   <li>grant permission to the TIS's
 *   <li>enable {@link TvActivity} if necessary
 *   <li>start the {@link DvrRecordingService}
 * </ul>
 */
public class BootCompletedReceiver extends BroadcastReceiver {
    private static final String TAG = "BootCompletedReceiver";
    private static final boolean DEBUG = false;

    @Override
    public void onReceive(Context context, Intent intent) {
        if (!TvSingletons.getSingletons(context).getTvInputManagerHelper().hasTvInputManager()) {
            Log.wtf(TAG, "Stopping because device does not have a TvInputManager");
            return;
        }
        if (DEBUG) Log.d(TAG, "boot completed " + intent);
        Starter.start(context);

        if (!TextUtils.isEmpty(SystemPropertiesProxy.getString("ro.com.google.gmsversion", ""))) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                ChannelPreviewUpdater.getInstance(context).updatePreviewDataForChannelsImmediately();
            } else {
                Intent notificationIntent = new Intent(context, NotificationService.class);
                notificationIntent.setAction(NotificationService.ACTION_SHOW_RECOMMENDATION);
                context.startService(notificationIntent);
            }
        }

        // Grant permission to already set up packages after the system has finished booting.
        SetupUtils.grantEpgPermissionToSetUpPackages(context);

        if (TvFeatures.UNHIDE.isEnabled(context)) {
            if (OnboardingUtils.isFirstBoot(context)) {
                // Enable the application if this is the first "unhide" feature is enabled just in
                // case when the app has been disabled before.
                PackageManager pm = context.getPackageManager();
                ComponentName name = new ComponentName(context, TvActivity.class);
                if (pm.getComponentEnabledSetting(name)
                        != PackageManager.COMPONENT_ENABLED_STATE_ENABLED) {
                    pm.setComponentEnabledSetting(
                            name, PackageManager.COMPONENT_ENABLED_STATE_ENABLED, 0);
                }
                OnboardingUtils.setFirstBootCompleted(context);
            }
        }

        RecordingScheduler scheduler = TvSingletons.getSingletons(context).getRecordingScheduler();
        if (scheduler != null) {
            scheduler.updateAndStartServiceIfNeeded();
        }
    }
}
