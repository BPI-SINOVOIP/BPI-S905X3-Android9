/*
 * Copyright (C) 2015 The Android Open Source Project
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
 * limitations under the License
 */

package com.droidlogic.tv.settings.tvoption;

import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.text.TextUtils;
import android.util.Log;
import android.os.SystemProperties;
import android.os.SystemClock;
import android.media.AudioManager;
import android.app.ActivityManager;
import android.content.pm.PackageManager;
//import android.content.pm.IPackageDataObserver;
import android.provider.Settings;
import android.widget.Toast;
import android.app.AlarmManager;
import android.app.PendingIntent;

import com.droidlogic.tv.settings.R;
import android.media.tv.TvContract;
import android.media.tv.TvContract.Channels;
import com.droidlogic.app.tv.ChannelInfo;
import com.droidlogic.app.tv.TvDataBaseManager;
import com.droidlogic.app.tv.TvInSignalInfo;
import com.droidlogic.app.tv.TvControlManager;
import com.droidlogic.app.tv.DroidLogicTvUtils;
import com.droidlogic.app.SystemControlManager;
import com.droidlogic.app.tv.TvControlManager.FreqList;
import com.droidlogic.app.tv.TvChannelParams;
import com.droidlogic.app.DaylightSavingTime;
import com.droidlogic.app.HdmiCecManager;
import com.droidlogic.app.OutputModeManager;

import com.droidlogic.tv.settings.TvSettingsActivity;

import java.util.ArrayList;
import java.util.Calendar;
import java.util.HashMap;
import java.util.TimeZone;
import java.util.SimpleTimeZone;
import android.media.tv.TvInputManager;
import android.media.tv.TvInputInfo;
import java.util.List;
import java.lang.reflect.Method;
public class TvOptionSettingManager {
    public static final int SET_DTMB = 0;
    public static final int SET_DVB_C = 1;
    public static final int SET_DVB_T = 2;
    public static final int SET_DVB_T2 = 3;
    public static final int SET_ATSC_T = 4;
    public static final int SET_ATSC_C= 5;
    public static final int SET_ISDB_T = 6;

    public static final String KEY_MENU_TIME = DroidLogicTvUtils.KEY_MENU_TIME;
    public static final int DEFUALT_MENU_TIME = DroidLogicTvUtils.DEFUALT_MENU_TIME;
    public static final String AUDIO_LATENCY = "media.dtv.passthrough.latencyms";

    public static final String STRING_NAME = "name";
    public static final String STRING_STATUS = "status";
    public static final String DTV_AUTOSYNC_TVTIME = "autosync_tvtime";

    public static final String TAG = "TvOptionSettingManager";

    private Resources mResources;
    private Context mContext;
    private ChannelInfo mCurrentChannel;
    private int mDeviceId;
    private String mInputInfoId = null;
    private TvDataBaseManager mTvDataBaseManager;
    private TvControlManager mTvControlManager;
    private TvControlManager.SourceInput mTvSourceInput;
    private SystemControlManager mSystemControlManager;
    private DaylightSavingTime mDaylightSavingTime = null;
    private AudioManager mAudioManager;

    public TvOptionSettingManager (Context context, boolean isothercontext) {
        mContext = context;
        mResources = mContext.getResources();
        mSystemControlManager = SystemControlManager.getInstance();
        mAudioManager = (AudioManager) context.getSystemService(context.AUDIO_SERVICE);

        if (SystemProperties.getBoolean("persist.sys.daylight.control", false)) {
            mDaylightSavingTime = DaylightSavingTime.getInstance();
        }
        if (isothercontext) {
            return;
        }
        mTvDataBaseManager = new TvDataBaseManager(mContext);
        mTvControlManager = TvControlManager.getInstance();
        mDeviceId = ((TvSettingsActivity)context).getIntent().getIntExtra("tv_current_device_id", -1);
        mTvSourceInput = DroidLogicTvUtils.parseTvSourceInputFromDeviceId(mDeviceId);
        mInputInfoId = ((TvSettingsActivity)context).getIntent().getStringExtra("current_tvinputinfo_id");
        mCurrentChannel = mTvDataBaseManager.getChannelInfo(TvContract.buildChannelUri(((TvSettingsActivity)context).getIntent().getLongExtra("current_channel_id", -1)));
    }

    private boolean CanDebug() {
        return TvOptionFragment.CanDebug();
    }

    public int getDtvTypeStatus () {
        String type = getDtvType();
        int ret = SET_ATSC_T;
        if (type != null) {
            if (CanDebug()) Log.d(TAG, "getDtvTypeStatus = " + type);
            if (TextUtils.equals(type, TvContract.Channels.TYPE_DTMB)) {
                    ret = SET_DTMB;
            } else if (TextUtils.equals(type, TvContract.Channels.TYPE_DVB_C)) {
                    ret = SET_DVB_C;
            } else if (TextUtils.equals(type, TvContract.Channels.TYPE_DVB_T)) {
                    ret = SET_DVB_T;
            } else if (TextUtils.equals(type, TvContract.Channels.TYPE_DVB_T2)) {
                    ret = SET_DVB_T2;
            } else if (TextUtils.equals(type, TvContract.Channels.TYPE_ATSC_T)) {
                    ret = SET_ATSC_T;
            } else if (TextUtils.equals(type, TvContract.Channels.TYPE_ATSC_C)) {
                    ret = SET_ATSC_C;
            } else if (TextUtils.equals(type, TvContract.Channels.TYPE_ISDB_T)) {
                    ret = SET_ISDB_T;
            }
            return ret;
        } else {
            ret = -1;
            return ret;
        }
    }

    public String getDtvType() {
        String type = Settings.System.getString(mContext.getContentResolver(),
            DroidLogicTvUtils.TV_KEY_DTV_TYPE);
        return type;
    }

    public int getSoundChannelStatus () {
        int type = 0;
        if (mCurrentChannel != null) {
            type = mCurrentChannel.getAudioChannel();
        }
        if (type < 0 || type > 2) {
            type = 0;
        }
        if (CanDebug()) Log.d(TAG, "getSoundChannelStatus = " + type);
        return type;
    }

    public ArrayList<HashMap<String, String>> getChannelInfoStatus() {
        ArrayList<HashMap<String, String>> list =  new ArrayList<HashMap<String, String>>();
        TvControlManager.SourceInput_Type tvSource = DroidLogicTvUtils.parseTvSourceTypeFromDeviceId(mDeviceId);
        TvControlManager.SourceInput_Type virtualTvSource = tvSource;
        if (tvSource == TvControlManager.SourceInput_Type.SOURCE_TYPE_ADTV) {
            if (mCurrentChannel != null) {
                tvSource = DroidLogicTvUtils.parseTvSourceTypeFromSigType(DroidLogicTvUtils.getSigType(mCurrentChannel));
            }
            if (virtualTvSource == tvSource) {//no channels in adtv input, DTV for default.
                tvSource = TvControlManager.SourceInput_Type.SOURCE_TYPE_DTV;
            }
        }
        if (mCurrentChannel != null) {
            HashMap<String, String> item = new HashMap<String, String>();
            item.put(STRING_NAME, mResources.getString(R.string.channel_info_channel));
            item.put(STRING_STATUS, mCurrentChannel.getDisplayNameLocal());
            list.add(item);

            item = new HashMap<String, String>();
            item.put(STRING_NAME, mResources.getString(R.string.channel_info_frequency));
            item.put(STRING_STATUS, Integer.toString(mCurrentChannel.getFrequency() + mCurrentChannel.getFineTune()));
            list.add(item);

            if (tvSource == TvControlManager.SourceInput_Type.SOURCE_TYPE_DTV) {
                item = new HashMap<String, String>();
                item.put(STRING_NAME, mResources.getString(R.string.channel_info_type));
                item.put(STRING_STATUS, mCurrentChannel.getType());
                list.add(item);

                item = new HashMap<String, String>();
                item.put(STRING_NAME, mResources.getString(R.string.channel_info_service_id));
                item.put(STRING_STATUS, Integer.toString(mCurrentChannel.getServiceId()));
                list.add(item);

                item = new HashMap<String, String>();
                item.put(STRING_NAME, mResources.getString(R.string.channel_info_pcr_id));
                item.put(STRING_STATUS, Integer.toString(mCurrentChannel.getPcrPid()));
                list.add(item);
            }
        }
        return list;
    }

    public int getMenuTimeStatus () {
        int type = Settings.System.getInt(mContext.getContentResolver(), KEY_MENU_TIME, DEFUALT_MENU_TIME);
        if (CanDebug()) Log.d(TAG, "getMenuTimeStatus = " + type);
        return type;
    }

    public int getSleepTimerStatus () {
        String ret = "";
        int time = mSystemControlManager.getPropertyInt("persist.tv.sleep_timer", 0);
        Log.d(TAG, "getSleepTimerStatus:" + time);
        return time;
    }

    //0 1 ~ luncher livetv
    public int getStartupSettingStatus () {
        int type = Settings.System.getInt(mContext.getContentResolver(), DroidLogicTvUtils.TV_START_UP_ENTER_APP, 0);
        if (CanDebug()) Log.d(TAG, "getStartupSettingStatus = " + type);
        if (type != 0) {
            type = 1;
        }
        return type;
    }

    // 0 1 ~ off on
    public int getDynamicBacklightStatus () {
        int switchVal = mSystemControlManager.GetDynamicBacklight();
        if (CanDebug()) Log.d(TAG, "getDynamicBacklightStatus = " + switchVal);

        if (switchVal != 0) {
            switchVal = 1;
        }

        return switchVal;
    }

    // 0 1 ~ off on others on
    public int getADSwitchStatus () {
        int switchVal = Settings.System.getInt(mContext.getContentResolver(), DroidLogicTvUtils.TV_KEY_AD_SWITCH, 0);
        if (CanDebug()) Log.d(TAG, "getADSwitchStatus = " + switchVal);
        if (switchVal != 0) {
            switchVal = 1;
        }
        return switchVal;
    }

    public int getVolumeCompensateStatus () {
        int value = 0;
        if (mCurrentChannel != null)
            value = mCurrentChannel.getAudioCompensation();
        else
            value = 0;
        return value;
    }

    public int getSwitchChannelStatus () {
        if (mTvControlManager.getBlackoutEnable() == 0)
            return 0;
        else
            return 1;
    }

    public int getADMixStatus () {
        int val = Settings.System.getInt(mContext.getContentResolver(), DroidLogicTvUtils.TV_KEY_AD_MIX, 50);
        if (CanDebug()) Log.d(TAG, "getADMixStatus = " + val);
        return val;
    }

    public int[] getFourHdmi20Status() {
        int[] fourhdmistatus = new int[4];
        TvControlManager.HdmiPortID[] allport = {TvControlManager.HdmiPortID.HDMI_PORT_1, TvControlManager.HdmiPortID.HDMI_PORT_2,
            TvControlManager.HdmiPortID.HDMI_PORT_3, TvControlManager.HdmiPortID.HDMI_PORT_4};
        for (int i = 0; i < allport.length; i++) {
            if (mTvControlManager.GetHdmiEdidVersion(allport[i]) == TvControlManager.HdmiEdidVer.HDMI_EDID_VER_20.toInt()) {
                fourhdmistatus[i] = 1;
            } else {
                fourhdmistatus[i] = 0;
            }
        }
        if (CanDebug()) Log.d(TAG, "getFourHdmi20Status 1 to 4 " + fourhdmistatus[0] + ", " + fourhdmistatus[1] + ", " + fourhdmistatus[2] + ", " + fourhdmistatus[3]);
        return fourhdmistatus;
    }

    public int getNumOfHdmi() {
        TvInputManager inputManager = (TvInputManager) mContext.getSystemService(Context.TV_INPUT_SERVICE);
        List<TvInputInfo> inputs = inputManager.getTvInputList();
        int num_hdmi = 0;
        for (TvInputInfo input : inputs) {
            Log.d(TAG,"input:"+ input.toString());
            if (input.getId().contains("Hdmi") && input.getParentId() == null)
                num_hdmi++;
        }
        Log.d(TAG,"num_hdmi:"+num_hdmi);
        return num_hdmi;
    }

    public int GetRelativeSourceInput() {
        int result = -1;
        //hdmi1~hdmi4 5~8 TvControlManager.SourceInput.HDMI1~TvControlManager.SourceInput.HDMI4
        result = mTvControlManager.GetCurrentSourceInput() - TvControlManager.SourceInput.HDMI1.toInt();
        if (CanDebug()) Log.d(TAG, "GetRelativeSourceInput = " + result);
        return result;
    }

    public void setDtvType (int value) {
        if (CanDebug()) Log.d(TAG, "setDtvType = " + value);
        String type = null;
        switch (value) {
            case SET_DTMB:
                type = TvContract.Channels.TYPE_DTMB;
                break;
            case SET_DVB_C:
                type = TvContract.Channels.TYPE_DVB_C;
                break;
            case SET_DVB_T:
                type = TvContract.Channels.TYPE_DVB_T;
                break;
            case SET_DVB_T2:
                type = TvContract.Channels.TYPE_DVB_T2;
                break;
            case SET_ATSC_T:
                type = TvContract.Channels.TYPE_ATSC_T;
                break;
            case SET_ATSC_C:
                type = TvContract.Channels.TYPE_ATSC_C;
                break;
            case SET_ISDB_T:
                type = TvContract.Channels.TYPE_ISDB_T;
                break;
        }
        if (type != null) {
            Settings.System.putString(mContext.getContentResolver(), DroidLogicTvUtils.TV_KEY_DTV_TYPE, type);
        }
    }

    public void setSoundChannel (int type) {
        if (CanDebug()) Log.d(TAG, "setSoundChannel = " + type);
        if (mCurrentChannel != null) {
            mCurrentChannel.setAudioChannel(type);
            mTvDataBaseManager.updateChannelInfo(mCurrentChannel);
            mTvControlManager.DtvSetAudioChannleMod(mCurrentChannel.getAudioChannel());
        }
    }

    public void setStartupSetting (int type) {
        if (CanDebug()) Log.d(TAG, "setStartupSetting = " + type);
        Settings.System.putInt(mContext.getContentResolver(), DroidLogicTvUtils.TV_START_UP_ENTER_APP, type);
    }

    public void setMenuTime (int type) {
        if (CanDebug()) Log.d(TAG, "setMenuTime = " + type);
        Settings.System.putInt(mContext.getContentResolver(), KEY_MENU_TIME, type);
    }

    public void setSleepTimer (int mode) {
        AlarmManager alarm = (AlarmManager)mContext.getSystemService(Context.ALARM_SERVICE);
        Intent intent = new Intent("droidlogic.intent.action.TIMER_SUSPEND");
        intent.addFlags(0x01000000/*Intent.FLAG_RECEIVER_INCLUDE_BACKGROUND*/);
        intent.putExtra(DroidLogicTvUtils.KEY_ENABLE_SUSPEND_TIMEOUT, true);
        PendingIntent pendingIntent = PendingIntent.getBroadcast(mContext, 0,
                intent, 0);
        alarm.cancel(pendingIntent);

        mSystemControlManager.setProperty("persist.tv.sleep_timer", mode+"");

        long timeout = 0;
        if (mode == 0) {
            return;
        } else if (mode < 5) {
            timeout = (mode * 15  - 1) * 60 * 1000;
        } else {
            timeout = ((mode - 4) * 30 + 4 * 15  - 1) * 60 * 1000;
        }

        alarm.setExact(AlarmManager.ELAPSED_REALTIME, SystemClock.elapsedRealtime() + timeout, pendingIntent);
        Log.d(TAG, "start time count down elapsedRealtime " + SystemClock.elapsedRealtime() + "  timeout after " + timeout + " ms");
    }


    public void setAutoBacklightStatus(int value) {
        if (CanDebug()) Log.d(TAG, "setAutoBacklightStatus = " + value);
        SystemControlManager.Dynamic_Backlight_Mode mode;
        if (value != 0) {
            mode = SystemControlManager.Dynamic_Backlight_Mode.DYNAMIC_BACKLIGHT_HIGH;
        } else {
            mode = SystemControlManager.Dynamic_Backlight_Mode.DYNAMIC_BACKLIGHT_OFF;
        }
        mSystemControlManager.SetDynamicBacklight(mode, 1);
    }

    public void setAudioADSwitch (int switchVal) {
        if (CanDebug()) Log.d(TAG, "setAudioADSwitch = " + switchVal);
        Settings.System.putInt(mContext.getContentResolver(), DroidLogicTvUtils.TV_KEY_AD_SWITCH, switchVal);
        Intent intent = new Intent(DroidLogicTvUtils.ACTION_AD_SWITCH);
        intent.putExtra(DroidLogicTvUtils.EXTRA_SWITCH_VALUE, switchVal);
        mContext.sendBroadcast(intent);
    }

    public void setVolumeCompensate (int value) {
        if (mCurrentChannel != null) {
            int current = mCurrentChannel.getAudioCompensation();
            int offset = 0;
            if (value > current) {
                offset = 1;
            } else if (value < current) {
                offset = -1;
            }
            if ((current < 20 && offset > 0)
                || (current > -20 && offset < 0)) {
                mCurrentChannel.setAudioCompensation(current + offset);
                mTvDataBaseManager.updateChannelInfo(mCurrentChannel);
                mTvControlManager.SetCurProgVolumeCompesition(mCurrentChannel.getAudioCompensation());
            }
        }
    }

    public void setBlackoutEnable(int status, int isSave) {
        mTvControlManager.setBlackoutEnable(status, isSave);
    }

    public void setADMix (int step) {
        if (CanDebug()) Log.d(TAG, "setADMix = " + step);
        int level = getADMixStatus() + step;
        if (level <= 100 && level >= 0) {
            Settings.System.putInt(mContext.getContentResolver(), DroidLogicTvUtils.TV_KEY_AD_MIX, level);
            Intent intent = new Intent(DroidLogicTvUtils.ACTION_AD_MIXING_LEVEL);
            intent.putExtra(DroidLogicTvUtils.PARA_VALUE1, level);
            mContext.sendBroadcast(intent);
        }
    }

    public boolean isAnalogChannel() {
        if (mCurrentChannel != null) {
            return mCurrentChannel.isAnalogChannel();
        } else {
            return false;
        }
    }

    public void doFactoryReset() {
        if (CanDebug()) Log.d(TAG, "doFactoryReset");
        mTvControlManager.StopTv();
        resetAudioSettings();
        setStartupSetting(0);
        setAudioADSwitch(0);
        setAutoBacklightStatus(0);
        setMenuTime(0);
        setSleepTimer(0);
        setDefAudioStreamVolume();
        clearHdmi20Mode();
        HdmiCecManager.reset(mContext.getContentResolver(), mSystemControlManager);
        // SystemControlManager mSystemControlManager = SystemControlManager.getInstance();
        // mSystemControlManager.setBootenv("ubootenv.var.upgrade_step", "1");
        /*final String[] tvPackages = {"com.android.providers.tv"};
        for (int i = 0; i < tvPackages.length; i++) {
            ClearPackageData(tvPackages[i]);
        }*/
        mTvDataBaseManager.deleteChannels("com.droidlogic.tvinput/.services.ADTVInputService/HW16",  null);
        mTvControlManager.SSMInitDevice();
        mTvControlManager.FactoryCleanAllTableForProgram();
    }

    private void setDefAudioStreamVolume() {
        int maxVolume = SystemProperties.getInt("ro.config.media_vol_steps", 100);
        int streamMaxVolume = mAudioManager.getStreamMaxVolume(AudioManager.STREAM_MUSIC);
        int defaultVolume = maxVolume == streamMaxVolume ? (maxVolume * 3) / 10 : (streamMaxVolume * 3) / 4;
        mAudioManager.setStreamVolume(AudioManager.STREAM_MUSIC, defaultVolume, 0);
    }

    private void resetAudioSettings() {
        SoundParameterSettingManager soundparameter = new SoundParameterSettingManager(mContext);
        sendResetSoundEffectBroadcast();
        if (soundparameter != null) {
            soundparameter.resetParameter();
        }
    }

    private void sendResetSoundEffectBroadcast() {
        Intent intent = new Intent();
        intent.setAction("droid.action.resetsoundeffect");
        mContext.sendBroadcast(intent);
    }

    private  void ClearPackageData(String packageName) {
        Log.d(TAG, "ClearPackageData:" + packageName);
        //clear data
        ActivityManager am = (ActivityManager)mContext.getSystemService(Context.ACTIVITY_SERVICE);
        //ClearUserDataObserver mClearDataObserver = new ClearUserDataObserver();
        boolean res = false;
        try {
            Class IPackageManagerclz = Class.forName("android.content.pm.IPackageDataObserver");
            Method clearApplicationUserDataMethod = ActivityManager.class.getMethod("clearApplicationUserData", String.class, IPackageManagerclz);
            res = (boolean)clearApplicationUserDataMethod.invoke(am, packageName, null);
                    //boolean res = am.clearApplicationUserData(packageName, mClearDataObserver);
        if (!res) {
            Log.i(TAG, " clear " + packageName + " data failed");
        } else {
            Log.i(TAG, " clear " + packageName + " data succeed");
        }

        //clear cache
        PackageManager packageManager = mContext.getPackageManager();
            //ClearUserDataObserver mClearCacheObserver = new ClearUserDataObserver();
            //packageManager.deleteApplicationCacheFiles(packageName, mClearCacheObserver);
            Method delApplicationCacheFilesMethod = ActivityManager.class.getMethod("deleteApplicationCacheFiles", String.class, IPackageManagerclz);
            delApplicationCacheFilesMethod.invoke(packageManager, packageName, null);
        //clear default
        packageManager.clearPackagePreferredActivities(packageName);
        }catch(Exception ex){
            ex.printStackTrace();
    }

        }


    public void doFbcUpgrade() {
        Log.d(TAG, "doFactoryReset need add");
    }

    public void setHdmi20Mode(int order, int mode) {
        if (CanDebug()) Log.d(TAG, "setHdmi20Mode order = " + order + ", mode = " + mode);
        TvControlManager.HdmiPortID[] allport = {TvControlManager.HdmiPortID.HDMI_PORT_1, TvControlManager.HdmiPortID.HDMI_PORT_2,
            TvControlManager.HdmiPortID.HDMI_PORT_3, TvControlManager.HdmiPortID.HDMI_PORT_4};
        if (order < 0 || order > 3) {
            Log.d(TAG, "setHdmi20Mode device id erro");
            return;
        }
        if (mode == 1) {
            // set HDMI mode sequence: save than set
            mTvControlManager.SaveHdmiEdidVersion(allport[order],
                TvControlManager.HdmiEdidVer.HDMI_EDID_VER_20);
            mTvControlManager.SetHdmiEdidVersion(allport[order],
                TvControlManager.HdmiEdidVer.HDMI_EDID_VER_20);
        } else {
            mTvControlManager.SaveHdmiEdidVersion(allport[order],
                TvControlManager.HdmiEdidVer.HDMI_EDID_VER_14);
            mTvControlManager.SetHdmiEdidVersion(allport[order],
                TvControlManager.HdmiEdidVer.HDMI_EDID_VER_14);
        }
    }

    public void clearHdmi20Mode() {
        if (CanDebug()) Log.d(TAG, "reset Hdmi20Mode status");
        TvControlManager.HdmiPortID[] allport = {TvControlManager.HdmiPortID.HDMI_PORT_1, TvControlManager.HdmiPortID.HDMI_PORT_2,
                TvControlManager.HdmiPortID.HDMI_PORT_3, TvControlManager.HdmiPortID.HDMI_PORT_4};

        for (int i = 0; i  < allport.length; i++) {
            mTvControlManager.SaveHdmiEdidVersion(allport[i],
                    TvControlManager.HdmiEdidVer.HDMI_EDID_VER_14);
        }
    }

    public void setDaylightSavingTime(int value) {
        mDaylightSavingTime.setDaylightSavingTime(value);
    }

    public int getDaylightSavingTime() {
        return mDaylightSavingTime.getDaylightSavingTime();
    }

    public void setHdmiAudioLatency(String value) {
        if (CanDebug()) Log.d(TAG, "setHdmiAudioLatency = " + value);
        mSystemControlManager.setProperty(AUDIO_LATENCY, value);
    }

    public int getHdmiAudioLatency() {
        int result = mSystemControlManager.getPropertyInt(AUDIO_LATENCY, 0);
        if (CanDebug()) Log.d(TAG, "getHdmiAudioLatency = " + result);
        return result;
    }
}
