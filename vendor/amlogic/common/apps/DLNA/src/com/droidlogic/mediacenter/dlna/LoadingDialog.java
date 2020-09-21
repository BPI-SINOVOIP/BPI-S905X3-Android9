/******************************************************************
*
*Copyright (C) 2016  Amlogic, Inc.
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
package com.droidlogic.mediacenter.dlna;

import java.util.Timer;
import java.util.TimerTask;

import com.droidlogic.mediacenter.R;
import android.app.Activity;
import android.app.ActivityManager;
import android.app.Dialog;
import android.content.Context;
import android.content.ComponentName;
import android.graphics.drawable.AnimationDrawable;
import android.os.Handler;
import android.os.Message;
import android.util.TypedValue;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.WindowManager;
import android.view.WindowManager.LayoutParams;

import android.widget.ImageView;
import android.widget.TextView;
import org.cybergarage.util.Debug;
/**
 * @ClassName LoadingDialog
 * @Description TODO
 * @Date 2013-8-27
 * @Email
 * @Author
 * @Version V1.0
 */
public class LoadingDialog extends Dialog {
        public static final int   TYPE_LOADING    = 0;
        public static final int   TYPE_ERROR      = 1;
        public static final int   TYPE_INIT       = 2;
        public static final int   TYPE_EXIT_TIMER = 3;
        protected static final String TAG = "LoadingDialog";

        private static final int COUNT_TIMER = 1;
        private TextView          mTvShow         = null;
        private int mCountTime = 10;
        private AnimationDrawable anim_loading    = null;
        private int type = TYPE_LOADING;
        public LoadingDialog ( Context context, int type, String mtitle ) {
            super ( context, R.style.theme_dialog_loading );
            WindowManager.LayoutParams params = getWindow().getAttributes();
            params.flags |= LayoutParams.FLAG_NOT_FOCUSABLE | LayoutParams.FLAG_NOT_TOUCHABLE;
            getWindow().setAttributes ( params );
            this.setCanceledOnTouchOutside ( true );
            if ( type == TYPE_LOADING ) {
                setContentView ( R.layout.loading );
                TextView tx = ( TextView ) findViewById ( R.id.tx );
                tx.setText ( mtitle );
                ImageView img = ( ImageView ) findViewById ( R.id.anim );
                anim_loading = ( AnimationDrawable ) img.getDrawable();
                anim_loading.start();
                // setCancelable(false);
            } else if ( type == TYPE_ERROR ) {
                setContentView ( R.layout.dialog_error );
                mTvShow = ( TextView ) findViewById ( R.id.tx );
                mTvShow.setText ( mtitle );
            } else if ( type == TYPE_EXIT_TIMER ) {
                setContentView ( R.layout.dialog_error );
                mTvShow = ( TextView ) findViewById ( R.id.tx );
                mTvShow.setText ( "" + mCountTime );
                mHandler.sendEmptyMessageDelayed ( COUNT_TIMER, 1000 );
            }
        }
        private Handler mHandler = new Handler() {

            @Override
            public void handleMessage ( Message msg ) {
                switch ( msg.what ) {
                    case COUNT_TIMER:
                        showTimer();
                }
            }
        };

        public String getTopActivity ( Context cxt ) {
            ActivityManager mactivitymanager = ( ActivityManager ) cxt.getSystemService ( Activity.ACTIVITY_SERVICE );
            ComponentName cn = mactivitymanager.getRunningTasks ( 1 ).get ( 0 ).topActivity;
            return cn.getClassName();
        }

        public int getCountNum() {
            Debug.d ( TAG, "Type:" + type + " mCountTime:" + mCountTime );
            return mCountTime;
        }
        public void setCountNum ( int num ) {
            mCountTime = num;
        }
        private void showTimer() {
            if ( mCountTime == 0 ) {
                LoadingDialog.this.dismiss();
            } else {
                mCountTime--;
                mTvShow.setText ( "" + mCountTime );
                mHandler.sendEmptyMessageDelayed ( COUNT_TIMER, 1000 );
            }
        }
        public void stopAnim() {
            if ( anim_loading != null ) {
                anim_loading.stop();
            }
        }

        @Override
        protected void onStop() {
            super.onStop();
        }

}
