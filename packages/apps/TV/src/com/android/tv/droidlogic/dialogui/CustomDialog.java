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
 * limitations under the License.
 */

package com.android.tv.droidlogic;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.View;
import android.view.WindowManager;
import android.util.TypedValue;
import android.content.Context;
import android.widget.Button;
import android.widget.TextView;
import android.media.tv.TvInputManager;
import android.content.Intent;
import android.media.tv.TvInputInfo;
import android.media.tv.TvContract;
import android.widget.Toast;
import android.widget.LinearLayout;
import android.provider.Settings;
import android.os.PowerManager;
import android.os.SystemClock;

import org.json.JSONException;
import org.json.JSONObject;

import java.util.Timer;
import java.util.TimerTask;
import java.util.List;
import java.util.ArrayList;
import java.lang.reflect.Method;

import com.android.tv.dvr.data.ScheduledRecording;
import com.android.tv.R;
import com.android.tv.MainActivity;
import com.android.tv.MainActivityWrapper;
import com.android.tv.dvr.WritableDvrDataManager;
import com.android.tv.data.api.Channel;
import com.android.tv.TvSingletons;
import com.android.tv.ui.TunableTvView;
import com.android.tv.util.TvInputManagerHelper;
import com.android.tv.InputSessionManager;
import com.android.tv.data.ChannelDataManager;
import com.android.tv.dvr.DvrDataManager;
import com.android.tv.dvr.DvrManager;
import com.android.tv.common.util.SystemProperties;
import com.android.tv.ui.TvOverlayManager;
import com.android.tv.util.Utils;

import com.droidlogic.app.tv.DroidLogicTvUtils;

public class CustomDialog {
    private final static String TAG = "CustomDialog";
    private final boolean DEBUG = false;
    private List<ScheduledRecording> mScheduledRecordingList = new ArrayList<ScheduledRecording>();
    private Object mScheduledRecordingListLock = new Object();
    private Context mContext;
    private DvrManager mDvrManager;
    private TvSingletons mTvSingletons = null;
    private MainActivityWrapper mMainActivityWrapper = null;
    private DvrDataManager mDataManager = null;
    private TvInputManagerHelper mTvInputManagerHelper = null;
    private InputSessionManager mInputSessionManager = null;
    private ChannelDataManager mChannelDataManager = null;

    public static final String TYPE_ACTION_KEY = "action_type";
    public static final String TYPE_ACTION_KEY_PVR_CONFIRM = "pvr_confirm";
    public static final String ACTION_KEY_PVR_CONFIRM_ACTION = "action";
    public static final String ACTION_KEY_PVR_CONFIRM_FUNCTION = "function";
    public static final String ACTION_PVR_CONFIRM_TIMEOUT = "timeout";
    public static final String ACTION_PVR_CONFIRM_RECORD = "record";
    public static final String ACTION_PVR_CONFIRM_CANCEL = "cancel";
    public static final String PROP_TV_PREVIEW_STATUS = "tv.is.preview.window.playing";

    private static final int MSG_UPDATE_CONFIRM_DIALOG = 1;
    private static final int MSG_UPDATE_TITLE = 2;
    private static final int MSG_UPDATE_CLOSE_DIALOG = 3;

    private final int DIALOG_DISPLAY_TIMEOUT = 60;
    private final int DIALOG_WAIT_PLAY_TIMEOUT = 10;

    private Handler handler = new Handler() {
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            switch (msg.what) {
                case MSG_UPDATE_CONFIRM_DIALOG:
                    ;
                    break;
                case MSG_UPDATE_TITLE:
                    ;
                    break;
                case MSG_UPDATE_CLOSE_DIALOG:
                    ;
                    break;
                default:
                    break;
            }
        }
    };

    public CustomDialog(Context context) {
        this.mContext = context;
        this.mTvSingletons = TvSingletons.getSingletons(context);
        this.mDvrManager = mTvSingletons.getDvrManager();
        this.mMainActivityWrapper = mTvSingletons.getMainActivityWrapper();
        this.mDataManager = mTvSingletons.getDvrDataManager();
        this.mTvInputManagerHelper = mTvSingletons.getTvInputManagerHelper();
        this.mInputSessionManager = mTvSingletons.getInputSessionManager();
        this.mChannelDataManager = mTvSingletons.getChannelDataManager();
    }

    public boolean hasRelatedDialog(final ScheduledRecording scheduler) {
        boolean result = false;
        synchronized (mScheduledRecordingListLock) {
            result = mScheduledRecordingList.contains(scheduler);
        }
        return result;
    }

    private void addScheduledRecording(final ScheduledRecording scheduler) {
        synchronized (mScheduledRecordingListLock) {
            mScheduledRecordingList.add(scheduler);
        }
    }

    private void removeScheduledRecording(final ScheduledRecording scheduler) {
        synchronized (mScheduledRecordingListLock) {
            mScheduledRecordingList.remove(scheduler);
        }
    }

    private boolean isMainActivtyRunning() {
        boolean result = false;
        if (mMainActivityWrapper != null) {
            result = mMainActivityWrapper.isResumed();
        }
        Log.d(TAG, "isMainActivtyRunning " + result);
        return result;
    }

    private boolean isPlayStarted() {
        boolean result = false;
        if (isMainActivtyRunning()) {
            MainActivity mainActivity = mMainActivityWrapper.getMainActivity();
            if (mainActivity != null) {
                TunableTvView tvview = mainActivity.getTvView();
                if (tvview != null && tvview.getCurrentChannel() != null) {
                    result = true;
                }
            }
        }
        Log.d(TAG, "isPlayStarted " + result);
        return result;
    }

    private boolean isVideoAvailable() {
        boolean result = false;
        if (isMainActivtyRunning()) {
            MainActivity mainActivity = mMainActivityWrapper.getMainActivity();
            if (mainActivity != null) {
                TunableTvView tvview = mainActivity.getTvView();
                if (tvview != null && tvview.isVideoOrAudioAvailable()) {
                    result = true;
                }
            }
        }
        Log.d(TAG, "isVideoAvailable " + result);
        return result;
    }

    private long getCurrentPlayingChannelId() {
        long result = -1;
        if (isMainActivtyRunning()) {
            MainActivity mainActivity = mMainActivityWrapper.getMainActivity();
            if (mainActivity != null) {
                TunableTvView tvview = mainActivity.getTvView();
                if (tvview != null) {
                    Channel channel = tvview.getCurrentChannel();
                    if (channel != null) {
                        result = channel.getId();
                    }
                }
            }
        }
        Log.d(TAG, "getCurrentPlayingChannelId " + result);
        return result;
    }

    private void updateChannelBanner(ScheduledRecording scheduler) {
        if (scheduler != null && isMainActivtyRunning()) {
            MainActivity mainActivity = mMainActivityWrapper.getMainActivity();
            if (mainActivity != null) {
                TvOverlayManager overlay = mainActivity.getOverlayManager();
                if (overlay != null && getCurrentPlayingChannelId() == scheduler.getChannelId() && overlay.isChannelBannerViewActive()) {
                    Log.d(TAG, "updateChannelBanner start");
                    overlay.updateChannelBannerView();
                }
            }
        }
        Log.d(TAG, "updateChannelBanner over");
    }

    private void switchToChannel(Channel channel) {
        Log.d(TAG, "switchToChannel " + channel);
        if (isMainActivtyRunning() && channel != null) {
            MainActivity mainActivity = mMainActivityWrapper.getMainActivity();
            if (mainActivity != null) {
                mainActivity.tuneToChannel(channel);
                return;
            }
        }
    }

    private int getScheduleRecordingNumber() {
        int count = 0;
        if (mDvrManager != null && mDataManager != null) {
            List<ScheduledRecording> startedScheduled = mDataManager.getStartedRecordings();
            if (startedScheduled != null && startedScheduled.size() > 0) {
                for (ScheduledRecording one : startedScheduled) {
                    count++;
                }
            }
        }
        Log.d(TAG, "getScheduleRecordingNumber " + count);
        return count;
    }

    private boolean isRecordTunerAvailableForSchedule(ScheduledRecording scheduler) {
        boolean result = false;
        TvInputInfo schedulerInputInfo = null;
        if (scheduler != null && mTvInputManagerHelper != null && mInputSessionManager != null) {
            schedulerInputInfo = mTvInputManagerHelper.getTvInputInfo(scheduler.getInputId());
            if (schedulerInputInfo != null && schedulerInputInfo.canRecord() &&
                    !mInputSessionManager.isTunedForRecording(TvContract.buildChannelUri(scheduler.getChannelId())) &&
                    mInputSessionManager.getAllTunedSessionCount(scheduler.getInputId()) < schedulerInputInfo.getTunerCount()) {
                result = true;
            }
        }
        Log.d(TAG, "isRecordTunerAvailableForSchedule result = " + result);
        return result;
    }

    private boolean isSchedulChannelRecording(ScheduledRecording scheduler) {
        boolean result = false;
        TvInputInfo schedulerInputInfo = null;
        if (scheduler != null && mInputSessionManager != null) {
            result = mInputSessionManager.isTunedForRecording(TvContract.buildChannelUri(scheduler.getChannelId()));
        }
        Log.d(TAG, "isSchedulChannelRecording result = " + result);
        return result;
    }

    private boolean isScheduleDiffrentFrequency(ScheduledRecording scheduler) {
        boolean result = false;
        if (mDataManager != null && mChannelDataManager != null) {
            List<ScheduledRecording> startedScheduled = mDataManager.getStartedRecordings();
            if (startedScheduled != null && startedScheduled.size() > 0) {
                Channel scheduleChannel = mChannelDataManager.getChannel(scheduler.getChannelId());
                long channelId = -1;
                Channel channel = null;
                for (ScheduledRecording one : startedScheduled) {
                    channelId = one.getChannelId();
                    channel = mChannelDataManager.getChannel(channelId);
                    if (channel != null && scheduleChannel != null) {
                        result = (channel.getFrequency() != scheduleChannel.getFrequency());
                        if (result) {
                            break;
                        }
                    }
                }
            }
        }
        Log.d(TAG, "isScheduleDiffrentFrequency result = " + result);
        return result;
    }

    private ScheduledRecording getFirstScheduledRecording() {
        ScheduledRecording result = null;
        if (mDataManager != null && mChannelDataManager != null) {
            List<ScheduledRecording> startedScheduled = mDataManager.getStartedRecordings();
            if (startedScheduled != null && startedScheduled.size() > 0) {
                result = startedScheduled.get(0);
            }
        }
        Log.d(TAG, "getFirstScheduledRecording result = " + result);
        return result;
    }

    private boolean isLauncherPlaying() {
        boolean result = false;
        result = getPropertyBoolean(PROP_TV_PREVIEW_STATUS, false);
        Log.d(TAG, "isLauncherPlaying result = " + result);
        return result;
    }

    private boolean isLauncherPlayingDiffrentFrequency(ScheduledRecording scheduler) {
        boolean result = false;
        boolean launchPlaying = isLauncherPlaying();
        if (launchPlaying && scheduler != null) {
            long playingChannelId = Settings.System.getLong(mContext.getContentResolver(), DroidLogicTvUtils.TV_DTV_CHANNEL_INDEX, -1);
            int deviceId = Settings.System.getInt(mContext.getContentResolver(), DroidLogicTvUtils.TV_CURRENT_DEVICE_ID, 0);
            long scheduleChannelId = scheduler.getChannelId();
            Channel playingChannel = null;
            Channel scheduleChannel = null;
            if (deviceId == DroidLogicTvUtils.DEVICE_ID_ADTV) {
                if (playingChannelId != -1) {
                    playingChannel = mChannelDataManager.getChannel(playingChannelId);
                }
                if (scheduleChannelId != -1) {
                    scheduleChannel = mChannelDataManager.getChannel(scheduleChannelId);
                }
                if (playingChannel != null && scheduleChannel != null) {
                    result = (playingChannel.getFrequency() != scheduleChannel.getFrequency());
                }
            }
        }
        Log.d(TAG, "isLauncherPlayingDiffrentFrequency result = " + result);
        return result;
    }

    public void showConfirmRecord(final Context context,final ScheduledRecording scheduler, final ActionCallback callback) {
        final Context applicationContext = context != null ? context : mContext.getApplicationContext();
        final AlertDialog.Builder builder = new AlertDialog.Builder(applicationContext);
        final AlertDialog alert = builder.create();
        final View dialogView = View.inflate(applicationContext, R.layout.appoint_record_confirm_dialog, null);
        final TextView title = (TextView) dialogView.findViewById(R.id.dialog_title);
        final Button record = (Button) dialogView.findViewById(R.id.dialog_record);
        final Button play = (Button) dialogView.findViewById(R.id.dialog_switch);
        final Button cancel = (Button) dialogView.findViewById(R.id.dialog_cancel);
        final LinearLayout containner = (LinearLayout) dialogView.findViewById(R.id.dialog_button_container);
        final int[] count = {DIALOG_DISPLAY_TIMEOUT};
        final int[] needRemove = {0};
        addScheduledRecording(scheduler);
        final boolean isDiffrentDrequency = isScheduleDiffrentFrequency(scheduler);
        final boolean isChannelRecording = isSchedulChannelRecording(scheduler);
        final boolean isMainActivityRunning = isMainActivtyRunning();
        final boolean isRecordTunerAvailable = isRecordTunerAvailableForSchedule(scheduler);
        //if need to light the screen please run the interface below
        if (!SystemProperties.USE_DEBUG_ENABLE_SUSPEND_RECORD.getValue()) {
            checkSystemWakeUp();
        }
        if (SystemProperties.USE_DEBUG_ENABLE_SUSPEND_RECORD.getValue()) {
            if (!isScreenOn()) {
                //deal directly if screen is off
                Log.d(TAG, "showConfirmRecord deal directly if screen is off");
                if (callback != null) {
                    JSONObject obj = new JSONObject();
                    try {
                        obj.put(TYPE_ACTION_KEY, TYPE_ACTION_KEY_PVR_CONFIRM);
                        obj.put(ACTION_KEY_PVR_CONFIRM_ACTION, ACTION_PVR_CONFIRM_TIMEOUT);
                        obj.put(ACTION_KEY_PVR_CONFIRM_FUNCTION, "showConfirmRecord");
                    } catch (JSONException e) {
                        e.printStackTrace();
                    }
                    containner.setVisibility(View.GONE);
                    title.setText("");
                    long channelId = scheduler.getChannelId();
                    Utils.setLastWatchedChannelUri(applicationContext, TvContract.buildChannelUri(channelId).toString());
                    Utils.saveChannelIdForAtvDtvMode(applicationContext, channelId);
                    waitPlayAndRecord(scheduler, obj, alert, title, callback);
                }
                return;
            }
        }
        updateReminderTips(scheduler, title, count[0]);
        //enable background record at any case
        if (!isMainActivityRunning) {
            if (isLauncherPlayingDiffrentFrequency(scheduler)) {
                record.setEnabled(false);
                showToastInHandle(mContext.getString(R.string.pvr_confirm_tuning_diffrent_frequency));
            }
        }
        final Timer timer = new Timer();
        final TimerTask task = new TimerTask() {
            @Override
            public void run() {
                dialogView.post(new Runnable() {
                    @Override
                    public void run() {
                        (count[0])--;
                        if (count[0] >= 0) {
                            updateReminderTips(scheduler, title, count[0]);
                        }
                        if (count[0] == 0) {
                            if (callback != null) {
                                JSONObject obj = new JSONObject();
                                try {
                                    obj.put(TYPE_ACTION_KEY, TYPE_ACTION_KEY_PVR_CONFIRM);
                                    obj.put(ACTION_KEY_PVR_CONFIRM_ACTION, ACTION_PVR_CONFIRM_TIMEOUT);
                                    obj.put(ACTION_KEY_PVR_CONFIRM_FUNCTION, "showConfirmRecord");
                                } catch (JSONException e) {
                                    e.printStackTrace();
                                }
                                containner.setVisibility(View.GONE);
                                title.setText(applicationContext.getString(R.string.pvr_appoint_confirm_wait_play, DIALOG_WAIT_PLAY_TIMEOUT));
                                waitPlayAndRecord(scheduler, obj, alert, title, callback);
                            }
                            //alert.dismiss();
                        }
                    }
                });
            }
        };
        timer.schedule(task, 1000, 1000);
        record.requestFocus();

        record.setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View v) {
                //appoint to record
                Log.d(TAG, "record onClick " + callback);
                timer.cancel();
                //clear conflict schedule
                if (isScheduleDiffrentFrequency(scheduler)) {
                    mDvrManager.stopInprogressRecording();
                } else if (!isRecordTunerAvailableForSchedule(scheduler)) {
                    ScheduledRecording result = getFirstScheduledRecording();
                    if (result != null) {
                        Log.d(TAG, "record onClick stop firstly found schedule");
                        mDvrManager.stopRecording(result);
                    } else {
                        Log.d(TAG, "record onClick can't find first schedule");
                    }
                }
                if (callback != null) {
                    JSONObject obj = new JSONObject();
                    try {
                        obj.put(TYPE_ACTION_KEY, TYPE_ACTION_KEY_PVR_CONFIRM);
                        obj.put(ACTION_KEY_PVR_CONFIRM_ACTION, ACTION_PVR_CONFIRM_RECORD);
                        obj.put(ACTION_KEY_PVR_CONFIRM_FUNCTION, "showConfirmRecord");
                    } catch (JSONException e) {
                        e.printStackTrace();
                    }
                    //delay for remove or stop schedules
                    sendCallBackDelayedByHandle(callback, obj, scheduler);
                } else {
                    needRemove[0] = 1;
                }
                alert.dismiss();
            }
        });
        play.setOnClickListener(new Button.OnClickListener() {
            //appoint to switch and record
            @Override
            public void onClick(View v) {
                Log.d(TAG, "play onClick " + callback);
                timer.cancel();
                if (callback != null) {
                    JSONObject obj = new JSONObject();
                    try {
                        obj.put(TYPE_ACTION_KEY, TYPE_ACTION_KEY_PVR_CONFIRM);
                        obj.put(ACTION_KEY_PVR_CONFIRM_ACTION, ACTION_PVR_CONFIRM_RECORD);
                        obj.put(ACTION_KEY_PVR_CONFIRM_FUNCTION, "showConfirmRecord");
                    } catch (JSONException e) {
                        e.printStackTrace();
                    }
                    containner.setVisibility(View.GONE);
                    title.setText(applicationContext.getString(R.string.pvr_appoint_confirm_wait_play, DIALOG_WAIT_PLAY_TIMEOUT));
                    waitPlayAndRecord(scheduler, obj, alert, title, callback);
                }
                needRemove[0] = 0;
                //wait enter livetv and play
                //alert.dismiss();
            }
        });
        cancel.setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View v) {
                //appoint to record
                Log.d(TAG, "cancel onClick " + callback);
                timer.cancel();
                if (callback != null) {
                    JSONObject obj = new JSONObject();
                    try {
                        obj.put(TYPE_ACTION_KEY, TYPE_ACTION_KEY_PVR_CONFIRM);
                        obj.put(ACTION_KEY_PVR_CONFIRM_ACTION, ACTION_PVR_CONFIRM_CANCEL);
                        obj.put(ACTION_KEY_PVR_CONFIRM_FUNCTION, "showConfirmRecord");
                    } catch (JSONException e) {
                        e.printStackTrace();
                    }
                    callback.onActionBack(obj.toString(), scheduler);
                }
                needRemove[0] = 1;
                alert.dismiss();
            }
        });
        alert.setOnDismissListener(new DialogInterface.OnDismissListener() {
            @Override
            public void onDismiss(DialogInterface dialog) {
                Log.d(TAG, "onDismiss " + callback);
                timer.cancel();
                if (needRemove[0] == 1) {
                    removeScheduledRecording(scheduler);
                }
            }
        });
        alert.getWindow().setType(WindowManager.LayoutParams.TYPE_SYSTEM_ALERT/*TYPE_APPLICATION_OVERLAY*/);
        alert.setView(dialogView);
        alert.show();
        //alert.getWindow().setContentView(dialogView);
        //set size and background
        /*WindowManager.LayoutParams params = alert.getWindow().getAttributes();
        params.width = (int)TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 400, applicationContext.getResources().getDisplayMetrics());
        params.height = WindowManager.LayoutParams.WRAP_CONTENT ;
        alert.getWindow().setAttributes(params);
        alert.getWindow().setBackgroundDrawableResource(R.drawable.appoint_confirm_dialog_background);*/
    }

    private void updateReminderTips(final ScheduledRecording scheduler, final TextView title, final int timeout) {
        if (scheduler != null && title != null) {
            final Context applicationContext = mContext.getApplicationContext();
            final boolean isDiffrentDrequency = isScheduleDiffrentFrequency(scheduler);
            final boolean isChannelRecording = isSchedulChannelRecording(scheduler);
            final boolean isMainActivityRunning = isMainActivtyRunning();
            final boolean isRecordTunerAvailable = isRecordTunerAvailableForSchedule(scheduler);
            if (isDiffrentDrequency) {
                //stop started recordings
                Log.d(TAG, "updateReminderTips new frequency schedule");
                title.setText(applicationContext.getString(R.string.pvr_appoint_confirm_title, scheduler.getProgramDisplayTitle(applicationContext), timeout) +
                    applicationContext.getString(R.string.pvr_appoint_confirm_title_stop_other_frequency));
            } else if (!isRecordTunerAvailable) {
                //stop first found started recording to release resource
                ScheduledRecording result = getFirstScheduledRecording();
                if (result != null) {
                    Log.d(TAG, "updateReminderTips need to find started schedules and stop the first one");
                    title.setText(applicationContext.getString(R.string.pvr_appoint_confirm_title, scheduler.getProgramDisplayTitle(applicationContext), timeout) +
                        applicationContext.getString(R.string.pvr_appoint_confirm_title_stop_first_started, result.getProgramDisplayTitle(applicationContext)));
                } else {
                    Log.d(TAG, "updateReminderTips can't find first schedule");
                    title.setText(applicationContext.getString(R.string.pvr_appoint_confirm_title, scheduler.getProgramDisplayTitle(applicationContext), timeout));
                }
            } else {
                Log.d(TAG, "updateReminderTips normal schedule");
                title.setText(applicationContext.getString(R.string.pvr_appoint_confirm_title, scheduler.getProgramDisplayTitle(applicationContext), timeout));
            }
        }
    }

    private void waitPlayAndRecord(final ScheduledRecording scheduler, final JSONObject obj,
            final AlertDialog alert, final TextView title, final ActionCallback callback) {
        Log.d(TAG, "waitPlayAndRecord");
        new Thread(new Runnable() {
            @Override
            public void run() {
                //clear conflict schedule
                if (isScheduleDiffrentFrequency(scheduler)) {
                    mDvrManager.stopInprogressRecording();
                } else if (!isRecordTunerAvailableForSchedule(scheduler)) {
                    ScheduledRecording result = getFirstScheduledRecording();
                    if (result != null) {
                        Log.d(TAG, "waitPlayAndRecord stop firstly found schedule");
                        mDvrManager.stopRecording(result);
                    } else {
                        Log.d(TAG, "waitPlayAndRecord can't find first schedule");
                    }
                }
                String action = ACTION_PVR_CONFIRM_RECORD;
                if (obj != null) {
                    try {
                        action = obj.getString(ACTION_KEY_PVR_CONFIRM_ACTION);
                    } catch (JSONException e) {
                        e.printStackTrace();
                        Log.d(TAG, "waitPlayAndRecord get action JSONException = " + e.getMessage());
                    }
                }
                if (ACTION_PVR_CONFIRM_RECORD.equals(action)) {
                    //record in livetv
                    Log.d(TAG, "waitPlayAndRecord wait play in livetv");
                } else {
                    //background record
                    //start background record if user hasn't select a certain action
                    Log.d(TAG, "waitPlayAndRecord record in background as user hasn't select a certain action");
                    sendCallBackByHandle(callback , obj, scheduler);
                    hideDialogByHandle(alert);
                    removeScheduledRecording(scheduler);
                    delay(1000);
                    if (isPlayStarted() && isVideoAvailable() && getCurrentPlayingChannelId() == scheduler.getChannelId()) {
                        updateChannelBannerByHandle(scheduler);
                    }
                    return;
                }
                //start livetv firstly
                hideDialogByHandle(alert);
                startLiveTvByHandle(scheduler);
                int timeout = DIALOG_WAIT_PLAY_TIMEOUT;
                while (true) {
                    Log.d(TAG, "waitPlayAndRecord timeout = " + timeout);
                    if (isPlayStarted() && isVideoAvailable() && getCurrentPlayingChannelId() == scheduler.getChannelId()) {
                        Log.d(TAG, "waitPlayAndRecord play ok");
                        delay(2000);
                        sendCallBackByHandle(callback , obj, scheduler);
                        //hideDialogByHandle(alert);
                        removeScheduledRecording(scheduler);
                        delay(1000);
                        updateChannelBannerByHandle(scheduler);
                        return;
                    } else {
                        Log.d(TAG, "waitPlayAndRecord update timeout = " + timeout);
                        updateTitleByHandle(title, mContext.getString(R.string.pvr_appoint_confirm_wait_play, timeout));
                    }
                    delay(1000);
                    timeout--;
                    if (timeout <= 0) {
                        break;
                    }
                }
                Log.d(TAG, "waitPlayAndRecord timeout");
                showToastInHandle(mContext.getString(R.string.pvr_appoint_confirm_wait_play_timeout));
                try {
                    obj.put(ACTION_KEY_PVR_CONFIRM_ACTION, ACTION_PVR_CONFIRM_CANCEL);
                } catch (JSONException e) {
                    e.printStackTrace();
                    Log.d(TAG, "waitPlayAndRecord JSONException = " + e.getMessage());
                }
                sendCallBackByHandle(callback , obj, scheduler);
                //hideDialogByHandle(alert);
                removeScheduledRecording(scheduler);
            }
        }).start();
    }

    private void delay(int ms) {
        try {
            Thread.currentThread();
            Thread.sleep(ms);
        } catch (InterruptedException e) {
            e.printStackTrace();
            Log.d(TAG, "delay InterruptedException = " + e.getMessage());
        }
    }

    private void startLiveTvByHandle(final ScheduledRecording scheduler) {
        if (scheduler != null && handler != null) {
            handler.post(new Runnable() {
                @Override
                public void run() {
                    Log.d(TAG, "startLiveTvByHandle send stop_luncher_play");
                    Intent intent = new Intent("droidlogic.intent.action.stop_luncher_play");
                    mContext.sendBroadcast(intent);
                }
            });
            handler.postDelayed(new Runnable() {
                @Override
                public void run() {
                    Log.d(TAG, "startLiveTvByHandle send start livetv");
                    startLiveTv(scheduler.getInputId(), scheduler.getChannelId());
                }
            }, 500);//start livetv 300ms as need to stop luncher firstly
        }
    }

    private void updateTitleByHandle(final TextView title, final String value) {
        if (title != null) {
            title.post(new Runnable() {
                @Override
                public void run() {
                    title.setText(value);
                }
            });
        }
    }

    private void hideDialogByHandle(final AlertDialog alert) {
        if (alert != null && handler != null) {
            handler.post(new Runnable() {
                @Override
                public void run() {
                    alert.dismiss();
                }
            });
        }
    }

    private void showToastInHandle(String value) {
        handler.post(new Runnable() {
            @Override
            public void run() {
                Toast.makeText(mContext, value, Toast.LENGTH_SHORT).show();
            }
        });
    }

    private void sendCallBackByHandle(final ActionCallback callback , final JSONObject obj, final ScheduledRecording scheduler) {
        if (callback != null && obj != null && handler != null) {
            handler.post(new Runnable() {
                @Override
                public void run() {
                    callback.onActionBack(obj.toString(), scheduler);
                }
            });
        }
    }

    private void sendCallBackDelayedByHandle(final ActionCallback callback , final JSONObject obj, final ScheduledRecording scheduler) {
        if (callback != null && obj != null && handler != null) {
            handler.postDelayed(new Runnable() {
                @Override
                public void run() {
                    callback.onActionBack(obj.toString(), scheduler);
                    removeScheduledRecording(scheduler);
                }
            }, 300);
        }
    }

    private void updateChannelBannerByHandle(final ScheduledRecording scheduler) {
        if (scheduler != null && handler != null) {
            handler.post(new Runnable() {
                @Override
                public void run() {
                    updateChannelBanner(scheduler);
                }
            });
        }
    }

    private void startLiveTv(String inputId, long channelId) {
        Intent intent = new Intent(TvInputManager.ACTION_SETUP_INPUTS);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        intent.putExtra("from_tv_source", true);
        intent.putExtra(TvInputInfo.EXTRA_INPUT_ID, inputId);
        intent.putExtra(DroidLogicTvUtils.EXTRA_CHANNEL_ID, channelId);
        intent.putExtra(DroidLogicTvUtils.KEY_LIVETV_PROGRAM_APPOINTED, true);
        mContext.startActivity(intent);
    }

    public interface ActionCallback {
        void onActionBack(String json, Object obj);
    }

    public boolean getPropertyBoolean(String key, boolean defValue) {
        boolean getValue = TvSingletons.getSingletons(mContext).getSystemControlManager().getPropertyBoolean(key, defValue);
        if (DEBUG) {
            Log.d(TAG, "getPropertyBoolean key = " + key + ", defValue = " + defValue + ", getValue = " + getValue);
        }
        return getValue;
    }

    private void checkSystemWakeUp() {
        PowerManager powerManager = (PowerManager) mContext.getSystemService(Context.POWER_SERVICE);
        boolean isScreenOpen = powerManager.isScreenOn();
        Log.d(TAG, "checkSystemWakeUp isScreenOpen = " + isScreenOpen);
        //Resume if the system is suspending
        if (!isScreenOpen) {
            Log.d(TAG, "checkSystemWakeUp wakeUp the android.");
            long time = SystemClock.uptimeMillis();
            wakeUp(powerManager, time);
        }
    }

    private void wakeUp(PowerManager powerManager, long time) {
         try {
             Class<?> cls = Class.forName("android.os.PowerManager");
             Method method = cls.getMethod("wakeUp", long.class);
             method.invoke(powerManager, time);
         } catch(Exception e) {
             e.printStackTrace();
             Log.d(TAG, "wakeUp Exception = " + e.getMessage());
         }
    }

    private boolean isScreenOn() {
        PowerManager powerManager = (PowerManager) mContext.getSystemService(Context.POWER_SERVICE);
        boolean isScreenOpen = powerManager.isScreenOn();
        Log.d(TAG, "isScreenOn = " + isScreenOpen);
        return isScreenOpen;
    }
}

