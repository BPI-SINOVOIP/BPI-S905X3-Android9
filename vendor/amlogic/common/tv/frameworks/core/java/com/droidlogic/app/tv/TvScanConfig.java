/*
 * Copyright (c) 2018 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: JAVA file
 */

package com.droidlogic.app.tv;

import java.util.HashMap;
import android.util.Log;
import android.text.TextUtils;
import java.util.ArrayList;

/**
 * Created by yu.fang on 2018/7/9.
 */

public class TvScanConfig {
    private static String TAG = "TvScanConfig";
    private boolean mDebug = true;

    public static final int TV_SEARCH_MANUAL_INDEX      = 0;
    public static final int TV_SEARCH_AUTO_INDEX        = 1;
    public static final int TV_SEARCH_NUMBER_INDEX      = 2;
    public static final int TV_SEARCH_NIT_INDEX         = 3;
    public static final ArrayList<String> TV_SEARCH_MODE = new ArrayList<String>(){{add("manual"); add("auto"); add("number"); add("nit");}};

    public static final int TV_SEARCH_TYPE_ATSC_C_AUTO_INDEX    = 0;
    public static final int TV_SEARCH_TYPE_ATSC_C_HRC_INDEX     = 1;
    public static final int TV_SEARCH_TYPE_ATSC_C_LRC_INDEX     = 2;
    public static final int TV_SEARCH_TYPE_ATSC_C_STD_INDEX     = 3;
    public static final int TV_SEARCH_TYPE_ATSC_T_INDEX         = 4;
    public static final int TV_SEARCH_TYPE_ATV_INDEX            = 5;
    public static final int TV_SEARCH_TYPE_DTMB_INDEX           = 6;
    public static final int TV_SEARCH_TYPE_DVBC_INDEX           = 7;
    public static final int TV_SEARCH_TYPE_DVBS_INDEX           = 8;
    public static final int TV_SEARCH_TYPE_DVBT_INDEX           = 9;
    public static final int TV_SEARCH_TYPE_ISDBT_INDEX          = 10;
    public static final ArrayList<String> TV_SEARCH_TYPE = new ArrayList<String>() {
        {
            add("ATSC-C-AUTO"); add("ATSC-C-HRC"); add("ATSC-C-LRC"); add("ATSC-C-STD"); add("ATSC-T"); add("ATV");
            add("DTMB"); add("DVB-C"); add("DVB-S"); add("DVB-T"); add("ISDB-T");
        }
    };

    //definition about country
    public static final int TV_COUNTRY_AMERICA_INDEX    = 0;
    public static final int TV_COUNTRY_INDIA_INDEX      = 1;
    public static final int TV_COUNTRY_INDONESIA_INDEX  = 2;
    public static final int TV_COUNTRY_MEXICO_INDEX     = 3;
    public static final int TV_COUNTRY_GERMANY_INDEX    = 4;
    public static final int TV_COUNTRY_CHINA_INDEX      = 5;
    public static final int TV_COUNTRY_BRAZIL_INDEX     = 6;
    public static final int TV_COUNTRY_FRANCE_INDEX     = 7;
    public static final ArrayList<String> TV_COUNTRY = new ArrayList<String>(){{add("US"); add("IN"); add("ID"); add("MX"); add("DE"); add("CN"); add("BR"); add("FR");}};

    public static final int TV_COLOR_SYS_AUTO_INDEX     = 0;
    public static final int TV_COLOR_SYS_PAL_INDEX      = 1;
    public static final int TV_COLOR_SYS_NTSC_INDEX     = 2;
    public static final int TV_COLOR_SYS_SECAM_INDEX    = 3;
    public static final ArrayList<String> TV_COLOR_SYS = new ArrayList<String>(){{add("AUTO"); add("PAL"); add("NTSC"); add("SECAM");}};

    public static final int TV_SOUND_SYS_AUTO_INDEX     = 0;
    public static final int TV_SOUND_SYS_DK_INDEX       = 1;
    public static final int TV_SOUND_SYS_I_INDEX        = 2;
    public static final int TV_SOUND_SYS_BG_INDEX       = 3;
    public static final int TV_SOUND_SYS_M_INDEX        = 4;
    public static final int TV_SOUND_SYS_L_INDEX        = 5;
    public static final ArrayList<String> TV_SOUND_SYS = new ArrayList<String>(){{add("AUTO"); add("DK"); add("I"); add("BG"); add("M"); add("L");}};

    public static final ArrayList<String> TV_MIN_MAX_FREQ = new ArrayList<String>(){{add("44250000"); add("868250000");}};

    public static final int TV_ATV_AUTO_FREQ_LIST        = 0; /* 0: freq table list sacn mode */
    public static final int TV_ATV_AUTO_ALL_BAND         = 1;  /* 1: all band sacn mode */

    public static ArrayList<String> stringToWordsList(String strings, ArrayList<String> defaultStrings){
        if (!TextUtils.isEmpty(strings)) {
            ArrayList<String> getSupportList = new ArrayList<String>();
            String[] supportList = strings.split(",");
            for (String temp : supportList) {
                // print duplicate values
                if (getSupportList.contains(temp)) {
                    Log.w(TAG, "The element:" + temp + ", already exists in current list");
                }
                getSupportList.add(temp);
            }
            // Check the validity of the element
            for (String temp : getSupportList) {
                if (!defaultStrings.contains(temp)) {
                    Log.w(TAG, "not support value:" + temp + ", element deleted");
                    getSupportList.remove(temp);
                }
            }
            if (0 == getSupportList.size()) {
                Log.w(TAG, "support list size is 0, use defaultList");
                return defaultStrings;
            }
            return getSupportList;
        } else {
            Log.e(TAG, "strings is null in *.conf, use default list value");
            return defaultStrings;
        }
    }

    public static ArrayList<String> GetTVSupportCountries() {
        String countryStrings = TvControlManager.getInstance().GetTVSupportCountries();

        return stringToWordsList(countryStrings, TV_COUNTRY);
    }

    public static String getTvDefaultCountry() {
        String country = TvControlManager.getInstance().getTvDefaultCountry();
        if (country.isEmpty()) {
            Log.e(TAG, "get default country is null in *.conf, use default value China");
            country = TV_COUNTRY.get(TV_COUNTRY_CHINA_INDEX);
        }
        return country;
    }

    public static String GetTvCountryNameById(String countryId) {
        return TvControlManager.getInstance().GetTvCountryNameById(countryId);
    }

    public static ArrayList<String> GetTvSearchModeList(String countryId) {
        return stringToWordsList(TvControlManager.getInstance().GetTvSearchMode(countryId), TV_SEARCH_MODE);
    }

    public static boolean GetTvDtvSupport(String countryId) {
        return TvControlManager.getInstance().GetTvDtvSupport(countryId);
    }

    public static ArrayList<String> GetTvDtvSystemList(String countryId) {
        return stringToWordsList(TvControlManager.getInstance().GetTvDtvSystem(countryId), TV_SEARCH_TYPE);
    }

    public static boolean GetTvAtvSupport(String countryId) {
        return TvControlManager.getInstance().GetTvAtvSupport(countryId);
    }

    public static ArrayList<String> GetTvAtvColorSystemList(String countryId) {
        return stringToWordsList(TvControlManager.getInstance().GetTvAtvColorSystem(countryId), TV_COLOR_SYS);
    }

    public static ArrayList<String> GetTvAtvSoundSystemList(String countryId) {
        return stringToWordsList(TvControlManager.getInstance().GetTvAtvSoundSystem(countryId), TV_SOUND_SYS);
    }

    public static int GetTvAtvMinMaxFreq(String countryId, int param[]) {
        String strings = TvControlManager.getInstance().GetTvAtvMinMaxFreq(countryId);
        String[] supportList = strings.split(",");
        if (supportList.length != 2) {
            Log.e(TAG, "get atv search freq param length= " + supportList.length + "error, use default freq [" + TV_MIN_MAX_FREQ.get(0) + ", " + TV_MIN_MAX_FREQ.get(1) + "]");
            param[0] = Integer.parseInt(TV_MIN_MAX_FREQ.get(0));
            param[1] = Integer.parseInt(TV_MIN_MAX_FREQ.get(1));
        } else {
            param[0] = Integer.parseInt(supportList[0]);
            param[1] = Integer.parseInt(supportList[1]);
        }
        return 0;
    }

    public static int GetTvAtvStepScan(String country_code) {
        return TvControlManager.getInstance().GetTvAtvStepScan(country_code) ? TV_ATV_AUTO_ALL_BAND : TV_ATV_AUTO_FREQ_LIST;
    }
}
