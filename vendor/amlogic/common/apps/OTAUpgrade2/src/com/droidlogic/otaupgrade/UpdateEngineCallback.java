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

import android.os.IUpdateEngineCallback;
import android.content.Context;
import android.content.Intent;
import android.util.Log;
import android.graphics.PixelFormat;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewGroup.LayoutParams;
import android.view.WindowManager;
import android.view.Gravity;
import android.view.Display;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.os.Handler;
import android.os.Message;
import android.util.DisplayMetrics;
import android.content.BroadcastReceiver;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
public class UpdateEngineCallback extends IUpdateEngineCallback.Stub{
    private static int STATUS_IDLE = 0;
    private static int UPDATE_AVAILABLE = 2;
    private static int DOWNLOADING = 3;
    private static int FINALIZING = 5;
    private static int UPDATED_NEED_REBOOT = 6;
    private static final String TAG = "UpdateEngineCallback";
    private static final int MSG_SHOWPROGRESS =0;
    private static final int MSG_HIDEPROGRESS =1;
    private static final int MSG_UPDATEPROCESS=2;
    private Context mContext;
    private ViewGroup rootView;
    private WindowManager mWindowManager;
    private boolean isShowing=false;
    private ProgressBar mBar;
    private Handler mHandler;
    private TextView mText;
    public UpdateEngineCallback(Context context) {
        mContext = context;
        mHandler = new Handler(mContext.getMainLooper()){
            public void dispatchMessage(Message msg) {
                switch ( msg.what ) {
                    case MSG_SHOWPROGRESS:
                    if (!isShowing)
                        showProgress();
                    break;
                    case MSG_HIDEPROGRESS:
                    if (isShowing)
                        hideProgress();
                    break;
                    case MSG_UPDATEPROCESS:
                    mBar.setProgress(msg.arg1);
                    if (mText != null) {
                        mText.setText(msg.arg1+"%");
                    }
                    break;
                }
            }
        };
    }
    @Override
    public void onStatusUpdate(int status_code, float percentage){
        if ( status_code >= UPDATE_AVAILABLE && !isShowing) {
            mHandler.sendEmptyMessage(MSG_SHOWPROGRESS);
            registNetWork();
        }
        if ( status_code == DOWNLOADING && mBar != null ) {
            Message msg = mHandler.obtainMessage(MSG_UPDATEPROCESS);
            msg.arg1 = (int) (percentage*100);
            mHandler.sendMessage(msg);
        }else if ( status_code > DOWNLOADING && isShowing ) {
            mHandler.sendEmptyMessage(MSG_HIDEPROGRESS);
        }
    }
    @Override
    public void onPayloadApplicationComplete(int error_code){
        Log.d(TAG,"onPayloadApplicationComplete"+error_code);
        mHandler.sendEmptyMessage(MSG_HIDEPROGRESS);
        mContext.unregisterReceiver(mNetWorkListener);
    }
    private void showProgress() {
        isShowing = true;
        Log.d(TAG,"showProgress");
        LayoutInflater mInflater = (LayoutInflater) mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        rootView =  (ViewGroup) mInflater.inflate(R.layout.downloading_process, null);
        rootView.setBackgroundColor(R.color.black);
        mBar = (ProgressBar)rootView.findViewById(R.id.progressBar1);
        mText = (TextView)rootView.findViewById(R.id.download_percent);
        mWindowManager = (WindowManager)mContext.getSystemService(Context.WINDOW_SERVICE);
        WindowManager.LayoutParams mWmParams = new WindowManager.LayoutParams();
        mWmParams.type = WindowManager.LayoutParams.TYPE_SYSTEM_ALERT;
        Display display = mWindowManager.getDefaultDisplay();
        DisplayMetrics displayinfo = new DisplayMetrics();
        display.getMetrics(displayinfo);

        mWmParams.y=displayinfo.heightPixels-rootView.getMeasuredHeight();
        mWmParams.x=displayinfo.widthPixels-rootView.getMeasuredWidth();
        mWmParams.width = LayoutParams.WRAP_CONTENT;
        mWmParams.height =  LayoutParams.WRAP_CONTENT;
        mWmParams.format = PixelFormat.RGBA_8888;
        mWmParams.flags = WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL|WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE
                | WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN |WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON
                | WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE;
        mWmParams.gravity = Gravity.CENTER;
        mWindowManager.addView(rootView, mWmParams);
    }
    private void registNetWork() {
         IntentFilter intentFilter = new IntentFilter(ConnectivityManager.CONNECTIVITY_ACTION);
         mContext.registerReceiver(mNetWorkListener, intentFilter);
    }
    private BroadcastReceiver mNetWorkListener = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            Log.d(TAG,"disconnect.............."+intent.getAction());
             ConnectivityManager cMgr = ( ConnectivityManager ) mContext.getSystemService ( Context.CONNECTIVITY_SERVICE );
             NetworkInfo netInfo = ( NetworkInfo ) intent.getExtras ().getParcelable(ConnectivityManager.EXTRA_NETWORK_INFO);
             if (netInfo != null) {
                if (netInfo.getState() == NetworkInfo.State.DISCONNECTED ) {
                    mHandler.sendEmptyMessage(MSG_HIDEPROGRESS);
                }

            }
        }
    };
    private void hideProgress() {
        isShowing = false;
        Log.d(TAG,"hideprogress");
        if ( rootView != null && rootView.isShown())
            mWindowManager.removeView(rootView);
    }

}
