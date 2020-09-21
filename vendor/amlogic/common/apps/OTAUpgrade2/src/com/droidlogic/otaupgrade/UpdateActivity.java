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

import android.app.Activity;
import android.app.Dialog;
import android.app.Service;

import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;

import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.Uri;

import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.StatFs;

import android.util.Log;

import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;

import android.view.View.OnClickListener;

import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

import com.amlogic.update.CheckUpdateTask;

import com.amlogic.update.CheckUpdateTask.CheckUpdateResult;

import com.amlogic.update.DownloadUpdateTask;

import com.amlogic.update.DownloadUpdateTask.DownloadResult;

import com.amlogic.update.OtaUpgradeUtils;

import com.amlogic.update.OtaUpgradeUtils.ProgressListener;

import com.amlogic.update.UpdateTasks;

import com.droidlogic.otaupgrade.UpdateService.uiBinder;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;

import java.util.HashSet;
import java.util.Set;


/**
 * @ClassName UpdateActivity
 * @Description TODO
 * @Date 2013-7-17
 * @Email
 * @Author
 * @Version V1.0
 */
public class UpdateActivity extends Activity {
    private static final String TAG = "UpdateActivity";
    public static final String WIFI_STATUS_INTENT = "android.net.conn.CONNECTIVITY_CHANGE";
    private TextView mDescription;
    private boolean sdcardFlag = true;
    private boolean networkFlag = true;
    private Button mCancel;
    private boolean enableBtn;
    private Button mCombineBtn;
    private ProgressBar mProgress;
    private ProgressBar mCheckProgress;
    private TextView mCheckingStatus;
    private TextView mPercent;
    private uiBinder mServiceBinder;
    private QueryThread[] mQueryThread = new QueryThread[2];
    private PrefUtils mPreference;
    private Handler mHandler = new Handler();
    private long mDownSize = 0;
    private Button mCheckBtn;
    private BroadcastReceiver sdcardListener = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                String action = intent.getAction();

                if (Intent.ACTION_MEDIA_REMOVED.equals(action) ||
                        Intent.ACTION_MEDIA_EJECT.equals(action) ||
                        Intent.ACTION_MEDIA_UNMOUNTED.equals(action) ||
                        Intent.ACTION_MEDIA_BAD_REMOVAL.equals(action)) {
                    if ((mServiceBinder != null) && (mCombineBtn != null) &&
                            mCombineBtn.getTag()
                                           .equals(Integer.valueOf(
                                    R.string.download_pause))) {
                        mServiceBinder.setTaskPause(UpdateService.TASK_ID_DOWNLOAD);
                        Toast.makeText(UpdateActivity.this,
                            R.string.sdcard_info, 20000).show();
                        UpdateActivity.this.finish();
                    }
                }
            }
        };

    /** -----------------Internal Class-------------------------- */
    private ServiceConnection mConn = new ServiceConnection() {
            @Override
            public void onServiceConnected(ComponentName name, IBinder service) {
                mServiceBinder = (uiBinder) service;

                Intent extrasIntent = getIntent();
                String action = extrasIntent.getAction();
                boolean b = mServiceBinder.getTaskRunnningStatus(UpdateService.TASK_ID_DOWNLOAD) == UpdateTasks.RUNNING_STATUS_UNSTART;
                long lastDownSize = mServiceBinder.getTaskProgress(UpdateService.TASK_ID_DOWNLOAD);

                if ((lastDownSize == 0) && b) {
                    mServiceBinder.startTask(UpdateService.TASK_ID_CHECKING);
                    setCheckView();
                } else {
                    setDownloadView(mPreference.getDescri());
                }

                if (UpdateService.ACTION_DOWNLOAD_FAILED.equals(action)) {
                    setDownloadView(UpdateActivity.this.getResources()
                                                       .getString(R.string.download_suc));
                    mPercent.setVisibility(View.VISIBLE);
                    mPercent.setText(R.string.download_error);
                } else if (UpdateService.ACTION_DOWNLOAD_SUCCESS.equals(action)) {
                    setDownloadView(UpdateActivity.this.getResources()
                                                       .getString(R.string.download_err));
                    mPercent.setVisibility(View.VISIBLE);
                    mPercent.setText(getString(R.string.Download_succeed));
                    mCombineBtn.setText(R.string.download_update);
                    mCombineBtn.setTag(Integer.valueOf(R.string.download_update));
                }
            }

            public void onServiceDisconnected(ComponentName name) {
            }
        };

    private BroadcastReceiver WifiReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                if (intent.getAction()
                              .equals("android.net.conn.CONNECTIVITY_CHANGE")) {

                    ConnectivityManager manager = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);

                    NetworkInfo activeInfo = manager.getActiveNetworkInfo();
                    if ( activeInfo == null|| !activeInfo.isConnected()) {
                        Toast.makeText(context, R.string.net_status_change,
                                1).show();
                        if (Integer.valueOf(R.string.download_pause).equals(mCombineBtn.getTag())) {
                            mCombineBtn
                                .setText(R.string.download_resume);
                            mCombineBtn
                                .setTag(Integer
                                        .valueOf(R.string.download_resume));
                            if (mServiceBinder != null) {
                                mServiceBinder.setTaskPause(UpdateService.TASK_ID_DOWNLOAD);
                            }
                        }
                    }else if ((activeInfo != null) && activeInfo.isConnected()&&Integer.valueOf(R.string.download_resume).equals(mCombineBtn.getTag())) {
                        mCombineBtn.setText(R.string.download_pause);
                        mCombineBtn.setTag(Integer
                                        .valueOf(R.string.download_pause));
                        if (mServiceBinder != null) {
                            mServiceBinder.setTaskResume(UpdateService.TASK_ID_DOWNLOAD);
                        }
                    }
                }
            }
        };

    /** ------------------------------------------------------ */
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        Intent mIntent = new Intent(this, UpdateService.class);
        bindService(mIntent, mConn, Service.BIND_AUTO_CREATE);
        mPreference = new PrefUtils(this);
        mQueryThread[0] = new QueryCheck();
        mQueryThread[1] = new QueryDownload();
        enableBtn = false;
    }

    @Override
    public void onSaveInstanceState(Bundle savedInstanceState) {
        super.onSaveInstanceState(savedInstanceState);

        if (mDescription != null) {
            savedInstanceState.putString("message",
                mDescription.getText().toString());
        }
    }

    @Override
    public void onBackPressed() {
        // super.onBackPressed();
        if ((mServiceBinder != null) &&
                (mServiceBinder.getTaskRunnningStatus(
                    UpdateService.TASK_ID_DOWNLOAD) == DownloadUpdateTask.RUNNING_STATUS_RUNNING)) {
            Intent i = new Intent(Intent.ACTION_MAIN);
            i.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            i.addCategory(Intent.CATEGORY_HOME);
            startActivity(i);
        } else {
            super.onBackPressed();
        }
    }

    @Override
    public void onRestoreInstanceState(Bundle savedInstanceState) {
        super.onRestoreInstanceState(savedInstanceState);

        String message = savedInstanceState.getString("message");

        if ((message != null) && (mDescription != null)) {
            mDescription.setText(message);
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        mQueryThread[0].stop();
        mQueryThread[1].stop();
        unbindService(mConn);

        if (!sdcardFlag) {
            unregisterReceiver(sdcardListener);
        }

        if (!networkFlag) {
            unregisterReceiver(WifiReceiver);
        }
    }

    private void setCheckView() {
        setContentView(R.layout.update_checking);
        mCheckBtn = (Button) findViewById(R.id.check_update);
        mCheckBtn.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    mServiceBinder.startTask(UpdateService.TASK_ID_CHECKING);
                    mCheckProgress.setVisibility(View.VISIBLE);
                    mCheckBtn.setVisibility(View.GONE);
                }
            });
        mCheckProgress = (ProgressBar) findViewById(R.id.checking_progress);
        mCheckingStatus = (TextView) findViewById(R.id.checking_status);
        mCheckingStatus.setText(R.string.check_connecting);
        mQueryThread[0].start();
    }

    private String setStr(long size, long status) {
        String str = null;
        int block = 1024;
        long k = (size / 1024);
        // if(k > block){
        str = (status / 1024) + "KB/" + k + "KB";

        // }
        /*
         * if (k <( block * block)){ showInt = (int)(status/block/block*100.0);
         * str = (showInt/100.0)+"MB/"+(k/block)+"MB"; }
         */
        return str;
    }

    private boolean handleSDcardSituation() {
        boolean isScriptAsk = mPreference.getScriptAsk();

        String filePath = mPreference.getUpdatePath();
        File saveFilePath = null;
        long freeSpace = 0L;
        mDownSize = mPreference.getFileSize();
        if ( filePath == null ) {return false;}
        if ( isScriptAsk ) {
            saveFilePath = new File(filePath).getParentFile();

            if (saveFilePath.canWrite()) {
                freeSpace = saveFilePath.getFreeSpace();

                if ((mDownSize > 0) &&
                        (mDownSize > freeSpace)) {
                    Toast.makeText(UpdateActivity.this,
                        R.string.capacty_not_enough, Toast.LENGTH_LONG).show();
                    return false;
                } else {
                    if (sdcardFlag && isScriptAsk) {
                        IntentFilter intentFilter = new IntentFilter();
                        intentFilter.addDataScheme("file");
                        intentFilter.addAction(Intent.ACTION_MEDIA_EJECT);
                        intentFilter.addAction(Intent.ACTION_MEDIA_REMOVED);
                        intentFilter.addAction(Intent.ACTION_MEDIA_UNMOUNTED);
                        intentFilter.addAction(Intent.ACTION_MEDIA_BAD_REMOVAL);
                        registerReceiver(sdcardListener, intentFilter);
                        sdcardFlag = false;
                    }
                }
            } else {
                enableBtn = true;

                Intent intent0 = new Intent(this, BadMovedSDcard.class);
                Activity activity = (Activity) this;
                startActivityForResult(intent0, 1);

                return false;
            }
        } else {
            //data test now
            saveFilePath = new File(filePath).getParentFile();
            freeSpace = saveFilePath.getFreeSpace();
            if ((mDownSize > 0) &&
                    (mDownSize > freeSpace)) {
                Toast.makeText(UpdateActivity.this,
                    R.string.data_not_enough, Toast.LENGTH_LONG).show();
                return false;
            }
        }

        if (networkFlag) {
            registerReceiver(WifiReceiver, new IntentFilter(WIFI_STATUS_INTENT));
            networkFlag = false;
        }

        return true;
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);

        if ((data != null) && (requestCode == 1)) {
            if (resultCode == BadMovedSDcard.SDCANCEL) {
                Intent startMain = new Intent(UpdateActivity.this,
                        MainActivity.class);
                startActivity(startMain);
                this.finish();
            } else if (resultCode == BadMovedSDcard.SDOK) {
                enableBtn = false;

                if (sdcardFlag && mPreference.getScriptAsk()) {
                    IntentFilter intentFilter = new IntentFilter();
                    intentFilter.addDataScheme("file");
                    intentFilter.addAction(Intent.ACTION_MEDIA_EJECT);
                    intentFilter.addAction(Intent.ACTION_MEDIA_REMOVED);
                    intentFilter.addAction(Intent.ACTION_MEDIA_UNMOUNTED);
                    intentFilter.addAction(Intent.ACTION_MEDIA_BAD_REMOVAL);
                    registerReceiver(sdcardListener, intentFilter);

                    if (PrefUtils.DEBUG) {
                        Log.d(TAG, "register sdcard listener");
                    }

                    sdcardFlag = false;
                }
            }
        }
    }

    private void setDownloadView(String desc) {
        setContentView(R.layout.update_download);
        mDescription = (TextView) findViewById(R.id.new_version_description);
        mCombineBtn = (Button) findViewById(R.id.download_pause_and_update);
        mCancel = (Button) findViewById(R.id.cancel);
        mProgress = (ProgressBar) findViewById(R.id.download_progress);
        mPercent = (TextView) findViewById(R.id.percent);
        mCombineBtn.setOnClickListener(new OnClickListener() {
                @Override
                public void onClick(View v) {
                    Object tag = v.getTag();

                    if (Integer.valueOf(R.string.download_download).equals(tag) &&
                            !enableBtn) {
                        if (handleSDcardSituation()) {
                            mServiceBinder.startTask(UpdateService.TASK_ID_DOWNLOAD);
                        }
                    } else if (Integer.valueOf(R.string.download_pause)
                                          .equals(tag) && !enableBtn) {
                        mServiceBinder.setTaskPause(UpdateService.TASK_ID_DOWNLOAD);
                    } else if (Integer.valueOf(R.string.download_resume)
                                          .equals(tag) && !enableBtn) {
                        if (handleSDcardSituation()) {
                            mServiceBinder.setTaskResume(UpdateService.TASK_ID_DOWNLOAD);
                        }
                    } else if (Integer.valueOf(R.string.download_update)
                                          .equals(tag) && !enableBtn) {
                        int UpdateMode = OtaUpgradeUtils.UPDATE_OTA;

                        if (mPreference.getScriptAsk()) {
                                UpdateMode = OtaUpgradeUtils.UPDATE_UPDATE;
                        }

                        final Dialog dlg = new Dialog(UpdateActivity.this);
                        dlg.setTitle(R.string.confirm_update);

                        LayoutInflater inflater = LayoutInflater.from(UpdateActivity.this);
                        InstallPackage dlgView = (InstallPackage) inflater.inflate(R.layout.install_ota,
                                null, false);
                        dlgView.setPackagePath(mPreference.getUpdatePath());
                        dlgView.setDelParam(true);
                        dlgView.setParamter(UpdateMode);
                        dlg.setCancelable(false);
                        dlg.setContentView(dlgView);
                        dlg.findViewById(R.id.confirm_cancel).setOnClickListener(new View.OnClickListener() {
                                @Override
                                public void onClick(View v) {
                                    dlg.dismiss();
                                }
                            });
                        dlg.show();
                    }
                }
            });

        mDownSize = mPreference.getFileSize();

        mCancel.setOnClickListener(new OnClickListener() {
                @Override
                public void onClick(View arg0) {
                    if (!mCombineBtn.getTag()
                                        .equals(Integer.valueOf(
                                    R.string.download_pause))) {
                        mServiceBinder.resetTask(UpdateService.TASK_ID_DOWNLOAD);

                        Intent startMain = new Intent(UpdateActivity.this,
                                MainActivity.class);
                        startActivity(startMain);
                        UpdateActivity.this.finish();
                    } else {
                        Toast.makeText(UpdateActivity.this,
                            R.string.stop_download_first, Toast.LENGTH_SHORT)
                             .show();
                    }
                }
            });
        mDescription.setText(desc.replace("\\n", "\n"));
        mQueryThread[1].start();
    }

    private class QueryDownload extends QueryThread {
        long lastProgress = -1;

        protected void loop() {
            int status = mServiceBinder.getTaskRunnningStatus(UpdateService.TASK_ID_DOWNLOAD);

            switch (status) {
            case DownloadUpdateTask.RUNNING_STATUS_UNSTART:
                mHandler.post(new Runnable() {
                        @Override
                        public void run() {
                            mPercent.setVisibility(View.GONE);
                            mCombineBtn.setText(R.string.download_download);
                            mCombineBtn.setTag(Integer.valueOf(
                                    R.string.download_download));
                        }
                    });


                break;

            case DownloadUpdateTask.RUNNING_STATUS_RUNNING:
                mHandler.post(new Runnable() {
                        @Override
                        public void run() {
                            mCombineBtn.setText(R.string.download_pause);
                            mCombineBtn.setTag(Integer.valueOf(
                                    R.string.download_pause));
                        }
                    });


                long progress = mServiceBinder.getTaskProgress(UpdateService.TASK_ID_DOWNLOAD);

                if (lastProgress < progress) {
                    lastProgress = progress;
                    mHandler.post(new Runnable() {
                            @Override
                            public void run() {
                                if (PrefUtils.DEBUG) {
                                    Log.d(TAG,
                                        "progress fileSize:" + mDownSize +
                                        " lastProgress:" + lastProgress);
                                }

                                if (mDownSize > 0 && lastProgress > 10*1024) {
                                    mProgress.setVisibility(View.VISIBLE);
                                    mProgress.setMax((int)(mDownSize/1024));
                                    mPercent.setVisibility(View.VISIBLE);

                                    if (lastProgress < mDownSize) {
                                        mPercent.setText(setStr(mDownSize,
                                                lastProgress));
                                    }

                                    if (lastProgress == mDownSize) {
                                        mPercent.setText(getString(
                                                R.string.Download_succeed));
                                    }
                                } else if (lastProgress < 10*1024) {
                                    mPercent.setVisibility(View.VISIBLE);
                                    mPercent.setText(getString(
                                                R.string.scan_tip));
                                }

                                mProgress.setProgress((int)(lastProgress/1024));
                            }
                        });
                }

                break;

            case DownloadUpdateTask.RUNNING_STATUS_PAUSE:
                mHandler.post(new Runnable() {
                        @Override
                        public void run() {
                            mCombineBtn.setText(R.string.download_resume);
                            mCombineBtn.setTag(Integer.valueOf(
                                    R.string.download_resume));
                        }
                    });


                break;

            case DownloadUpdateTask.RUNNING_STATUS_FINISH:

                int errorCode = mServiceBinder.getTaskErrorCode(UpdateService.TASK_ID_DOWNLOAD);

                if (errorCode == DownloadUpdateTask.NO_ERROR) {
                    mHandler.post(new Runnable() {
                            @Override
                            public void run() {
                                mProgress.setProgress(mProgress.getMax());
                                mPercent.setVisibility(View.GONE);
                                mCombineBtn.setText(R.string.download_update);
                                mCombineBtn.setTag(Integer.valueOf(
                                        R.string.download_update));
                            }
                        });
                } else {
                    mHandler.post(new Runnable() {
                            @Override
                            public void run() {
                                Toast.makeText(UpdateActivity.this,
                                    R.string.download_error, Toast.LENGTH_SHORT)
                                     .show();
                                mServiceBinder.resetTask(UpdateService.TASK_ID_DOWNLOAD);
                                mPercent.setText(R.string.download_error);
                            }
                        });
                }

                stop();

                break;
            }
        }
    }

    private class QueryCheck extends QueryThread {
        protected void loop() {
            int status = mServiceBinder.getTaskRunnningStatus(UpdateService.TASK_ID_CHECKING);

            switch (status) {
            case CheckUpdateTask.RUNNING_STATUS_UNSTART:
                stop();

                break;

            case CheckUpdateTask.RUNNING_STATUS_RUNNING:
                mHandler.post(new Runnable() {
                        @Override
                        public void run() {
                            mCheckingStatus.setText(R.string.check_now);
                        }
                    });


                break;

            case CheckUpdateTask.RUNNING_STATUS_FINISH:

                int errorCode = mServiceBinder.getTaskErrorCode(UpdateService.TASK_ID_CHECKING);
                measureError(errorCode);

                if ((mCheckBtn != null) && (mCheckBtn.VISIBLE == View.GONE)) {
                    mCheckBtn.setVisibility(View.VISIBLE);
                }

                break;
            }
        }

        private void measureError(int errorCode) {
            switch (errorCode) {
            case CheckUpdateTask.ERROR_UNDISCOVERY_NEW_VERSION:
                mHandler.post(new Runnable() {
                        @Override
                        public void run() {
                            mCheckProgress.setVisibility(View.INVISIBLE);
                            mCheckingStatus.setText(R.string.check_failed);
                        }
                    });


                break;

            case CheckUpdateTask.ERROR_UNKNOWN:
                mHandler.post(new Runnable() {
                        @Override
                        public void run() {
                            mCheckProgress.setVisibility(View.INVISIBLE);
                            mCheckingStatus.setText(R.string.check_unknown);
                        }
                    });


                break;

            case CheckUpdateTask.NO_ERROR:
                mHandler.post(new Runnable() {
                        @Override
                        public void run() {
                            CheckUpdateResult result = (CheckUpdateResult) mServiceBinder.getTaskResult(UpdateService.TASK_ID_CHECKING);
                            mPreference.saveFileSize(result.getMemoryAsk());
                            mPreference.setScriptAsk(result.ismIsScriptAsk());
                            mPreference.setUpdatePath(result.getmUpdateFileName());

                            String desc = result.getmUpdateDescript();
                            mPreference.setDescrib(desc);
                            setDownloadView(desc);
                        }
                    });


                stop();

                break;

            case CheckUpdateTask.ERROR_NETWORK_UNAVAIBLE:
                mHandler.post(new Runnable() {
                        @Override
                        public void run() {
                            mCheckingStatus.setText(R.string.net_error);
                        }
                    });


                break;
            }

            mServiceBinder.resetTask(UpdateService.TASK_ID_CHECKING);
        }
    }

    class QueryThread implements Runnable {
        private boolean mStop = false;

        public void start() {
            mStop = false;
            new Thread(this).start();
        }

        @Override
        public void run() {
            while (!mStop) {
                try {
                    Thread.sleep(500);
                } catch (Exception e) {
                    e.printStackTrace();
                }

                loop();
            }
        }

        public void stop() {
            mStop = true;
        }

        protected void loop() {
        }
    }
}
