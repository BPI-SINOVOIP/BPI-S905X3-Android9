/**
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 */
package android.content.pm.cts;

import android.app.ActivityManager;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.FeatureGroupInfo;
import android.content.pm.FeatureInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.os.BatteryManager;
import android.os.Environment;
import android.test.AndroidTestCase;
import android.util.DisplayMetrics;
import android.util.Log;

import android.view.WindowManager;
import java.util.Arrays;
import java.util.Comparator;

public class FeatureTest extends AndroidTestCase {

    private static final String TAG = "FeatureTest";

    private PackageManager mPackageManager;
    private ActivityManager mActivityManager;
    private WindowManager mWindowManager;
    private boolean mSupportsDeviceAdmin;
    private boolean mSupportsManagedProfiles;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mPackageManager = getContext().getPackageManager();
        mActivityManager = (ActivityManager)getContext().getSystemService(Context.ACTIVITY_SERVICE);
        mWindowManager = (WindowManager) getContext().getSystemService(Context.WINDOW_SERVICE);
        mSupportsDeviceAdmin =
                mPackageManager.hasSystemFeature(PackageManager.FEATURE_DEVICE_ADMIN);
        mSupportsManagedProfiles =
                mPackageManager.hasSystemFeature(PackageManager.FEATURE_MANAGED_USERS);
    }

    public void testNoManagedUsersIfLowRamDevice() {
        if (mPackageManager == null || mActivityManager == null) {
            Log.w(TAG, "Skipping testNoManagedUsersIfLowRamDevice");
            return;
        }
        if (mActivityManager.isLowRamDevice()) {
            assertFalse(mSupportsManagedProfiles);
        }
    }

    /**
     * Test whether device supports managed profiles as required by CDD
     */
    public void testManagedProfileSupported() throws Exception {
        // Managed profiles only required if device admin feature is supported
        if (!mSupportsDeviceAdmin) {
            Log.w(TAG, "Skipping testManagedProfileSupported");
            return;
        }

        if (mSupportsManagedProfiles) {
            // Managed profiles supported nothing to check.
            return;
        }

        // Managed profiles only required for handheld devices
        if (!isHandheldDevice()) {
            return;
        }

        // Skip the tests for non-emulated sdcard
        if (!Environment.isExternalStorageEmulated()) {
            return;
        }

        // Skip the tests for low-RAM devices
        if (mActivityManager.isLowRamDevice()) {
            return;
        }

        fail("Device should support managed profiles, but "
                + PackageManager.FEATURE_MANAGED_USERS + " is not enabled");
    }

    /**
     * The CDD defines a handheld device as one that has a battery and a screen size between
     * 2.5 and 8 inches.
     */
    private boolean isHandheldDevice() throws Exception {
        double screenInches = getScreenSizeInInches();
        return deviceHasBattery() && screenInches >= 2.5 && screenInches <= 8.0;
    }

    private boolean deviceHasBattery() {
        final Intent batteryInfo = getContext().registerReceiver(null,
                new IntentFilter(Intent.ACTION_BATTERY_CHANGED));
        return batteryInfo.getBooleanExtra(BatteryManager.EXTRA_PRESENT, true);
    }

    private double getScreenSizeInInches() {
        DisplayMetrics dm = new DisplayMetrics();
        mWindowManager.getDefaultDisplay().getMetrics(dm);
        double widthInInchesSquared = Math.pow(dm.widthPixels/dm.xdpi,2);
        double heightInInchesSquared = Math.pow(dm.heightPixels/dm.ydpi,2);
        return Math.sqrt(widthInInchesSquared + heightInInchesSquared);
    }
}
