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
import android.content.SharedPreferences.Editor;
import android.content.Intent;
import android.os.Bundle;
import android.view.View.OnClickListener;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.Spinner;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.Window;
import android.widget.TextView;
import android.widget.ImageView;

import com.droidlogic.app.SystemControlManager;


public class keyStoneCorrectionActivity extends Activity {
       private static String TAG = "keyStoneCorrectionActivity";
       private Button controller_text;
       private Spinner mSpinner;
       private int curpos;

       private SharedPreferences pointAxisSp = null;
       private SystemControlManager mSystemControl;

       @Override
       protected void onCreate(Bundle savedInstanceState) {
             super.onCreate(savedInstanceState);
             requestWindowFeature(Window.FEATURE_NO_TITLE);
             setContentView(R.layout.controlkeystone);
             pointAxisSp = getSharedPreferences ("keyStone", Activity.MODE_PRIVATE);
             mSystemControl = SystemControlManager.getInstance();
             curpos = ReadParameter();
             mSpinner = (Spinner) findViewById(R.id.spinner1);
             String[] mItems = getResources().getStringArray(R.array.switchselect);
             Log.i(TAG,"mItems:" + mItems);
             ArrayAdapter<String> SizeAdapter=new ArrayAdapter<String>(this,android.R.layout.simple_spinner_item, mItems);
             mSpinner.setAdapter(SizeAdapter);
             mSpinner.setSelection(curpos);
             controller_text = (Button) findViewById (R.id.controller_text);
             controller_text.setOnClickListener(new OnClickListener() {
                 @Override
                 public void onClick(View v) {
                     if (curpos == 0)
                         mSystemControl.setProperty("persist.vendor.sys.hwc.enable", "0");
                     else
                         mSystemControl.setProperty("persist.vendor.sys.hwc.enable", "1");
                    StoreParameter(curpos);
                    Intent it = new Intent(keyStoneCorrectionActivity.this, keyStoneControlActivity.class);
                    startActivity (it);
                 }
             });

             mSpinner.setOnItemSelectedListener(new OnItemSelectedListener() {
                 @Override
                 public void onItemSelected(AdapterView<?> parent, View view,
                                int position, long id) {
                     curpos = position;
                     }
                 @Override
                 public void onNothingSelected(AdapterView<?> parent) {
                     // TODO Auto-generated method stub
                 }
             });
        }

       private void StoreParameter(int pos) {
           SharedPreferences DealData = keyStoneCorrectionActivity.this.getSharedPreferences("switchkeystonekeep", Activity.MODE_PRIVATE);
           Editor editor = DealData.edit();
           editor.putInt("switchKeystone", pos);
           editor.commit();
       }

	private int ReadParameter() {
           SharedPreferences DealData = keyStoneCorrectionActivity.this.getSharedPreferences("switchkeystonekeep", Activity.MODE_PRIVATE);
           return DealData.getInt("switchKeystone", 0);
       }

        @Override
        public void onDestroy() {
            super.onDestroy();
        }

    private boolean getKeystoneEnable() {
        boolean ret = mSystemControl.getPropertyBoolean("persist.vendor.sys.hwc.enable", false);
        return ret;
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
