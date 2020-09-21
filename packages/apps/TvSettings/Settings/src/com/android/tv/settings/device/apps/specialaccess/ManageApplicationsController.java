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
 * limitations under the License.
 */

package com.android.tv.settings.device.apps.specialaccess;

import android.app.Application;
import android.arch.lifecycle.Lifecycle;
import android.arch.lifecycle.LifecycleObserver;
import android.content.Context;
import android.support.annotation.NonNull;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceGroup;

import com.android.settingslib.applications.ApplicationsState;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;

/**
 * A class to manage a list of apps in a {@link PreferenceGroup}. The list is configured by passing
 * an {@link ApplicationsState.AppFilter} and a {@link Comparator} for
 * {@link ApplicationsState.AppEntry} objects, and the PreferenceGroup is manipulated through the
 * {@link Callback} object.
 */
public class ManageApplicationsController implements LifecycleObserver {
    /**
     *  Use this preference key for a header pref not removed during refresh
     */
    public static final String HEADER_KEY = "header";

    private final Callback mCallback;
    private final Lifecycle mLifecycle;
    private final ApplicationsState.AppFilter mFilter;
    private final Comparator<ApplicationsState.AppEntry> mComparator;

    private ApplicationsState.Session mAppSession;
    private ApplicationsState mApplicationsState;
    private final ApplicationsState.Callbacks mAppSessionCallbacks =
            new ApplicationsState.Callbacks() {

                @Override
                public void onRunningStateChanged(boolean running) {
                    updateAppList();
                }

                @Override
                public void onPackageListChanged() {
                    updateAppList();
                }

                @Override
                public void onRebuildComplete(ArrayList<ApplicationsState.AppEntry> apps) {
                    updateAppList(apps);
                }

                @Override
                public void onPackageIconChanged() {
                    updateAppList();
                }

                @Override
                public void onPackageSizeChanged(String packageName) {
                    updateAppList();
                }

                @Override
                public void onAllSizesComputed() {
                    updateAppList();
                }

                @Override
                public void onLauncherInfoChanged() {
                    updateAppList();
                }

                @Override
                public void onLoadEntriesCompleted() {
                    updateAppList();
                }
            };

    public ManageApplicationsController(@NonNull Context context, @NonNull Callback callback,
            @NonNull Lifecycle lifecycle, ApplicationsState.AppFilter filter,
            Comparator<ApplicationsState.AppEntry> comparator) {
        mCallback = callback;
        lifecycle.addObserver(this);
        mLifecycle = lifecycle;
        mFilter = filter;
        mComparator = comparator;
        mApplicationsState = ApplicationsState.getInstance(
                (Application) context.getApplicationContext());
        mAppSession = mApplicationsState.newSession(mAppSessionCallbacks, mLifecycle);
        updateAppList();
    }

    /**
     * Call this method to trigger the app list to refresh.
     */
    public void updateAppList() {
        ApplicationsState.AppFilter filter = new ApplicationsState.CompoundFilter(
                mFilter, ApplicationsState.FILTER_NOT_HIDE);
        ArrayList<ApplicationsState.AppEntry> apps = mAppSession.rebuild(filter, mComparator);
        if (apps != null) {
            updateAppList(apps);
        }
    }

    private void updateAppList(ArrayList<ApplicationsState.AppEntry> apps) {
        PreferenceGroup group = mCallback.getAppPreferenceGroup();
        final List<Preference> newList = new ArrayList<>(apps.size() + 1);
        for (final ApplicationsState.AppEntry entry : apps) {
            final String packageName = entry.info.packageName;
            mApplicationsState.ensureIcon(entry);
            Preference recycle = group.findPreference(packageName);
            if (recycle == null) {
                recycle = mCallback.createAppPreference();
            }
            newList.add(mCallback.bindPreference(recycle, entry));
        }
        final Preference header = group.findPreference(HEADER_KEY);
        // Because we're sorting the app entries, we should remove-all to ensure that sort order
        // is retained
        group.removeAll();
        if (header != null) {
            group.addPreference(header);
        }
        if (newList.size() > 0) {
            for (Preference prefToAdd : newList) {
                group.addPreference(prefToAdd);
            }
        } else {
            group.addPreference(mCallback.getEmptyPreference());
        }
    }

    /**
     * Callback interface for this class to manipulate the list of app preferences.
     */
    public interface Callback {
        /**
         * Configure the {@link Preference} object with the data in the
         * {@link ApplicationsState.AppEntry}
         * @param preference Preference to configure
         * @param entry Entry containing data to bind
         * @return Return the configured Preference object
         */
        @NonNull Preference bindPreference(@NonNull Preference preference,
                ApplicationsState.AppEntry entry);

        /**
         * Create a new instance of a {@link Preference} subclass to be used to display an
         * {@link ApplicationsState.AppEntry}
         * @return New Preference object
         */
        @NonNull Preference createAppPreference();

        /**
         * @return {@link Preference} object to be used as an empty state placeholder
         */
        @NonNull Preference getEmptyPreference();

        /**
         * The {@link PreferenceGroup} returned here should contain only app preference objects,
         * plus optionally a header preference with the key {@link #HEADER_KEY}
         * @return PreferenceGroup to manipulate
         */
        @NonNull PreferenceGroup getAppPreferenceGroup();
    }
}
