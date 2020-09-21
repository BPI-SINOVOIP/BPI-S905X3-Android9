/*
 * Copyright (C) 2009 The Android Open Source Project
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

package android.hardware.cts;

import android.hardware.GeomagneticField;
import android.platform.test.annotations.Presubmit;
import android.test.AndroidTestCase;

import java.util.GregorianCalendar;

public class GeomagneticFieldTest extends AndroidTestCase {
    private static final float DECLINATION_THRESHOLD = 0.1f;
    private static final float INCLINATION_THRESHOLD = 0.1f;
    private static final float FIELD_STRENGTH_THRESHOLD = 100;

    @Presubmit
    public void testGeomagneticField() {
        // Reference values calculated from NOAA online calculator for WMM 2015
        // https://www.ngdc.noaa.gov/geomag-web/#igrfwmm
        TestDataPoint testPoints[] = new TestDataPoint[] {
            // Mountain View, CA, USA on 2017/1/1
            new TestDataPoint(37.386f, -122.083f, 32, 2017, 1, 1,
                    13.4589f, 60.9542f, 48168.0f),
            // Chengdu, China on 2017/8/8
            new TestDataPoint(30.658f, 103.935f, 500f, 2017, 8, 8,
                    -1.9784f, 47.9723f, 50717.3f),
            // Sao Paulo, Brazil on 2018/12/25
            new TestDataPoint(-23.682f, -46.875f, 760f, 2018, 12, 25,
                    -21.3130f, -37.9940f, 22832.3f),
            // Boston, MA, USA on 2019/2/10
            new TestDataPoint(42.313f, -71.127f, 43f, 2019, 2, 10,
                    -14.5391f, 66.9693f, 51815.1f),
            // Cape Town, South Africa on 2019/5/1
            new TestDataPoint(-33.913f, 18.095f, 100f, 2019, 5, 1,
                    -25.2454f, -65.8887f, 25369.2f),
            // Sydney, Australia on 2020/1/1
            new TestDataPoint(-33.847f, 150.791f, 19f, 2020, 1, 1,
                    12.4469f, -64.3443f, 57087.9f)
        };

        for (TestDataPoint t : testPoints) {
            GeomagneticField field =
                    new GeomagneticField(t.latitude, t.longitude, t.altitude, t.epochTimeMillis);
            assertEquals(t.declinationDegree,  field.getDeclination(), DECLINATION_THRESHOLD);
            assertEquals(t.inclinationDegree,  field.getInclination(), INCLINATION_THRESHOLD);
            assertEquals(t.fieldStrengthNanoTesla, field.getFieldStrength(),
                    FIELD_STRENGTH_THRESHOLD);

            float horizontalFieldStrengthNanoTesla = (float)(
                    Math.cos(Math.toRadians(t.inclinationDegree)) * t.fieldStrengthNanoTesla);
            assertEquals(horizontalFieldStrengthNanoTesla, field.getHorizontalStrength(),
                    FIELD_STRENGTH_THRESHOLD);

            float verticalFieldStrengthNanoTesla = (float)(
                    Math.sin(Math.toRadians(t.inclinationDegree)) * t.fieldStrengthNanoTesla);
            assertEquals(verticalFieldStrengthNanoTesla, field.getZ(), FIELD_STRENGTH_THRESHOLD);

            float declinationDegree = (float)(
                    Math.toDegrees(Math.atan2(field.getY(), field.getX())));
            assertEquals(t.declinationDegree, declinationDegree, DECLINATION_THRESHOLD);
            assertEquals(horizontalFieldStrengthNanoTesla,
                    Math.sqrt(field.getX() * field.getX() + field.getY() * field.getY()),
                    FIELD_STRENGTH_THRESHOLD);
        }
    }

    private class TestDataPoint {
        public final float latitude;
        public final float longitude;
        public final float altitude;
        public final long epochTimeMillis;
        public final float declinationDegree;
        public final float inclinationDegree;
        public final float fieldStrengthNanoTesla;

        TestDataPoint(float lat, float lng, float alt, int year, int month, int day,
                float dec, float inc, float strength) {
            latitude = lat;
            longitude = lng;
            altitude = alt;
            epochTimeMillis = new GregorianCalendar(year, month, day).getTimeInMillis();
            declinationDegree = dec;
            inclinationDegree = inc;
            fieldStrengthNanoTesla = strength;
        }
    }
}
