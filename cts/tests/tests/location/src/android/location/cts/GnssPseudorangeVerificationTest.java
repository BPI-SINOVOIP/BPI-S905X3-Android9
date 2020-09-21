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

import android.location.cts.pseudorange.PseudorangePositionVelocityFromRealTimeEvents;
import android.location.GnssMeasurement;
import android.location.GnssMeasurementsEvent;
import android.location.GnssStatus;
import android.location.Location;
import android.platform.test.annotations.AppModeFull;
import android.util.Log;
import com.android.compatibility.common.util.CddTest;
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.concurrent.TimeUnit;
import java.util.HashMap;

/**
 * Test computing and verifying the pseudoranges based on the raw measurements
 * reported by the GNSS chipset
 */
public class GnssPseudorangeVerificationTest extends GnssTestCase {
  private static final String TAG = "GnssPseudorangeValTest";
  private static final int LOCATION_TO_COLLECT_COUNT = 5;
  private static final int MEASUREMENT_EVENTS_TO_COLLECT_COUNT = 10;
  private static final int MIN_SATELLITES_REQUIREMENT = 4;
  private static final double SECONDS_PER_NANO = 1.0e-9;

  // GPS/GLONASS: according to http://cdn.intechopen.com/pdfs-wm/27712.pdf, the pseudorange in time
  // is 65-83 ms, which is 18 ms range.
  // GLONASS: orbit is a bit closer than GPS, so we add 0.003ms to the range, hence deltaiSeconds
  // should be in the range of [0.0, 0.021] seconds.
  // QZSS and BEIDOU: they have higher orbit, which will result in a small svTime, the deltai can be
  // calculated as follows:
  // assume a = QZSS/BEIDOU orbit Semi-Major Axis(42,164km for QZSS);
  // b = GLONASS orbit Semi-Major Axis (25,508km);
  // c = Speed of light (299,792km/s);
  // e = earth radius (6,378km);
  // in the extremely case of QZSS is on the horizon and GLONASS is on the 90 degree top
  // max difference should be (sqrt(a^2-e^2) - (b-e))/c,
  // which is around 0.076s.
  private static final double PSEUDORANGE_THRESHOLD_IN_SEC = 0.021;
  // Geosync constellations have a longer range vs typical MEO orbits
  // that are the short end of the range.
  private static final double PSEUDORANGE_THRESHOLD_BEIDOU_QZSS_IN_SEC = 0.076;

  private static final float LOW_ENOUGH_POSITION_UNCERTAINTY_METERS = 100;
  private static final float LOW_ENOUGH_VELOCITY_UNCERTAINTY_MPS = 5;
  private static final float HORIZONTAL_OFFSET_FLOOR_METERS = 10;
  private static final float HORIZONTAL_OFFSET_SIGMA = 3;  // 3 * the ~68%ile level
  private static final float HORIZONTAL_OFFSET_FLOOR_MPS = 1;

  private TestGnssMeasurementListener mMeasurementListener;
  private TestLocationListener mLocationListener;

  @Override
  protected void setUp() throws Exception {
    super.setUp();

    mTestLocationManager = new TestLocationManager(getContext());
  }

  @Override
  protected void tearDown() throws Exception {
    // Unregister listeners
    if (mLocationListener != null) {
      mTestLocationManager.removeLocationUpdates(mLocationListener);
    }
    if (mMeasurementListener != null) {
      mTestLocationManager.unregisterGnssMeasurementCallback(mMeasurementListener);
    }
    super.tearDown();
  }

  /**
   * Tests that one can listen for {@link GnssMeasurementsEvent} for collection purposes.
   * It only performs sanity checks for the measurements received.
   * This tests uses actual data retrieved from Gnss HAL.
   */
  @CddTest(requirement="7.3.3")
  public void testPseudorangeValue() throws Exception {
    // Checks if Gnss hardware feature is present, skips test (pass) if not,
    // and hard asserts that Location/Gnss (Provider) is turned on if is Cts Verifier.
    // From android O, CTS tests should run in the lab with GPS signal.
    if (!TestMeasurementUtil.canTestRunOnCurrentDevice(mTestLocationManager,
        TAG, MIN_HARDWARE_YEAR_MEASUREMENTS_REQUIRED, true)) {
      return;
    }

    mLocationListener = new TestLocationListener(LOCATION_TO_COLLECT_COUNT);
    mTestLocationManager.requestLocationUpdates(mLocationListener);

    mMeasurementListener = new TestGnssMeasurementListener(TAG,
                                                MEASUREMENT_EVENTS_TO_COLLECT_COUNT, true);
    mTestLocationManager.registerGnssMeasurementCallback(mMeasurementListener);

    boolean success = mLocationListener.await();
    success &= mMeasurementListener.await();
    SoftAssert.failOrWarning(isMeasurementTestStrict(),
        "Time elapsed without getting enough location fixes."
            + " Possibly, the test has been run deep indoors."
            + " Consider retrying test outdoors.",
        success);

    Log.i(TAG, "Location status received = " + mLocationListener.isLocationReceived());

    if (!mMeasurementListener.verifyStatus(isMeasurementTestStrict())) {
      // If test is strict and verifyStatus reutrns false, an assert exception happens and
      // test fails.   If test is not strict, we arrive here, and:
      return; // exit (with pass)
    }

    List<GnssMeasurementsEvent> events = mMeasurementListener.getEvents();
    int eventCount = events.size();
    Log.i(TAG, "Number of GNSS measurement events received = " + eventCount);
    SoftAssert.failOrWarning(isMeasurementTestStrict(),
        "GnssMeasurementEvent count: expected > 0, received = " + eventCount,
        eventCount > 0);

    SoftAssert softAssert = new SoftAssert(TAG);

    boolean hasEventWithEnoughMeasurements = false;
    // we received events, so perform a quick sanity check on mandatory fields
    for (GnssMeasurementsEvent event : events) {
      // Verify Gnss Event mandatory fields are in required ranges
      assertNotNull("GnssMeasurementEvent cannot be null.", event);

      long timeInNs = event.getClock().getTimeNanos();
      TestMeasurementUtil.assertGnssClockFields(event.getClock(), softAssert, timeInNs);

      ArrayList<GnssMeasurement> filteredMeasurements = filterMeasurements(event.getMeasurements());
      HashMap<Integer, ArrayList<GnssMeasurement>> measurementConstellationMap =
          groupByConstellation(filteredMeasurements);
      for (ArrayList<GnssMeasurement> measurements : measurementConstellationMap.values()) {
        validatePseudorange(measurements, softAssert, timeInNs);
      }

      // we need at least 4 satellites to calculate the pseudorange
      if(event.getMeasurements().size() >= MIN_SATELLITES_REQUIREMENT) {
        hasEventWithEnoughMeasurements = true;
      }
    }

    SoftAssert.failOrWarning(isMeasurementTestStrict(),
        "Should have at least one GnssMeasurementEvent with at least 4"
            + "GnssMeasurement. If failed, retry near window or outdoors?",
        hasEventWithEnoughMeasurements);

    softAssert.assertAll();
  }

  private HashMap<Integer, ArrayList<GnssMeasurement>> groupByConstellation(
      Collection<GnssMeasurement> measurements) {
    HashMap<Integer, ArrayList<GnssMeasurement>> measurementConstellationMap = new HashMap<>();
    for (GnssMeasurement measurement: measurements){
      int constellationType = measurement.getConstellationType();
      if (!measurementConstellationMap.containsKey(constellationType)) {
        measurementConstellationMap.put(constellationType, new ArrayList<>());
      }
      measurementConstellationMap.get(constellationType).add(measurement);
    }
    return measurementConstellationMap;
  }

  private ArrayList<GnssMeasurement> filterMeasurements(Collection<GnssMeasurement> measurements) {
    ArrayList<GnssMeasurement> filteredMeasurement = new ArrayList<>();
    for (GnssMeasurement measurement: measurements){
      int constellationType = measurement.getConstellationType();
      if (constellationType == GnssStatus.CONSTELLATION_GLONASS) {
        if ((measurement.getState()
            & (measurement.STATE_GLO_TOD_DECODED | measurement.STATE_GLO_TOD_KNOWN)) != 0) {
          filteredMeasurement.add(measurement);
        }
      }
      else if ((measurement.getState()
            & (measurement.STATE_TOW_DECODED | measurement.STATE_TOW_KNOWN)) != 0) {
          filteredMeasurement.add(measurement);
        }
    }
    return filteredMeasurement;
  }

  /**
   * Uses the common reception time approach to calculate pseudorange time
   * measurements reported by the receiver according to http://cdn.intechopen.com/pdfs-wm/27712.pdf.
   */
  private void validatePseudorange(Collection<GnssMeasurement> measurements,
      SoftAssert softAssert, long timeInNs) {
    long largestReceivedSvTimeNanosTod = 0;
    // closest satellite has largest time (closest to now), as of nano secs of the day
    // so the largestReceivedSvTimeNanosTod will be the svTime we got from one of the GPS/GLONASS sv
    for(GnssMeasurement measurement : measurements) {
      long receivedSvTimeNanosTod =  measurement.getReceivedSvTimeNanos()
                                        % TimeUnit.DAYS.toNanos(1);
      if (largestReceivedSvTimeNanosTod < receivedSvTimeNanosTod) {
        largestReceivedSvTimeNanosTod = receivedSvTimeNanosTod;
      }
    }
    for (GnssMeasurement measurement : measurements) {
      double threshold = PSEUDORANGE_THRESHOLD_IN_SEC;
      int constellationType = measurement.getConstellationType();
      // BEIDOU and QZSS's Orbit are higher, so the value of ReceivedSvTimeNanos should be small
      if (constellationType == GnssStatus.CONSTELLATION_BEIDOU
          || constellationType == GnssStatus.CONSTELLATION_QZSS) {
        threshold = PSEUDORANGE_THRESHOLD_BEIDOU_QZSS_IN_SEC;
      }
      double deltaiNanos = largestReceivedSvTimeNanosTod
                          - (measurement.getReceivedSvTimeNanos() % TimeUnit.DAYS.toNanos(1));
      double deltaiSeconds = deltaiNanos * SECONDS_PER_NANO;

      softAssert.assertTrue("deltaiSeconds in Seconds.",
          timeInNs,
          "0.0 <= deltaiSeconds <= " + String.valueOf(threshold),
          String.valueOf(deltaiSeconds),
          (deltaiSeconds >= 0.0 && deltaiSeconds <= threshold));
    }
  }

    /*
     * Use pseudorange calculation library to calculate position then compare to location from
     * Location Manager.
     */
    @CddTest(requirement = "7.3.3")
    @AppModeFull(reason = "Flaky in instant mode")
    public void testPseudoPosition() throws Exception {
        // Checks if Gnss hardware feature is present, skips test (pass) if not,
        // and hard asserts that Location/Gnss (Provider) is turned on if is Cts Verifier.
        // From android O, CTS tests should run in the lab with GPS signal.
        if (!TestMeasurementUtil.canTestRunOnCurrentDevice(mTestLocationManager,
                TAG, MIN_HARDWARE_YEAR_MEASUREMENTS_REQUIRED, true)) {
            return;
        }

        mLocationListener = new TestLocationListener(LOCATION_TO_COLLECT_COUNT);
        mTestLocationManager.requestLocationUpdates(mLocationListener);

        mMeasurementListener = new TestGnssMeasurementListener(TAG,
                MEASUREMENT_EVENTS_TO_COLLECT_COUNT, true);
        mTestLocationManager.registerGnssMeasurementCallback(mMeasurementListener);

        boolean success = mLocationListener.await();

        List<Location> receivedLocationList = mLocationListener.getReceivedLocationList();
        assertTrue("Time elapsed without getting enough location fixes."
                        + " Possibly, the test has been run deep indoors."
                        + " Consider retrying test outdoors.",
                success && receivedLocationList.size() > 0);
        Location locationFromApi = receivedLocationList.get(0);

        // Since we are checking the eventCount later, there is no need to check the return value
        // here.
        mMeasurementListener.await();

        List<GnssMeasurementsEvent> events = mMeasurementListener.getEvents();
        int eventCount = events.size();
        Log.i(TAG, "Number of Gps Event received = " + eventCount);
        int gnssYearOfHardware = mTestLocationManager.getLocationManager().getGnssYearOfHardware();
        if (eventCount == 0 && gnssYearOfHardware < MIN_HARDWARE_YEAR_MEASUREMENTS_REQUIRED) {
            return;
        }

        Log.i(TAG, "This is a device from 2016 or later.");
        assertTrue("GnssMeasurementEvent count: expected > 0, received = " + eventCount,
                eventCount > 0);

        PseudorangePositionVelocityFromRealTimeEvents mPseudorangePositionFromRealTimeEvents
                = new PseudorangePositionVelocityFromRealTimeEvents();
        mPseudorangePositionFromRealTimeEvents.setReferencePosition(
                (int) (locationFromApi.getLatitude() * 1E7),
                (int) (locationFromApi.getLongitude() * 1E7),
                (int) (locationFromApi.getAltitude() * 1E7));

        Log.i(TAG, "Location from Location Manager"
                + ", Latitude:" + locationFromApi.getLatitude()
                + ", Longitude:" + locationFromApi.getLongitude()
                + ", Altitude:" + locationFromApi.getAltitude());

        // Ensure at least some calculated locations have a reasonably low uncertainty
        boolean someLocationsHaveLowPosUnc = false;
        boolean someLocationsHaveLowVelUnc = false;

        int totalCalculatedLocationCnt = 0;
        for (GnssMeasurementsEvent event : events) {
            // In mMeasurementListener.getEvents() we already filtered out events, at this point
            // every event will have at least 4 satellites in one constellation.
            mPseudorangePositionFromRealTimeEvents.computePositionVelocitySolutionsFromRawMeas(
                    event);
            double[] calculatedLatLngAlt =
                    mPseudorangePositionFromRealTimeEvents.getPositionSolutionLatLngDeg();
            // it will return NaN when there is no enough measurements to calculate the position
            if (Double.isNaN(calculatedLatLngAlt[0])) {
                continue;
            } else {
                totalCalculatedLocationCnt++;
                Log.i(TAG, "Calculated Location"
                        + ", Latitude:" + calculatedLatLngAlt[0]
                        + ", Longitude:" + calculatedLatLngAlt[1]
                        + ", Altitude:" + calculatedLatLngAlt[2]);

                double[] posVelUncertainties =
                        mPseudorangePositionFromRealTimeEvents.getPositionVelocityUncertaintyEnu();

                double horizontalPositionUncertaintyMeters =
                        Math.sqrt(posVelUncertainties[0] * posVelUncertainties[0]
                                + posVelUncertainties[1] * posVelUncertainties[1]);

                // Tolerate large offsets, when the device reports a large uncertainty - while also
                // ensuring (here) that some locations are produced before the test ends
                // with a reasonably low set of error estimates
                if (horizontalPositionUncertaintyMeters < LOW_ENOUGH_POSITION_UNCERTAINTY_METERS) {
                    someLocationsHaveLowPosUnc = true;
                }

                // Root-sum-sqaure the WLS, and device generated 68%ile accuracies is a conservative
                // 68%ile offset (given likely correlated errors) - then this is scaled by
                // initially 3 sigma to give a high enough tolerance to make the test tolerant
                // enough of noise to pass reliably.  Floor adds additional robustness in case of
                // small errors and small error estimates.
                double horizontalOffsetThresholdMeters = HORIZONTAL_OFFSET_SIGMA * Math.sqrt(
                        horizontalPositionUncertaintyMeters * horizontalPositionUncertaintyMeters
                                + locationFromApi.getAccuracy() * locationFromApi.getAccuracy())
                        + HORIZONTAL_OFFSET_FLOOR_METERS;

                Location calculatedLocation = new Location("gps");
                calculatedLocation.setLatitude(calculatedLatLngAlt[0]);
                calculatedLocation.setLongitude(calculatedLatLngAlt[1]);
                calculatedLocation.setAltitude(calculatedLatLngAlt[2]);

                double horizontalOffset = calculatedLocation.distanceTo(locationFromApi);

                Log.i(TAG, "Calculated Location Offset: " + horizontalOffset
                        + ", Threshold: " + horizontalOffsetThresholdMeters);
                assertTrue("Latitude & Longitude calculated from pseudoranges should be close to "
                                + "those reported from Location Manager.  Offset = "
                                + horizontalOffset + " meters. Threshold = "
                                + horizontalOffsetThresholdMeters + " meters ",
                        horizontalOffset < horizontalOffsetThresholdMeters);

                //TODO: Check for the altitude offset

                // This 2D velocity uncertainty is conservatively larger than speed uncertainty
                // as it also contains the effect of bearing uncertainty at a constant speed
                double horizontalVelocityUncertaintyMps =
                        Math.sqrt(posVelUncertainties[4] * posVelUncertainties[4]
                                + posVelUncertainties[5] * posVelUncertainties[5]);
                if (horizontalVelocityUncertaintyMps < LOW_ENOUGH_VELOCITY_UNCERTAINTY_MPS) {
                    someLocationsHaveLowVelUnc = true;
                }

                // Assume 1m/s uncertainty from API, for this test, if not provided
                float speedUncFromApiMps = locationFromApi.hasSpeedAccuracy()
                        ? locationFromApi.getSpeedAccuracyMetersPerSecond()
                        : HORIZONTAL_OFFSET_FLOOR_MPS;

                // Similar 3-standard deviation plus floor threshold as
                // horizontalOffsetThresholdMeters above
                double horizontalSpeedOffsetThresholdMps = HORIZONTAL_OFFSET_SIGMA * Math.sqrt(
                        horizontalVelocityUncertaintyMps * horizontalVelocityUncertaintyMps
                                + speedUncFromApiMps * speedUncFromApiMps)
                        + HORIZONTAL_OFFSET_FLOOR_MPS;

                double[] calculatedVelocityEnuMps =
                        mPseudorangePositionFromRealTimeEvents.getVelocitySolutionEnuMps();
                double calculatedHorizontalSpeedMps =
                        Math.sqrt(calculatedVelocityEnuMps[0] * calculatedVelocityEnuMps[0]
                                + calculatedVelocityEnuMps[1] * calculatedVelocityEnuMps[1]);

                Log.i(TAG, "Calculated Speed: " + calculatedHorizontalSpeedMps
                        + ", Reported Speed: " + locationFromApi.getSpeed()
                        + ", Threshold: " + horizontalSpeedOffsetThresholdMps);
                assertTrue("Speed (" + calculatedHorizontalSpeedMps + " m/s) calculated from"
                                + " pseudoranges should be close to the speed ("
                                + locationFromApi.getSpeed() + " m/s) reported from"
                                + " Location Manager.",
                        Math.abs(calculatedHorizontalSpeedMps - locationFromApi.getSpeed())
                                < horizontalSpeedOffsetThresholdMps);
            }
        }

        assertTrue("Calculated Location Count should be greater than 0.",
                totalCalculatedLocationCnt > 0);
        assertTrue("Calculated Horizontal Location Uncertainty should at least once be less than "
                        + LOW_ENOUGH_POSITION_UNCERTAINTY_METERS,
                someLocationsHaveLowPosUnc);
        assertTrue("Calculated Horizontal Velocity Uncertainty should at least once be less than "
                        + LOW_ENOUGH_VELOCITY_UNCERTAINTY_MPS,
                someLocationsHaveLowVelUnc);
    }
}
