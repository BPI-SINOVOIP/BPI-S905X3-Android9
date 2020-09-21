/*
 * Copyright (C) 2016 The Android Open Source Project
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
package com.android.car.pm;

import android.app.Activity;
import android.car.Car;
import android.car.CarNotConnectedException;
import android.car.content.pm.CarPackageManager;
import android.car.drivingstate.CarUxRestrictions;
import android.car.drivingstate.CarUxRestrictionsManager;
import android.content.ComponentName;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.IBinder;
import android.text.TextUtils;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import com.android.car.CarLog;
import com.android.car.R;

/**
 * Default activity that will be launched when the current foreground activity is not allowed.
 * Additional information on blocked Activity will be passed as extra in Intent
 * via {@link #INTENT_KEY_BLOCKED_ACTIVITY} key.
 */
public class ActivityBlockingActivity extends Activity {
    public static final String INTENT_KEY_BLOCKED_ACTIVITY = "blocked_activity";
    public static final String EXTRA_BLOCKED_TASK = "blocked_task";

    private static final int INVALID_TASK_ID = -1;

    private Car mCar;
    private CarUxRestrictionsManager mUxRManager;

    private TextView mBlockedTitle;
    private Button mExitButton;
    // Exiting depends on Car connection, which might not be available at the time exit was
    // requested (e.g. user presses Exit Button). In that case, we record exiting was requested, and
    // Car connection will perform exiting once it is established.
    private boolean mExitRequested;
    private int mBlockedTaskId;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_blocking);

        mBlockedTitle = findViewById(R.id.activity_blocked_title);
        mExitButton = findViewById(R.id.exit);
        mExitButton.setOnClickListener(v -> handleFinish());

        // Listen to the CarUxRestrictions so this blocking activity can be dismissed when the
        // restrictions are lifted.
        mCar = Car.createCar(this, new ServiceConnection() {
            @Override
            public void onServiceConnected(ComponentName name, IBinder service) {
                try {
                    if (mExitRequested) {
                        handleFinish();
                    }
                    mUxRManager = (CarUxRestrictionsManager) mCar.getCarManager(
                            Car.CAR_UX_RESTRICTION_SERVICE);
                    // This activity would have been launched only in a restricted state.
                    // But ensuring when the service connection is established, that we are still
                    // in a restricted state.
                    handleUxRChange(mUxRManager.getCurrentCarUxRestrictions());
                    mUxRManager.registerListener(ActivityBlockingActivity.this::handleUxRChange);
                } catch (CarNotConnectedException e) {
                    Log.e(CarLog.TAG_AM, "Failed to get CarUxRestrictionsManager", e);
                }
            }

            @Override
            public void onServiceDisconnected(ComponentName name) {
                finish();
                mUxRManager = null;
            }
        });
        mCar.connect();
    }

    @Override
    protected void onResume() {
        super.onResume();

        // Display message about the current blocked activity, and optionally show an exit button
        // to restart the blocked task (stack of activities) if its root activity is DO.

        // blockedActivity is expected to be always passed in as the topmost activity of task.
        String blockedActivity = getIntent().getStringExtra(INTENT_KEY_BLOCKED_ACTIVITY);
        mBlockedTitle.setText(getString(R.string.activity_blocked_string,
                findHumanReadableLabel(blockedActivity)));
        if (Log.isLoggable(CarLog.TAG_AM, Log.DEBUG)) {
            Log.d(CarLog.TAG_AM, "Blocking activity " + blockedActivity);
        }

        // taskId is available as extra if the task can be restarted.
        mBlockedTaskId = getIntent().getIntExtra(EXTRA_BLOCKED_TASK, INVALID_TASK_ID);

        mExitButton.setVisibility(mBlockedTaskId == INVALID_TASK_ID ? View.GONE : View.VISIBLE);
        if (Log.isLoggable(CarLog.TAG_AM, Log.DEBUG) && mBlockedTaskId == INVALID_TASK_ID) {
            Log.d(CarLog.TAG_AM, "Blocked task ID is not available. Hiding exit button.");
        }
    }

    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        setIntent(intent);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (mCar.isConnected() && mUxRManager != null) {
            try {
                mUxRManager.unregisterListener();
            } catch (CarNotConnectedException e) {
                Log.e(CarLog.TAG_AM, "Cannot unregisterListener", e);
            }
            mUxRManager = null;
            mCar.disconnect();
        }
    }

    // If no distraction optimization is required in the new restrictions, then dismiss the
    // blocking activity (self).
    private void handleUxRChange(CarUxRestrictions restrictions) {
        if (restrictions == null) {
            return;
        }
        if (!restrictions.isRequiresDistractionOptimization()) {
            finish();
        }
    }

    /**
     * Returns a human-readable string for {@code flattenComponentName}.
     *
     * <p>It first attempts to return the application label for this activity. If that fails,
     * it will return the last part in the activity name.
     */
    private String findHumanReadableLabel(String flattenComponentName) {
        ComponentName componentName = ComponentName.unflattenFromString(flattenComponentName);
        String label = null;
        // Attempt to find application label.
        try {
            ApplicationInfo applicationInfo = getPackageManager().getApplicationInfo(
                    componentName.getPackageName(), 0);
            CharSequence appLabel = getPackageManager().getApplicationLabel(applicationInfo);
            if (appLabel != null) {
                label = appLabel.toString();
            }
        } catch (PackageManager.NameNotFoundException e) {
            if (Log.isLoggable(CarLog.TAG_AM, Log.INFO)) {
                Log.i(CarLog.TAG_AM, "Could not find package for component name "
                        + componentName.toString());
            }
        }
        if (TextUtils.isEmpty(label)) {
            label = componentName.getClass().getSimpleName();
        }
        return label;
    }

    private void handleFinish() {
        if (!mCar.isConnected()) {
            mExitRequested = true;
            return;
        }
        if (isFinishing()) {
            return;
        }

        // Lock on self (assuming single instance) to avoid restarting the same task twice.
        synchronized (this) {
            try {
                CarPackageManager carPm = (CarPackageManager)
                        mCar.getCarManager(Car.PACKAGE_SERVICE);
                carPm.restartTask(mBlockedTaskId);
            } catch (CarNotConnectedException e) {
                // We should never be here since Car connection is already checked.
                Log.e(CarLog.TAG_AM, "Car connection is not available.", e);
                return;
            }
            finish();
        }
    }
}
