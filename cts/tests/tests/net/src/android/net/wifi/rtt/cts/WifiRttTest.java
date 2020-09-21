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

package android.net.wifi.rtt.cts;

import android.content.IntentFilter;
import android.net.wifi.ScanResult;
import android.net.wifi.rtt.RangingRequest;
import android.net.wifi.rtt.RangingResult;
import android.net.wifi.rtt.WifiRttManager;

import com.android.compatibility.common.util.DeviceReportLog;
import com.android.compatibility.common.util.ResultType;
import com.android.compatibility.common.util.ResultUnit;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * Wi-Fi RTT CTS test: range to all available Access Points which support IEEE 802.11mc.
 */
public class WifiRttTest extends TestBase {
    // Number of scans to do while searching for APs supporting IEEE 802.11mc
    private static final int NUM_SCANS_SEARCHING_FOR_IEEE80211MC_AP = 2;

    // Number of RTT measurements per AP
    private static final int NUM_OF_RTT_ITERATIONS = 10;

    // Maximum failure rate of RTT measurements (percentage)
    private static final int MAX_FAILURE_RATE_PERCENT = 10;

    // Maximum variation from the average measurement (measures consistency)
    private static final int MAX_VARIATION_FROM_AVERAGE_DISTANCE_MM = 1000;

    // Minimum valid RSSI value
    private static final int MIN_VALID_RSSI = -100;

    /**
     * Test Wi-Fi RTT ranging operation:
     * - Scan for visible APs for the test AP (which is validated to support IEEE 802.11mc)
     * - Perform N (constant) RTT operations
     * - Validate:
     *   - Failure ratio < threshold (constant)
     *   - Result margin < threshold (constant)
     */
    public void testRangingToTestAp() throws InterruptedException {
        if (!shouldTestWifiRtt(getContext())) {
            return;
        }

        // Scan for IEEE 802.11mc supporting APs
        ScanResult testAp = scanForTestAp(NUM_SCANS_SEARCHING_FOR_IEEE80211MC_AP);
        assertTrue("Cannot find test AP", testAp != null);

        // Perform RTT operations
        RangingRequest request = new RangingRequest.Builder().addAccessPoint(testAp).build();
        List<RangingResult> allResults = new ArrayList<>();
        int numFailures = 0;
        int distanceSum = 0;
        int distanceMin = 0;
        int distanceMax = 0;
        int[] statuses = new int[NUM_OF_RTT_ITERATIONS];
        int[] distanceMms = new int[NUM_OF_RTT_ITERATIONS];
        int[] distanceStdDevMms = new int[NUM_OF_RTT_ITERATIONS];
        int[] rssis = new int[NUM_OF_RTT_ITERATIONS];
        int[] numAttempted = new int[NUM_OF_RTT_ITERATIONS];
        int[] numSuccessful = new int[NUM_OF_RTT_ITERATIONS];
        long[] timestampsMs = new long[NUM_OF_RTT_ITERATIONS];
        byte[] lastLci = null;
        byte[] lastLcr = null;
        for (int i = 0; i < NUM_OF_RTT_ITERATIONS; ++i) {
            ResultCallback callback = new ResultCallback();
            mWifiRttManager.startRanging(request, mExecutor, callback);
            assertTrue("Wi-Fi RTT results: no callback on iteration " + i,
                    callback.waitForCallback());

            List<RangingResult> currentResults = callback.getResults();
            assertTrue("Wi-Fi RTT results: null results (onRangingFailure) on iteration " + i,
                    currentResults != null);
            assertTrue("Wi-Fi RTT results: unexpected # of results (expect 1) on iteration " + i,
                    currentResults.size() == 1);
            RangingResult result = currentResults.get(0);
            assertTrue("Wi-Fi RTT results: invalid result (wrong BSSID) entry on iteration " + i,
                    result.getMacAddress().toString().equals(testAp.BSSID));
            assertEquals(
                    "Wi-Fi RTT results: invalid result (non-null PeerHandle) entry on iteration "
                            + i, null, result.getPeerHandle());

            allResults.add(result);
            int status = result.getStatus();
            statuses[i] = status;
            if (status == RangingResult.STATUS_SUCCESS) {
                distanceSum += result.getDistanceMm();
                if (i == 0) {
                    distanceMin = result.getDistanceMm();
                    distanceMax = result.getDistanceMm();
                } else {
                    distanceMin = Math.min(distanceMin, result.getDistanceMm());
                    distanceMax = Math.max(distanceMax, result.getDistanceMm());
                }

                assertTrue("Wi-Fi RTT results: invalid RSSI on iteration " + i,
                        result.getRssi() >= MIN_VALID_RSSI);

                distanceMms[i - numFailures] = result.getDistanceMm();
                distanceStdDevMms[i - numFailures] = result.getDistanceStdDevMm();
                rssis[i - numFailures] = result.getRssi();
                numAttempted[i - numFailures] = result.getNumAttemptedMeasurements();
                numSuccessful[i - numFailures] = result.getNumSuccessfulMeasurements();
                timestampsMs[i - numFailures] = result.getRangingTimestampMillis();

                byte[] currentLci = result.getLci();
                byte[] currentLcr = result.getLcr();
                if (i - numFailures > 0) {
                    assertTrue("Wi-Fi RTT results: invalid result (LCI mismatch) on iteration " + i,
                            Arrays.equals(currentLci, lastLci));
                    assertTrue("Wi-Fi RTT results: invalid result (LCR mismatch) on iteration " + i,
                            Arrays.equals(currentLcr, lastLcr));
                }
                lastLci = currentLci;
                lastLcr = currentLcr;
            } else {
                numFailures++;
            }
        }

        // Save results to log
        int numGoodResults = NUM_OF_RTT_ITERATIONS - numFailures;
        DeviceReportLog reportLog = new DeviceReportLog(TAG, "testRangingToTestAp");
        reportLog.addValues("status_codes", statuses, ResultType.NEUTRAL, ResultUnit.NONE);
        reportLog.addValues("distance_mm", Arrays.copyOf(distanceMms, numGoodResults),
                ResultType.NEUTRAL, ResultUnit.NONE);
        reportLog.addValues("distance_stddev_mm", Arrays.copyOf(distanceStdDevMms, numGoodResults),
                ResultType.NEUTRAL, ResultUnit.NONE);
        reportLog.addValues("rssi_dbm", Arrays.copyOf(rssis, numGoodResults), ResultType.NEUTRAL,
                ResultUnit.NONE);
        reportLog.addValues("num_attempted", Arrays.copyOf(numAttempted, numGoodResults),
                ResultType.NEUTRAL, ResultUnit.NONE);
        reportLog.addValues("num_successful", Arrays.copyOf(numSuccessful, numGoodResults),
                ResultType.NEUTRAL, ResultUnit.NONE);
        reportLog.addValues("timestamps", Arrays.copyOf(timestampsMs, numGoodResults),
                ResultType.NEUTRAL, ResultUnit.NONE);
        reportLog.submit();

        // Analyze results
        assertTrue("Wi-Fi RTT failure rate exceeds threshold",
                numFailures <= NUM_OF_RTT_ITERATIONS * MAX_FAILURE_RATE_PERCENT / 100);
        if (numFailures != NUM_OF_RTT_ITERATIONS) {
            double distanceAvg = distanceSum / (NUM_OF_RTT_ITERATIONS - numFailures);
            assertTrue("Wi-Fi RTT: Variation (max direction) exceeds threshold",
                    (distanceMax - distanceAvg) <= MAX_VARIATION_FROM_AVERAGE_DISTANCE_MM);
            assertTrue("Wi-Fi RTT: Variation (min direction) exceeds threshold",
                    (distanceAvg - distanceMin) <= MAX_VARIATION_FROM_AVERAGE_DISTANCE_MM);
            for (int i = 0; i < numGoodResults; ++i) {
                assertNotSame("Number of attempted measurements is 0", 0, numAttempted[i]);
                assertNotSame("Number of successful measurements is 0", 0, numSuccessful[i]);
            }
        }
    }

    /**
     * Validate that when a request contains more range operations than allowed (by API) that we
     * get an exception.
     */
    public void testRequestTooLarge() {
        if (!shouldTestWifiRtt(getContext())) {
            return;
        }

        ScanResult dummy = new ScanResult();
        dummy.BSSID = "00:01:02:03:04:05";

        RangingRequest.Builder builder = new RangingRequest.Builder();
        for (int i = 0; i < RangingRequest.getMaxPeers() - 2; ++i) {
            builder.addAccessPoint(dummy);
        }

        List<ScanResult> scanResults = new ArrayList<>();
        scanResults.add(dummy);
        scanResults.add(dummy);
        scanResults.add(dummy);

        builder.addAccessPoints(scanResults);

        try {
            mWifiRttManager.startRanging(builder.build(), mExecutor, new ResultCallback());
        } catch (IllegalArgumentException e) {
            return;
        }

        assertTrue(
                "Did not receive expected IllegalArgumentException when tried to range to too "
                        + "many peers",
                false);
    }
}
