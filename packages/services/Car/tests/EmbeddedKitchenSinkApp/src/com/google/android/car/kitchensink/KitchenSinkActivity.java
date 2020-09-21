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

package com.google.android.car.kitchensink;


import android.car.hardware.CarSensorManager;
import android.car.hardware.hvac.CarHvacManager;
import android.car.hardware.power.CarPowerManager;
import android.car.hardware.property.CarPropertyManager;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.support.car.Car;
import android.support.car.CarAppFocusManager;
import android.support.car.CarConnectionCallback;
import android.support.car.CarNotConnectedException;
import android.support.v4.app.Fragment;
import android.util.Log;

import androidx.car.drawer.CarDrawerActivity;
import androidx.car.drawer.CarDrawerAdapter;
import androidx.car.drawer.DrawerItemViewHolder;

import com.google.android.car.kitchensink.activityview.ActivityViewTestFragment;
import com.google.android.car.kitchensink.alertdialog.AlertDialogTestFragment;
import com.google.android.car.kitchensink.assistant.CarAssistantFragment;
import com.google.android.car.kitchensink.audio.AudioTestFragment;
import com.google.android.car.kitchensink.bluetooth.BluetoothHeadsetFragment;
import com.google.android.car.kitchensink.bluetooth.MapMceTestFragment;
import com.google.android.car.kitchensink.cluster.InstrumentClusterFragment;
import com.google.android.car.kitchensink.cube.CubesTestFragment;
import com.google.android.car.kitchensink.diagnostic.DiagnosticTestFragment;
import com.google.android.car.kitchensink.displayinfo.DisplayInfoFragment;
import com.google.android.car.kitchensink.hvac.HvacTestFragment;
import com.google.android.car.kitchensink.input.InputTestFragment;
import com.google.android.car.kitchensink.job.JobSchedulerFragment;
import com.google.android.car.kitchensink.notification.NotificationFragment;
import com.google.android.car.kitchensink.orientation.OrientationTestFragment;
import com.google.android.car.kitchensink.power.PowerTestFragment;
import com.google.android.car.kitchensink.property.PropertyTestFragment;
import com.google.android.car.kitchensink.sensor.SensorsTestFragment;
import com.google.android.car.kitchensink.setting.CarServiceSettingsActivity;
import com.google.android.car.kitchensink.storagelifetime.StorageLifetimeFragment;
import com.google.android.car.kitchensink.touch.TouchTestFragment;
import com.google.android.car.kitchensink.vhal.VehicleHalFragment;
import com.google.android.car.kitchensink.volume.VolumeTestFragment;

import java.util.ArrayList;
import java.util.List;

public class KitchenSinkActivity extends CarDrawerActivity {
    private static final String TAG = "KitchenSinkActivity";

    private interface ClickHandler {
        void onClick();
    }

    private static abstract class MenuEntry implements ClickHandler {
        abstract String getText();
    }

    private final class OnClickMenuEntry extends MenuEntry {
        private final String mText;
        private final ClickHandler mClickHandler;

        OnClickMenuEntry(String text, ClickHandler clickHandler) {
            mText = text;
            mClickHandler = clickHandler;
        }

        @Override
        String getText() {
            return mText;
        }

        @Override
        public void onClick() {
            mClickHandler.onClick();
        }
    }

    private final class FragmentMenuEntry<T extends Fragment> extends MenuEntry {
        private final class FragmentClassOrInstance<T extends Fragment> {
            final Class<T> mClazz;
            T mFragment = null;

            FragmentClassOrInstance(Class<T> clazz) {
                mClazz = clazz;
            }

            T getFragment() {
                if (mFragment == null) {
                    try {
                        mFragment = mClazz.newInstance();
                    } catch (InstantiationException | IllegalAccessException e) {
                        Log.e(TAG, "unable to create fragment", e);
                    }
                }
                return mFragment;
            }
        }

        private final String mText;
        private final FragmentClassOrInstance<T> mFragment;

        FragmentMenuEntry(String text, Class<T> clazz) {
            mText = text;
            mFragment = new FragmentClassOrInstance<>(clazz);
        }

        @Override
        String getText() {
            return mText;
        }

        @Override
        public void onClick() {
            Fragment fragment = mFragment.getFragment();
            if (fragment != null) {
                KitchenSinkActivity.this.showFragment(fragment);
            } else {
                Log.e(TAG, "cannot show fragment for " + getText());
            }
        }
    }

    private final List<MenuEntry> mMenuEntries = new ArrayList<MenuEntry>() {
        {
            add("alert window", AlertDialogTestFragment.class);
            add("assistant", CarAssistantFragment.class);
            add("audio", AudioTestFragment.class);
            add("bluetooth headset",BluetoothHeadsetFragment.class);
            add("bluetooth messaging test", MapMceTestFragment.class);
            add("cubes test", CubesTestFragment.class);
            add("diagnostic", DiagnosticTestFragment.class);
            add("display info", DisplayInfoFragment.class);
            add("hvac", HvacTestFragment.class);
            add("inst cluster", InstrumentClusterFragment.class);
            add("input test", InputTestFragment.class);
            add("job scheduler", JobSchedulerFragment.class);
            add("notification", NotificationFragment.class);
            add("orientation test", OrientationTestFragment.class);
            add("power test", PowerTestFragment.class);
            add("property test", PropertyTestFragment.class);
            add("sensors", SensorsTestFragment.class);
            add("storage lifetime", StorageLifetimeFragment.class);
            add("touch test", TouchTestFragment.class);
            add("volume test", VolumeTestFragment.class);
            add("vehicle hal", VehicleHalFragment.class);
            add("car service settings", () -> {
                Intent intent = new Intent(KitchenSinkActivity.this,
                    CarServiceSettingsActivity.class);
                startActivity(intent);
            });
            add("activity view", ActivityViewTestFragment.class);
            add("quit", KitchenSinkActivity.this::finish);
        }

        <T extends Fragment> void add(String text, Class<T> clazz) {
            add(new FragmentMenuEntry(text, clazz));
        }
        void add(String text, ClickHandler onClick) {
            add(new OnClickMenuEntry(text, onClick));
        }
    };
    private Car mCarApi;
    private CarHvacManager mHvacManager;
    private CarPowerManager mPowerManager;
    private CarPropertyManager mPropertyManager;
    private CarSensorManager mSensorManager;
    private CarAppFocusManager mCarAppFocusManager;

    public CarHvacManager getHvacManager() {
        return mHvacManager;
    }

    public CarPowerManager getPowerManager() {
        return mPowerManager;
    }

    public CarPropertyManager getPropertyManager() {
        return mPropertyManager;
    }

    @Override
    protected CarDrawerAdapter getRootAdapter() {
        return new DrawerAdapter();
    }

    public CarSensorManager getSensorManager() {
        return mSensorManager;
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setToolbarElevation(0f);
        setMainContent(R.layout.kitchen_content);
        // Connection to Car Service does not work for non-automotive yet.
        if (getPackageManager().hasSystemFeature(PackageManager.FEATURE_AUTOMOTIVE)) {
            mCarApi = Car.createCar(this, mCarConnectionCallback);
            mCarApi.connect();
        }
        Log.i(TAG, "onCreate");
    }

    @Override
    protected void onStart() {
        super.onStart();
        Log.i(TAG, "onStart");
    }

    @Override
    protected void onRestart() {
        super.onRestart();
        Log.i(TAG, "onRestart");
    }

    @Override
    protected void onResume() {
        super.onResume();
        Log.i(TAG, "onResume");
    }

    @Override
    protected void onPause() {
        super.onPause();
        Log.i(TAG, "onPause");
    }

    @Override
    protected void onStop() {
        super.onStop();
        Log.i(TAG, "onStop");
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (mCarApi != null) {
            mCarApi.disconnect();
        }
        Log.i(TAG, "onDestroy");
    }

    private void showFragment(Fragment fragment) {
        getSupportFragmentManager().beginTransaction()
                .replace(R.id.kitchen_content, fragment)
                .commit();
    }

    private final CarConnectionCallback mCarConnectionCallback =
            new CarConnectionCallback() {
        @Override
        public void onConnected(Car car) {
            Log.d(TAG, "Connected to Car Service");
            try {
                mHvacManager = (CarHvacManager) mCarApi.getCarManager(android.car.Car.HVAC_SERVICE);
                mPowerManager = (CarPowerManager) mCarApi.getCarManager(
                    android.car.Car.POWER_SERVICE);
                mPropertyManager = (CarPropertyManager) mCarApi.getCarManager(
                    android.car.Car.PROPERTY_SERVICE);
                mSensorManager = (CarSensorManager) mCarApi.getCarManager(
                    android.car.Car.SENSOR_SERVICE);
                mCarAppFocusManager =
                        (CarAppFocusManager) mCarApi.getCarManager(Car.APP_FOCUS_SERVICE);
            } catch (CarNotConnectedException e) {
                Log.e(TAG, "Car is not connected!", e);
            }
        }

        @Override
        public void onDisconnected(Car car) {
            Log.d(TAG, "Disconnect from Car Service");
        }
    };

    public Car getCar() {
        return mCarApi;
    }

    private final class DrawerAdapter extends CarDrawerAdapter {

        public DrawerAdapter() {
            super(KitchenSinkActivity.this, true /* showDisabledOnListOnEmpty */);
            setTitle(getString(R.string.app_title));
        }

        @Override
        protected int getActualItemCount() {
            return mMenuEntries.size();
        }

        @Override
        protected void populateViewHolder(DrawerItemViewHolder holder, int position) {
            holder.getTitle().setText(mMenuEntries.get(position).getText());
        }

        @Override
        public void onItemClick(int position) {
            if ((position < 0) || (position >= mMenuEntries.size())) {
                Log.wtf(TAG, "Unknown menu item: " + position);
                return;
            }

            mMenuEntries.get(position).onClick();

            getDrawerController().closeDrawer();
        }
    }
}
