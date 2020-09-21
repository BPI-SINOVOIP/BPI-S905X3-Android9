/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC TimerSuspendService
 */

package com.droidlogic.droidlivetv;

import android.app.AlertDialog;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.input.InputManager;
import android.os.Handler;
import android.os.IBinder;
import android.os.SystemClock;
import android.os.SystemProperties;
import android.view.KeyEvent;
import android.view.InputEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.TextView;
import android.util.Log;
import java.lang.reflect.Method;

import com.droidlogic.droidlivetv.R;
import com.droidlogic.app.tv.DroidLogicTvUtils;
import com.droidlogic.app.DroidLogicKeyEvent;
import com.droidlogic.app.SystemControlManager;

public class TimerSuspendService extends Service {
    private final String TAG = "TimerSuspendService";

    /* time suspend dialog */
    private AlertDialog mDialog;//use to dismiss
    private TextView mCountDownText;
    private int mSuspendCount = 0;
    private boolean mEnableNoSignalTimeout = false;
    private boolean mEnableSuspendTimeout = false;

    private SystemControlManager mSystemControlManager;
    private Context mContext = null;
    public static final int INJECT_INPUT_EVENT_MODE_WAIT_FOR_FINISH = 2;
    private static final int TIMEOUT_10MIN = 10 * 60;//10min
    private static final int TIMEOUT_1MIN = 1 * 60;//1min

    @Override
    public void onCreate() {
        super.onCreate();
        mSystemControlManager = SystemControlManager.getInstance();
        this.mContext = this;
        initTimeSuspend();
        Log.d(TAG, "onCreate");
    }

    @Override
    public IBinder onBind ( Intent intent ) {
        return null;
    }

    @Override
    public int onStartCommand ( Intent intent, int flags, int startId ) {
        Log.d ( TAG, "onStartCommand");
        if (intent != null)
            Log.d(TAG, "intent=" + intent);
        mEnableSuspendTimeout = intent.getBooleanExtra(DroidLogicTvUtils.KEY_ENABLE_SUSPEND_TIMEOUT, false);
        mEnableNoSignalTimeout = intent.getBooleanExtra(DroidLogicTvUtils.KEY_ENABLE_NOSIGNAL_TIMEOUT, false);
        //stop it if need cancel
        if (!mEnableSuspendTimeout) {
            if (!mEnableNoSignalTimeout) {
                mSystemControlManager.setProperty("tv.sleep_timer", 0 + "");
                stopSelf();
            } else {
                reset_shutdown_time(TIMEOUT_10MIN);//10min
            }
        } else {
            reset_shutdown_time(TIMEOUT_1MIN);//one min
        }

        return super.onStartCommand ( intent, flags, startId );
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        Log.d ( TAG, "onDestroy");
        remove_shutdown_time();
    }

    private void initTimeSuspend() {
        AlertDialog.Builder suspendDialog = new AlertDialog.Builder(this);
        View suspendDialogView = View.inflate(this, R.layout.timesuspend_dialog, null);
        Button mBn = (Button) suspendDialogView.findViewById(R.id.btn_cancel_suspend);
        mBn.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View view) {
                //checkTimeoutStatus();
                hideDialog();
                Log.d(TAG, "onClick");
            }
        });
        mCountDownText = (TextView) suspendDialogView.findViewById(R.id.tv_dialog);
        suspendDialog.setView(suspendDialogView);
        mDialog = suspendDialog.create();
        mDialog.getWindow().setType(WindowManager.LayoutParams.TYPE_SYSTEM_ALERT);
        mDialog.setOnDismissListener(new DialogInterface.OnDismissListener() {
            @Override
            public void onDismiss(DialogInterface dialog) {
                Log.d(TAG, "onDismiss mSuspendCount = " + mSuspendCount);
                if (mSuspendCount > 0) {
                    checkTimeoutStatus();
                }
            }
        });
    }

    private void checkTimeoutStatus() {
        if (!mEnableSuspendTimeout) {
            if (mEnableNoSignalTimeout) {
                reset_shutdown_time(TIMEOUT_10MIN);//10min
            }
        } else {
            mSystemControlManager.setProperty("tv.sleep_timer", 0 + "");
            stopSelf();
        }
    }

    private void GetinjectInputEvent(InputEvent keyevent, int mode) {
        try {
            Class<?> cls = Class.forName("android.hardware.input.InputManager");
            Method constructor = cls.getMethod("getInstance");
            Method method = cls.getMethod("injectInputEvent", InputEvent.class, int.class);
            method.invoke(constructor.invoke(null), keyevent, mode);
        } catch(Exception e) {
            e.printStackTrace();
        }
    }

    private Handler timeSuspend_handler = new Handler();
    private Runnable timeSuspend_runnable = new Runnable() {
        @Override
        public void run() {
            try {
                if (mSuspendCount == 0) {
                    long now = SystemClock.uptimeMillis();
                    KeyEvent down = new KeyEvent(now, now, KeyEvent.ACTION_DOWN, DroidLogicKeyEvent.KEYCODE_POWER, 0);
                    KeyEvent up = new KeyEvent(now, now, KeyEvent.ACTION_UP, DroidLogicKeyEvent.KEYCODE_POWER, 0);
                    GetinjectInputEvent(down, INJECT_INPUT_EVENT_MODE_WAIT_FOR_FINISH);
                    GetinjectInputEvent(up, INJECT_INPUT_EVENT_MODE_WAIT_FOR_FINISH);
                    hideDialog();
                    stopSelf();
                } else {
                    if (mSuspendCount == 60) {
                        String str = mSuspendCount + " " + getResources().getString(R.string.countdown_tips);
                        //initTimeSuspend();
                        mCountDownText.setText(str);
                        mDialog.show();
                    } else if (mSuspendCount < 60) {
                        String str = mSuspendCount + " " + getResources().getString(R.string.countdown_tips);
                        mCountDownText.setText(str);
                    }
                    Log.d(TAG, "mSuspendCount=" + mSuspendCount);
                    timeSuspend_handler.postDelayed(timeSuspend_runnable, 1000);
                }
                mSuspendCount--;
            } catch (RuntimeException e) {
                e.printStackTrace();
            }
        }
    };

    private void reset_shutdown_time(int time) {
        Log.d(TAG, "reset_shutdown_time = " + time);
        mSuspendCount =  time;
        remove_shutdown_time();
        timeSuspend_handler.post(timeSuspend_runnable);
        hideDialog();
    }

    private void remove_shutdown_time() {
        Log.d ( TAG, "remove_shutdown_time");
        timeSuspend_handler.removeCallbacksAndMessages(null);
    }

    private void hideDialog() {
        Log.d ( TAG, "hideDialog");
        if (mDialog != null && mDialog.isShowing()) {
            mDialog.dismiss();
        }
    }
}
