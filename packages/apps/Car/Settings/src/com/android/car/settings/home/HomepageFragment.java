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
package com.android.car.settings.home;


import static com.android.car.settings.home.ExtraSettingsLoader.DEVICE_CATEGORY;
import static com.android.car.settings.home.ExtraSettingsLoader.PERSONAL_CATEGORY;
import static com.android.car.settings.home.ExtraSettingsLoader.WIRELESS_CATEGORY;

import android.bluetooth.BluetoothAdapter;
import android.car.user.CarUserManagerHelper;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.drawable.Icon;
import android.os.Bundle;

import com.android.car.list.LaunchAppLineItem;
import com.android.car.list.TypedPagedListAdapter;
import com.android.car.settings.R;
import com.android.car.settings.accounts.AccountsListFragment;
import com.android.car.settings.applications.ApplicationSettingsFragment;
import com.android.car.settings.common.ListSettingsFragment;
import com.android.car.settings.common.Logger;
import com.android.car.settings.datetime.DatetimeSettingsFragment;
import com.android.car.settings.display.DisplaySettingsFragment;
import com.android.car.settings.security.SettingsScreenLockActivity;
import com.android.car.settings.sound.SoundSettingsFragment;
import com.android.car.settings.suggestions.SettingsSuggestionsController;
import com.android.car.settings.system.SystemSettingsFragment;
import com.android.car.settings.users.UsersListFragment;
import com.android.car.settings.wifi.CarWifiManager;
import com.android.car.settings.wifi.WifiUtil;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Map;

/**
 * Homepage for settings for car.
 */
public class HomepageFragment extends ListSettingsFragment implements
        CarWifiManager.Listener,
        SettingsSuggestionsController.Listener {
    private static final Logger LOG = new Logger(HomepageFragment.class);

    private SettingsSuggestionsController mSettingsSuggestionsController;
    private CarWifiManager mCarWifiManager;
    private WifiLineItem mWifiLineItem;
    private BluetoothLineItem mBluetoothLineItem;
    private CarUserManagerHelper mCarUserManagerHelper;
    // This tracks the number of suggestions currently shown in the fragment. This is based off of
    // the assumption that suggestions are 0 through (num suggestions - 1) in the adapter. Do not
    // change this assumption without updating the code in onSuggestionLoaded.
    private int mNumSettingsSuggestions;

    private final BroadcastReceiver mBtStateReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();

            if (action.equals(BluetoothAdapter.ACTION_STATE_CHANGED)) {
                int state = intent.getIntExtra(BluetoothAdapter.EXTRA_STATE,
                        BluetoothAdapter.ERROR);
                switch (state) {
                    case BluetoothAdapter.STATE_TURNING_OFF:
                        // TODO show a different status icon?
                    case BluetoothAdapter.STATE_OFF:
                        mBluetoothLineItem.onBluetoothStateChanged(false);
                        break;
                    default:
                        mBluetoothLineItem.onBluetoothStateChanged(true);
                }
            }
        }
    };

    private final IntentFilter mBtStateChangeFilter =
            new IntentFilter(BluetoothAdapter.ACTION_STATE_CHANGED);

    /**
     * Gets an instance of this class.
     */
    public static HomepageFragment newInstance() {
        HomepageFragment homepageFragment = new HomepageFragment();
        Bundle bundle = ListSettingsFragment.getBundle();
        bundle.putInt(EXTRA_TITLE_ID, R.string.settings_label);
        homepageFragment.setArguments(bundle);
        return homepageFragment;
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        LOG.v("onActivityCreated: " + savedInstanceState);
        mSettingsSuggestionsController =
                new SettingsSuggestionsController(
                        getContext(),
                        getLoaderManager(),
                        /* listener= */ this);
        mCarWifiManager = new CarWifiManager(getContext(), /* listener= */ this);
        if (WifiUtil.isWifiAvailable(getContext())) {
            mWifiLineItem = new WifiLineItem(
                    getContext(), mCarWifiManager, getFragmentController());
        }
        mBluetoothLineItem = new BluetoothLineItem(getContext(), getFragmentController());
        mCarUserManagerHelper = new CarUserManagerHelper(getContext());

        // reset the suggestion count.
        mNumSettingsSuggestions = 0;

        // Call super after the wifiLineItem and BluetoothLineItem are setup, because
        // those are needed in super.onCreate().
        super.onActivityCreated(savedInstanceState);
    }

    @Override
    public void onAccessPointsChanged() {
        // don't care
    }

    @Override
    public void onWifiStateChanged(int state) {
        mWifiLineItem.onWifiStateChanged(state);
    }

    @Override
    public void onStart() {
        super.onStart();
        mCarWifiManager.start();
        mSettingsSuggestionsController.start();
        getActivity().registerReceiver(mBtStateReceiver, mBtStateChangeFilter);
    }

    @Override
    public void onStop() {
        super.onStop();
        mCarWifiManager.stop();
        mSettingsSuggestionsController.stop();
        getActivity().unregisterReceiver(mBtStateReceiver);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        mCarWifiManager.destroy();
    }

    @Override
    public ArrayList<TypedPagedListAdapter.LineItem> getLineItems() {
        ExtraSettingsLoader extraSettingsLoader = new ExtraSettingsLoader(getContext());
        Map<String, Collection<TypedPagedListAdapter.LineItem>> extraSettings =
                extraSettingsLoader.load();
        ArrayList<TypedPagedListAdapter.LineItem> lineItems = new ArrayList<>();
        lineItems.add(new SimpleIconTransitionLineItem(
                R.string.display_settings,
                R.drawable.ic_settings_display,
                getContext(),
                null,
                DisplaySettingsFragment.newInstance(),
                getFragmentController()));
        lineItems.add(new SimpleIconTransitionLineItem(
                R.string.sound_settings,
                R.drawable.ic_settings_sound,
                getContext(),
                null,
                SoundSettingsFragment.newInstance(),
                getFragmentController()));
        if (mWifiLineItem != null) {
            lineItems.add(mWifiLineItem);
        }
        lineItems.addAll(extraSettings.get(WIRELESS_CATEGORY));
        lineItems.add(mBluetoothLineItem);
        lineItems.add(new SimpleIconTransitionLineItem(
                R.string.applications_settings,
                R.drawable.ic_settings_applications,
                getContext(),
                null,
                ApplicationSettingsFragment.newInstance(),
                getFragmentController()));
        lineItems.add(new SimpleIconTransitionLineItem(
                R.string.date_and_time_settings_title,
                R.drawable.ic_settings_date_time,
                getContext(),
                null,
                DatetimeSettingsFragment.getInstance(),
                getFragmentController()));
        lineItems.add(new SimpleIconTransitionLineItem(
                R.string.users_list_title,
                R.drawable.ic_user,
                getContext(),
                null,
                UsersListFragment.newInstance(),
                getFragmentController()));

        // Guest users can't set screen locks or add/remove accounts.
        if (!mCarUserManagerHelper.isCurrentProcessGuestUser()) {
            lineItems.add(new SimpleIconTransitionLineItem(
                    R.string.accounts_settings_title,
                    R.drawable.ic_account,
                    getContext(),
                    null,
                    AccountsListFragment.newInstance(),
                    getFragmentController()));
            lineItems.add(new LaunchAppLineItem(
                    getString(R.string.security_settings_title),
                    Icon.createWithResource(getContext(), R.drawable.ic_lock),
                    getContext(),
                    null,
                    new Intent(getContext(), SettingsScreenLockActivity.class)));
        }

        lineItems.add(new SimpleIconTransitionLineItem(
                R.string.system_setting_title,
                R.drawable.ic_settings_about,
                getContext(),
                null,
                SystemSettingsFragment.getInstance(),
                getFragmentController()));

        lineItems.addAll(extraSettings.get(DEVICE_CATEGORY));
        lineItems.addAll(extraSettings.get(PERSONAL_CATEGORY));
        return lineItems;
    }

    @Override
    public void onSuggestionsLoaded(ArrayList<TypedPagedListAdapter.LineItem> suggestions) {
        LOG.v("onDeferredSuggestionsLoaded");
        for (int index = 0; index < mNumSettingsSuggestions; index++) {
            mPagedListAdapter.remove(0);
        }
        mNumSettingsSuggestions = suggestions.size();
        mPagedListAdapter.addAll(0, suggestions);
    }

    @Override
    public void onSuggestionDismissed(int adapterPosition) {
        LOG.v("onSuggestionDismissed adapterPosition " + adapterPosition);
        mPagedListAdapter.remove(adapterPosition);
    }
}
