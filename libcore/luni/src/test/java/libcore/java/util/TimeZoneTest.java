/*
 * Copyright (C) 2010 The Android Open Source Project
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

package libcore.java.util;

import junit.framework.TestCase;

import java.text.SimpleDateFormat;
import java.time.ZoneId;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Collections;
import java.util.Date;
import java.util.List;
import java.util.Locale;
import java.util.SimpleTimeZone;
import java.util.TimeZone;
import java.util.concurrent.BrokenBarrierException;
import java.util.concurrent.CyclicBarrier;
import java.util.concurrent.atomic.AtomicInteger;

public class TimeZoneTest extends TestCase {
    // http://code.google.com/p/android/issues/detail?id=877
    public void test_useDaylightTime_Taiwan() {
        TimeZone asiaTaipei = TimeZone.getTimeZone("Asia/Taipei");
        assertFalse("Taiwan doesn't use DST", asiaTaipei.useDaylightTime());
    }

    // http://code.google.com/p/android/issues/detail?id=8016
    public void test_useDaylightTime_Iceland() {
        TimeZone atlanticReykjavik = TimeZone.getTimeZone("Atlantic/Reykjavik");
        assertFalse("Reykjavik doesn't use DST", atlanticReykjavik.useDaylightTime());
    }

    // http://code.google.com/p/android/issues/detail?id=11542
    public void test_clone_SimpleTimeZone() {
        SimpleTimeZone stz = new SimpleTimeZone(21600000, "Central Standard Time");
        stz.setStartYear(1000);
        stz.inDaylightTime(new Date());
        stz.clone();
    }

    // http://b/3049014
    public void testCustomTimeZoneDisplayNames() {
        TimeZone tz0001 = new SimpleTimeZone(60000, "ONE MINUTE");
        TimeZone tz0130 = new SimpleTimeZone(5400000, "ONE HOUR, THIRTY");
        TimeZone tzMinus0130 = new SimpleTimeZone(-5400000, "NEG ONE HOUR, THIRTY");
        assertEquals("GMT+00:01", tz0001.getDisplayName(false, TimeZone.SHORT, Locale.US));
        assertEquals("GMT+01:30", tz0130.getDisplayName(false, TimeZone.SHORT, Locale.US));
        assertEquals("GMT-01:30", tzMinus0130.getDisplayName(false, TimeZone.SHORT, Locale.US));
    }

    // http://code.google.com/p/android/issues/detail?id=14395
    public void testPreHistoricInDaylightTime() throws Exception {
        Locale.setDefault(Locale.US);
        TimeZone tz = TimeZone.getTimeZone("America/Los_Angeles");
        TimeZone.setDefault(tz);
        SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss.SSSZ");
        Date date = sdf.parse("1902-11-01T00:00:00.000+0800");
        assertEquals(-2119680000000L, date.getTime());
        assertEquals(-28800000, tz.getOffset(date.getTime()));
        assertFalse(tz.inDaylightTime(date));
        assertEquals("Fri Oct 31 08:00:00 PST 1902", date.toString());
        assertEquals("31 Oct 1902 16:00:00 GMT", date.toGMTString());
        // Any time before we have transition data is considered non-daylight, even in summer.
        date = sdf.parse("1902-06-01T00:00:00.000+0800");
        assertEquals(-28800000, tz.getOffset(date.getTime()));
        assertFalse(tz.inDaylightTime(date));
    }

    public void testGetDisplayNameShort_nonHourOffsets() {
        TimeZone iranTz = TimeZone.getTimeZone("Asia/Tehran");
        assertEquals("GMT+03:30", iranTz.getDisplayName(false, TimeZone.SHORT, Locale.UK));
        assertEquals("GMT+04:30", iranTz.getDisplayName(true, TimeZone.SHORT, Locale.UK));
    }

    public void testPreHistoricOffsets() throws Exception {
        // "Africa/Bissau" has just a few transitions and hasn't changed in a long time.
        // 1912-01-01 00:02:19-0100 ... 1912-01-01 00:02:20-0100
        // 1974-12-31 23:59:59-0100 ... 1975-01-01 01:00:00+0000
        TimeZone tz = TimeZone.getTimeZone("Africa/Bissau");

        // Times before our first transition should assume we're still following that transition.
        assertNonDaylightOffset(-3600, parseIsoTime("1911-01-01T00:00:00.0+0000"), tz);

        assertNonDaylightOffset(-3600, parseIsoTime("1912-01-01T12:00:00.0-0100"), tz);

        // Times after our last transition should assume we're still following that transition.
        assertNonDaylightOffset(0, parseIsoTime("1980-01-01T00:00:00.0+0000"), tz);
    }

    private static void assertNonDaylightOffset(int expectedOffsetSeconds, long epochSeconds, TimeZone tz) {
        assertEquals(expectedOffsetSeconds, tz.getOffset(epochSeconds * 1000) / 1000);
        assertFalse(tz.inDaylightTime(new Date(epochSeconds * 1000)));
    }

    private static long parseIsoTime(String isoTime) throws Exception {
        SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss.SSSZ");
        Date date = sdf.parse(isoTime);
        return date.getTime() / 1000;
    }

    public void testZeroTransitionZones() throws Exception {
        // Zones with no transitions historical or future seem ideal for testing.
        String[] ids = new String[] { "Africa/Bujumbura", "Indian/Cocos", "Pacific/Wake", "UTC" };
        for (String id : ids) {
            TimeZone tz = TimeZone.getTimeZone(id);
            assertFalse(tz.useDaylightTime());
            assertFalse(tz.inDaylightTime(new Date(Integer.MIN_VALUE)));
            assertFalse(tz.inDaylightTime(new Date(0)));
            assertFalse(tz.inDaylightTime(new Date(Integer.MAX_VALUE)));
            int currentOffset = tz.getOffset(new Date(0).getTime());
            assertEquals(currentOffset, tz.getOffset(new Date(Integer.MIN_VALUE).getTime()));
            assertEquals(currentOffset, tz.getOffset(new Date(Integer.MAX_VALUE).getTime()));
        }
    }

    // http://code.google.com/p/android/issues/detail?id=16608
    public void testHelsinkiOverflow() throws Exception {
        long time = 2147483648000L;// Tue, 19 Jan 2038 03:14:08 GMT
        TimeZone timeZone = TimeZone.getTimeZone("Europe/Helsinki");
        long offset = timeZone.getOffset(time);
        // This might cause us trouble if Europe/Helsinki changes its rules. See the bug for
        // details of the intent of this test.
        assertEquals(7200000, offset);
    }

    // http://code.google.com/p/android/issues/detail?id=11918
    public void testHasSameRules() throws Exception {
        TimeZone denver = TimeZone.getTimeZone("America/Denver");
        TimeZone phoenix = TimeZone.getTimeZone("America/Phoenix");
        assertFalse(denver.hasSameRules(phoenix));
    }

    // http://code.google.com/p/android/issues/detail?id=24036
    public void testNullId() throws Exception {
        try {
            TimeZone.getTimeZone((String) null);
            fail();
        } catch (NullPointerException expected) {
        }
    }

    public void testNullZoneId() throws Exception {
        try {
            TimeZone.getTimeZone((ZoneId) null);
            fail();
        } catch (NullPointerException expected) {
        }
    }

    // http://b.corp.google.com/issue?id=6556561
    public void testCustomZoneIds() throws Exception {
        // These are all okay (and equivalent).
        assertEquals("GMT+05:00", TimeZone.getTimeZone("GMT+05:00").getID());
        assertEquals("GMT+05:00", TimeZone.getTimeZone("GMT+5:00").getID());
        assertEquals("GMT+05:00", TimeZone.getTimeZone("GMT+0500").getID());
        assertEquals("GMT+05:00", TimeZone.getTimeZone("GMT+500").getID());
        assertEquals("GMT+05:00", TimeZone.getTimeZone("GMT+5").getID());
        // These aren't.
        assertEquals("GMT", TimeZone.getTimeZone("GMT+5.5").getID());
        assertEquals("GMT", TimeZone.getTimeZone("GMT+5:5").getID());
        assertEquals("GMT", TimeZone.getTimeZone("GMT+5:0").getID());
        assertEquals("GMT", TimeZone.getTimeZone("GMT+5:005").getID());
        assertEquals("GMT", TimeZone.getTimeZone("GMT+5:000").getID());
        assertEquals("GMT", TimeZone.getTimeZone("GMT+005:00").getID());
        assertEquals("GMT", TimeZone.getTimeZone("GMT+05:99").getID());
        assertEquals("GMT", TimeZone.getTimeZone("GMT+28:00").getID());
        assertEquals("GMT", TimeZone.getTimeZone("GMT+05:00.00").getID());
        assertEquals("GMT", TimeZone.getTimeZone("GMT+05:00:00").getID());
        assertEquals("GMT", TimeZone.getTimeZone("GMT+5:").getID());
        assertEquals("GMT", TimeZone.getTimeZone("GMT+junk").getID());
        assertEquals("GMT", TimeZone.getTimeZone("GMT+5junk").getID());
        assertEquals("GMT", TimeZone.getTimeZone("GMT+5:junk").getID());
        assertEquals("GMT", TimeZone.getTimeZone("GMT+5:00junk").getID());
        assertEquals("GMT", TimeZone.getTimeZone("junkGMT+5:00").getID());
        assertEquals("GMT", TimeZone.getTimeZone("junk").getID());
        assertEquals("GMT", TimeZone.getTimeZone("gmt+5:00").getID());
    }

    public void test_getDSTSavings() throws Exception {
      assertEquals(0, TimeZone.getTimeZone("UTC").getDSTSavings());
      assertEquals(3600000, TimeZone.getTimeZone("America/Los_Angeles").getDSTSavings());
      assertEquals(1800000, TimeZone.getTimeZone("Australia/Lord_Howe").getDSTSavings());
    }

    public void testSimpleTimeZoneDoesNotCallOverrideableMethodsFromConstructor() {
        new SimpleTimeZone(0, "X", Calendar.MARCH, 1, 1, 1, Calendar.SEPTEMBER, 1, 1, 1) {
            @Override public void setStartRule(int m, int d, int dow, int time) {
                fail();
            }
            @Override public void setStartRule(int m, int d, int dow, int time, boolean after) {
                fail();
            }
            @Override public void setEndRule(int m, int d, int dow, int time) {
                fail();
            }
            @Override public void setEndRule(int m, int d, int dow, int time, boolean after) {
                fail();
            }
        };
    }

    // http://b/7955614 and http://b/8026776.
    public void testDisplayNames() throws Exception {
        checkDisplayNames(Locale.US);
    }

    public void testDisplayNames_nonUS() throws Exception {
        // run checkDisplayNames with an arbitrary set of Locales.
        checkDisplayNames(Locale.CHINESE);
        checkDisplayNames(Locale.FRENCH);
        checkDisplayNames(Locale.forLanguageTag("bn-BD"));
    }

    public void checkDisplayNames(Locale locale) throws Exception {
        // Check that there are no time zones that use DST but have the same display name for
        // both standard and daylight time.
        StringBuilder failures = new StringBuilder();
        for (String id : TimeZone.getAvailableIDs()) {
            TimeZone tz = TimeZone.getTimeZone(id);
            String longDst = tz.getDisplayName(true, TimeZone.LONG, locale);
            String longStd = tz.getDisplayName(false, TimeZone.LONG, locale);
            String shortDst = tz.getDisplayName(true, TimeZone.SHORT, locale);
            String shortStd = tz.getDisplayName(false, TimeZone.SHORT, locale);

            if (tz.useDaylightTime()) {
                // The long std and dst strings must differ!
                if (longDst.equals(longStd)) {
                    failures.append(String.format("\n%20s: LD='%s' LS='%s'!",
                                                  id, longDst, longStd));
                }
                // The short std and dst strings must differ!
                if (shortDst.equals(shortStd)) {
                    failures.append(String.format("\n%20s: SD='%s' SS='%s'!",
                                                  id, shortDst, shortStd));
                }

                // If the short std matches the long dst, or the long std matches the short dst,
                // it probably means we have a time zone that icu4c doesn't believe has ever
                // observed dst.
                if (shortStd.equals(longDst)) {
                    failures.append(String.format("\n%20s: SS='%s' LD='%s'!",
                                                  id, shortStd, longDst));
                }
                if (longStd.equals(shortDst)) {
                    failures.append(String.format("\n%20s: LS='%s' SD='%s'!",
                                                  id, longStd, shortDst));
                }

                // The long and short dst strings must differ!
                if (longDst.equals(shortDst) && !longDst.startsWith("GMT")) {
                  failures.append(String.format("\n%20s: LD='%s' SD='%s'!",
                                                id, longDst, shortDst));
                }
            }

            // Sanity check that whenever a display name is just a GMT string that it's the
            // right GMT string.
            String gmtDst = formatGmtString(tz, true);
            String gmtStd = formatGmtString(tz, false);
            if (isGmtString(longDst) && !longDst.equals(gmtDst)) {
                failures.append(String.format("\n%s: LD %s", id, longDst));
            }
            if (isGmtString(longStd) && !longStd.equals(gmtStd)) {
                failures.append(String.format("\n%s: LS %s", id, longStd));
            }
            if (isGmtString(shortDst) && !shortDst.equals(gmtDst)) {
                failures.append(String.format("\n%s: SD %s", id, shortDst));
            }
            if (isGmtString(shortStd) && !shortStd.equals(gmtStd)) {
                failures.append(String.format("\n%s: SS %s", id, shortStd));
            }
        }
        assertEquals("", failures.toString());
    }

    // http://b/30527513
    public void testDisplayNamesWithScript() throws Exception {
        Locale latinLocale = Locale.forLanguageTag("sr-Latn-RS");
        Locale cyrillicLocale = Locale.forLanguageTag("sr-Cyrl-RS");
        Locale noScriptLocale = Locale.forLanguageTag("sr-RS");
        TimeZone tz = TimeZone.getTimeZone("Europe/London");

        final String latinName = "Srednje vreme po Griniču";
        final String cyrillicName = "Средње време по Гриничу";

        // Check java.util.TimeZone
        assertEquals(latinName, tz.getDisplayName(latinLocale));
        assertEquals(cyrillicName, tz.getDisplayName(cyrillicLocale));
        assertEquals(cyrillicName, tz.getDisplayName(noScriptLocale));

        // Check ICU TimeZoneNames
        // The one-argument getDisplayName() override uses LONG_GENERIC style which is different
        // from what java.util.TimeZone uses. Force the LONG style to get equivalent results.
        final int style = android.icu.util.TimeZone.LONG;
        android.icu.util.TimeZone utz = android.icu.util.TimeZone.getTimeZone(tz.getID());
        assertEquals(latinName, utz.getDisplayName(false, style, latinLocale));
        assertEquals(cyrillicName, utz.getDisplayName(false, style, cyrillicLocale));
        assertEquals(cyrillicName, utz.getDisplayName(false, style, noScriptLocale));
    }

    // http://b/7955614
    public void testApia() throws Exception {
        TimeZone tz = TimeZone.getTimeZone("Pacific/Apia");
        assertEquals("Apia Daylight Time", tz.getDisplayName(true, TimeZone.LONG, Locale.US));
        assertEquals("Apia Standard Time", tz.getDisplayName(false, TimeZone.LONG, Locale.US));
        assertEquals("GMT+14:00", tz.getDisplayName(true, TimeZone.SHORT, Locale.US));
        assertEquals("GMT+13:00", tz.getDisplayName(false, TimeZone.SHORT, Locale.US));
    }

    private static boolean isGmtString(String s) {
        return s.startsWith("GMT+") || s.startsWith("GMT-");
    }

    private static String formatGmtString(TimeZone tz, boolean daylight) {
        int offset = tz.getRawOffset();
        if (daylight) {
            offset += tz.getDSTSavings();
        }
        offset /= 60000;
        char sign = '+';
        if (offset < 0) {
            sign = '-';
            offset = -offset;
        }
        return String.format("GMT%c%02d:%02d", sign, offset / 60, offset % 60);
    }

    // http://b/18839557
    public void testOverflowing32BitUnixDates() {
        final TimeZone tz = TimeZone.getTimeZone("America/New_York");

        // This timezone didn't have any daylight savings prior to 1917 and this
        // date is sometime in 1901.
        assertFalse(tz.inDaylightTime(new Date(-2206292400000L)));
        assertEquals(-18000000, tz.getOffset(-2206292400000L));

        // Nov 30th 2039, no daylight savings as per current rules.
        assertFalse(tz.inDaylightTime(new Date(2206292400000L)));
        assertEquals(-18000000, tz.getOffset(2206292400000L));
    }

    public void testTimeZoneIDLocalization() {
        Locale defaultLocale = Locale.getDefault();
        try {
            Locale.setDefault(new Locale("en"));
            TimeZone en_timezone = TimeZone.getTimeZone("GMT+09:00");
            Locale.setDefault(new Locale("ar"));
            TimeZone ar_timezone = TimeZone.getTimeZone("GMT+09:00");

            assertEquals(en_timezone.getID(), ar_timezone.getID());
        } finally {
            Locale.setDefault(defaultLocale);
        }
    }

    // http://b/28949992
    public void testSetDefaultAppliesToIcuTimezone() {
        TimeZone origTz = TimeZone.getDefault();
        try {
            android.icu.util.TimeZone origIcuTz = android.icu.util.TimeZone.getDefault();
            assertEquals(origTz.getID(), origIcuTz.getID());

            TimeZone tz = TimeZone.getTimeZone("GMT-05:00");
            TimeZone.setDefault(tz);
            android.icu.util.TimeZone icuTz = android.icu.util.TimeZone.getDefault();
            assertEquals(tz.getID(), icuTz.getID());
        } finally {
            TimeZone.setDefault(origTz);
        }
    }

    // http://b/33197219
    public void testDisplayNameForNonCanonicalTimezones() {
        TimeZone canonical = TimeZone.getTimeZone("Europe/London");
        TimeZone nonCanonical = TimeZone.getTimeZone("GB");

        // verify that GB is actually an alias for Europe/London
        assertTrue(canonical.hasSameRules(nonCanonical));

        assertEquals(canonical.getDisplayName(true, TimeZone.LONG, Locale.ENGLISH),
                nonCanonical.getDisplayName(true, TimeZone.LONG, Locale.ENGLISH));
    }

    // http://b/30937209
    public void testSetDefaultDeadlock() throws InterruptedException, BrokenBarrierException {
        // Since this tests a deadlock, the test has two fundamental problems:
        // - it is probabilistic: it's not guaranteed to fail if the problem exists
        // - if it fails, it will effectively hang the current runtime, as no other thread will
        //   be able to call TimeZone.getDefault()/setDefault() successfully any more.

        // 10 was too low to be reliable, 100 failed more than half the time (on a bullhead).
        final int iterations = 100;
        TimeZone otherTimeZone = TimeZone.getTimeZone("Europe/London");
        AtomicInteger setterCount = new AtomicInteger();
        CyclicBarrier startBarrier = new CyclicBarrier(2);
        Thread setter = new Thread(() -> {
            waitFor(startBarrier);
            for (int i = 0; i < iterations; i++) {
                TimeZone.setDefault(otherTimeZone);
                TimeZone.setDefault(null);
                setterCount.set(i+1);
            }
        });
        setter.setName("testSetDefaultDeadlock setter");

        AtomicInteger getterCount = new AtomicInteger();
        Thread getter = new Thread(() -> {
            waitFor(startBarrier);
            for (int i = 0; i < iterations; i++) {
                android.icu.util.TimeZone.getDefault();
                getterCount.set(i+1);
            }
        });
        getter.setName("testSetDefaultDeadlock getter");

        setter.start();
        getter.start();

        // 2 seconds is plenty: If successful, we usually complete much faster.
        setter.join(1000);
        getter.join(1000);
        if (setter.isAlive() || getter.isAlive()) {
            fail("Threads are still alive. Getter iteration count: " + getterCount.get()
                    + ", setter iteration count: " + setterCount.get());
        }
        // Guard against unexpected uncaught exceptions.
        assertEquals("Setter iterations", iterations, setterCount.get());
        assertEquals("Getter iterations", iterations, getterCount.get());
    }

    // http://b/30979219
    public void testSetDefaultRace() throws InterruptedException {
        // Since this tests a race condition, the test is probabilistic: it's not guaranteed to
        // fail if the problem exists

        // These iterations are significantly faster than the ones in #testSetDefaultDeadlock
        final int iterations = 10000;
        List<Throwable> exceptions = Collections.synchronizedList(new ArrayList<>());
        Thread.UncaughtExceptionHandler handler = (t, e) -> exceptions.add(e);

        CyclicBarrier startBarrier = new CyclicBarrier(2);
        Thread clearer = new Thread(() -> {
            waitFor(startBarrier);
            for (int i = 0; i < iterations; i++) {
                // This is not public API but can effectively be invoked via
                // java.util.TimeZone.setDefault. Call it directly to reduce the amount of code
                // involved in this test.
                android.icu.util.TimeZone.setICUDefault(null);
            }
        });
        clearer.setName("testSetDefaultRace clearer");
        clearer.setUncaughtExceptionHandler(handler);

        Thread getter = new Thread(() -> {
            waitFor(startBarrier);
            for (int i = 0; i < iterations; i++) {
                android.icu.util.TimeZone.getDefault();
            }
        });
        getter.setName("testSetDefaultRace getter");
        getter.setUncaughtExceptionHandler(handler);

        clearer.start();
        getter.start();

        // 20 seconds is plenty: If successful, we usually complete much faster.
        clearer.join(10000);
        getter.join(10000);

        if (!exceptions.isEmpty()) {
            Throwable firstException = exceptions.get(0);
            firstException.printStackTrace();
            fail("Threads did not succeed successfully: " + firstException);
        }
        assertFalse("clearer thread is still alive", clearer.isAlive());
        assertFalse("getter thread is still alive", getter.isAlive());
    }

    private static void waitFor(CyclicBarrier barrier) {
        try {
            barrier.await();
        } catch (InterruptedException | BrokenBarrierException e) {
            throw new RuntimeException(e);
        }
    }
}
