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
package com.google.android.car.defaultstoragemonitoringcompanionapp;

import android.app.Activity;
import android.car.Car;
import android.car.CarNotConnectedException;
import android.car.storagemonitoring.CarStorageMonitoringManager;
import android.car.storagemonitoring.WearEstimate;
import android.car.storagemonitoring.WearEstimateChange;
import android.content.ComponentName;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.util.Log;
import android.widget.TextView;
import java.time.Duration;
import java.time.ZoneId;
import java.time.format.DateTimeFormatter;
import java.time.format.FormatStyle;
import java.util.List;
import java.util.Locale;

public class MainActivity extends Activity {
    private static final boolean DEBUG = false;
    private static final String TAG = MainActivity.class.getSimpleName();

    private final ServiceConnection mCarConnectionListener = new ServiceConnection() {
        private String wearLevelToString(int wearLevel) {
            if (wearLevel == WearEstimate.UNKNOWN) return "unknown";
            return wearLevel + "%";
        }

        private String durationPairToString(long value1, String name1,
            long value2, String name2) {
            if (value1 > 0) {
                String s = value1 + " " + name1;
                if (value2 > 0) {
                    s += " and " + value2 + " " + name2;
                }
                return s;
            }
            return null;
        }

        private String durationToString(Duration duration) {
            final long days = duration.toDays();
            duration = duration.minusDays(days);
            final long hours = duration.toHours();
            duration = duration.minusHours(hours);
            final long minutes = duration.toMinutes();

            // for a week or more, just return days
            if (days >= 7) {
                return days + " days";
            }

            // otherwise try a few pairs of units
            final String daysHours = durationPairToString(days, "days", hours, "hours");
            if (daysHours != null) return daysHours;
            final String hoursMinutes = durationPairToString(hours, "hours", minutes, "minutes");
            if (hoursMinutes != null) return hoursMinutes;

            // either minutes, or less than a minute
            if (minutes > 0) {
                return minutes + " minutes";
            } else {
                return "less than a minute";
            }
        }

        private String wearChangeToString(WearEstimateChange wearEstimateChange) {
            final int oldLevel = Math.max(wearEstimateChange.oldEstimate.typeA,
                wearEstimateChange.oldEstimate.typeB);
            final int newLevel = Math.max(wearEstimateChange.newEstimate.typeA,
                wearEstimateChange.newEstimate.typeB);

            final String oldLevelString = wearLevelToString(oldLevel);
            final String newLevelString = wearLevelToString(newLevel);

            final DateTimeFormatter formatter =
                DateTimeFormatter.ofLocalizedDateTime(FormatStyle.SHORT).withZone(
                    ZoneId.systemDefault()).withLocale(Locale.getDefault());

            final String wallClockTimeAtChange = formatter.format(wearEstimateChange.dateAtChange);
            final String uptimeAtChange = durationToString(
                Duration.ofMillis(wearEstimateChange.uptimeAtChange));

            return String.format(
                "Wear level went from %s to %s.\nThe vehicle has been running for %s.\nWall clock time: %s",
                oldLevelString,
                newLevelString,
                uptimeAtChange,
                wallClockTimeAtChange);
        }

        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            Log.d(TAG, "Connected to " + name.flattenToString());

            try {
                CarStorageMonitoringManager storageMonitoringManager =
                    (CarStorageMonitoringManager) mCar.getCarManager(
                        Car.STORAGE_MONITORING_SERVICE);
                Log.d(TAG, "Acquired a CarStorageMonitoringManager " + storageMonitoringManager);
                List<WearEstimateChange> wearEstimateChanges = storageMonitoringManager
                    .getWearEstimateHistory();
                if (wearEstimateChanges.isEmpty()) {
                    finish();
                }

                WearEstimateChange currentChange = wearEstimateChanges
                    .get(wearEstimateChanges.size() - 1);

                if (!DEBUG && currentChange.isAcceptableDegradation) {
                    finish();
                }

                    mNotificationTextView.setText(wearChangeToString(currentChange));
            } catch (CarNotConnectedException e) {
                Log.e(TAG, "Failed to get a connection", e);
            }
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {
            Log.d(TAG, "Disconnected from " + name.flattenToString());

            mCar = null;
        }
    };

    private final Handler mHandler = new Handler();

    private Car mCar = null;
    private TextView mNotificationTextView = null;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        mNotificationTextView = findViewById(R.id.notification);

        mCar = Car.createCar(this, mCarConnectionListener);
        mCar.connect();

        mHandler.postDelayed(this::finish, 30000);
    }

    @Override
    protected void onDestroy() {
        if (mCar != null) {
            mCar.disconnect();
        }
        super.onDestroy();
    }
}
