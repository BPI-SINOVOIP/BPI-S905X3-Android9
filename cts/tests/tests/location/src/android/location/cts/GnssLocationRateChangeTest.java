/*
 * Copyright (C) 2017 Google Inc.
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

package android.location.cts;

import android.platform.test.annotations.AppModeFull;

/**
 * Test the "gps" location output works through various rate changes
 *
 * Tests:
 * 1. Toggle through various rates.
 * 2. Mix toggling through various rates with start & stop.
 *
 * Inspired by bugs 65246279, 65425110
 */

public class GnssLocationRateChangeTest extends GnssTestCase {

    private static final String TAG = "GnssLocationRateChangeTest";
    private static final int LOCATION_TO_COLLECT_COUNT = 1;

    private TestLocationListener mLocationListenerMain;
    private TestLocationListener mLocationListenerAfterRateChanges;
    // Various rates, where underlying GNSS hardware states may enter different modes
    private static final int[] TBF_MSEC = {0, 4_000, 250_000, 6_000_000, 10, 1_000, 16_000, 64_000};
    private static final int LOOPS_FOR_STRESS_TEST = 20;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mTestLocationManager = new TestLocationManager(getContext());
        // Using separate listeners, so the await trigger for the after-rate-changes listener is
        // independent of any possible locations that flow during setup, and rate change stress
        // testing
        mLocationListenerMain = new TestLocationListener(LOCATION_TO_COLLECT_COUNT);
        mLocationListenerAfterRateChanges = new TestLocationListener(LOCATION_TO_COLLECT_COUNT);
    }

    @Override
    protected void tearDown() throws Exception {
        // Unregister listeners
        if (mLocationListenerMain != null) {
            mTestLocationManager.removeLocationUpdates(mLocationListenerMain);
        }
        if (mLocationListenerAfterRateChanges != null) {
            mTestLocationManager.removeLocationUpdates(mLocationListenerAfterRateChanges);
        }
        super.tearDown();
    }

    /**
     * Requests (GPS) Locations at various rates that may stress the underlying GNSS software
     * and firmware state machine layers, ensuring Location output
     * remains responsive after all is done.
     */
    @AppModeFull(reason = "Flaky in instant mode.")
    public void testVariedRates() throws Exception {
        if (!TestUtils.deviceHasGpsFeature(getContext())) {
            return;
        }

        SoftAssert softAssert = new SoftAssert(TAG);
        mTestLocationManager.requestLocationUpdates(mLocationListenerMain);
        softAssert.assertTrue("Location should be received at test start",
                mLocationListenerMain.await());

        for (int timeBetweenLocationsMsec : TBF_MSEC) {
            // Rapidly change rates requested, to ensure GNSS provider state changes can handle this
            mTestLocationManager.requestLocationUpdates(mLocationListenerMain,
                    timeBetweenLocationsMsec);
        }
        mTestLocationManager.removeLocationUpdates(mLocationListenerMain);

        mTestLocationManager.requestLocationUpdates(mLocationListenerAfterRateChanges);
        softAssert.assertTrue("Location should be received at test end",
                mLocationListenerAfterRateChanges.await());

        softAssert.assertAll();
    }

    /**
     * Requests (GPS) Locations at various rates, and toggles between requests and removals,
     * that may stress the underlying GNSS software
     * and firmware state machine layers, ensuring Location output remains responsive
     * after all is done.
     */
    public void testVariedRatesOnOff() throws Exception {
        if (!TestUtils.deviceHasGpsFeature(getContext())) {
            return;
        }

        SoftAssert softAssert = new SoftAssert(TAG);
        mTestLocationManager.requestLocationUpdates(mLocationListenerMain);
        softAssert.assertTrue("Location should be received at test start",
                mLocationListenerMain.await());

        for (int timeBetweenLocationsMsec : TBF_MSEC) {
            // Rapidly change rates requested, to ensure GNSS provider state changes can handle this
            mTestLocationManager.requestLocationUpdates(mLocationListenerMain,
                    timeBetweenLocationsMsec);
            // Also flip the requests on and off quickly
            mTestLocationManager.removeLocationUpdates(mLocationListenerMain);
        }

        mTestLocationManager.requestLocationUpdates(mLocationListenerAfterRateChanges);
        softAssert.assertTrue("Location should be received at test end",
                mLocationListenerAfterRateChanges.await());

        softAssert.assertAll();
    }

    /**
     * Requests (GPS) Locations at various rates, and toggles between requests and removals,
     * in multiple loops to provide additional stress to the underlying GNSS software
     * and firmware state machine layers, ensuring Location output remains responsive
     * after all is done.
     */
    public void testVariedRatesRepetitive() throws Exception {
        if (!TestUtils.deviceHasGpsFeature(getContext())) {
            return;
        }

        SoftAssert softAssert = new SoftAssert(TAG);
        mTestLocationManager.requestLocationUpdates(mLocationListenerMain);
        softAssert.assertTrue("Location should be received at test start",
                mLocationListenerMain.await());

        // Two loops, first without removes, then with removes
        for (int i = 0; i < LOOPS_FOR_STRESS_TEST; i++) {
            for (int timeBetweenLocationsMsec : TBF_MSEC) {
               mTestLocationManager.requestLocationUpdates(mLocationListenerMain,
                        timeBetweenLocationsMsec);
            }
        }
        for (int i = 0; i < LOOPS_FOR_STRESS_TEST; i++) {
            for (int timeBetweenLocationsMsec : TBF_MSEC) {
                mTestLocationManager.requestLocationUpdates(mLocationListenerMain,
                        timeBetweenLocationsMsec);
                // Also flip the requests on and off quickly
                mTestLocationManager.removeLocationUpdates(mLocationListenerMain);
            }
        }

        mTestLocationManager.requestLocationUpdates(mLocationListenerAfterRateChanges);
        softAssert.assertTrue("Location should be received at test end",
                mLocationListenerAfterRateChanges.await());

        softAssert.assertAll();
    }
}
