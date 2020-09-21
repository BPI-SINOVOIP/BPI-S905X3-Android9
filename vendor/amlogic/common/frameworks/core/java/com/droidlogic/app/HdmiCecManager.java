/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC HdmiCecManager
 */

package com.droidlogic.app;

import android.content.Context;
import android.content.ContentResolver;
import android.content.res.Configuration;
import android.provider.Settings;
import android.util.Log;

import java.io.File;
import java.util.Locale;
import java.lang.reflect.Method;


public class HdmiCecManager {
    private static final boolean DEBUG = true;
    private static final String TAG = "HdmiCecManager";
    //switch box cec control
    public static final String HDMI_CONTROL_ONE_TOUCH_PLAY_ENABLED          = "hdmi_control_one_touch_play_enabled";
    public static final String HDMI_CONTROL_AUTO_CHANGE_LANGUAGE_ENABLED    = "hdmi_control_auto_change_language_enabled";

    // same with frameworks/base/core/java/android/provider/Settings.java file
    public static final String HDMI_SYSTEM_AUDIO_CONTROL_ENABLED            = "hdmi_system_audio_control_enabled";
    // same with frameworks/base/core/java/android/content/pm/PackageManager.java file
    public static final String FEATURE_HDMI_CEC                             = "android.hardware.hdmi.cec";
    //CEC device node
    public static final String CEC_DEVICE_FILE = "/sys/devices/virtual/switch/lang_config/state";
    public static final String CEC_SYS = "/sys/class/amhdmitx/amhdmitx0/cec_config";
    public static final String CEC_ENABLE = "/sys/class/hdmirx/hdmirx0/cec";
    public static final String CEC_SUPPORT = "/sys/class/amhdmitx/amhdmitx0/tv_support_cec";
    public static final String CEC_PROP = "ubootenv.var.cecconfig";
    public static final String CEC_TAG = "cec0x";
    public static final String CEC_0   = "cec0x0";

    public static final int FUN_CEC = 0x00;
    public static final int FUN_ONE_KEY_POWER_OFF = 0x01;
    public static final int FUN_ONE_KEY_PLAY = 0x02;
    public static final int FUN_AUTO_CHANGE_LANGUAGE = 0x03;

    public static final int MASK_FUN_CEC = 0x01;                   // bit 0
    public static final int MASK_ONE_KEY_PLAY = 0x02;              // bit 1
    public static final int MASK_ONE_KEY_STANDBY = 0x04;           // bit 2
    public static final int MASK_AUTO_CHANGE_LANGUAGE = 0x20;      // bit 5
    public static final int MASK_ALL = 0x2f;                       // all mask

    // Local device type
    public static final int CEC_LOCAL_DEVICE_TYPE_TV    = 0;
    public static final int CEC_LOCAL_DEVICE_TYPE_OTT   = 4;
    public static final int CEC_LOCAL_DEVICE_TYPE_AUDIO = 5;

    public static final boolean FUN_OPEN = true;
    public static final boolean FUN_CLOSE = false;

    private static String[] mCecLanguageList;
    private static String[] mLanguageList;
    private static String[] mCountryList;

    private Context mContext;
    private SystemControlManager mSystemControlManager;


    public HdmiCecManager(Context context) {
        mContext = context;
        mSystemControlManager = SystemControlManager.getInstance();
    }

    public void initCec() {
        String fun = mSystemControlManager.readSysFs(CEC_SYS);
        if (mSystemControlManager != null) {
            Log.d(TAG, "set cec fun = " + fun);
            mSystemControlManager.writeSysFs(CEC_SYS, fun);
        }
    }

    public boolean remoteSupportCec() {
        if ((new File(CEC_SUPPORT).exists())) {
            return !mSystemControlManager.readSysFs(CEC_SUPPORT).equals("0");
        }
        return true;
    }

    public int[] getBinaryArray(String binaryString) {
        int[] tmp = new int[4];
        for (int i = 0; i < binaryString.length(); i++) {
            String tmpString = String.valueOf(binaryString.charAt(i));
            tmp[i] = Integer.parseInt(tmpString);
        }
        return tmp;
    }

    public String getCurConfig() {
        return mSystemControlManager.readSysFs(CEC_SYS);
    }

    public void setCecEnv(int cfg) {
        int cec_config = cfg & MASK_ALL;
        String str = CEC_TAG + Integer.toHexString(cec_config);
        Log.d(TAG, "save env:" + str);
        mSystemControlManager.setBootenv(CEC_PROP, str);
    }

    public String getCurLanguage() {
        return mSystemControlManager.readSysFs(CEC_DEVICE_FILE);
    }

    public void setLanguageList(String[] cecLanguageList, String[] languageList, String[] countryList) {
        mCecLanguageList = cecLanguageList;
        mLanguageList = languageList;
        mCountryList = countryList;
    }

    public boolean isChangeLanguageOpen() {
        boolean exist = new File(CEC_SYS).exists();
        if (exist) {
            String cec_cfg = mSystemControlManager.readSysFs(CEC_SYS);
            int value = Integer.valueOf(cec_cfg.substring(2, cec_cfg.length()), 16);
            Log.d(TAG, "cec_cfg:" + cec_cfg + ", value:" + value);
            return (value & MASK_AUTO_CHANGE_LANGUAGE) != 0;
        } else {
            return false;
        }
    }

    public void doUpdateCECLanguage(String curLanguage) {
        int i = -1;
        for (int j = 0; j < mCecLanguageList.length; j++) {
            if (curLanguage != null && curLanguage.trim().equals(mCecLanguageList[j])) {
                i = j;
                break;
            }
        }
        if (i >= 0) {
            String able = mContext.getResources().getConfiguration().locale.getCountry();
            if (able.equals(mCountryList[i])) {
                if (DEBUG) Log.d(TAG, "no need to change language");
                return;
            } else {
                Locale l = new Locale(mLanguageList[i], mCountryList[i]);
                if (DEBUG) Log.d(TAG, "change the language right now !!!");
                updateLanguage(l);
            }
        } else {
            Log.d(TAG, "the language code is not support right now !!!");
        }
    }

    public void updateLanguage(Locale locale) {
        try {
            Object objIActMag;
            Class clzIActMag = Class.forName("android.app.IActivityManager");
            Class clzActMagNative = Class.forName("android.app.ActivityManagerNative");
            Method mtdActMagNative$getDefault = clzActMagNative.getDeclaredMethod("getDefault");

            objIActMag = mtdActMagNative$getDefault.invoke(clzActMagNative);
            Method mtdIActMag$getConfiguration = clzIActMag.getDeclaredMethod("getConfiguration");
            Configuration config = (Configuration) mtdIActMag$getConfiguration.invoke(objIActMag);
            config.locale = locale;

            Class[] clzParams = { Configuration.class };
            Method mtdIActMag$updateConfiguration = clzIActMag.getDeclaredMethod("updateConfiguration", clzParams);
            mtdIActMag$updateConfiguration.invoke(objIActMag, config);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
    public void setCecEnable(String value) {
        mSystemControlManager.writeSysFs(CEC_ENABLE,value);
        Log.d(TAG,"setCecEnable CEC_SYS:"+value);
    }
    public void setCecSysfsValue(int fun, boolean isOn) {
        String cec_config = mSystemControlManager.getBootenv(CEC_PROP, CEC_0);
        String writeConfig, s;
        // get rid of '0x' prefix
        int cec_cfg_value = Integer.valueOf(cec_config.substring(5, cec_config.length()), 16);
        Log.d(TAG, "cec config str:" + cec_config + ", value:" + cec_cfg_value);
        if (fun != FUN_CEC) {
            if (CEC_0.equals(cec_config)) {
                return;
            }
        }

        if (fun == FUN_CEC) {
            if (isOn) {
                mSystemControlManager.setBootenv(CEC_PROP, CEC_TAG + "2f");
                mSystemControlManager.writeSysFs(CEC_SYS, "2f");
            } else {
                mSystemControlManager.setBootenv(CEC_PROP, CEC_0);
                mSystemControlManager.writeSysFs(CEC_SYS, "0");
            }
            return ;
        } else if (fun == FUN_ONE_KEY_PLAY) {
            if (isOn) {
                cec_cfg_value |= MASK_ONE_KEY_PLAY;
            } else {
                cec_cfg_value &= ~MASK_ONE_KEY_PLAY;
            }
        } else if (fun == FUN_ONE_KEY_POWER_OFF) {
            if (isOn) {
                cec_cfg_value |= MASK_ONE_KEY_STANDBY;
            } else {
                cec_cfg_value &= ~MASK_ONE_KEY_STANDBY;
            }
        }else if(fun == FUN_AUTO_CHANGE_LANGUAGE){
            if (isOn) {
                cec_cfg_value |= MASK_AUTO_CHANGE_LANGUAGE;
            } else {
                cec_cfg_value &= ~MASK_AUTO_CHANGE_LANGUAGE;
            }
        }
        writeConfig = CEC_TAG + Integer.toHexString(cec_cfg_value);
        mSystemControlManager.setBootenv(CEC_PROP, writeConfig);
        s = writeConfig.substring(3, writeConfig.length());
        mSystemControlManager.writeSysFs(CEC_SYS, s);
        Log.d(TAG, "==== cec set config : " + writeConfig);
    }


    private static final String SETTINGS_HDMI_CONTROL_ENABLED = "hdmi_control_enabled";
    private static final String SETTINGS_ONE_TOUCH_PLAY = HdmiCecManager.HDMI_CONTROL_ONE_TOUCH_PLAY_ENABLED;
    private static final String SETTINGS_AUTO_POWER_OFF = "hdmi_control_auto_device_off_enabled";
    private static final String SETTINGS_AUTO_WAKE_UP = "hdmi_control_auto_wakeup_enabled";
    private static final String SETTINGS_ARC_ENABLED = HdmiCecManager.HDMI_SYSTEM_AUDIO_CONTROL_ENABLED;
    private static final String PERSIST_HDMI_CEC_SET_MENU_LANGUAGE= "persist.vendor.sys.cec.set_menu_language";
    private static final String PERSIST_HDMI_CEC_DEVICE_AUTO_POWEROFF = "persist.vendor.sys.cec.deviceautopoweroff";
    private static final int ON = 1;
    private static final int OFF = 0;

    public boolean isHdmiControlEnabled () {
        return readValue(SETTINGS_HDMI_CONTROL_ENABLED);
    }

    public boolean isOneTouchPlayEnabled () {
        return readValue(SETTINGS_ONE_TOUCH_PLAY);
    }

    public boolean isAutoPowerOffEnabled () {
        return readValue(SETTINGS_AUTO_POWER_OFF);
    }

    public boolean isAutoWakeUpEnabled () {
        return readValue(SETTINGS_AUTO_WAKE_UP);
    }

    public boolean isAutoChangeLanguageEnabled () {
        return mSystemControlManager.getPropertyBoolean(PERSIST_HDMI_CEC_SET_MENU_LANGUAGE, true);
    }

    public boolean isArcEnabled () {
       return readValue(SETTINGS_ARC_ENABLED);
    }

    public void enableHdmiControl (boolean value) {
        writeValue(SETTINGS_HDMI_CONTROL_ENABLED, value);
    }

    public void enableOneTouchPlay (boolean value) {
        writeValue(SETTINGS_ONE_TOUCH_PLAY, value);
    }

    public void enableAutoPowerOff (boolean value) {
        writeValue(SETTINGS_AUTO_POWER_OFF, value);
        mSystemControlManager.setProperty(PERSIST_HDMI_CEC_DEVICE_AUTO_POWEROFF, value ? "true" : "false");
    }

    public void enableAutoWakeUp (boolean value) {
        writeValue(SETTINGS_AUTO_WAKE_UP, value);

    }

    public void enableAutoChangeLanguage (boolean value) {
        mSystemControlManager.setProperty(PERSIST_HDMI_CEC_SET_MENU_LANGUAGE, value ? "true" : "false");

    }

    public void enableArc (boolean value) {
        writeValue(SETTINGS_ARC_ENABLED, value);
    }

    private boolean readValue(String key) {
        return Settings.Global.getInt(mContext.getContentResolver(), key, ON) == ON;
    }

    private void writeValue(String key, boolean value) {
        Settings.Global.putInt(mContext.getContentResolver(), key, value ? ON : OFF);
    }

    public static void reset(ContentResolver contentResolver, SystemControlManager sytemControlManager) {
        Settings.Global.putInt(contentResolver, SETTINGS_HDMI_CONTROL_ENABLED,  ON);
        Settings.Global.putInt(contentResolver, SETTINGS_ONE_TOUCH_PLAY,  ON);
        Settings.Global.putInt(contentResolver, SETTINGS_AUTO_POWER_OFF,  ON);
        Settings.Global.putInt(contentResolver, SETTINGS_AUTO_WAKE_UP,  ON);
        Settings.Global.putInt(contentResolver, SETTINGS_ARC_ENABLED,  ON);
        sytemControlManager.setProperty(PERSIST_HDMI_CEC_SET_MENU_LANGUAGE, "true");
        sytemControlManager.setProperty(PERSIST_HDMI_CEC_DEVICE_AUTO_POWEROFF, "true");
    }
}

