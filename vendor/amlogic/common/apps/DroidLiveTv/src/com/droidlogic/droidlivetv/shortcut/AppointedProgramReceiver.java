/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC AppointedProgramReceiver
 */

package com.droidlogic.droidlivetv.shortcut;

import android.app.ActivityManager;
import android.app.AlertDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.ComponentName;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.PowerManager;
import android.os.SystemClock;
import android.provider.Settings;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnFocusChangeListener;
import android.view.LayoutInflater;
import android.view.Window;
import android.view.WindowManager;
import android.widget.TextView;
import android.widget.Button;
import android.text.TextUtils;
import android.util.Log;
import android.media.tv.TvInputManager;
import android.media.tv.TvInputInfo;
import android.app.PendingIntent;
import android.app.AlarmManager;

import android.media.tv.TvContract;
import android.media.tv.TvContract.Channels;
import com.droidlogic.app.tv.DroidLogicTvUtils;
import com.droidlogic.app.tv.ChannelInfo;
import com.droidlogic.app.tv.Program;
import com.droidlogic.app.tv.TvDataBaseManager;

import java.util.ArrayList;
import java.util.Timer;
import java.util.TimerTask;
import java.util.List;

import java.lang.reflect.Method;

import com.droidlogic.droidlivetv.R;

public class AppointedProgramReceiver extends BroadcastReceiver implements OnFocusChangeListener {
    private static final String TAG = "AppointedProgramReceiver";
    private static final String PACKAGE_LAUNCHER = "com.droidlogic.mboxlauncher";
    private static final int MSG_COUNT_DOWN = 0;
    private static final long PROGRAM_INTERVAL = 60000;
    private int countdown = 60;

    private Context mContext;
    private TextView tx_title;
    private Timer timer;
    private long channelid = -1;
    private long programid = -1;
    private int tvtype = 1;//atv 0, dtv 1
    private String inputid = null;

    private boolean isRadio = false;
    private AlertDialog mAlertDialog = null;
    private PowerManager mPowerManager;
    private TvDataBaseManager mTvDataBaseManager;

    private void wakeUp(long time) {
         try {
             Class<?> cls = Class.forName("android.os.PowerManager");
             Method method = cls.getMethod("wakeUp", long.class);
             method.invoke(mPowerManager, time);
         } catch(Exception e) {
             e.printStackTrace();
         }
     }

    @Override
    public void onReceive(Context context, Intent intent) {
        mContext = context;
        mPowerManager = (PowerManager) mContext.getSystemService(Context.POWER_SERVICE);
        boolean isScreenOpen = mPowerManager.isScreenOn();
        Log.d(TAG, "isScreenOpen = " + isScreenOpen);
        //Resume if the system is suspending
        if (!isScreenOpen) {
            Log.d(TAG, "wakeUp the android." );
            long time = SystemClock.uptimeMillis();
            wakeUp(time);
        }
        programid = intent.getLongExtra(DroidLogicTvUtils.EXTRA_PROGRAM_ID, -1L);
        channelid = intent.getLongExtra(DroidLogicTvUtils.EXTRA_CHANNEL_ID, -1L);
        boolean isSetting = intent.getBooleanExtra(DroidLogicTvUtils.EXTRA_APPOINTED_SETTING, false);
        boolean isAdd = intent.getBooleanExtra(DroidLogicTvUtils.EXTRA_APPOINTED_ACTION, false);
        if (isSetting) {
            Log.d(TAG, "onReceive excute appoint setting");
            if (isAdd) {
                intent.putExtra(DroidLogicTvUtils.EXTRA_APPOINTED_SETTING, false);
                setAppointedProgramAlarm(intent);
            } else {
                intent.putExtra(DroidLogicTvUtils.EXTRA_APPOINTED_SETTING, false);
                cancelAppointedProgramAlarm(intent);
            }
            return;
        }
        mTvDataBaseManager = new TvDataBaseManager(mContext);
        List<Program> programList = mTvDataBaseManager.getPrograms(TvContract.buildProgramUri(programid));

        if (programList.size() > 0) {
            cancelAppointedProgramAlarm(programid);
            Program program = mTvDataBaseManager.getPrograms(TvContract.buildProgramUri(programid)).get(0);
            program.setIsAppointed(false);
            mTvDataBaseManager.updateProgram(program);

            ChannelInfo channel = mTvDataBaseManager.getChannelInfo(TvContract.buildChannelUri(program.getChannelId()));
            if (channel == null) {
                Log.d(TAG, "the appointed channel is not exist");
                return;
            }
            if (channel.isAnalogChannel()) {
                tvtype = 0;
            } else if (channel.isDigitalChannel()) {
                 tvtype = 1;
            }
            inputid = channel.getInputId();
            Log.d(TAG, "receive appointed channel:" + channel.getDisplayName() + " program: " + program.getTitle());

            LayoutInflater inflater = (LayoutInflater)mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
            View view = inflater.inflate(R.layout.layout_dialog, null);

            if (mAlertDialog == null) {
                AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
                mAlertDialog = builder.create();
                mAlertDialog.getWindow().setType(WindowManager.LayoutParams.TYPE_SYSTEM_ALERT);
            }
            mAlertDialog.show();
            mAlertDialog.getWindow().setContentView(view);

            tx_title = (TextView)view.findViewById(R.id.dialog_title);
            TextView tx_content = (TextView)view.findViewById(R.id.dialog_content);
            tx_content.setText(mContext.getResources().getString(R.string.watch_program) + " " + program.getTitle());

            TextView button_cancel = (TextView)view.findViewById(R.id.dialog_cancel);
            //button_cancel.setOnClickListener(this);
            button_cancel.setOnFocusChangeListener(this);

            TextView button_ok = (TextView)view.findViewById(R.id.dialog_ok);
            //button_ok.setOnClickListener(this);
            button_ok.setOnFocusChangeListener(this);

            button_cancel.setOnClickListener(new Button.OnClickListener() {
                @Override
                public void onClick(View v) {
                    if (mAlertDialog != null) {
                        mAlertDialog.dismiss();
                    }
                }
            });
            button_ok.setOnClickListener(new Button.OnClickListener() {
                @Override
                public void onClick(View v) {
                    if (mAlertDialog != null) {
                        mAlertDialog.dismiss();
                    }
                    startLiveTv();
                }
            });
            mAlertDialog.setOnDismissListener(new DialogInterface.OnDismissListener() {
                @Override
                public void onDismiss(DialogInterface dialog) {
                    Log.d(TAG, "mAlertDialog onDismiss");
                    timer.cancel();
                }
            });
            timer = new Timer(true);
            timer.schedule(task, 0, 1000);
        } else {
            Log.d(TAG, "the appointed program is not exist" + programid);
        }
    }

    @Override
    public void onFocusChange(View v, boolean hasFocus) {
        if (v instanceof TextView) {
            if (hasFocus) {
                ((TextView) v).setTextColor(mContext.getResources().getColor(R.color.color_text_focused));
            } else {
                ((TextView) v).setTextColor(mContext.getResources().getColor(R.color.color_text_item));
            }
        }
    }

    public void startLiveTv() {
        if (channelid < 0) {
            Log.d(TAG, "startLiveTv channel is invalid");
             return;
        }
        ActivityManager am = (ActivityManager) mContext.getSystemService(Context.ACTIVITY_SERVICE);
        if (am.getRunningTasks(1).get(0).topActivity.getPackageName().equals(PACKAGE_LAUNCHER)) {
            Log.d(TAG, "current screen is launcher, so kill it");
            am.forceStopPackage(PACKAGE_LAUNCHER);
        }
        Intent intent = new Intent(TvInputManager.ACTION_SETUP_INPUTS);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK );
        intent.putExtra("from_tv_source", true);
        intent.putExtra(TvInputInfo.EXTRA_INPUT_ID, inputid);
        intent.putExtra(DroidLogicTvUtils.EXTRA_CHANNEL_ID, channelid);
        intent.putExtra(DroidLogicTvUtils.KEY_LIVETV_PROGRAM_APPOINTED, true);
        //DroidLogicTvUtils.setSearchType(mContext, tvtype);
        cancelAppointedProgramAlarm(programid);
        mContext.startActivity(intent);
    }

    private void cancelAppointedProgramAlarm (long programid) {
        Program logicprogram = mTvDataBaseManager.getProgram(programid);
        if (logicprogram != null) {
            Log.d(TAG, "cancelAppointedProgramAlarm id = " + programid);
            logicprogram.setIsAppointed(false);
            mTvDataBaseManager.updateProgram(logicprogram);
            AlarmManager alarm = (AlarmManager)mContext.getSystemService(Context.ALARM_SERVICE);
            Intent intent = new Intent(DroidLogicTvUtils.ACTION_DROID_PROGRAM_WATCH_APPOINTED);
            intent.addFlags(0x01000000/*Intent.FLAG_RECEIVER_INCLUDE_BACKGROUND*/);
            intent.setData(TvContract.buildProgramUri(logicprogram.getId()));
            intent.setClassName("com.droidlogic.droidlivetv", "com.droidlogic.droidlivetv.shortcut.AppointedProgramReceiver");
            intent.putExtra(DroidLogicTvUtils.EXTRA_PROGRAM_ID, logicprogram.getId());
            intent.putExtra(DroidLogicTvUtils.EXTRA_CHANNEL_ID, logicprogram.getChannelId());
            PendingIntent pendingIntent = PendingIntent.getBroadcast(mContext, 0, intent, PendingIntent.FLAG_NO_CREATE);
            if (pendingIntent != null) {
                alarm.cancel(pendingIntent);
            } else {
                Log.d(TAG, "cancelAppointedProgramAlarm no alarm");
            }
        }
    }

    private TimerTask task = new TimerTask() {
        @Override
        public void run() {
            mHandler.sendEmptyMessage(MSG_COUNT_DOWN);
        }
    };

    private Handler mHandler = new Handler() {
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_COUNT_DOWN:
                    tx_title.setText(Integer.toString(countdown) + " " + mContext.getResources().getString(R.string.switch_program_propmt));
                    if (countdown == 0) {
                        if (mAlertDialog != null) {
                            mAlertDialog.dismiss();
                        }
                        startLiveTv();
                        timer.cancel();
                    }
                    countdown--;
                    break;
                default:
                    break;
            }
        }
    };

    private void setAppointedProgramAlarm(Intent data) {
        if (data == null) {
            Log.d(TAG, "setAppointedProgramAlarm null data");
            return;
        }
        AlarmManager alarm = (AlarmManager)mContext.getSystemService(Context.ALARM_SERVICE);
        long pendingTime = data.getLongExtra(DroidLogicTvUtils.EXTRA_APPOINTED_DELAY, -1l);
        if (pendingTime >= 0) {
            Log.d(TAG, "" + pendingTime / 60000 + " min " + pendingTime % 60000 / 1000 + " sec later show program prompt");
            //check it one minute before appointed time is up
            if (pendingTime > 60000) {
                pendingTime = pendingTime - 60000;
            } else {
                pendingTime = 0;
            }
            PendingIntent pendingIntent = buildPendingIntent(data, true);
            alarm.setExact(AlarmManager.RTC_WAKEUP, System.currentTimeMillis() + pendingTime, pendingIntent);
            Log.d(TAG, "setAppointedProgramAlarm pendingIntent = " + pendingIntent);
        }
    }

    private void cancelAppointedProgramAlarm(Intent data) {
        if (data == null) {
            Log.d(TAG, "cancelAppointedProgramAlarm null data");
            return;
        }
        AlarmManager alarm = (AlarmManager)mContext.getSystemService(Context.ALARM_SERVICE);
        PendingIntent pendingIntent = buildPendingIntent(data, false);
        if (pendingIntent != null) {
            alarm.cancel(pendingIntent);
        }
        Log.d(TAG, "cancelAppointedProgramAlarm pendingIntent = " + pendingIntent);
    }

    private PendingIntent buildPendingIntent (Intent data, boolean creatNew) {
        if (data == null) {
            Log.d(TAG, "buildPendingIntent null data");
            return null;
        }
        Intent intent = new Intent(DroidLogicTvUtils.ACTION_DROID_PROGRAM_WATCH_APPOINTED);
        intent.addFlags(0x01000000/*Intent.FLAG_RECEIVER_INCLUDE_BACKGROUND*/);
        intent.setData(data.getData());
        intent.setComponent(data.getComponent());
        intent.putExtra(DroidLogicTvUtils.EXTRA_PROGRAM_ID, data.getLongExtra(DroidLogicTvUtils.EXTRA_PROGRAM_ID, -1l));
        intent.putExtra(DroidLogicTvUtils.EXTRA_CHANNEL_ID, data.getLongExtra(DroidLogicTvUtils.EXTRA_CHANNEL_ID, -1l));
        //sendBroadcast(intent);
        PendingIntent pendingIntent = null;
        if (creatNew) {
            pendingIntent = PendingIntent.getBroadcast(mContext, 0, intent, PendingIntent.FLAG_CANCEL_CURRENT);
        } else {
            pendingIntent = PendingIntent.getBroadcast(mContext, 0, intent, PendingIntent.FLAG_NO_CREATE);
        }
        Log.d(TAG, "buildPendingIntent creatNew = " + creatNew + ", pendingIntent = " + pendingIntent);
        return pendingIntent;
    }
}
