/*
 * Copyright (C) 2008 The Android Open Source Project
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

import android.content.Context;
import android.content.Intent;
import android.location.Location;
import android.location.SettingInjectorService;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.Messenger;
import android.os.Bundle;
import android.os.Parcel;
import android.test.AndroidTestCase;
import android.util.Printer;
import android.util.StringBuilderPrinter;

import java.text.DecimalFormat;

public class LocationTest extends AndroidTestCase {
    private static final float DELTA = 0.1f;
    private final float TEST_ACCURACY = 1.0f;
    private final float TEST_VERTICAL_ACCURACY = 2.0f;
    private final float TEST_SPEED_ACCURACY = 3.0f;
    private final float TEST_BEARING_ACCURACY = 4.0f;
    private final double TEST_ALTITUDE = 1.0;
    private final double TEST_LATITUDE = 50;
    private final float TEST_BEARING = 1.0f;
    private final double TEST_LONGITUDE = 20;
    private final float TEST_SPEED = 5.0f;
    private final long TEST_TIME = 100;
    private final String TEST_PROVIDER = "LocationProvider";
    private final String TEST_KEY1NAME = "key1";
    private final String TEST_KEY2NAME = "key2";
    private final boolean TEST_KEY1VALUE = false;
    private final byte TEST_KEY2VALUE = 10;

    private static final String ENABLED_KEY = "enabled";
    private static final String MESSENGER_KEY = "messenger";


    public void testConstructor() {
        new Location("LocationProvider");

        Location l = createTestLocation();
        Location location = new Location(l);
        assertTestLocation(location);

        try {
            new Location((Location) null);
            fail("should throw NullPointerException");
        } catch (NullPointerException e) {
            // expected.
        }
    }

    public void testDump() {
        StringBuilder sb = new StringBuilder();
        StringBuilderPrinter printer = new StringBuilderPrinter(sb);
        Location location = new Location("LocationProvider");
        location.dump(printer, "");
        assertNotNull(sb.toString());
    }

    public void testBearingTo() {
        Location location = new Location("");
        Location dest = new Location("");

        // set the location to Beijing
        location.setLatitude(39.9);
        location.setLongitude(116.4);
        // set the destination to Chengdu
        dest.setLatitude(30.7);
        dest.setLongitude(104.1);
        assertEquals(-128.66, location.bearingTo(dest), DELTA);

        float bearing;
        Location zeroLocation = new Location("");
        zeroLocation.setLatitude(0);
        zeroLocation.setLongitude(0);

        Location testLocation = new Location("");
        testLocation.setLatitude(0);
        testLocation.setLongitude(150);

        bearing = zeroLocation.bearingTo(zeroLocation);
        assertEquals(0.0f, bearing, DELTA);

        bearing = zeroLocation.bearingTo(testLocation);
        assertEquals(90.0f, bearing, DELTA);

        testLocation.setLatitude(90);
        testLocation.setLongitude(0);
        bearing = zeroLocation.bearingTo(testLocation);
        assertEquals(0.0f, bearing, DELTA);

        try {
            location.bearingTo(null);
            fail("should throw NullPointerException");
        } catch (NullPointerException e) {
            // expected.
        }
    }

    public void testConvert_CoordinateToRepresentation() {
        DecimalFormat df = new DecimalFormat("###.#####");
        String result;

        result = Location.convert(-80.0, Location.FORMAT_DEGREES);
        assertEquals("-" + df.format(80.0), result);

        result = Location.convert(-80.085, Location.FORMAT_MINUTES);
        assertEquals("-80:" + df.format(5.1), result);

        result = Location.convert(-80, Location.FORMAT_MINUTES);
        assertEquals("-80:" + df.format(0), result);

        result = Location.convert(-80.075, Location.FORMAT_MINUTES);
        assertEquals("-80:" + df.format(4.5), result);

        result = Location.convert(-80.075, Location.FORMAT_DEGREES);
        assertEquals("-" + df.format(80.075), result);

        result = Location.convert(-80.075, Location.FORMAT_SECONDS);
        assertEquals("-80:4:30", result);

        try {
            Location.convert(-181, Location.FORMAT_SECONDS);
            fail("should throw IllegalArgumentException.");
        } catch (IllegalArgumentException e) {
            // expected.
        }

        try {
            Location.convert(181, Location.FORMAT_SECONDS);
            fail("should throw IllegalArgumentException.");
        } catch (IllegalArgumentException e) {
            // expected.
        }

        try {
            Location.convert(-80.075, -1);
            fail("should throw IllegalArgumentException.");
        } catch (IllegalArgumentException e) {
            // expected.
        }
    }

    public void testConvert_RepresentationToCoordinate() {
        double result;

        result = Location.convert("-80.075");
        assertEquals(-80.075, result);

        result = Location.convert("-80:05.10000");
        assertEquals(-80.085, result);

        result = Location.convert("-80:04:03.00000");
        assertEquals(-80.0675, result);

        result = Location.convert("-80:4:3");
        assertEquals(-80.0675, result);

        try {
            Location.convert(null);
            fail("should throw NullPointerException.");
        } catch (NullPointerException e){
            // expected.
        }

        try {
            Location.convert(":");
            fail("should throw IllegalArgumentException.");
        } catch (IllegalArgumentException e){
            // expected.
        }

        try {
            Location.convert("190:4:3");
            fail("should throw IllegalArgumentException.");
        } catch (IllegalArgumentException e){
            // expected.
        }

        try {
            Location.convert("-80:60:3");
            fail("should throw IllegalArgumentException.");
        } catch (IllegalArgumentException e){
            // expected.
        }

        try {
            Location.convert("-80:4:60");
            fail("should throw IllegalArgumentException.");
        } catch (IllegalArgumentException e){
            // expected.
        }
    }

    public void testDescribeContents() {
        Location location = new Location("");
        location.describeContents();
    }

    public void testDistanceBetween() {
        float[] result = new float[3];
        Location.distanceBetween(0, 0, 0, 0, result);
        assertEquals(0.0, result[0], DELTA);
        assertEquals(0.0, result[1], DELTA);
        assertEquals(0.0, result[2], DELTA);

        Location.distanceBetween(20, 30, -40, 140, result);
        assertEquals(1.3094936E7, result[0], 1);
        assertEquals(125.4538, result[1], DELTA);
        assertEquals(93.3971, result[2], DELTA);

        try {
            Location.distanceBetween(20, 30, -40, 140, null);
            fail("should throw IllegalArgumentException");
        } catch (IllegalArgumentException e) {
            // expected.
        }

        try {
            Location.distanceBetween(20, 30, -40, 140, new float[0]);
            fail("should throw IllegalArgumentException");
        } catch (IllegalArgumentException e) {
            // expected.
        }
    }

    public void testDistanceTo() {
        float distance;
        Location zeroLocation = new Location("");
        zeroLocation.setLatitude(0);
        zeroLocation.setLongitude(0);

        Location testLocation = new Location("");
        testLocation.setLatitude(30);
        testLocation.setLongitude(50);

        distance = zeroLocation.distanceTo(zeroLocation);
        assertEquals(0, distance, DELTA);

        distance = zeroLocation.distanceTo(testLocation);
        assertEquals(6244139.0, distance, 1);
    }

    public void testAccessAccuracy() {
        Location location = new Location("");
        assertFalse(location.hasAccuracy());

        location.setAccuracy(1.0f);
        assertEquals(1.0, location.getAccuracy(), DELTA);
        assertTrue(location.hasAccuracy());

        location.removeAccuracy();
        assertEquals(0.0, location.getAccuracy(), DELTA);
        assertFalse(location.hasAccuracy());
    }

    public void testAccessVerticalAccuracy() {
        Location location = new Location("");
        assertFalse(location.hasVerticalAccuracy());

        location.setVerticalAccuracyMeters(1.0f);
        assertEquals(1.0, location.getVerticalAccuracyMeters(), DELTA);
        assertTrue(location.hasVerticalAccuracy());
    }

    public void testAccessSpeedAccuracy() {
        Location location = new Location("");
        assertFalse(location.hasSpeedAccuracy());

        location.setSpeedAccuracyMetersPerSecond(1.0f);
        assertEquals(1.0, location.getSpeedAccuracyMetersPerSecond(), DELTA);
        assertTrue(location.hasSpeedAccuracy());
    }

    public void testAccessBearingAccuracy() {
        Location location = new Location("");
        assertFalse(location.hasBearingAccuracy());

        location.setBearingAccuracyDegrees(1.0f);
        assertEquals(1.0, location.getBearingAccuracyDegrees(), DELTA);
        assertTrue(location.hasBearingAccuracy());
    }


    public void testAccessAltitude() {
        Location location = new Location("");
        assertFalse(location.hasAltitude());

        location.setAltitude(1.0);
        assertEquals(1.0, location.getAltitude(), DELTA);
        assertTrue(location.hasAltitude());

        location.removeAltitude();
        assertEquals(0.0, location.getAltitude(), DELTA);
        assertFalse(location.hasAltitude());
    }

    public void testAccessBearing() {
        Location location = new Location("");
        assertFalse(location.hasBearing());

        location.setBearing(1.0f);
        assertEquals(1.0, location.getBearing(), DELTA);
        assertTrue(location.hasBearing());

        location.setBearing(371.0f);
        assertEquals(11.0, location.getBearing(), DELTA);
        assertTrue(location.hasBearing());

        location.setBearing(-361.0f);
        assertEquals(359.0, location.getBearing(), DELTA);
        assertTrue(location.hasBearing());

        location.removeBearing();
        assertEquals(0.0, location.getBearing(), DELTA);
        assertFalse(location.hasBearing());
    }

    public void testAccessExtras() {
        Location location = createTestLocation();

        assertTestBundle(location.getExtras());

        location.setExtras(null);
        assertNull(location.getExtras());
    }

    public void testAccessLatitude() {
        Location location = new Location("");

        location.setLatitude(0);
        assertEquals(0, location.getLatitude(), DELTA);

        location.setLatitude(90);
        assertEquals(90, location.getLatitude(), DELTA);

        location.setLatitude(-90);
        assertEquals(-90, location.getLatitude(), DELTA);
    }

    public void testAccessLongitude() {
        Location location = new Location("");

        location.setLongitude(0);
        assertEquals(0, location.getLongitude(), DELTA);

        location.setLongitude(180);
        assertEquals(180, location.getLongitude(), DELTA);

        location.setLongitude(-180);
        assertEquals(-180, location.getLongitude(), DELTA);
    }

    public void testAccessProvider() {
        Location location = new Location("");

        String provider = "Location Provider";
        location.setProvider(provider);
        assertEquals(provider, location.getProvider());

        location.setProvider(null);
        assertNull(location.getProvider());
    }

    public void testAccessSpeed() {
        Location location = new Location("");
        assertFalse(location.hasSpeed());

        location.setSpeed(234.0045f);
        assertEquals(234.0045, location.getSpeed(), DELTA);
        assertTrue(location.hasSpeed());

        location.removeSpeed();
        assertEquals(0.0, location.getSpeed(), DELTA);
        assertFalse(location.hasSpeed());
    }

    public void testAccessTime() {
        Location location = new Location("");

        location.setTime(0);
        assertEquals(0, location.getTime());

        location.setTime(Long.MAX_VALUE);
        assertEquals(Long.MAX_VALUE, location.getTime());

        location.setTime(12000);
        assertEquals(12000, location.getTime());
    }

    public void testAccessElapsedRealtime() {
        Location location = new Location("");

        location.setElapsedRealtimeNanos(0);
        assertEquals(0, location.getElapsedRealtimeNanos());

        location.setElapsedRealtimeNanos(Long.MAX_VALUE);
        assertEquals(Long.MAX_VALUE, location.getElapsedRealtimeNanos());

        location.setElapsedRealtimeNanos(12000);
        assertEquals(12000, location.getElapsedRealtimeNanos());
    }

    public void testSet() {
        Location location = new Location("");

        Location loc = createTestLocation();

        location.set(loc);
        assertTestLocation(location);

        location.reset();
        assertNull(location.getProvider());
        assertEquals(0, location.getTime());
        assertEquals(0, location.getLatitude(), DELTA);
        assertEquals(0, location.getLongitude(), DELTA);
        assertEquals(0, location.getAltitude(), DELTA);
        assertFalse(location.hasAltitude());
        assertEquals(0, location.getSpeed(), DELTA);
        assertFalse(location.hasSpeed());
        assertEquals(0, location.getBearing(), DELTA);
        assertFalse(location.hasBearing());
        assertEquals(0, location.getAccuracy(), DELTA);
        assertFalse(location.hasAccuracy());

        assertEquals(0, location.getVerticalAccuracyMeters(), DELTA);
        assertEquals(0, location.getSpeedAccuracyMetersPerSecond(), DELTA);
        assertEquals(0, location.getBearingAccuracyDegrees(), DELTA);

        assertFalse(location.hasVerticalAccuracy());
        assertFalse(location.hasSpeedAccuracy());
        assertFalse(location.hasBearingAccuracy());

        assertNull(location.getExtras());
    }

    public void testToString() {
        Location location = createTestLocation();

        assertNotNull(location.toString());
    }

    public void testWriteToParcel() {
        Location location = createTestLocation();

        Parcel parcel = Parcel.obtain();
        location.writeToParcel(parcel, 0);
        parcel.setDataPosition(0);
        Location newLocation = Location.CREATOR.createFromParcel(parcel);
        assertTestLocation(newLocation);

        parcel.recycle();
    }

    public void testSettingInjectorService() {
        Context c = getContext();
        SettingInjectorServiceDerived service = new SettingInjectorServiceDerived();

        assertNotNull(c);

        Intent intent =
            new Intent(c, android.location.SettingInjectorService.class);

        assertNotNull(c.getMainLooper());
        SettingInjectorResultHandler resultHandler =
            new SettingInjectorResultHandler(c.getMainLooper());

        Messenger m = new Messenger(resultHandler);
        intent.putExtra(MESSENGER_KEY, m);

        int ret;
        final long timeout = 500;

        // should refuse binding
        IBinder binder = service.callOnBind(intent);
        assertNull("onBind should always fail.", binder);

        // test if result consistent with the truth
        // enabled == false case
        service.setEnabled(false);
        resultHandler.expectEnabled(false);
        resultHandler.expectMessage(true);
        ret = service.callOnStartCommand(intent, SettingInjectorService.START_NOT_STICKY, 0);
        assertEquals("onStartCommand return value invalid in (enabled == false) case.",
            ret, SettingInjectorService.START_NOT_STICKY);
        assertTrue("Message time out in (enabled == false case).",
            resultHandler.waitForMessage(timeout));

        // enabled == true case
        service.setEnabled(true);
        assertTrue(service.onGetEnabled());
        assertEquals("Summary", service.onGetSummary());
        resultHandler.expectEnabled(true);
        resultHandler.expectMessage(true);
        ret = service.callOnStartCommand(intent, SettingInjectorService.START_NOT_STICKY, 0);
        assertEquals("onStartCommand return value invalid in (enabled == true) case.",
            ret, SettingInjectorService.START_NOT_STICKY);
        assertTrue("Message time out in (enabled == true) case.",
            resultHandler.waitForMessage(timeout));

        // should not respond to the deprecated method
        resultHandler.expectMessage(false);
        service.callOnStart(intent, 0);
        resultHandler.waitForMessage(timeout);
    }

    private void assertTestLocation(Location l) {
        assertNotNull(l);
        assertEquals(TEST_PROVIDER, l.getProvider());
        assertEquals(TEST_ACCURACY, l.getAccuracy(), DELTA);
        assertEquals(TEST_VERTICAL_ACCURACY, l.getVerticalAccuracyMeters(), DELTA);
        assertEquals(TEST_SPEED_ACCURACY, l.getSpeedAccuracyMetersPerSecond(), DELTA);
        assertEquals(TEST_BEARING_ACCURACY, l.getBearingAccuracyDegrees(), DELTA);
        assertEquals(TEST_ALTITUDE, l.getAltitude(), DELTA);
        assertEquals(TEST_LATITUDE, l.getLatitude(), DELTA);
        assertEquals(TEST_BEARING, l.getBearing(), DELTA);
        assertEquals(TEST_LONGITUDE, l.getLongitude(), DELTA);
        assertEquals(TEST_SPEED, l.getSpeed(), DELTA);
        assertEquals(TEST_TIME, l.getTime());
        assertTestBundle(l.getExtras());
    }

    private Location createTestLocation() {
        Location l = new Location(TEST_PROVIDER);
        l.setAccuracy(TEST_ACCURACY);
        l.setVerticalAccuracyMeters(TEST_VERTICAL_ACCURACY);
        l.setSpeedAccuracyMetersPerSecond(TEST_SPEED_ACCURACY);
        l.setBearingAccuracyDegrees(TEST_BEARING_ACCURACY);

        l.setAltitude(TEST_ALTITUDE);
        l.setLatitude(TEST_LATITUDE);
        l.setBearing(TEST_BEARING);
        l.setLongitude(TEST_LONGITUDE);
        l.setSpeed(TEST_SPEED);
        l.setTime(TEST_TIME);
        Bundle bundle = new Bundle();
        bundle.putBoolean(TEST_KEY1NAME, TEST_KEY1VALUE);
        bundle.putByte(TEST_KEY2NAME, TEST_KEY2VALUE);
        l.setExtras(bundle);

        return l;
    }

    private void assertTestBundle(Bundle bundle) {
        assertFalse(bundle.getBoolean(TEST_KEY1NAME));
        assertEquals(TEST_KEY2VALUE, bundle.getByte(TEST_KEY2NAME));
    }

    private class SettingInjectorResultHandler extends Handler {
        private boolean mEnabledShouldBe;
        private boolean mExpectingMessage;
        private boolean mMessageArrived;

        SettingInjectorResultHandler(Looper l) {
            super(l);
        }

        @Override
        public void handleMessage(Message m) {

            assertTrue("Unexpected message.", mExpectingMessage);

            boolean enabled = m.getData().getBoolean(ENABLED_KEY);

            assertEquals(String.format(
                    "Message value (%s) inconsistent with service state (%s).",
                    String.valueOf(enabled), String.valueOf(mEnabledShouldBe) ),
                    mEnabledShouldBe, enabled);

            synchronized (this) {
                mMessageArrived = true;
                notify();
            }
        }

        public void expectEnabled(boolean enabled) {
            mEnabledShouldBe = enabled;
        }

        public void expectMessage(boolean expecting) {
            mMessageArrived = false;
            mExpectingMessage = expecting;
        }

        public synchronized boolean waitForMessage(long millis) {
            synchronized (this) {
                try {
                    wait(millis);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
                return mMessageArrived;
            }
        }
    }


    private class SettingInjectorServiceDerived extends SettingInjectorService {

        private boolean mEnabled;

        SettingInjectorServiceDerived() {
            super("SettingInjectorServiceDerived");
            setEnabled(false);
        }

        @Override
        // Deprecated API
        protected String onGetSummary() {
            return "Summary";
        }

        @Override
        protected boolean onGetEnabled() {
            return mEnabled;
        }

        public void setEnabled(boolean enabled) {
            mEnabled = enabled;
        }

        // API coverage dashboard will not count method call from derived class.
        // Thus, it is necessary to make explicit call to SettingInjectorService public methods.
        public IBinder callOnBind(Intent intent) {
            return super.onBind(intent);
        }

        public void callOnStart(Intent intent, int startId) {
            super.onStart(intent, startId);
        }

        public int callOnStartCommand(Intent intent, int flags, int startId) {
            return super.onStartCommand(intent, flags, startId);
        }
    }

}
