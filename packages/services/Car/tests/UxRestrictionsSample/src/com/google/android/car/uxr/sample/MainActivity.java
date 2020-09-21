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
package com.google.android.car.uxr.sample;

import android.annotation.DrawableRes;
import android.app.Activity;
import android.car.Car;
import android.car.CarNotConnectedException;
import android.car.content.pm.CarPackageManager;
import android.car.drivingstate.CarDrivingStateEvent;
import android.car.drivingstate.CarDrivingStateManager;
import android.car.drivingstate.CarUxRestrictions;
import android.car.drivingstate.CarUxRestrictionsManager;
import android.content.ComponentName;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.util.Log;
import android.widget.Button;
import android.widget.TextView;

import androidx.car.widget.ListItem;
import androidx.car.widget.ListItemAdapter;
import androidx.car.widget.ListItemProvider;
import androidx.car.widget.PagedListView;
import androidx.car.widget.TextListItem;

import java.util.ArrayList;
import java.util.List;

/**
 * Sample app that uses components in car support library to demonstrate Car drivingstate UXR
 * status.
 */
public class MainActivity extends Activity {
    public static final String TAG = "drivingstate";

    private Car mCar;
    private CarDrivingStateManager mCarDrivingStateManager;
    private CarUxRestrictionsManager mCarUxRestrictionsManager;
    private CarPackageManager mCarPackageManager;
    private TextView mDrvStatus;
    private TextView mDistractionOptStatus;
    private TextView mUxrStatus;
    private Button mToggleButton;
    private boolean mEnableUxR;
    private PagedListView mPagedListView;

    private final ServiceConnection mCarConnectionListener =
            new ServiceConnection() {
                @Override
                public void onServiceConnected(ComponentName name, IBinder iBinder) {
                    Log.d(TAG, "Connected to " + name.flattenToString());
                    // Get Driving State & UXR manager
                    try {
                        mCarDrivingStateManager = (CarDrivingStateManager) mCar.getCarManager(
                                Car.CAR_DRIVING_STATE_SERVICE);
                        mCarUxRestrictionsManager = (CarUxRestrictionsManager) mCar.getCarManager(
                                Car.CAR_UX_RESTRICTION_SERVICE);
                        mCarPackageManager = (CarPackageManager) mCar.getCarManager(
                                Car.PACKAGE_SERVICE);

                        if (mCarDrivingStateManager != null) {
                            mCarDrivingStateManager.registerListener(mDrvStateChangeListener);
                            updateDrivingStateText(
                                    mCarDrivingStateManager.getCurrentCarDrivingState());
                        }
                        if (mCarUxRestrictionsManager != null) {
                            mCarUxRestrictionsManager.registerListener(mUxRChangeListener);
                            updateUxRText(mCarUxRestrictionsManager.getCurrentCarUxRestrictions());
                        }

                    } catch (CarNotConnectedException e) {
                        Log.e(TAG, "Failed to get a connection", e);
                    }
                }

                @Override
                public void onServiceDisconnected(ComponentName name) {
                    Log.d(TAG, "Disconnected from " + name.flattenToString());
                    mCarDrivingStateManager = null;
                    mCarUxRestrictionsManager = null;
                    mCarPackageManager = null;
                }
            };

    private void updateUxRText(CarUxRestrictions restrictions) {
        mDistractionOptStatus.setText(
                restrictions.isRequiresDistractionOptimization()
                        ? "Requires Distraction Optimization"
                        : "No Distraction Optimization required");

        mUxrStatus.setText("Active Restrictions : 0x"
                + Integer.toHexString(restrictions.getActiveRestrictions()));

        mDistractionOptStatus.requestLayout();
        mUxrStatus.requestLayout();
    }

    private void updateToggleUxREnable() {
        if (mCarPackageManager == null) {
            return;
        }
        mCarPackageManager.setEnableActivityBlocking(mEnableUxR);
        if (mEnableUxR) {
            mToggleButton.setText("Disable UX Restrictions");
        } else {
            mToggleButton.setText("Enable UX Restrictions");
        }
        mEnableUxR = !mEnableUxR;
        mToggleButton.requestLayout();

    }

    private void updateDrivingStateText(CarDrivingStateEvent state) {
        if (state == null) {
            return;
        }
        String displayText;
        switch (state.eventValue) {
            case CarDrivingStateEvent.DRIVING_STATE_PARKED:
                displayText = "Parked";
                break;
            case CarDrivingStateEvent.DRIVING_STATE_IDLING:
                displayText = "Idling";
                break;
            case CarDrivingStateEvent.DRIVING_STATE_MOVING:
                displayText = "Moving";
                break;
            default:
                displayText = "Unknown";
        }
        mDrvStatus.setText("Driving State: " + displayText);
        mDrvStatus.requestLayout();
    }

    private CarUxRestrictionsManager.OnUxRestrictionsChangedListener mUxRChangeListener =
            this::updateUxRText;


    private CarDrivingStateManager.CarDrivingStateEventListener mDrvStateChangeListener =
            this::updateDrivingStateText;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main_activity);

        mDrvStatus = findViewById(R.id.driving_state);
        mDistractionOptStatus = findViewById(R.id.do_status);
        mUxrStatus = findViewById(R.id.uxr_status);
        mToggleButton = findViewById(R.id.toggle_status);
        mPagedListView = findViewById(R.id.paged_list_view);

        setUpPagedListView();

        mToggleButton.setOnClickListener(v -> {
            updateToggleUxREnable();
        });

        // Connect to car service
        mCar = Car.createCar(this, mCarConnectionListener);
        mCar.connect();
    }

    private void setUpPagedListView() {
        ListItemAdapter adapter = new ListItemAdapter(this, populateData());
        mPagedListView.setAdapter(adapter);
    }

    private ListItemProvider populateData() {
        List<ListItem> items = new ArrayList<>();
        items.add(createMessage(android.R.drawable.ic_menu_myplaces, "alice",
                "i have a really important message but it may hinder your ability to drive. "));

        items.add(createMessage(android.R.drawable.ic_menu_myplaces, "bob",
                "hey this is a really long message that i have always wanted to say. but before " +
                        "saying it i feel it's only appropriate if i lay some groundwork for it. "
                        + ""));
        items.add(createMessage(android.R.drawable.ic_menu_myplaces, "mom",
                "i think you are the best. i think you are the best. i think you are the best. " +
                        "i think you are the best. i think you are the best. i think you are the "
                        + "best. "
                        +
                        "i think you are the best. i think you are the best. i think you are the "
                        + "best. "
                        +
                        "i think you are the best. i think you are the best. i think you are the "
                        + "best. "
                        +
                        "i think you are the best. i think you are the best. i think you are the "
                        + "best. "
                        +
                        "i think you are the best. i think you are the best. "));
        items.add(createMessage(android.R.drawable.ic_menu_myplaces, "john", "hello world"));
        items.add(createMessage(android.R.drawable.ic_menu_myplaces, "jeremy",
                "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor " +
                        "incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, " +
                        "quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo " +
                        "consequat. Duis aute irure dolor in reprehenderit in voluptate velit " +
                        "esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat " +
                        "cupidatat non proident, sunt in culpa qui officia deserunt mollit " +
                        "anim id est laborum."));
        return new ListItemProvider.ListProvider(items);
    }

    private TextListItem createMessage(@DrawableRes int profile, String contact, String message) {
        TextListItem item = new TextListItem(this);
        item.setPrimaryActionIcon(profile, false /* useLargeIcon */);
        item.setTitle(contact);
        item.setBody(message);
        item.setSupplementalIcon(android.R.drawable.stat_notify_chat, false);
        return item;
    }

    @Override
    protected void onDestroy() {
        try {
            if (mCarUxRestrictionsManager != null) {
                mCarUxRestrictionsManager.unregisterListener();
            }
            if (mCarDrivingStateManager != null) {
                mCarDrivingStateManager.unregisterListener();
            }
        } catch (CarNotConnectedException e) {
            Log.e(TAG, "Error unregistering listeners", e);
        }
        if (mCar != null) {
            mCar.disconnect();
        }
    }
}

