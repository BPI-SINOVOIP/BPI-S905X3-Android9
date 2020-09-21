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
 * limitations under the License
 */
package com.android.car.settings.datetime;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.provider.Settings;

import androidx.car.widget.ListItem;
import androidx.car.widget.ListItemProvider;
import androidx.car.widget.ListItemProvider.ListProvider;

import com.android.car.settings.R;
import com.android.car.settings.common.ListItemSettingsFragment;

import java.util.ArrayList;
import java.util.List;

/**
 * Configures date and time.
 */
public class DatetimeSettingsFragment extends ListItemSettingsFragment {
    private static final IntentFilter TIME_CHANGED_FILTER =
            new IntentFilter(Intent.ACTION_TIME_CHANGED);

    // Minimum time is Nov 5, 2007, 0:00.
    public static final long MIN_DATE = 1194220800000L;

    private List<ListItem> mListItems;

    private final TimeChangedBroadCastReceiver mTimeChangedBroadCastReceiver =
            new TimeChangedBroadCastReceiver();

    /**
     * Observes list refreshes.
     */
    public interface ListRefreshObserver {

        /**
         * Gets called when the list is about to refresh. Subclass should set the view to ListItem
         * state, so can be reflected on next fresh.
         */
        void onPreRefresh();
    }

    public static DatetimeSettingsFragment getInstance() {
        DatetimeSettingsFragment datetimeSettingsFragment = new DatetimeSettingsFragment();
        Bundle bundle = ListItemSettingsFragment.getBundle();
        bundle.putInt(EXTRA_TITLE_ID, R.string.date_and_time_settings_title);
        datetimeSettingsFragment.setArguments(bundle);
        return datetimeSettingsFragment;
    }

    @Override
    public ListItemProvider getItemProvider() {
        return new ListProvider(initializeListItems());
    }

    private List<ListItem> initializeListItems() {
        mListItems = new ArrayList<>();
        mListItems.add(new DateTimeToggleLineItem(getContext(),
                getString(R.string.date_time_auto),
                getString(R.string.date_time_auto_summary),
                Settings.Global.AUTO_TIME));
        mListItems.add(new DateTimeToggleLineItem(getContext(),
                getString(R.string.zone_auto),
                getString(R.string.zone_auto_summary),
                Settings.Global.AUTO_TIME_ZONE));
        mListItems.add(new SetDateLineItem(getContext(), getFragmentController()));
        mListItems.add(new SetTimeLineItem(getContext(), getFragmentController()));
        mListItems.add(new SetTimeZoneLineItem(getContext(), getFragmentController()));
        mListItems.add(new TimeFormatToggleLineItem(getContext()));
        return mListItems;
    }

    @Override
    public void onStart() {
        super.onStart();
        getActivity().registerReceiver(mTimeChangedBroadCastReceiver, TIME_CHANGED_FILTER);
        refreshList();
    }

    @Override
    public void onStop() {
        super.onStop();
        getActivity().unregisterReceiver(mTimeChangedBroadCastReceiver);
    }

    private class TimeChangedBroadCastReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            for (ListItem listItem : mListItems) {
                if (!(listItem instanceof ListRefreshObserver)) {
                    throw new IllegalArgumentException(
                            "all list items should be ListRefreshObserver");
                }
                ((ListRefreshObserver) listItem).onPreRefresh();
            }
            refreshList();
        }
    }
}
