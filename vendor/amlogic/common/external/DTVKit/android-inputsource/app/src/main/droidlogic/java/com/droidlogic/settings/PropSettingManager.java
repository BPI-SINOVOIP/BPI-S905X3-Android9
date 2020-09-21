package com.droidlogic.settings;

import android.util.Log;

import java.lang.reflect.Method;

import com.droidlogic.app.SystemControlManager;

public class PropSettingManager {

    private static final String TAG = "PropSettingManager";
    private static final boolean DEBUG = true;

    public static final String TV_STREAM_TIME = "vendor.sys.tv.stream.realtime";//sync with TvTime.java
    public static final String PVR_RECORD_MODE = "tv.dtv.dvr.mode";//used in dtvkit pvr
    public static final String TIMESHIFT_DISABLE = "tv.dtv.tf.disable";
    public static final String PVR_RECORD_MODE_CHANNEL = "0";
    public static final String PVR_RECORD_MODE_FREQUENCY = "1";

    public static String getProp(String key) {
        String result = null;
        SystemControlManager systemControl = SystemControlManager.getInstance();
        if (systemControl != null && key != null) {
            systemControl.getProperty(key);
        }
        if (DEBUG) {
            Log.d(TAG, "getProp key = " + key + ", result = " + result);
        }
        return result;
    }

    public static boolean setProp(String key, String val) {
        boolean result = false;
        SystemControlManager systemControl = SystemControlManager.getInstance();
        if (systemControl != null && key != null) {
            systemControl.setProperty(key, val);
            result = true;
        }
        if (DEBUG) {
            Log.d(TAG, "setProp key = " + key + ", val = " + val + ", result = " + result);
        }
        return result;
    }

    public static long getLong(String key, long def) {
        long result = def;
        try {
            Class clz = Class.forName("android.os.SystemProperties");
            Method method = clz.getMethod("getLong", String.class, long.class);
            result = (long)method.invoke(clz, key, def);
        } catch (Exception e) {
            e.printStackTrace();
            Log.e(TAG, "getLong Exception = " + e.getMessage());
        }
        if (DEBUG) {
            Log.i(TAG, "getLong key = " + key + ", result = " + result);
        }
        return result;
    }

    public static int getInt(String key, int def) {
        int result = def;
        try {
            Class clz = Class.forName("android.os.SystemProperties");
            Method method = clz.getMethod("getInt", String.class, int.class);
            result = (int)method.invoke(clz, key, def);
        } catch (Exception e) {
            e.printStackTrace();
            Log.d(TAG, "getLInt Exception = " + e.getMessage());
        }
        if (DEBUG) {
            Log.i(TAG, "getInt key = " + key + ", result = " + result);
        }
        return result;
    }

    public static String getString(String key, String def) {
        String result = def;
        try {
            Class clz = Class.forName("android.os.SystemProperties");
            Method method = clz.getMethod("get", String.class, String.class);
            result = (String)method.invoke(clz, key, def);
        } catch (Exception e) {
            e.printStackTrace();
            Log.d(TAG, "getString Exception = " + e.getMessage());
        }
        if (DEBUG) {
            Log.i(TAG, "getString key = " + key + ", result = " + result);
        }
        return result;
    }

    public static boolean getBoolean(String key, boolean def) {
        boolean result = def;
        try {
            Class clz = Class.forName("android.os.SystemProperties");
            Method method = clz.getMethod("getBoolean", String.class, boolean.class);
            result = (boolean)method.invoke(clz, key, def);
        } catch (Exception e) {
            e.printStackTrace();
            Log.d(TAG, "getBoolean Exception = " + e.getMessage());
        }
        if (DEBUG) {
            Log.i(TAG, "getBoolean key = " + key + ", result = " + result);
        }
        return result;
    }

    public static long getCurrentStreamTime(boolean streamtime) {
        long result = System.currentTimeMillis();
        if (streamtime) {
            result = result + getLong(TV_STREAM_TIME, 0);
        }
        return result;
    }

    public static long getStreamTimeDiff() {
        long result = 0;
        result = getLong(TV_STREAM_TIME, 0);
        return result;
    }

    public static boolean resetRecordFrequencyFlag() {
        boolean result = false;
        String pvrRecordMode = getString(PropSettingManager.PVR_RECORD_MODE, PropSettingManager.PVR_RECORD_MODE_CHANNEL);
        if (PropSettingManager.PVR_RECORD_MODE_FREQUENCY.equals(pvrRecordMode)) {
            result = setProp(PropSettingManager.PVR_RECORD_MODE, PropSettingManager.PVR_RECORD_MODE_CHANNEL);
        }
        return result;
    }
}
