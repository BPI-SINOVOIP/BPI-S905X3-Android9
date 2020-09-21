/******************************************************************
*
*Copyright (C) 2012 Amlogic, Inc.
*
*Licensed under the Apache License, Version 2.0 (the "License");
*you may not use this file except in compliance with the License.
*You may obtain a copy of the License at
*
*    http://www.apache.org/licenses/LICENSE-2.0
*
*Unless required by applicable law or agreed to in writing, software
*distributed under the License is distributed on an "AS IS" BASIS,
*WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*See the License for the specific language governing permissions and
*limitations under the License.
******************************************************************/
package com.droidlogic.otaupgrade;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;

import android.content.Context;
import android.content.Intent;

import android.os.Binder;
import android.os.IBinder;

import android.util.Log;

import com.amlogic.update.CheckUpdateTask;
import com.amlogic.update.DownloadUpdateTask;
import com.amlogic.update.Notifier;
import com.amlogic.update.UpdateTasks;
import com.amlogic.update.util.UpgradeInfo;
import java.util.Timer;
import java.util.TimerTask;


/**
 * @ClassName UpdateService
 * @Description TODO
 * @Date 2013-7-15
 * @Email
 * @Author
 * @Version V1.0
 */
public class UpdateService extends Service {
    /*
     * (non-Javadoc)
     *
     * @see android.app.Service#onBind(android.content.Intent)
     */
    public static final String TAG = "otaupgrade";
    public static final String KEY_START_COMMAND = "start_command";
    public static final String ACTION_CHECK = "com.android.update.check";
    public static final String ACTION_DOWNLOAD = "com.android.update.download";
    public static final String ACTION_AUTOCHECK = "com.android.update.action.autocheck";
    public static final String ACTION_DOWNLOAD_OK = "com.android.update.DOWNLOAD_OK";
    public static final String ACTION_DOWNLOAD_SUCCESS = "com.android.update.downloadsuccess";
    public static final String ACTION_DOWNLOAD_FAILED = "com.android.update.downloadfailed";
    public static final int CHECK_UPGRADE_OK = 103;
    public static final int TASK_ID_CHECKING = 101;
    public static final int TASK_ID_DOWNLOAD = 102;
    private Timer timer = null;
    private UpdateTasks mCheckingTask;
    private UpdateTasks mDownloadTask;
    private Notifier notice = new DownloadNotify();
    private Context mContext = null;
    private PrefUtils mPrefUtils;
    private TimerTask autoCheckTask = new TimerTask() {
            public void run() {
                if ((mDownloadTask.getRunningStatus() != UpdateTasks.RUNNING_STATUS_RUNNING) &&
                        (mCheckingTask.getRunningStatus() != UpdateTasks.RUNNING_STATUS_RUNNING)) {
                    mCheckingTask.start();
                }
            }
        };

    @Override
    public IBinder onBind(Intent arg0) {
        return new uiBinder();
    }

    public int onStartCommand(Intent intent, int paramInt1, int paramInt2) {
        if (intent == null) {
            return 0;
        }

        String act = intent.getAction();

        if (PrefUtils.DEBUG) {
            Log.i(TAG, "get a start cmd : " + act);
        }

        if (act.equals(ACTION_CHECK)) {
            if (PrefUtils.DEBUG) {
                Log.v(TAG, "status=" + mCheckingTask.getRunningStatus());
            }

            if (mCheckingTask.getRunningStatus() != UpdateTasks.RUNNING_STATUS_RUNNING) {
                mCheckingTask.start();
            }
        } else if (act.equals(ACTION_DOWNLOAD)) {
            if (PrefUtils.DEBUG) {
                Log.v(TAG, "status=" + mDownloadTask.getRunningStatus());
            }

            if (mDownloadTask.getRunningStatus() != UpdateTasks.RUNNING_STATUS_RUNNING) {
                mDownloadTask.start();
            }
        } else if (act.equals(ACTION_AUTOCHECK)) {
            if (timer == null) {
                timer = new Timer();
                timer.schedule(autoCheckTask, 0, 60000 * 30); //period=30min
            }
        }

        return 0;
    }

    private void initInstance() {
        if (mCheckingTask == null) {
            mCheckingTask = new CheckUpdateTask(this);
        }

        if (mDownloadTask == null) {
            mDownloadTask = new DownloadUpdateTask(this);
            ((DownloadUpdateTask) mDownloadTask).setCallbacks(mPrefUtils);
            ((DownloadUpdateTask) mDownloadTask).setNotify(notice);
        }
    }

    @Override
    public void onCreate() {
        super.onCreate();
        mContext = getBaseContext();
        mPrefUtils = new PrefUtils(mContext);
        UpgradeInfo upgrade = new UpgradeInfo(this);
        if (UpgradeInfo.firmware.equals("unknown")) {
            String url = UpdateService.this.getResources().getString(R.string.otaupdateurl);
            String version = UpdateService.this.getResources().getString(R.string.cur_version);
            upgrade.setCustomValue(version,url);
        }
        initInstance();
    }


    public class uiBinder extends Binder {
        public boolean resetTask(int taskId) {
            boolean b = false;

            switch (taskId) {
            case TASK_ID_CHECKING:
                b = mCheckingTask.reset();

                break;

            case TASK_ID_DOWNLOAD:
                b = mDownloadTask.reset();

                break;
            }

            return b;
        }

        public void setTaskPause(int taskId) {
            switch (taskId) {
            case TASK_ID_CHECKING:
                mCheckingTask.pause();

                break;

            case TASK_ID_DOWNLOAD:
                mDownloadTask.pause();

                break;
            }
        }

        public void setTaskResume(int taskId) {
            switch (taskId) {
            case TASK_ID_CHECKING:
                mCheckingTask.resume();

                break;

            case TASK_ID_DOWNLOAD:
                mDownloadTask.resume();

                break;
            }
        }

        public int getTaskRunnningStatus(int taskId) {
            int status = -1;

            switch (taskId) {
            case TASK_ID_CHECKING:
                status = mCheckingTask.getRunningStatus();

                break;

            case TASK_ID_DOWNLOAD:
                status = mDownloadTask.getRunningStatus();

                break;
            }

            return status;
        }

        public Object getTaskResult(int taskId) {
            Object result = null;

            switch (taskId) {
            case TASK_ID_CHECKING:
                result = mCheckingTask.getResult();

                break;

            case TASK_ID_DOWNLOAD:
                result = mDownloadTask.getResult();

                break;
            }

            return result;
        }

        public int getTaskErrorCode(int taskId) {
            int errorCode = -1;

            switch (taskId) {
            case TASK_ID_CHECKING:
                errorCode = mCheckingTask.getErrorCode();

                break;

            case TASK_ID_DOWNLOAD:
                errorCode = mDownloadTask.getErrorCode();

                break;
            }

            return errorCode;
        }

        public long getTaskProgress(int taskId) {
            long progress = -1;

            switch (taskId) {
            case TASK_ID_CHECKING:
                progress = mCheckingTask.getProgress();

                break;

            case TASK_ID_DOWNLOAD:
                progress = mDownloadTask.getProgress();

                break;
            }

            return progress;
        }

        public void startTask(int taskId) {
            switch (taskId) {
            case TASK_ID_CHECKING:
                Log.v(TAG, "status=" + mDownloadTask.getRunningStatus());

                if (mDownloadTask.getRunningStatus() != UpdateTasks.RUNNING_STATUS_UNSTART) {
                    mDownloadTask.reset();
                }

                if (mCheckingTask.getRunningStatus() == UpdateTasks.RUNNING_STATUS_UNSTART) {
                    mCheckingTask.start();
                }

                break;

            case TASK_ID_DOWNLOAD:

                if (mDownloadTask.getRunningStatus() != UpdateTasks.RUNNING_STATUS_RUNNING) {
                    mDownloadTask.start();
                }

                break;
            }
        }
    }

    public class DownloadNotify implements Notifier {
        /* (non-Javadoc)
         * @see com.amlogic.update.Notifier#Failednotify()
         */
        @Override
        public void Failednotify() {
        }

        /* (non-Javadoc)
         * @see com.amlogic.update.Notifier#Successnotify()
         */
        @Override
        public void Successnotify() {
            NotificationManager notificationManager = (NotificationManager) mContext.getSystemService(Context.NOTIFICATION_SERVICE);
            Intent intent = new Intent(mContext, UpdateActivity.class);
            intent.setAction(ACTION_DOWNLOAD_SUCCESS);
            PendingIntent contentItent = PendingIntent.getActivity(mContext, 0,
                    intent, 0);
            Notification.Builder notification = new Notification.Builder(mContext)
            .setTicker(mContext.getString(R.string.noti_msg))
            .setWhen(System.currentTimeMillis())
            .setContentTitle(mContext.getString(R.string.app_name))
            .setContentText(mContext.getString(R.string.noti_msg))
            .setSmallIcon(R.drawable.ic_icon)
            .setContentIntent(contentItent);

            notificationManager.notify(0, notification.getNotification());
        }
    }
}
