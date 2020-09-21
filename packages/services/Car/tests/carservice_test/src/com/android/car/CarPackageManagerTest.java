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
package com.android.car;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import android.car.Car;
import android.car.CarNotConnectedException;
import android.car.content.pm.AppBlockingPackageInfo;
import android.car.content.pm.CarAppBlockingPolicy;
import android.car.content.pm.CarPackageManager;
import android.support.test.filters.SmallTest;
import android.support.test.runner.AndroidJUnit4;
import android.util.Log;

import com.android.car.pm.CarPackageManagerService;

import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
@SmallTest
public class CarPackageManagerTest extends MockedCarTestBase {
    private static final String TAG = CarPackageManagerTest.class.getSimpleName();

    private static final int POLLING_MAX_RETRY = 10;
    private static final long POLLING_SLEEP = 100;

    private CarPackageManager mCarPm;
    private CarPackageManagerService mCarPmService;

    private void init(boolean policyFromService) throws Exception {
        Log.i(TAG, "init started");
        TestAppBlockingPolicyService.controlPolicySettingFromService(policyFromService);
        mCarPm = (CarPackageManager) getCar().getCarManager(Car.PACKAGE_SERVICE);
        assertNotNull(mCarPm);
        mCarPmService = getPackageManagerService();
        assertNotNull(mCarPmService);
        mCarPmService.startAppBlockingPolicies();
    }

    @Test
    public void testServiceLaunched() throws Exception {
        init(true);
        Log.i(TAG, "testServiceLaunched, init called");
        assertTrue(pollingCheck(new PollingChecker() {
            @Override
            public boolean check() {
                Log.i(TAG, "checking instance ...");
                return TestAppBlockingPolicyService.getInstance() != null;
            }
        }, POLLING_MAX_RETRY, POLLING_SLEEP));
        final String thisPackage = getContext().getPackageName();
        final String serviceClassName = "DOES_NOT_MATTER";
        assertTrue(pollingCheck(new PollingChecker() {
            @Override
            public boolean check() {
                try {
                    return mCarPm.isServiceDistractionOptimized(thisPackage, serviceClassName);
                } catch (CarNotConnectedException e) {
                    return false;
                }
            }
        }, POLLING_MAX_RETRY, POLLING_SLEEP));
        assertTrue(mCarPm.isServiceDistractionOptimized(thisPackage, null));
        assertFalse(mCarPm.isServiceDistractionOptimized(serviceClassName, serviceClassName));
        assertFalse(mCarPm.isServiceDistractionOptimized(serviceClassName, null));
    }

    @Test
    public void testSettingWhitelist() throws Exception {
        init(false);
        final String carServicePackageName = "com.android.car";
        final String activityAllowed = "NO_SUCH_ACTIVITY_BUT_ALLOWED";
        final String activityNotAllowed = "NO_SUCH_ACTIVITY_AND_NOT_ALLOWED";
        final String acticityAllowed2 = "NO_SUCH_ACTIVITY_BUT_ALLOWED2";
        final String thisPackage = getContext().getPackageName();

        AppBlockingPackageInfo info = new AppBlockingPackageInfo(carServicePackageName, 0, 0,
                AppBlockingPackageInfo.FLAG_SYSTEM_APP, null, new String[] { activityAllowed });
        CarAppBlockingPolicy policy = new CarAppBlockingPolicy(new AppBlockingPackageInfo[] { info }
                , null);
        Log.i(TAG, "setting policy");
        mCarPm.setAppBlockingPolicy(thisPackage, policy,
                CarPackageManager.FLAG_SET_POLICY_WAIT_FOR_CHANGE);
        Log.i(TAG, "setting policy done");
        assertTrue(mCarPm.isActivityDistractionOptimized(carServicePackageName, activityAllowed));
        assertFalse(mCarPm.isActivityDistractionOptimized(carServicePackageName,
                activityNotAllowed));

        // replace policy
        info = new AppBlockingPackageInfo(carServicePackageName, 0, 0,
                AppBlockingPackageInfo.FLAG_SYSTEM_APP, null, new String[] { acticityAllowed2 });
        policy = new CarAppBlockingPolicy(new AppBlockingPackageInfo[] { info }
                , null);
        mCarPm.setAppBlockingPolicy(thisPackage, policy,
                CarPackageManager.FLAG_SET_POLICY_WAIT_FOR_CHANGE);
        assertFalse(mCarPm.isActivityDistractionOptimized(carServicePackageName, activityAllowed));
        assertTrue(mCarPm.isActivityDistractionOptimized(carServicePackageName, acticityAllowed2));
        assertFalse(mCarPm.isActivityDistractionOptimized(carServicePackageName,
                activityNotAllowed));

        //add, it replace the whole package policy. So activities are not added.
        info = new AppBlockingPackageInfo(carServicePackageName, 0, 0,
                AppBlockingPackageInfo.FLAG_SYSTEM_APP, null, new String[] { activityAllowed });
        policy = new CarAppBlockingPolicy(new AppBlockingPackageInfo[] { info }
                , null);
        mCarPm.setAppBlockingPolicy(thisPackage, policy,
                CarPackageManager.FLAG_SET_POLICY_WAIT_FOR_CHANGE |
                CarPackageManager.FLAG_SET_POLICY_ADD);
        assertTrue(mCarPm.isActivityDistractionOptimized(carServicePackageName, activityAllowed));
        assertFalse(mCarPm.isActivityDistractionOptimized(carServicePackageName, acticityAllowed2));
        assertFalse(mCarPm.isActivityDistractionOptimized(carServicePackageName,
                activityNotAllowed));

        //remove
        info = new AppBlockingPackageInfo(carServicePackageName, 0, 0,
                AppBlockingPackageInfo.FLAG_SYSTEM_APP, null, new String[] { activityAllowed });
        policy = new CarAppBlockingPolicy(new AppBlockingPackageInfo[] { info }
                , null);
        mCarPm.setAppBlockingPolicy(thisPackage, policy,
                CarPackageManager.FLAG_SET_POLICY_WAIT_FOR_CHANGE |
                CarPackageManager.FLAG_SET_POLICY_REMOVE);
        assertFalse(mCarPm.isActivityDistractionOptimized(carServicePackageName, activityAllowed));
        assertFalse(mCarPm.isActivityDistractionOptimized(carServicePackageName, acticityAllowed2));
        assertFalse(mCarPm.isActivityDistractionOptimized(carServicePackageName,
                activityNotAllowed));
    }

    interface PollingChecker {
        boolean check();
    }

    static boolean pollingCheck(PollingChecker checker, int maxRetry, long sleepMs)
            throws Exception {
        int retry = 0;
        boolean checked = checker.check();
        while (!checked && (retry < maxRetry)) {
            Thread.sleep(sleepMs);
            retry++;
            checked = checker.check();
        }
        return checked;
    }
}
