/*
 * Copyright 2018 The Android Open Source Project
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

package com.android.car.media.common;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.service.media.MediaBrowserService;
import android.util.Log;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.function.Consumer;
import java.util.stream.Collectors;

/**
 * This manager provides access to the list of all possible media sources that can be selected
 * to be played.
 * <p>
 * It also provides means to set and retrieve the last played media source, in order to disambiguate
 * in cases where there is no media application currently playing.
 */
public class MediaSourcesManager {
    private static final String TAG = "MediaSourcesManager";
    private final Context mContext;
    private List<MediaSource> mMediaSources;
    private List<Observer> mObservers = new ArrayList<>();
    private static AppInstallUninstallReceiver sReceiver;

    /**
     * Observer of media source changes
     */
    public interface Observer {
        /**
         * Invoked when the list of media sources has changed
         */
        void onMediaSourcesChanged();
    }

    /**
     * Creates a new instance of the manager for the given context
     */
    public MediaSourcesManager(Context context) {
        mContext = context;
    }

    /**
     * Registers an observer. Consumers must remember to unregister their observers to avoid
     * memory leaks.
     */
    public void registerObserver(Observer observer) {
        mObservers.add(observer);
        if (sReceiver == null) {
            registerBroadcastReceiver();
            updateMediaSources();
        }
    }

    /**
     * Unregisters an observer.
     */
    public void unregisterObserver(Observer observer) {
        mObservers.remove(observer);
        if (mObservers.isEmpty() && sReceiver != null) {
            unregisterBroadcastReceiver();
        }
    }

    private void notify(Consumer<Observer> notification) {
        for (Observer observer : mObservers) {
            notification.accept(observer);
        }
    }

    /**
     * Returns the list of available media sources.
     */
    public List<MediaSource> getMediaSources() {
        if (sReceiver == null) {
            updateMediaSources();
        }
        return mMediaSources;
    }

    private void registerBroadcastReceiver() {
        sReceiver = new AppInstallUninstallReceiver();
        IntentFilter filter = new IntentFilter();
        filter.addAction(Intent.ACTION_PACKAGE_ADDED);
        filter.addAction(Intent.ACTION_PACKAGE_REMOVED);
        filter.addDataScheme("package");
        mContext.getApplicationContext().registerReceiver(sReceiver, filter);
    }

    private void unregisterBroadcastReceiver() {
        mContext.getApplicationContext().unregisterReceiver(sReceiver);
        sReceiver = null;
    }

    private void updateMediaSources() {
        mMediaSources = getPackageNames().stream()
                .filter(packageName -> packageName != null)
                .map(packageName -> new MediaSource(mContext, packageName))
                .filter(mediaSource -> {
                    if (mediaSource.getName() == null) {
                        Log.w(TAG, "Found media source without name: "
                                + mediaSource.getPackageName());
                        return false;
                    }
                    return true;
                })
                .sorted(Comparator.comparing(mediaSource -> mediaSource.getName().toString()))
                .collect(Collectors.toList());
    }

    /**
     * Generates a set of all possible apps to choose from, including the ones that are just
     * media services.
     */
    private Set<String> getPackageNames() {
        PackageManager packageManager = mContext.getPackageManager();
        Intent intent = new Intent(Intent.ACTION_MAIN, null);
        intent.addCategory(Intent.CATEGORY_APP_MUSIC);

        Intent mediaIntent = new Intent();
        mediaIntent.setAction(MediaBrowserService.SERVICE_INTERFACE);

        List<ResolveInfo> availableActivities = packageManager.queryIntentActivities(intent, 0);
        List<ResolveInfo> mediaServices = packageManager.queryIntentServices(mediaIntent,
                PackageManager.GET_RESOLVED_FILTER);

        Set<String> apps = new HashSet<>();
        for (ResolveInfo info : mediaServices) {
            apps.add(info.serviceInfo.packageName);
        }
        for (ResolveInfo info : availableActivities) {
            apps.add(info.activityInfo.packageName);
        }
        return apps;
    }

    private class AppInstallUninstallReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            updateMediaSources();
            MediaSourcesManager.this.notify(Observer::onMediaSourcesChanged);
        }
    }
}
