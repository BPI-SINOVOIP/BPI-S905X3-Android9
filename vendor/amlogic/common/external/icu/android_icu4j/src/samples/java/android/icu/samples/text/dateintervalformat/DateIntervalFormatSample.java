/* GENERATED SOURCE. DO NOT MODIFY. */
// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html#License
/*
 *******************************************************************************
 * Copyright (C) 2013, International Business Machines Corporation and         *
 * others. All Rights Reserved.                                                *
 *******************************************************************************
 */
package android.icu.samples.text.dateintervalformat;


// IGNORED_INCLUDE(dtitvfmtPreDefinedExample)
import java.util.Date;

import android.icu.text.DateFormat;
import android.icu.text.DateIntervalFormat;
import android.icu.text.DateIntervalInfo;
import android.icu.util.Calendar;
import android.icu.util.DateInterval;
import android.icu.util.GregorianCalendar;
import android.icu.util.ULocale;
// IGNORED_INCLUDE(dtitvfmtPreDefinedExample)
public class DateIntervalFormatSample{

    public static void main (String[] args){

        dtitvfmtPreDefined();
        dtitvfmtCustomized();

    }

    public static void dtitvfmtPreDefined()
    {
        System.out.println("==============================================================================");
        System.out.println(" dtitvfmtPreDefined()");
        System.out.println();
        System.out.println(" Use DateIntervalFormat to get Date interval format for pre-defined skeletons:");
        System.out.println(" yMMMd, MMMd per locale");
        System.out.println("==============================================================================");
        // BEGIN_INCLUDE(dtitvfmtPreDefinedExample)
        final Date date[] = {
                new GregorianCalendar(2007,10,10,10,10,10).getTime(),
                new GregorianCalendar(2008,10,10,10,10,10).getTime(),
                new GregorianCalendar(2008,11,10,10,10,10).getTime(),
                new GregorianCalendar(2008,11,10,15,10,10).getTime(),
        };
        final DateInterval dtitv[] = {
                new DateInterval(date[0].getTime(),date[1].getTime()),
                new DateInterval(date[1].getTime(),date[2].getTime()),
                new DateInterval(date[2].getTime(),date[3].getTime()),
        };
        final String [] skeletons = {
                DateFormat.YEAR_ABBR_MONTH_DAY,
                DateFormat.MONTH_DAY,
                DateFormat.HOUR_MINUTE,
                };
        System.out.printf("%-15s%-35s%-35s%-35s%-35s\n", "Skeleton", "from","to","Date Interval in en_US", "Date Interval in Ja");
        int i=0;
        for (String skeleton:skeletons) {
        System.out.printf("%-15s%-35s%-35s", skeleton,date[i].toString(), date[i+1].toString());
                DateIntervalFormat dtitvfmtEn = DateIntervalFormat.getInstance(skeleton, ULocale.ENGLISH);
                DateIntervalFormat dtitvfmtJa = DateIntervalFormat.getInstance(skeleton, ULocale.JAPANESE);
                System.out.printf("%-35s%-35s\n", dtitvfmtEn.format(dtitv[i]),dtitvfmtJa.format(dtitv[i]));
                i++;
        }
        /** output of the sample code:
         *********************************************************************************************************************************************************
         Skeleton       from                               to                                 Date Interval in en_US             Date Interval in Ja
         yMMMd          Sat Nov 10 10:10:10 EST 2007       Mon Nov 10 10:10:10 EST 2008       Nov 10, 2007 – Nov 10, 2008        2007年11月10日～2008年11月10日 
         MMMMd          Mon Nov 10 10:10:10 EST 2008       Wed Dec 10 10:10:10 EST 2008       November 10 – December 10          11月10日～12月10日 
         jm             Wed Dec 10 10:10:10 EST 2008       Wed Dec 10 15:10:10 EST 2008       10:10 AM – 3:10 PM                 10:10～15:10

         *********************************************************************************************************************************************************/
        // END_INCLUDE(dtitvfmtPreDefinedExample)
}
    public static void dtitvfmtCustomized()
    {
        System.out.println("================================================================================");
        System.out.println(" dtitvfmtCustomized()");
        System.out.println();
        System.out.println(" Use DateIntervalFormat to create customized date interval format for yMMMd, Hm");
        System.out.println("================================================================================");
        // BEGIN_INCLUDE(dtitvfmtCustomizedExample)
        final Date date[] = {
                new GregorianCalendar(2007,9,10,10,10,10).getTime(),
                new GregorianCalendar(2007,10,10,10,10,10).getTime(),
                new GregorianCalendar(2007,10,10,22,10,10).getTime(),
        };
        final DateInterval dtitv[] = {
                new DateInterval(date[0].getTime(),date[1].getTime()),
                new DateInterval(date[1].getTime(),date[2].getTime()),
        };
        final String [] skeletons = {
                DateFormat.YEAR_ABBR_MONTH_DAY,
                DateFormat.HOUR24_MINUTE,
                };
        System.out.printf("%-15s%-35s%-35s%-45s%-35s\n", "Skeleton", "from","to", "Date Interval in en_US", "Date Interval in Ja");
        // Create an empty DateIntervalInfo object
        DateIntervalInfo dtitvinf = new DateIntervalInfo(ULocale.ENGLISH);
        // Set Date Time internal pattern for MONTH, DAY_OF_MONTH, HOUR_OF_DAY
        dtitvinf.setIntervalPattern("yMMMd", Calendar.MONTH, "y 'Diff' MMM d --- MMM d");
        dtitvinf.setIntervalPattern("Hm", Calendar.HOUR_OF_DAY, "yyyy MMM d HH:mm ~ HH:mm");
        // Set fallback interval pattern
        dtitvinf.setFallbackIntervalPattern("{0} ~~~ {1}");
        // Get the DateIntervalFormat with the custom pattern
        for (String skeleton:skeletons){
            for (int i=0;i<2;i++) {
            System.out.printf("%-15s%-35s%-35s", skeleton,date[i].toString(), date[i+1].toString());
            DateIntervalFormat dtitvfmtEn = DateIntervalFormat.getInstance(skeleton,ULocale.ENGLISH,dtitvinf);
            DateIntervalFormat dtitvfmtJa = DateIntervalFormat.getInstance(skeleton,ULocale.JAPANESE,dtitvinf);
            System.out.printf("%-45s%-35s\n", dtitvfmtEn.format(dtitv[i]),dtitvfmtJa.format(dtitv[i]));
            }
       }
        /** output of the sample code:
         *************************************************************************************************************************************************************************
          Skeleton       from                               to                                 Date Interval in en_US                       Date Interval in Ja
          yMMMd          Wed Oct 10 10:10:10 EDT 2007       Sat Nov 10 10:10:10 EST 2007       2007 Diff Oct 10 --- Nov 10                  2007 Diff 10月 10 --- 11月 10
          yMMMd          Sat Nov 10 10:10:10 EST 2007       Sat Nov 10 22:10:10 EST 2007       Nov 10, 2007                                 2007年11月10日
          Hm             Wed Oct 10 10:10:10 EDT 2007       Sat Nov 10 10:10:10 EST 2007       10/10/2007, 10:10 ~~~ 11/10/2007, 10:10      2007/10/10 10:10 ~~~ 2007/11/10 10:10
          Hm             Sat Nov 10 10:10:10 EST 2007       Sat Nov 10 22:10:10 EST 2007       2007 Nov 10 10:10 ~ 22:10                    2007 11月 10 10:10 ~ 22:10
         *************************************************************************************************************************************************************************/
      // END_INCLUDE(dtitvfmtCustomizedExample)

    }
}

