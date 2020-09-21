/* GENERATED SOURCE. DO NOT MODIFY. */
// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html#License
/*
 *******************************************************************************
 * Copyright (C) 1996-2012, International Business Machines Corporation and    *
 * others. All Rights Reserved.                                                *
 *******************************************************************************
 *
 */

package android.icu.dev.test.serializable;

import java.util.Locale;

import android.icu.util.BuddhistCalendar;
import android.icu.util.Calendar;
import android.icu.util.ChineseCalendar;
import android.icu.util.CopticCalendar;
import android.icu.util.DangiCalendar;
import android.icu.util.EthiopicCalendar;
import android.icu.util.GregorianCalendar;
import android.icu.util.HebrewCalendar;
import android.icu.util.IndianCalendar;
import android.icu.util.IslamicCalendar;
import android.icu.util.JapaneseCalendar;
import android.icu.util.PersianCalendar;
import android.icu.util.TaiwanCalendar;
import android.icu.util.TimeZone;
import android.icu.util.ULocale;

/**
 * @author emader
 *
 */
public abstract class CalendarHandler implements SerializableTestUtility.Handler
{
    @Override
    public boolean hasSameBehavior(Object a, Object b)
    {
        Calendar cal_a = (Calendar) a;
        Calendar cal_b = (Calendar) b;

        // Make sure tzid is preserved
        TimeZone tz_a = cal_a.getTimeZone();
        TimeZone tz_b = cal_b.getTimeZone();

        if (!tz_a.getID().equals(tz_b.getID())) {
            return false;
        }

        // The deserialized TimeZone may have different rule.
        // To compare the behavior, we need to use the same
        // TimeZone.
        cal_a.setTimeZone(tz_b);

        long now = System.currentTimeMillis();

        cal_a.setTimeInMillis(now);
        cal_a.roll(Calendar.MONTH, 1);

        cal_b.setTimeInMillis(now);
        cal_b.roll(Calendar.MONTH, 1);

        return cal_a.getTime().equals(cal_b.getTime());
    }

    static class BasicCalendarHandler extends CalendarHandler
    {
        @Override
        public Object[] getTestObjects()
        {
            Locale locales[] = SerializableTestUtility.getLocales();
            TimeZone pst = TimeZone.getTimeZone("America/Los_Angeles");
            Calendar calendars[] = new Calendar[locales.length];

            for (int i = 0; i < locales.length; i += 1) {
                calendars[i] = Calendar.getInstance(pst, locales[i]);
            }

            return calendars;
        }
    }

    static class BuddhistCalendarHandler extends CalendarHandler
    {
        @Override
        public Object[] getTestObjects()
        {
            Locale locales[] = SerializableTestUtility.getLocales();
            TimeZone tst = TimeZone.getTimeZone("Asia/Bangkok");
            BuddhistCalendar calendars[] = new BuddhistCalendar[locales.length];

            for (int i = 0; i < locales.length; i += 1) {
                calendars[i] = new BuddhistCalendar(tst, locales[i]);
            }

            return calendars;
        }
    }

    static class ChineseCalendarHandler extends CalendarHandler
    {
        @Override
        public Object[] getTestObjects()
        {
            Locale locales[] = SerializableTestUtility.getLocales();
            TimeZone cst = TimeZone.getTimeZone("Asia/Shanghai");
            ChineseCalendar calendars[] = new ChineseCalendar[locales.length];

            for (int i = 0; i < locales.length; i += 1) {
                calendars[i] = new ChineseCalendar(cst, locales[i]);
            }

            return calendars;
        }
    }

    static class CopticCalendarHandler extends CalendarHandler
    {
        @Override
        public Object[] getTestObjects()
        {
            Locale locales[] = SerializableTestUtility.getLocales();
            TimeZone ast = TimeZone.getTimeZone("Europe/Athens");
            CopticCalendar calendars[] = new CopticCalendar[locales.length];

            for (int i = 0; i < locales.length; i += 1) {
                calendars[i] = new CopticCalendar(ast, locales[i]);
            }

            return calendars;
        }
    }

    static class DangiCalendarHandler extends CalendarHandler
    {
        @Override
        public Object[] getTestObjects()
        {
            Locale locales[] = SerializableTestUtility.getLocales();
            TimeZone kst = TimeZone.getTimeZone("Asia/Seoul");
            DangiCalendar calendars[] = new DangiCalendar[locales.length];

            for (int i = 0; i < locales.length; i += 1) {
                calendars[i] = new DangiCalendar(kst, ULocale.forLocale(locales[i]));
            }

            return calendars;
        }
    }

    static class EthiopicCalendarHandler extends CalendarHandler
    {
        @Override
        public Object[] getTestObjects()
        {
            Locale locales[] = SerializableTestUtility.getLocales();
            TimeZone ast = TimeZone.getTimeZone("Africa/Addis_Ababa");
            EthiopicCalendar calendars[] = new EthiopicCalendar[locales.length];

            for (int i = 0; i < locales.length; i += 1) {
                calendars[i] = new EthiopicCalendar(ast, locales[i]);
            }

            return calendars;
        }
    }

    static class GregorianCalendarHandler extends CalendarHandler
    {
        @Override
        public Object[] getTestObjects()
        {
            Locale locales[] = SerializableTestUtility.getLocales();
            TimeZone pst = TimeZone.getTimeZone("America/Los_Angeles");
            GregorianCalendar calendars[] = new GregorianCalendar[locales.length];

            for (int i = 0; i < locales.length; i += 1) {
                calendars[i] = new GregorianCalendar(pst, locales[i]);
            }

            return calendars;
        }
    }

    static class HebrewCalendarHandler extends CalendarHandler
    {
        @Override
        public Object[] getTestObjects()
        {
            Locale locales[] = SerializableTestUtility.getLocales();
            TimeZone jst = TimeZone.getTimeZone("Asia/Jerusalem");
            HebrewCalendar calendars[] = new HebrewCalendar[locales.length];

            for (int i = 0; i < locales.length; i += 1) {
                calendars[i] = new HebrewCalendar(jst, locales[i]);
            }

            return calendars;
        }
    }

    static class IndianCalendarHandler extends CalendarHandler
    {
        @Override
        public Object[] getTestObjects()
        {
            Locale locales[] = SerializableTestUtility.getLocales();
            TimeZone jst = TimeZone.getTimeZone("Asia/Calcutta");
            IndianCalendar calendars[] = new IndianCalendar[locales.length];

            for (int i = 0; i < locales.length; i += 1) {
                calendars[i] = new IndianCalendar(jst, locales[i]);
            }

            return calendars;
        }
    }

    static class IslamicCalendarHandler extends CalendarHandler
    {
        @Override
        public Object[] getTestObjects() {
            Locale locales[] = SerializableTestUtility.getLocales();
            TimeZone cst = TimeZone.getTimeZone("Africa/Cairo");
            IslamicCalendar calendars[] = new IslamicCalendar[locales.length];

            for (int i = 0; i < locales.length; i += 1) {
                calendars[i] = new IslamicCalendar(cst, locales[i]);
            }

            return calendars;
        }
    }

    static class JapaneseCalendarHandler extends CalendarHandler
    {
        @Override
        public Object[] getTestObjects()
        {
            Locale locales[] = SerializableTestUtility.getLocales();
            TimeZone jst = TimeZone.getTimeZone("Asia/Tokyo");
            JapaneseCalendar calendars[] = new JapaneseCalendar[locales.length];

            for (int i = 0; i < locales.length; i += 1) {
                calendars[i] = new JapaneseCalendar(jst, locales[i]);
            }

            return calendars;
        }
    }

    static class PersianCalendarHandler extends CalendarHandler
    {
        @Override
        public Object[] getTestObjects()
        {
            Locale locales[] = SerializableTestUtility.getLocales();
            TimeZone kst = TimeZone.getTimeZone("Asia/Tehran");
            PersianCalendar calendars[] = new PersianCalendar[locales.length];

            for (int i = 0; i < locales.length; i += 1) {
                calendars[i] = new PersianCalendar(kst, ULocale.forLocale(locales[i]));
            }

            return calendars;
        }
    }

    static class TaiwanCalendarHandler extends CalendarHandler {
        @Override
        public Object[] getTestObjects() {
            Locale locales[] = SerializableTestUtility.getLocales();
            TimeZone cst = TimeZone.getTimeZone("Asia/Shanghai");
            TaiwanCalendar calendars[] = new TaiwanCalendar[locales.length];

            for (int i = 0; i < locales.length; i += 1) {
                calendars[i] = new TaiwanCalendar(cst, locales[i]);
            }

            return calendars;
        }
    }
}
