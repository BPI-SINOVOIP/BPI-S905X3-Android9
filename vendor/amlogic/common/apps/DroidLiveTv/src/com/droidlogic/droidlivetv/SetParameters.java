/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC SetParameters
 */

package com.droidlogic.droidlivetv;

import android.app.AlarmManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.os.Bundle;
import android.os.SystemClock;
import android.util.Log;
import android.media.tv.TvContract;
import android.provider.Settings;

import com.droidlogic.app.tv.AudioEffectManager;
import com.droidlogic.app.tv.TvControlManager;
import com.droidlogic.app.SystemControlManager;
import com.droidlogic.app.tv.TvDataBaseManager;
import com.droidlogic.app.tv.DroidLogicTvUtils;
import com.droidlogic.app.tv.TvInSignalInfo;
import com.droidlogic.app.tv.ChannelInfo;

public class SetParameters {
    private final static String TAG = "SetParameters";
    private TvControlManager mTvControlManager;
    private TvDataBaseManager mTvDataBaseManager;
    private SystemControlManager mSystemControlManager;
    private TvControlManager.SourceInput mTvSourceInput;
    private TvControlManager.SourceInput_Type mVirtualTvSource;
    private TvControlManager.SourceInput_Type mTvSource;
    private Resources mResources;
    private Context mContext;
    private Bundle mBundle;
    private int mDeviceId;

    public static final int PIC_STANDARD = 0;
    public static final int PIC_VIVID    = 1;
    public static final int PIC_SOFT     = 2;
    public static final int PIC_USER     = 3;
    public static final int PIC_MOVIE    = 4;
    public static final int PIC_MONITOR  = 6;
    public static final int PIC_SPORT    = 8;

    public static final int STATUS_STANDARD = 0;
    public static final int STATUS_VIVID    = 1;
    public static final int STATUS_SOFT     = 2;
    public static final int STATUS_SPORT    = 3;
    public static final int STATUS_MOVIE    = 4;
    public static final int STATUS_MONITOR  = 5;
    public static final int STATUS_USER     = 6;

    public static final int MODE_STANDARD = 0;

    private static final String DB_ID_SOUND_EFFECT_SOUND_MODE                   = "db_id_sound_effect_sound_mode";
    private static final String DB_ID_SOUND_EFFECT_SOUND_MODE_TYPE              = "db_id_sound_effect_sound_mode_type";
    private static final String DB_ID_SOUND_EFFECT_SOUND_MODE_DAP_VALUE         = "db_id_sound_effect_sound_mode_dap";
    private static final String DB_ID_SOUND_EFFECT_SOUND_MODE_EQ_VALUE          = "db_id_sound_effect_sound_mode_eq";
    private static final String DB_ID_SOUND_EFFECT_SOUND_MODE_TYPE_DAP          = "db_id_sound_effect_sound_mode_type_dap";
    private static final String DB_ID_SOUND_EFFECT_SOUND_MODE_TYPE_EQ           = "db_id_sound_effect_sound_mode_type_eq";

    public SetParameters(Context context, Bundle bundle) {
        this.mContext = context;
        this.mBundle = bundle;
        this.mDeviceId = bundle.getInt("deviceid", -1);
        mTvDataBaseManager = new TvDataBaseManager(mContext);
        mSystemControlManager = SystemControlManager.getInstance();
        mTvControlManager = TvControlManager.getInstance();
        mResources = mContext.getResources();
        setCurrentChannelData(bundle);
    }

    private void setCurrentChannelData(Bundle bundle) {
        ChannelInfo currentChannel;

        mTvSource = DroidLogicTvUtils.parseTvSourceTypeFromDeviceId(mDeviceId);
        mTvSourceInput = DroidLogicTvUtils.parseTvSourceInputFromDeviceId(mDeviceId);
        mVirtualTvSource = mTvSource;

        if (mTvSource == TvControlManager.SourceInput_Type.SOURCE_TYPE_ADTV) {
            long channelId = bundle.getLong("channelid", -1);
            currentChannel = mTvDataBaseManager.getChannelInfo(TvContract.buildChannelUri(channelId));
            if (currentChannel != null) {
                mTvSource = DroidLogicTvUtils.parseTvSourceTypeFromSigType(DroidLogicTvUtils.getSigType(currentChannel));
                mTvSourceInput = DroidLogicTvUtils.parseTvSourceInputFromSigType(DroidLogicTvUtils.getSigType(currentChannel));
            }
            if (mVirtualTvSource == mTvSource) {//no channels in adtv input, DTV for default.
                mTvSource = TvControlManager.SourceInput_Type.SOURCE_TYPE_DTV;
                mTvSourceInput = TvControlManager.SourceInput.DTV;
            }
        }
    }

    public int getPictureModeStatus () {
        int pictureModeIndex = mSystemControlManager.GetPQMode();
        Log.d(TAG, "getPictureModeStatus:" + pictureModeIndex);
        switch (pictureModeIndex) {
            case PIC_STANDARD:
                return STATUS_STANDARD;
            case PIC_VIVID:
                return STATUS_VIVID;
            case PIC_SOFT:
                return STATUS_SOFT;
            case PIC_SPORT:
                return STATUS_SPORT;
            case PIC_MOVIE:
                return STATUS_MOVIE;
            case PIC_MONITOR:
                return STATUS_MONITOR;
            case PIC_USER:
                return isHdmiSource() ? STATUS_USER : STATUS_MONITOR;
            default:
                return STATUS_STANDARD;
        }
    }

    public void setPictureMode (int mode) {
        Log.d(TAG, "setPictureMode:" + mode);
        if (mode == 0) {
            mSystemControlManager.SetPQMode(SystemControlManager.PQMode.PQ_MODE_STANDARD.toInt(), 1, 0);
        } else if (mode == 1) {
            mSystemControlManager.SetPQMode(SystemControlManager.PQMode.PQ_MODE_BRIGHT.toInt(), 1, 0);
        } else if (mode == 2) {
            mSystemControlManager.SetPQMode(SystemControlManager.PQMode.PQ_MODE_SOFTNESS.toInt(), 1, 0);
        } else if (mode == 3) {
            mSystemControlManager.SetPQMode(SystemControlManager.PQMode.PQ_MODE_SPORTS.toInt(), 1, 0);
        } else if (mode == 4) {
            mSystemControlManager.SetPQMode(SystemControlManager.PQMode.PQ_MODE_MOVIE.toInt(), 1, 0);
        } else if (mode == 5) {
            mSystemControlManager.SetPQMode(isHdmiSource()
                ? SystemControlManager.PQMode.PQ_MODE_MONITOR.toInt() : SystemControlManager.PQMode.PQ_MODE_USER.toInt(), 1, 0);
        } else if (mode == 6) {
            mSystemControlManager.SetPQMode(SystemControlManager.PQMode.PQ_MODE_USER.toInt(), 1, 0);
        }
    }

    public  int getSoundModeStatus () {
        int itemPosition = -1;
        String soundmodetype = Settings.Global.getString(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_SOUND_MODE_TYPE);
        Log.d(TAG, "getSoundModeStatus soundmodetype = " + soundmodetype);
        if (soundmodetype == null || DB_ID_SOUND_EFFECT_SOUND_MODE_TYPE_EQ.equals(soundmodetype)) {
            itemPosition = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_SOUND_MODE_EQ_VALUE, MODE_STANDARD);
        } else if ((DB_ID_SOUND_EFFECT_SOUND_MODE_TYPE_DAP.equals(soundmodetype))) {
            itemPosition = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_SOUND_MODE_DAP_VALUE, MODE_STANDARD);
        } else {
            itemPosition = Settings.Global.getInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_SOUND_MODE, MODE_STANDARD);
        }
        Log.d(TAG, "getSoundModeStatus:" + itemPosition);
        return itemPosition;
    }

    public void setSoundMode (int mode) {
        Log.d(TAG, "setSoundMode:" + mode);
        String soundmodetype = Settings.Global.getString(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_SOUND_MODE_TYPE);
        if (soundmodetype == null || DB_ID_SOUND_EFFECT_SOUND_MODE_TYPE_EQ.equals(soundmodetype)) {
            Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_SOUND_MODE_EQ_VALUE, mode);
        } else if ((DB_ID_SOUND_EFFECT_SOUND_MODE_TYPE_DAP.equals(soundmodetype))) {
            Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_SOUND_MODE_DAP_VALUE, mode);
        } else {
            Settings.Global.putInt(mContext.getContentResolver(), DB_ID_SOUND_EFFECT_SOUND_MODE, mode);
        }
    }

    public int getAspectRatioStatus () {
        int itemPosition = mSystemControlManager.GetDisplayMode(mTvSourceInput != null ? mTvSourceInput.toInt() : 10);//default 10 for mpeg
        Log.d(TAG, "getAspectRatioStatus:" + itemPosition);
        if (itemPosition == SystemControlManager.Display_Mode.DISPLAY_MODE_MODE43.toInt())
            return 1;
        else if (itemPosition == SystemControlManager.Display_Mode.DISPLAY_MODE_FULL.toInt())
            return 2;
        else if (itemPosition == SystemControlManager.Display_Mode.DISPLAY_MODE_169.toInt())
            return 3;
        else
            return 0;
    }

    public void setAspectRatio(int mode) {
        Log.d(TAG, "setAspectRatio:" + mode);
        if (mode == 0) {
            mSystemControlManager.SetDisplayMode(mTvSourceInput != null ? mTvSourceInput.toInt() : 10, SystemControlManager.Display_Mode.DISPLAY_MODE_NORMAL, 1);
        } else if (mode == 1) {
            mSystemControlManager.SetDisplayMode(mTvSourceInput != null ? mTvSourceInput.toInt() : 10, SystemControlManager.Display_Mode.DISPLAY_MODE_MODE43, 1);
        } else if (mode == 2) {
            mSystemControlManager.SetDisplayMode(mTvSourceInput != null ? mTvSourceInput.toInt() : 10, SystemControlManager.Display_Mode.DISPLAY_MODE_FULL, 1);
        } else if (mode == 3) {
            mSystemControlManager.SetDisplayMode(mTvSourceInput != null ? mTvSourceInput.toInt() : 10, SystemControlManager.Display_Mode.DISPLAY_MODE_169,  1);
        }
    }

    public int getSleepTimerStatus () {
        String ret = "";
        int time = mSystemControlManager.getPropertyInt("persist.tv.sleep_timer", 0);
        Log.d(TAG, "getSleepTimerStatus:" + time);
        return time;
    }

    public void setSleepTimer (int mode) {
        AlarmManager alarm = (AlarmManager)mContext.getSystemService(Context.ALARM_SERVICE);
        Intent intent = new Intent("droidlogic.intent.action.TIMER_SUSPEND");
        intent.addFlags(0x01000000/*Intent.FLAG_RECEIVER_INCLUDE_BACKGROUND*/);
        intent.putExtra(DroidLogicTvUtils.KEY_ENABLE_SUSPEND_TIMEOUT, true);
        PendingIntent pendingIntent = PendingIntent.getBroadcast(mContext, 0, intent, 0);
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
        Log.d(TAG, "start time count down after " + timeout + " ms");
    }

    public boolean isHdmiSource() {
        if (mTvSourceInput != null) {
            return mTvSource == TvControlManager.SourceInput_Type.SOURCE_TYPE_HDMI;
        }
        return false;
    }
}
