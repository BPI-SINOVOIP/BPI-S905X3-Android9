/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description: java file
*/
package com.droidlogic.keystone;
import android.app.Activity;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.os.Bundle;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.ViewGroup.LayoutParams;
import android.view.Window;
import android.widget.TextView;
import android.widget.ImageView;


import android.graphics.Bitmap;
import android.graphics.BitmapFactory;


public class keyStoneControlActivity extends Activity {
       private static String TAG = "keyStoneControlActivity";
       private TextView warningTv = null;
       private TextView tvNumUp = null;
       private TextView tvNumDown = null;
       private ImageView animation = null;

       private Bitmap animationBitmap = null;

       private int animationWidth;
       private int animationHeight;

       private int screenWidth;
       private int screenHeight;

       private float strokeWidth = 10.0f;

       private CircleView circleView;

       private SharedPreferences pointAxisSp = null;


       @Override
       protected void onCreate(Bundle savedInstanceState) {
             super.onCreate(savedInstanceState);
             requestWindowFeature(Window.FEATURE_NO_TITLE);
             setContentView(R.layout.activity_main);
             pointAxisSp = getSharedPreferences ("keyStone", Activity.MODE_PRIVATE);
             animationBitmap = BitmapFactory.decodeResource(getResources(), R.drawable.animation);
             circleView = (CircleView) findViewById (R.id.circle);
             setPointState();
             setKeyStoneAxis();
        }

        @Override
        public void onDestroy() {
            super.onDestroy();
            putSpStr("keyStonePointState",getPointState());
            putSpStr("keyStoneAxis",getKeyStoneAxis());
        }

        private void putSpStr (String pointAxis, String value) {
            if (pointAxisSp != null) {
                pointAxisSp.edit()
                .putString (pointAxis, value)
                .commit();
            }
        }

        private void putSpStr (String pointState, int para) {
            if (pointAxisSp != null) {
                pointAxisSp.edit()
                .putInt (pointState, para)
                .commit();
            }
        }

        public String getKeyStoneAxis () {
            return circleView.getAngleAxisDebug();
        }

        public int getPointState () {
            return circleView.getPointState();
        }

        public void setPointState () {
            int para = -1;
            if (pointAxisSp != null) {
                para = pointAxisSp.getInt ("keyStonePointState", -1);
                Log.i(TAG,"[setPointState]para:" + para);
                if (para != -1) {
                    circleView.setPointState(para);
                }
            }
        }

        public void setKeyStoneAxis () {
            String str = null;
            if (pointAxisSp != null) {
                str = pointAxisSp.getString ("keyStoneAxis", null);
                Log.i(TAG,"[setKeyStoneAxis]str:" + str);
                if (str != null) {
                    circleView.setAngleAxis(str);
                }
            }
        }

     @Override
     public boolean onKeyDown(int keyCode, KeyEvent event) {
        //Log.i(TAG, "[onKeyDown]keyCode:"+keyCode);
         if (keyCode == KeyEvent.KEYCODE_DPAD_UP) {

         } else if (keyCode == KeyEvent.KEYCODE_DPAD_DOWN) {

         } else if (keyCode == KeyEvent.KEYCODE_DPAD_LEFT) {
             //circleView.setVisibility(View.GONE);
         } else if (keyCode == KeyEvent.KEYCODE_DPAD_RIGHT) {
             //circleView.setVisibility(View.VISIBLE);
         }
         return super.onKeyDown(keyCode, event);
     }


}
