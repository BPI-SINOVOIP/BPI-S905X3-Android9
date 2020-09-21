/*
 * Copyright (C) 2013 The Android Open Source Project
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

package libcore.libcore.icu;

import android.icu.util.Calendar;
import android.icu.util.TimeZone;
import android.icu.util.ULocale;

import static libcore.icu.DateIntervalFormat.formatDateRange;
import static libcore.icu.DateUtilsBridge.*;

public class DateIntervalFormatTest extends junit.framework.TestCase {
  private static final long MINUTE = 60 * 1000;
  private static final long HOUR = 60 * MINUTE;
  private static final long DAY = 24 * HOUR;
  private static final long MONTH = 31 * DAY;
  private static final long YEAR = 12 * MONTH;

  // These are the old CTS tests for DateIntervalFormat.formatDateRange.
  public void test_formatDateInterval() throws Exception {
    TimeZone tz = TimeZone.getTimeZone("America/Los_Angeles");

    Calendar c = Calendar.getInstance(tz, ULocale.US);
    c.set(Calendar.MONTH, Calendar.JANUARY);
    c.set(Calendar.DAY_OF_MONTH, 19);
    c.set(Calendar.HOUR_OF_DAY, 3);
    c.set(Calendar.MINUTE, 30);
    c.set(Calendar.SECOND, 15);
    c.set(Calendar.MILLISECOND, 0);
    long timeWithCurrentYear = c.getTimeInMillis();

    c.set(Calendar.YEAR, 2009);
    long fixedTime = c.getTimeInMillis();

    c.set(Calendar.MINUTE, 0);
    c.set(Calendar.SECOND, 0);
    long onTheHour = c.getTimeInMillis();

    long noonDuration = (8 * 60 + 30) * 60 * 1000 - 15 * 1000;
    long midnightDuration = (3 * 60 + 30) * 60 * 1000 + 15 * 1000;

    ULocale de_DE = new ULocale("de", "DE");
    ULocale en_US = new ULocale("en", "US");
    ULocale es_ES = new ULocale("es", "ES");
    ULocale es_US = new ULocale("es", "US");

    assertEquals("Monday", formatDateRange(en_US, tz, fixedTime, fixedTime + HOUR, FORMAT_SHOW_WEEKDAY));
    assertEquals("January 19", formatDateRange(en_US, tz, timeWithCurrentYear, timeWithCurrentYear + HOUR, FORMAT_SHOW_DATE));
    assertEquals("3:30 AM", formatDateRange(en_US, tz, fixedTime, fixedTime, FORMAT_SHOW_TIME));
    assertEquals("January 19, 2009", formatDateRange(en_US, tz, fixedTime, fixedTime + HOUR, FORMAT_SHOW_YEAR));
    assertEquals("January 19", formatDateRange(en_US, tz, fixedTime, fixedTime + HOUR, FORMAT_NO_YEAR));
    assertEquals("January", formatDateRange(en_US, tz, timeWithCurrentYear, timeWithCurrentYear + HOUR, FORMAT_NO_MONTH_DAY));
    assertEquals("3:30 AM", formatDateRange(en_US, tz, fixedTime, fixedTime, FORMAT_12HOUR | FORMAT_SHOW_TIME));
    assertEquals("03:30", formatDateRange(en_US, tz, fixedTime, fixedTime, FORMAT_24HOUR | FORMAT_SHOW_TIME));
    assertEquals("3:30 AM", formatDateRange(en_US, tz, fixedTime, fixedTime, FORMAT_12HOUR /*| FORMAT_CAP_AMPM*/ | FORMAT_SHOW_TIME));
    assertEquals("12:00 PM", formatDateRange(en_US, tz, fixedTime + noonDuration, fixedTime + noonDuration, FORMAT_12HOUR | FORMAT_SHOW_TIME));
    assertEquals("12:00 PM", formatDateRange(en_US, tz, fixedTime + noonDuration, fixedTime + noonDuration, FORMAT_12HOUR | FORMAT_SHOW_TIME /*| FORMAT_CAP_NOON*/));
    assertEquals("12:00 PM", formatDateRange(en_US, tz, fixedTime + noonDuration, fixedTime + noonDuration, FORMAT_12HOUR /*| FORMAT_NO_NOON*/ | FORMAT_SHOW_TIME));
    assertEquals("12:00 AM", formatDateRange(en_US, tz, fixedTime - midnightDuration, fixedTime - midnightDuration, FORMAT_12HOUR | FORMAT_SHOW_TIME /*| FORMAT_NO_MIDNIGHT*/));
    assertEquals("3:30 AM", formatDateRange(en_US, tz, fixedTime, fixedTime, FORMAT_SHOW_TIME | FORMAT_UTC));
    assertEquals("3 AM", formatDateRange(en_US, tz, onTheHour, onTheHour, FORMAT_SHOW_TIME | FORMAT_ABBREV_TIME));
    assertEquals("Mon", formatDateRange(en_US, tz, fixedTime, fixedTime + HOUR, FORMAT_SHOW_WEEKDAY | FORMAT_ABBREV_WEEKDAY));
    assertEquals("Jan 19", formatDateRange(en_US, tz, timeWithCurrentYear, timeWithCurrentYear + HOUR, FORMAT_SHOW_DATE | FORMAT_ABBREV_MONTH));
    assertEquals("Jan 19", formatDateRange(en_US, tz, timeWithCurrentYear, timeWithCurrentYear + HOUR, FORMAT_SHOW_DATE | FORMAT_ABBREV_ALL));

    assertEquals("1/19/2009", formatDateRange(en_US, tz, fixedTime, fixedTime + 3 * HOUR, FORMAT_SHOW_YEAR | FORMAT_NUMERIC_DATE));
    assertEquals("1/19/2009 – 1/22/2009", formatDateRange(en_US, tz, fixedTime, fixedTime + 3 * DAY, FORMAT_SHOW_YEAR | FORMAT_NUMERIC_DATE));
    assertEquals("1/19/2009 – 4/22/2009", formatDateRange(en_US, tz, fixedTime, fixedTime + 3 * MONTH, FORMAT_SHOW_YEAR | FORMAT_NUMERIC_DATE));
    assertEquals("1/19/2009 – 2/9/2012", formatDateRange(en_US, tz, fixedTime, fixedTime + 3 * YEAR, FORMAT_SHOW_YEAR | FORMAT_NUMERIC_DATE));

    assertEquals("19.1.2009", formatDateRange(de_DE, tz, fixedTime, fixedTime + HOUR, FORMAT_SHOW_YEAR | FORMAT_NUMERIC_DATE));
    assertEquals("19.01.2009 – 22.01.2009", formatDateRange(de_DE, tz, fixedTime, fixedTime + 3 * DAY, FORMAT_SHOW_YEAR | FORMAT_NUMERIC_DATE));
    assertEquals("19.01.2009 – 22.04.2009", formatDateRange(de_DE, tz, fixedTime, fixedTime + 3 * MONTH, FORMAT_SHOW_YEAR | FORMAT_NUMERIC_DATE));
    assertEquals("19.01.2009 – 09.02.2012", formatDateRange(de_DE, tz, fixedTime, fixedTime + 3 * YEAR, FORMAT_SHOW_YEAR | FORMAT_NUMERIC_DATE));

    assertEquals("19/1/2009", formatDateRange(es_US, tz, fixedTime, fixedTime + HOUR, FORMAT_SHOW_YEAR | FORMAT_NUMERIC_DATE));
    assertEquals("19/1/2009–22/1/2009", formatDateRange(es_US, tz, fixedTime, fixedTime + 3 * DAY, FORMAT_SHOW_YEAR | FORMAT_NUMERIC_DATE));
    assertEquals("19/1/2009–22/4/2009", formatDateRange(es_US, tz, fixedTime, fixedTime + 3 * MONTH, FORMAT_SHOW_YEAR | FORMAT_NUMERIC_DATE));
    assertEquals("19/1/2009–9/2/2012", formatDateRange(es_US, tz, fixedTime, fixedTime + 3 * YEAR, FORMAT_SHOW_YEAR | FORMAT_NUMERIC_DATE));

    assertEquals("19/1/2009", formatDateRange(es_ES, tz, fixedTime, fixedTime + HOUR, FORMAT_SHOW_YEAR | FORMAT_NUMERIC_DATE));
    assertEquals("19/1/2009–22/1/2009", formatDateRange(es_ES, tz, fixedTime, fixedTime + 3 * DAY, FORMAT_SHOW_YEAR | FORMAT_NUMERIC_DATE));
    assertEquals("19/1/2009–22/4/2009", formatDateRange(es_ES, tz, fixedTime, fixedTime + 3 * MONTH, FORMAT_SHOW_YEAR | FORMAT_NUMERIC_DATE));
    assertEquals("19/1/2009–9/2/2012", formatDateRange(es_ES, tz, fixedTime, fixedTime + 3 * YEAR, FORMAT_SHOW_YEAR | FORMAT_NUMERIC_DATE));

    // These are some random other test cases I came up with.

    assertEquals("January 19 – 22, 2009", formatDateRange(en_US, tz, fixedTime, fixedTime + 3 * DAY, 0));
    assertEquals("Jan 19 – 22, 2009", formatDateRange(en_US, tz, fixedTime, fixedTime + 3 * DAY, FORMAT_SHOW_DATE | FORMAT_ABBREV_ALL));
    assertEquals("Mon, Jan 19 – Thu, Jan 22, 2009", formatDateRange(en_US, tz, fixedTime, fixedTime + 3 * DAY, FORMAT_SHOW_WEEKDAY | FORMAT_ABBREV_ALL));
    assertEquals("Monday, January 19 – Thursday, January 22, 2009", formatDateRange(en_US, tz, fixedTime, fixedTime + 3 * DAY, FORMAT_SHOW_WEEKDAY));

    assertEquals("January 19 – April 22, 2009", formatDateRange(en_US, tz, fixedTime, fixedTime + 3 * MONTH, 0));
    assertEquals("Jan 19 – Apr 22, 2009", formatDateRange(en_US, tz, fixedTime, fixedTime + 3 * MONTH, FORMAT_SHOW_DATE | FORMAT_ABBREV_ALL));
    assertEquals("Mon, Jan 19 – Wed, Apr 22, 2009", formatDateRange(en_US, tz, fixedTime, fixedTime + 3 * MONTH, FORMAT_SHOW_WEEKDAY | FORMAT_ABBREV_ALL));
    assertEquals("January – April 2009", formatDateRange(en_US, tz, fixedTime, fixedTime + 3 * MONTH, FORMAT_NO_MONTH_DAY));

    assertEquals("Jan 19, 2009 – Feb 9, 2012", formatDateRange(en_US, tz, fixedTime, fixedTime + 3 * YEAR, FORMAT_SHOW_DATE | FORMAT_ABBREV_ALL));
    assertEquals("Jan 2009 – Feb 2012", formatDateRange(en_US, tz, fixedTime, fixedTime + 3 * YEAR, FORMAT_NO_MONTH_DAY | FORMAT_ABBREV_ALL));
    assertEquals("January 19, 2009 – February 9, 2012", formatDateRange(en_US, tz, fixedTime, fixedTime + 3 * YEAR, 0));
    assertEquals("Monday, January 19, 2009 – Thursday, February 9, 2012", formatDateRange(en_US, tz, fixedTime, fixedTime + 3 * YEAR, FORMAT_SHOW_WEEKDAY));

    // The same tests but for de_DE.

    assertEquals("19.–22. Januar 2009", formatDateRange(de_DE, tz, fixedTime, fixedTime + 3 * DAY, 0));
    assertEquals("19.–22. Jan. 2009", formatDateRange(de_DE, tz, fixedTime, fixedTime + 3 * DAY, FORMAT_SHOW_DATE | FORMAT_ABBREV_ALL));
    assertEquals("Mo., 19. – Do., 22. Jan. 2009", formatDateRange(de_DE, tz, fixedTime, fixedTime + 3 * DAY, FORMAT_SHOW_WEEKDAY | FORMAT_ABBREV_ALL));
    assertEquals("Montag, 19. – Donnerstag, 22. Januar 2009", formatDateRange(de_DE, tz, fixedTime, fixedTime + 3 * DAY, FORMAT_SHOW_WEEKDAY));

    assertEquals("19. Januar – 22. April 2009", formatDateRange(de_DE, tz, fixedTime, fixedTime + 3 * MONTH, 0));
    assertEquals("19. Jan. – 22. Apr. 2009", formatDateRange(de_DE, tz, fixedTime, fixedTime + 3 * MONTH, FORMAT_SHOW_DATE | FORMAT_ABBREV_ALL));
    assertEquals("Mo., 19. Jan. – Mi., 22. Apr. 2009", formatDateRange(de_DE, tz, fixedTime, fixedTime + 3 * MONTH, FORMAT_SHOW_WEEKDAY | FORMAT_ABBREV_ALL));
    assertEquals("Januar–April 2009", formatDateRange(de_DE, tz, fixedTime, fixedTime + 3 * MONTH, FORMAT_NO_MONTH_DAY));

    assertEquals("19. Jan. 2009 – 9. Feb. 2012", formatDateRange(de_DE, tz, fixedTime, fixedTime + 3 * YEAR, FORMAT_SHOW_DATE | FORMAT_ABBREV_ALL));
    assertEquals("Jan. 2009 – Feb. 2012", formatDateRange(de_DE, tz, fixedTime, fixedTime + 3 * YEAR, FORMAT_NO_MONTH_DAY | FORMAT_ABBREV_ALL));
    assertEquals("19. Januar 2009 – 9. Februar 2012", formatDateRange(de_DE, tz, fixedTime, fixedTime + 3 * YEAR, 0));
    assertEquals("Montag, 19. Januar 2009 – Donnerstag, 9. Februar 2012", formatDateRange(de_DE, tz, fixedTime, fixedTime + 3 * YEAR, FORMAT_SHOW_WEEKDAY));

    // The same tests but for es_US.

    assertEquals("19–22 de enero de 2009", formatDateRange(es_US, tz, fixedTime, fixedTime + 3 * DAY, 0));
    assertEquals("19–22 de ene. de 2009", formatDateRange(es_US, tz, fixedTime, fixedTime + 3 * DAY, FORMAT_SHOW_DATE | FORMAT_ABBREV_ALL));
    assertEquals("lun., 19 de ene. – jue., 22 de ene. de 2009", formatDateRange(es_US, tz, fixedTime, fixedTime + 3 * DAY, FORMAT_SHOW_WEEKDAY | FORMAT_ABBREV_ALL));
    assertEquals("lunes, 19 de enero–jueves, 22 de enero de 2009", formatDateRange(es_US, tz, fixedTime, fixedTime + 3 * DAY, FORMAT_SHOW_WEEKDAY));

    assertEquals("19 de enero–22 de abril de 2009", formatDateRange(es_US, tz, fixedTime, fixedTime + 3 * MONTH, 0));
    assertEquals("19 de ene. – 22 de abr. 2009", formatDateRange(es_US, tz, fixedTime, fixedTime + 3 * MONTH, FORMAT_SHOW_DATE | FORMAT_ABBREV_ALL));
    assertEquals("lun., 19 de ene. – mié., 22 de abr. de 2009", formatDateRange(es_US, tz, fixedTime, fixedTime + 3 * MONTH, FORMAT_SHOW_WEEKDAY | FORMAT_ABBREV_ALL));
    assertEquals("enero–abril de 2009", formatDateRange(es_US, tz, fixedTime, fixedTime + 3 * MONTH, FORMAT_NO_MONTH_DAY));

    assertEquals("19 de ene. de 2009 – 9 de feb. de 2012", formatDateRange(es_US, tz, fixedTime, fixedTime + 3 * YEAR, FORMAT_SHOW_DATE | FORMAT_ABBREV_ALL));
    assertEquals("ene. de 2009 – feb. de 2012", formatDateRange(es_US, tz, fixedTime, fixedTime + 3 * YEAR, FORMAT_NO_MONTH_DAY | FORMAT_ABBREV_ALL));
    assertEquals("19 de enero de 2009–9 de febrero de 2012", formatDateRange(es_US, tz, fixedTime, fixedTime + 3 * YEAR, 0));
    assertEquals("lunes, 19 de enero de 2009–jueves, 9 de febrero de 2012", formatDateRange(es_US, tz, fixedTime, fixedTime + 3 * YEAR, FORMAT_SHOW_WEEKDAY));

    // The same tests but for es_ES.

    assertEquals("19–22 de enero de 2009", formatDateRange(es_ES, tz, fixedTime, fixedTime + 3 * DAY, 0));
    assertEquals("19–22 ene. 2009", formatDateRange(es_ES, tz, fixedTime, fixedTime + 3 * DAY, FORMAT_SHOW_DATE | FORMAT_ABBREV_ALL));
    assertEquals("lun., 19 ene. – jue., 22 ene. 2009", formatDateRange(es_ES, tz, fixedTime, fixedTime + 3 * DAY, FORMAT_SHOW_WEEKDAY | FORMAT_ABBREV_ALL));
    assertEquals("lunes, 19 de enero–jueves, 22 de enero de 2009", formatDateRange(es_ES, tz, fixedTime, fixedTime + 3 * DAY, FORMAT_SHOW_WEEKDAY));

    assertEquals("19 de enero–22 de abril de 2009", formatDateRange(es_ES, tz, fixedTime, fixedTime + 3 * MONTH, 0));
    assertEquals("19 ene. – 22 abr. 2009", formatDateRange(es_ES, tz, fixedTime, fixedTime + 3 * MONTH, FORMAT_SHOW_DATE | FORMAT_ABBREV_ALL));
    assertEquals("lun., 19 ene. – mié., 22 abr. 2009", formatDateRange(es_ES, tz, fixedTime, fixedTime + 3 * MONTH, FORMAT_SHOW_WEEKDAY | FORMAT_ABBREV_ALL));
    assertEquals("enero–abril de 2009", formatDateRange(es_ES, tz, fixedTime, fixedTime + 3 * MONTH, FORMAT_NO_MONTH_DAY));

    assertEquals("19 ene. 2009 – 9 feb. 2012", formatDateRange(es_ES, tz, fixedTime, fixedTime + 3 * YEAR, FORMAT_SHOW_DATE | FORMAT_ABBREV_ALL));
    assertEquals("ene. 2009 – feb. 2012", formatDateRange(es_ES, tz, fixedTime, fixedTime + 3 * YEAR, FORMAT_NO_MONTH_DAY | FORMAT_ABBREV_ALL));
    assertEquals("19 de enero de 2009–9 de febrero de 2012", formatDateRange(es_ES, tz, fixedTime, fixedTime + 3 * YEAR, 0));
    assertEquals("lunes, 19 de enero de 2009–jueves, 9 de febrero de 2012", formatDateRange(es_ES, tz, fixedTime, fixedTime + 3 * YEAR, FORMAT_SHOW_WEEKDAY));
  }

  // http://b/8862241 - we should be able to format dates past 2038.
  // See also http://code.google.com/p/android/issues/detail?id=13050.
  public void test8862241() throws Exception {
    ULocale l = ULocale.US;
    TimeZone tz = TimeZone.getTimeZone("America/Los_Angeles");
    Calendar c = Calendar.getInstance(tz, l);
    c.clear();
    c.set(2042, Calendar.JANUARY, 19, 3, 30);
    long jan_19_2042 = c.getTimeInMillis();
    c.set(2046, Calendar.OCTOBER, 4, 3, 30);
    long oct_4_2046 = c.getTimeInMillis();
    int flags = FORMAT_SHOW_DATE | FORMAT_ABBREV_ALL;
    assertEquals("Jan 19, 2042 – Oct 4, 2046", formatDateRange(l, tz, jan_19_2042, oct_4_2046, flags));
  }

  // http://b/10089890 - we should take the given time zone into account.
  public void test10089890() throws Exception {
    ULocale l = ULocale.US;
    TimeZone utc = TimeZone.getTimeZone("UTC");
    TimeZone pacific = TimeZone.getTimeZone("America/Los_Angeles");
    int flags = FORMAT_SHOW_DATE | FORMAT_ABBREV_ALL | FORMAT_SHOW_TIME | FORMAT_24HOUR;

    // The Unix epoch is UTC, so 0 is 1970-01-01T00:00Z...
    assertEquals("Jan 1, 1970, 00:00 – Jan 2, 1970, 00:00", formatDateRange(l, utc, 0, DAY + 1, flags));
    // But MTV is hours behind, so 0 was still the afternoon of the previous day...
    assertEquals("Dec 31, 1969, 16:00 – Jan 1, 1970, 16:00", formatDateRange(l, pacific, 0, DAY, flags));
  }

  // http://b/10318326 - we can drop the minutes in a 12-hour time if they're zero,
  // but not if we're using the 24-hour clock. That is: "4 PM" is reasonable, "16" is not.
  public void test10318326() throws Exception {
    long midnight = 0;
    long teaTime = 16 * HOUR;

    int time12 = FORMAT_12HOUR | FORMAT_SHOW_TIME;
    int time24 = FORMAT_24HOUR | FORMAT_SHOW_TIME;
    int abbr12 = time12 | FORMAT_ABBREV_ALL;
    int abbr24 = time24 | FORMAT_ABBREV_ALL;

    ULocale l = ULocale.US;
    TimeZone utc = TimeZone.getTimeZone("UTC");

    // Full length on-the-hour times.
    assertEquals("00:00", formatDateRange(l, utc, midnight, midnight, time24));
    assertEquals("12:00 AM", formatDateRange(l, utc, midnight, midnight, time12));
    assertEquals("16:00", formatDateRange(l, utc, teaTime, teaTime, time24));
    assertEquals("4:00 PM", formatDateRange(l, utc, teaTime, teaTime, time12));

    // Abbreviated on-the-hour times.
    assertEquals("00:00", formatDateRange(l, utc, midnight, midnight, abbr24));
    assertEquals("12 AM", formatDateRange(l, utc, midnight, midnight, abbr12));
    assertEquals("16:00", formatDateRange(l, utc, teaTime, teaTime, abbr24));
    assertEquals("4 PM", formatDateRange(l, utc, teaTime, teaTime, abbr12));

    // Abbreviated on-the-hour ranges.
    assertEquals("00:00 – 16:00", formatDateRange(l, utc, midnight, teaTime, abbr24));
    assertEquals("12 AM – 4 PM", formatDateRange(l, utc, midnight, teaTime, abbr12));

    // Abbreviated mixed ranges.
    assertEquals("00:00 – 16:01", formatDateRange(l, utc, midnight, teaTime + MINUTE, abbr24));
    assertEquals("12:00 AM – 4:01 PM", formatDateRange(l, utc, midnight, teaTime + MINUTE, abbr12));
  }

  // http://b/10560853 - when the time is not displayed, an end time 0 ms into the next day is
  // considered to belong to the previous day.
  public void test10560853_when_time_not_displayed() throws Exception {
    ULocale l = ULocale.US;
    TimeZone utc = TimeZone.getTimeZone("UTC");

    long midnight = 0;
    long midnightNext = 1 * DAY;

    int flags = FORMAT_SHOW_DATE | FORMAT_SHOW_WEEKDAY;

    // An all-day event runs until 0 milliseconds into the next day, but is formatted as if it's
    // just the first day.
    assertEquals("Thursday, January 1, 1970", formatDateRange(l, utc, midnight, midnightNext, flags));

    // Run one millisecond over, though, and you're into the next day.
    long nextMorning = 1 * DAY + 1;
    assertEquals("Thursday, January 1 – Friday, January 2, 1970", formatDateRange(l, utc, midnight, nextMorning, flags));

    // But the same reasoning applies for that day.
    long nextMidnight = 2 * DAY;
    assertEquals("Thursday, January 1 – Friday, January 2, 1970", formatDateRange(l, utc, midnight, nextMidnight, flags));
  }

  // http://b/10560853 - when the start and end times are otherwise on the same day,
  // an end time 0 ms into the next day is considered to belong to the previous day.
  public void test10560853_for_single_day_events() throws Exception {
    ULocale l = ULocale.US;
    TimeZone utc = TimeZone.getTimeZone("UTC");

    int flags = FORMAT_SHOW_TIME | FORMAT_24HOUR | FORMAT_SHOW_DATE;

    assertEquals("January 1, 1970, 22:00 – 00:00", formatDateRange(l, utc, 22 * HOUR, 24 * HOUR, flags));
    assertEquals("January 1, 1970, 22:00 – January 2, 1970, 00:30", formatDateRange(l, utc, 22 * HOUR, 24 * HOUR + 30 * MINUTE, flags));
  }

  // The fix for http://b/10560853 didn't work except for the day around the epoch, which was
  // all the unit test checked!
  public void test_single_day_events_later_than_epoch() throws Exception {
    ULocale l = ULocale.US;
    TimeZone utc = TimeZone.getTimeZone("UTC");

    int flags = FORMAT_SHOW_TIME | FORMAT_24HOUR | FORMAT_SHOW_DATE;

    Calendar c = Calendar.getInstance(utc, l);
    c.clear();
    c.set(1980, Calendar.JANUARY, 1, 0, 0);
    long jan_1_1980 = c.getTimeInMillis();
    assertEquals("January 1, 1980, 22:00 – 00:00",
                 formatDateRange(l, utc, jan_1_1980 + 22 * HOUR, jan_1_1980 + 24 * HOUR, flags));
    assertEquals("January 1, 1980, 22:00 – January 2, 1980, 00:30",
                 formatDateRange(l, utc, jan_1_1980 + 22 * HOUR, jan_1_1980 + 24 * HOUR + 30 * MINUTE, flags));
  }

  // The fix for http://b/10560853 didn't work except for UTC, which was
  // all the unit test checked!
  public void test_single_day_events_not_in_UTC() throws Exception {
    ULocale l = ULocale.US;
    TimeZone pacific = TimeZone.getTimeZone("America/Los_Angeles");

    int flags = FORMAT_SHOW_TIME | FORMAT_24HOUR | FORMAT_SHOW_DATE;

    Calendar c = Calendar.getInstance(pacific, l);
    c.clear();
    c.set(1980, Calendar.JANUARY, 1, 0, 0);
    long jan_1_1980 = c.getTimeInMillis();
    assertEquals("January 1, 1980, 22:00 – 00:00",
                 formatDateRange(l, pacific, jan_1_1980 + 22 * HOUR, jan_1_1980 + 24 * HOUR, flags));

    c.set(1980, Calendar.JULY, 1, 0, 0);
    long jul_1_1980 = c.getTimeInMillis();
    assertEquals("July 1, 1980, 22:00 – 00:00",
                 formatDateRange(l, pacific, jul_1_1980 + 22 * HOUR, jul_1_1980 + 24 * HOUR, flags));
  }

  // http://b/10209343 - even if the caller didn't explicitly ask us to include the year,
  // we should do so for years other than the current year.
  public void test10209343_when_not_this_year() {
    ULocale l = ULocale.US;
    TimeZone utc = TimeZone.getTimeZone("UTC");

    int flags = FORMAT_SHOW_DATE | FORMAT_SHOW_WEEKDAY | FORMAT_SHOW_TIME | FORMAT_24HOUR;

    assertEquals("Thursday, January 1, 1970, 00:00", formatDateRange(l, utc, 0L, 0L, flags));

    long t1833 = ((long) Integer.MIN_VALUE + Integer.MIN_VALUE) * 1000L;
    assertEquals("Sunday, November 24, 1833, 17:31", formatDateRange(l, utc, t1833, t1833, flags));

    long t1901 = Integer.MIN_VALUE * 1000L;
    assertEquals("Friday, December 13, 1901, 20:45", formatDateRange(l, utc, t1901, t1901, flags));

    long t2038 = Integer.MAX_VALUE * 1000L;
    assertEquals("Tuesday, January 19, 2038, 03:14", formatDateRange(l, utc, t2038, t2038, flags));

    long t2106 = (2L + Integer.MAX_VALUE + Integer.MAX_VALUE) * 1000L;
    assertEquals("Sunday, February 7, 2106, 06:28", formatDateRange(l, utc, t2106, t2106, flags));
  }

  // http://b/10209343 - for the current year, we should honor the FORMAT_SHOW_YEAR flags.
  public void test10209343_when_this_year() {
    // Construct a date in the current year (whenever the test happens to be run).
    ULocale l = ULocale.US;
    TimeZone utc = TimeZone.getTimeZone("UTC");
    Calendar c = Calendar.getInstance(utc, l);
    c.set(Calendar.MONTH, Calendar.FEBRUARY);
    c.set(Calendar.DAY_OF_MONTH, 10);
    c.set(Calendar.HOUR_OF_DAY, 0);
    long thisYear = c.getTimeInMillis();

    // You don't get the year if it's this year...
    assertEquals("February 10", formatDateRange(l, utc, thisYear, thisYear, FORMAT_SHOW_DATE));

    // ...unless you explicitly ask for it.
    assertEquals(String.format("February 10, %d", c.get(Calendar.YEAR)),
                 formatDateRange(l, utc, thisYear, thisYear, FORMAT_SHOW_DATE | FORMAT_SHOW_YEAR));

    // ...or it's not actually this year...
    Calendar c2 = (Calendar) c.clone();
    c2.set(Calendar.YEAR, 1980);
    long oldYear = c2.getTimeInMillis();
    assertEquals("February 10, 1980", formatDateRange(l, utc, oldYear, oldYear, FORMAT_SHOW_DATE));

    // (But you can disable that!)
    assertEquals("February 10", formatDateRange(l, utc, oldYear, oldYear, FORMAT_SHOW_DATE | FORMAT_NO_YEAR));

    // ...or the start and end years aren't the same...
    assertEquals(String.format("February 10, 1980 – February 10, %d", c.get(Calendar.YEAR)),
                 formatDateRange(l, utc, oldYear, thisYear, FORMAT_SHOW_DATE));

    // (And you can't avoid that --- icu4c steps in and overrides you.)
    assertEquals(String.format("February 10, 1980 – February 10, %d", c.get(Calendar.YEAR)),
                 formatDateRange(l, utc, oldYear, thisYear, FORMAT_SHOW_DATE | FORMAT_NO_YEAR));
  }

  // http://b/8467515 - yet another y2k38 bug report.
  public void test8467515() throws Exception {
    ULocale l = ULocale.US;
    TimeZone utc = TimeZone.getTimeZone("UTC");
    int flags = FORMAT_SHOW_DATE | FORMAT_SHOW_WEEKDAY | FORMAT_SHOW_YEAR | FORMAT_ABBREV_MONTH | FORMAT_ABBREV_WEEKDAY;
    long t;

    Calendar calendar = Calendar.getInstance(utc, l);
    calendar.clear();

    calendar.set(2038, Calendar.JANUARY, 19, 12, 0, 0);
    t = calendar.getTimeInMillis();
    assertEquals("Tue, Jan 19, 2038", formatDateRange(l, utc, t, t, flags));

    calendar.set(1900, Calendar.JANUARY, 1, 0, 0, 0);
    t = calendar.getTimeInMillis();
    assertEquals("Mon, Jan 1, 1900", formatDateRange(l, utc, t, t, flags));
  }

  // http://b/12004664
  public void test12004664() throws Exception {
    TimeZone utc = TimeZone.getTimeZone("UTC");
    Calendar c = Calendar.getInstance(utc, ULocale.US);
    c.clear();
    c.set(Calendar.YEAR, 1980);
    c.set(Calendar.MONTH, Calendar.FEBRUARY);
    c.set(Calendar.DAY_OF_MONTH, 10);
    c.set(Calendar.HOUR_OF_DAY, 0);
    long thisYear = c.getTimeInMillis();

    int flags = FORMAT_SHOW_DATE | FORMAT_SHOW_WEEKDAY | FORMAT_SHOW_YEAR;
    assertEquals("Sunday, February 10, 1980", formatDateRange(new ULocale("en", "US"), utc, thisYear, thisYear, flags));

    // If we supported non-Gregorian calendars, this is what that we'd expect for these ULocales.
    // This is really the correct behavior, but since java.util.Calendar currently only supports
    // the Gregorian calendar, we want to deliberately force icu4c to agree, otherwise we'd have
    // a mix of calendars throughout an app's UI depending on whether Java or native code formatted
    // the date.
    // assertEquals("یکشنبه ۲۱ بهمن ۱۳۵۸ ه‍.ش.", formatDateRange(new ULocale("fa"), utc, thisYear, thisYear, flags));
    // assertEquals("AP ۱۳۵۸ سلواغه ۲۱, یکشنبه", formatDateRange(new ULocale("ps"), utc, thisYear, thisYear, flags));
    // assertEquals("วันอาทิตย์ 10 กุมภาพันธ์ 2523", formatDateRange(new ULocale("th"), utc, thisYear, thisYear, flags));

    // For now, here are the localized Gregorian strings instead...
    assertEquals("یکشنبه ۱۰ فوریهٔ ۱۹۸۰", formatDateRange(new ULocale("fa"), utc, thisYear, thisYear, flags));
    assertEquals("يونۍ د ۱۹۸۰ د فبروري ۱۰", formatDateRange(new ULocale("ps"), utc, thisYear, thisYear, flags));
    assertEquals("วันอาทิตย์ที่ 10 กุมภาพันธ์ ค.ศ. 1980", formatDateRange(new ULocale("th"), utc, thisYear, thisYear, flags));
  }

  // http://b/13234532
  public void test13234532() throws Exception {
    ULocale l = ULocale.US;
    TimeZone utc = TimeZone.getTimeZone("UTC");

    int flags = FORMAT_SHOW_TIME | FORMAT_ABBREV_ALL | FORMAT_12HOUR;

    assertEquals("10 – 11 AM", formatDateRange(l, utc, 10*HOUR, 11*HOUR, flags));
    assertEquals("11 AM – 1 PM", formatDateRange(l, utc, 11*HOUR, 13*HOUR, flags));
    assertEquals("2 – 3 PM", formatDateRange(l, utc, 14*HOUR, 15*HOUR, flags));
  }

  // http://b/20708022
  public void testEndOfDayOnLastDayOfMonth() throws Exception {
    final ULocale locale = new ULocale("en");
    final TimeZone timeZone = TimeZone.getTimeZone("UTC");

    assertEquals("11:00 PM – 12:00 AM", formatDateRange(locale, timeZone,
            1430434800000L, 1430438400000L, FORMAT_SHOW_TIME));
  }

  // http://b/68847519
  public void testEndAtMidnight() {
    ULocale locale = new ULocale("en");
    TimeZone timeZone = TimeZone.getTimeZone("UTC");
    int dateTimeFlags = FORMAT_SHOW_DATE | FORMAT_SHOW_TIME | FORMAT_24HOUR;
    // If we're showing times and the end-point is midnight the following day, we want the
    // behaviour of suppressing the date for the end...
    assertEquals("February 27, 04:00 – 00:00",
        formatDateRange(locale, timeZone, 1519704000000L, 1519776000000L, dateTimeFlags));
    // ...unless the start-point is also midnight, in which case we need dates to disambiguate.
    assertEquals("February 27, 00:00 – February 28, 00:00",
        formatDateRange(locale, timeZone, 1519689600000L, 1519776000000L, dateTimeFlags));
    // We want to show the date if the end-point is a millisecond after midnight the following
    // day, or if it is exactly midnight the day after that.
    assertEquals("February 27, 04:00 – February 28, 00:00",
        formatDateRange(locale, timeZone, 1519704000000L, 1519776000001L, dateTimeFlags));
    assertEquals("February 27, 04:00 – March 1, 00:00",
        formatDateRange(locale, timeZone, 1519704000000L, 1519862400000L, dateTimeFlags));
    // We want to show the date if the start-point is anything less than a minute after midnight,
    // since that gets displayed as midnight...
    assertEquals("February 27, 00:00 – February 28, 00:00",
        formatDateRange(locale, timeZone, 1519689659999L, 1519776000000L, dateTimeFlags));
    // ...but not if it is exactly one minute after midnight.
    assertEquals("February 27, 00:01 – 00:00",
        formatDateRange(locale, timeZone, 1519689660000L, 1519776000000L, dateTimeFlags));
    int dateOnlyFlags = FORMAT_SHOW_DATE;
    // If we're only showing dates and the end-point is midnight of any day, we want the
    // behaviour of showing an end date one earlier. So if the end-point is March 2, 00:00, show
    // March 1 instead (whether the start-point is midnight or not).
    assertEquals("February 27 – March 1",
        formatDateRange(locale, timeZone, 1519689600000L, 1519948800000L, dateOnlyFlags));
    assertEquals("February 27 – March 1",
        formatDateRange(locale, timeZone, 1519704000000L, 1519948800000L, dateOnlyFlags));
    // We want to show the true date if the end-point is a millisecond after midnight.
    assertEquals("February 27 – March 2",
        formatDateRange(locale, timeZone, 1519689600000L, 1519948800001L, dateOnlyFlags));
  }
}
