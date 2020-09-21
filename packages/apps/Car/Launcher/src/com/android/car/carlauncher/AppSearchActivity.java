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
package com.android.car.carlauncher;

import android.annotation.Nullable;
import android.app.Activity;
import android.car.Car;
import android.car.CarNotConnectedException;
import android.car.content.pm.CarPackageManager;
import android.car.drivingstate.CarUxRestrictionsManager;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.content.pm.LauncherApps;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.IBinder;
import android.text.Editable;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.util.Log;
import android.view.View;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;

import java.util.List;

import androidx.car.widget.PagedListView;

/**
 * Activity that allows user to search in apps.
 */
public final class AppSearchActivity extends Activity {

    private static final String TAG = "AppSearchActivity";

    private PackageManager mPackageManager;
    private SearchResultAdapter mSearchResultAdapter;
    private AppInstallUninstallReceiver mInstallUninstallReceiver;
    private Car mCar;
    private CarPackageManager mCarPackageManager;
    private CarUxRestrictionsManager mCarUxRestrictionsManager;

    private ServiceConnection mCarConnectionListener = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            try {
                mCarUxRestrictionsManager = (CarUxRestrictionsManager) mCar.getCarManager(
                        Car.CAR_UX_RESTRICTION_SERVICE);
                mSearchResultAdapter.setIsDistractionOptimizationRequired(
                        mCarUxRestrictionsManager
                                .getCurrentCarUxRestrictions()
                                .isRequiresDistractionOptimization());
                mCarUxRestrictionsManager.registerListener(
                        restrictionInfo ->
                                mSearchResultAdapter.setIsDistractionOptimizationRequired(
                                        restrictionInfo.isRequiresDistractionOptimization()));

                mCarPackageManager = (CarPackageManager) mCar.getCarManager(Car.PACKAGE_SERVICE);
                mSearchResultAdapter.setAllApps(getAllApps());
            } catch (CarNotConnectedException e) {
                Log.e(TAG, "Car not connected in CarConnectionListener", e);
            }
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {
            mCarUxRestrictionsManager = null;
            mCarPackageManager = null;
        }
    };

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mPackageManager = getPackageManager();
        mCar = Car.createCar(this, mCarConnectionListener);

        setContentView(R.layout.app_search_activity);
        findViewById(R.id.container).setOnTouchListener(
                (view, event) -> {
                    hideKeyboard();
                    return false;
                });
        findViewById(R.id.exit_button_container).setOnClickListener(view -> finish());

        PagedListView searchResultView = findViewById(R.id.search_result);
        searchResultView.setClipToOutline(true);
        mSearchResultAdapter = new SearchResultAdapter(this);
        searchResultView.setAdapter(mSearchResultAdapter);

        EditText searchBar = findViewById(R.id.app_search_bar);
        searchBar.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {
            }

            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {
            }

            @Override
            public void afterTextChanged(Editable s) {
                if (TextUtils.isEmpty(s)) {
                    searchResultView.setVisibility(View.GONE);
                    mSearchResultAdapter.clearResults();
                } else {
                    searchResultView.setVisibility(View.VISIBLE);
                    mSearchResultAdapter.getFilter().filter(s.toString());
                }
            }
        });
    }

    @Override
    protected void onStart() {
        super.onStart();
        // register broadcast receiver for package installation and uninstallation
        mInstallUninstallReceiver = new AppInstallUninstallReceiver();
        IntentFilter filter = new IntentFilter();
        filter.addAction(Intent.ACTION_PACKAGE_ADDED);
        filter.addAction(Intent.ACTION_PACKAGE_CHANGED);
        filter.addAction(Intent.ACTION_PACKAGE_REPLACED);
        filter.addAction(Intent.ACTION_PACKAGE_REMOVED);
        filter.addDataScheme("package");
        registerReceiver(mInstallUninstallReceiver, filter);

        // Connect to car service
        mCar.connect();
    }

    @Override
    protected void onStop() {
        super.onPause();
        // disconnect from app install/uninstall receiver
        if (mInstallUninstallReceiver != null) {
            unregisterReceiver(mInstallUninstallReceiver);
            mInstallUninstallReceiver = null;
        }
        // disconnect from car listeners
        try {
            if (mCarUxRestrictionsManager != null) {
                mCarUxRestrictionsManager.unregisterListener();
            }
        } catch (CarNotConnectedException e) {
            Log.e(TAG, "Error unregistering listeners", e);
        }
        if (mCar != null) {
            mCar.disconnect();
        }
    }

    private List<AppMetaData> getAllApps() {
        return AppLauncherUtils.getAllLaunchableApps(
                getSystemService(LauncherApps.class), mCarPackageManager, mPackageManager);
    }

    public void hideKeyboard() {
        InputMethodManager inputMethodManager = (InputMethodManager) getSystemService(
                Activity.INPUT_METHOD_SERVICE);
        inputMethodManager.hideSoftInputFromWindow(getCurrentFocus().getWindowToken(), 0);
    }

    private class AppInstallUninstallReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            String packageName = intent.getData().getSchemeSpecificPart();

            if (TextUtils.isEmpty(packageName)) {
                Log.e(TAG, "System sent an empty app install/uninstall broadcast");
                return;
            }

            mSearchResultAdapter.setAllApps(getAllApps());
        }
    }
}
