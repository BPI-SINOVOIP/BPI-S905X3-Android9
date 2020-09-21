package com.droidlogic.settings;

import android.util.Log;

import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.TimeZone;

public class ConvertSettingManager {

    private static final String TAG = "ConvertSettingManager";
    private static final boolean DEBUG = true;

    public static String convertLongToDate(long time) {
        String result = null;
        SimpleDateFormat formatter = new SimpleDateFormat("yyyy.MM.dd HH:mm:ss");
        formatter.setTimeZone(TimeZone.getDefault());
        result = formatter.format(new Date(time));
        if (DEBUG) {
            Log.d(TAG, "convertLongToDate = " + result + ", time = " + time);
        }
        return result;
    }
}
