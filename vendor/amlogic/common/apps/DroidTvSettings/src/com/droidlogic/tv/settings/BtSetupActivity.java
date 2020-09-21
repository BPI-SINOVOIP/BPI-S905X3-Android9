/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC BtSetupActivity
 */

package com.droidlogic.tv.settings;

import java.lang.reflect.Method;
import java.util.Iterator;
import java.util.Set;
import java.util.Timer;
import java.util.TimerTask;

import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothProfile;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.SystemProperties;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;

public class BtSetupActivity extends Activity implements BluetoothDevicePairer.EventListener{
    private static final String TAG = "MainActivity";
    private TextView mTextStatus,eTextStatus;
    private ImageView mTV, mStatus, mRemote;
    private Button mBottonSkip;
    private BluetoothDevicePairer mBluetoothPairer;
    private int mPreviousStatus = BluetoothDevicePairer.STATUS_NONE;
    private Context mContext;
    private  Handler mMsgHandler;
    private static final String NAME_NONE =  "NONE";
    private static final String STR_TIP =  "Press the <BACK> button and the <HOME>/<OK> button for 6 seconds,Go in pairing mode";
    private static final String STR_TIP_EXIT = "Press the <BACK> button or the <EXIT> button exit";
    private static final String STR_BONDFAIL = "   bond fail";
    private static final String STR_CONFAIL  = "   connect fail";
    private static final String STR_SUCESS = "  paired successfully.";
    private static final String STR_ERROR = "  NO BLUETOOTH INSIDE";
    private static final String STR_PAIR = " pairing";
    private static final int DONE_MESSAGE_TIMEOUT = 3000;

    private static final int MSG_UPDATA_VIEW = 1;
    private static final int MSG_FINISH = 2;

   @Override
   protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        setContentView(R.layout.activity_main);
        mContext = this;

        mTextStatus = (TextView) findViewById(R.id.TiptextView);
        eTextStatus = (TextView) findViewById(R.id.ExittextView);
        mTextStatus.setText(STR_TIP);
        eTextStatus.setText(STR_TIP_EXIT);
        mBluetoothPairer = new BluetoothDevicePairer(this, this);

        mMsgHandler = new Handler() {
            @Override
            public void handleMessage(Message msg) {
                switch (msg.what) {
                    case MSG_UPDATA_VIEW:
                        break;
                    case MSG_FINISH:
                        FinishActivity();
                        break;
                    default:
                        Log.d(TAG, "No handler case available for message: " + msg.what);
                }
            }
          };
     }


    @Override
    public void onStart() {
        super.onStart();
        Log.d(TAG, "activity start");
        mBluetoothPairer.start();
    }

    @Override
    public void onBackPressed() {
        Log.d(TAG, "onBackPressed()");
        RemoveActivity();
        RemoveReceiver();
        super.onBackPressed();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        Log.d(TAG, "activity destroy");
        if (mBluetoothPairer != null) {
            mBluetoothPairer.stop();
            mBluetoothPairer.setListener(null);
            mBluetoothPairer.scanner.quit();
            mBluetoothPairer = null;
        }
    }


   @Override
   public void statusChanged(){
        if (mBluetoothPairer == null) return;
        int status = mBluetoothPairer.getStatus();
        int oldStatus = mPreviousStatus;
        mPreviousStatus = status;
       String RemoteName = mBluetoothPairer.getTargetDevice() == null ? NAME_NONE :
                mBluetoothPairer.getTargetDevice().getName();

       switch (status) {
            case BluetoothDevicePairer.STATUS_NONE:
                break;
            case BluetoothDevicePairer.STATUS_SCANNING:
               mTextStatus.setText(STR_TIP);
                break;
            case BluetoothDevicePairer.STATUS_BONDFAIL:
              mTextStatus.setText( RemoteName + STR_BONDFAIL);
                break;
            case BluetoothDevicePairer.STATUS_CONNECTFAIL:
               mTextStatus.setText( RemoteName + STR_CONFAIL);
                break;
            case BluetoothDevicePairer.STATUS_CONNECTED:
                mTextStatus.setText(RemoteName + STR_SUCESS);
                mMsgHandler.sendEmptyMessageDelayed(MSG_FINISH,DONE_MESSAGE_TIMEOUT);
                 break;
            case BluetoothDevicePairer.STATUS_FINDED:
                mTextStatus.setText(RemoteName + STR_PAIR);
                break;
            case BluetoothDevicePairer.STATUS_ERROR:
                mTextStatus.setText( STR_ERROR);
                break;
        }

    }

    private void RemoveActivity() {

        Intent data=null;
        setResult(1,data);
        Log.d(TAG, "RemoveActivity SUCCESS");
    }

    private void RemoveReceiver() {
        PackageManager pm = mContext.getPackageManager();
        ComponentName name = new ComponentName(mContext, BluetoothAutoPairReceiver.class);
        pm.setComponentEnabledSetting(name, PackageManager.COMPONENT_ENABLED_STATE_DISABLED,
        PackageManager.DONT_KILL_APP);
    }

    private void FinishActivity(){
        RemoveActivity();
        RemoveReceiver();
        finish();
   }
}
