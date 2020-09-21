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

import android.location.LocationManager;
import android.util.Log;

/**
 * Test the {@link LocationManager#getGnssYearOfHardware} and
 * {@link LocationManager#getGnssHardwareModelName} values.
 */
public class GnssHardwareInfoTest extends GnssTestCase {

  private static final String TAG = "GnssHardwareInfoTest";
  private static final int MIN_HARDWARE_YEAR = 2015;

  /**
   * Minimum plausible descriptive hardware model name length, e.g. "ABC1" for first GNSS version
   * ever shipped by ABC company.
   */
  private static final int MIN_HARDWARE_MODEL_NAME_LENGTH = 4;
  private static final int MIN_HARDWARE_YEAR_FOR_VALID_STRING = 2018;

  @Override
  protected void setUp() throws Exception {
    super.setUp();
    mTestLocationManager = new TestLocationManager(getContext());
  }

  /**
   * Verify hardware year is reported as 2015 or higher, with exception that in 2015, some
   * devies did not implement the underlying HAL API, and thus will report 0.
   */
  public void testHardwareYear() throws Exception {
    if (!TestUtils.deviceHasGpsFeature(getContext())) {
      return;
    }

    int gnssHardwareYear = mTestLocationManager.getLocationManager().getGnssYearOfHardware();
    // Allow 0 until Q, as older, upgrading devices may report 0.
    assertTrue("Hardware year must be 2015 or higher",
        gnssHardwareYear >= MIN_HARDWARE_YEAR || gnssHardwareYear == 0);
  }

  /**
   * Verify GNSS hardware model year is reported as a valid, descriptive value.
   * Descriptive is limited to a character count, and not the older values.
   */
  public void testHardwareModelName() throws Exception {
    if (!TestUtils.deviceHasGpsFeature(getContext())) {
      return;
    }

    int gnssHardwareYear = mTestLocationManager.getLocationManager().getGnssYearOfHardware();
    String gnssHardwareModelName =
        mTestLocationManager.getLocationManager().getGnssHardwareModelName();
    if (gnssHardwareYear >= MIN_HARDWARE_YEAR_FOR_VALID_STRING) {
      assertTrue("gnssHardwareModelName must not be null, when hardware year >= 2018",
          gnssHardwareModelName != null);
    }
    if (gnssHardwareModelName != null) {
      assertTrue("gnssHardwareModelName must be descriptive - at least 4 characters long",
          gnssHardwareModelName.length() >= MIN_HARDWARE_MODEL_NAME_LENGTH);
    }
  }
}
